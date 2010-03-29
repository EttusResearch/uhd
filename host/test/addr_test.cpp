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
#include <iostream>

BOOST_AUTO_TEST_CASE(test_mac_addr){
    std::cout << "Testing mac addr..." << std::endl;
    const std::string mac_addr_str("00:01:23:45:67:89");
    uhd::mac_addr_t mac_addr = uhd::mac_addr_t::from_string(mac_addr_str);
    std::cout << "Input: " << mac_addr_str << std::endl;
    std::cout << "Output: " << mac_addr.to_string() << std::endl;
    BOOST_CHECK_EQUAL(mac_addr_str, mac_addr.to_string());
}
