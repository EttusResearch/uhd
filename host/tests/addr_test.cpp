//
// Copyright 2010-2011 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: GPL-3.0-or-later
//

#include <boost/test/unit_test.hpp>
#include <uhd/types/mac_addr.hpp>
#include <uhd/types/device_addr.hpp>
#include <uhd/usrp/dboard_id.hpp>
#include <boost/assign/list_of.hpp>
#include <algorithm>
#include <iostream>

BOOST_AUTO_TEST_CASE(test_mac_addr){
    std::cout << "Testing mac addr..." << std::endl;
    const std::string mac_addr_str("00:01:23:45:67:89");
    uhd::mac_addr_t mac_addr = uhd::mac_addr_t::from_string(mac_addr_str);
    std::cout << "Input: " << mac_addr_str << std::endl;
    std::cout << "Output: " << mac_addr.to_string() << std::endl;
    BOOST_CHECK_EQUAL(mac_addr_str, mac_addr.to_string());
}

BOOST_AUTO_TEST_CASE(test_device_addr){
    std::cout << "Testing device addr..." << std::endl;

    //load the device address with something
    uhd::device_addr_t dev_addr;
    dev_addr["key1"] = "val1";
    dev_addr["key1"] = "val1";
    dev_addr["key3"] = "";

    //convert to and from args string
    std::cout << "Pretty Print: " << std::endl << dev_addr.to_pp_string();
    std::string args_str = dev_addr.to_string();
    std::cout << "Args String: " << args_str << std::endl;
    uhd::device_addr_t new_dev_addr(args_str);

    //they should be the same size
    BOOST_REQUIRE_EQUAL(dev_addr.size(), new_dev_addr.size());

    //the keys should match
    std::vector<std::string> old_dev_addr_keys = dev_addr.keys();
    std::vector<std::string> new_dev_addr_keys = new_dev_addr.keys();
    BOOST_CHECK_EQUAL_COLLECTIONS(
        old_dev_addr_keys.begin(), old_dev_addr_keys.end(),
        new_dev_addr_keys.begin(), new_dev_addr_keys.end()
    );

    //the vals should match
    std::vector<std::string> old_dev_addr_vals = dev_addr.vals();
    std::vector<std::string> new_dev_addr_vals = new_dev_addr.vals();
    BOOST_CHECK_EQUAL_COLLECTIONS(
        old_dev_addr_vals.begin(), old_dev_addr_vals.end(),
        new_dev_addr_vals.begin(), new_dev_addr_vals.end()
    );

    uhd::device_addr_t dev_addr_lhs1("key1=val1,key2=val2");
    dev_addr_lhs1.update(uhd::device_addr_t("key2=val2x,key3=val3"), false);
    BOOST_CHECK_EQUAL(dev_addr_lhs1["key1"], "val1");
    BOOST_CHECK_EQUAL(dev_addr_lhs1["key2"], "val2x");
    BOOST_CHECK_EQUAL(dev_addr_lhs1["key3"], "val3");
    std::cout << "Merged: " << dev_addr_lhs1.to_string() << std::endl;
}

BOOST_AUTO_TEST_CASE(test_dboard_id){
    std::cout << "Testing dboard id..." << std::endl;

    using namespace uhd::usrp;

    BOOST_CHECK(dboard_id_t() == dboard_id_t::none());
    BOOST_CHECK_EQUAL(dboard_id_t().to_uint16(), dboard_id_t::none().to_uint16());
    BOOST_CHECK_EQUAL(dboard_id_t::from_string("0x1234").to_uint16(), 0x1234);
    BOOST_CHECK_EQUAL(dboard_id_t::from_string("1234").to_uint16(), 1234);
    std::cout << "Pretty Print: " << std::endl << dboard_id_t::none().to_pp_string();
}

BOOST_AUTO_TEST_CASE(test_map_device_addr){
    std::map<std::string, std::string> dev_addr_map;
    dev_addr_map["key1"] = "val1";
    dev_addr_map["key2"] = "val2";
    uhd::device_addr_t dev_addr(dev_addr_map);
    BOOST_CHECK_EQUAL(dev_addr["key1"], "val1");
    BOOST_CHECK_EQUAL(dev_addr["key2"], "val2");
}
