//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/device.hpp>
#include <cppunit/extensions/HelperMacros.h>

/***********************************************************************
 * cpp unit setup
 **********************************************************************/
class device_test : public CppUnit::TestFixture{
    CPPUNIT_TEST_SUITE(device_test);
    CPPUNIT_TEST(test);
    CPPUNIT_TEST_SUITE_END();

public:
    void test(void);
};

CPPUNIT_TEST_SUITE_REGISTRATION(device_test);

using namespace usrp_uhd;

void device_test::test(void){
    device_addr_t device_addr(DEVICE_ADDR_TYPE_VIRTUAL);
    device_addr.virtual_args.num_dboards = 2;
    device_addr.virtual_args.num_rx_dsps = 3;
    device_addr.virtual_args.num_tx_dsps = 4;
    device::sptr dev = device::make(device_addr);
    std::cout << wax::cast<std::string>((*dev)[DEVICE_PROP_NAME]) << std::endl;
}
