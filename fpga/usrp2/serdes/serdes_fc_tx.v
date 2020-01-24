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



module serdes_fc_tx
  (input clk, input rst,
   input xon_rcvd, input xoff_rcvd, output reg inhibit_tx);

   // XOFF means stop sending, XON means start sending
   // clock domain stuff happens elsewhere, everything here is on main clk

   reg [15:0] state;
   always @(posedge clk)
     if(rst)
       state <= 0;
     else if(xoff_rcvd)
       state <= 255;
     else if(xon_rcvd)
       state <= 0;
     else if(state !=0)
       state <= state - 1;

   always @(posedge clk)
     inhibit_tx <= (state != 0);
   
endmodule // serdes_fc_tx
