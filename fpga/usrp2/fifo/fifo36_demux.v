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


// Demux packets from a fifo based on the contents of the first line
//  If first line matches the parameter and mask, send to data1, otherwise send to data0

module fifo36_demux
  #(parameter match_data = 0,
    parameter match_mask = 0)
   (input clk, input reset, input clear,
    input [35:0] data_i, input src_rdy_i, output dst_rdy_o,
    output [35:0] data0_o, output src0_rdy_o, input dst0_rdy_i,
    output [35:0] data1_o, output src1_rdy_o, input dst1_rdy_i);

   localparam DMX_IDLE = 0;
   localparam DMX_DATA0 = 1;
   localparam DMX_DATA1 = 2;
   
   reg [1:0] 	  state;

   wire 	  match = |( (data_i ^ match_data) & match_mask );
   wire 	  eof = data_i[33];
   
   always @(posedge clk)
     if(reset | clear)
       state <= DMX_IDLE;
     else
       case(state)
	 DMX_IDLE :
	   if(src_rdy_i)
	     if(match)
	       state <= DMX_DATA1;
	     else
	       state <= DMX_DATA0;
	 DMX_DATA0 :
	   if(src_rdy_i & dst0_rdy_i & eof)
	     state <= DMX_IDLE;
	 DMX_DATA1 :
	   if(src_rdy_i & dst1_rdy_i & eof)
	     state <= DMX_IDLE;
	 default :
	   state <= DMX_IDLE;
       endcase // case (state)

   assign dst_rdy_o = (state==DMX_IDLE) ? 0 : (state==DMX_DATA0) ? dst0_rdy_i : dst1_rdy_i;
   assign src0_rdy_o = (state==DMX_DATA0) ? src_rdy_i : 0;
   assign src1_rdy_o = (state==DMX_DATA1) ? src_rdy_i : 0;

   assign data0_o = data_i;
   assign data1_o = data_i;
   
endmodule // fifo36_demux
