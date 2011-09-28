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

`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////

module u1e
  (input CLK_FPGA_P, input CLK_FPGA_N,  // Diff
   output [3:0] debug_led, output [31:0] debug, output [1:0] debug_clk,
   input debug_pb, output FPGA_TXD, input FPGA_RXD,
   output fpga_txd1, input fpga_rxd1, input overo_txd1, output overo_rxd1,

   // GPMC
   input EM_CLK, inout [15:0] EM_D, input [10:1] EM_A, input [1:0] EM_NBE,
   input EM_WAIT0, input EM_NCS4, input EM_NCS5, input EM_NCS6,
   input EM_NWE, input EM_NOE,

   inout db_sda, inout db_scl, // I2C

   output db_sclk_tx, output db_sen_tx, output db_mosi_tx, input db_miso_tx,   // DB TX SPI
   output db_sclk_rx, output db_sen_rx, output db_mosi_rx, input db_miso_rx,   // DB TX SPI
   output sclk_codec, output sen_codec, output mosi_codec, input miso_codec,   // AD9862 main SPI
   output cgen_sclk, output cgen_sen_b, output cgen_mosi, input cgen_miso,     // Clock gen SPI

   input cgen_st_status, input cgen_st_ld, input cgen_st_refmon, output cgen_sync_b, output cgen_ref_sel,
   input overo_gpio65, input overo_gpio128, input overo_gpio145, output overo_gpio147, //aux SPI
   
   output overo_gpio144, output overo_gpio146,  // Fifo controls
   input overo_gpio0, input overo_gpio14, input overo_gpio21, input overo_gpio22,  // Misc GPIO
   input overo_gpio23, input overo_gpio64, input overo_gpio127, // Misc GPIO
   input overo_gpio176, input overo_gpio163, input overo_gpio170, // Misc GPIO
   
   inout [15:0] io_tx, inout [15:0] io_rx,

   output [13:0] TX, output TXSYNC, output TXBLANK,
   input [11:0] DA, input [11:0] DB, input RXSYNC,
  
   input PPS_IN
   );

   // /////////////////////////////////////////////////////////////////////////
   // Clocking
   wire  clk_fpga;

   IBUFGDS #(.IOSTANDARD("LVDS_33"), .DIFF_TERM("TRUE")) 
   clk_fpga_pin (.O(clk_fpga),.I(CLK_FPGA_P),.IB(CLK_FPGA_N));

   // /////////////////////////////////////////////////////////////////////////
   // UART level conversion
   assign fpga_txd1 = overo_txd1;
   assign overo_rxd1 = fpga_rxd1;
   
   // SPI
   wire  mosi, sclk, miso;
   assign { db_sclk_tx, db_mosi_tx } = ~db_sen_tx ? {sclk,mosi} : 2'b0;
   assign { db_sclk_rx, db_mosi_rx } = ~db_sen_rx ? {sclk,mosi} : 2'b0;
   assign { sclk_codec, mosi_codec } = ~sen_codec ? {sclk,mosi} : 2'b0;
   //assign { cgen_sclk, cgen_mosi } = ~cgen_sen_b ? {sclk,mosi} : 2'b0; //replaced by aux spi
   assign miso = (~db_sen_tx & db_miso_tx) | (~db_sen_rx & db_miso_rx) |
                 (~sen_codec & miso_codec) | (~cgen_sen_b & cgen_miso);

   //assign the aux spi to the cgen (bypasses wishbone)
   assign cgen_sclk = overo_gpio65;
   assign cgen_sen_b = overo_gpio128;
   assign cgen_mosi = overo_gpio145;
   wire proc_int; //re-purpose gpio for interrupt when we are not using aux spi
   assign overo_gpio147 = (cgen_sen_b == 1'b0)? cgen_miso : proc_int;

   wire _cgen_sen_b;
   //assign cgen_sen_b = _cgen_sen_b; //replaced by aux spi

   // /////////////////////////////////////////////////////////////////////////
   // TX DAC -- handle the interleaved data bus to DAC, with clock doubling DLL

   assign TXBLANK = 0;
   wire [13:0] tx_i, tx_q;

   reg[13:0] delay_q;
   always @(posedge clk_fpga)
     delay_q <= tx_q;
   
   genvar i;
   generate
      for(i=0;i<14;i=i+1)
	begin : gen_dacout
	   ODDR2 #(.DDR_ALIGNMENT("NONE"), // Sets output alignment to "NONE", "C0" or "C1" 
		   .INIT(1'b0),            // Sets initial state of the Q output to 1'b0 or 1'b1
		   .SRTYPE("SYNC"))        // Specifies "SYNC" or "ASYNC" set/reset
	   ODDR2_inst (.Q(TX[i]),      // 1-bit DDR output data
		       .C0(clk_fpga),  // 1-bit clock input
		       .C1(~clk_fpga), // 1-bit clock input
		       .CE(1'b1),      // 1-bit clock enable input
		       .D0(tx_i[i]),   // 1-bit data input (associated with C0)
		       .D1(delay_q[i]),   // 1-bit data input (associated with C1)
		       .R(1'b0),       // 1-bit reset input
		       .S(1'b0));      // 1-bit set input
	end // block: gen_dacout
      endgenerate
   ODDR2 #(.DDR_ALIGNMENT("NONE"), // Sets output alignment to "NONE", "C0" or "C1" 
	   .INIT(1'b0),            // Sets initial state of the Q output to 1'b0 or 1'b1
	   .SRTYPE("SYNC"))        // Specifies "SYNC" or "ASYNC" set/reset
   ODDR2_txsnc (.Q(TXSYNC),      // 1-bit DDR output data
		.C0(clk_fpga),  // 1-bit clock input
		.C1(~clk_fpga), // 1-bit clock input
		.CE(1'b1),      // 1-bit clock enable input
		.D0(1'b0),   // 1-bit data input (associated with C0)
		.D1(1'b1),   // 1-bit data input (associated with C1)
		.R(1'b0),       // 1-bit reset input
		.S(1'b0));      // 1-bit set input
   
   // /////////////////////////////////////////////////////////////////////////
   // Main U1E Core
   u1e_core u1e_core(.clk_fpga(clk_fpga), .rst_fpga(~debug_pb),
		     .debug_led(debug_led), .debug(debug), .debug_clk(debug_clk),
		     .debug_txd(FPGA_TXD), .debug_rxd(FPGA_RXD),
		     .EM_CLK(EM_CLK), .EM_D(EM_D), .EM_A(EM_A), .EM_NBE(EM_NBE),
		     .EM_WAIT0(EM_WAIT0), .EM_NCS4(EM_NCS4), .EM_NCS5(EM_NCS5), 
		     .EM_NCS6(EM_NCS6), .EM_NWE(EM_NWE), .EM_NOE(EM_NOE),
		     .db_sda(db_sda), .db_scl(db_scl),
		     .sclk(sclk), .sen({_cgen_sen_b,sen_codec,db_sen_tx,db_sen_rx}), .mosi(mosi), .miso(miso),
		     .cgen_st_status(cgen_st_status), .cgen_st_ld(cgen_st_ld),.cgen_st_refmon(cgen_st_refmon), 
		     .cgen_sync_b(cgen_sync_b), .cgen_ref_sel(cgen_ref_sel),
		     .tx_have_space(overo_gpio144),
		     .rx_have_data(overo_gpio146),
		     .io_tx(io_tx), .io_rx(io_rx),
		     .tx_i(tx_i), .tx_q(tx_q), 
		     .rx_i(DA), .rx_q(DB),
		     .pps_in(PPS_IN), .proc_int(proc_int) );

   // /////////////////////////////////////////////////////////////////////////
   // Local Debug
   // assign debug_clk = {clk_fpga, clk_2x };
   // assign debug = { TXSYNC, TXBLANK, TX };
   
endmodule // u1e
