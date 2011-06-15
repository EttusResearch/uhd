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


// Pads a packet out to the minimum length
//  Packets already longer than min length are unchanged


module fifo19_pad
  #(parameter LENGTH=16,
    parameter PAD_VALUE=0)
   (input clk, input reset, input clear,
    input [18:0] data_i,
    input src_rdy_i,
    output dst_rdy_o,
    output [18:0] data_o,
    output src_rdy_o,
    input dst_rdy_i);

   reg [15:0] count;
   reg [1:0]  pad_state;
   localparam PAD_IDLE = 0;
   localparam PAD_TOOSHORT = 1;
   localparam PAD_LONGENOUGH = 2;
   localparam PAD_PADDING = 3;

   always @(posedge clk)
     if(reset | clear)
       pad_state <= PAD_IDLE;
     else
       case(pad_state)
	 PAD_IDLE :
	   begin
	      count <= 1;
	      pad_state <= PAD_TOOSHORT;
	   end
	 PAD_TOOSHORT :
	   if(src_rdy_i & dst_rdy_i)
	     begin
		count <= count + 1;
		if(data_i[17])
		  pad_state <= PAD_PADDING;
		else if(count == (LENGTH-1))
		  pad_state <= PAD_LONGENOUGH;
	     end
	 PAD_PADDING :
	   if(dst_rdy_i)
	     begin
		count <= count + 1;
		if(count == LENGTH)
		  pad_state <= PAD_IDLE;
	     end
	 PAD_LONGENOUGH :
	   if(src_rdy_i & dst_rdy_i & data_i[17])
	     pad_state <= PAD_IDLE;
       endcase // case (pad_state)
   
   wire passthru = (pad_state == PAD_TOOSHORT) | (pad_state == PAD_LONGENOUGH);
   
   assign dst_rdy_o = passthru ? dst_rdy_i : 1'b0;
   assign src_rdy_o = passthru ? src_rdy_i : (pad_state == PAD_PADDING);
   
   assign data_o[15:0] = (pad_state == PAD_PADDING) ? PAD_VALUE : data_i[15:0];
   assign data_o[16] = (count == 1);
   assign data_o[17] = (pad_state == PAD_LONGENOUGH) ? data_i[17] : (count == LENGTH);
   assign data_o[18] = (pad_state == PAD_LONGENOUGH) ? data_i[18] : 1'b0;
   
         
endmodule // fifo19_pad
