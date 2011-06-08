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



module serdes_fc_rx
  #(parameter LWMARK = 64,
    parameter HWMARK = 320)
    (input clk, input rst,
     input [15:0] fifo_space, 
     output reg send_xon,
     output reg send_xoff,
     input sent);
    
   reg [15:0] 	  countdown;
   reg 		  send_xon_int, send_xoff_int;
   
   always @(posedge clk)
     if(rst)
       begin
	  send_xon_int <= 0;
	  send_xoff_int <= 0;
	  countdown <= 0;
       end
     else 
       begin
	  send_xon_int <= 0;
	  send_xoff_int <= 0;
	  if(countdown == 0)
	    if(fifo_space < LWMARK)
	      begin
		 send_xoff_int <= 1;
		 countdown <= 240;
	      end
	    else
	      ;
	  else
	    if(fifo_space > HWMARK)
	      begin
		 send_xon_int <= 1;
		 countdown <= 0;
	      end
	    else
	      countdown <= countdown - 1;
       end // else: !if(rst)

   // If we are between the high and low water marks, we let the countdown expire

   always @(posedge clk)
     if(rst)
       send_xon <= 0;
     else if(send_xon_int)
       send_xon <= 1;
     else if(sent)
       send_xon <= 0;

   always @(posedge clk)
     if(rst)
       send_xoff <= 0;
     else if(send_xoff_int)
       send_xoff <= 1;
     else if(sent)
       send_xoff <= 0;
   
endmodule // serdes_fc_rx
