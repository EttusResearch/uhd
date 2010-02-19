`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////

module u1e_core
  (input clk_fpga, output [2:0] debug_led, output [31:0] debug, output [1:0] debug_clk,
   input [2:0] debug_pb, input [7:0] dip_sw, output debug_txd, input debug_rxd,
   
   // GPMC
   input EM_CLK, inout [15:0] EM_D, input [10:1] EM_A, input [1:0] EM_NBE,
   input EM_WAIT0, input EM_NCS4, input EM_NCS6, input EM_NWE, input EM_NOE,
   
   inout db_sda, inout db_scl,
   output tx_have_space, output tx_underrun, output rx_have_data, output rx_overrun
   );

   wire 	wb_clk, wb_rst;

   assign tx_have_space = 0;
   assign tx_underrun = 0;
   assign rx_have_data = 0;
   assign rx_overrun = 0;
   
   // /////////////////////////////////////////////////////////////////////////////////////
   // GPMC Slave to Wishbone Master
   localparam dw = 16;
   localparam aw = 11;
   localparam sw = 2;
      
   wire [dw-1:0] m0_dat_mosi, m0_dat_miso;
   wire [aw-1:0] m0_adr;
   wire [sw-1:0] m0_sel;
   wire 	 m0_cyc, m0_stb, m0_we, m0_ack, m0_err, m0_rty;
   
   gpmc gpmc (.EM_CLK(EM_CLK), .EM_D(EM_D), .EM_A(EM_A), .EM_NBE(EM_NBE),
	      .EM_WAIT0(EM_WAIT0), .EM_NCS4(EM_NCS4), .EM_NCS6(EM_NCS6), .EM_NWE(EM_NWE), 
	      .EM_NOE(EM_NOE),

	      .wb_clk(wb_clk), .wb_rst(wb_rst),
	      .wb_adr_o(m0_adr), .wb_dat_mosi(m0_dat_mosi), .wb_dat_miso(m0_dat_miso),
	      .wb_sel_o(m0_sel), .wb_cyc_o(m0_cyc), .wb_stb_o(m0_stb), .wb_we_o(m0_we),
	      .wb_ack_i(m0_ack));

   assign wb_clk = clk_fpga;

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
		.s8_addr(4'h8), .s8_mask(4'hF),	.s9_addr(4'h9), .s9_mask(4'hF),
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

   assign s4_ack = 0;   assign s5_ack = 0;   assign s6_ack = 0;   assign s7_ack = 0;
   assign s8_ack = 0;   assign s9_ack = 0;   assign sa_ack = 0;   assign sb_ack = 0;
   assign sc_ack = 0;   assign sd_ack = 0;   assign se_ack = 0;   assign sf_ack = 0;

   // /////////////////////////////////////////////////////////////////////////////////////
   // Slave 0, LEDs and Switches
   
   reg [15:0] 	reg_fast, reg_slow;
   localparam REG_FAST = 7'd4;
   localparam REG_SWITCHES = 7'd5;
   
   always @(posedge wb_clk)
     if(s0_cyc & s0_stb & s0_we & (s0_adr[6:0] == REG_FAST))
       reg_fast <= s0_dat_mosi;

   assign s0_dat_miso = (s0_adr[6:0] == REG_FAST) ? reg_fast : 
			(s0_adr[6:0] == REG_SWITCHES) ? {5'b0,debug_pb[2:0],dip_sw[7:0]} :
			16'hBEEF;
   assign s0_ack = s0_stb & s0_cyc;


   // /////////////////////////////////////////////////////////////////////////////////////
   // Slave 1, UART
   //    depth of 3 is 128 entries, clkdiv of 278 gives 230.4k with a 64 MHz system clock
   
   simple_uart #(.TXDEPTH(3),.RXDEPTH(3), .CLKDIV_DEFAULT(278)) uart 
     (.clk_i(wb_clk),.rst_i(wb_rst),
      .we_i(s1_we),.stb_i(s1_stb),.cyc_i(s1_cyc),.ack_o(s1_ack),
      .adr_i(s1_adr[4:2]),.dat_i({16'd0,s1_dat_mosi}),.dat_o(s1_dat_miso),
      .rx_int_o(),.tx_int_o(),
      .tx_o(debug_txd),.rx_i(debug_rxd),.baud_o());

   // /////////////////////////////////////////////////////////////////////////////////////
   // Slave 2, SPI

   /*
   spi_top shared_spi
     (.wb_clk_i(wb_clk),.wb_rst_i(wb_rst),.wb_adr_i(s2_adr[4:0]),.wb_dat_i(s2_dat_o),
      .wb_dat_o(s2_dat_i),.wb_sel_i(s2_sel),.wb_we_i(s2_we),.wb_stb_i(s2_stb),
      .wb_cyc_i(s2_cyc),.wb_ack_o(s2_ack),.wb_err_o(),.wb_int_o(spi_int),
      .ss_pad_o({sen_tx_db,sen_tx_adc,sen_tx_dac,sen_rx_db,sen_rx_adc,sen_rx_dac,sen_dac,sen_clk}),
      .sclk_pad_o(sclk),.mosi_pad_o(mosi),.miso_pad_i(miso) );
    */
   
   // /////////////////////////////////////////////////////////////////////////
   // Slave 3, I2C

   wire 	scl_pad_i, scl_pad_o, scl_pad_oen_o, sda_pad_i, sda_pad_o, sda_pad_oen_o;
   i2c_master_top #(.ARST_LVL(1)) i2c 
     (.wb_clk_i(wb_clk),.wb_rst_i(wb_rst),.arst_i(1'b0), 
      .wb_adr_i(s3_adr[4:2]),.wb_dat_i(s3_dat_mosi[7:0]),.wb_dat_o(s3_dat_miso[7:0]),
      .wb_we_i(s3_we),.wb_stb_i(s3_stb),.wb_cyc_i(s3_cyc),
      .wb_ack_o(s3_ack),.wb_inta_o(),
      .scl_pad_i(scl_pad_i),.scl_pad_o(scl_pad_o),.scl_padoen_o(scl_pad_oen_o),
      .sda_pad_i(sda_pad_i),.sda_pad_o(sda_pad_o),.sda_padoen_o(sda_pad_oen_o) );

   assign 	 s3_dat_miso[15:8] = 8'd0;

   // I2C -- Don't use external transistors for open drain, the FPGA implements this
   IOBUF scl_pin(.O(scl_pad_i), .IO(db_scl), .I(scl_pad_o), .T(scl_pad_oen_o));
   IOBUF sda_pin(.O(sda_pad_i), .IO(db_sda), .I(sda_pad_o), .T(sda_pad_oen_o));

   // /////////////////////////////////////////////////////////////////////////////////////
   // Debug Pins
   
   // Debug circuitry
   assign debug_clk = { EM_CLK, clk_fpga };
   assign debug = { { 1'b0, EM_WAIT0, EM_NCS6, EM_NCS4, EM_NWE, EM_NOE, EM_A[10:1] },
		    { EM_D } };

   assign { debug_led[2],debug_led[0],debug_led[1] } = reg_fast;  // LEDs are arranged funny on board
   
endmodule // u1e_core
