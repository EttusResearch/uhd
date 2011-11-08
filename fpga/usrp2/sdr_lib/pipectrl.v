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

// Control DSP pipeline with 1 cycle per stage.  Minimum 2 stages or this won't work
module pipectrl
  #(parameter STAGES = 2, 
    parameter TAGWIDTH = 1)
   (input clk,
    input reset,
    input src_rdy_i,
    output dst_rdy_o,
    output src_rdy_o,
    input dst_rdy_i,
    output [STAGES-1:0] strobes,
    output [STAGES-1:0] valids,
    input [TAGWIDTH-1:0] tag_i,
    output [TAGWIDTH-1:0] tag_o);

   wire 		new_input = src_rdy_i & dst_rdy_o;
   wire 		new_output = src_rdy_o & dst_rdy_i;
   wire [TAGWIDTH-1:0] 	tags [STAGES-1:0];

   assign dst_rdy_o = ~valids[0] | strobes[1];

   pipestage #(.TAGWIDTH(TAGWIDTH)) head
     (.clk(clk),.reset(reset), .stb_in(strobes[0]), .stb_out(strobes[1]),.valid(valids[0]),
      .tag_in(tag_i), .tag_out(tags[0]));
   assign strobes[0] = src_rdy_i & (~valids[0] | strobes[1]);

   genvar 		i;
   generate
      for(i = 1; i < STAGES - 1; i = i + 1)
	begin : gen_stages
	   pipestage #(.TAGWIDTH(TAGWIDTH)) pipestage
	     (.clk(clk),.reset(reset), .stb_in(strobes[i]),.stb_out(strobes[i+1]),.valid(valids[i]),
	      .tag_in(tags[i-1]),.tag_out(tags[i]));
	   assign strobes[i] = valids[i-1] & (~valids[i] | strobes[i+1]);
	end
   endgenerate

   pipestage #(.TAGWIDTH(TAGWIDTH)) tail
     (.clk(clk),.reset(reset), .stb_in(strobes[STAGES-1]), .stb_out(dst_rdy_i),.valid(valids[STAGES-1]),
      .tag_in(tags[STAGES-2]), .tag_out(tags[STAGES-1]));
   assign strobes[STAGES-1] = valids[STAGES-2] & (~valids[STAGES-1] | new_output);
   
   assign src_rdy_o = valids[STAGES-1];

   assign tag_o = tags[STAGES-1];
   
endmodule // pipectrl


