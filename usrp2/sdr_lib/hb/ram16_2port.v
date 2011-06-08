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



module ram16_2port (input clock, input write, 
		    input [3:0] wr_addr, input [15:0] wr_data,
		    input [3:0] rd_addr1, output reg [15:0] rd_data1,
		    input [3:0] rd_addr2, output reg [15:0] rd_data2);
   
   reg [15:0] 			ram_array [0:31];
   
   always @(posedge clock)
     rd_data1 <= #1 ram_array[rd_addr1];
   
   always @(posedge clock)
     rd_data2 <= #1 ram_array[rd_addr2];
   
   always @(posedge clock)
     if(write)
       ram_array[wr_addr] <= #1 wr_data;
   
endmodule // ram16_2port


