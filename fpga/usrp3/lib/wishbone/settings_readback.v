//
// Copyright 2011-2012 Ettus Research LLC
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

//
// Use this module in conjunction with settings_bus.v to add stateful reads
// to the settings bis. This enables you to do things like have registers reset atomicly
// as they are read. It also pipelines the address path to ease timing.
//

module settings_readback
  #(parameter AWIDTH=16, parameter DWIDTH=32, parameter RB_ADDRW=2)
   (
    input wb_clk, 
    input wb_rst, 
    input [AWIDTH-1:0] wb_adr_i,
    input wb_stb_i,
    input wb_we_i,
    input [DWIDTH-1:0] rb_data,
    output reg [RB_ADDRW-1:0] rb_addr,
    output [DWIDTH-1:0] wb_dat_o,
    output reg rb_rd_stb
    );

   always @(posedge wb_clk)
     if (wb_stb_i && ~wb_we_i) begin
	rb_addr <= wb_adr_i[RB_ADDRW+1:2];
	rb_rd_stb <= 1'b1;	
     end else begin
	rb_rd_stb <= 1'b0;	
     end
   
   assign wb_dat_o = rb_data;

   

endmodule // settings_readback

  