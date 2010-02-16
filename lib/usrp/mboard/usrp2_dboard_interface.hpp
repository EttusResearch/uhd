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
#include "usrp2_impl.hpp"

#ifndef INCLUDED_USRP2_DBOARD_INTERFACE_HPP
#define INCLUDED_USRP2_DBOARD_INTERFACE_HPP

class usrp2_dboard_interface : public uhd::usrp::dboard::interface{
public:
    usrp2_dboard_interface(usrp2_impl *impl){
        _impl = impl;
    }

    ~usrp2_dboard_interface(void){
        /* NOP */
    }

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

    double get_rx_clock_rate(void){
        return 100e6;
    }

    double get_tx_clock_rate(void){
        return 100e6;
    }

private:
    usrp2_impl *_impl;
};

#endif /* INCLUDED_USRP2_DBOARD_INTERFACE_HPP */
