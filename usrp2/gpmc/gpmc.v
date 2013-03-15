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
    output rx_have_data, output tx_have_space, output resp_have_data,

    // FIFO interface
    input fifo_clk, input fifo_rst,
    output [35:0] tx_data, output tx_src_rdy, input tx_dst_rdy,
    input [35:0] rx_data, input rx_src_rdy, output rx_dst_rdy,
    output [35:0] ctrl_data, output ctrl_src_rdy, input ctrl_dst_rdy,
    input [35:0] resp_data, input resp_src_rdy, output resp_dst_rdy,

    output [31:0] debug
    );

   wire EM_output_enable = (~EM_NOE & (~EM_NCS4 | ~EM_NCS6));
   wire [15:0] 	  EM_D_data;
   wire [15:0] 	  EM_D_ctrl;

   assign EM_D = ~EM_output_enable ? 16'bz : ~EM_NCS4 ? EM_D_data : EM_D_ctrl;

   // CS4 is RAM_2PORT for DATA PATH (high-speed data)
   //    Writes go into one RAM, reads come from the other
   // CS6 is for CONTROL PATH (slow)

   // ////////////////////////////////////////////
   // TX Data Path

   wire [17:0] 	  tx18_data;
   wire 	  tx18_src_rdy, tx18_dst_rdy;
   wire [35:0] 	  txb_data;
   wire 	  txb_src_rdy, txb_dst_rdy;

   gpmc_to_fifo #(.ADDR_WIDTH(10), .LAST_ADDR(10'h3ff), .PTR_WIDTH(2)) gpmc_to_fifo
     (.EM_D(EM_D), .EM_A(EM_A[10:1]), .EM_CLK(EM_CLK), .EM_WE(~EM_NCS4 & ~EM_NWE),
      .clk(fifo_clk), .reset(fifo_rst), .clear(1'b0), .arst(fifo_rst | arst),
      .data_o(tx18_data), .src_rdy_o(tx18_src_rdy), .dst_rdy_i(tx18_dst_rdy),
      .have_space(tx_have_space));

   fifo19_to_fifo36 #(.LE(1)) f19_to_f36   // Little endian because ARM is LE
     (.clk(fifo_clk), .reset(fifo_rst), .clear(1'b0),
      .f19_datain({1'b0,tx18_data}), .f19_src_rdy_i(tx18_src_rdy), .f19_dst_rdy_o(tx18_dst_rdy),
      .f36_dataout(txb_data), .f36_src_rdy_o(txb_src_rdy), .f36_dst_rdy_i(txb_dst_rdy));

   fifo_cascade #(.WIDTH(36), .SIZE(TXFIFOSIZE)) tx_buffering(
        .clk(fifo_clk), .reset(fifo_rst), .clear(1'b0),
        .datain(txb_data), .src_rdy_i(txb_src_rdy), .dst_rdy_o(txb_dst_rdy),
        .dataout(tx_data), .src_rdy_o(tx_src_rdy), .dst_rdy_i(tx_dst_rdy)
   );

   // ////////////////////////////////////////////
   // RX Data Path
   
   wire [17:0] 	  rx18_data;
   wire 	  rx18_src_rdy, rx18_dst_rdy;
   wire [35:0] 	  rxb_data;
   wire 	  rxb_src_rdy, rxb_dst_rdy;
   wire 	  dummy;

   fifo_cascade #(.WIDTH(36), .SIZE(RXFIFOSIZE)) rx_buffering(
        .clk(fifo_clk), .reset(fifo_rst), .clear(1'b0),
        .datain(rx_data), .src_rdy_i(rx_src_rdy), .dst_rdy_o(rx_dst_rdy),
        .dataout(rxb_data), .src_rdy_o(rxb_src_rdy), .dst_rdy_i(rxb_dst_rdy)
   );

   fifo36_to_fifo19 #(.LE(1)) f36_to_f19   // Little endian because ARM is LE
     (.clk(fifo_clk), .reset(fifo_rst), .clear(1'b0),
      .f36_datain(rxb_data), .f36_src_rdy_i(rxb_src_rdy), .f36_dst_rdy_o(rxb_dst_rdy),
      .f19_dataout({dummy,rx18_data}), .f19_src_rdy_o(rx18_src_rdy), .f19_dst_rdy_i(rx18_dst_rdy) );

   fifo_to_gpmc #(.ADDR_WIDTH(ADDR_WIDTH), .LAST_ADDR(10'h3ff)) fifo_to_gpmc
     (.clk(fifo_clk), .reset(fifo_rst), .clear(1'b0), .arst(fifo_rst | arst),
      .data_i(rx18_data), .src_rdy_i(rx18_src_rdy), .dst_rdy_o(rx18_dst_rdy),
      .EM_D(EM_D_data), .EM_A(EM_A), .EM_CLK(EM_CLK), .EM_OE(~EM_NCS4 & ~EM_NOE),
      .data_available(rx_have_data));

   // ////////////////////////////////////////////
   // Control path on CS6

   // ////////////////////////////////////////////////////////////////////
   // CTRL TX Data Path

   wire [17:0] 	  ctrl18_data;
   wire 	  ctrl18_src_rdy, ctrl18_dst_rdy;
   wire [35:0] 	  ctrlb_data;
   wire 	  ctrlb_src_rdy, ctrlb_dst_rdy;

   gpmc_to_fifo #(.PTR_WIDTH(5), .ADDR_WIDTH(5), .LAST_ADDR(5'h0f)) ctrl_gpmc_to_fifo
     (.EM_D(EM_D), .EM_A(EM_A[5:1]), .EM_CLK(EM_CLK), .EM_WE(~EM_NCS6 & ~EM_NWE),
      .clk(fifo_clk), .reset(fifo_rst), .clear(1'b0), .arst(fifo_rst | arst),
      .data_o(ctrl18_data), .src_rdy_o(ctrl18_src_rdy), .dst_rdy_i(ctrl18_dst_rdy),
      .have_space(/*always*/));

   fifo19_to_fifo36 #(.LE(1)) ctrl_f19_to_f36   // Little endian because ARM is LE
     (.clk(fifo_clk), .reset(fifo_rst), .clear(1'b0),
      .f19_datain({1'b0,ctrl18_data}), .f19_src_rdy_i(ctrl18_src_rdy), .f19_dst_rdy_o(ctrl18_dst_rdy),
      .f36_dataout(ctrlb_data), .f36_src_rdy_o(ctrlb_src_rdy), .f36_dst_rdy_i(ctrlb_dst_rdy));

   fifo_cascade #(.WIDTH(36), .SIZE(9)) ctrl_buffering(
        .clk(fifo_clk), .reset(fifo_rst), .clear(1'b0),
        .datain(ctrlb_data), .src_rdy_i(ctrlb_src_rdy), .dst_rdy_o(ctrlb_dst_rdy),
        .dataout(ctrl_data), .src_rdy_o(ctrl_src_rdy), .dst_rdy_i(ctrl_dst_rdy)
   );

   // ////////////////////////////////////////////
   // CTRL RX Data Path

   wire [17:0] 	  resp18_data;
   wire 	  resp18_src_rdy, resp18_dst_rdy;
   wire [35:0] 	  respb_data;
   wire 	  respb_src_rdy, respb_dst_rdy;
   wire 	  resp_dummy;

   fifo_cascade #(.WIDTH(36), .SIZE(9)) resp_buffering(
        .clk(fifo_clk), .reset(fifo_rst), .clear(1'b0),
        .datain(resp_data), .src_rdy_i(resp_src_rdy), .dst_rdy_o(resp_dst_rdy),
        .dataout(respb_data), .src_rdy_o(respb_src_rdy), .dst_rdy_i(respb_dst_rdy)
   );

   fifo36_to_fifo19 #(.LE(1)) resp_f36_to_f19   // Little endian because ARM is LE
     (.clk(fifo_clk), .reset(fifo_rst), .clear(1'b0),
      .f36_datain(respb_data), .f36_src_rdy_i(respb_src_rdy), .f36_dst_rdy_o(respb_dst_rdy),
      .f19_dataout({resp_dummy,resp18_data}), .f19_src_rdy_o(resp18_src_rdy), .f19_dst_rdy_i(resp18_dst_rdy) );

   fifo_to_gpmc #(.PTR_WIDTH(5), .ADDR_WIDTH(5), .LAST_ADDR(5'h0f)) resp_fifo_to_gpmc
     (.clk(fifo_clk), .reset(fifo_rst), .clear(1'b0), .arst(fifo_rst | arst),
      .data_i(resp18_data), .src_rdy_i(resp18_src_rdy), .dst_rdy_o(resp18_dst_rdy),
      .EM_D(EM_D_ctrl), .EM_A(EM_A[5:1]), .EM_CLK(EM_CLK), .EM_OE(~EM_NCS6 & ~EM_NOE),
      .data_available(resp_have_data));
//*
    assign debug = {
        EM_D,
        //resp18_data[15:0], //16
        EM_A, //10
        //resp18_data[17:16], resp18_src_rdy, resp18_dst_rdy, //4
        EM_NCS4, EM_NCS6, EM_NWE, EM_NOE, //4
        EM_CLK, resp_have_data //2
    };
//*/
endmodule // gpmc
