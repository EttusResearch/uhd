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


// NOTE:  Will not work with single-line frames

module ethrx_realign
   (input clk, input reset, input clear,
    input [35:0] datain, input src_rdy_i, output dst_rdy_o,
    output [35:0] dataout, output src_rdy_o, input dst_rdy_i);

   reg [1:0] 	  state;
   reg [15:0] 	  held;
   reg [1:0] 	  held_occ;
	  
   wire 	  xfer_in = src_rdy_i & dst_rdy_o;
   wire 	  xfer_out = src_rdy_o & dst_rdy_i;

   wire 	  sof_in = datain[32];
   wire 	  eof_in = datain[33];
   wire [1:0] 	  occ_in = datain[35:34];
   wire 	  sof_out, eof_out;
   wire [1:0] 	  occ_out;
      
   always @(posedge clk)
     if(reset | clear)
       begin
	  held <= 0;
	  held_occ <= 0;
       end
     else if(xfer_in)
       begin
	  held <= datain[15:0];
	  held_occ <= datain[35:34];
       end
   
   localparam RE_IDLE = 0;
   localparam RE_HELD = 1;
   localparam RE_DONE = 2;

   always @(posedge clk)
     if(reset | clear)
       state <= RE_IDLE;
     else
       case(state)
	 RE_IDLE :
	   if(src_rdy_i & dst_rdy_i)
	      if(eof_in)
		state <= RE_DONE;
	      else
		state <= RE_HELD;

	 RE_HELD :
	   if(src_rdy_i & dst_rdy_i & eof_in)
	     if((occ_in==0)|(occ_in==3))
	       state <= RE_DONE;
	     else
	       state <= RE_IDLE;

	 RE_DONE :
	   if(dst_rdy_i)
	     state <= RE_IDLE;
	 
       endcase // case (state)
   
   
   assign sof_out = (state == RE_IDLE);
   assign eof_out = (state == RE_DONE) | (occ_in == 1) | (occ_in == 2);
   assign occ_out = (state == RE_DONE) ? ((held_occ == 3) ? 1 : 2) :
		    (occ_in == 1) ? 3 : 0;
   		    
   assign dataout = {occ_out,eof_out,sof_out,held,datain[31:16]};
   assign src_rdy_o = (state == RE_DONE) | src_rdy_i;
   assign dst_rdy_o = dst_rdy_i & ((state == RE_IDLE)|(state == RE_HELD));
endmodule // ethrx_realign
