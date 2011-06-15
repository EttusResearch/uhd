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



module ram16_2sum (input clock, input write, 
		   input [3:0] wr_addr, input [15:0] wr_data,
		   input [3:0] rd_addr1, input [3:0] rd_addr2,
                   output reg [15:0] sum);
   
   reg signed [15:0] 	  ram_array [0:15];
   reg signed [15:0] 	  a,b;
   wire signed [16:0] 	  sum_int;
   
   always @(posedge clock)
     if(write)
       ram_array[wr_addr] <= #1 wr_data;
      
   always @(posedge clock)
     begin
	a <= #1 ram_array[rd_addr1];
	b <= #1 ram_array[rd_addr2];
     end
   
   assign sum_int = {a[15],a} + {b[15],b};
   
   always @(posedge clock)
     sum <= #1 sum_int[16:1] + (sum_int[16]&sum_int[0]);
     
endmodule // ram16_2sum
