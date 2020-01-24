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



module timer
  (input wb_clk_i, input rst_i,
   input cyc_i, input stb_i, input [2:0] adr_i,
   input we_i, input [31:0] dat_i, output [31:0] dat_o, output ack_o,
   input sys_clk_i, input [31:0] master_time_i,
   output int_o );

   reg [31:0] time_wb;
   always @(posedge wb_clk_i)
     time_wb <= master_time_i;

   assign     ack_o = stb_i;

   reg [31:0] int_time;
   reg 	      int_reg;
   
   always @(posedge sys_clk_i)
     if(rst_i)
       begin
	  int_time <= 0;
	  int_reg <= 0;
       end
     else if(|int_time && (master_time_i == int_time))
       begin
	  int_time <= 0;
	  int_reg <= 1;
       end
     else if(stb_i & we_i)
       begin
	  int_time <= dat_i;
	  int_reg <= 0;
       end

   assign dat_o = time_wb;
   assign int_o = int_reg;
   
endmodule // timer

