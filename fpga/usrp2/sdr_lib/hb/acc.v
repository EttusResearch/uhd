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



module acc (input clock, input reset, input clear, input enable_in, output reg enable_out,
	    input signed [30:0] addend, output reg signed [33:0] sum );

   always @(posedge clock)
     if(reset)
       sum <= #1 34'd0;
     //else if(clear & enable_in)
     //  sum <= #1 addend;
     //else if(clear)
     //  sum <= #1 34'd0;
     else if(clear)
       sum <= #1 addend;
     else if(enable_in)
       sum <= #1 sum + addend;

   always @(posedge clock)
     enable_out <= #1 enable_in;
   
endmodule // acc

