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

#include <stdint.h>
#include <uhd/config.hpp>

namespace uhd { namespace usrp { namespace e300 { namespace radio {

static UHD_INLINE uint32_t sr_addr(const uint32_t offset)
{
    return offset * 4;
}

static const uint32_t DACSYNC    = 5;
static const uint32_t LOOPBACK   = 6;
static const uint32_t TEST       = 7;
static const uint32_t SPI        = 8;
static const uint32_t GPIO       = 16;
static const uint32_t MISC_OUTS  = 24;
static const uint32_t READBACK   = 32;
static const uint32_t TX_CTRL    = 64;
static const uint32_t RX_CTRL    = 96;
static const uint32_t TIME       = 128;
static const uint32_t RX_DSP     = 144;
static const uint32_t TX_DSP     = 184;
static const uint32_t LEDS       = 195;
static const uint32_t FP_GPIO    = 200;
static const uint32_t RX_FRONT   = 208;
static const uint32_t TX_FRONT   = 216;
static const uint32_t CODEC_IDLE = 250;

static const uint32_t RB32_GPIO            = 0;
static const uint32_t RB32_SPI             = 4;
static const uint32_t RB64_TIME_NOW        = 8;
static const uint32_t RB64_TIME_PPS        = 16;
static const uint32_t RB32_TEST            = 24;
static const uint32_t RB32_RX              = 28;
static const uint32_t RB32_FP_GPIO         = 32;
static const uint32_t RB32_MISC_INS        = 36;
static const uint32_t RB64_CODEC_READBACK  = 40;
static const uint32_t RB32_RADIO_NUM       = 48;

}}}} // namespace

#define localparam static const int

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
