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



module gpmc_wb
  (input EM_CLK, input [15:0] EM_D_in, output [15:0] EM_D_out, input [10:1] EM_A, input [1:0] EM_NBE,
   input EM_NCS, input EM_NWE, input EM_NOE,

   input wb_clk, input wb_rst,
   output reg [10:0] wb_adr_o, output reg [15:0] wb_dat_mosi, input [15:0] wb_dat_miso,
   output reg [1:0] wb_sel_o, output wb_cyc_o, output reg wb_stb_o, output reg wb_we_o, input wb_ack_i);
   
   // ////////////////////////////////////////////
   // Control Path, Wishbone bus bridge (wb master)
   reg [1:0] 	cs_del, we_del, oe_del;

   // Synchronize the async control signals
   always @(posedge wb_clk)
     begin
	cs_del <= { cs_del[0], EM_NCS };
	we_del <= { we_del[0], EM_NWE };
	oe_del <= { oe_del[0], EM_NOE };
     end

   always @(posedge wb_clk)
     if(cs_del == 2'b10)  // Falling Edge
       wb_adr_o <= { EM_A, 1'b0 };

   always @(posedge wb_clk)
     if(we_del == 2'b10)  // Falling Edge
       begin
	  wb_dat_mosi <= EM_D_in;
	  wb_sel_o <= ~EM_NBE;
       end

   reg [15:0] EM_D_hold;
   
   always @(posedge wb_clk)
     if(wb_ack_i)
       EM_D_hold <= wb_dat_miso;

   assign EM_D_out = wb_ack_i ? wb_dat_miso : EM_D_hold;
   
   assign wb_cyc_o = wb_stb_o;

   always @(posedge wb_clk)
     if(~cs_del[0] & (we_del == 2'b10) )
       wb_we_o <= 1;
     else if(wb_ack_i)  // Turn off we when done.  Could also use we_del[0], others...
       wb_we_o <= 0;

   // FIXME should this look at cs_del[1]?
   always @(posedge wb_clk)
     if(~cs_del[0] & ((we_del == 2'b10) | (oe_del == 2'b10)))
       wb_stb_o <= 1;
     else if(wb_ack_i)
       wb_stb_o <= 0;
   
endmodule // gpmc_wb
