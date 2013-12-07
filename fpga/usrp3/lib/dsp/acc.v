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


module acc
  #(parameter IWIDTH=16, OWIDTH=30)
    (input clk,
     input clear,
     input acc,
     input [IWIDTH-1:0] in,
     output reg [OWIDTH-1:0] out);

   wire [OWIDTH-1:0] in_signext;
   sign_extend #(.bits_in(IWIDTH),.bits_out(OWIDTH)) 
     acc_signext (.in(in),.out(in_signext));
   
   //  CLEAR & ~ACC  -->  clears the accumulator
   //  CLEAR & ACC -->    loads the accumulator
   //  ~CLEAR & ACC -->   accumulates
   //  ~CLEAR & ~ACC -->  hold
   
   wire [OWIDTH-1:0] addend1 = clear ? 0 : out;
   wire [OWIDTH-1:0] addend2 = ~acc ? 0 : in_signext;
   wire [OWIDTH-1:0] sum_int = addend1 + addend2;

   always @(posedge clk)
     out <= sum_int;
   
endmodule // acc


