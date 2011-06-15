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

module gpmc_async
  #(parameter TXFIFOSIZE = 11, 
    parameter RXFIFOSIZE = 11,
    parameter BUSDEBUG = 1)
   (// GPMC signals
    input arst,
    input EM_CLK, inout [15:0] EM_D, input [10:1] EM_A, input [1:0] EM_NBE,
    input EM_WAIT0, input EM_NCS4, input EM_NCS6, input EM_NWE, input EM_NOE,
    
    // GPIOs for FIFO signalling
    output rx_have_data, output tx_have_space, output reg bus_error, input bus_reset,
    
    // Wishbone signals
    input wb_clk, input wb_rst,
    output [10:0] wb_adr_o, output [15:0] wb_dat_mosi, input [15:0] wb_dat_miso,
    output [1:0] wb_sel_o, output wb_cyc_o, output wb_stb_o, output wb_we_o, input wb_ack_i,
    
    // FIFO interface
    input fifo_clk, input fifo_rst, input clear_tx, input clear_rx,
    output [35:0] tx_data_o, output tx_src_rdy_o, input tx_dst_rdy_i,
    input [35:0] rx_data_i, input rx_src_rdy_i, output rx_dst_rdy_o,
    
    input [15:0] tx_frame_len, output [15:0] rx_frame_len,

    output tx_underrun, output rx_overrun,
    input [7:0] test_rate, input [3:0] test_ctrl,
    output [31:0] debug
    );

   wire 	  EM_output_enable = (~EM_NOE & (~EM_NCS4 | ~EM_NCS6));
   wire [15:0] 	  EM_D_fifo;
   wire [15:0] 	  EM_D_wb;
   
   assign EM_D = ~EM_output_enable ? 16'bz : ~EM_NCS4 ? EM_D_fifo : EM_D_wb;
   
   wire 	  bus_error_tx, bus_error_rx;
   
   always @(posedge fifo_clk)
     if(fifo_rst | clear_tx | clear_rx)
       bus_error <= 0;
     else
       bus_error <= bus_error_tx | bus_error_rx;
   
   // CS4 is RAM_2PORT for DATA PATH (high-speed data)
   //    Writes go into one RAM, reads come from the other
   // CS6 is for CONTROL PATH (wishbone)

   // ////////////////////////////////////////////
   // TX Data Path

   wire [17:0] 	  tx18_data, tx18b_data;
   wire 	  tx18_src_rdy, tx18_dst_rdy, tx18b_src_rdy, tx18b_dst_rdy;
   wire [15:0] 	  tx_fifo_space;
   wire [35:0] 	  tx36_data, tx_data;
   wire 	  tx36_src_rdy, tx36_dst_rdy, tx_src_rdy, tx_dst_rdy;
   
   gpmc_to_fifo_async gpmc_to_fifo_async
     (.EM_D(EM_D), .EM_NBE(EM_NBE), .EM_NCS(EM_NCS4), .EM_NWE(EM_NWE),
      .fifo_clk(fifo_clk), .fifo_rst(fifo_rst), .clear(clear_tx),
      .data_o(tx18_data), .src_rdy_o(tx18_src_rdy), .dst_rdy_i(tx18_dst_rdy),
      .frame_len(tx_frame_len), .fifo_space(tx_fifo_space), .fifo_ready(tx_have_space),
      .bus_error(bus_error_tx) );
   
   fifo_cascade #(.WIDTH(18), .SIZE(10)) tx_fifo
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_tx),
      .datain(tx18_data), .src_rdy_i(tx18_src_rdy), .dst_rdy_o(tx18_dst_rdy), .space(tx_fifo_space),
      .dataout(tx18b_data), .src_rdy_o(tx18b_src_rdy), .dst_rdy_i(tx18b_dst_rdy), .occupied());

   fifo19_to_fifo36 #(.LE(1)) f19_to_f36   // Little endian because ARM is LE
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_tx),
      .f19_datain({1'b0,tx18b_data}), .f19_src_rdy_i(tx18b_src_rdy), .f19_dst_rdy_o(tx18b_dst_rdy),
      .f36_dataout(tx36_data), .f36_src_rdy_o(tx36_src_rdy), .f36_dst_rdy_i(tx36_dst_rdy));
   
   fifo_cascade #(.WIDTH(36), .SIZE(TXFIFOSIZE)) tx_fifo36
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_tx),
      .datain(tx36_data), .src_rdy_i(tx36_src_rdy), .dst_rdy_o(tx36_dst_rdy),
      .dataout(tx_data), .src_rdy_o(tx_src_rdy), .dst_rdy_i(tx_dst_rdy));

   // ////////////////////////////////////////////
   // RX Data Path
   
   wire [17:0] 	  rx18_data, rx18b_data;
   wire 	  rx18_src_rdy, rx18_dst_rdy, rx18b_src_rdy, rx18b_dst_rdy;
   wire [15:0] 	  rx_fifo_space;
   wire [35:0] 	  rx36_data, rx_data;
   wire 	  rx36_src_rdy, rx36_dst_rdy, rx_src_rdy, rx_dst_rdy;
   wire 	  dummy;
   
   fifo_cascade #(.WIDTH(36), .SIZE(RXFIFOSIZE)) rx_fifo36
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .datain(rx_data), .src_rdy_i(rx_src_rdy), .dst_rdy_o(rx_dst_rdy),
      .dataout(rx36_data), .src_rdy_o(rx36_src_rdy), .dst_rdy_i(rx36_dst_rdy));

   fifo36_to_fifo19 #(.LE(1)) f36_to_f19   // Little endian because ARM is LE
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .f36_datain(rx36_data), .f36_src_rdy_i(rx36_src_rdy), .f36_dst_rdy_o(rx36_dst_rdy),
      .f19_dataout({dummy,rx18_data}), .f19_src_rdy_o(rx18_src_rdy), .f19_dst_rdy_i(rx18_dst_rdy) );

   fifo_cascade #(.WIDTH(18), .SIZE(12)) rx_fifo
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .datain(rx18_data), .src_rdy_i(rx18_src_rdy), .dst_rdy_o(rx18_dst_rdy), .space(rx_fifo_space),
      .dataout(rx18b_data), .src_rdy_o(rx18b_src_rdy), .dst_rdy_i(rx18b_dst_rdy), .occupied());

   fifo_to_gpmc_async fifo_to_gpmc_async
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .data_i(rx18b_data), .src_rdy_i(rx18b_src_rdy), .dst_rdy_o(rx18b_dst_rdy),
      .EM_D(EM_D_fifo), .EM_NCS(EM_NCS4), .EM_NOE(EM_NOE),
      .frame_len(rx_frame_len) );

   wire [31:0] 	pkt_count;
   
   fifo_watcher fifo_watcher
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .src_rdy1(rx18_src_rdy), .dst_rdy1(rx18_dst_rdy), .sof1(rx18_data[16]), .eof1(rx18_data[17]),
      .src_rdy2(rx18b_src_rdy), .dst_rdy2(rx18b_dst_rdy), .sof2(rx18b_data[16]), .eof2(rx18b_data[17]),
      .have_packet(rx_have_data), .length(rx_frame_len), .bus_error(bus_error_rx),
      .debug(pkt_count));

   // ////////////////////////////////////////////
   // Control path on CS6
   
   gpmc_wb gpmc_wb
     (.EM_CLK(EM_CLK), .EM_D_in(EM_D), .EM_D_out(EM_D_wb), .EM_A(EM_A), .EM_NBE(EM_NBE),
      .EM_NCS(EM_NCS6), .EM_NWE(EM_NWE), .EM_NOE(EM_NOE),
      .wb_clk(wb_clk), .wb_rst(wb_rst),
      .wb_adr_o(wb_adr_o), .wb_dat_mosi(wb_dat_mosi), .wb_dat_miso(wb_dat_miso),
      .wb_sel_o(wb_sel_o), .wb_cyc_o(wb_cyc_o), .wb_stb_o(wb_stb_o), .wb_we_o(wb_we_o),
      .wb_ack_i(wb_ack_i) );
   
