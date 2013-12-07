//
// Copyright 2013 Ettus Research LLC
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


module por_gen
  (input clk,
   output reset_out);

   reg 		por_rst;
   reg [7:0] 	por_counter = 8'h0;

   always @(posedge clk)
     if (por_counter != 8'h55)
       begin
          por_counter <= por_counter + 8'h1;
          por_rst <= 1'b1;
       end
     else
       por_rst <= 1'b0;

   assign reset_out = por_rst;

endmodule // por_gen
