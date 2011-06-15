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



module fifo_watcher
  (input clk, input reset, input clear,
   input src_rdy1, input dst_rdy1, input sof1, input eof1,
   input src_rdy2, input dst_rdy2, input sof2, input eof2,
   output reg have_packet, output [15:0] length, output reg bus_error,
   output [31:0] debug);

   wire   write = src_rdy1 & dst_rdy1 & eof1;
   wire   read = src_rdy2 & dst_rdy2 & eof2;
   wire   have_packet_int;
   reg [15:0] counter;
   wire [4:0] pkt_count;
   assign debug = pkt_count;
   
   fifo_short #(.WIDTH(16)) frame_lengths
     (.clk(clk), .reset(reset), .clear(clear),
      .datain(counter), .src_rdy_i(write), .dst_rdy_o(),
      .dataout(length), .src_rdy_o(have_packet_int), .dst_rdy_i(read),
      .occupied(pkt_count), .space());

   always @(posedge clk)
     if(reset | clear)
       counter <= 1;   // Start at 1
     else if(src_rdy1 & dst_rdy1)
       if(eof1)
	 counter <= 1;
       else
	 counter <= counter + 1;

   always @(posedge clk)
     if(reset | clear)
       bus_error <= 0;
     else if(dst_rdy2 & ~src_rdy2)
       bus_error <= 1;
     else if(read & ~have_packet_int)
       bus_error <= 1;

   reg 	      in_packet;
   always @(posedge clk)
     if(reset | clear)
       have_packet <= 0;
     else 
       have_packet <= (have_packet_int & ~in_packet) | (pkt_count>1) ;
   
   always @(posedge clk)
     if(reset | clear)
       in_packet <= 0;
     else if(src_rdy2 & dst_rdy2)
       if(eof2)
	 in_packet <= 0;
       else
	 in_packet <= 1;
   
endmodule // fifo_watcher
