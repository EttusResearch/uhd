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


// Join vita packets longer than one GPIF frame

module packet_reframer
  (input clk, input reset, input clear,
   input [15:0] data_i,
   input src_rdy_i,
   output dst_rdy_o,
   output [18:0] data_o,
   output src_rdy_o,
   input dst_rdy_i,
   output reg state,
   output eof_out,
   output reg [15:0] length);

   //reg state;
   //reg [15:0] length;
   
   localparam RF_IDLE = 0;
   localparam RF_PKT = 1;
   
   always @(posedge clk)
     if(reset | clear)
       state <= RF_IDLE;
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
		if(eof_out) state <= RF_IDLE;
		length <= length - 1;
	     end
	 endcase // case (state)
   
   assign dst_rdy_o = dst_rdy_i; // this is a little pessimistic but ok
   assign src_rdy_o = src_rdy_i;
   
   wire occ_out = 0;
   assign eof_out = (state == RF_PKT) & (length == 2);
   wire sof_out = (state == RF_IDLE);
   assign data_o = {occ_out, eof_out, sof_out, data_i[15:0]};
   
      
endmodule // packet_reframer




