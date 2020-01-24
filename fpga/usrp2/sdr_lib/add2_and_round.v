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


module add2_and_round
  #(parameter WIDTH=16)
    (input [WIDTH-1:0] in1,
     input [WIDTH-1:0] in2,
     output [WIDTH-1:0] sum);

   wire [WIDTH:0] 	sum_int = {in1[WIDTH-1],in1} + {in2[WIDTH-1],in2};
   assign 		sum = sum_int[WIDTH:1] + (sum_int[WIDTH] & sum_int[0]);
   
endmodule // add2_and_round
