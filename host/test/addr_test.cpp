//
// Copyright 2010 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <boost/test/unit_test.hpp>
#include <uhd/types/mac_addr.hpp>
#include <uhd/types/device_addr.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/foreach.hpp>
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
    dev_addr["key2"] = "val2";

    //convert to and from args string
    std::cout << "Pretty Print: " << std::endl << dev_addr.to_string();
    std::string args_str = dev_addr.to_args_str();
    std::cout << "Args String: " << args_str << std::endl;
    uhd::device_addr_t new_dev_addr = uhd::device_addr_t::from_args_str(args_str);

    //they should be the same size
    BOOST_CHECK_EQUAL(dev_addr.size(), new_dev_addr.size());

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
}
