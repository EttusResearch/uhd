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



module add_onescomp
  #(parameter WIDTH = 16)
   (input [WIDTH-1:0] A,
    input [WIDTH-1:0] B,
    output [WIDTH-1:0] SUM);

   wire [WIDTH:0] SUM_INT = {1'b0,A} + {1'b0,B};
   assign SUM  = SUM_INT[WIDTH-1:0] + {{WIDTH-1{1'b0}},SUM_INT[WIDTH]};
   
endmodule // add_onescomp
