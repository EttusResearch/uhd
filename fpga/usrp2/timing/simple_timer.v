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



module simple_timer
  #(parameter BASE=0)
   (input clk, input reset,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    output reg onetime_int, output reg periodic_int);

   reg [31:0]  onetime_ctr;
   always @(posedge clk)
     if(reset)
       begin
	  onetime_int 	  <= 0;
	  onetime_ctr 	  <= 0;
       end
     else
       if(set_stb & (set_addr == BASE))
	 begin
	    onetime_int   <= 0;
	    onetime_ctr   <= set_data;
	 end
       else 
	 begin
	    if(onetime_ctr == 1)
	      onetime_int <= 1;
	    if(onetime_ctr != 0)
	      onetime_ctr <= onetime_ctr - 1;
	    else
	      onetime_int <= 0;
	 end // else: !if(set_stb & (set_addr == BASE))
   
   reg [31:0]  periodic_ctr, period;
   always @(posedge clk)
     if(reset)
       begin
	  periodic_int 	     <= 0;
	  periodic_ctr 	     <= 0;
	  period 	     <= 0;
       end
     else
       if(set_stb & (set_addr == (BASE+1)))
	 begin
	    periodic_int     <= 0;
	    periodic_ctr     <= set_data;
	    period 	     <= set_data;
	 end
       else 
	 if(periodic_ctr == 1)
	   begin
	      periodic_int   <= 1;
	      periodic_ctr   <= period;
	   end
	 else
	   if(periodic_ctr != 0)
	     begin
		periodic_int <= 0;
		periodic_ctr <= periodic_ctr - 1;
	     end
   
endmodule // simple_timer
