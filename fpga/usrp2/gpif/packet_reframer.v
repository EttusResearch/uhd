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


// Join vita packets longer than one GPIF frame, drop padding on short frames

module packet_reframer
  (input clk, input reset, input clear,
   input [18:0] data_i,
   input src_rdy_i,
   output dst_rdy_o,
   output [18:0] data_o,
   output src_rdy_o,
   input dst_rdy_i);

   reg [1:0] state;
   reg [15:0] length;
   
   localparam RF_IDLE = 0;
   localparam RF_PKT = 1;
   localparam RF_DUMP = 2;
   
   always @(posedge clk)
     if(reset | clear)
       state <= 0;
     else
       if(src_rdy_i & dst_rdy_i)
	 case(state)
	   RF_IDLE :
	     begin
		length <= {data_i[14:0],1'b0};
		state <= RF_PKT;
	     end
	   RF_PKT :
	     begin
		if(length == 2)
		  if(data_i[17])
		    state <= RF_IDLE;
		  else
		    state <= RF_DUMP;
		else
		  length <= length - 1;
	     end
	   RF_DUMP :
	     if(data_i[17])
	       state <= RF_IDLE;
	   default :
	     state<= RF_IDLE;
	 endcase // case (state)
   
   assign dst_rdy_o = dst_rdy_i; // this is a little pessimistic but ok
   assign src_rdy_o = src_rdy_i & (state != RF_DUMP);
   
   wire occ_out = 0;
   wire eof_out = (state == RF_PKT) & (length == 2);
   wire sof_out = (state == RF_IDLE);
   wire [15:0] data_out = data_i[15:0];
   assign data_o = {occ_out, eof_out, sof_out, data_out};
   
      
endmodule // packet_reframer




