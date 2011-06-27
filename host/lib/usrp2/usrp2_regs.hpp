//
// Copyright 2010-2011 Ettus Research LLC
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

////////////////////////////////////////////////////////////////////////
// Define slave bases
////////////////////////////////////////////////////////////////////////
#define ROUTER_RAM_BASE     0x4000
#define SPI_BASE            0x5000
#define I2C_BASE            0x5400
#define GPIO_BASE           0x5800
#define READBACK_BASE       0x5C00
#define ETH_BASE            0x6000
#define SETTING_REGS_BASE   0x7000
#define PIC_BASE            0x8000
#define UART_BASE           0x8800
#define ATR_BASE            0x8C00

////////////////////////////////////////////////////////////////////////
// Setting register offsets
////////////////////////////////////////////////////////////////////////
#define SR_MISC       0   // 7 regs
#define SR_SIMTIMER   8   // 2
#define SR_TIME64    10   // 6
#define SR_BUF_POOL  16   // 4

#define SR_RX_FRONT  24   // 5
#define SR_RX_CTRL0  32   // 9
#define SR_RX_DSP0   48   // 7
#define SR_RX_CTRL1  80   // 9
#define SR_RX_DSP1   96   // 7

#define SR_TX_FRONT 128   // ?
#define SR_TX_CTRL  144   // 6
#define SR_TX_DSP   160   // 5

#define SR_UDP_SM   192   // 64

#define U2_REG_SR_ADDR(sr) (SETTING_REGS_BASE + (4 * (sr)))

/////////////////////////////////////////////////
// SPI Slave Constants
////////////////////////////////////////////////
// Masks for controlling different peripherals
#define SPI_SS_AD9510    1
#define SPI_SS_AD9777    2
#define SPI_SS_RX_DAC    4
#define SPI_SS_RX_ADC    8
#define SPI_SS_RX_DB    16
#define SPI_SS_TX_DAC   32
#define SPI_SS_TX_ADC   64
#define SPI_SS_TX_DB   128
#define SPI_SS_ADS62P44 256 //for usrp2p

/////////////////////////////////////////////////
// Misc Control
////////////////////////////////////////////////
#define U2_REG_MISC_CTRL_CLOCK U2_REG_SR_ADDR(0)
#define U2_REG_MISC_CTRL_SERDES U2_REG_SR_ADDR(1)
#define U2_REG_MISC_CTRL_ADC U2_REG_SR_ADDR(2)
#define U2_REG_MISC_CTRL_LEDS U2_REG_SR_ADDR(3)
#define U2_REG_MISC_CTRL_PHY U2_REG_SR_ADDR(4)
#define U2_REG_MISC_CTRL_DBG_MUX U2_REG_SR_ADDR(5)
#define U2_REG_MISC_CTRL_RAM_PAGE U2_REG_SR_ADDR(6)
#define U2_REG_MISC_CTRL_FLUSH_ICACHE U2_REG_SR_ADDR(7)
#define U2_REG_MISC_CTRL_LED_SRC U2_REG_SR_ADDR(8)

#define U2_FLAG_MISC_CTRL_SERDES_ENABLE 8
#define U2_FLAG_MISC_CTRL_SERDES_PRBSEN 4
#define U2_FLAG_MISC_CTRL_SERDES_LOOPEN 2
#define U2_FLAG_MISC_CTRL_SERDES_RXEN   1

#define U2_FLAG_MISC_CTRL_ADC_ON  0x0F
#define U2_FLAG_MISC_CTRL_ADC_OFF 0x00

/////////////////////////////////////////////////
// VITA49 64 bit time (write only)
////////////////////////////////////////////////
#define U2_REG_TIME64_SECS U2_REG_SR_ADDR(SR_TIME64 + 0)
#define U2_REG_TIME64_TICKS U2_REG_SR_ADDR(SR_TIME64 + 1)
#define U2_REG_TIME64_FLAGS U2_REG_SR_ADDR(SR_TIME64 + 2)
#define U2_REG_TIME64_IMM U2_REG_SR_ADDR(SR_TIME64 + 3)
#define U2_REG_TIME64_TPS U2_REG_SR_ADDR(SR_TIME64 + 4)
#define U2_REG_TIME64_MIMO_SYNC U2_REG_SR_ADDR(SR_TIME64 + 5)

