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


// Mux packets from multiple FIFO interfaces onto a single one.
//  Can alternate or give priority to one port (port 0)
//  In prio mode, port 1 will never get access if port 0 is always busy

module fifo36_mux
  #(parameter prio = 0)
   (input clk, input reset, input clear,
    input [35:0] data0_i, input src0_rdy_i, output dst0_rdy_o,
    input [35:0] data1_i, input src1_rdy_i, output dst1_rdy_o,
    output [35:0] data_o, output src_rdy_o, input dst_rdy_i);

   wire [35:0] 	  data0_int, data1_int;
   wire 	  src0_rdy_int, dst0_rdy_int, src1_rdy_int, dst1_rdy_int;
   
   fifo_short #(.WIDTH(36)) mux_fifo_in0
     (.clk(clk), .reset(reset), .clear(clear),
      .datain(data0_i), .src_rdy_i(src0_rdy_i), .dst_rdy_o(dst0_rdy_o),
      .dataout(data0_int), .src_rdy_o(src0_rdy_int), .dst_rdy_i(dst0_rdy_int));

   fifo_short #(.WIDTH(36)) mux_fifo_in1
     (.clk(clk), .reset(reset), .clear(clear),
      .datain(data1_i), .src_rdy_i(src1_rdy_i), .dst_rdy_o(dst1_rdy_o),
      .dataout(data1_int), .src_rdy_o(src1_rdy_int), .dst_rdy_i(dst1_rdy_int));

   localparam MUX_IDLE0 = 0;
   localparam MUX_DATA0 = 1;
   localparam MUX_IDLE1 = 2;
   localparam MUX_DATA1 = 3;
   
   reg [1:0] 	  state;

   wire 	  eof0 = data0_int[33];
   wire 	  eof1 = data1_int[33];
   
   wire [35:0] 	  data_int;
   wire 	  src_rdy_int, dst_rdy_int;
   
   always @(posedge clk)
     if(reset | clear)
       state <= MUX_IDLE0;
     else
       case(state)
	 MUX_IDLE0 :
	   if(src0_rdy_int)
	     state <= MUX_DATA0;
	   else if(src1_rdy_int)
	     state <= MUX_DATA1;

	 MUX_DATA0 :
	   if(src0_rdy_int & dst_rdy_int & eof0)
	     state <= prio ? MUX_IDLE0 : MUX_IDLE1;

	 MUX_IDLE1 :
	   if(src1_rdy_int)
	     state <= MUX_DATA1;
	   else if(src0_rdy_int)
	     state <= MUX_DATA0;
	   
	 MUX_DATA1 :
	   if(src1_rdy_int & dst_rdy_int & eof1)
	     state <= MUX_IDLE0;
	 
	 default :
	   state <= MUX_IDLE0;
       endcase // case (state)

   assign dst0_rdy_int = (state==MUX_DATA0) ? dst_rdy_int : 0;
   assign dst1_rdy_int = (state==MUX_DATA1) ? dst_rdy_int : 0;
   assign src_rdy_int = (state==MUX_DATA0) ? src0_rdy_int : (state==MUX_DATA1) ? src1_rdy_int : 0;
   assign data_int = (state==MUX_DATA0) ? data0_int : data1_int;
   
   fifo_short #(.WIDTH(36)) mux_fifo
     (.clk(clk), .reset(reset), .clear(clear),
      .datain(data_int), .src_rdy_i(src_rdy_int), .dst_rdy_o(dst_rdy_int),
      .dataout(data_o), .src_rdy_o(src_rdy_o), .dst_rdy_i(dst_rdy_i));
endmodule // fifo36_demux
