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



module valve36
  (input clk, input reset, input clear,
   input shutoff,
   input [35:0] data_i, input src_rdy_i, output dst_rdy_o,
   output [35:0] data_o, output src_rdy_o, input dst_rdy_i);
   
   reg 		 shutoff_int, active;
   wire active_next = (src_rdy_i & dst_rdy_o)? ~data_i[33] : active;

   assign data_o = data_i;

   assign dst_rdy_o = shutoff_int ? 1'b1 : dst_rdy_i;
   assign src_rdy_o = shutoff_int ? 1'b0 : src_rdy_i;
   
   always @(posedge clk)
     if(reset | clear)
       active <= 0;
     else
       active <= active_next;
   
   always @(posedge clk)
     if(reset | clear)
       shutoff_int <= 0;
     else if(~active_next)
       shutoff_int <= shutoff;
   
endmodule // valve36
