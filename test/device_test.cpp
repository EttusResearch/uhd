//
// Copyright 2010 Ettus Research LLC
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
