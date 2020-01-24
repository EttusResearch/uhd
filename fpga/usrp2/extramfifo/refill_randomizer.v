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

//
// EMI mitigation.
// Process FULL flag from FIFO so that de-assertion
// (FIFO now not FULL) is delayed by a pseudo random
// value, but assertion is passed straight through.
// 


module refill_randomizer
  #(parameter BITS=7)
    (
     input clk,
     input rst,
     input full_in,
     output full_out
     );
   
   wire 	    feedback;
   reg 	    full_last;
   wire     full_deasserts;
   reg [6:0] shift_reg;
   reg [6:0] count;
   reg 	     delayed_fall;
   

   always @(posedge clk)
     full_last <= full_in;
   
   assign    full_deasserts = full_last & ~full_in;

   // 7 bit LFSR
   always @(posedge clk)
     if (rst)
       shift_reg <= 7'b1;
     else
       if (full_deasserts)
	 shift_reg <= {shift_reg[5:0],feedback};

   assign    feedback = ^(shift_reg & 7'h41);

   always @(posedge clk)
     if (rst)
       begin
	  count <= 1;
	  delayed_fall  <= 1;
       end
     else if (full_deasserts)
       begin
	  count <= shift_reg;
	  delayed_fall <= 1;
       end
     else if (count == 1)
       begin
	  count <= 1;
	  delayed_fall <= 0;
       end
     else
       begin
	  count <= count - 1;
	  delayed_fall <= 1;
       end
   
   // Full_out goes instantly high if full_in does. However its fall is delayed.
   assign    full_out = (full_in == 1) || (full_last == 1) || delayed_fall;

endmodule