#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>
#include "util/tc_ex.h"
#include "util/tc_platform.h"


#if TARGET_PLATFORM_WINDOWS
#include <tchar.h>
#include <windows.h>
#include <winioctl.h>
#endif


namespace taf
{

#if TARGET_PLATFORM_WINDOWS
#pragma pack(1)

#define  IDENTIFY_BUFFER_SIZE  512

//  IOCTL commands
#define  DFP_GET_VERSION          0x00074080
#define  DFP_SEND_DRIVE_COMMAND   0x0007c084
#define  DFP_RECEIVE_DRIVE_DATA   0x0007c088

#define  FILE_DEVICE_SCSI              0x0000001b
#define  IOCTL_SCSI_MINIPORT_IDENTIFY  ((FILE_DEVICE_SCSI << 16) + 0x0501)
#define  IOCTL_SCSI_MINIPORT 0x0004D008  //  see NTDDSCSI.H for definition

#define SMART_GET_VERSION               CTL_CODE(IOCTL_DISK_BASE, 0x0020, METHOD_BUFFERED, FILE_READ_ACCESS)
#define SMART_SEND_DRIVE_COMMAND        CTL_CODE(IOCTL_DISK_BASE, 0x0021, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
#define SMART_RCV_DRIVE_DATA            CTL_CODE(IOCTL_DISK_BASE, 0x0022, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)
//  GETVERSIONOUTPARAMS contains the data returned from the
//  Get Driver Version function.
typedef struct _GETVERSIONOUTPARAMS
{
	BYTE bVersion;      // Binary driver version.
	BYTE bRevision;     // Binary driver revision.
	BYTE bReserved;     // Not used.
	BYTE bIDEDeviceMap; // Bit map of IDE devices.
	DWORD fCapabilities; // Bit mask of driver capabilities.
	DWORD dwReserved[4]; // For future use.
} GETVERSIONOUTPARAMS, *PGETVERSIONOUTPARAMS, *LPGETVERSIONOUTPARAMS;


//  Bits returned in the fCapabilities member of GETVERSIONOUTPARAMS
#define  CAP_IDE_ID_FUNCTION             1  // ATA ID command supported
#define  CAP_IDE_ATAPI_ID                2  // ATAPI ID command supported
#define  CAP_IDE_EXECUTE_SMART_FUNCTION  4  // SMART commannds supported

//  Valid values for the bCommandReg member of IDEREGS.
#define  IDE_ATAPI_IDENTIFY  0xA1  //  Returns ID sector for ATAPI.
#define  IDE_ATA_IDENTIFY    0xEC  //  Returns ID sector for ATA.

// The following struct defines the interesting part of the IDENTIFY
// buffer:
typedef struct _IDSECTOR
{
	USHORT  wGenConfig;
	USHORT  wNumCyls;
	USHORT  wReserved;
	USHORT  wNumHeads;
	USHORT  wBytesPerTrack;
	USHORT  wBytesPerSector;
	USHORT  wSectorsPerTrack;
	USHORT  wVendorUnique[3];
	CHAR    sSerialNumber[20];
	USHORT  wBufferType;
	USHORT  wBufferSize;
	USHORT  wECCSize;
	CHAR    sFirmwareRev[8];
	CHAR    sModelNumber[40];
	USHORT  wMoreVendorUnique;
	USHORT  wDoubleWordIO;
	USHORT  wCapabilities;
	USHORT  wReserved1;
	USHORT  wPIOTiming;
	USHORT  wDMATiming;
	USHORT  wBS;
	USHORT  wNumCurrentCyls;
	USHORT  wNumCurrentHeads;
	USHORT  wNumCurrentSectorsPerTrack;
	ULONG   ulCurrentSectorCapacity;
	USHORT  wMultSectorStuff;
	ULONG   ulTotalAddressableSectors;
	USHORT  wSingleWordDMA;
	USHORT  wMultiWordDMA;
	BYTE    bReserved[128];
} IDSECTOR, *PIDSECTOR;


typedef struct _SRB_IO_CONTROL
{
	ULONG HeaderLength;
	UCHAR Signature[8];
	ULONG Timeout;
	ULONG ControlCode;
	ULONG ReturnCode;
	ULONG Length;
} SRB_IO_CONTROL, *PSRB_IO_CONTROL;

//  Max number of drives assuming primary/secondary, master/slave topology
//	Modified to read only the master serial
#define  MAX_IDE_DRIVES  1

//
// IDENTIFY data (from ATAPI driver source)
//

#pragma pack(1)

typedef struct _IDENTIFY_DATA {
	USHORT GeneralConfiguration;            // 00 00
	USHORT NumberOfCylinders;               // 02  1
	USHORT Reserved1;                       // 04  2
	USHORT NumberOfHeads;                   // 06  3
	USHORT UnformattedBytesPerTrack;        // 08  4
	USHORT UnformattedBytesPerSector;       // 0A  5
	USHORT SectorsPerTrack;                 // 0C  6
	USHORT VendorUnique1[3];                // 0E  7-9
	USHORT SerialNumber[10];                // 14  10-19
	USHORT BufferType;                      // 28  20
	USHORT BufferSectorSize;                // 2A  21
	USHORT NumberOfEccBytes;                // 2C  22
	USHORT FirmwareRevision[4];             // 2E  23-26
	USHORT ModelNumber[20];                 // 36  27-46
	UCHAR  MaximumBlockTransfer;            // 5E  47
	UCHAR  VendorUnique2;                   // 5F
	USHORT DoubleWordIo;                    // 60  48
	USHORT Capabilities;                    // 62  49
	USHORT Reserved2;                       // 64  50
	UCHAR  VendorUnique3;                   // 66  51
	UCHAR  PioCycleTimingMode;              // 67
	UCHAR  VendorUnique4;                   // 68  52
	UCHAR  DmaCycleTimingMode;              // 69
	USHORT TranslationFieldsValid : 1;        // 6A  53
	USHORT Reserved3 : 15;
	USHORT NumberOfCurrentCylinders;        // 6C  54
	USHORT NumberOfCurrentHeads;            // 6E  55
	USHORT CurrentSectorsPerTrack;          // 70  56
	ULONG  CurrentSectorCapacity;           // 72  57-58
	USHORT CurrentMultiSectorSetting;       //     59
	ULONG  UserAddressableSectors;          //     60-61
	USHORT SingleWordDMASupport : 8;        //     62
	USHORT SingleWordDMAActive : 8;
	USHORT MultiWordDMASupport : 8;         //     63
	USHORT MultiWordDMAActive : 8;
	USHORT AdvancedPIOModes : 8;            //     64
	USHORT Reserved4 : 8;
	USHORT MinimumMWXferCycleTime;          //     65
	USHORT RecommendedMWXferCycleTime;      //     66
	USHORT MinimumPIOCycleTime;             //     67
	USHORT MinimumPIOCycleTimeIORDY;        //     68
	USHORT Reserved5[2];                    //     69-70
	USHORT ReleaseTimeOverlapped;           //     71
	USHORT ReleaseTimeServiceCommand;       //     72
	USHORT MajorRevision;                   //     73
	USHORT MinorRevision;                   //     74
	USHORT Reserved6[50];                   //     75-126
	USHORT SpecialFunctionsEnabled;         //     127
	USHORT Reserved7[128];                  //     128-255
} IDENTIFY_DATA, *PIDENTIFY_DATA;

#pragma pack()
//  Required to ensure correct PhysicalDrive IOCTL structure setup


//
// IOCTL_STORAGE_QUERY_PROPERTY
//
// Input Buffer:
//      a STORAGE_PROPERTY_QUERY structure which describes what type of query
//      is being done, what property is being queried for, and any additional
//      parameters which a particular property query requires.
//
//  Output Buffer:
//      Contains a buffer to place the results of the query into.  Since all
//      property descriptors can be cast into a STORAGE_DESCRIPTOR_HEADER,
//      the IOCTL can be called once with a small buffer then again using
//      a buffer as large as the header reports is necessary.
//
//
// Types of queries
//
// define some initial property id's
//
// Query structure - additional parameters for specific queries can follow
// the header
//
#define IOCTL_STORAGE_QUERY_PROPERTY   CTL_CODE(IOCTL_STORAGE_BASE, 0x0500, METHOD_BUFFERED, FILE_ANY_ACCESS)

//
// Device property descriptor - this is really just a rehash of the inquiry
// data retrieved from a scsi device
//
// This may only be retrieved from a target device.  Sending this to the bus
// will result in an error
//


#define  SENDIDLENGTH  sizeof (SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE
#define IOCTL_DISK_GET_DRIVE_GEOMETRY_EX CTL_CODE(IOCTL_DISK_BASE, 0x0028, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif


// struct TC_Device_Exception : public TC_Exception
// {
// 	TC_Device_Exception(const std::string &buffer) : TC_Exception(buffer){};
// 	TC_Device_Exception(const std::string &buffer, int err) : TC_Exception(buffer, err){};
// 	~TC_Device_Exception() throw(){};
// };


/////////////////////////////////////////////////

/**
* @brief  基础工具类，提供了一些获取系统或硬件信息的基本函数.
*
* 局域网IP:LIP
*
* MAC地址:MAC
*
* 硬盘序列号:HD
*
* 硬盘分区信息:PI
*
* CUP序列号:CPU
*
* PC终端设备名:PCN(这里暂时返回的是操作系统简称)
*
* PC终端设备序列号:SCN
*
* 操作系统版本:OSV
*
* 设备通识码IMEI:IMEI
*
* 系统卷盘标号:VOL
*/

struct MAC_INFO {
	std::string str_mac_ 	= "";		//mac地址
	std::string str_name_ 	= "";		//mac名称
	std::string str_ip_ 	="";		//ip名称
	int dw_type_;						//mac类型，和PIP_ADAPTER_INFO结构体里的Type字段保持一致，例如MIB_IF_TYPE_ETHERNET=6，表示以太网网卡类型
	std::string str_description_  = ""; //mac描述
};

#if TARGET_PLATFORM_WINDOWS
struct PartitionInfo {
	string name;				// 盘符
	wstring partition_format;	// 分区格式
	LPDWORD volume_serial_number;// 磁盘序列号
	ULARGE_INTEGER totalBytes;	// 磁盘大小
	ULARGE_INTEGER freeBytes;	// 磁盘可以大小
};



class MasterHardDiskSerial
{
public:
	MasterHardDiskSerial();
	~MasterHardDiskSerial();
	int getSerialNo(std::string &strSerialNumber);//获取所有系统硬盘序列号，其中入参strSerialNumber为当前运行系统所在硬盘序列号
	int getErrorMessage(TCHAR* _ptszErrorMessage = NULL);
	void getSerialNo(std::vector<string> &serialNumber); //获取多系统硬盘序列号
	void getSystemSerialNo(std::vector<string> &serialNumber) { serialNumber = m_vecSysDirveSerNum; }//获取系统盘序列号，第一个可以认为是当前正在运行系统所在盘的序列号
	bool getFirstActiveMAC(taf::MAC_INFO &strMac);	//获取当前激活的MAC

