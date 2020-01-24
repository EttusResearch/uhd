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



module wb_ram_dist
  #(parameter AWIDTH=8)
    (input clk_i,
     input stb_i,
     input we_i,
     input [AWIDTH-1:0] adr_i,
     input [31:0] dat_i,
     input [3:0] sel_i,
     output [31:0] dat_o,
     output ack_o);

   reg [31:0] distram [0:1<<(AWIDTH-1)];

   always @(posedge clk_i)
     begin
	if(stb_i & we_i & sel_i[3])
	  distram[adr_i][31:24] <= dat_i[31:24];
	if(stb_i & we_i & sel_i[2])
	  distram[adr_i][24:16] <= dat_i[24:16];
	if(stb_i & we_i & sel_i[1])
	  distram[adr_i][15:8] <= dat_i[15:8];
	if(stb_i & we_i & sel_i[0])
	  distram[adr_i][7:0] <= dat_i[7:0];
     end // always @ (posedge clk_i)

   assign dat_o = distram[adr_i];
   assign ack_o = stb_i;

endmodule // wb_ram_dist

    
