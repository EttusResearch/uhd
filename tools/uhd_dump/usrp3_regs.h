//
// Copyright 2013-2014 Ettus Research LLC
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

const struct radio_ctrl_names reg_list[] =
  {
    {5,   "SR_DACSYNC"},
    {6,   "SR_LOOPBACK"},
    {7,   "SR_TEST"},
    {8,   "SR_SPI_DIVIDER"},
    {9,   "SR_SPI_CONFIG"},
    {10,  "SR_SPI_DATA"},
    {16,  "SR_GPIO_IDLE"},
    {17,  "SR_GPIO_RX"},
    {18,  "SR_GPIO_TX"},
    {19,  "SR_GPIO_FDX"},
    {20,  "SR_GPIO_DDR"},
    {24,  "SR_MISC_OUTS"},
    {32,  "SR_READBACK"},
    {64,  "SR_ERROR_POLICY"},
    {66,  "SR_CYCLES"},
    {67,  "SR_PACKETS"},
    {96,  "SR_RX_CTRL_CMD"},
    {97,  "SR_RX_CTRL_TIME_H"},
    {98,  "SR_RX_CTRL_TIME_L"},
    {99,  "SR_RX_CTRL_RX_HALT"},
    {100, "SR_RX_CTRL_MAXLEN"},
    {101, "SR_RX_CTRL_SID"},
    {102, "SR_RX_CTRL_WINDOW_SIZE"},
    {103, "SR_RX_CTRL_WINDOW_ENABLE"},
    {128, "SR_TIME_HI"},
    {129, "SR_TIME_LO"},
    {130, "SR_TIME_CTRL"},
    {144, "SR_RX_DSP_0"},
    {145, "SR_RX_DSP_1"},
    {146, "SR_RX_DSP_2"},
    {147, "SR_RX_DSP_3"},
    {184, "SR_TX_DSP_0"},
    {185, "SR_TX_DSP_1"},
    {186, "SR_TX_DSP_2"},
    {196, "SR_LEDS_IDLE"},
    {197, "SR_LEDS_RX"},
    {198, "SR_LEDS_TX"},
    {199, "SR_LEDS_FDX"},
    {200, "SR_LEDS_DDR"},
    {208, "SR_SWAP_IQ"},
    {209, "SR_MAG_CORR"},
    {210, "SR_PHASE_CORR"},
    {211, "SR_RX_DC_OFFSET_I"},
    {212, "SR_RX_DC_OFFSET_Q"},
    {999, "NOT FOUND"}
  }
;
    
