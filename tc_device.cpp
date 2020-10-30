#include    "tc_device.h"
#include    <stdio.h>
#include    <iostream>

#if        defined(_WIN64)
#include 	<locale>
#include 	<codecvt>
#include 	<intrin.h>
#include 	<winsock2.h>
#include 	<IPHlpApi.h>
#include 	<WinInet.h>
#pragma 	comment(lib, "wininet.lib")
#pragma 	comment(lib, "ws2_32.lib")
#include 	"HardDriveSerialNumer.h"

#if 		defined(_WIN32)
#include 	<Windows.h>
#include 	<WinBase.h>
#endif

#include 	<comdef.h>
#include 	<Wbemidl.h>
#pragma 	comment(lib, "wbemuuid.lib")

#else

#include    "util/tc_split.h"
#include    "util/tc_common.h"
#include    <cstring>
#include    <stdexcept>
#include    <limits>
#include    <unistd.h>         /* gethostname */
#include    <tuple>
#include    <algorithm>
#include    <unistd.h>
#include    <fcntl.h>
#include    <linux/hdreg.h>
#include    <sys/ioctl.h>
#include    <sys/types.h>
#include    <sys/socket.h>
#include    <sys/sysctl.h>
#include    <sys/ioctl.h>
#include    <netinet/in.h>
#include    <arpa/inet.h>
#include    <net/if.h>
#include    <net/if_arp.h>
#include    <netinet/ip.h>
#endif

namespace taf
{

#if defined(_WIN64) || defined(_WIN32)
std::wstring utf8_to_wstring(const std::string& str)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > strCnv;
	return strCnv.from_bytes(str);
}

std::string wstring_to_utf8(const std::wstring& str)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t> > strCnv;
	return strCnv.to_bytes(str);
}

//字符串格式化函数
std::string format(const char* fmt, ...)
{
	std::string strResult = "";
	if (NULL != fmt)
	{
		va_list marker = NULL;
		va_start(marker, fmt);                            //初始化变量参数
		size_t nLength = _vscprintf(fmt, marker) + 1;    //获取格式化字符串长度
		std::vector<char> vBuffer(nLength, '\0');        //创建用于存储格式化字符串的字符数组
		int nWritten = _vsnprintf_s(&vBuffer[0], vBuffer.size(), nLength, fmt, marker);
		if (nWritten > 0)
		{
			strResult = &vBuffer[0];
		}
		va_end(marker);                                    //重置变量参数
	}
	return strResult;
}

void getcpuidex(unsigned int* CPUInfo, unsigned int InfoType, unsigned int ECXValue)
{
#if defined(_MSC_VER) // MSVC  
#if defined(_WIN64) // 64位下不支持内联汇编. 1600: VS2010, 据说VC2008 SP1之后才支持__cpuidex.  
	__cpuidex((int*)(void*)CPUInfo, (int)InfoType, (int)ECXValue);
#else
	if (NULL == CPUInfo)
		return;
	_asm {
		// load. 读取参数到寄存器.
		mov edi, CPUInfo;
		mov eax, InfoType;
		mov ecx, ECXValue;
		// CPUID
		cpuid;
		// save. 将寄存器保存到CPUInfo
		mov[edi], eax;
		mov[edi + 4], ebx;
		mov[edi + 8], ecx;
		mov[edi + 12], edx;
	}
#endif
#endif
}

void getcpuid(unsigned int* CPUInfo, unsigned int InfoType)
{
#if defined(__GNUC__)// GCC  
	__cpuid(InfoType, CPUInfo[0], CPUInfo[1], CPUInfo[2], CPUInfo[3]);
#elif defined(_MSC_VER)// MSVC  
#if _MSC_VER >= 1400 //VC2005才支持__cpuid  
	__cpuid((int*)(void*)CPUInfo, (int)(InfoType));
#else //其他使用getcpuidex  
	getcpuidex(CPUInfo, InfoType, 0);
#endif
#endif
}

void get_cpuId(std::string& str_cpu)
{
	char pCpuId[32] = "";
	int dwBuf[4];
	getcpuid((unsigned int*)dwBuf, 1);
	sprintf(pCpuId, "%08X", dwBuf[3]);
	sprintf(pCpuId + 8, "%08X", dwBuf[0]);
	str_cpu = pCpuId;
}

