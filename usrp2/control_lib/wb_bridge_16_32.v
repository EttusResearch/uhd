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



module wb_bridge_16_32
  #(parameter AWIDTH=16)
    (input wb_clk, input wb_rst,
     input A_cyc_i, input A_stb_i, input A_we_i, input [3:0] A_sel_i,
     input [AWIDTH-1:0] A_adr_i, input [31:0] A_dat_i, output [31:0] A_dat_o, output A_ack_o,
     output B_cyc_o, output B_stb_o, output B_we_o, output [1:0] B_sel_o,
     output [AWIDTH-1:0] B_adr_o, output [15:0] B_dat_o, input [15:0] B_dat_i, input B_ack_i
     );

   reg [15:0] 		 holding;
   reg 			 phase;
   
   assign 		 B_adr_o = {A_adr_i[AWIDTH-1:2],phase,1'b0};
   assign 		 B_cyc_o = A_cyc_i;
   assign 		 B_stb_o = A_stb_i;
   assign 		 B_we_o = A_we_i;

   assign 		 B_dat_o = ~phase ? A_dat_i[15:0] : A_dat_i[31:16];
   assign 		 B_sel_o = ~phase ? A_sel_i[1:0] : A_sel_i[3:2];

   assign 		 A_dat_o = {B_dat_i,holding};
   assign 		 A_ack_o = phase & B_ack_i;

   always @(posedge wb_clk)
     if(wb_rst)
       phase <= 0;
     else if(B_ack_i)
       phase <= ~phase;

   always @(posedge wb_clk)
     if(~phase & B_ack_i)
       holding <= B_dat_i;
   
endmodule // wb_bridge_16_32
