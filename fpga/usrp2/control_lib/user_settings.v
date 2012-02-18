//
// Copyright 2012 Ettus Research LLC
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

// User settings bus
//
// Provides 8-bit address, 32-bit data write only bus for user settings, consumes to addresses in
// normal settings bus.
//
// Write user address to BASE
// Write user data to BASE+1
//
// The user_set_stb will strobe after data write, must write new address even if same as previous one.

module user_settings
  #(parameter BASE=0)
  (input clk,
   input rst,

   input         set_stb,
   input [7:0]   set_addr,
   input [31:0]  set_data,

   output        set_stb_user,
   output [7:0]  set_addr_user,
   output [31:0] set_data_user
   );

   wire 	 addr_changed, data_changed;
   reg 		 stb_int;
   
   setting_reg #(.my_addr(BASE+0),.width(8)) sr_0
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(set_addr_user),.changed(addr_changed) );
   
   setting_reg #(.my_addr(BASE+1)) sr_1
     (.clk(clk),.rst(rst),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(set_data_user),.changed(data_changed) );
   
   always @(posedge clk)
     if (rst|set_stb_user)
       stb_int <= 0;
     else
       if (addr_changed)
         stb_int <= 1;
   
   assign set_stb_user = stb_int & data_changed;
   
endmodule // user_settings