	/*
	* @brief 获取第一个物理以太网卡MAC
	*/
	bool getFristEthernetMac(taf::MAC_INFO &macOut);
	/*
	* @brief 获取所有网卡MAC
	*/
	bool getAllMAC(std::vector<taf::MAC_INFO> &in_vecMac);
	/*
	* @brief 获取所有物理网卡MAC
	*/
	bool getAllPhysicalMAC(std::vector<taf::MAC_INFO> &in_vecMac);
private:
	char* convertToString(DWORD dwDiskdata[256], int iFirstIndex, int iLastIndex, char* pcBuf = NULL);
	BOOL doIDENTIFY(HANDLE, PSENDCMDINPARAMS, PSENDCMDOUTPARAMS, BYTE, BYTE, PDWORD);
	int readPhysicalDriveInNTWithAdminRights(void);
	int	readPhysicalDriveInNTUsingSmart(void);
	int	readPhysicalDriveInNTWithZeroRights(void);
	int	readIdeDriveAsScsiDriveInNT(void);
	char* flipAndCodeBytes(int iPos, int iFlip, const char * pcStr = NULL, char * pcBuf = NULL);
	void printIdeInfo(int iDrive, DWORD dwDiskdata[256]);
	long getHardDriveComputerID();
	void trims(char* data);           //去掉字符串中的空格
	int getSysDriveID(std::vector<int> &vecSysDriveID);	//获取系统所在硬盘ID

private:
	char m_cszHardDriveSerialNumber[1024];
	char m_cszHardDriveModelNumber[1024];
	char m_cszErrorMessage[256];
	BYTE byIdOutCmd[sizeof(SENDCMDOUTPARAMS) + IDENTIFY_BUFFER_SIZE - 1];
	std::vector<std::string> m_vecSysDirveSerNum;
};

#endif

class  TC_Device
{
public:
	/**
	* @brief  获取局域网IP
	* @return string 返回局域网IP字符串
	*/
	static std::string getLIP();