//pps flags (see above)
#define U2_FLAG_TIME64_PPS_NEGEDGE (0 << 0)
#define U2_FLAG_TIME64_PPS_POSEDGE (1 << 0)
#define U2_FLAG_TIME64_PPS_SMA     (0 << 1)
#define U2_FLAG_TIME64_PPS_MIMO    (1 << 1)

#define U2_FLAG_TIME64_LATCH_NOW 1
#define U2_FLAG_TIME64_LATCH_NEXT_PPS 0

/////////////////////////////////////////////////
// Readback regs
////////////////////////////////////////////////
#define U2_REG_STATUS READBACK_BASE + 4*8
#define U2_REG_TIME64_SECS_RB_IMM READBACK_BASE + 4*10
#define U2_REG_TIME64_TICKS_RB_IMM READBACK_BASE + 4*11
#define U2_REG_COMPAT_NUM_RB READBACK_BASE + 4*12
#define U2_REG_IRQ_RB READBACK_BASE + 4*13
#define U2_REG_TIME64_SECS_RB_PPS READBACK_BASE + 4*14
#define U2_REG_TIME64_TICKS_RB_PPS READBACK_BASE + 4*15

/////////////////////////////////////////////////
// RX FE
////////////////////////////////////////////////
#define U2_REG_RX_FE_SWAP_IQ             U2_REG_SR_ADDR(SR_RX_FRONT + 0) //lower bit
#define U2_REG_RX_FE_MAG_CORRECTION      U2_REG_SR_ADDR(SR_RX_FRONT + 1) //18 bits
#define U2_REG_RX_FE_PHASE_CORRECTION    U2_REG_SR_ADDR(SR_RX_FRONT + 2) //18 bits
#define U2_REG_RX_FE_OFFSET_I            U2_REG_SR_ADDR(SR_RX_FRONT + 3) //18 bits
#define U2_REG_RX_FE_OFFSET_Q            U2_REG_SR_ADDR(SR_RX_FRONT + 4) //18 bits

/////////////////////////////////////////////////
// TX FE
////////////////////////////////////////////////
#define U2_REG_TX_FE_DC_OFFSET_I         U2_REG_SR_ADDR(SR_TX_FRONT + 0) //24 bits
#define U2_REG_TX_FE_DC_OFFSET_Q         U2_REG_SR_ADDR(SR_TX_FRONT + 1) //24 bits
#define U2_REG_TX_FE_MAC_CORRECTION      U2_REG_SR_ADDR(SR_TX_FRONT + 2) //18 bits
#define U2_REG_TX_FE_PHASE_CORRECTION    U2_REG_SR_ADDR(SR_TX_FRONT + 3) //18 bits
#define U2_REG_TX_FE_MUX                 U2_REG_SR_ADDR(SR_TX_FRONT + 4) //8 bits (std output = 0x10, reversed = 0x01)

/////////////////////////////////////////////////
// DSP TX Regs
////////////////////////////////////////////////
#define U2_REG_DSP_TX_FREQ U2_REG_SR_ADDR(SR_TX_DSP + 0)
#define U2_REG_DSP_TX_SCALE_IQ U2_REG_SR_ADDR(SR_TX_DSP + 1)
#define U2_REG_DSP_TX_INTERP_RATE U2_REG_SR_ADDR(SR_TX_DSP + 2)

/////////////////////////////////////////////////
// DSP RX Regs
////////////////////////////////////////////////
#define U2_REG_DSP_RX_HELPER(which, offset) ((which == 0)? \
    (U2_REG_SR_ADDR(SR_RX_DSP0 + offset)) : \
    (U2_REG_SR_ADDR(SR_RX_DSP1 + offset)))

#define U2_REG_DSP_RX_FREQ(which)       U2_REG_DSP_RX_HELPER(which, 0)
#define U2_REG_DSP_RX_DECIM(which)      U2_REG_DSP_RX_HELPER(which, 2)
#define U2_REG_DSP_RX_MUX(which)        U2_REG_DSP_RX_HELPER(which, 3)

#define U2_FLAG_DSP_RX_MUX_SWAP_IQ   (1 << 0)
#define U2_FLAG_DSP_RX_MUX_REAL_MODE (1 << 1)

////////////////////////////////////////////////
// GPIO
////////////////////////////////////////////////
#define U2_REG_GPIO_IO GPIO_BASE + 0
#define U2_REG_GPIO_DDR GPIO_BASE + 4
#define U2_REG_GPIO_TX_SEL GPIO_BASE + 8
#define U2_REG_GPIO_RX_SEL GPIO_BASE + 12

