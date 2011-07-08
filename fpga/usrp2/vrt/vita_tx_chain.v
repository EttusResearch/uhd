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


module vita_tx_chain
  #(parameter BASE_CTRL=0,
    parameter BASE_DSP=0,
    parameter REPORT_ERROR=0,
    parameter DO_FLOW_CONTROL=0,
    parameter PROT_ENG_FLAGS=0,
    parameter USE_TRANS_HEADER=0,
    parameter DSP_NUMBER=0)
   (input clk, input reset,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    input [63:0] vita_time,
    input [35:0] tx_data_i, input tx_src_rdy_i, output tx_dst_rdy_o,
    output [35:0] err_data_o, output err_src_rdy_o, input err_dst_rdy_i,
    output [23:0] tx_i, output [23:0] tx_q,
    output underrun, output run,
    output [31:0] debug);

   localparam MAXCHAN = 1;
   localparam FIFOWIDTH = 5+64+16+(32*MAXCHAN);

   wire [FIFOWIDTH-1:0] tx1_data;
   wire 		tx1_src_rdy, tx1_dst_rdy;
   wire 		clear_vita;
   wire [31:0] 		sample_tx;
   wire [31:0] 		streamid, message;
   wire 		trigger, sent;
   wire [31:0] 		debug_vtc, debug_vtd, debug_tx_dsp;

   wire 		error, packet_consumed, ack;
   wire [31:0] 		error_code;
   wire 		clear_seqnum;
   wire [31:0] 		current_seqnum;
   wire 		strobe_tx;
   
   assign underrun = error;
   assign message = error_code;
   
   setting_reg #(.my_addr(BASE_CTRL+1)) sr
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(),.changed(clear_vita));

   setting_reg #(.my_addr(BASE_CTRL+2), .at_reset(0)) sr_streamid
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(streamid),.changed(clear_seqnum));

   vita_tx_deframer #(.BASE(BASE_CTRL), 
		      .MAXCHAN(MAXCHAN), 
		      .USE_TRANS_HEADER(USE_TRANS_HEADER)) 
   vita_tx_deframer
     (.clk(clk), .reset(reset), .clear(clear_vita), .clear_seqnum(clear_seqnum),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .data_i(tx_data_i), .src_rdy_i(tx_src_rdy_i), .dst_rdy_o(tx_dst_rdy_o),
      .sample_fifo_o(tx1_data), .sample_fifo_src_rdy_o(tx1_src_rdy), .sample_fifo_dst_rdy_i(tx1_dst_rdy),
      .current_seqnum(current_seqnum),
      .debug(debug_vtd) );

   vita_tx_control #(.BASE(BASE_CTRL), .WIDTH(32*MAXCHAN)) vita_tx_control
     (.clk(clk), .reset(reset), .clear(clear_vita),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .vita_time(vita_time), .error(error), .ack(ack), .error_code(error_code),
      .sample_fifo_i(tx1_data), .sample_fifo_src_rdy_i(tx1_src_rdy), .sample_fifo_dst_rdy_o(tx1_dst_rdy),
      .sample(sample_tx), .run(run), .strobe(strobe_tx), .packet_consumed(packet_consumed),
      .debug(debug_vtc) );
   
   dsp_core_tx #(.BASE(BASE_DSP)) dsp_core_tx
     (.clk(clk),.rst(reset),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .sample(sample_tx), .run(run), .strobe(strobe_tx),
      .tx_i(tx_i),.tx_q(tx_q),
      .debug(debug_tx_dsp) );

   wire [35:0] 		flow_data, err_data_int;
   wire 		flow_src_rdy, flow_dst_rdy, err_src_rdy_int, err_dst_rdy_int;
   
   gen_context_pkt #(.PROT_ENG_FLAGS(PROT_ENG_FLAGS),.DSP_NUMBER(DSP_NUMBER)) gen_flow_pkt
     (.clk(clk), .reset(reset), .clear(clear_vita),
      .trigger(trigger & (DO_FLOW_CONTROL==1)), .sent(), 
      .streamid(streamid), .vita_time(vita_time), .message(32'd0),
      .seqnum(current_seqnum),
      .data_o(flow_data), .src_rdy_o(flow_src_rdy), .dst_rdy_i(flow_dst_rdy));
   trigger_context_pkt #(.BASE(BASE_CTRL)) trigger_context_pkt
     (.clk(clk), .reset(reset), .clear(clear_vita),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .packet_consumed(packet_consumed), .trigger(trigger));
   
   gen_context_pkt #(.PROT_ENG_FLAGS(PROT_ENG_FLAGS),.DSP_NUMBER(DSP_NUMBER)) gen_tx_err_pkt
     (.clk(clk), .reset(reset), .clear(clear_vita),
      .trigger((error|ack) & (REPORT_ERROR==1)), .sent(), 
      .streamid(streamid), .vita_time(vita_time), .message(message),
      .seqnum(current_seqnum),
      .data_o(err_data_int), .src_rdy_o(err_src_rdy_int), .dst_rdy_i(err_dst_rdy_int));
      
   assign debug = debug_vtc | debug_vtd;
   
   fifo36_mux #(.prio(1)) mux_err_and_flow  // Priority to err messages
     (.clk(clk), .reset(reset), .clear(0),  // Don't clear this or it could get clogged
      .data0_i(err_data_int), .src0_rdy_i(err_src_rdy_int), .dst0_rdy_o(err_dst_rdy_int),
      .data1_i(flow_data), .src1_rdy_i(flow_src_rdy), .dst1_rdy_o(flow_dst_rdy),
      .data_o(err_data_o), .src_rdy_o(err_src_rdy_o), .dst_rdy_i(err_dst_rdy_i));
   
endmodule // vita_tx_chain