	/**
	* @brief  获取所有的局域网IP
	* @return vector 返回所有的局域网IP数组
	*/
	static std::vector<std::string> getLocalHosts();

	/**
	* @brief  获取所有的网卡信息
	* @return unordered_map 返回所有的网卡字典信息
	*/
	static std::unordered_map<std::string, taf::MAC_INFO> getAllNetCard();

	/**
	* @brief  获取所有物理网卡信息
	* @return unordered_map<网卡名， 网卡信息>  获取所有物理网卡信息
	*/
	static std::unordered_map<std::string, taf::MAC_INFO> getAllPhysicalNetCard();

	/**
	* @brief  获取当前激活的第一个有效网卡
	* @return taf::MAC_INFO 返回MAC地址字符串
	*/
	static taf::MAC_INFO getFirstActiveNetCard();

	/**
	* @brief  获取第一个能获取到的有效物理以太网网卡
	* @return taf::MAC_INFO 返回MAC地址字符串
	*/
	static taf::MAC_INFO getFristEthernetNetCard();

	/**
	* @brief  获取系统硬盘型号和序列号
	* @return tuple 返回系统硬盘型号和序列号元组
	*/
	static std::tuple<std::string, std::string> getHD();

	/**
	* @brief  获取硬盘分区信息
	* @return string 返回硬盘分区信息字符串
	*/
	static std::string getPI();

#if TARGET_PLATFORM_WINDOWS
	/*
	* @brief 返回获取到的分区数量
	*/
	static int readDisk(vector<PartitionInfo> &partition_info);

