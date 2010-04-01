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
#define TX_PROTOCOL_ENGINE_BASE 0xD480
#define RX_PROTOCOL_ENGINE_BASE 0xD4C0
#define BUFFER_POOL_CTRL_BASE   0xD500
#define LAST_SETTING_REG        0xD7FC  // last valid setting register

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

#define _SR_ADDR(sr)    (MISC_OUTPUT_BASE + (sr) * sizeof(uint32_t))

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
// VITA49 64 bit time (write only)
////////////////////////////////////////////////

#define TIME64_BASE _SR_ADDR(SR_TIME64)

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
typedef struct {
    boost::uint32_t secs;   // value to set absolute secs to on next PPS
    boost::uint32_t ticks;  // value to set absolute ticks to on next PPS
    boost::uint32_t flags;  // flags - see chart above
    boost::uint32_t imm;    // set immediate (0=latch on next pps, 1=latch immediate, default=0)
} sr_time64_t;

//pps flags (see above)
#define PPS_FLAG_NEGEDGE (0 << 0)
#define PPS_FLAG_POSEDGE (1 << 0)
#define PPS_FLAG_SMA     (0 << 1)
#define PPS_FLAG_MIMO    (1 << 1)

#define TIME64_LATCH_NOW 1
#define TIME64_LATCH_NEXT_PPS 0

/////////////////////////////////////////////////
// DSP TX Regs
////////////////////////////////////////////////

#define DSP_TX_BASE _SR_ADDR(SR_TX_DSP)

typedef struct {
    boost::int32_t      freq;
    boost::uint32_t     scale_iq;       // {scale_i,scale_q}
    boost::uint32_t     interp_rate;
    boost::uint32_t     _padding0;      // padding for the tx_mux
                                        //   NOT freq, scale, interp
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
    boost::uint32_t tx_mux;

} dsp_tx_regs_t;

/////////////////////////////////////////////////
// DSP RX Regs
////////////////////////////////////////////////

#define DSP_RX_BASE _SR_ADDR(SR_RX_DSP)

typedef struct {
    boost::int32_t      freq;
    boost::uint32_t     scale_iq;       // {scale_i,scale_q}
    boost::uint32_t     decim_rate;
    boost::uint32_t     dcoffset_i;     // Bit 31 high sets fixed offset mode, using lower 14 bits,
                                        // otherwise it is automatic 
    boost::uint32_t     dcoffset_q;     // Bit 31 high sets fixed offset mode, using lower 14 bits

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
    boost::uint32_t     rx_mux;        // called adc_mux in dsp_core_rx.v

} dsp_rx_regs_t;

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
#define GPIO_SEL_SW        0 // if pin is an output, set by software in the io reg
#define GPIO_SEL_ATR       1 // if pin is an output, set by ATR logic
#define GPIO_SEL_DEBUG_0   2 // if pin is an output, debug lines from FPGA fabric
#define GPIO_SEL_DEBUG_1   3 // if pin is an output, debug lines from FPGA fabric

///////////////////////////////////////////////////
// ATR Controller, Slave 11

#define ATR_BASE  0xE400

typedef struct {
    boost::uint32_t v[16];
} atr_regs_t;

#define ATR_IDLE    0x0 // indicies into v
#define ATR_TX      0x1
#define ATR_RX      0x2
#define ATR_FULL    0x3

#endif /* INCLUDED_USRP2_REGS_HPP */
