//
// Copyright 2011-2012 Ettus Research LLC
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

module E1x0
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

    assign FPGA_TXD = 0; //dont care

   // /////////////////////////////////////////////////////////////////////////
   // Clocking
   wire clk_fpga;
   wire reset;

   reg                 por_rst;
   reg [7:0]   por_counter = 8'h0;

   always @(posedge clk_fpga)
     if (por_counter != 8'h55)
       begin
          por_counter <= por_counter + 8'h1;
          por_rst <= 1'b1;
       end
     else por_rst <= 1'b0;

   wire async_reset;
   cross_clock_reader #(.WIDTH(1)) read_gpio_reset
       (.clk(clk_fpga), .rst(por_rst), .in(cgen_sen_b & ~cgen_sclk), .out(async_reset));

   IBUFGDS #(.IOSTANDARD("LVDS_33"), .DIFF_TERM("TRUE")) 
   clk_fpga_pin (.O(clk_fpga),.I(CLK_FPGA_P),.IB(CLK_FPGA_N));

   reset_sync reset_sync(.clk(clk_fpga), .reset_in(async_reset), .reset_out(reset));

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

   //assign the aux spi to the cgen (bypasses control fifo)
   assign cgen_sclk = overo_gpio65;
   assign cgen_sen_b = overo_gpio128;
   assign cgen_mosi = overo_gpio145;
   wire has_resp; //re-purpose gpio for interrupt when we are not using aux spi
   assign overo_gpio147 = (cgen_sen_b == 1'b0)? cgen_miso : has_resp;

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
   // RX ADC -- handles inversion

    reg [11:0] rx_i, rx_q;
    always @(posedge clk_fpga) begin
        rx_i <= ~DA;
        rx_q <= ~DB;
    end

   // /////////////////////////////////////////////////////////////////////////
   // Main Core
   wire [35:0] rx_data, tx_data, ctrl_data, resp_data;
   wire rx_src_rdy, rx_dst_rdy, tx_src_rdy, tx_dst_rdy, resp_src_rdy, resp_dst_rdy, ctrl_src_rdy, ctrl_dst_rdy;
   wire dsp_rx_run, dsp_tx_run;
   wire [7:0] sen8;
   assign {_cgen_sen_b,sen_codec,db_sen_tx,db_sen_rx} = sen8[3:0];
   wire [31:0] core_debug;

   assign debug_led = ~{PPS_IN, dsp_tx_run, dsp_rx_run, cgen_st_ld};
   wire cgen_sync;
   assign { cgen_sync_b, cgen_ref_sel } = {~cgen_sync, 1'b1};

   u1plus_core #(
        .NUM_RX_DSPS(2),
        .DSP_RX_XTRA_FIFOSIZE(10),
        .DSP_TX_XTRA_FIFOSIZE(10),
        .USE_PACKET_PADDER(0)
    ) core(
         .clk(clk_fpga), .reset(reset),
         .debug(core_debug), .debug_clk(debug_clk),

         .rx_data(rx_data), .rx_src_rdy(rx_src_rdy), .rx_dst_rdy(rx_dst_rdy),
         .tx_data(tx_data), .tx_src_rdy(tx_src_rdy), .tx_dst_rdy(tx_dst_rdy),
         .ctrl_data(ctrl_data), .ctrl_src_rdy(ctrl_src_rdy), .ctrl_dst_rdy(ctrl_dst_rdy),
         .resp_data(resp_data), .resp_src_rdy(resp_src_rdy), .resp_dst_rdy(resp_dst_rdy),

         .dsp_rx_run(dsp_rx_run), .dsp_tx_run(dsp_tx_run),
         .clock_sync(cgen_sync),

         .db_sda(db_sda), .db_scl(db_scl),
         .sclk(sclk), .sen(sen8), .mosi(mosi), .miso(miso),
         .io_tx(io_tx), .io_rx(io_rx),
         .tx_i(tx_i), .tx_q(tx_q),
         .rx_i(rx_i), .rx_q(rx_q),
         .pps_in(PPS_IN) );

   // /////////////////////////////////////////////////////////////////////////
   // Interface between GPMC/host
    wire [31:0] gpmc_debug;

   gpmc #(.TXFIFOSIZE(13), .RXFIFOSIZE(13))
   gpmc (.arst(async_reset),
         .EM_CLK(EM_CLK), .EM_D(EM_D), .EM_A(EM_A), .EM_NBE(EM_NBE),
         .EM_WAIT0(EM_WAIT0), .EM_NCS4(EM_NCS4), .EM_NCS6(EM_NCS6), .EM_NWE(EM_NWE),
         .EM_NOE(EM_NOE),

         .rx_have_data(overo_gpio146), .tx_have_space(overo_gpio144),
         .resp_have_data(has_resp),

         .fifo_clk(clk_fpga), .fifo_rst(reset),
         .rx_data(rx_data), .rx_src_rdy(rx_src_rdy), .rx_dst_rdy(rx_dst_rdy),
         .tx_data(tx_data), .tx_src_rdy(tx_src_rdy), .tx_dst_rdy(tx_dst_rdy),
         .ctrl_data(ctrl_data), .ctrl_src_rdy(ctrl_src_rdy), .ctrl_dst_rdy(ctrl_dst_rdy),
         .resp_data(resp_data), .resp_src_rdy(resp_src_rdy), .resp_dst_rdy(resp_dst_rdy),

         .debug(gpmc_debug));

    //assign debug = gpmc_debug;
    assign debug = core_debug;

endmodule // E1x0
