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

#define USRP2_MISC_OUTPUT_BASE  0xD400
#define USRP2_GPIO_BASE         0xC800
#define USRP2_ATR_BASE          0xE400
#define USRP2_BP_STATUS_BASE    0xCC00

#define USRP2P_MISC_OUTPUT_BASE 0x5000
#define USRP2P_GPIO_BASE        0x6200
#define USRP2P_ATR_BASE         0x6800
#define USRP2P_BP_STATUS_BASE   0x6300

typedef struct {
    int sr_misc;
    int sr_tx_prot_eng;
    int sr_rx_prot_eng;
    int sr_buffer_pool_ctrl;
    int sr_udp_sm;
    int sr_tx_dsp;
    int sr_tx_ctrl;
    int sr_rx_dsp;
    int sr_rx_ctrl;
    int sr_time64;
    int sr_simtimer;
    int sr_last;
    int misc_ctrl_clock;
    int misc_ctrl_serdes;
    int misc_ctrl_adc;
    int misc_ctrl_leds;
    int misc_ctrl_phy;
    int misc_ctrl_dbg_mux;
    int misc_ctrl_ram_page;
    int misc_ctrl_flush_icache;
    int misc_ctrl_led_src;
    int time64_secs; // value to set absolute secs to on next PPS
    int time64_ticks; // value to set absolute ticks to on next PPS
    int time64_flags; // flags -- see chart below
    int time64_imm; // set immediate (0=latch on next pps, 1=latch immediate, default=0)
    int time64_tps; // ticks per second rollover count
    int time64_mimo_sync;
    int status;
    int time64_secs_rb_imm;
    int time64_ticks_rb_imm;
    int time64_secs_rb_pps;
    int time64_ticks_rb_pps;
    int compat_num_rb;
    int dsp_tx_freq;
    int dsp_tx_scale_iq;
    int dsp_tx_interp_rate;
    int dsp_tx_mux;
    int dsp_rx_freq;
    int dsp_rx_scale_iq;
    int dsp_rx_decim_rate;
    int dsp_rx_dcoffset_i;
    int dsp_rx_dcoffset_q;
    int dsp_rx_mux;
    int gpio_base;
    int gpio_io;
    int gpio_ddr;
    int gpio_tx_sel;
    int gpio_rx_sel;
    int atr_base;
    int atr_idle_txside;
    int atr_idle_rxside;
    int atr_intx_txside;
    int atr_intx_rxside;
    int atr_inrx_txside;
    int atr_inrx_rxside;
    int atr_full_txside;
    int atr_full_rxside;
    int rx_ctrl_stream_cmd;
    int rx_ctrl_time_secs;
    int rx_ctrl_time_ticks;
    int rx_ctrl_clear_overrun;
    int rx_ctrl_vrt_header;
    int rx_ctrl_vrt_stream_id;
    int rx_ctrl_vrt_trailer;
    int rx_ctrl_nsamps_per_pkt;
    int rx_ctrl_nchannels;
    int tx_ctrl_num_chan;
    int tx_ctrl_clear_state;
    int tx_ctrl_report_sid;
    int tx_ctrl_policy;
    int tx_ctrl_cycles_per_up;
    int tx_ctrl_packets_per_up;
} usrp2_regs_t;

extern const usrp2_regs_t usrp2_regs; //the register definitions, set in usrp2_regs.cpp and usrp2p_regs.cpp

usrp2_regs_t usrp2_get_regs(bool);

////////////////////////////////////////////////////
// Settings Bus, Slave #7, Not Byte Addressable!
//
// Output-only from processor point-of-view.
// 1KB of address space (== 256 32-bit write-only regs)


//#define MISC_OUTPUT_BASE        0xD400
//#define TX_PROTOCOL_ENGINE_BASE 0xD480
//#define RX_PROTOCOL_ENGINE_BASE 0xD4C0
//#define BUFFER_POOL_CTRL_BASE   0xD500
//#define LAST_SETTING_REG        0xD7FC  // last valid setting register

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
#define U2_FLAG_MISC_CTRL_SERDES_ENABLE 8
#define U2_FLAG_MISC_CTRL_SERDES_PRBSEN 4
#define U2_FLAG_MISC_CTRL_SERDES_LOOPEN 2
#define U2_FLAG_MISC_CTRL_SERDES_RXEN   1

