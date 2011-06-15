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


module vita_rx_chain
  #(parameter BASE=0,
    parameter UNIT=0,
    parameter FIFOSIZE=10,
    parameter PROT_ENG_FLAGS=1)
   (input clk, input reset, input clear,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    input [63:0] vita_time, output overrun,
    input [31:0] sample, output run, input strobe,
    output [35:0] rx_data_o, output rx_src_rdy_o, input rx_dst_rdy_i,
    output [31:0] debug );
       
   wire [100:0] sample_data;
   wire 	sample_dst_rdy, sample_src_rdy;
   wire [31:0] 	vrc_debug, vrf_debug;

   wire [35:0] 	rx_data_int;
   wire 	rx_src_rdy_int, rx_dst_rdy_int;
   
   vita_rx_control #(.BASE(BASE), .WIDTH(32)) vita_rx_control
     (.clk(clk), .reset(reset), .clear(clear),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .vita_time(vita_time), .overrun(overrun),
      .sample(sample), .run(run), .strobe(strobe),
      .sample_fifo_o(sample_data), .sample_fifo_dst_rdy_i(sample_dst_rdy), .sample_fifo_src_rdy_o(sample_src_rdy),
      .debug_rx(vrc_debug));
   
   vita_rx_framer #(.BASE(BASE), .MAXCHAN(1)) vita_rx_framer
     (.clk(clk), .reset(reset), .clear(clear),
      .set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
      .sample_fifo_i(sample_data), .sample_fifo_dst_rdy_o(sample_dst_rdy), .sample_fifo_src_rdy_i(sample_src_rdy),
      .data_o(rx_data_int), .src_rdy_o(rx_src_rdy_int), .dst_rdy_i(rx_dst_rdy_int),
      .debug_rx(vrf_debug) );

   dsp_framer36 #(.BUF_SIZE(FIFOSIZE), 
		  .PORT_SEL(UNIT), 
		  .PROT_ENG_FLAGS(PROT_ENG_FLAGS)) dsp0_framer36
     (.clk(clk), .reset(reset), .clear(clear),
      .data_i(rx_data_int), .src_rdy_i(rx_src_rdy_int), .dst_rdy_o(rx_dst_rdy_int),
      .data_o(rx_data_o), .src_rdy_o(rx_src_rdy_o), .dst_rdy_i(rx_dst_rdy_i) );

   assign debug = vrc_debug; //  | vrf_debug;
   
endmodule // vita_rx_chain
