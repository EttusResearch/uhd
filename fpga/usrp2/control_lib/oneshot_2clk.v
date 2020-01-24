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


// Retime a single bit from one clock domain to another
// Guarantees that no matter what the relative clock rates, if the in signal is high for at least
//   one clock cycle in the clk_in domain, then the out signal will be high for at least one
//   clock cycle in the clk_out domain.  If the in signal goes high again before the process is done
//   the behavior is undefined.  No other guarantees.  Designed for passing reset into a new
//   clock domain.

module oneshot_2clk
  (input clk_in,
   input in,
   input clk_out,
   output reg out);

   reg 	  del_in = 0;
   reg 	  sendit = 0, gotit = 0;
   reg 	  sendit_d = 0, gotit_d = 0;
   
   always @(posedge clk_in) del_in <= in;

   always @(posedge clk_in)
     if(in & ~del_in)  // we have a positive edge
       sendit <= 1;
     else if(gotit)
       sendit <= 0;

   always @(posedge clk_out) sendit_d <= sendit;
   always @(posedge clk_out) out <= sendit_d;

   always @(posedge clk_in) gotit_d <= out;
   always @(posedge clk_in) gotit <= gotit_d;

endmodule // oneshot_2clk

  
