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



module mult (input clock, input signed [15:0] x, input signed [15:0] y, output reg signed [30:0] product,
	     input enable_in, output reg enable_out );

   always @(posedge clock)
     if(enable_in)
       product <= #1 x*y;
     else
       product <= #1 31'd0;
   
   always @(posedge clock)
     enable_out <= #1 enable_in;
   
endmodule // mult

