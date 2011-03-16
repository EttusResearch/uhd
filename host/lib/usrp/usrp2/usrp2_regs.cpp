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

#include "usrp2_regs.hpp"
#include "usrp2_iface.hpp"

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

int sr_addr(int misc_output_base, int sr) {
    return misc_output_base + 4 * sr;
}

////////////////////////////////////////////////////////////////////////
// Slave bases
////////////////////////////////////////////////////////////////////////
#define USRP2_MISC_OUTPUT_BASE  0xD400
#define USRP2_GPIO_BASE         0xC800
#define USRP2_ATR_BASE          0xE400
#define USRP2_BP_STATUS_BASE    0xCC00

#define USRP2P_MISC_OUTPUT_BASE 0x5000
#define USRP2P_GPIO_BASE        0x6200
#define USRP2P_ATR_BASE         0x6800
#define USRP2P_BP_STATUS_BASE   0x6300

usrp2_regs_t usrp2_get_regs(bool use_n2xx_map) {

  //how about you just make this dependent on hw_rev instead of doing the init before main, and give up the const globals, since the application won't ever need both.
  const int misc_output_base = (use_n2xx_map) ? USRP2P_MISC_OUTPUT_BASE : USRP2_MISC_OUTPUT_BASE,
            gpio_base        = (use_n2xx_map) ? USRP2P_GPIO_BASE        : USRP2_GPIO_BASE,
            atr_base         = (use_n2xx_map) ? USRP2P_ATR_BASE         : USRP2_ATR_BASE,
            bp_base          = (use_n2xx_map) ? USRP2P_BP_STATUS_BASE   : USRP2_BP_STATUS_BASE;

  usrp2_regs_t x;
  x.misc_ctrl_clock = sr_addr(misc_output_base, 0);
  x.misc_ctrl_serdes = sr_addr(misc_output_base, 1);
  x.misc_ctrl_adc = sr_addr(misc_output_base, 2);
  x.misc_ctrl_leds = sr_addr(misc_output_base, 3);
  x.misc_ctrl_phy = sr_addr(misc_output_base, 4);
  x.misc_ctrl_dbg_mux = sr_addr(misc_output_base, 5);
  x.misc_ctrl_ram_page = sr_addr(misc_output_base, 6);
  x.misc_ctrl_flush_icache = sr_addr(misc_output_base, 7);
  x.misc_ctrl_led_src = sr_addr(misc_output_base, 8);
  x.time64_secs = sr_addr(misc_output_base, SR_TIME64 + 0);
  x.time64_ticks = sr_addr(misc_output_base, SR_TIME64 + 1);
  x.time64_flags = sr_addr(misc_output_base, SR_TIME64 + 2);
  x.time64_imm = sr_addr(misc_output_base, SR_TIME64 + 3);
  x.time64_tps = sr_addr(misc_output_base, SR_TIME64 + 4);
  x.time64_mimo_sync = sr_addr(misc_output_base, SR_TIME64 + 5);
  x.status = bp_base + 4*8;
  x.time64_secs_rb_imm = bp_base + 4*10;
  x.time64_ticks_rb_imm = bp_base + 4*11;
  x.compat_num_rb = bp_base + 4*12;
  x.time64_secs_rb_pps = bp_base + 4*14;
  x.time64_ticks_rb_pps = bp_base + 4*15;
  x.dsp_tx_freq = sr_addr(misc_output_base, SR_TX_DSP + 0);
  x.dsp_tx_scale_iq = sr_addr(misc_output_base, SR_TX_DSP + 1);
  x.dsp_tx_interp_rate = sr_addr(misc_output_base, SR_TX_DSP + 2);
  x.dsp_tx_mux = sr_addr(misc_output_base, SR_TX_DSP + 4);
  x.dsp_rx[0].freq = sr_addr(misc_output_base, SR_RX_DSP0 + 0);
  x.dsp_rx[0].scale_iq = sr_addr(misc_output_base, SR_RX_DSP0 + 1);
  x.dsp_rx[0].decim_rate = sr_addr(misc_output_base, SR_RX_DSP0 + 2);
  x.dsp_rx[0].dcoffset_i = sr_addr(misc_output_base, SR_RX_DSP0 + 3);
  x.dsp_rx[0].dcoffset_q = sr_addr(misc_output_base, SR_RX_DSP0 + 4);
  x.dsp_rx[0].mux = sr_addr(misc_output_base, SR_RX_DSP0 + 5);
  x.dsp_rx[1].freq = sr_addr(misc_output_base, SR_RX_DSP1 + 0);
  x.dsp_rx[1].scale_iq = sr_addr(misc_output_base, SR_RX_DSP1 + 1);
  x.dsp_rx[1].decim_rate = sr_addr(misc_output_base, SR_RX_DSP1 + 2);
  x.dsp_rx[1].dcoffset_i = sr_addr(misc_output_base, SR_RX_DSP1 + 3);
  x.dsp_rx[1].dcoffset_q = sr_addr(misc_output_base, SR_RX_DSP1 + 4);
  x.dsp_rx[1].mux = sr_addr(misc_output_base, SR_RX_DSP1 + 5);
  x.gpio_io = gpio_base + 0;
  x.gpio_ddr = gpio_base + 4;
  x.gpio_tx_sel = gpio_base + 8;
  x.gpio_rx_sel = gpio_base + 12;
  x.atr_idle_txside = atr_base + 0;
  x.atr_idle_rxside = atr_base + 2;
  x.atr_intx_txside = atr_base + 4;
  x.atr_intx_rxside = atr_base + 6;
  x.atr_inrx_txside = atr_base + 8;
  x.atr_inrx_rxside = atr_base + 10;
  x.atr_full_txside = atr_base + 12;
  x.atr_full_rxside = atr_base + 14;
  x.rx_ctrl[0].stream_cmd = sr_addr(misc_output_base, SR_RX_CTRL0 + 0);
  x.rx_ctrl[0].time_secs = sr_addr(misc_output_base, SR_RX_CTRL0 + 1);
  x.rx_ctrl[0].time_ticks = sr_addr(misc_output_base, SR_RX_CTRL0 + 2);
  x.rx_ctrl[0].clear_overrun = sr_addr(misc_output_base, SR_RX_CTRL0 + 3);
  x.rx_ctrl[0].vrt_header = sr_addr(misc_output_base, SR_RX_CTRL0 + 4);
  x.rx_ctrl[0].vrt_stream_id = sr_addr(misc_output_base, SR_RX_CTRL0 + 5);
  x.rx_ctrl[0].vrt_trailer = sr_addr(misc_output_base, SR_RX_CTRL0 + 6);
  x.rx_ctrl[0].nsamps_per_pkt = sr_addr(misc_output_base, SR_RX_CTRL0 + 7);
  x.rx_ctrl[0].nchannels = sr_addr(misc_output_base, SR_RX_CTRL0 + 8);
  x.rx_ctrl[1].stream_cmd = sr_addr(misc_output_base, SR_RX_CTRL1 + 0);
  x.rx_ctrl[1].time_secs = sr_addr(misc_output_base, SR_RX_CTRL1 + 1);
  x.rx_ctrl[1].time_ticks = sr_addr(misc_output_base, SR_RX_CTRL1 + 2);
  x.rx_ctrl[1].clear_overrun = sr_addr(misc_output_base, SR_RX_CTRL1 + 3);
  x.rx_ctrl[1].vrt_header = sr_addr(misc_output_base, SR_RX_CTRL1 + 4);
  x.rx_ctrl[1].vrt_stream_id = sr_addr(misc_output_base, SR_RX_CTRL1 + 5);
  x.rx_ctrl[1].vrt_trailer = sr_addr(misc_output_base, SR_RX_CTRL1 + 6);
  x.rx_ctrl[1].nsamps_per_pkt = sr_addr(misc_output_base, SR_RX_CTRL1 + 7);
  x.rx_ctrl[1].nchannels = sr_addr(misc_output_base, SR_RX_CTRL1 + 8);
  x.tx_ctrl_num_chan = sr_addr(misc_output_base, SR_TX_CTRL + 0);
  x.tx_ctrl_clear_state = sr_addr(misc_output_base, SR_TX_CTRL + 1);
  x.tx_ctrl_report_sid = sr_addr(misc_output_base, SR_TX_CTRL + 2);
  x.tx_ctrl_policy = sr_addr(misc_output_base, SR_TX_CTRL + 3);
  x.tx_ctrl_cycles_per_up = sr_addr(misc_output_base, SR_TX_CTRL + 4);
  x.tx_ctrl_packets_per_up = sr_addr(misc_output_base, SR_TX_CTRL + 5);

  return x;
}