#elif defined(__linux)

static bool get_cpuId(std::string& cpu_id)
{
	cpu_id.clear();

	unsigned int s1 = 0;
	unsigned int s2 = 0;
	asm volatile
	(
	"movl $0x01, %%eax; \n\t"
	"xorl %%edx, %%edx; \n\t"
	"cpuid; \n\t"
	"movl %%edx, %0; \n\t"
	"movl %%eax, %1; \n\t"
	: "=m"(s1), "=m"(s2)
	);

	if (0 == s1 && 0 == s2)
	{
		throw std::logic_error("get cpuid failed!!!");
	}

	char cpu[32] = { 0 };
	snprintf(cpu, sizeof(cpu), "%08X%08X", htonl(s2), htonl(s1));
	std::string(cpu).swap(cpu_id);
	return (true);
}

char* getName(char* name, char* p)
{
	while (isspace(*p))
		p++;
	while (*p)
	{
		if (isspace(*p))
			break;
		if (*p == ':')
		{    /* could be an alias */
			char* dot = p, * dotname = name;
			*name++ = *p++;
			while (isdigit(*p))
				*name++ = *p++;
			if (*p != ':')
			{    /* it wasn't, backup */
				p = dot;
				name = dotname;
			}
			if (*p == '\0')
				return NULL;
			p++;
			break;
		}
		*name++ = *p++;
	}
	*name++ = '\0';
	return p;
}

std::string getIpFromEthx(const std::string& interface, std::string& macaddr)
{
	if (interface.size() == 0)
	{
		throw std::logic_error("input the eroor interface");
	}

	struct sockaddr_in* addr;
	struct ifreq ifr;
	int sockfd;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ - 1);
	if (ioctl(sockfd, SIOCGIFADDR, &ifr) == -1)
	{
		return std::numeric_limits<std::string>::quiet_NaN();
	}
	addr = (struct sockaddr_in*)&(ifr.ifr_addr);
	std::string ip(inet_ntoa(addr->sin_addr));

	if (ioctl(sockfd, SIOCGIFHWADDR, &ifr) == -1)
	{
		return std::numeric_limits<std::string>::quiet_NaN();
	}
	unsigned char* mac;
	mac = (unsigned char*)(ifr.ifr_hwaddr.sa_data);
	char mac_addr[32] = {};
	sprintf(mac_addr, "%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\0", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	macaddr.assign(mac_addr);
	return ip;
}

#endif

std::vector<std::string> TC_Device::getLocalHosts()
{
	static std::vector<std::string> hosts;
	if (!hosts.empty())
	{
		return hosts;
	}

	auto net_cards = getAllNetCard();
	if (net_cards.empty())
	{
		return hosts;
	}

	for (auto& item : net_cards)
	{
		hosts.push_back(item.second.str_ip_);
	}
	return hosts;
}

std::unordered_map<std::string, taf::MAC_INFO> TC_Device::getAllNetCard()
{
	static std::unordered_map<std::string, taf::MAC_INFO> interface_ip_mac;
	if (!interface_ip_mac.empty())
	{
		return interface_ip_mac;
	}

	//TODO windows
#if defined(_WIN64) || defined(_WIN32)
	WORD wVersionRequested = MAKEWORD(2, 2);

	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
		return interface_ip_mac;

	char local[255] = { 0 };
	gethostname(local, sizeof(local));
	hostent* ph = gethostbyname(local);
	if (ph == NULL)
		return hosts;
	in_addr addr;
	if (ph->h_addrtype == AF_INET)
	{
		int i = 0;
		while (ph->h_addr_list[i] != 0)
		{
			addr.s_addr = *(u_long*)ph->h_addr_list[i++];
			hosts.emplace_back(inet_ntoa(addr));
		}
	}
	else
	{
		// unsupport AF_INET6  ...
		return ip_mac;
	}
	WSACleanup();
	return ip_mac;
#else
	FILE* fh;
	char buf[512] = { 0 };
	fh = fopen("/proc/net/dev", "r");
	if (!fh)
	{
		throw std::logic_error("get localhosts failed");
	}

	/* eat title lines */
	fgets(buf, sizeof buf, fh);
	fgets(buf, sizeof buf, fh);
	int icnt = 1;
	while (fgets(buf, sizeof buf, fh))
	{
		char name[IFNAMSIZ] = { 0 };
		getName(name, buf);
		std::string interface(name);
		if (interface == "lo")
		{
			continue;
		}
		std::string macaddr;
		auto ip = getIpFromEthx(interface, macaddr);
		if (ip == std::numeric_limits<std::string>::quiet_NaN())
		{
			continue;
		}
		taf::MAC_INFO info;
		info.str_ip_ = ip;
		info.str_mac_ = macaddr;
		info.str_name_ = interface;
		interface_ip_mac.insert(std::make_pair(interface, info));
	}
	fclose(fh);
	return interface_ip_mac;
#endif
}

