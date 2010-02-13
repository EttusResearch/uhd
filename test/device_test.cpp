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
#include <uhd/device.hpp>

using namespace uhd;

BOOST_AUTO_TEST_CASE(test_device){
    device_addr_t device_addr;
    device_addr["type"] = "test";
    device_addr["num_dboards"] = "2";
    device::sptr dev = device::make(device_addr);

    std::cout << "Access the device" << std::endl;
    std::cout << wax::cast<std::string>((*dev)[DEVICE_PROP_NAME]) << std::endl;

    std::cout << "Access the mboard" << std::endl;
    wax::obj mb0 = (*dev)[DEVICE_PROP_MBOARD];
    std::cout << wax::cast<std::string>(mb0[MBOARD_PROP_NAME]) << std::endl;
    BOOST_CHECK_EQUAL(
        2,
        wax::cast<prop_names_t>(mb0[MBOARD_PROP_RX_DBOARD_NAMES]).size()
    );
    BOOST_CHECK_EQUAL(
        2,
        wax::cast<prop_names_t>(mb0[MBOARD_PROP_TX_DBOARD_NAMES]).size()
    );

}
