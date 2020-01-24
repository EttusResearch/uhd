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



module coeff_rom (input clock, input [2:0] addr, output reg [15:0] data);

   always @(posedge clock)
     case (addr)
       3'd0 : data <= #1 -16'd49;
       3'd1 : data <= #1 16'd165;
       3'd2 : data <= #1 -16'd412;
       3'd3 : data <= #1 16'd873;
       3'd4 : data <= #1 -16'd1681;
       3'd5 : data <= #1 16'd3135;
       3'd6 : data <= #1 -16'd6282;
       3'd7 : data <= #1 16'd20628;
     endcase // case(addr)
      
endmodule // coeff_rom


