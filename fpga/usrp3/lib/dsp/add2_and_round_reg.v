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


module add2_and_round_reg
  #(parameter WIDTH=16)
    (input clk,
     input [WIDTH-1:0] in1,
     input [WIDTH-1:0] in2,
     output reg [WIDTH-1:0] sum);

   wire [WIDTH-1:0] sum_int;
   
   add2_and_round #(.WIDTH(WIDTH)) add2_n_rnd (.in1(in1),.in2(in2),.sum(sum_int));

   always @(posedge clk)
     sum <= sum_int;
   
endmodule // add2_and_round_reg
