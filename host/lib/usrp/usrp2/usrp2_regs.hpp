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

#ifndef INCLUDED_USRP2_REGS_HPP
#define INCLUDED_USRP2_REGS_HPP

#include <boost/cstdint.hpp>

////////////////////////////////////////////////
// GPIO, Slave 4
//
// These go to the daughterboard i/o pins

#define GPIO_BASE 0xC800

typedef struct {
    boost::uint32_t    io;       // tx data in high 16, rx in low 16
    boost::uint32_t    ddr;      // 32 bits, 1 means output. tx in high 16, rx in low 16
    boost::uint32_t    tx_sel;   // 16 2-bit fields select which source goes to TX DB
    boost::uint32_t    rx_sel;   // 16 2-bit fields select which source goes to RX DB
} gpio_regs_t;

// each 2-bit sel field is layed out this way
#define GPIO_SEL_SW	   0 // if pin is an output, set by software in the io reg
#define	GPIO_SEL_ATR	   1 // if pin is an output, set by ATR logic
#define	GPIO_SEL_DEBUG_0   2 // if pin is an output, debug lines from FPGA fabric
#define	GPIO_SEL_DEBUG_1   3 // if pin is an output, debug lines from FPGA fabric

///////////////////////////////////////////////////
// ATR Controller, Slave 11

#define ATR_BASE  0xE400

typedef struct {
    boost::uint32_t	v[16];
} atr_regs_t;

#define	ATR_IDLE	0x0	// indicies into v
#define ATR_TX		0x1
#define	ATR_RX		0x2
#define	ATR_FULL	0x3

#endif /* INCLUDED_USRP2_REGS_HPP */
