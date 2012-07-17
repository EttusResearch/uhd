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

//////////////////////////////////////////////////////////////////////////////////

module gpmc
  #(parameter TXFIFOSIZE = 11, 
    parameter RXFIFOSIZE = 11,
    parameter ADDR_WIDTH = 10,
    parameter BUSDEBUG = 1)
   (// GPMC signals
    input arst,
    input EM_CLK, inout [15:0] EM_D, input [ADDR_WIDTH:1] EM_A, input [1:0] EM_NBE,
    input EM_WAIT0, input EM_NCS4, input EM_NCS6, input EM_NWE, input EM_NOE,
    
    // GPIOs for FIFO signalling
    output rx_have_data, output tx_have_space,
    
    // Wishbone signals
    input wb_clk, input wb_rst,
    output [ADDR_WIDTH:0] wb_adr_o, output [15:0] wb_dat_mosi, input [15:0] wb_dat_miso,
    output [1:0] wb_sel_o, output wb_cyc_o, output wb_stb_o, output wb_we_o, input wb_ack_i,
    
    // FIFO interface
    input fifo_clk, input fifo_rst, input clear_tx, input clear_rx,
    output [35:0] tx_data_o, output tx_src_rdy_o, input tx_dst_rdy_i,
    input [35:0] rx_data_i, input rx_src_rdy_i, output rx_dst_rdy_o,

    output tx_underrun, output rx_overrun,
    input [7:0] test_rate, input [3:0] test_ctrl,
    output [31:0] debug
    );

   wire 	  EM_output_enable = (~EM_NOE & (~EM_NCS4 | ~EM_NCS6));
   wire [15:0] 	  EM_D_fifo;
   wire [15:0] 	  EM_D_wb;

   assign EM_D = ~EM_output_enable ? 16'bz : ~EM_NCS4 ? EM_D_fifo : EM_D_wb;

   // CS4 is RAM_2PORT for DATA PATH (high-speed data)
   //    Writes go into one RAM, reads come from the other
   // CS6 is for CONTROL PATH (wishbone)

   // ////////////////////////////////////////////
   // TX Data Path

   wire [17:0] 	  tx18_data;
   wire 	  tx18_src_rdy, tx18_dst_rdy;
   wire [35:0] 	  tx_data, txb_data;
   wire 	  tx_src_rdy, tx_dst_rdy;
   wire 	  txb_src_rdy, txb_dst_rdy;

   gpmc_to_fifo #(.ADDR_WIDTH(ADDR_WIDTH)) gpmc_to_fifo
     (.EM_D(EM_D), .EM_A(EM_A), .EM_CLK(EM_CLK), .EM_WE(~EM_NCS4 & ~EM_NWE),
      .clk(fifo_clk), .reset(fifo_rst), .clear(clear_tx), .arst(fifo_rst | clear_tx | arst),
      .data_o(tx18_data), .src_rdy_o(tx18_src_rdy), .dst_rdy_i(tx18_dst_rdy),
      .have_space(tx_have_space));

   fifo19_to_fifo36 #(.LE(1)) f19_to_f36   // Little endian because ARM is LE
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_tx),
      .f19_datain({1'b0,tx18_data}), .f19_src_rdy_i(tx18_src_rdy), .f19_dst_rdy_o(tx18_dst_rdy),
      .f36_dataout(txb_data), .f36_src_rdy_o(txb_src_rdy), .f36_dst_rdy_i(txb_dst_rdy));

   fifo_cascade #(.WIDTH(36), .SIZE(TXFIFOSIZE)) tx_buffering(
        .clk(fifo_clk), .reset(fifo_rst), .clear(clear_tx),
        .datain(txb_data), .src_rdy_i(txb_src_rdy), .dst_rdy_o(txb_dst_rdy),
        .dataout(tx_data), .src_rdy_o(tx_src_rdy), .dst_rdy_i(tx_dst_rdy)
   );

   // ////////////////////////////////////////////
   // RX Data Path
   
   wire [17:0] 	  rx18_data;
   wire 	  rx18_src_rdy, rx18_dst_rdy;
   wire [35:0] 	  rx_data, rxb_data;
   wire 	  rx_src_rdy, rx_dst_rdy;
   wire 	  rxb_src_rdy, rxb_dst_rdy;
   wire 	  dummy;

   fifo_cascade #(.WIDTH(36), .SIZE(RXFIFOSIZE)) rx_buffering(
        .clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
        .datain(rx_data), .src_rdy_i(rx_src_rdy), .dst_rdy_o(rx_dst_rdy),
        .dataout(rxb_data), .src_rdy_o(rxb_src_rdy), .dst_rdy_i(rxb_dst_rdy)
   );

   fifo36_to_fifo19 #(.LE(1)) f36_to_f19   // Little endian because ARM is LE
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .f36_datain(rxb_data), .f36_src_rdy_i(rxb_src_rdy), .f36_dst_rdy_o(rxb_dst_rdy),
      .f19_dataout({dummy,rx18_data}), .f19_src_rdy_o(rx18_src_rdy), .f19_dst_rdy_i(rx18_dst_rdy) );

   fifo_to_gpmc #(.ADDR_WIDTH(ADDR_WIDTH)) fifo_to_gpmc
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx), .arst(fifo_rst | clear_rx | arst),
      .data_i(rx18_data), .src_rdy_i(rx18_src_rdy), .dst_rdy_o(rx18_dst_rdy),
      .EM_D(EM_D_fifo), .EM_A(EM_A), .EM_CLK(EM_CLK), .EM_OE(~EM_NCS4 & ~EM_NOE),
      .data_available(rx_have_data));

   // ////////////////////////////////////////////
   // Control path on CS6
   
   gpmc_wb gpmc_wb
     (.EM_CLK(EM_CLK), .EM_D_in(EM_D), .EM_D_out(EM_D_wb), .EM_A(EM_A), .EM_NBE(EM_NBE),
      .EM_WE(~EM_NCS6 & ~EM_NWE), .EM_OE(~EM_NCS6 & ~EM_NOE),
      .wb_clk(wb_clk), .wb_rst(wb_rst),
      .wb_adr_o(wb_adr_o), .wb_dat_mosi(wb_dat_mosi), .wb_dat_miso(wb_dat_miso),
      .wb_sel_o(wb_sel_o), .wb_cyc_o(wb_cyc_o), .wb_stb_o(wb_stb_o), .wb_we_o(wb_we_o),
      .wb_ack_i(wb_ack_i) );

   // ////////////////////////////////////////////
   // Test support, traffic generator, loopback, etc.

   // RX side muxes test data into the same stream
   wire [35:0] loopbackrx_data, testrx_data;
   wire [35:0] loopbacktx_data, testtx_data;
   wire        loopbackrx_src_rdy, loopbackrx_dst_rdy;
   wire        loopbacktx_src_rdy, loopbacktx_dst_rdy;
   wire        sel_testtx = test_ctrl[0];

   fifo36_mux rx_test_mux_lvl_2
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .data0_i(loopbackrx_data), .src0_rdy_i(loopbackrx_src_rdy), .dst0_rdy_o(loopbackrx_dst_rdy),
      .data1_i(rx_data_i), .src1_rdy_i(rx_src_rdy_i), .dst1_rdy_o(rx_dst_rdy_o),
      .data_o(rx_data), .src_rdy_o(rx_src_rdy), .dst_rdy_i(rx_dst_rdy));

   fifo_short #(.WIDTH(36)) loopback_fifo
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_tx | clear_rx),
      .datain(loopbacktx_data), .src_rdy_i(loopbacktx_src_rdy), .dst_rdy_o(loopbacktx_dst_rdy),
      .dataout(loopbackrx_data), .src_rdy_o(loopbackrx_src_rdy), .dst_rdy_i(loopbackrx_dst_rdy));

   // Crossbar used as a demux for switching TX stream to main DSP or to test logic
   crossbar36 tx_crossbar_lvl_1
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_tx),
      .cross(sel_testtx),
      .data0_i(tx_data), .src0_rdy_i(tx_src_rdy), .dst0_rdy_o(tx_dst_rdy),
      .data1_i(tx_data), .src1_rdy_i(1'b0), .dst1_rdy_o(),  // No 2nd input
      .data0_o(tx_data_o), .src0_rdy_o(tx_src_rdy_o), .dst0_rdy_i(tx_dst_rdy_i),
      .data1_o(loopbacktx_data), .src1_rdy_o(loopbacktx_src_rdy), .dst1_rdy_i(loopbacktx_dst_rdy) );

   assign debug = {
        EM_D, //16
        EM_A, //10
        EM_CLK, EM_NCS4, EM_NWE, EM_NOE, //4
        EM_NCS6, wb_ack_i
   };

endmodule // gpmc
