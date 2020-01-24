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



// Dual ported, Harvard architecture

module ram_harvard2
  #(parameter AWIDTH=15,
    parameter RAM_SIZE=32768)
   (input wb_clk_i, 
    input wb_rst_i,
    // Instruction fetch port.
    input [AWIDTH-1:0] if_adr,
    output reg [31:0] if_data,
    // Data access port.
    input [AWIDTH-1:0] dwb_adr_i,
    input [31:0] dwb_dat_i, 
    output reg [31:0] dwb_dat_o,
    input dwb_we_i,
    output dwb_ack_o,
    input dwb_stb_i,
    input [3:0] dwb_sel_i);
   
   reg 	  ack_d1;
   reg 	  stb_d1;
   
   assign dwb_ack_o = dwb_stb_i & (dwb_we_i | (stb_d1 & ~ack_d1));

   always @(posedge wb_clk_i) 
     if(wb_rst_i)
       ack_d1 <= 1'b0;
     else 
       ack_d1 <= dwb_ack_o;

   always @(posedge wb_clk_i)
     if(wb_rst_i)
       stb_d1 <= 0;
     else
       stb_d1 <= dwb_stb_i;

   reg [7:0] ram0 [0:(RAM_SIZE/4)-1];
   reg [7:0] ram1 [0:(RAM_SIZE/4)-1];
   reg [7:0] ram2 [0:(RAM_SIZE/4)-1];
   reg [7:0] ram3 [0:(RAM_SIZE/4)-1];
   
   // Port 1, Read only
   always @(posedge wb_clk_i)
     if_data[31:24] <= ram3[if_adr[AWIDTH-1:2]];
   always @(posedge wb_clk_i)
     if_data[23:16] <= ram2[if_adr[AWIDTH-1:2]];
   always @(posedge wb_clk_i)
     if_data[15:8] <= ram1[if_adr[AWIDTH-1:2]];
   always @(posedge wb_clk_i)
     if_data[7:0] <= ram0[if_adr[AWIDTH-1:2]];

   // Port 2, R/W
   always @(posedge wb_clk_i)
     if(dwb_stb_i) dwb_dat_o[31:24] <= ram3[dwb_adr_i[AWIDTH-1:2]];
   always @(posedge wb_clk_i)
     if(dwb_stb_i) dwb_dat_o[23:16] <= ram2[dwb_adr_i[AWIDTH-1:2]];
   always @(posedge wb_clk_i)
     if(dwb_stb_i) dwb_dat_o[15:8] <= ram1[dwb_adr_i[AWIDTH-1:2]];
   always @(posedge wb_clk_i)
     if(dwb_stb_i) dwb_dat_o[7:0] <= ram0[dwb_adr_i[AWIDTH-1:2]];
   
   always @(posedge wb_clk_i)
     if(dwb_we_i & dwb_stb_i & dwb_sel_i[3])
       ram3[dwb_adr_i[AWIDTH-1:2]] <= dwb_dat_i[31:24];
   always @(posedge wb_clk_i)
     if(dwb_we_i & dwb_stb_i & dwb_sel_i[2])
       ram2[dwb_adr_i[AWIDTH-1:2]] <= dwb_dat_i[23:16];
   always @(posedge wb_clk_i)
     if(dwb_we_i & dwb_stb_i & dwb_sel_i[1])
       ram1[dwb_adr_i[AWIDTH-1:2]] <= dwb_dat_i[15:8];
   always @(posedge wb_clk_i)
     if(dwb_we_i & dwb_stb_i & dwb_sel_i[0])
       ram0[dwb_adr_i[AWIDTH-1:2]] <= dwb_dat_i[7:0];
   
endmodule // ram_harvard