//      assign debug = pkt_count;

   // ////////////////////////////////////////////
   // Test support, traffic generator, loopback, etc.

   // RX side muxes test data into the same stream
   wire [35:0] 	timedrx_data, loopbackrx_data, testrx_data;
   wire [35:0] 	timedtx_data, loopbacktx_data, testtx_data;
   wire 	timedrx_src_rdy, timedrx_dst_rdy, loopbackrx_src_rdy, loopbackrx_dst_rdy,
		testrx_src_rdy, testrx_dst_rdy;
   wire 	timedtx_src_rdy, timedtx_dst_rdy, loopbacktx_src_rdy, loopbacktx_dst_rdy,
		testtx_src_rdy, testtx_dst_rdy;
   wire 	timedrx_src_rdy_int, timedrx_dst_rdy_int, timedtx_src_rdy_int, timedtx_dst_rdy_int;

   wire [31:0] 	total, crc_err, seq_err, len_err;
   wire 	sel_testtx = test_ctrl[0];
   wire 	sel_loopbacktx = test_ctrl[1];
   wire 	pkt_src_enable = test_ctrl[2];
   wire 	pkt_sink_enable = test_ctrl[3];
   
   fifo36_mux rx_test_mux_lvl_1
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .data0_i(timedrx_data), .src0_rdy_i(timedrx_src_rdy), .dst0_rdy_o(timedrx_dst_rdy),
      .data1_i(loopbackrx_data), .src1_rdy_i(loopbackrx_src_rdy), .dst1_rdy_o(loopbackrx_dst_rdy),
      .data_o(testrx_data), .src_rdy_o(testrx_src_rdy), .dst_rdy_i(testrx_dst_rdy));
   
   fifo36_mux rx_test_mux_lvl_2
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .data0_i(testrx_data), .src0_rdy_i(testrx_src_rdy), .dst0_rdy_o(testrx_dst_rdy),
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
      .data1_o(testtx_data), .src1_rdy_o(testtx_src_rdy), .dst1_rdy_i(testtx_dst_rdy) );
   
   crossbar36 tx_crossbar_lvl_2
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_tx),
      .cross(sel_loopbacktx),
      .data0_i(testtx_data), .src0_rdy_i(testtx_src_rdy), .dst0_rdy_o(testtx_dst_rdy),
      .data1_i(testtx_data), .src1_rdy_i(1'b0), .dst1_rdy_o(),  // No 2nd input
      .data0_o(timedtx_data), .src0_rdy_o(timedtx_src_rdy), .dst0_rdy_i(timedtx_dst_rdy),
      .data1_o(loopbacktx_data), .src1_rdy_o(loopbacktx_src_rdy), .dst1_rdy_i(loopbacktx_dst_rdy) );
   
   // Fixed rate TX traffic consumer
   fifo_pacer tx_pacer
     (.clk(fifo_clk), .reset(fifo_rst), .rate(test_rate), .enable(pkt_sink_enable),
      .src1_rdy_i(timedtx_src_rdy), .dst1_rdy_o(timedtx_dst_rdy),
      .src2_rdy_o(timedtx_src_rdy_int), .dst2_rdy_i(timedtx_dst_rdy_int),
      .underrun(tx_underrun), .overrun());

   packet_verifier32 pktver32
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_tx),
      .data_i(timedtx_data), .src_rdy_i(timedtx_src_rdy_int), .dst_rdy_o(timedtx_dst_rdy_int),
      .total(total), .crc_err(crc_err), .seq_err(seq_err), .len_err(len_err));

   // Fixed rate RX traffic generator
   packet_generator32 pktgen32
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .header({len_err,seq_err,crc_err,total}),
      .data_o(timedrx_data), .src_rdy_o(timedrx_src_rdy_int), .dst_rdy_i(timedrx_dst_rdy_int));

   fifo_pacer rx_pacer
     (.clk(fifo_clk), .reset(fifo_rst), .rate(test_rate), .enable(pkt_src_enable),
      .src1_rdy_i(timedrx_src_rdy_int), .dst1_rdy_o(timedrx_dst_rdy_int),
      .src2_rdy_o(timedrx_src_rdy), .dst2_rdy_i(timedrx_dst_rdy),
      .underrun(), .overrun(rx_overrun));

   // FIXME -- hook up crossbar controls
   // // FIXME -- collect error stats
   // FIXME -- set rates and enables on pacers
   // FIXME -- make sure packet completes before we shutoff
   // FIXME -- handle overrun and underrun

wire [0:17] dummy18;

assign debug = {8'd0,
		test_rate,
		pkt_src_enable, pkt_sink_enable, timedrx_src_rdy_int, timedrx_dst_rdy_int,
		timedrx_src_rdy, timedrx_dst_rdy,
		testrx_src_rdy, testrx_dst_rdy,
		rx_src_rdy, rx_dst_rdy,
		rx36_src_rdy, rx36_dst_rdy,
		rx18_src_rdy, rx18_dst_rdy,
		rx18b_src_rdy, rx18b_dst_rdy};

endmodule // gpmc_async
