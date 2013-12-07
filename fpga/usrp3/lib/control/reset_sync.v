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



module reset_sync
  (input clk,
   input reset_in,
   output  reset_out);

   reg 	   reset_int;

   reg 	   reset_out_tmp;

   //synthesis attribute async_reg of reset_out_tmp is "true";
   //synthesis attribute async_reg of reset_int is "true";
   always @(posedge clk or posedge reset_in)
     if(reset_in)
       {reset_out_tmp,reset_int} <= 2'b11;
     else
       {reset_out_tmp,reset_int} <= {reset_int,1'b0};

   assign reset_out = reset_out_tmp;


endmodule // reset_sync