std::string TC_Device::getLIP()
{
#if defined(_WIN64) || defined(_WIN32)
	static std::string localIP;
	if (!localIP.empty())
	{
		return localIP;
	}
	WORD wVersionRequested = MAKEWORD(2, 2);

	WSADATA wsaData;
	if (WSAStartup(wVersionRequested, &wsaData) != 0)
		return "";

	char local[255] = { 0 };
	gethostname(local, sizeof(local));
	hostent* ph = gethostbyname(local);
	if (ph == NULL)
		return "";

	in_addr addr;
	memcpy(&addr, ph->h_addr_list[0], sizeof(in_addr)); // 这里仅获取第一个ip

	localIP.assign(inet_ntoa(addr));

	WSACleanup();
	return localIP;
#elif defined(__linux)
	auto net_card = getAllPhysicalNetCard();
	if (!net_card.empty())
	{
		return net_card.begin()->second.str_ip_;
	}
	return "";
#else
	throw std::logic_error("not complete");
#endif
}

taf::MAC_INFO TC_Device::getFirstActiveNetCard()
{
#if defined(_WIN64) || defined(_WIN32)
	/*	static std::string strActiveMac;
		if (!strActiveMac.empty())
		{
			return strActiveMac;
		}
		MasterHardDiskSerial mhds;
		mhds.getFirstActiveMAC(strActiveMac);
		return strActiveMac;*/
#elif  defined(__linux)
	char buf_ps[128];
	std::string cmd = "ip a | grep \"state UP\" |sed -n \"1p\" | awk -F ':' '{print $2}' | sed 's/ //g'";
	FILE* ptr = NULL;

	if ((ptr = popen(cmd.c_str(), "r")) != NULL)
	{
		if (fgets(buf_ps, 128, ptr) == NULL)
		{
			std::logic_error("get all active net card failed!!!");
		}
		pclose(ptr);
		ptr = NULL;
	}
	auto interface_ip_mac = getLocalHosts();
	std::string info(buf_ps);
	std::string name = info.substr(0, info.length() - 1);
	auto tmp = getAllNetCard();
	if (tmp.find(name) != tmp.end())
	{
		return tmp.at(name);
	}
	else
	{
		return taf::MAC_INFO();
	}

#else
	throw std::logic_error("not complete");
#endif
}

taf::MAC_INFO TC_Device::getFristEthernetNetCard()
{
#if defined(_WIN64) || defined(_WIN32)
	static std::string strFristEthernetMac;
	if (!strFristEthernetMac.empty())
	{
		return strFristEthernetMac;
	}
	MasterHardDiskSerial mhds;
	mhds.getFristEthernetMac(strFristEthernetMac);
	return strFristEthernetMac;
#elif defined(__linux)
	auto net_cards = getAllPhysicalNetCard();
	if (!net_cards.empty())
	{
		return net_cards.begin()->second;
	}
	else
	{
		return taf::MAC_INFO();
	}
#else
	throw std::logic_error("not complete");
#endif
}

