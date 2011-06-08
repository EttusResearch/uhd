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



module coeff_ram (input clock, input [3:0] rd_addr, output reg [15:0] rd_data);

   always @(posedge clock)
     case (rd_addr)
       4'd0 : rd_data <= #1 -16'd16;
       4'd1 : rd_data <= #1 16'd74;
       4'd2 : rd_data <= #1 -16'd254;
       4'd3 : rd_data <= #1 16'd669;
       4'd4 : rd_data <= #1 -16'd1468;
       4'd5 : rd_data <= #1 16'd2950;
       4'd6 : rd_data <= #1 -16'd6158;
       4'd7 : rd_data <= #1 16'd20585;
       4'd8 : rd_data <= #1 16'd20585;
       4'd9 : rd_data <= #1 -16'd6158;
       4'd10 : rd_data <= #1 16'd2950;
       4'd11 : rd_data <= #1 -16'd1468;
       4'd12 : rd_data <= #1 16'd669;
       4'd13 : rd_data <= #1 -16'd254;
       4'd14 : rd_data <= #1 16'd74;
       4'd15 : rd_data <= #1 -16'd16;
       default : rd_data <= #1 16'd0;
     endcase // case(rd_addr)
   
endmodule // ram
