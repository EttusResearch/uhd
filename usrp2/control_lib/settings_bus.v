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


// Grab settings off the wishbone bus, send them out to our simpler bus on the fast clock

module settings_bus
  #(parameter AWIDTH=16, parameter DWIDTH=32)
    (input wb_clk, 
     input wb_rst, 
     input [AWIDTH-1:0] wb_adr_i,
     input [DWIDTH-1:0] wb_dat_i,
     input wb_stb_i,
     input wb_we_i,
     output reg wb_ack_o,
     output reg strobe,
     output reg [7:0] addr,
     output reg [31:0] data);

   reg 	    stb_int, stb_int_d1;
   
   always @(posedge wb_clk)
     if(wb_rst)
       begin
	  strobe <= 1'b0;
	  addr <= 8'd0;
	  data <= 32'd0;
       end
     else if(wb_we_i & wb_stb_i & ~wb_ack_o)
       begin
	  strobe <= 1'b1;
	  addr <= wb_adr_i[9:2];
	  data <= wb_dat_i;
       end
     else
       strobe <= 1'b0;

   always @(posedge wb_clk)
     if(wb_rst)
       wb_ack_o <= 0;
     else
       wb_ack_o <= wb_stb_i & ~wb_ack_o;

endmodule // settings_bus
