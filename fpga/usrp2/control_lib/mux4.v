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



module mux4
  #(parameter WIDTH=32, parameter DISABLED=0)
    (input en,
     input [1:0] sel,
     input [WIDTH-1:0] i0,
     input [WIDTH-1:0] i1,
     input [WIDTH-1:0] i2,
     input [WIDTH-1:0] i3,
     output [WIDTH-1:0] o);

   assign 		o = en ? (sel[1] ? (sel[0] ? i3 : i2) : (sel[0] ? i1 : i0)) :
			DISABLED;
   
endmodule // mux4
