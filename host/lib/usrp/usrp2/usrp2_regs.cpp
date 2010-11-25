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

#include "usrp2_regs.hpp"
#include "usrp2_iface.hpp"

int sr_addr(int misc_output_base, int sr) {
	return misc_output_base + 4 * sr;
}

usrp2_regs_t usrp2_get_regs(bool use_n2xx_map) {

  //how about you just make this dependent on hw_rev instead of doing the init before main, and give up the const globals, since the application won't ever need both.
  const int misc_output_base = (use_n2xx_map) ? USRP2P_MISC_OUTPUT_BASE : USRP2_MISC_OUTPUT_BASE,
            gpio_base        = (use_n2xx_map) ? USRP2P_GPIO_BASE        : USRP2_GPIO_BASE,
            atr_base         = (use_n2xx_map) ? USRP2P_ATR_BASE         : USRP2_ATR_BASE,
            bp_base          = (use_n2xx_map) ? USRP2P_BP_STATUS_BASE   : USRP2_BP_STATUS_BASE;

  usrp2_regs_t x;
  x.sr_misc = 0;
  x.sr_tx_prot_eng = 32;
  x.sr_rx_prot_eng = 48;
  x.sr_buffer_pool_ctrl = 64;
  x.sr_udp_sm = 96;
  x.sr_tx_dsp = 208;
  x.sr_tx_ctrl = 224;
  x.sr_rx_dsp = 160;
  x.sr_rx_ctrl = 176;
  x.sr_time64 = 192;
  x.sr_simtimer = 198;
  x.sr_last = 255;
  x.misc_ctrl_clock = sr_addr(misc_output_base, 0);
  x.misc_ctrl_serdes = sr_addr(misc_output_base, 1);
  x.misc_ctrl_adc = sr_addr(misc_output_base, 2);
  x.misc_ctrl_leds = sr_addr(misc_output_base, 3);
  x.misc_ctrl_phy = sr_addr(misc_output_base, 4);
  x.misc_ctrl_dbg_mux = sr_addr(misc_output_base, 5);
  x.misc_ctrl_ram_page = sr_addr(misc_output_base, 6);
  x.misc_ctrl_flush_icache = sr_addr(misc_output_base, 7);
  x.misc_ctrl_led_src = sr_addr(misc_output_base, 8);
  x.time64_secs = sr_addr(misc_output_base, x.sr_time64 + 0);
  x.time64_ticks = sr_addr(misc_output_base, x.sr_time64 + 1);
  x.time64_flags = sr_addr(misc_output_base, x.sr_time64 + 2);
  x.time64_imm = sr_addr(misc_output_base, x.sr_time64 + 3);
  x.time64_tps = sr_addr(misc_output_base, x.sr_time64 + 4);
  x.status = bp_base + 4*8;
  x.time64_secs_rb = bp_base + 4*10;
  x.time64_ticks_rb = bp_base + 4*11;
  x.compat_num_rb = bp_base + 4*12;
  x.dsp_tx_freq = sr_addr(misc_output_base, x.sr_tx_dsp + 0);
  x.dsp_tx_scale_iq = sr_addr(misc_output_base, x.sr_tx_dsp + 1);
  x.dsp_tx_interp_rate = sr_addr(misc_output_base, x.sr_tx_dsp + 2);
  x.dsp_tx_mux = sr_addr(misc_output_base, x.sr_tx_dsp + 4);
  x.dsp_rx_freq = sr_addr(misc_output_base, x.sr_rx_dsp + 0);
  x.dsp_rx_scale_iq = sr_addr(misc_output_base, x.sr_rx_dsp + 1);
  x.dsp_rx_decim_rate = sr_addr(misc_output_base, x.sr_rx_dsp + 2);
  x.dsp_rx_dcoffset_i = sr_addr(misc_output_base, x.sr_rx_dsp + 3);
  x.dsp_rx_dcoffset_q = sr_addr(misc_output_base, x.sr_rx_dsp + 4);
  x.dsp_rx_mux = sr_addr(misc_output_base, x.sr_rx_dsp + 5);
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
  x.rx_ctrl_stream_cmd = sr_addr(misc_output_base, x.sr_rx_ctrl + 0);
  x.rx_ctrl_time_secs = sr_addr(misc_output_base, x.sr_rx_ctrl + 1);
  x.rx_ctrl_time_ticks = sr_addr(misc_output_base, x.sr_rx_ctrl + 2);
  x.rx_ctrl_clear_overrun = sr_addr(misc_output_base, x.sr_rx_ctrl + 3);
  x.rx_ctrl_vrt_header = sr_addr(misc_output_base, x.sr_rx_ctrl + 4);
  x.rx_ctrl_vrt_stream_id = sr_addr(misc_output_base, x.sr_rx_ctrl + 5);
  x.rx_ctrl_vrt_trailer = sr_addr(misc_output_base, x.sr_rx_ctrl + 6);
  x.rx_ctrl_nsamps_per_pkt = sr_addr(misc_output_base, x.sr_rx_ctrl + 7);
  x.rx_ctrl_nchannels = sr_addr(misc_output_base, x.sr_rx_ctrl + 8);
  x.tx_ctrl_num_chan = sr_addr(misc_output_base, x.sr_tx_ctrl + 0);
  x.tx_ctrl_clear_state = sr_addr(misc_output_base, x.sr_tx_ctrl + 1);
  x.tx_ctrl_report_sid = sr_addr(misc_output_base, x.sr_tx_ctrl + 2);
  x.tx_ctrl_policy = sr_addr(misc_output_base, x.sr_tx_ctrl + 3);
  x.tx_ctrl_cycles_per_up = sr_addr(misc_output_base, x.sr_tx_ctrl + 4);
  x.tx_ctrl_packets_per_up = sr_addr(misc_output_base, x.sr_tx_ctrl + 5);

  return x;
}
