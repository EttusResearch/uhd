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


// up to 8 semaphores

// After a read operation, the semaphore is always locked
//  If it was already locked before the read (meaning someone else holds the lock)
//    then a 1 is returned
//  If it was not already locked (meaning the reader now holds the lock)
//    then a 0 is returned

// A write operation clears the lock

module wb_semaphore 
  #(parameter count=8, DBUS_WIDTH=32)
    (input wb_clk_i,
     input wb_rst_i,
     input [DBUS_WIDTH-1:0] wb_dat_i,
     input [2:0] wb_adr_i,
     input wb_cyc_i,
     input wb_stb_i,
     input wb_we_i,
     output wb_ack_o,
     output [DBUS_WIDTH-1:0] wb_dat_o);

   reg [count-1:0] locked;

   always @(posedge clock)
     if(wb_rst_i)
       locked <= {count{1'b0}};
     else if(wb_stb_i)
       if(wb_we_i)
	 locked[adr_i] <= 1'b0;
       else
	 locked[adr_i] <= 1'b1;

   assign 	   wb_dat_o[DBUS_WIDTH-1:1] = {(DBUS_WIDTH-1){1'b0}};
   assign 	   wb_dat_o[0] = locked[adr_i];
   assign 	   wb_ack_o = wb_stb_i;
   

endmodule // wb_semaphore

     