std::unordered_map<std::string, taf::MAC_INFO> TC_Device::getAllPhysicalNetCard()
{
	static std::unordered_map<std::string, taf::MAC_INFO> net_card;
	if (!net_card.empty())
	{
		return net_card;
	}

#if defined(_WIN64) || defined(_WIN32)
	/*	in_vecMac.clear();
		static std::vector<MAC_INFO> s_vecMac;
		if (s_vecMac.size() > 0)
		{
			in_vecMac.assign(s_vecMac.begin(), s_vecMac.end());
			return;
		}
		MasterHardDiskSerial mhds;
		mhds.getAllPhysicalMAC(s_vecMac);
		in_vecMac.assign(s_vecMac.begin(), s_vecMac.end());*/
#elif defined(__linux)

	char buf_ps[128];
	std::string cmd = "ls /sys/class/net/ |grep -v \"`ls /sys/devices/virtual/net/`\"";
	FILE* ptr = NULL;
	if ((ptr = popen(cmd.c_str(), "r")) != NULL)
	{
		if (fgets(buf_ps, 128, ptr) == NULL)
		{
			std::logic_error("get all physical mac failed!!!");
		}
		pclose(ptr);
		ptr = NULL;
	}
	auto interface_ip_mac = getLocalHosts();

	//获取到的字符串中换行符需要进行处理
	std::string info(buf_ps);
	auto interfaces = taf::TC_Common::sepstr<std::string>(info.substr(0, info.length() - 1), " ");
	interfaces.erase(std::remove(interfaces.begin(), interfaces.end(), "lo"), interfaces.end());
	auto tmp = getAllNetCard();
	for (auto& name : interfaces)
	{
		auto iter = tmp.find(name);
		if (iter != tmp.end())
		{

			net_card.insert(std::make_pair(name, iter->second));
		}
		else
		{
			continue;
		}
	}
	return net_card;

#else
	throw std::logic_error("not complete");
#endif
}

std::tuple<std::string, std::string> TC_Device::getHD()
{
#if defined(_WIN64) || defined(_WIN32)
	static std::string strSystemSerialNo;
	if (!strSystemSerialNo.empty())
	{
		return strSystemSerialNo;
	}
	MasterHardDiskSerial mhds;
	mhds.getSerialNo(strSystemSerialNo);
	return strSystemSerialNo;
#elif defined(__linux)
	char buf_ps[128];
	std::string cmd = "df -Tlh |sed -n \"2p\" |awk -F ' ' '{printf(\"%s,%s,%s\",$1,$2,$3)}'";
	FILE* ptr = NULL;
	if ((ptr = popen(cmd.c_str(), "r")) != NULL)
	{
		if (fgets(buf_ps, 128, ptr) == NULL)
		{
			std::logic_error("get Unix OS PI!!!");
		}
		pclose(ptr);
		ptr = NULL;
	}
	std::string info(buf_ps);
	auto PI = taf::TC_Common::sepstr<string>(info, ",");

	static int open_flags = O_RDONLY | O_NONBLOCK;
	static struct hd_driveid id;

	if (PI.empty())
	{
		throw std::logic_error("get HD failed");
	}

	int fd = open(PI.front().c_str(), open_flags);
	if (fd < 0)
	{
		throw std::logic_error("open the dev file failed, please checkout the existence of " + PI.front() + ", if exist, try use root right to excute!!!");
	}

	ioctl(fd, HDIO_GET_IDENTITY, &id);
	std::string model;
	std::string serial_id;
	//长度是固定的不要随意修改
	for (int i = 0; i < 40; i++)
	{
		char a = id.model[i];
		if (a == ' ')
		{
			continue;
		}
		model += a;
	}

	for (int i = 0; i < 20; i++)
	{
		char a = id.serial_no[i];
		if (a == ' ')
		{
			continue;
		}
		serial_id += a;
	}

	return std::make_tuple(model, serial_id);
#else
	//	throw std::logic_error("not complete");
#endif
}

std::string TC_Device::getPI()
{
#if defined(_WIN64) || defined(_WIN32)
	//TODO
	throw std::logic_error("not complete");
#elif defined(__linux)
	char buf_ps[128];
	std::string cmd = "df -Tlh |sed -n \"2p\" |awk -F ' ' '{printf(\"%s,%s,%s\",$1,$2,$3)}'";
	FILE* ptr = NULL;
	if ((ptr = popen(cmd.c_str(), "r")) != NULL)
	{
		if (fgets(buf_ps, 128, ptr) == NULL)
		{
			std::logic_error("get Unix OS PI!!!");
		}
		pclose(ptr);
		ptr = NULL;
	}
	std::string info(buf_ps);
	return info;
#else
	throw std::logic_error("not complete");

#endif
}

