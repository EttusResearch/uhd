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



// Note -- clocks must be synchronous (derived from the same source)
// Assumes alt_clk is running at a multiple of wb_clk

// Note -- assumes that the lower-16 bits will be requested first,
// and that the upper-16 bit request will come immediately after.

module wb_readback_mux_16LE
  (input wb_clk_i,
   input wb_rst_i,
   input wb_stb_i,
   input [15:0] wb_adr_i,
   output [15:0] wb_dat_o,
   output reg wb_ack_o,

   input [31:0] word00,
   input [31:0] word01,
   input [31:0] word02,
   input [31:0] word03,
   input [31:0] word04,
   input [31:0] word05,
   input [31:0] word06,
   input [31:0] word07,
   input [31:0] word08,
   input [31:0] word09,
   input [31:0] word10,
   input [31:0] word11,
   input [31:0] word12,
   input [31:0] word13,
   input [31:0] word14,
   input [31:0] word15
   );

   wire ack_next = wb_stb_i & ~wb_ack_o;

   always @(posedge wb_clk_i)
     if(wb_rst_i)
       wb_ack_o <= 0;
     else
       wb_ack_o <= ack_next;

   reg [31:0] data;
   assign wb_dat_o = data[15:0];

   always @(posedge wb_clk_i)
    if (wb_adr_i[1] & ack_next) begin //upper half
        data[15:0] <= data[31:16];
    end
    else if (~wb_adr_i[1] & ack_next) begin //lower half
     case(wb_adr_i[5:2])
       0 : data <= word00;
       1 : data <= word01;
       2 : data <= word02;
       3 : data <= word03;
       4 : data <= word04;
       5 : data <= word05;
       6 : data <= word06;
       7 : data <= word07;
       8 : data <= word08;
       9 : data <= word09;
       10: data <= word10;
       11: data <= word11;
       12: data <= word12;
       13: data <= word13;
       14: data <= word14;
       15: data <= word15;
     endcase // case(wb_adr_i[5:2])
    end

endmodule // wb_readback_mux


