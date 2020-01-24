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



module fifo_pacer
  (input clk,
   input reset,
   input [7:0] rate,
   input enable,
   input src1_rdy_i, output dst1_rdy_o,
   output src2_rdy_o, input dst2_rdy_i,
   output underrun, overrun);

   wire   strobe;
   
   cic_strober strober (.clock(clk), .reset(reset), .enable(enable),
			.rate(rate), .strobe_fast(1), .strobe_slow(strobe));

   wire   all_ready = src1_rdy_i & dst2_rdy_i;
   assign dst1_rdy_o = all_ready & strobe;
   assign src2_rdy_o = dst1_rdy_o;

   assign underrun = strobe & ~src1_rdy_i;
   assign overrun = strobe & ~dst2_rdy_i;
   
endmodule // fifo_pacer
