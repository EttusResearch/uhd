`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////

module u1e
  (input CLK_FPGA_P, input CLK_FPGA_N,  // Diff
   output [2:0] debug_led, output [31:0] debug, output [1:0] debug_clk,
   input [2:0] debug_pb, input [7:0] dip_sw, output FPGA_TXD, input FPGA_RXD,

   // GPMC
   input EM_CLK, inout [15:0] EM_D, input [10:1] EM_A, input [1:0] EM_NBE,
   input EM_WAIT0, input EM_NCS4, input EM_NCS6, input EM_NWE, input EM_NOE,

   inout db_sda, inout db_scl, // I2C

   output db_sclk_tx, output db_sen_tx, output db_mosi_tx, input db_miso_tx,   // DB TX SPI
   output db_sclk_rx, output db_sen_rx, output db_mosi_rx, input db_miso_rx,   // DB TX SPI
   output sclk_codec, output sen_codec, output mosi_codec, input miso_codec,   // AD9862 main SPI
   output cgen_sclk, output cgen_sen_b, output cgen_mosi, input cgen_miso,     // Clock gen SPI
   
   output overo_gpio144, output overo_gpio145, output overo_gpio146, output overo_gpio147,  // Fifo controls
   inout [15:0] io_tx, inout [15:0] io_rx
   );

   // FPGA-specific pins connections
   wire  clk_fpga;
   
   IBUFGDS #(.IOSTANDARD("LVDS_33"), .DIFF_TERM("TRUE")) 
   clk_fpga_pin (.O(clk_fpga),.I(CLK_FPGA_P),.IB(CLK_FPGA_N));

   // SPI pins
   wire  mosi, sclk, miso;
   assign { db_sclk_tx, db_mosi_tx } = ~db_sen_tx ? {sclk,mosi} : 2'b0;
   assign { db_sclk_rx, db_mosi_rx } = ~db_sen_rx ? {sclk,mosi} : 2'b0;
   assign { sclk_codec, mosi_codec } = ~sen_codec ? {sclk,mosi} : 2'b0;
   assign { cgen_sclk, cgen_mosi } = ~cgen_sen_b ? {sclk,mosi} : 2'b0;
   assign miso = (~db_sen_tx & db_miso_tx) | (~db_sen_rx & db_miso_rx) |
		 (~sen_codec & miso_codec) | (~cgen_sen_b & cgen_miso);
   
   u1e_core u1e_core(.clk_fpga(clk_fpga), .rst_fpga(~debug_pb[2]),
		     .debug_led(debug_led), .debug(debug), .debug_clk(debug_clk),
		     .debug_pb(~debug_pb), .dip_sw(dip_sw), .debug_txd(FPGA_TXD), .debug_rxd(FPGA_RXD),
		     .EM_CLK(EM_CLK), .EM_D(EM_D), .EM_A(EM_A), .EM_NBE(EM_NBE),
		     .EM_WAIT0(EM_WAIT0), .EM_NCS4(EM_NCS4), .EM_NCS6(EM_NCS6), 
		     .EM_NWE(EM_NWE), .EM_NOE(EM_NOE),
		     .db_sda(db_sda), .db_scl(db_scl),
		     .sclk(sclk), .sen({cgen_sen_b,sen_codec,db_sen_tx,db_sen_rx}), .mosi(mosi), .miso(miso),
		     .tx_have_space(overo_gpio144), .tx_underrun(overo_gpio145),
		     .rx_have_data(overo_gpio146), .rx_overrun(overo_gpio147),
		     .io_tx(io_tx), .io_rx(io_rx) );
   
endmodule // u1e
