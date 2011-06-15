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


// Grab settings off the wishbone bus, send them out to settings bus
// 16 bits little endian, but all registers need to be written 32 bits at a time.
// This means that you write the low 16 bits first and then the high 16 bits.
// The setting regs are strobed when the high 16 bits are written

module settings_bus_16LE
  #(parameter AWIDTH=16, RWIDTH=8)
    (input wb_clk, 
     input wb_rst, 
     input [AWIDTH-1:0] wb_adr_i,
     input [15:0] wb_dat_i,
     input wb_stb_i,
     input wb_we_i,
     output reg wb_ack_o,
     output strobe,
     output reg [7:0] addr,
     output reg [31:0] data);

   reg 		       stb_int;
   
   always @(posedge wb_clk)
     if(wb_rst)
       begin
	  stb_int <= 1'b0;
	  addr <= 8'd0;
	  data <= 32'd0;
       end
     else if(wb_we_i & wb_stb_i)
       begin
	  addr <= wb_adr_i[RWIDTH+1:2];  // Zero pad high bits
	  if(wb_adr_i[1])
	    begin
	       stb_int <= 1'b1;     // We now have both halves
	       data[31:16] <= wb_dat_i;
	    end
	  else
	    begin
	       stb_int <= 1'b0;     // Don't strobe, we need other half
	       data[15:0] <= wb_dat_i;
	    end
       end
     else
       stb_int <= 1'b0;

   always @(posedge wb_clk)
     if(wb_rst)
       wb_ack_o <= 0;
     else
       wb_ack_o <= wb_stb_i & ~wb_ack_o;

   assign strobe = stb_int & wb_ack_o;
          
endmodule // settings_bus_16LE
