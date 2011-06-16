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


module new_read
  (input clk, input reset, input clear,
   input [17:0] data_i, input src_rdy_i, output dst_rdy_o,
   output reg [15:0] EM_D, input EM_NCS, input EM_NOE,
   output have_packet, output [15:0] frame_len, output bus_error);

   wire [17:0] 	data_int;
   wire 	src_rdy_int, dst_rdy_int;
   
   fifo_cascade #(.WIDTH(18), .SIZE(12)) rx_fifo
     (.clk(clk), .reset(rst), .clear(clear),
      .datain(data_i), .src_rdy_i(src_rdy_i), .dst_rdy_o(dst_rdy_o), .space(),
      .dataout(data_int), .src_rdy_o(src_rdy_int), .dst_rdy_i(dst_rdy_int), .occupied());

   fifo_watcher fifo_watcher
     (.clk(clk), .reset(reset), .clear(clear),
      .src_rdy1(src_rdy_i), .dst_rdy1(dst_rdy_i), .sof1(data_i[16]), .eof1(data_i[17]),
      .src_rdy2(src_rdy_int), .dst_rdy2(dst_rdy_int), .sof2(data_int[16]), .eof2(data_int[17]),
      .have_packet(have_packet), .length(frame_len), .bus_error(bus_error),
      .debug());
   
   // Synchronize the async control signals
   reg [1:0] 	cs_del, oe_del;
   reg [15:0] 	counter;
   
   always @(posedge clk)
     if(reset)
       begin
	  cs_del <= 2'b11;
	  oe_del <= 2'b11;
       end
     else
       begin
	  cs_del <= { cs_del[0], EM_NCS };
	  oe_del <= { oe_del[0], EM_NOE };
       end

   assign dst_rdy_int = ( ~cs_del[1] & ~oe_del[1] & oe_del[0]);  // change output on trailing edge

   //always @(posedge clk)   // 3 cycle latency ( OE -> OE_del -> FIFO -> output REG )
   always @*                 // 2 cycle latency ( OE -> OE_del -> FIFO )
     EM_D <= data_int[15:0];
   
endmodule // new_read