std::string TC_Device::getCPU()
{
	static std::string cpuid;
	if (!cpuid.empty())
	{
		return cpuid;
	}
#if defined(_WIN64) || defined(_WIN32)
	get_cpuId(cpuid);
	return cpuid;
#elif defined(__linux)
	get_cpuId(cpuid);
	return cpuid;
#else
	throw std::logic_error("not complete");
#endif
}

#if !defined(_WIN64) && !defined(_WIN32)

std::tuple<std::string, std::string> TC_Device::getUnixOsInfo()
{
	char buf_ps[128];
	std::string cmd = "uname -a|awk -F ' ' '{printf(\"%s %s\",$1,$3)}'";
	FILE* ptr = NULL;
	if ((ptr = popen(cmd.c_str(), "r")) != NULL)
	{
		if (fgets(buf_ps, 128, ptr) == NULL)
		{
			std::logic_error("get Unix OS info failed!!!");
		}
		pclose(ptr);
		ptr = NULL;
	}
	std::string info(buf_ps);
	auto pos = info.find(' ');
	if (pos == std::string::npos)
	{
		throw std::logic_error("get Unix OS info failed!!!");
	}
	auto osname = info.substr(0, pos);
	auto osversion = info.substr(pos + 1);

	return std::make_tuple(osname, osversion);
}

#endif

std::string TC_Device::getPCN()
{
	std::string osname = "";
#if defined(_WIN64) || defined(_WIN32)
	SYSTEM_INFO info;        //用SYSTEM_INFO结构判断64位AMD处理器
	GetSystemInfo(&info);    //调用getSystemInfo函数填充结构
	OSVERSIONINFOEX os;
	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

	osname = "unknown OperatingSystem.";

	if (GetVersionEx((OSVERSIONINFO*)&os))
	{
		//下面根据版本信息判断操作系统名称
		switch (os.dwMajorVersion)//判断主版本号
		{
		case 4:
			switch (os.dwMinorVersion)//判断次版本号
			{
			case 0:
				if (os.dwPlatformId == VER_PLATFORM_WIN32_NT)
					osname = "Microsoft Windows NT 4.0"; //1996年7月发布
				else if (os.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
					osname = ("Microsoft Windows 95");
				break;
			case 10:
				osname = ("Microsoft Windows 98");
				break;
			case 90:
				osname = ("Microsoft Windows Me");
				break;
			}
			break;

		case 5:
			switch (os.dwMinorVersion)    //再比较dwMinorVersion的值
			{
			case 0:
				osname = ("Microsoft Windows 2000");//1999年12月发布
				break;

			case 1:
				osname = ("Microsoft Windows XP");//2001年8月发布
				break;

			case 2:
				if (os.wProductType == VER_NT_WORKSTATION
				&& info.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64)
				{
					osname = ("Microsoft Windows XP Professional x64 Edition");
				}
				else if (GetSystemMetrics(SM_SERVERR2) == 0)
					osname = ("Microsoft Windows Server 2003");//2003年3月发布
				else if (GetSystemMetrics(SM_SERVERR2) != 0)
					osname = ("Microsoft Windows Server 2003 R2");
				break;
			}
			break;

		case 6:
			switch (os.dwMinorVersion)
			{
			case 0:
				if (os.wProductType == VER_NT_WORKSTATION)
					osname = ("Microsoft Windows Vista");
				else
					osname = ("Microsoft Windows Server 2008");//服务器版本
				break;
			case 1:
				if (os.wProductType == VER_NT_WORKSTATION)
					osname = ("Microsoft Windows 7");
				else
					osname = ("Microsoft Windows Server 2008 R2");
				break;
			case 2:
				if (os.wProductType != VER_NT_WORKSTATION)
					osname = "Windows Server 2012";
				else
					osname = "Windows 8";
				break;
			case 3:
				if (os.wProductType == VER_NT_WORKSTATION)
					osname = "Windows 8.1";
				else
					osname = "Windows Server 2012 R2";
				break;
			}
			break;
		case 10:
			switch (os.dwMinorVersion)
			{
			case 0:
				if (os.wProductType == VER_NT_WORKSTATION)
					osname = "Windows 10";
				else
					osname = "Windows Server 2016";
				break;
			default:
				break;
			}
			break;
		}
	}
	return osname;
#else
	auto osinfo = getUnixOsInfo();
	return std::get<0>(osinfo);
#endif
}

std::string TC_Device::getOSV()
{
#if defined(_WIN64) || defined(_WIN32)
	static std::string str_osv;
	if (!str_osv.empty())
	{
		return str_osv;
	}
	OSVERSIONINFO Version;
	Version.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&Version);
	str_osv = std::to_string(Version.dwMajorVersion) + "." + std::to_string(Version.dwMinorVersion) + "."
	+ std::to_string(Version.dwBuildNumber);
	return str_osv;
#else
	auto osinfo = getUnixOsInfo();
	return std::get<1>(osinfo);
#endif
}

