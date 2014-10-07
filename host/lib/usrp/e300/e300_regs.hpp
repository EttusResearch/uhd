//
// Copyright 2012-2014 Ettus Research LLC
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

#ifndef INCLUDED_E300_REGS_HPP
#define INCLUDED_E300_REGS_HPP

#include <boost/cstdint.hpp>

#define TOREG(x) ((x)*4)

#define localparam static const int

localparam SR_TEST       = 7;
localparam SR_SPI        = 8;
localparam SR_GPIO       = 16;
localparam SR_MISC_OUTS  = 24;
localparam SR_READBACK   = 32;
localparam SR_TX_CTRL    = 64;
localparam SR_RX_CTRL    = 96;
localparam SR_TIME       = 128;
localparam SR_RX_DSP     = 144;
localparam SR_TX_DSP     = 184;
localparam SR_LEDS       = 196;
localparam SR_FP_GPIO    = 200;
localparam SR_RX_FRONT   = 208;
localparam SR_TX_FRONT   = 216;
localparam SR_CODEC_IDLE = 250;


localparam RB32_SPI             = 4;
localparam RB64_TIME_NOW        = 8;
localparam RB64_TIME_PPS        = 16;
localparam RB32_TEST            = 24;
localparam RB32_FP_GPIO         = 32;
localparam RB64_CODEC_READBACK  = 40;
localparam RB32_RADIO_NUM       = 48;

localparam ST_RX_ENABLE = 20;
localparam ST_TX_ENABLE = 19;

localparam LED_TXRX_TX = 18;
localparam LED_TXRX_RX = 17;
localparam LED_RX_RX = 16;
localparam VCRX_V2 = 15;
localparam VCRX_V1 = 14;
localparam VCTXRX_V2 = 13;
localparam VCTXRX_V1 = 12;
localparam TX_ENABLEB = 11;
localparam TX_ENABLEA = 10;
localparam RXC_BANDSEL = 8;
localparam RXB_BANDSEL = 6;
localparam RX_BANDSEL = 3;
localparam TX_BANDSEL = 0;

#endif /* INCLUDED_E300_REGS_HPP */
