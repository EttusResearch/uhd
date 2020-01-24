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


module priority_enc
  (input [31:0] in,
   output reg [31:0] out);

   always @*
     casex(in)
       32'b1xxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx : out <= 31;
       32'b01xx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx : out <= 30;
       32'b001x_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx : out <= 29;
       32'b0001_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx : out <= 28;
       32'b0000_1xxx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx : out <= 27;
       32'b0000_01xx_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx : out <= 26;
       32'b0000_001x_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx : out <= 25;
       32'b0000_0001_xxxx_xxxx_xxxx_xxxx_xxxx_xxxx : out <= 24;
       32'b0000_0000_1xxx_xxxx_xxxx_xxxx_xxxx_xxxx : out <= 23;
       32'b0000_0000_01xx_xxxx_xxxx_xxxx_xxxx_xxxx : out <= 22;
       32'b0000_0000_001x_xxxx_xxxx_xxxx_xxxx_xxxx : out <= 21;
       32'b0000_0000_0001_xxxx_xxxx_xxxx_xxxx_xxxx : out <= 20;
       32'b0000_0000_0000_1xxx_xxxx_xxxx_xxxx_xxxx : out <= 19;
       32'b0000_0000_0000_01xx_xxxx_xxxx_xxxx_xxxx : out <= 18;
       32'b0000_0000_0000_001x_xxxx_xxxx_xxxx_xxxx : out <= 17;
       32'b0000_0000_0000_0001_xxxx_xxxx_xxxx_xxxx : out <= 16;
       32'b0000_0000_0000_0000_1xxx_xxxx_xxxx_xxxx : out <= 15;
       32'b0000_0000_0000_0000_01xx_xxxx_xxxx_xxxx : out <= 14;
       32'b0000_0000_0000_0000_001x_xxxx_xxxx_xxxx : out <= 13;
       32'b0000_0000_0000_0000_0001_xxxx_xxxx_xxxx : out <= 12;
       32'b0000_0000_0000_0000_0000_1xxx_xxxx_xxxx : out <= 11;
       32'b0000_0000_0000_0000_0000_01xx_xxxx_xxxx : out <= 10;
       32'b0000_0000_0000_0000_0000_001x_xxxx_xxxx : out <=  9;
       32'b0000_0000_0000_0000_0000_0001_xxxx_xxxx : out <=  8;
       32'b0000_0000_0000_0000_0000_0000_1xxx_xxxx : out <=  7;
       32'b0000_0000_0000_0000_0000_0000_01xx_xxxx : out <=  6;
       32'b0000_0000_0000_0000_0000_0000_001x_xxxx : out <=  5;
       32'b0000_0000_0000_0000_0000_0000_0001_xxxx : out <=  4;
       32'b0000_0000_0000_0000_0000_0000_0000_1xxx : out <=  3;
       32'b0000_0000_0000_0000_0000_0000_0000_01xx : out <=  2;
       32'b0000_0000_0000_0000_0000_0000_0000_001x : out <=  1;
       32'b0000_0000_0000_0000_0000_0000_0000_0001 : out <=  0;
       32'b0000_0000_0000_0000_0000_0000_0000_0000 : out <= 32'hFFFF_FFFF;
       default : out <= 32'hFFFF_FFFF;
     endcase // casex (in)
   
endmodule // priority_enc
