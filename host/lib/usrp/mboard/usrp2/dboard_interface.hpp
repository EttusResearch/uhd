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

#include <uhd/usrp/dboard/interface.hpp>
#include "impl_base.hpp"

#ifndef INCLUDED_DBOARD_INTERFACE_HPP
#define INCLUDED_DBOARD_INTERFACE_HPP

class dboard_interface : public uhd::usrp::dboard::interface{
public:
    dboard_interface(impl_base *impl);

    ~dboard_interface(void);

    void write_aux_dac(unit_type_t, int, int);

    int read_aux_adc(unit_type_t, int);

    void set_atr_reg(gpio_bank_t, uint16_t, uint16_t, uint16_t);

    void set_gpio_ddr(gpio_bank_t, uint16_t, uint16_t);

    void write_gpio(gpio_bank_t, uint16_t, uint16_t);

    uint16_t read_gpio(gpio_bank_t);

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

    impl_base *_impl;
};

#endif /* INCLUDED_DBOARD_INTERFACE_HPP */
