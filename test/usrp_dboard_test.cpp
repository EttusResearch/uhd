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
#include <usrp_uhd/usrp/dboard/manager.hpp>

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
    double get_rx_clock_rate(void){return 0.0;}
    double get_tx_clock_rate(void){return 0.0;}
};

BOOST_AUTO_TEST_CASE(test_manager){
    std::cout << "Making a dummy usrp dboard interface..." << std::endl;
    interface::sptr ifc0(new dummy_interface());

    std::cout << "Making a usrp dboard manager..." << std::endl;
    manager::sptr mgr0(new manager(ID_BASIC_RX, ID_BASIC_TX, ifc0));

    std::cout << "Testing the dboard manager..." << std::endl;
    BOOST_CHECK_EQUAL(size_t(3), mgr0->get_rx_subdev_names().size());
    BOOST_CHECK_EQUAL(size_t(1), mgr0->get_tx_subdev_names().size());

    std::cout << "Testing access (will fail later when db code filled in)..." << std::endl;
    BOOST_CHECK_THROW(mgr0->get_rx_subdev(""), std::invalid_argument);
    BOOST_CHECK_THROW(mgr0->get_tx_subdev("x"), std::invalid_argument);
    (*mgr0->get_rx_subdev("a"))[NULL];
    (*mgr0->get_tx_subdev(""))[NULL];
}
