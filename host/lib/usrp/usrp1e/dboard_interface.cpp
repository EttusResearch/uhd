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

#include <uhd/utils.hpp>
#include "usrp1e_impl.hpp"

using namespace uhd::usrp;

class usrp1e_dboard_interface : public dboard_interface{
public:
    usrp1e_dboard_interface(usrp1e_impl *impl);
    ~usrp1e_dboard_interface(void);

    void write_aux_dac(unit_type_t, int, int);
    int read_aux_adc(unit_type_t, int);

    void set_atr_reg(gpio_bank_t, boost::uint16_t, boost::uint16_t, boost::uint16_t);
    void set_gpio_ddr(gpio_bank_t, boost::uint16_t, boost::uint16_t);
    void write_gpio(gpio_bank_t, boost::uint16_t, boost::uint16_t);
    boost::uint16_t read_gpio(gpio_bank_t);

    void write_i2c(int, const byte_vector_t &);
    byte_vector_t read_i2c(int, size_t);

    double get_rx_clock_rate(void);
    double get_tx_clock_rate(void);

private:
    byte_vector_t transact_spi(
        spi_dev_t dev,
        spi_latch_t latch,
        spi_push_t push,
        const byte_vector_t &buf,
        bool readback
    );

    usrp1e_impl *_impl;
};

/***********************************************************************
 * Make Function
 **********************************************************************/
dboard_interface::sptr make_usrp1e_dboard_interface(usrp1e_impl *impl){
    return dboard_interface::sptr(new usrp1e_dboard_interface(impl));
}

/***********************************************************************
 * Structors
 **********************************************************************/
usrp1e_dboard_interface::usrp1e_dboard_interface(usrp1e_impl *impl){
    _impl = impl;
}

usrp1e_dboard_interface::~usrp1e_dboard_interface(void){
    /* NOP */
}

/***********************************************************************
 * Clock Rates
 **********************************************************************/
double usrp1e_dboard_interface::get_rx_clock_rate(void){
    throw std::runtime_error("not implemented");
}

double usrp1e_dboard_interface::get_tx_clock_rate(void){
    throw std::runtime_error("not implemented");
}

/***********************************************************************
 * GPIO
 **********************************************************************/
void usrp1e_dboard_interface::set_gpio_ddr(gpio_bank_t bank, boost::uint16_t value, boost::uint16_t mask){
    throw std::runtime_error("not implemented");
}

void usrp1e_dboard_interface::write_gpio(gpio_bank_t bank, boost::uint16_t value, boost::uint16_t mask){
    throw std::runtime_error("not implemented");
}

boost::uint16_t usrp1e_dboard_interface::read_gpio(gpio_bank_t bank){
    throw std::runtime_error("not implemented");
}

void usrp1e_dboard_interface::set_atr_reg(gpio_bank_t bank, boost::uint16_t tx_value, boost::uint16_t rx_value, boost::uint16_t mask){
    throw std::runtime_error("not implemented");
}

/***********************************************************************
 * SPI
 **********************************************************************/
dboard_interface::byte_vector_t usrp1e_dboard_interface::transact_spi(
    spi_dev_t dev,
    spi_latch_t latch,
    spi_push_t push,
    const byte_vector_t &buf,
    bool readback
){
    throw std::runtime_error("not implemented");
}

/***********************************************************************
 * I2C
 **********************************************************************/
void usrp1e_dboard_interface::write_i2c(int i2c_addr, const byte_vector_t &buf){
    throw std::runtime_error("not implemented");
}

dboard_interface::byte_vector_t usrp1e_dboard_interface::read_i2c(int i2c_addr, size_t num_bytes){
    throw std::runtime_error("not implemented");
}

/***********************************************************************
 * Aux DAX/ADC
 **********************************************************************/
void usrp1e_dboard_interface::write_aux_dac(dboard_interface::unit_type_t unit, int which, int value){
    throw std::runtime_error("not implemented");
}

int usrp1e_dboard_interface::read_aux_adc(dboard_interface::unit_type_t unit, int which){
    throw std::runtime_error("not implemented");
}
