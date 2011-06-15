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



module edge_sync
  #(parameter POSEDGE = 1)
   (input clk,
    input rst,
    input sig,
    output trig);
   
   reg [1:0] delay;
   
   always @(posedge clk)
     if(rst)
       delay <= 2'b00;
     else
       delay <= {delay[0],sig};
   
   assign trig = POSEDGE ? (delay==2'b01) : (delay==2'b10);
   
endmodule // edge_sync


