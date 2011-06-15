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

module gpif
  #(parameter TXFIFOSIZE = 11, parameter RXFIFOSIZE = 11)
   (// GPIF signals
    input gpif_clk, input gpif_rst,
    inout [15:0] gpif_d, input [3:0] gpif_ctl, output [3:0] gpif_rdy,
    output [2:0] gpif_misc,
    
    // Wishbone signals
    input wb_clk, input wb_rst,
    output [15:0] wb_adr_o, output [15:0] wb_dat_mosi, input [15:0] wb_dat_miso,
    output [1:0] wb_sel_o, output wb_cyc_o, output wb_stb_o, output wb_we_o, input wb_ack_i,
    input [7:0] triggers,
    
    // FIFO interface
    input fifo_clk, input fifo_rst, input clear_tx, input clear_rx,
    output [35:0] tx_data_o, output tx_src_rdy_o, input tx_dst_rdy_i,
    input [35:0] rx_data_i, input rx_src_rdy_i, output rx_dst_rdy_o,
    input [35:0] tx_err_data_i, input tx_err_src_rdy_i, output tx_err_dst_rdy_o,
    
    output tx_underrun, output rx_overrun,
    input [7:0] frames_per_packet, input [15:0] test_len, input [7:0] test_rate, input [3:0] test_ctrl,
    output [31:0] debug0, output [31:0] debug1
    );

   wire 	  WR = gpif_ctl[0];
   wire 	  RD = gpif_ctl[1];
   wire 	  OE = gpif_ctl[2];
   wire 	  EP = gpif_ctl[3];

   wire 	  CF, CE, DF, DE;
   
   assign gpif_rdy = { CF, CE, DF, DE };
   
   wire [15:0] 	  gpif_d_out;
   assign gpif_d = OE ? gpif_d_out : 16'bz;

   wire [15:0] 	  gpif_d_copy = gpif_d;

   wire [31:0] 	  debug_rd, debug_wr, debug_split0, debug_split1;
   
   // ////////////////////////////////////////////////////////////////////
   // TX Data Path

   wire [18:0] 	  tx19_data;
   wire 	  tx19_src_rdy, tx19_dst_rdy;
   wire [35:0] 	  tx36_data, tx_data;
   wire 	  tx36_src_rdy, tx36_dst_rdy, tx_src_rdy, tx_dst_rdy;

   wire [18:0] 	  ctrl_data;
   wire 	  ctrl_src_rdy, ctrl_dst_rdy;
   
   gpif_wr gpif_wr
     (.gpif_clk(gpif_clk), .gpif_rst(gpif_rst), 
      .gpif_data(gpif_d), .gpif_wr(WR), .gpif_ep(EP),
      .gpif_full_d(DF), .gpif_full_c(CF),
      
      .sys_clk(fifo_clk), .sys_rst(fifo_rst),
      .data_o(tx19_data), .src_rdy_o(tx19_src_rdy), .dst_rdy_i(tx19_dst_rdy),
      .ctrl_o(ctrl_data), .ctrl_src_rdy_o(ctrl_src_rdy), .ctrl_dst_rdy_i(ctrl_dst_rdy),
      .debug(debug_wr) );

   // join vita packets which are longer than one frame, drop frame padding
   wire [18:0] 	  refr_data;
   wire 	  refr_src_rdy, refr_dst_rdy;
   
   packet_reframer tx_packet_reframer 
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_tx),
      .data_i(tx19_data), .src_rdy_i(tx19_src_rdy), .dst_rdy_o(tx19_dst_rdy),
      .data_o(refr_data), .src_rdy_o(refr_src_rdy), .dst_rdy_i(refr_dst_rdy));

   fifo19_to_fifo36 #(.LE(1)) f19_to_f36
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .f19_datain(refr_data), .f19_src_rdy_i(refr_src_rdy), .f19_dst_rdy_o(refr_dst_rdy),
      .f36_dataout(tx36_data), .f36_src_rdy_o(tx36_src_rdy), .f36_dst_rdy_i(tx36_dst_rdy));
   
   fifo_cascade #(.WIDTH(36), .SIZE(TXFIFOSIZE)) tx_fifo36
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_tx),
      .datain(tx36_data), .src_rdy_i(tx36_src_rdy), .dst_rdy_o(tx36_dst_rdy),
      .dataout(tx_data), .src_rdy_o(tx_src_rdy), .dst_rdy_i(tx_dst_rdy));

   // ////////////////////////////////////////////
   // RX Data Path

   wire [35:0] 	  rx36_data, rx_data;
   wire 	  rx36_src_rdy, rx36_dst_rdy, rx_src_rdy, rx_dst_rdy;
   wire [18:0] 	  rx19_data, splt_data;
   wire 	  rx19_src_rdy, rx19_dst_rdy, splt_src_rdy, splt_dst_rdy;
   wire [18:0] 	  resp_data, resp_int1, resp_int2;
   wire 	  resp_src_rdy, resp_dst_rdy;
   wire 	  resp_src_rdy_int1, resp_dst_rdy_int1, resp_src_rdy_int2, resp_dst_rdy_int2;
   
   fifo_cascade #(.WIDTH(36), .SIZE(RXFIFOSIZE)) rx_fifo36
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .datain(rx_data), .src_rdy_i(rx_src_rdy), .dst_rdy_o(rx_dst_rdy),
      .dataout(rx36_data), .src_rdy_o(rx36_src_rdy), .dst_rdy_i(rx36_dst_rdy));

   fifo36_to_fifo19 #(.LE(1)) f36_to_f19
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .f36_datain(rx36_data), .f36_src_rdy_i(rx36_src_rdy), .f36_dst_rdy_o(rx36_dst_rdy),
      .f19_dataout(rx19_data), .f19_src_rdy_o(rx19_src_rdy), .f19_dst_rdy_i(rx19_dst_rdy) );

   packet_splitter #(.FRAME_LEN(256)) packet_splitter
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .frames_per_packet(frames_per_packet),
      .data_i(rx19_data), .src_rdy_i(rx19_src_rdy), .dst_rdy_o(rx19_dst_rdy),
      .data_o(splt_data), .src_rdy_o(splt_src_rdy), .dst_rdy_i(splt_dst_rdy),
      .debug0(debug_split0), .debug1(debug_split1));
     
   gpif_rd gpif_rd
     (.gpif_clk(gpif_clk), .gpif_rst(gpif_rst),
      .gpif_data(gpif_d_out), .gpif_rd(RD), .gpif_ep(EP),
      .gpif_empty_d(DE), .gpif_empty_c(CE), .gpif_flush(gpif_misc[0]),
      
      .sys_clk(fifo_clk), .sys_rst(fifo_rst),
      .data_i(splt_data), .src_rdy_i(splt_src_rdy), .dst_rdy_o(splt_dst_rdy),
      .resp_i(resp_data), .resp_src_rdy_i(resp_src_rdy), .resp_dst_rdy_o(resp_dst_rdy),
      .debug(debug_rd) );

   // ////////////////////////////////////////////////////////////////////
   // FIFO to Wishbone interface

   fifo_to_wb fifo_to_wb
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .data_i(ctrl_data), .src_rdy_i(ctrl_src_rdy), .dst_rdy_o(ctrl_dst_rdy),
      .data_o(resp_int1), .src_rdy_o(resp_src_rdy_int1), .dst_rdy_i(resp_dst_rdy_int1),
      .wb_adr_o(wb_adr_o), .wb_dat_mosi(wb_dat_mosi), .wb_dat_miso(wb_dat_miso), .wb_sel_o(wb_sel_o), 
      .wb_cyc_o(wb_cyc_o), .wb_stb_o(wb_stb_o), .wb_we_o(wb_we_o), .wb_ack_i(wb_ack_i),
      .triggers(triggers),
      .debug0(), .debug1());

   wire [18:0] 	  tx_err19_data;
   wire 	  tx_err19_src_rdy, tx_err19_dst_rdy;
   
   fifo36_to_fifo19 #(.LE(1)) f36_to_f19_txerr
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .f36_datain(tx_err_data_i), .f36_src_rdy_i(tx_err_src_rdy_i), .f36_dst_rdy_o(tx_err_dst_rdy_o),
      .f19_dataout(tx_err19_data), .f19_src_rdy_o(tx_err19_src_rdy), .f19_dst_rdy_i(tx_err19_dst_rdy) );

   fifo19_mux #(.prio(0)) mux_err_stream
     (.clk(wb_clk), .reset(wb_rst), .clear(0),
      .data0_i(resp_int1), .src0_rdy_i(resp_src_rdy_int1), .dst0_rdy_o(resp_dst_rdy_int1),
      .data1_i(tx_err19_data), .src1_rdy_i(tx_err19_src_rdy), .dst1_rdy_o(tx_err19_dst_rdy),
      .data_o(resp_int2), .src_rdy_o(resp_src_rdy_int2), .dst_rdy_i(resp_dst_rdy_int2));
									
   fifo19_pad #(.LENGTH(16)) fifo19_pad
     (.clk(fifo_clk), .reset(fifo_rst), .clear(0),
      .data_i(resp_int2), .src_rdy_i(resp_src_rdy_int2), .dst_rdy_o(resp_dst_rdy_int2),
      .data_o(resp_data), .src_rdy_o(resp_src_rdy), .dst_rdy_i(resp_dst_rdy));
        
   // ////////////////////////////////////////////////////////////////////
   // Debug support, timed and loopback
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
   vita_pkt_gen pktgen
     (.clk(fifo_clk), .reset(fifo_rst), .clear(clear_rx),
      .len(test_len),
      .data_o(timedrx_data), .src_rdy_o(timedrx_src_rdy_int), .dst_rdy_i(timedrx_dst_rdy_int));

   fifo_pacer rx_pacer
     (.clk(fifo_clk), .reset(fifo_rst), .rate(test_rate), .enable(pkt_src_enable),
      .src1_rdy_i(timedrx_src_rdy_int), .dst1_rdy_o(timedrx_dst_rdy_int),
      .src2_rdy_o(timedrx_src_rdy), .dst2_rdy_i(timedrx_dst_rdy),
      .underrun(), .overrun(rx_overrun));

   // ////////////////////////////////////////////
   //    DEBUG
   
   //assign debug0 = { rx19_src_rdy, rx19_dst_rdy, resp_src_rdy, resp_dst_rdy, gpif_ctl[3:0], gpif_rdy[3:0], 
	//	     gpif_d_copy[15:0] };
   
   //assign debug1 = { { debug_rd[15:8] },
	//	     { debug_rd[7:0] },
		//     { rx_src_rdy_i, rx_dst_rdy_o, rx36_src_rdy, rx36_dst_rdy, rx19_src_rdy, rx19_dst_rdy, resp_src_rdy, resp_dst_rdy},
		  //   { tx_src_rdy_o, tx_dst_rdy_i, tx19_src_rdy, tx19_dst_rdy, tx36_src_rdy, tx36_dst_rdy, ctrl_src_rdy, ctrl_dst_rdy} };
   
   assign debug0 = { gpif_ctl[3:0], gpif_rdy[3:0], debug_split0[23:0] };
   assign debug1 = { gpif_misc[0], debug_rd[14:0], debug_split1[15:8], debug_split1[7:0] };
endmodule // gpif
