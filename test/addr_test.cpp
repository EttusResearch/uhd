//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/device_addr.hpp>
#include <cppunit/extensions/HelperMacros.h>

/***********************************************************************
 * cpp unit setup
 **********************************************************************/
class addr_test : public CppUnit::TestFixture{
    CPPUNIT_TEST_SUITE(addr_test);
    CPPUNIT_TEST(test_mac_addr);
    CPPUNIT_TEST(test_ip_addr);
    CPPUNIT_TEST_SUITE_END();

public:
    void test_mac_addr(void);
    void test_ip_addr(void);
};

CPPUNIT_TEST_SUITE_REGISTRATION(addr_test);

void addr_test::test_mac_addr(void){
    std::cout << "Testing mac addr..." << std::endl;
    const std::string mac_addr_str("00:01:23:45:67:89");
    usrp_uhd::mac_addr_t mac_addr(mac_addr_str);
    std::cout << "Input: " << mac_addr_str << std::endl;
    std::cout << "Output: " << mac_addr << std::endl;
    CPPUNIT_ASSERT_EQUAL(mac_addr_str, mac_addr.to_string());
}

void addr_test::test_ip_addr(void){
    std::cout << "Testing ip addr..." << std::endl;
    const std::string ip_addr_str("192.168.1.10");
    usrp_uhd::ip_addr_t ip_addr(ip_addr_str);
    std::cout << "Input: " << ip_addr_str << std::endl;
    std::cout << "Output: " << ip_addr << std::endl;
    CPPUNIT_ASSERT_EQUAL(ip_addr_str, ip_addr.to_string());
}
