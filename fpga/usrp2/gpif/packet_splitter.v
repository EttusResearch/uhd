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


// Split vita packets longer than one GPIF frame, add padding on short frames

module packet_splitter
  #(parameter FRAME_LEN=256)
   (input clk, input reset, input clear,
    input  [7:0] frames_per_packet,
    input [18:0] data_i,
    input src_rdy_i,
    output dst_rdy_o,
    output [18:0] data_o,
    output src_rdy_o,
    input dst_rdy_i,
    output [31:0] debug0,
    output [31:0] debug1);
   
   reg [1:0] state;
   reg [15:0] length;
   reg [15:0] frame_len;
   reg [7:0]  frame_count;
   
   localparam PS_IDLE = 0;
   localparam PS_FRAME = 1;
   localparam PS_NEW_FRAME = 2;
   localparam PS_PAD = 3;

   wire       eof_i = data_i[17];
   
   always @(posedge clk)
     if(reset | clear)
       begin
	  state <= PS_IDLE;
	  frame_count <= 0;
       end
     else
       case(state)
	 PS_IDLE :
	   if(src_rdy_i & dst_rdy_i)
	     begin
		length <= { data_i[14:0],1'b0};
		frame_len <= FRAME_LEN;
		state <= PS_FRAME;
		frame_count <= 1;
	     end
	 PS_FRAME :
	   if(src_rdy_i & dst_rdy_i)
	     if((frame_len == 2) & ((length == 2) | eof_i))
	       state <= PS_IDLE;
	     else if(frame_len == 2)
	       begin
		  length <= length - 1;
		  state <= PS_NEW_FRAME;
		  frame_count <= frame_count + 1;
	       end
	     else if((length == 2)|eof_i)
	       begin
		  frame_len <= frame_len - 1;
		  state <= PS_PAD;
	       end
	     else
	       begin
		  frame_len <= frame_len - 1;
		  length <= length - 1;
	       end
	 PS_NEW_FRAME :
	   if(src_rdy_i & dst_rdy_i)
	     begin
		frame_len <= FRAME_LEN;
		if((length == 2)|eof_i)
		  state <= PS_PAD;
		else
		  begin
		     state <= PS_FRAME;
		     length <= length - 1;
		  end // else: !if((length == 2)|eof_i)
	     end // if (src_rdy_i & dst_rdy_i)
	 
	 PS_PAD :
	   if(dst_rdy_i)
	     if(frame_len == 2)
	       state <= PS_IDLE;
	     else
	       frame_len <= frame_len - 1;

       endcase // case (state)

   wire next_state_is_idle = dst_rdy_i & (frame_len==2) & 
	( (state==PS_PAD) | ( (state==PS_FRAME) & src_rdy_i & ((length==2)|eof_i) ) );
	  
	 
	
   
   assign dst_rdy_o = dst_rdy_i & (state != PS_PAD);
   assign src_rdy_o = src_rdy_i | (state == PS_PAD);
   
   wire eof_out = (frame_len == 2) & (state != PS_IDLE) & (state != PS_NEW_FRAME);
   wire sof_out = (state == PS_IDLE) | (state == PS_NEW_FRAME);
   wire occ_out = eof_out & next_state_is_idle & (frames_per_packet != frame_count);
   
   wire [15:0] data_out = data_i[15:0];
   assign data_o = {occ_out, eof_out, sof_out, data_out};
   
   assign debug0 = { 8'd0, dst_rdy_o, src_rdy_o, next_state_is_idle, eof_out, sof_out, occ_out, state[1:0], frame_count[7:0], frames_per_packet[7:0] };
   assign debug1 = { length[15:0], frame_len[15:0] };
   
endmodule // packet_splitter
