//
// Copyright 2010 Ettus Research LLC
//

#include <usrp_uhd/usrp/dboard/manager.hpp>
#include <cppunit/extensions/HelperMacros.h>

using namespace usrp_uhd::usrp::dboard;

/***********************************************************************
 * dummy interface for dboards
 **********************************************************************/
class dummy_interface : public interface{
public:
    dummy_interface(void){}
    ~dummy_interface(void){}
    void write_aux_dac(int, int){}
    int read_aux_adc(int){return 0;}
    void set_atr_reg(gpio_bank_t, uint16_t, uint16_t, uint16_t){}
    void set_gpio_ddr(gpio_bank_t, uint16_t, uint16_t){}
    void write_gpio(gpio_bank_t, uint16_t, uint16_t){}
    uint16_t read_gpio(gpio_bank_t){return 0;}
    void write_i2c (int, const std::string &){}
    std::string read_i2c (int, size_t){return "";}
    void write_spi (spi_dev_t, spi_push_t, const std::string &){}
    std::string read_spi (spi_dev_t, spi_latch_t, size_t){return "";}
};

/***********************************************************************
 * cpp unit setup
 **********************************************************************/
class dboard_test : public CppUnit::TestFixture{
    CPPUNIT_TEST_SUITE(dboard_test);
    CPPUNIT_TEST(test_manager);
    CPPUNIT_TEST_SUITE_END();

public:
    void test_manager(void);
};

CPPUNIT_TEST_SUITE_REGISTRATION(dboard_test);

void dboard_test::test_manager(void){
    std::cout << "Making a dummy usrp dboard interface..." << std::endl;
    interface::sptr ifc0(new dummy_interface());

    std::cout << "Making a usrp dboard manager..." << std::endl;
    manager::sptr mgr0(new manager(0x0001, 0x0000, ifc0)); //basic rx, basic tx

    std::cout << "Testing the dboard manager..." << std::endl;
    CPPUNIT_ASSERT_EQUAL(size_t(3), mgr0->get_num_rx_subdevs());
    CPPUNIT_ASSERT_EQUAL(size_t(1), mgr0->get_num_tx_subdevs());

    std::cout << "Testing access (will fail later when db code filled in)..." << std::endl;
    CPPUNIT_ASSERT_THROW(mgr0->get_rx_subdev(3), std::out_of_range);
    CPPUNIT_ASSERT_THROW(mgr0->get_tx_subdev(1), std::out_of_range);
    (*mgr0->get_rx_subdev(0))[NULL];
    (*mgr0->get_tx_subdev(0))[NULL];
}