// each 2-bit sel field is layed out this way
#define U2_FLAG_GPIO_SEL_GPIO      0 // if pin is an output, set by GPIO register
#define U2_FLAG_GPIO_SEL_ATR       1 // if pin is an output, set by ATR logic
#define U2_FLAG_GPIO_SEL_DEBUG_0   2 // if pin is an output, debug lines from FPGA fabric
#define U2_FLAG_GPIO_SEL_DEBUG_1   3 // if pin is an output, debug lines from FPGA fabric

///////////////////////////////////////////////////
// ATR Controller
////////////////////////////////////////////////
#define U2_REG_ATR_IDLE_TXSIDE ATR_BASE + 0
#define U2_REG_ATR_IDLE_RXSIDE ATR_BASE + 2
#define U2_REG_ATR_INTX_TXSIDE ATR_BASE + 4
#define U2_REG_ATR_INTX_RXSIDE ATR_BASE + 6
#define U2_REG_ATR_INRX_TXSIDE ATR_BASE + 8
#define U2_REG_ATR_INRX_RXSIDE ATR_BASE + 10
#define U2_REG_ATR_FULL_TXSIDE ATR_BASE + 12
#define U2_REG_ATR_FULL_RXSIDE ATR_BASE + 14

///////////////////////////////////////////////////
// RX CTRL regs
///////////////////////////////////////////////////
#define U2_REG_RX_CTRL_HELPER(which, offset) ((which == 0)? \
    (U2_REG_SR_ADDR(SR_RX_CTRL0 + offset)) : \
    (U2_REG_SR_ADDR(SR_RX_CTRL1 + offset)))

#define U2_REG_RX_CTRL_STREAM_CMD(which)     U2_REG_RX_CTRL_HELPER(which, 0)
#define U2_REG_RX_CTRL_TIME_SECS(which)      U2_REG_RX_CTRL_HELPER(which, 1)
#define U2_REG_RX_CTRL_TIME_TICKS(which)     U2_REG_RX_CTRL_HELPER(which, 2)
#define U2_REG_RX_CTRL_CLEAR(which)          U2_REG_RX_CTRL_HELPER(which, 3)
#define U2_REG_RX_CTRL_VRT_HDR(which)        U2_REG_RX_CTRL_HELPER(which, 4)
#define U2_REG_RX_CTRL_VRT_SID(which)        U2_REG_RX_CTRL_HELPER(which, 5)
#define U2_REG_RX_CTRL_VRT_TLR(which)        U2_REG_RX_CTRL_HELPER(which, 6)
#define U2_REG_RX_CTRL_NSAMPS_PP(which)      U2_REG_RX_CTRL_HELPER(which, 7)
#define U2_REG_RX_CTRL_NCHANNELS(which)      U2_REG_RX_CTRL_HELPER(which, 8)

///////////////////////////////////////////////////
// TX CTRL regs
///////////////////////////////////////////////////
#define U2_REG_TX_CTRL_NUM_CHAN U2_REG_SR_ADDR(SR_TX_CTRL + 0)
#define U2_REG_TX_CTRL_CLEAR_STATE U2_REG_SR_ADDR(SR_TX_CTRL + 1)
#define U2_REG_TX_CTRL_REPORT_SID U2_REG_SR_ADDR(SR_TX_CTRL + 2)
#define U2_REG_TX_CTRL_POLICY U2_REG_SR_ADDR(SR_TX_CTRL + 3)
#define U2_REG_TX_CTRL_CYCLES_PER_UP U2_REG_SR_ADDR(SR_TX_CTRL + 4)
#define U2_REG_TX_CTRL_PACKETS_PER_UP U2_REG_SR_ADDR(SR_TX_CTRL + 5)

#define U2_FLAG_TX_CTRL_POLICY_WAIT          (0x1 << 0)
#define U2_FLAG_TX_CTRL_POLICY_NEXT_PACKET   (0x1 << 1)
#define U2_FLAG_TX_CTRL_POLICY_NEXT_BURST    (0x1 << 2)

//enable flag for registers: cycles and packets per update packet
#define U2_FLAG_TX_CTRL_UP_ENB              (1ul << 31)

#endif /* INCLUDED_USRP2_REGS_HPP */
