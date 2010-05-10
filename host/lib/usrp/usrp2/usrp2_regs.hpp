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

////////////////////////////////////////////////////
// Settings Bus, Slave #7, Not Byte Addressable!
//
// Output-only from processor point-of-view.
// 1KB of address space (== 256 32-bit write-only regs)


#define MISC_OUTPUT_BASE        0xD400
//#define TX_PROTOCOL_ENGINE_BASE 0xD480
//#define RX_PROTOCOL_ENGINE_BASE 0xD4C0
//#define BUFFER_POOL_CTRL_BASE   0xD500
//#define LAST_SETTING_REG        0xD7FC  // last valid setting register

#define SR_MISC 0
#define SR_TX_PROT_ENG 32
#define SR_RX_PROT_ENG 48
#define SR_BUFFER_POOL_CTRL 64
#define SR_UDP_SM 96
#define SR_TX_DSP 208
#define SR_TX_CTRL 224
#define SR_RX_DSP 160
#define SR_RX_CTRL 176
#define SR_TIME64 192
#define SR_SIMTIMER 198
#define SR_LAST 255

#define _SR_ADDR(sr)    (MISC_OUTPUT_BASE + (sr) * sizeof(boost::uint32_t))

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

/////////////////////////////////////////////////
// Misc Control
////////////////////////////////////////////////
#define FR_MISC_CTRL_CLOCK           _SR_ADDR(0)
#define FR_MISC_CTRL_SERDES          _SR_ADDR(1)
#define FR_MISC_CTRL_ADC             _SR_ADDR(2)
#define FR_MISC_CTRL_LEDS            _SR_ADDR(3)
#define FR_MISC_CTRL_PHY             _SR_ADDR(4) // LSB is reset line to eth phy
#define FR_MISC_CTRL_DBG_MUX         _SR_ADDR(5)
#define FR_MISC_CTRL_RAM_PAGE        _SR_ADDR(6) // FIXME should go somewhere else...
#define FR_MISC_CTRL_FLUSH_ICACHE    _SR_ADDR(7) // Flush the icache
#define FR_MISC_CTRL_LED_SRC         _SR_ADDR(8) // HW or SW control for LEDs

#define FRF_MISC_CTRL_SERDES_ENABLE 8
#define FRF_MISC_CTRL_SERDES_PRBSEN 4
#define FRF_MISC_CTRL_SERDES_LOOPEN 2
#define FRF_MISC_CTRL_SERDES_RXEN   1

#define FRF_MISC_CTRL_ADC_ON  0x0F
#define FRF_MISC_CTRL_ADC_OFF 0x00

/////////////////////////////////////////////////
// VITA49 64 bit time (write only)
////////////////////////////////////////////////
  /*!
   * \brief Time 64 flags
   *
   * <pre>
   *
   *    3                   2                   1                       
   *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   * +-----------------------------------------------------------+-+-+
   * |                                                           |S|P|
   * +-----------------------------------------------------------+-+-+
   *
   * P - PPS edge selection (0=negedge, 1=posedge, default=0)
   * S - Source (0=sma, 1=mimo, 0=default)
   *
   * </pre>
   */
#define FR_TIME64_SECS  _SR_ADDR(SR_TIME64 + 0)  // value to set absolute secs to on next PPS
#define FR_TIME64_TICKS _SR_ADDR(SR_TIME64 + 1)  // value to set absolute ticks to on next PPS
#define FR_TIME64_FLAGS _SR_ADDR(SR_TIME64 + 2)  // flags - see chart above
#define FR_TIME64_IMM   _SR_ADDR(SR_TIME64 + 3) // set immediate (0=latch on next pps, 1=latch immediate, default=0)

//pps flags (see above)
#define FRF_TIME64_PPS_NEGEDGE (0 << 0)
#define FRF_TIME64_PPS_POSEDGE (1 << 0)
#define FRF_TIME64_PPS_SMA     (0 << 1)
#define FRF_TIME64_PPS_MIMO    (1 << 1)

#define FRF_TIME64_LATCH_NOW 1
#define FRF_TIME64_LATCH_NEXT_PPS 0

/////////////////////////////////////////////////
// DSP TX Regs
////////////////////////////////////////////////
#define FR_DSP_TX_FREQ         _SR_ADDR(SR_TX_DSP + 0)
#define FR_DSP_TX_SCALE_IQ     _SR_ADDR(SR_TX_DSP + 1) // {scale_i,scale_q}
#define FR_DSP_TX_INTERP_RATE  _SR_ADDR(SR_TX_DSP + 2)

  /*!
   * \brief output mux configuration.
   *
   * <pre>
   *     3                   2                   1                       
   *   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   *  +-------------------------------+-------+-------+-------+-------+
   *  |                                               | DAC1  |  DAC0 |
   *  +-------------------------------+-------+-------+-------+-------+
   * 
   *  There are N DUCs (1 now) with complex inputs and outputs.
   *  There are two DACs.
   * 
   *  Each 4-bit DACx field specifies the source for the DAC
   *  Each subfield is coded like this: 
   * 
   *     3 2 1 0
   *    +-------+
   *    |   N   |
   *    +-------+
   * 
   *  N specifies which DUC output is connected to this DAC.
   * 
   *   N   which interp output
   *  ---  -------------------
   *   0   DUC 0 I
   *   1   DUC 0 Q
   *   2   DUC 1 I
   *   3   DUC 1 Q
   *   F   All Zeros
   *   
   * The default value is 0x10
   * </pre>
   */
#define FR_DSP_TX_MUX  _SR_ADDR(SR_TX_DSP + 4)

