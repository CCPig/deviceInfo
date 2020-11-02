#pragma once

#include <cstdio>
#include <string>
#include <vector>
#include <unordered_map>
#include "util/tc_ex.h"
#include "util/tc_platform.h"


//using namespace std;

namespace taf
{


struct TC_Device_Exception : public TC_Exception
{
	TC_Device_Exception(const std::string &buffer) : TC_Exception(buffer){};
	TC_Device_Exception(const std::string &buffer, int err) : TC_Exception(buffer, err){};
	~TC_Device_Exception() throw(){};
};


/////////////////////////////////////////////////
/**
* @file dev_info_get.h
* @brief  帮助类,都是静态函数,提供一些常用的函数 .
*
* @author  wisehuang@upchina.com
*
*/
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

