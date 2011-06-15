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


module decoder_3_8 
  (input [2:0] sel, 
   output reg [7:0] res);

   always @(sel or res)
     begin
        case (sel)
          3'b000 : res = 8'b00000001;
          3'b001 : res = 8'b00000010;
          3'b010 : res = 8'b00000100;
          3'b011 : res = 8'b00001000;
          3'b100 : res = 8'b00010000;
          3'b101 : res = 8'b00100000;
          3'b110 : res = 8'b01000000;
          default : res = 8'b10000000;
        endcase // case(sel)
     end // always @ (sel or res)

endmodule // decoder_3_8