/////////////////////////////////////////////////
// DSP RX Regs
////////////////////////////////////////////////
#define FR_DSP_RX_FREQ         _SR_ADDR(SR_RX_DSP + 0)
#define FR_DSP_RX_SCALE_IQ     _SR_ADDR(SR_RX_DSP + 1) // {scale_i,scale_q}
#define FR_DSP_RX_DECIM_RATE   _SR_ADDR(SR_RX_DSP + 2)
#define FR_DSP_RX_DCOFFSET_I   _SR_ADDR(SR_RX_DSP + 3) // Bit 31 high sets fixed offset mode, using lower 14 bits,
                                                       // otherwise it is automatic 
#define FR_DSP_RX_DCOFFSET_Q   _SR_ADDR(SR_RX_DSP + 4) // Bit 31 high sets fixed offset mode, using lower 14 bits
  /*!
   * \brief input mux configuration.
   *
   * This determines which ADC (or constant zero) is connected to 
   * each DDC input.  There are N DDCs (1 now).  Each has two inputs.
   *
   * <pre>
   * Mux value:
   *
   *    3                   2                   1                       
   *  1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
   * +-------+-------+-------+-------+-------+-------+-------+-------+
   * |                                                       |Q0 |I0 |
   * +-------+-------+-------+-------+-------+-------+-------+-------+
   *
   * Each 2-bit I field is either 00 (A/D A), 01 (A/D B) or 1X (const zero)
   * Each 2-bit Q field is either 00 (A/D A), 01 (A/D B) or 1X (const zero)
   *
   * The default value is 0x4
   * </pre>
   */
#define FR_DSP_RX_MUX  _SR_ADDR(SR_RX_DSP + 5)         // called adc_mux in dsp_core_rx.v

////////////////////////////////////////////////
// GPIO, Slave 4
////////////////////////////////////////////////
//
// These go to the daughterboard i/o pins
//
#define FR_GPIO_BASE 0xC800

#define FR_GPIO_IO         FR_GPIO_BASE + 0  // 32 bits, gpio io pins (tx high 16 bits, rx low 16 bits)
#define FR_GPIO_DDR        FR_GPIO_BASE + 4  // 32 bits, gpio ddr, 1 means output (tx high 16 bits, rx low 16 bits)
#define FR_GPIO_TX_SEL     FR_GPIO_BASE + 8  // 16 2-bit fields select which source goes to TX DB
#define FR_GPIO_RX_SEL     FR_GPIO_BASE + 12 // 16 2-bit fields select which source goes to RX DB

// each 2-bit sel field is layed out this way
#define FRF_GPIO_SEL_SW        0 // if pin is an output, set by software in the io reg
#define FRF_GPIO_SEL_ATR       1 // if pin is an output, set by ATR logic
#define FRF_GPIO_SEL_DEBUG_0   2 // if pin is an output, debug lines from FPGA fabric
#define FRF_GPIO_SEL_DEBUG_1   3 // if pin is an output, debug lines from FPGA fabric

///////////////////////////////////////////////////
// ATR Controller, Slave 11
////////////////////////////////////////////////
#define FR_ATR_BASE  0xE400

#define FR_ATR_IDLE_TXSIDE  FR_ATR_BASE + 0
#define FR_ATR_IDLE_RXSIDE  FR_ATR_BASE + 2
#define FR_ATR_INTX_TXSIDE  FR_ATR_BASE + 4
#define FR_ATR_INTX_RXSIDE  FR_ATR_BASE + 6
#define FR_ATR_INRX_TXSIDE  FR_ATR_BASE + 8
#define FR_ATR_INRX_RXSIDE  FR_ATR_BASE + 10
#define FR_ATR_FULL_TXSIDE  FR_ATR_BASE + 12
#define FR_ATR_FULL_RXSIDE  FR_ATR_BASE + 14

///////////////////////////////////////////////////
// VITA RX CTRL regs
///////////////////////////////////////////////////
// The following 3 are logically a single command register.
// They are clocked into the underlying fifo when time_ticks is written.
#define FR_RX_CTRL_STREAM_CMD        _SR_ADDR(SR_RX_CTRL + 0) // {now, chain, num_samples(30)
#define FR_RX_CTRL_TIME_SECS         _SR_ADDR(SR_RX_CTRL + 1)
#define FR_RX_CTRL_TIME_TICKS        _SR_ADDR(SR_RX_CTRL + 2)

#define FR_RX_CTRL_CLEAR_OVERRUN     _SR_ADDR(SR_RX_CTRL + 3) // write anything to clear overrun
#define FR_RX_CTRL_VRT_HEADER        _SR_ADDR(SR_RX_CTRL + 4) // word 0 of packet.  FPGA fills in packet counter
#define FR_RX_CTRL_VRT_STREAM_ID     _SR_ADDR(SR_RX_CTRL + 5) // word 1 of packet.
#define FR_RX_CTRL_VRT_TRAILER       _SR_ADDR(SR_RX_CTRL + 6)
#define FR_RX_CTRL_NSAMPS_PER_PKT    _SR_ADDR(SR_RX_CTRL + 7)
#define FR_RX_CTRL_NCHANNELS         _SR_ADDR(SR_RX_CTRL + 8) // 1 in basic case, up to 4 for vector sources

//helpful macros for dealing with stream cmd
#define FR_RX_CTRL_MAX_SAMPS_PER_CMD 0x1fffffff
#define FR_RX_CTRL_MAKE_CMD(nsamples, now, chain, reload) \
  ((((now) & 0x1) << 31) | (((chain) & 0x1) << 30) | (((reload) & 0x1) << 29) | ((nsamples) & 0x1fffffff))

#endif /* INCLUDED_USRP2_REGS_HPP */
