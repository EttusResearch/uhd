//
// Copyright 2010 Ettus Research LLC
//

#include <boost/test/unit_test.hpp>
#include <usrp_uhd/device_addr.hpp>

BOOST_AUTO_TEST_CASE(test_mac_addr){
    std::cout << "Testing mac addr..." << std::endl;
    const std::string mac_addr_str("00:01:23:45:67:89");
    usrp_uhd::mac_addr_t mac_addr(mac_addr_str);
    std::cout << "Input: " << mac_addr_str << std::endl;
    std::cout << "Output: " << mac_addr << std::endl;
    BOOST_CHECK_EQUAL(mac_addr_str, mac_addr.to_string());
}

BOOST_AUTO_TEST_CASE(test_ip_addr){
    std::cout << "Testing ip addr..." << std::endl;
    const std::string ip_addr_str("192.168.1.10");
    usrp_uhd::ip_addr_t ip_addr(ip_addr_str);
    std::cout << "Input: " << ip_addr_str << std::endl;
    std::cout << "Output: " << ip_addr << std::endl;
    BOOST_CHECK_EQUAL(ip_addr_str, ip_addr.to_string());
}