	/*
	* @brief 获取分区信息
	*/
	static PartitionInfo getPartitionInfo(LPTSTR lpDriveStrings);
#endif

	/**
	* @brief  获取CPU序列号
	* @return string 返回CPU序列号字符串
	*/
	static std::string getCPU();

#if (TARGET_PLATFORM_IOS || TARGET_PLATFORM_LINUX)
	/**
	* @brief  获取UNIX终端系统和版本信息
	* @return std::tuple 0:系统，1：版本
	*/
	static std::tuple<std::string, std::string> getUnixOsInfo();
#endif

	/**
	* @brief  获取PC终端设备名
	* @return string 返回PC终端设备名PCN字符串
	*/
	static std::string getPCN();

	/**
	* @brief  获取操作系统版本OSV
	* @return string 返回操作系统版本OSV字符串
	*/
	static std::string getOSV();

	/**
	* @brief  获取设备通识码IMEI
	* @return string 返回设备通识码IMEI字符串
	*/
	static std::string getIMEI();

	/**
	* @brief  获取系统卷盘标号
	* @return string 返回系统卷盘标号VOL字符串
	*/
	static std::string getVOL();

	static std::string getVendor();
	static std::string getModel();

private:
	/**
	 * @brief 通过WMI接口获取系统信息
	 * @param key 系统信息key
	 * @param val 系统信息val
	 * @return
	 *	@retval 0 成功
	 * @note 可通过命令行工具对比
	 * cmd> wmic computersystem get model,name,manufacturer,systemtype
	 * https://www.windows-commandline.com/get-computer-model/
	 */
	static int WmiQuery(const std::string &key, std::string &val);
};

}



