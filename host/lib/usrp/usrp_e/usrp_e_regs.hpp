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

#ifndef INCLUDED_USRP_E_REGS_HPP
#define INCLUDED_USRP_E_REGS_HPP

#include <boost/cstdint.hpp>

////////////////////////////////////////////////
// GPIO, Slave 4
//
// These go to the daughterboard i/o pins

#define GPIO_BASE 0x40

struct gpio_regs_t{
    boost::uint16_t rx_io;      // tx data in high 16, rx in low 16
    boost::uint16_t tx_io;
    boost::uint16_t rx_ddr;     // 32 bits, 1 means output. tx in high 16, rx in low 16
    boost::uint16_t tx_ddr;
    boost::uint16_t tx_sel_low; // 16 2-bit fields select which source goes to TX DB
    boost::uint16_t tx_sel_high;
    boost::uint16_t rx_sel_low; // 16 2-bit fields select which source goes to RX DB
    boost::uint16_t rx_sel_high;
};

// each 2-bit sel field is layed out this way
#define GPIO_SEL_SW        0 // if pin is an output, set by software in the io reg
#define GPIO_SEL_ATR       1 // if pin is an output, set by ATR logic
#define GPIO_SEL_DEBUG_0   2 // if pin is an output, debug lines from FPGA fabric
#define GPIO_SEL_DEBUG_1   3 // if pin is an output, debug lines from FPGA fabric

//#define gpio_base ((gpio_regs_t *) GPIO_BASE)

#endif /* INCLUDED_USRP_E_REGS_HPP */
