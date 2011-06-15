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



module address_filter_promisc
  (input clk,
   input reset,
   input go,
   input [7:0] data,
   output match,
   output done);

   reg [2:0] af_state;

   always @(posedge clk)
     if(reset)
       af_state     <= 0;
     else
       if(go)
	 af_state <= (data[0] == 1'b0) ? 1 : 7;
       else
	 case(af_state)
	   1 : af_state <= 2;
	   2 : af_state <= 3;
	   3 : af_state <= 4;
	   4 : af_state <= 5;
	   5 : af_state <= 6;
	   6, 7 : af_state <= 0;
	 endcase // case (af_state)

   assign match  = (af_state==6);
   assign done 	 = (af_state==6)|(af_state==7);
   
endmodule // address_filter_promisc
