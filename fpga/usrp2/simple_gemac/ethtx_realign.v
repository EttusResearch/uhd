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


////////////////////////////////////////////////////////////////////////
// Ethernet TX - Realign
//
// - removes a 2-byte pad from the front a fifo36 stream
// - occupancy is preserved
//

module ethtx_realign
   (input clk, input reset, input clear,
    input [35:0] datain, input src_rdy_i, output dst_rdy_o,
    output [35:0] dataout, output src_rdy_o, input dst_rdy_i);

   reg [1:0] 	  state;
   reg [15:0] 	  held;
   reg [1:0] 	  held_occ;
   reg 		  held_sof;
   
   wire 	  xfer_in = src_rdy_i & dst_rdy_o;
   wire 	  xfer_out = src_rdy_o & dst_rdy_i;

   wire 	  sof_in = datain[32];
   wire 	  eof_in = datain[33];
   wire [1:0] 	  occ_in = datain[35:34];
   wire 	  occ_low = occ_in[1] ^ occ_in[0]; //occ is 1 or 2

   always @(posedge clk)
     if(reset | clear)
       begin
	  held <= 0;
	  held_occ <= 0;
	  held_sof <= 0;
       end
     else if(xfer_in)
       begin
	  held <= datain[15:0];
	  held_occ <= occ_in;
	  held_sof <= sof_in;
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
	   if(xfer_in & eof_in)
	     state <= RE_DONE;
	   else if(xfer_in & sof_in)
	     state <= RE_HELD;

	 RE_HELD :
	   if(xfer_in & xfer_out & eof_in)
	     if(occ_low)
	       state <= RE_IDLE;
	     else
	       state <= RE_DONE;

	 RE_DONE :
	   if(xfer_out)
	     state <= RE_IDLE;
	 
       endcase // case (state)

   wire sof_out = held_sof;
   wire eof_out = (state == RE_HELD)? (eof_in & occ_low) : (state == RE_DONE);
   wire [1:0] occ_out = ((state == RE_DONE)? held_occ : occ_in) ^ 2'b10; //(occ + 2)%4

   assign dataout = {occ_out,eof_out,sof_out,held,datain[31:16]};
   assign src_rdy_o = (state == RE_HELD)? src_rdy_i : (state == RE_DONE);
   assign dst_rdy_o = (state == RE_HELD)? dst_rdy_i : (state == RE_IDLE);

endmodule // ethtx_realign