std::string TC_Device::getIMEI()
{
#if defined(_WIN64) || defined(_WIN32)
	static std::string strSystemSerialNo;
	if (!strSystemSerialNo.empty())
	{
		return strSystemSerialNo;
	}
	MasterHardDiskSerial mhds;
	mhds.getSerialNo(strSystemSerialNo);
	return strSystemSerialNo;
#else
	return get<1>(getHD());
	throw std::logic_error("not complete");
#endif
}

std::string TC_Device::getVOL()
{
#if defined(_WIN64) || defined(_WIN32)
	static std::string str_vol;
	if (!str_vol.empty())
	{
		return str_vol;
	}
	char szTmp[MAX_PATH] = { 0 };
	GetSystemDirectoryA(szTmp, MAX_PATH);
	str_vol = szTmp[0];
	return str_vol;
#else
	//暂时不支持
	return "";
#endif
}

// 参考链接:
// https://docs.microsoft.com/en-us/windows/win32/wmisdk/example--getting-wmi-data-from-the-local-computer
int TC_Device::WmiQuery(const std::string& key, std::string& val)
{
#if defined(_WIN64) || defined(_WIN32)
	HRESULT hres;

	// Step 1: --------------------------------------------------
	// Initialize COM. ------------------------------------------

	hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres))
	{
//	        std::cout << "Failed to initialize COM library. Error code = 0x"
//	            << hex << hres;
		return 1;                  // Program has failed.
	}

	/*
	// Step 2: --------------------------------------------------
	// Set general COM security levels --------------------------

	hres =  CoInitializeSecurity(
		NULL,
		-1,                          // COM authentication
		NULL,                        // Authentication services
		NULL,                        // Reserved
		RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication
		RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation
		NULL,                        // Authentication info
		EOAC_NONE,                   // Additional capabilities
		NULL                         // Reserved
		);


	if (FAILED(hres))
	{
		FW_LOG_INFOA << "Failed to initialize security. Error code = 0x"
			<< hex << hres;
		CoUninitialize();
		return 1;                    // Program has failed.
	}*/

	// Step 3: ---------------------------------------------------
	// Obtain the initial locator to WMI -------------------------

	IWbemLocator* pLoc = NULL;

	hres = CoCreateInstance(
	CLSID_WbemLocator,
	0,
	CLSCTX_INPROC_SERVER,
	IID_IWbemLocator, (LPVOID*)&pLoc);

	if (FAILED(hres))
	{
//	        FW_LOG_INFOA << "Failed to create IWbemLocator object."
//	            << " Err code = 0x"
//	            << hex << hres;
		CoUninitialize();
		return 1;                 // Program has failed.
	}

	// Step 4: -----------------------------------------------------
	// Connect to WMI through the IWbemLocator::ConnectServer method

	IWbemServices* pSvc = NULL;

	// Connect to the root\cimv2 namespace with
	// the current user and obtain pointer pSvc
	// to make IWbemServices calls.
	hres = pLoc->ConnectServer(
	_bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
	NULL,                    // User name. NULL = current user
	NULL,                    // User password. NULL = current
	0,                       // Locale. NULL indicates current
	NULL,                    // Security flags.
	0,                       // Authority (for example, Kerberos)
	0,                       // Context object
	&pSvc                    // pointer to IWbemServices proxy
	);

	if (FAILED(hres))
	{
//	        FW_LOG_INFOA << "Could not connect. Error code = 0x"
//	             << hex << hres;
		pLoc->Release();
		CoUninitialize();
		return 1;                // Program has failed.
	}

