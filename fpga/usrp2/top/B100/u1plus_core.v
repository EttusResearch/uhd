//
// Copyright 2011 Ettus Research LLC
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



module u1plus_core
  (input clk_fpga, input rst_fpga,
   output [2:0] debug_led, output [31:0] debug, output [1:0] debug_clk,
   output debug_txd, input debug_rxd,
   
   // GPIF
   inout [15:0] gpif_d, input [3:0] gpif_ctl, output [3:0] gpif_rdy,
   output [2:0] gpif_misc, input gpif_clk,
   
   inout db_sda, inout db_scl,
   output sclk, output [15:0] sen, output mosi, input miso,

   input cgen_st_status, input cgen_st_ld, input cgen_st_refmon, output cgen_sync_b, output cgen_ref_sel,   
   inout [15:0] io_tx, inout [15:0] io_rx, 
   output [13:0] tx_i, output [13:0] tx_q, 
   input [11:0] rx_i, input [11:0] rx_q, 
   input pps_in
   );

   localparam TXFIFOSIZE = 11;
   localparam RXFIFOSIZE = 11;

   // 64 total regs in address space
   localparam SR_RX_CTRL0 = 0;       // 9 regs (+0 to +8)
   localparam SR_RX_DSP0 = 10;       // 4 regs (+0 to +3)
   localparam SR_RX_CTRL1 = 16;      // 9 regs (+0 to +8)
   localparam SR_RX_DSP1 = 26;       // 4 regs (+0 to +3)
   localparam SR_TX_CTRL = 32;       // 4 regs (+0 to +3)
   localparam SR_TX_DSP = 38;        // 3 regs (+0 to +2)

   localparam SR_TIME64 = 42;        // 6 regs (+0 to +5)
   localparam SR_RX_FRONT = 48;      // 5 regs (+0 to +4)
   localparam SR_TX_FRONT = 54;      // 5 regs (+0 to +4)

   localparam SR_REG_TEST32 = 60;    // 1 reg
   localparam SR_CLEAR_RX_FIFO = 61; // 1 reg
   localparam SR_CLEAR_TX_FIFO = 62; // 1 reg
   localparam SR_GLOBAL_RESET = 63;  // 1 reg

   wire [7:0]	COMPAT_NUM = 8'd5;
   
   wire 	wb_clk = clk_fpga;
   wire 	wb_rst, global_reset;

   wire 	pps_int;
   wire [63:0] 	vita_time, vita_time_pps;
   reg [15:0] 	reg_leds, reg_cgen_ctrl, reg_test;
   wire [15:0] 	xfer_rate = 0;
   wire [7:0] 	test_rate;
   wire [3:0] 	test_ctrl;
   
   wire [7:0] 	set_addr;
   wire [31:0] 	set_data;
   wire 	set_stb;
   
   wire [31:0]  debug0;
   wire [31:0]  debug1;

   wire [31:0] 	debug_vt;
   wire 	gpif_rst;
   
   reg [7:0] 	frames_per_packet;
   
   wire 	rx_overrun_dsp0, rx_overrun_dsp1, rx_overrun_gpif, tx_underrun_dsp, tx_underrun_gpif;
   wire 	rx_overrun = rx_overrun_gpif | rx_overrun_dsp0 | rx_overrun_dsp1;
   wire 	tx_underrun = tx_underrun_gpif | tx_underrun_dsp;
   
   setting_reg #(.my_addr(SR_GLOBAL_RESET), .width(1)) sr_reset
     (.clk(wb_clk),.rst(wb_rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(),.changed(global_reset));

   reset_sync reset_sync_wb(.clk(wb_clk), .reset_in(rst_fpga | global_reset), .reset_out(wb_rst));
   reset_sync reset_sync_gp(.clk(gpif_clk), .reset_in(rst_fpga | global_reset), .reset_out(gpif_rst));
   wire [15:0] 	test_len;
   
   // /////////////////////////////////////////////////////////////////////////////////////
   // GPIF Slave to Wishbone Master
   localparam dw = 16;
   localparam aw = 11;
   localparam sw = 2;
   
   wire [dw-1:0] m0_dat_mosi, m0_dat_miso;
   wire [aw-1:0] m0_adr;
   wire [sw-1:0] m0_sel;
   wire 	 m0_cyc, m0_stb, m0_we, m0_ack, m0_err, m0_rty;

   wire [31:0] 	 debug_gpif;

   wire [35:0] 	 tx_data, rx_data, tx_err_data;
   wire 	 tx_src_rdy, tx_dst_rdy, rx_src_rdy, rx_dst_rdy, 
		 tx_err_src_rdy, tx_err_dst_rdy;

   wire 	 clear_tx, clear_rx;
   
   setting_reg #(.my_addr(SR_CLEAR_RX_FIFO), .width(1)) sr_clear_rx
     (.clk(wb_clk),.rst(wb_rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(),.changed(clear_rx));

   setting_reg #(.my_addr(SR_CLEAR_TX_FIFO), .width(1)) sr_clear_tx
     (.clk(wb_clk),.rst(wb_rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(),.changed(clear_tx));

   gpif #(.TXFIFOSIZE(TXFIFOSIZE), .RXFIFOSIZE(RXFIFOSIZE))
   gpif (.gpif_clk(gpif_clk), .gpif_rst(gpif_rst), .gpif_d(gpif_d),
	 .gpif_ctl(gpif_ctl), .gpif_rdy(gpif_rdy), .gpif_misc(gpif_misc),
	 
	 .wb_clk(wb_clk), .wb_rst(wb_rst),
	 .wb_adr_o(m0_adr), .wb_dat_mosi(m0_dat_mosi), .wb_dat_miso(m0_dat_miso),
	 .wb_sel_o(m0_sel), .wb_cyc_o(m0_cyc), .wb_stb_o(m0_stb), .wb_we_o(m0_we),
	 .wb_ack_i(m0_ack), .triggers(8'd0),
	 
	 .fifo_clk(wb_clk), .fifo_rst(wb_rst), .clear_tx(clear_tx), .clear_rx(clear_rx),
	 .tx_data_o(tx_data), .tx_src_rdy_o(tx_src_rdy), .tx_dst_rdy_i(tx_dst_rdy),
	 .rx_data_i(rx_data), .rx_src_rdy_i(rx_src_rdy), .rx_dst_rdy_o(rx_dst_rdy),
	 .tx_err_data_i(tx_err_data), .tx_err_src_rdy_i(tx_err_src_rdy), .tx_err_dst_rdy_o(tx_err_dst_rdy),
	 
	 .tx_underrun(tx_underrun_gpif), .rx_overrun(rx_overrun_gpif),

	 .frames_per_packet(frames_per_packet), .test_len(test_len), .test_rate(test_rate), .test_ctrl(test_ctrl),
	 .debug0(debug0), .debug1(debug1));

   // /////////////////////////////////////////////////////////////////////////
   // RX ADC Frontend, does IQ Balance, DC Offset, muxing

   wire [23:0] 	 adc_i, adc_q;  // 24 bits is total overkill here, but it matches u2/u2p
   wire 	 run_rx0, run_rx1;
   
   rx_frontend #(.BASE(SR_RX_FRONT)) rx_frontend
     (.clk(wb_clk),.rst(wb_rst),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .adc_a({rx_i,4'b00}),.adc_ovf_a(0),
      .adc_b({rx_q,4'b00}),.adc_ovf_b(0),
      .i_out(adc_i), .q_out(adc_q), .run(run_rx0 | run_rx1), .debug());
   
   // /////////////////////////////////////////////////////////////////////////
   // DSP RX 0

   wire [31:0] 	 sample_rx0;
   wire 	 strobe_rx0;
   wire [35:0] 	 vita_rx_data0;
   wire 	 vita_rx_src_rdy0, vita_rx_dst_rdy0;
   
   dsp_core_rx #(.BASE(SR_RX_DSP0)) dsp_core_rx0
     (.clk(wb_clk),.rst(wb_rst),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .adc_i(adc_i),.adc_ovf_i(0),.adc_q(adc_q),.adc_ovf_q(0),
      .sample(sample_rx0), .run(run_rx0), .strobe(strobe_rx0),
      .debug() );

   vita_rx_chain #(.BASE(SR_RX_CTRL0), .UNIT(0), .FIFOSIZE(9), .PROT_ENG_FLAGS(0)) vita_rx_chain0
     (.clk(wb_clk),.reset(wb_rst),.clear(clear_rx),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .vita_time(vita_time), .overrun(rx_overrun_dsp0),
      .sample(sample_rx0), .run(run_rx0), .strobe(strobe_rx0),
      .rx_data_o(vita_rx_data0), .rx_dst_rdy_i(vita_rx_dst_rdy0), .rx_src_rdy_o(vita_rx_src_rdy0),
      .debug() );
   
   // /////////////////////////////////////////////////////////////////////////
   // DSP RX 1

   wire [31:0] 	 sample_rx1;
   wire 	 strobe_rx1;
   wire [35:0] 	 vita_rx_data1;
   wire 	 vita_rx_src_rdy1, vita_rx_dst_rdy1;
   
   dsp_core_rx #(.BASE(SR_RX_DSP1)) dsp_core_rx1
     (.clk(wb_clk),.rst(wb_rst),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .adc_i(adc_i),.adc_ovf_i(0),.adc_q(adc_q),.adc_ovf_q(0),
      .sample(sample_rx1), .run(run_rx1), .strobe(strobe_rx1),
      .debug() );

   vita_rx_chain #(.BASE(SR_RX_CTRL1), .UNIT(1), .FIFOSIZE(9), .PROT_ENG_FLAGS(0)) vita_rx_chain1
     (.clk(wb_clk),.reset(wb_rst),.clear(clear_rx),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .vita_time(vita_time), .overrun(rx_overrun_dsp1),
      .sample(sample_rx1), .run(run_rx1), .strobe(strobe_rx1),
      .rx_data_o(vita_rx_data1), .rx_dst_rdy_i(vita_rx_dst_rdy1), .rx_src_rdy_o(vita_rx_src_rdy1),
      .debug() );

   // /////////////////////////////////////////////////////////////////////////
   // RX Stream muxing

   fifo36_mux #(.prio(0)) mux_data_streams
     (.clk(wb_clk), .reset(wb_rst), .clear(0),
      .data0_i(vita_rx_data0), .src0_rdy_i(vita_rx_src_rdy0), .dst0_rdy_o(vita_rx_dst_rdy0),
      .data1_i(vita_rx_data1), .src1_rdy_i(vita_rx_src_rdy1), .dst1_rdy_o(vita_rx_dst_rdy1),
      .data_o(rx_data), .src_rdy_o(rx_src_rdy), .dst_rdy_i(rx_dst_rdy));

   // ///////////////////////////////////////////////////////////////////////////////////
   // DSP TX

   wire [23:0] 	 tx_i_int, tx_q_int;
   wire 	 run_tx;
   
   vita_tx_chain #(.BASE_CTRL(SR_TX_CTRL), .BASE_DSP(SR_TX_DSP), 
		   .REPORT_ERROR(1), .DO_FLOW_CONTROL(0),
		   .PROT_ENG_FLAGS(0), .USE_TRANS_HEADER(0),
		   .DSP_NUMBER(0)) 
   vita_tx_chain
     (.clk(wb_clk), .reset(wb_rst),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .vita_time(vita_time),
      .tx_data_i(tx_data), .tx_src_rdy_i(tx_src_rdy), .tx_dst_rdy_o(tx_dst_rdy),
      .err_data_o(tx_err_data), .err_src_rdy_o(tx_err_src_rdy), .err_dst_rdy_i(tx_err_dst_rdy),
      .tx_i(tx_i_int),.tx_q(tx_q_int),
      .underrun(tx_underrun_dsp), .run(run_tx),
      .debug(debug_vt));

   tx_frontend #(.BASE(SR_TX_FRONT), .WIDTH_OUT(14)) tx_frontend
     (.clk(wb_clk), .rst(wb_rst),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .tx_i(tx_i_int), .tx_q(tx_q_int), .run(1'b1),
      .dac_a(tx_i), .dac_b(tx_q));

   // /////////////////////////////////////////////////////////////////////////////////////
   // Wishbone Intercon, single master
   wire [dw-1:0] s0_dat_mosi, s1_dat_mosi, s0_dat_miso, s1_dat_miso, s2_dat_mosi, s3_dat_mosi, s2_dat_miso, s3_dat_miso,
		 s4_dat_mosi, s5_dat_mosi, s4_dat_miso, s5_dat_miso, s6_dat_mosi, s7_dat_mosi, s6_dat_miso, s7_dat_miso,
		 s8_dat_mosi, s9_dat_mosi, s8_dat_miso, s9_dat_miso, sa_dat_mosi, sb_dat_mosi, sa_dat_miso, sb_dat_miso,
		 sc_dat_mosi, sd_dat_mosi, sc_dat_miso, sd_dat_miso, se_dat_mosi, sf_dat_mosi, se_dat_miso, sf_dat_miso;
   wire [aw-1:0] s0_adr,s1_adr,s2_adr,s3_adr,s4_adr,s5_adr,s6_adr,s7_adr;
   wire [aw-1:0] s8_adr,s9_adr,sa_adr,sb_adr,sc_adr, sd_adr, se_adr, sf_adr;
   wire [sw-1:0] s0_sel,s1_sel,s2_sel,s3_sel,s4_sel,s5_sel,s6_sel,s7_sel;
   wire [sw-1:0] s8_sel,s9_sel,sa_sel,sb_sel,sc_sel, sd_sel, se_sel, sf_sel;
   wire 	 s0_ack,s1_ack,s2_ack,s3_ack,s4_ack,s5_ack,s6_ack,s7_ack;
   wire 	 s8_ack,s9_ack,sa_ack,sb_ack,sc_ack, sd_ack, se_ack, sf_ack;
   wire 	 s0_stb,s1_stb,s2_stb,s3_stb,s4_stb,s5_stb,s6_stb,s7_stb;
   wire 	 s8_stb,s9_stb,sa_stb,sb_stb,sc_stb, sd_stb, se_stb, sf_stb;
   wire 	 s0_cyc,s1_cyc,s2_cyc,s3_cyc,s4_cyc,s5_cyc,s6_cyc,s7_cyc;
   wire 	 s8_cyc,s9_cyc,sa_cyc,sb_cyc,sc_cyc, sd_cyc, se_cyc, sf_cyc;
   wire 	 s0_we,s1_we,s2_we,s3_we,s4_we,s5_we,s6_we,s7_we;
   wire 	 s8_we,s9_we,sa_we,sb_we,sc_we,sd_we, se_we, sf_we;
   
   wb_1master #(.dw(dw), .aw(aw), .sw(sw), .decode_w(4),
		.s0_addr(4'h0), .s0_mask(4'hF), .s1_addr(4'h1), .s1_mask(4'hF),
		.s2_addr(4'h2), .s2_mask(4'hF),	.s3_addr(4'h3), .s3_mask(4'hF),
		.s4_addr(4'h4), .s4_mask(4'hF),	.s5_addr(4'h5), .s5_mask(4'hF),
		.s6_addr(4'h6), .s6_mask(4'hF),	.s7_addr(4'h7), .s7_mask(4'hF),
		.s8_addr(4'h8), .s8_mask(4'hE),	.s9_addr(4'hf), .s9_mask(4'hF), // slave 8 is double wide
		.sa_addr(4'ha), .sa_mask(4'hF),	.sb_addr(4'hb), .sb_mask(4'hF),
		.sc_addr(4'hc), .sc_mask(4'hF),	.sd_addr(4'hd), .sd_mask(4'hF),
		.se_addr(4'he), .se_mask(4'hF),	.sf_addr(4'hf), .sf_mask(4'hF))
   wb_1master
     (.clk_i(wb_clk),.rst_i(wb_rst),       
      .m0_dat_o(m0_dat_miso),.m0_ack_o(m0_ack),.m0_err_o(m0_err),.m0_rty_o(m0_rty),.m0_dat_i(m0_dat_mosi),
      .m0_adr_i(m0_adr),.m0_sel_i(m0_sel),.m0_we_i(m0_we),.m0_cyc_i(m0_cyc),.m0_stb_i(m0_stb),
      .s0_dat_o(s0_dat_mosi),.s0_adr_o(s0_adr),.s0_sel_o(s0_sel),.s0_we_o(s0_we),.s0_cyc_o(s0_cyc),.s0_stb_o(s0_stb),
      .s0_dat_i(s0_dat_miso),.s0_ack_i(s0_ack),.s0_err_i(0),.s0_rty_i(0),
      .s1_dat_o(s1_dat_mosi),.s1_adr_o(s1_adr),.s1_sel_o(s1_sel),.s1_we_o(s1_we),.s1_cyc_o(s1_cyc),.s1_stb_o(s1_stb),
      .s1_dat_i(s1_dat_miso),.s1_ack_i(s1_ack),.s1_err_i(0),.s1_rty_i(0),
      .s2_dat_o(s2_dat_mosi),.s2_adr_o(s2_adr),.s2_sel_o(s2_sel),.s2_we_o(s2_we),.s2_cyc_o(s2_cyc),.s2_stb_o(s2_stb),
      .s2_dat_i(s2_dat_miso),.s2_ack_i(s2_ack),.s2_err_i(0),.s2_rty_i(0),
      .s3_dat_o(s3_dat_mosi),.s3_adr_o(s3_adr),.s3_sel_o(s3_sel),.s3_we_o(s3_we),.s3_cyc_o(s3_cyc),.s3_stb_o(s3_stb),
      .s3_dat_i(s3_dat_miso),.s3_ack_i(s3_ack),.s3_err_i(0),.s3_rty_i(0),
      .s4_dat_o(s4_dat_mosi),.s4_adr_o(s4_adr),.s4_sel_o(s4_sel),.s4_we_o(s4_we),.s4_cyc_o(s4_cyc),.s4_stb_o(s4_stb),
      .s4_dat_i(s4_dat_miso),.s4_ack_i(s4_ack),.s4_err_i(0),.s4_rty_i(0),
      .s5_dat_o(s5_dat_mosi),.s5_adr_o(s5_adr),.s5_sel_o(s5_sel),.s5_we_o(s5_we),.s5_cyc_o(s5_cyc),.s5_stb_o(s5_stb),
      .s5_dat_i(s5_dat_miso),.s5_ack_i(s5_ack),.s5_err_i(0),.s5_rty_i(0),
      .s6_dat_o(s6_dat_mosi),.s6_adr_o(s6_adr),.s6_sel_o(s6_sel),.s6_we_o(s6_we),.s6_cyc_o(s6_cyc),.s6_stb_o(s6_stb),
      .s6_dat_i(s6_dat_miso),.s6_ack_i(s6_ack),.s6_err_i(0),.s6_rty_i(0),
      .s7_dat_o(s7_dat_mosi),.s7_adr_o(s7_adr),.s7_sel_o(s7_sel),.s7_we_o(s7_we),.s7_cyc_o(s7_cyc),.s7_stb_o(s7_stb),
      .s7_dat_i(s7_dat_miso),.s7_ack_i(s7_ack),.s7_err_i(0),.s7_rty_i(0),
      .s8_dat_o(s8_dat_mosi),.s8_adr_o(s8_adr),.s8_sel_o(s8_sel),.s8_we_o(s8_we),.s8_cyc_o(s8_cyc),.s8_stb_o(s8_stb),
      .s8_dat_i(s8_dat_miso),.s8_ack_i(s8_ack),.s8_err_i(0),.s8_rty_i(0),
      .s9_dat_o(s9_dat_mosi),.s9_adr_o(s9_adr),.s9_sel_o(s9_sel),.s9_we_o(s9_we),.s9_cyc_o(s9_cyc),.s9_stb_o(s9_stb),
      .s9_dat_i(s9_dat_miso),.s9_ack_i(s9_ack),.s9_err_i(0),.s9_rty_i(0),
      .sa_dat_o(sa_dat_mosi),.sa_adr_o(sa_adr),.sa_sel_o(sa_sel),.sa_we_o(sa_we),.sa_cyc_o(sa_cyc),.sa_stb_o(sa_stb),
      .sa_dat_i(sa_dat_miso),.sa_ack_i(sa_ack),.sa_err_i(0),.sa_rty_i(0),
      .sb_dat_o(sb_dat_mosi),.sb_adr_o(sb_adr),.sb_sel_o(sb_sel),.sb_we_o(sb_we),.sb_cyc_o(sb_cyc),.sb_stb_o(sb_stb),
      .sb_dat_i(sb_dat_miso),.sb_ack_i(sb_ack),.sb_err_i(0),.sb_rty_i(0),
      .sc_dat_o(sc_dat_mosi),.sc_adr_o(sc_adr),.sc_sel_o(sc_sel),.sc_we_o(sc_we),.sc_cyc_o(sc_cyc),.sc_stb_o(sc_stb),
      .sc_dat_i(sc_dat_miso),.sc_ack_i(sc_ack),.sc_err_i(0),.sc_rty_i(0),
      .sd_dat_o(sd_dat_mosi),.sd_adr_o(sd_adr),.sd_sel_o(sd_sel),.sd_we_o(sd_we),.sd_cyc_o(sd_cyc),.sd_stb_o(sd_stb),
      .sd_dat_i(sd_dat_miso),.sd_ack_i(sd_ack),.sd_err_i(0),.sd_rty_i(0),
      .se_dat_o(se_dat_mosi),.se_adr_o(se_adr),.se_sel_o(se_sel),.se_we_o(se_we),.se_cyc_o(se_cyc),.se_stb_o(se_stb),
      .se_dat_i(se_dat_miso),.se_ack_i(se_ack),.se_err_i(0),.se_rty_i(0),
      .sf_dat_o(sf_dat_mosi),.sf_adr_o(sf_adr),.sf_sel_o(sf_sel),.sf_we_o(sf_we),.sf_cyc_o(sf_cyc),.sf_stb_o(sf_stb),
      .sf_dat_i(sf_dat_miso),.sf_ack_i(sf_ack),.sf_err_i(0),.sf_rty_i(0) );

   assign s5_ack = 0;   
   assign s9_ack = 0;   assign sa_ack = 0;   assign sb_ack = 0;
   assign sc_ack = 0;   assign sd_ack = 0;   assign se_ack = 0;   assign sf_ack = 0;

   // /////////////////////////////////////////////////////////////////////////////////////
   // Slave 0, Misc LEDs, Switches, controls
   
   localparam REG_LEDS = 7'd0;         // out
   localparam REG_CGEN_CTRL = 7'd4;    // out
   localparam REG_CGEN_ST = 7'd6;      // in
   localparam REG_TEST = 7'd8;         // out
   localparam REG_RX_FRAMELEN = 7'd10; // in
   localparam REG_TX_FRAMELEN = 7'd12; // out
   localparam REG_XFER_RATE = 7'd14;   // out
   localparam REG_COMPAT = 7'd16;      // in
   
   always @(posedge wb_clk)
     if(wb_rst)
       begin
	  reg_leds <= 0;
	  reg_cgen_ctrl <= 2'b11;
	  reg_test <= 0;
	  //xfer_rate <= 0;
	  frames_per_packet <= 0;
       end
     else
       if(s0_cyc & s0_stb & s0_we) 
	 case(s0_adr[6:0])
	   REG_LEDS :
	     reg_leds <= s0_dat_mosi;
	   REG_CGEN_CTRL :
	     reg_cgen_ctrl <= s0_dat_mosi;
	   REG_TEST :
	     reg_test <= s0_dat_mosi;
	   REG_RX_FRAMELEN :
	     frames_per_packet <= s0_dat_mosi[7:0];
	   //REG_XFER_RATE :
	     //xfer_rate <= s0_dat_mosi;
	 endcase // case (s0_adr[6:0])

   assign test_ctrl = xfer_rate[11:8];
   assign test_rate = xfer_rate[7:0];
   assign test_len = reg_test[15:0];
   
   assign debug_led = {run_tx, (run_rx0 | run_rx1), cgen_st_ld};
   assign { cgen_sync_b, cgen_ref_sel } = reg_cgen_ctrl;
   
   assign s0_dat_miso = (s0_adr[6:0] == REG_LEDS) ? reg_leds : 
			(s0_adr[6:0] == REG_CGEN_CTRL) ? reg_cgen_ctrl :
			(s0_adr[6:0] == REG_CGEN_ST) ? {13'b0,cgen_st_status,cgen_st_ld,cgen_st_refmon} :
			(s0_adr[6:0] == REG_TEST) ? reg_test :
			(s0_adr[6:0] == REG_COMPAT) ? { 8'd0, COMPAT_NUM } :
			16'hBEEF;
   
   assign s0_ack = s0_stb & s0_cyc;

   // /////////////////////////////////////////////////////////////////////////////////////
   // Slave 1, UART
   //    depth of 3 is 128 entries, clkdiv of 278 gives 230.4k with a 64 MHz system clock

/*   
   simple_uart #(.TXDEPTH(3),.RXDEPTH(3), .CLKDIV_DEFAULT(278)) uart 
     (.clk_i(wb_clk),.rst_i(wb_rst),
      .we_i(s1_we),.stb_i(s1_stb),.cyc_i(s1_cyc),.ack_o(s1_ack),
      .adr_i(s1_adr[3:1]),.dat_i({16'd0,s1_dat_mosi}),.dat_o(s1_dat_miso),
      .rx_int_o(),.tx_int_o(),
      .tx_o(debug_txd),.rx_i(debug_rxd),.baud_o());
*/
   
   // /////////////////////////////////////////////////////////////////////////////////////
   // Slave 2, SPI

   spi_top16 shared_spi
     (.wb_clk_i(wb_clk),.wb_rst_i(wb_rst),.wb_adr_i(s2_adr[4:0]),.wb_dat_i(s2_dat_mosi),
      .wb_dat_o(s2_dat_miso),.wb_sel_i(s2_sel),.wb_we_i(s2_we),.wb_stb_i(s2_stb),
      .wb_cyc_i(s2_cyc),.wb_ack_o(s2_ack),.wb_err_o(),.wb_int_o(),
      .ss_pad_o(sen), .sclk_pad_o(sclk), .mosi_pad_o(mosi), .miso_pad_i(miso) );
   
   // /////////////////////////////////////////////////////////////////////////
   // Slave 3, I2C

   wire 	scl_pad_i, scl_pad_o, scl_pad_oen_o, sda_pad_i, sda_pad_o, sda_pad_oen_o;
   i2c_master_top #(.ARST_LVL(1)) i2c 
     (.wb_clk_i(wb_clk),.wb_rst_i(wb_rst),.arst_i(1'b0), 
      .wb_adr_i(s3_adr[3:1]),.wb_dat_i(s3_dat_mosi[7:0]),.wb_dat_o(s3_dat_miso[7:0]),
      .wb_we_i(s3_we),.wb_stb_i(s3_stb),.wb_cyc_i(s3_cyc),
      .wb_ack_o(s3_ack),.wb_inta_o(),
      .scl_pad_i(scl_pad_i),.scl_pad_o(scl_pad_o),.scl_padoen_o(scl_pad_oen_o),
      .sda_pad_i(sda_pad_i),.sda_pad_o(sda_pad_o),.sda_padoen_o(sda_pad_oen_o) );

   assign 	 s3_dat_miso[15:8] = 8'd0;

   // I2C -- Don't use external transistors for open drain, the FPGA implements this
   IOBUF scl_pin(.O(scl_pad_i), .IO(db_scl), .I(scl_pad_o), .T(scl_pad_oen_o));
   IOBUF sda_pin(.O(sda_pad_i), .IO(db_sda), .I(sda_pad_o), .T(sda_pad_oen_o));

   // /////////////////////////////////////////////////////////////////////////
   // GPIOs -- Slave #4

   wire [31:0] 	atr_lines;
   wire [31:0] 	debug_gpio_0, debug_gpio_1;
   
   nsgpio16LE 
     nsgpio16LE(.clk_i(wb_clk),.rst_i(wb_rst),
		.cyc_i(s4_cyc),.stb_i(s4_stb),.adr_i(s4_adr[3:0]),.we_i(s4_we),
		.dat_i(s4_dat_mosi),.dat_o(s4_dat_miso),.ack_o(s4_ack),
		.atr(atr_lines),.debug_0(debug_gpio_0),.debug_1(debug_gpio_1),
		.gpio( {io_tx,io_rx} ) );

   // /////////////////////////////////////////////////////////////////////////
   // Settings Bus -- Slave #8 + 9

   // only have 64 regs, 32 bits each with current setup...
   settings_bus_16LE #(.AWIDTH(11),.RWIDTH(6)) settings_bus_16LE
     (.wb_clk(wb_clk),.wb_rst(wb_rst),.wb_adr_i(s8_adr),.wb_dat_i(s8_dat_mosi),
      .wb_stb_i(s8_stb),.wb_we_i(s8_we),.wb_ack_o(s8_ack),
      .strobe(set_stb),.addr(set_addr),.data(set_data) );

   // /////////////////////////////////////////////////////////////////////////
   // ATR Controller -- Slave #6

   atr_controller16 atr_controller16
     (.clk_i(wb_clk), .rst_i(wb_rst),
      .adr_i(s6_adr[5:0]), .sel_i(s6_sel), .dat_i(s6_dat_mosi), .dat_o(s6_dat_miso),
      .we_i(s6_we), .stb_i(s6_stb), .cyc_i(s6_cyc), .ack_o(s6_ack),
      .run_rx(run_rx0 | run_rx1), .run_tx(run_tx), .ctrl_lines(atr_lines));

   // /////////////////////////////////////////////////////////////////////////
   // Readback mux 32 -- Slave #7

   wire [31:0] reg_test32;

   setting_reg #(.my_addr(SR_REG_TEST32)) sr_reg_test32
     (.clk(wb_clk),.rst(wb_rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(reg_test32),.changed());

   wb_readback_mux_16LE readback_mux_32
     (.wb_clk_i(wb_clk), .wb_rst_i(wb_rst), .wb_stb_i(s7_stb),
      .wb_adr_i({5'b0,s7_adr}), .wb_dat_o(s7_dat_miso), .wb_ack_o(s7_ack),

      .word00(vita_time[63:32]),        .word01(vita_time[31:0]),
      .word02(vita_time_pps[63:32]),    .word03(vita_time_pps[31:0]),
      .word04(reg_test32),              .word05(32'b0),
      .word06(32'b0),                   .word07(32'b0),
      .word08(32'b0),                   .word09(32'b0),
      .word10(32'b0),                   .word11(32'b0),
      .word12(32'b0),                   .word13(32'b0),
      .word14(32'b0),                   .word15(32'b0)
      );

   // /////////////////////////////////////////////////////////////////////////
   // VITA Timing

   time_64bit #(.TICKS_PER_SEC(32'd64000000),.BASE(SR_TIME64)) time_64bit
     (.clk(wb_clk), .rst(wb_rst), .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
      .pps(pps_in), .vita_time(vita_time), .vita_time_pps(vita_time_pps), .pps_int(pps_int),
      .exp_time_in(0));
   
   // /////////////////////////////////////////////////////////////////////////////////////
   // Debug circuitry

   assign debug_clk = 2'b00; // { gpif_clk, clk_fpga };
   assign debug = 0;
   assign debug_gpio_0 = 0;
   assign debug_gpio_1 = 0;
   
endmodule // u1plus_core
