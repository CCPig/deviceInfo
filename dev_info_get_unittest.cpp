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

	{
//		std::cout << "dev info:" << std::endl;
		auto lip = TC_Device::getLIP();
		std::cout << "lip:" << lip << std::endl;
//		EXPECT_TRUE(!lip.empty());
	}

	{
		// 本地IP列表
		auto vec = TC_Device::getLocalHosts();
		int i = 0;
		for(auto &ip : vec)
        {
			i++;
		    cout << i << ":" << ip<< endl;
        }
//		EXPECT_TRUE(!hosts.empty());
	}

	{
		// 第一激活网卡MAC
		auto first_act_mac = TC_Device::getFirstActiveNetCard();
		std::cout << "first_act_mac:" << first_act_mac.str_mac_ << std::endl;

		// 第一个以太网MAC
		auto first_eth_mac = TC_Device::getFristEthernetNetCard();
		std::cout << "first_eth_mac:" << first_eth_mac.str_mac_ << std::endl;

		// 所有MAC
		auto all_mac = TC_Device::getAllNetCard();
		cout << "all_net_card:" << endl;
		for (auto& item : all_mac)
		{
			cout << item.first << "| " << item.second.str_ip_ << "| " << item.second.str_mac_ << endl;
		}

		auto all_physical_mac = TC_Device::getAllPhysicalNetCard();
		cout << "all_physical_net_card:" << endl;
		for (auto& item: all_physical_mac)
		{
			cout << item.first << "| " << item.second.str_ip_ << "| " << item.second.str_mac_ << endl;
		}

//		std::cout << "all_mac:" << taf::TC_Common::tostr(all_mac_str) << std::endl;
	}


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

	{
		// 硬盘序列
		auto hd = TC_Device::getHD();
		std::cout << "hd.model:" << get<0>(hd) << std::endl;
		std::cout << "hd.serial_no:" << get<1>(hd) << std::endl;
	}

	{
		// IMEI：内部是硬盘序列, IMEI是移动端概念
		auto imei = TC_Device::getIMEI();
		std::cout << "imei:" << imei << std::endl;
	}

	{
		// VOL
		auto vol = TC_Device::getVOL();
		std::cout << "vol:" << vol << std::endl;
	}

	auto vendor = TC_Device::getVendor();
	std::cout << "vendor:"  << vendor << std::endl;

	auto model = TC_Device::getModel();
	std::cout << "model:"  << model << std::endl;
}


