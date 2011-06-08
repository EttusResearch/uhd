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


module gpif_rd
  (input gpif_clk, input gpif_rst,
   output [15:0] gpif_data, input gpif_rd, input gpif_ep,
   output reg gpif_empty_d, output reg gpif_empty_c,
   output reg gpif_flush,
   
   input sys_clk, input sys_rst,
   input [18:0] data_i, input src_rdy_i, output dst_rdy_o,
   input [18:0] resp_i, input resp_src_rdy_i, output resp_dst_rdy_o,
   output [31:0] debug
   );
   
   wire [18:0] 	 data_o; // occ bit indicates flush
   wire [17:0] 	 resp_o; // no occ bit
   wire 	 final_rdy_data, final_rdy_resp;
   
   // 33/257 Bug Fix
   reg [8:0] 	read_count;
   always @(negedge gpif_clk)
     if(gpif_rst)
       read_count <= 0;
     else if(gpif_rd)
       read_count <= read_count + 1;
     else
       read_count <= 0;

   // Data Path
   wire [18:0] 	data_int;
   wire 	src_rdy_int, dst_rdy_int;
   fifo_2clock_cascade #(.WIDTH(19), .SIZE(4)) rd_fifo_2clk
     (.wclk(sys_clk), .datain(data_i[18:0]), .src_rdy_i(src_rdy_i), .dst_rdy_o(dst_rdy_o), .space(),
      .rclk(~gpif_clk), .dataout(data_int), .src_rdy_o(src_rdy_int), .dst_rdy_i(dst_rdy_int), .occupied(),
      .arst(sys_rst));

   reg [7:0] 	packet_count;
   wire 	consume_data_line = gpif_rd & ~gpif_ep & ~read_count[8];
   wire 	produce_eop = src_rdy_int & dst_rdy_int & data_int[17];
   wire 	consume_sop = consume_data_line & final_rdy_data & data_o[16];
   wire 	consume_eop = consume_data_line & final_rdy_data & data_o[17];
   
   fifo_cascade #(.WIDTH(19), .SIZE(10)) rd_fifo
     (.clk(~gpif_clk), .reset(gpif_rst), .clear(0),
      .datain(data_int), .src_rdy_i(src_rdy_int), .dst_rdy_o(dst_rdy_int), .space(),
      .dataout(data_o), .src_rdy_o(final_rdy_data), .dst_rdy_i(consume_data_line), .occupied());

   always @(negedge gpif_clk)
     if(gpif_rst)
       packet_count <= 0;
     else
       if(produce_eop & ~consume_sop)
	 packet_count <= packet_count + 1;
       else if(consume_sop & ~produce_eop)
	 packet_count <= packet_count - 1;

   always @(negedge gpif_clk)
     if(gpif_rst)
       gpif_empty_d <= 1;
     else
       gpif_empty_d <= ~|packet_count;

   // Use occ bit to signal a gpif flush
   always @(negedge gpif_clk)
     if(gpif_rst)
       gpif_flush <= 0;
     else if(consume_eop & data_o[18])
       gpif_flush <= ~gpif_flush;
   
   // Response Path
   wire [15:0] 	resp_fifolevel;
   wire 	consume_resp_line = gpif_rd & gpif_ep & ~read_count[4];

   fifo_2clock_cascade #(.WIDTH(18), .SIZE(4)) resp_fifo_2clk
     (.wclk(sys_clk), .datain(resp_i[17:0]), .src_rdy_i(resp_src_rdy_i), .dst_rdy_o(resp_dst_rdy_o), .space(),
      .rclk(~gpif_clk), .dataout(resp_o), 
      .src_rdy_o(final_rdy_resp), .dst_rdy_i(consume_resp_line), .occupied(resp_fifolevel),
      .arst(sys_rst));

   // FIXME -- handle short packets
   
   always @(negedge gpif_clk)
     if(gpif_rst)
       gpif_empty_c <= 1;
     else
       gpif_empty_c <= resp_fifolevel < 16;
   
   // Output Mux
   assign gpif_data = gpif_ep ? resp_o[15:0] : data_o[15:0];

   assign debug = { { 16'd0 },
		    { data_int[17:16], data_o[17:16], packet_count[3:0] },
		    { consume_sop, consume_eop, final_rdy_data, data_o[18], consume_data_line, consume_resp_line, src_rdy_int, dst_rdy_int} };
   
endmodule // gpif_rd
