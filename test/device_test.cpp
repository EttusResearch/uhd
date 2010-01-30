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
#include <usrp_uhd/device.hpp>

using namespace usrp_uhd;

BOOST_AUTO_TEST_CASE(test_device){
    device_addr_t device_addr(DEVICE_ADDR_TYPE_VIRTUAL);
    device_addr.virtual_args.num_dboards = 2;
    device_addr.virtual_args.num_rx_dsps = 3;
    device_addr.virtual_args.num_tx_dsps = 4;
    device::sptr dev = device::make(device_addr);

    std::cout << "Access the device" << std::endl;
    std::cout << wax::cast<std::string>((*dev)[DEVICE_PROP_NAME]) << std::endl;

    std::cout << "Access the mboard" << std::endl;
    wax::proxy mb0 = (*dev)[DEVICE_PROP_MBOARD];
    std::cout << wax::cast<std::string>(mb0[MBOARD_PROP_NAME]) << std::endl;
    BOOST_CHECK_EQUAL(
        device_addr.virtual_args.num_dboards,
        wax::cast<prop_names_t>(mb0[MBOARD_PROP_RX_DBOARD_NAMES]).size()
    );
    BOOST_CHECK_EQUAL(
        device_addr.virtual_args.num_dboards,
        wax::cast<prop_names_t>(mb0[MBOARD_PROP_TX_DBOARD_NAMES]).size()
    );

}