//	    FW_LOG_INFOA << "Connected to ROOT\\CIMV2 WMI namespace";


	// Step 5: --------------------------------------------------
	// Set security levels on the proxy -------------------------

	hres = CoSetProxyBlanket(
	pSvc,                        // Indicates the proxy to set
	RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
	RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
	NULL,                        // Server principal name
	RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx
	RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
	NULL,                        // client identity
	EOAC_NONE                    // proxy capabilities
	);

	if (FAILED(hres))
	{
//	        FW_LOG_INFOA << "Could not set proxy blanket. Error code = 0x"
//	            << hex << hres;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return 1;               // Program has failed.
	}

	// Step 6: --------------------------------------------------
	// Use the IWbemServices pointer to make requests of WMI ----

	// For example, get the name of the operating system
	IEnumWbemClassObject* pEnumerator = NULL;
	hres = pSvc->ExecQuery(
	bstr_t("WQL"),
	//bstr_t("SELECT * FROM Win32_OperatingSystem"),
	bstr_t("SELECT * FROM Win32_ComputerSystem"),
	WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
	NULL,
	&pEnumerator);

	if (FAILED(hres))
	{
//	        FW_LOG_INFOA << "Query for operating system name failed."
//	            << " Error code = 0x"
//	            << hex << hres;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return 1;               // Program has failed.
	}

	// Step 7: -------------------------------------------------
	// get the data from the query in step 6 -------------------

	IWbemClassObject* pclsObj = NULL;
	ULONG uReturn = 0;

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1,
		&pclsObj, &uReturn);

		if (0 == uReturn)
		{
			break;
		}

		VARIANT vtProp;

		// get the value of the Name property
		//hr = pclsObj->get(L"Name", 0, &vtProp, 0, 0);
		//wcout << " OS Name : " << vtProp.bstrVal << endl;
		hr = pclsObj->Get(utf8_to_wstring(key).c_str(), 0, &vtProp, 0, 0);
		if (vtProp.bstrVal)
		{
			val = wstring_to_utf8(vtProp.bstrVal);
		}
//	        FW_LOG_INFOA << key << "|" << val << endl;
		VariantClear(&vtProp);

		pclsObj->Release();
	}

	// Cleanup
	// ========

	pSvc->Release();
	pLoc->Release();
	pEnumerator->Release();
	CoUninitialize();

	return 0;   // Program successfully completed.
#else
	throw std::logic_error("not complete");
#endif

}

std::string TC_Device::getVendor()
{
#if defined(_WIN64) || defined(_WIN32)
	std::string manufacturer;
	TC_Device::WmiQuery("Manufacturer", manufacturer);
	return manufacturer;
#elif defined(__linux)
	char buf_ps[128];
	std::string cmd = "cat /sys/class/dmi/id/board_vendor";
	FILE* ptr = NULL;
	if ((ptr = popen(cmd.c_str(), "r")) != NULL)
	{
		if (fgets(buf_ps, 128, ptr) == NULL)
		{
			std::logic_error("get product vendor info failed!!!");
		}
		pclose(ptr);
		ptr = NULL;
	}
	std::string info(buf_ps);
	return info.substr(0, info.length() - 1);
#else
	throw std::logic_error("not complete");
#endif
}

std::string TC_Device::getModel()
{
#if defined(_WIN64) || defined(_WIN32)
	std::string model;
	TC_Device::WmiQuery("Model", model);
	return manufacturer;
#elif defined(__linux)
	char buf_ps[128];
	std::string cmd = "cat /sys/class/dmi/id/board_name";
	FILE* ptr = NULL;
	if ((ptr = popen(cmd.c_str(), "r")) != NULL)
	{
		if (fgets(buf_ps, 128, ptr) == NULL)
		{
			std::logic_error("get product model info failed!!!");
		}
		pclose(ptr);
		ptr = NULL;
	}
	std::string info(buf_ps);
	return info.substr(0, info.length() - 1);
#else
	throw std::logic_error("not complete");
#endif
}
}
