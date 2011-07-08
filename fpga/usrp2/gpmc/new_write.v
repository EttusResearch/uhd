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


module new_write
  (input [15:0] EM_D, input [1:0] EM_NBE, input EM_NCS, input EM_NWE,

   input bus_clk,
   input fifo_clk, input fifo_rst, input clear,
   output [17:0] data_o, output src_rdy_o, input dst_rdy_i,

   input [15:0] frame_len, output reg fifo_ready,
   output reg bus_error );

   wire [15:0] fifo_space;
   reg [15:0]  counter;

   // Synchronize the async control signals
   reg [1:0] 	cs_del, we_del;
   reg [15:0] 	data_del[0:1];
   
   always @(posedge bus_clk)
     if(fifo_rst)
       begin
	  cs_del <= 2'b11;
	  we_del <= 2'b11;
       end
     else
       begin
	  cs_del <= { cs_del[0], EM_NCS };
	  we_del <= { we_del[0], EM_NWE };
	  data_del[1] <= data_del[0];
	  data_del[0] <= EM_D;
       end

   wire first_write = (counter == 0);
   wire last_write = ((counter+1) == frame_len);

   wire [17:0] data_int = {last_write,first_write,data_del[1]};
   wire        src_rdy_int = (~cs_del[1] & ~we_del[1] & we_del[0]); // rising edge
   wire        dst_rdy_int;
   
   always @(posedge bus_clk)
     if(fifo_rst | clear)
       counter <= 0;
     else if(src_rdy_int)
       if(last_write)
	 counter <= 0;
       else
	 counter <= counter + 1;

   always @(posedge bus_clk)
     if(fifo_rst | clear)
       fifo_ready <= 0;
     else
       fifo_ready <= /* first_write & */ (fifo_space > 16'd1023);

   always @(posedge bus_clk)
     if(fifo_rst | clear)
       bus_error <= 0;
     else if(src_rdy_int & ~dst_rdy_int)
       bus_error <= 1;

   fifo_2clock_cascade #(.WIDTH(18), .SIZE(10)) tx_fifo
     (.wclk(bus_clk), .datain(data_int), .src_rdy_i(src_rdy_int), .dst_rdy_o(dst_rdy_int), .space(fifo_space),
      .rclk(fifo_clk), .dataout(data_o), .src_rdy_o(src_rdy_o), .dst_rdy_i(dst_rdy_i), .occupied(), .arst(fifo_rst));

endmodule // new_write
