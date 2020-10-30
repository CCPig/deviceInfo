//#include "base/test/taf_adapter.h"
//#include "base/test/sys_init.h"
#include "tc_device.h"
#include "util/tc_common.h"
//#include "gtest/gtest.h"

//class DevInfoTest :public testing::Test {
//protected:
//	virtual void SetUp() {
//	}
//	virtual void TearDown() {}
//protected:
//};

using namespace taf;
using namespace std;

int main(int argc, char** argv)
{
//TEST_F(DevInfoTest, DevInfoget){
//		SysInit sys;
//		sys.InitConsoleLog(L"test");

/*	{
		std::cout << "dev info:" << std::endl;
		auto lip = TC_Device::getLIP();
		std::cout << "lip:" << lip << std::endl;
//		EXPECT_TRUE(!lip.empty());
	}*/

	{
		// 本地IP列表
		std::unordered_map<std::string, std::string> ip_macs = TC_Device::getLocalHosts();
		int i = 0;
		for(auto &item : ip_macs)
        {
			i++;
		    cout << i << ":" << item.first << "		" << item.second << endl;
        }
//		EXPECT_TRUE(!hosts.empty());
	}


/* 	{
		// 第一激活网卡MAC
		auto first_act_mac = TC_Device::getFirstActiveMAC();
		std::cout << "first_act_mac:" << first_act_mac << std::endl;

		// 第一个以太网MAC
		auto first_eth_mac = TC_Device::getFristEthernetMAC();
		std::cout << "first_eth_mac:" << first_eth_mac << std::endl;

		// 所有MAC
		std::vector<MAC_INFO> all_mac;
		TC_Device::getAllMAC(all_mac);

		std::vector<std::string> all_mac_str;
		for (auto &m: all_mac)
        {
		    all_mac_str.emplace_back(m.str_mac_);
        }
		for(auto &m: all_mac_str)
        {
            std::cout << "all_mac:" << m << std::endl;
        }
//		std::cout << "all_mac:" << taf::TC_Common::tostr(all_mac_str) << std::endl;

		// 所有物理MAC
		all_mac.clear();
		TC_Device::getAllPhysicalMAC(all_mac);

		all_mac_str.clear();
		for (auto &m: all_mac)
		{
            all_mac_str.emplace_back(m.str_mac_);
        }
		for(auto &m:all_mac_str)
        {
            std::cout << "all_physical_mac:" << m << std::endl;
        }
//		std::cout << "all_physical_mac:" << TC_Common::tostr(all_mac_str) << std::endl;
	}
*/

	{
		// CPU
		auto cpu = TC_Device::getCPU();
		std::cout << "cpu:" << cpu << std::endl;
	}

		// PCN
		auto pcn = TC_Device::getPCN();
		std::cout << "pcn:" << pcn << std::endl;

		// OSV
		auto osv = TC_Device::getOSV();
		std::cout << "osv:" << osv << std::endl;

		// PI
		auto pi = TC_Device::getPI();
		std::cout << "pi:" << pi << std::endl;

/*
	{
		// 硬盘序列
		auto hd = TC_Device::getHD();
		std::cout << "hd:" << hd << std::endl;
	}
*/

/*	{
		// IMEI：内部是硬盘序列, IMEI是移动端概念
		auto imei = TC_Device::getIMEI();
		std::cout << "imei:" << imei << std::endl;
	}*/

	{
		// VOL
		auto vol = TC_Device::getVOL();
		std::cout << "vol:" << vol << std::endl;
	}

/*	{
		// 系统manufacturer, model
		string manufacturer, model;
		TC_Device::WmiQuery("Manufacturer", manufacturer);
		TC_Device::WmiQuery("Model", model);
		std::cout << "Manufacturer:" << manufacturer << std::endl;
		std::cout << "Model:" << model << std::endl;
	}*/
}