#define U2_FLAG_MISC_CTRL_ADC_ON  0x0F
#define U2_FLAG_MISC_CTRL_ADC_OFF 0x00

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

//pps flags (see above)
#define U2_FLAG_TIME64_PPS_NEGEDGE (0 << 0)
#define U2_FLAG_TIME64_PPS_POSEDGE (1 << 0)
#define U2_FLAG_TIME64_PPS_SMA     (0 << 1)
#define U2_FLAG_TIME64_PPS_MIMO    (1 << 1)

#define U2_FLAG_TIME64_LATCH_NOW 1
#define U2_FLAG_TIME64_LATCH_NEXT_PPS 0

/////////////////////////////////////////////////
// DSP TX Regs
////////////////////////////////////////////////

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


/////////////////////////////////////////////////
// DSP RX Regs
////////////////////////////////////////////////

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

////////////////////////////////////////////////
// GPIO, Slave 4
////////////////////////////////////////////////

// each 2-bit sel field is layed out this way
#define U2_FLAG_GPIO_SEL_GPIO      0 // if pin is an output, set by GPIO register
#define U2_FLAG_GPIO_SEL_ATR       1 // if pin is an output, set by ATR logic
#define U2_FLAG_GPIO_SEL_DEBUG_0   2 // if pin is an output, debug lines from FPGA fabric
#define U2_FLAG_GPIO_SEL_DEBUG_1   3 // if pin is an output, debug lines from FPGA fabric

///////////////////////////////////////////////////
// ATR Controller, Slave 11
////////////////////////////////////////////////


///////////////////////////////////////////////////
// RX CTRL regs
///////////////////////////////////////////////////
// The following 3 are logically a single command register.
// They are clocked into the underlying fifo when time_ticks is written.
//#define U2_REG_RX_CTRL_STREAM_CMD        _SR_ADDR(SR_RX_CTRL + 0) // {now, chain, num_samples(30)
//#define U2_REG_RX_CTRL_TIME_SECS         _SR_ADDR(SR_RX_CTRL + 1)
//#define U2_REG_RX_CTRL_TIME_TICKS        _SR_ADDR(SR_RX_CTRL + 2)

//#define U2_REG_RX_CTRL_CLEAR_STATE       _SR_ADDR(SR_RX_CTRL + 3)
//#define U2_REG_RX_CTRL_VRT_HEADER        _SR_ADDR(SR_RX_CTRL + 4) // word 0 of packet.  FPGA fills in packet counter
//#define U2_REG_RX_CTRL_VRT_STREAM_ID     _SR_ADDR(SR_RX_CTRL + 5) // word 1 of packet.
//#define U2_REG_RX_CTRL_VRT_TRAILER       _SR_ADDR(SR_RX_CTRL + 6)
//#define U2_REG_RX_CTRL_NSAMPS_PER_PKT    _SR_ADDR(SR_RX_CTRL + 7)
//#define U2_REG_RX_CTRL_NCHANNELS         _SR_ADDR(SR_RX_CTRL + 8) // 1 in basic case, up to 4 for vector sources

///////////////////////////////////////////////////
// TX CTRL regs
///////////////////////////////////////////////////
//#define U2_REG_TX_CTRL_NUM_CHAN          _SR_ADDR(SR_TX_CTRL + 0)
//#define U2_REG_TX_CTRL_CLEAR_STATE       _SR_ADDR(SR_TX_CTRL + 1)
//#define U2_REG_TX_CTRL_REPORT_SID        _SR_ADDR(SR_TX_CTRL + 2)
//#define U2_REG_TX_CTRL_POLICY            _SR_ADDR(SR_TX_CTRL + 3)
//#define U2_REG_TX_CTRL_CYCLES_PER_UP     _SR_ADDR(SR_TX_CTRL + 4)
//#define U2_REG_TX_CTRL_PACKETS_PER_UP    _SR_ADDR(SR_TX_CTRL + 5)

#define U2_FLAG_TX_CTRL_POLICY_WAIT          (0x1 << 0)
#define U2_FLAG_TX_CTRL_POLICY_NEXT_PACKET   (0x1 << 1)
#define U2_FLAG_TX_CTRL_POLICY_NEXT_BURST    (0x1 << 2)

//enable flag for registers: cycles and packets per update packet
#define U2_FLAG_TX_CTRL_UP_ENB              (1ul << 31)

#endif /* INCLUDED_USRP2_REGS_HPP */
