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



// Dual ported, Harvard architecture, cached ram

module ram_harvard
  #(parameter AWIDTH=15,
    parameter RAM_SIZE=16384,
    parameter ICWIDTH=6,
    parameter DCWIDTH=6)
   
    (input wb_clk_i, 
     input wb_rst_i,
     // Firmware download port.
     input [AWIDTH-1:0] ram_loader_adr_i,
     input [31:0] ram_loader_dat_i,
     input [3:0] ram_loader_sel_i,
     input ram_loader_stb_i,
     input ram_loader_we_i,
     input ram_loader_done_i,    
     // Instruction fetch port.
     input [AWIDTH-1:0] if_adr,
     output [31:0] if_data,
     // Data access port.
     input [AWIDTH-1:0] dwb_adr_i,
     input [31:0] dwb_dat_i, 
     output [31:0] dwb_dat_o,
     input dwb_we_i,
     output dwb_ack_o,
     input dwb_stb_i,
     input [3:0] dwb_sel_i );

   reg 	   ack_d1;
   reg 	   stb_d1;
   
   dpram32 #(.AWIDTH(AWIDTH),.RAM_SIZE(RAM_SIZE)) 
   sys_ram
     (.clk(wb_clk_i),
      .adr1_i(ram_loader_done_i ? if_adr : ram_loader_adr_i),
      .dat1_i(ram_loader_dat_i),
      .dat1_o(if_data),
      .we1_i(ram_loader_done_i ? 1'b0 : ram_loader_we_i),
      .en1_i(ram_loader_done_i ? 1'b1 : ram_loader_stb_i),
      //.sel1_i(ram_loader_done_i ? 4'hF : ram_loader_sel_i),
      .sel1_i(ram_loader_sel_i), // Sel is only for writes anyway
      .adr2_i(dwb_adr_i),
      .dat2_i(dwb_dat_i),
      .dat2_o(dwb_dat_o),
      .we2_i(dwb_we_i),
      .en2_i(dwb_stb_i),
      .sel2_i(dwb_sel_i) 
      );

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

endmodule // ram_harvard
