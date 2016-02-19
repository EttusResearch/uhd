//
// Copyright 2014 Ettus Research LLC
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

#ifndef INCLUDED_DEVICE3_RADIO_REGS_HPP
#define INCLUDED_DEVICE3_RADIO_REGS_HPP

#include <uhd/config.hpp>
#include <boost/cstdint.hpp>

namespace uhd { namespace usrp { namespace device3 { namespace radio {

static UHD_INLINE boost::uint32_t sr_addr(const boost::uint32_t offset)
{
    return offset * 4;
}

static const uint32_t BASE       = 128;

// defined in radio_core_regs.vh
static const uint32_t TIME                 = 128; // time hi - 128, time lo - 129, ctrl - 130
static const uint32_t CLEAR_CMDS           = 131; // Any write to this reg clears the command FIFO
static const uint32_t LOOPBACK             = 132;
static const uint32_t TEST                 = 133;
static const uint32_t CODEC_IDLE           = 134;
static const uint32_t TX_CTRL_ERROR_POLICY = 144;
static const uint32_t RX_CTRL              = 152; // command - 152, time hi - 153, time lo - 154
static const uint32_t RX_CTRL_HALT         = 155;
static const uint32_t RX_CTRL_MAXLEN       = 156;
static const uint32_t RX_CTRL_CLEAR_CMDS   = 157;
static const uint32_t MISC_OUTS            = 160;
static const uint32_t DACSYNC              = 161;
static const uint32_t SPI                  = 168;
static const uint32_t LEDS                 = 176;
static const uint32_t FP_GPIO              = 184;
static const uint32_t GPIO                 = 192;
// NOTE: Upper 32 registers (224-255) are reserved for the output settings bus for use with
//       device specific front end control

// frontend control: needs rethinking TODO
//static const uint32_t TX_FRONT             = BASE + 96;
//static const uint32_t RX_FRONT             = BASE + 112;
//static const uint32_t READBACK             = BASE + 127;

static const uint32_t RB_TIME_NOW        = 0;
static const uint32_t RB_TIME_PPS        = 1;
static const uint32_t RB_TEST            = 2;
static const uint32_t RB_CODEC_READBACK  = 3;
static const uint32_t RB_RADIO_NUM       = 4;
static const uint32_t RB_MISC_IO         = 16;
static const uint32_t RB_SPI             = 17;
static const uint32_t RB_LEDS            = 18;
static const uint32_t RB_DB_GPIO         = 19;
static const uint32_t RB_FP_GPIO         = 20;

}}}} // namespace

#endif /* INCLUDED_DEVICE3_RADIO_REGS_HPP */
