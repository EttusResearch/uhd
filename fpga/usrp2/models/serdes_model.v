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


module serdes_model
  (input ser_tx_clk,
   input ser_tkmsb,
   input ser_tklsb,
   input [15:0] ser_t,
   
   output ser_rx_clk,
   output ser_rkmsb,
   output ser_rklsb,
   output [15:0] ser_r,
   
   input even,
   input error);
   
   wire [15:0] ser_r_odd;
   wire  ser_rklsb_odd, ser_rkmsb_odd;   
   
   reg [7:0] hold_dat;
   reg 	     hold_k;
   
   always @(posedge ser_tx_clk) hold_k <= ser_tklsb;
   always @(posedge ser_tx_clk) hold_dat <= ser_t[15:8];
   assign    ser_rklsb_odd = hold_k;
   assign    ser_rkmsb_odd = ser_tklsb;
   assign    ser_r_odd = {ser_t[7:0], hold_dat};
   
   // Set outputs
   assign    ser_rx_clk = ser_tx_clk;
   assign    ser_rkmsb = even ? ser_tkmsb : ser_rkmsb_odd;
   assign    ser_rklsb = even ? ser_tklsb : ser_rklsb_odd;
   assign    ser_r = error ^ (even ? ser_t : ser_r_odd);
   
endmodule // serdes_model
