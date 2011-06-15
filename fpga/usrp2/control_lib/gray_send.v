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




module gray_send
  #(parameter WIDTH = 8)
    (input clk_in, input [WIDTH-1:0] addr_in,
     input clk_out, output reg [WIDTH-1:0] addr_out);

   reg [WIDTH-1:0] gray_clkin, gray_clkout, gray_clkout_d1;
   wire [WIDTH-1:0] gray, bin;

   bin2gray #(.WIDTH(WIDTH)) b2g (.bin(addr_in), .gray(gray) );

   always @(posedge clk_in)
     gray_clkin <= gray;

   always @(posedge clk_out)
     gray_clkout <= gray_clkin;

   always @(posedge clk_out)
     gray_clkout_d1 <= gray_clkout;
   
   gray2bin #(.WIDTH(WIDTH)) g2b (.gray(gray_clkout_d1), .bin(bin) );

   // FIXME we may not need the next register, but it may help timing
   always @(posedge clk_out)
     addr_out <= bin;
   
endmodule // gray_send
