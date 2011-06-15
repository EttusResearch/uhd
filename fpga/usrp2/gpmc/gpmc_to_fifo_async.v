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


module gpmc_to_fifo_async
  (input [15:0] EM_D, input [1:0] EM_NBE, input EM_NCS, input EM_NWE,

   input fifo_clk, input fifo_rst, input clear,
   output reg [17:0] data_o, output reg src_rdy_o, input dst_rdy_i,

   input [15:0] frame_len, input [15:0] fifo_space, output reg fifo_ready,
   output reg bus_error );

   reg [15:0] counter;
   // Synchronize the async control signals
   reg [1:0] 	cs_del, we_del;
   always @(posedge fifo_clk)
     if(fifo_rst)
       begin
	  cs_del <= 2'b11;
	  we_del <= 2'b11;
       end
     else
       begin
	  cs_del <= { cs_del[0], EM_NCS };
	  we_del <= { we_del[0], EM_NWE };
       end

   wire do_write = (~cs_del[0] & (we_del == 2'b10));
   wire first_write = (counter == 0);
   wire last_write = ((counter+1) == frame_len);

   always @(posedge fifo_clk)
     if(do_write)
       begin
	  data_o[15:0] <= EM_D;
	  data_o[16] <= first_write;
	  data_o[17] <= last_write;
	  //  no byte writes data_o[18] <= |EM_NBE;  // mark half full if either is not enabled  FIXME
       end

   always @(posedge fifo_clk)
     if(fifo_rst | clear)
       src_rdy_o <= 0;
     else if(do_write)
       src_rdy_o <= 1;
     else
       src_rdy_o <= 0;    // Assume it was taken

   always @(posedge fifo_clk)
     if(fifo_rst | clear)
       counter <= 0;
     else if(do_write)
       if(last_write)
	 counter <= 0;
       else
	 counter <= counter + 1;

   always @(posedge fifo_clk)
     if(fifo_rst | clear)
       fifo_ready <= 0;
     else
       fifo_ready <= /* first_write & */ (fifo_space > 16'd1023);

   always @(posedge fifo_clk)
     if(fifo_rst | clear)
       bus_error <= 0;
     else if(src_rdy_o & ~dst_rdy_i)
       bus_error <= 1;
   
endmodule // gpmc_to_fifo_async
