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


module gpif_wr
  (input gpif_clk, input gpif_rst,
   input [15:0] gpif_data, input gpif_wr, input gpif_ep,
   output reg gpif_full_d, output reg gpif_full_c,

   input sys_clk, input sys_rst,
   output [18:0] data_o, output src_rdy_o, input dst_rdy_i,
   output [18:0] ctrl_o, output ctrl_src_rdy_o, input ctrl_dst_rdy_i,
   output [31:0] debug );

   reg 		 wr_reg, ep_reg;
   reg [15:0] 	 gpif_data_reg;
   
   always @(posedge gpif_clk)
     begin
	ep_reg <= gpif_ep;
	wr_reg <= gpif_wr;
	gpif_data_reg <= gpif_data;
     end

   reg [9:0] write_count;
   
   always @(posedge gpif_clk)
     if(gpif_rst)
       write_count <= 0;
     else if(wr_reg)
       write_count <= write_count + 1;
     else
       write_count <= 0;

   reg 	     sop;
   wire      eop = (write_count == 255);
   wire      eop_ctrl = (write_count == 15);
   
   always @(posedge gpif_clk)
     sop <= gpif_wr & ~wr_reg;

   // Data Path
   wire [15:0] fifo_space;
   always @(posedge gpif_clk)
     if(gpif_rst)
       gpif_full_d <= 1;
     else
       gpif_full_d <= fifo_space < 256;

   wire [17:0] data_int;
   wire        src_rdy_int, dst_rdy_int;

   fifo_cascade #(.WIDTH(18), .SIZE(10)) wr_fifo
     (.clk(gpif_clk), .reset(gpif_rst), .clear(0),
      .datain({eop,sop,gpif_data_reg}), .src_rdy_i(~ep_reg & wr_reg & ~write_count[8]), .dst_rdy_o(), .space(fifo_space),
      .dataout(data_int), .src_rdy_o(src_rdy_int), .dst_rdy_i(dst_rdy_int), .occupied());
   
   fifo_2clock_cascade #(.WIDTH(18), .SIZE(4)) wr_fifo_2clk
     (.wclk(gpif_clk), .datain(data_int), .src_rdy_i(src_rdy_int), .dst_rdy_o(dst_rdy_int), .space(),
      .rclk(sys_clk), .dataout(data_o[17:0]), .src_rdy_o(src_rdy_o), .dst_rdy_i(dst_rdy_i), .occupied(),
      .arst(sys_rst));
   assign data_o[18] = 1'b0;
   
   // Control Path
   wire [15:0] ctrl_fifo_space;
   always @(posedge gpif_clk)
     if(gpif_rst)
       gpif_full_c <= 1;
     else
       gpif_full_c <= ctrl_fifo_space < 16;
   
   fifo_2clock_cascade #(.WIDTH(19), .SIZE(4)) ctrl_fifo_2clk
     (.wclk(gpif_clk), .datain({1'b0,eop_ctrl,sop,gpif_data_reg}), 
      .src_rdy_i(ep_reg & wr_reg & ~write_count[4]), .dst_rdy_o(), .space(ctrl_fifo_space),
      .rclk(sys_clk), .dataout(ctrl_o[18:0]), 
      .src_rdy_o(ctrl_src_rdy_o), .dst_rdy_i(ctrl_dst_rdy_i), .occupied(),
      .arst(sys_rst));

   assign debug = { 16'd0, ep_reg, wr_reg, eop, sop, (~ep_reg & wr_reg & ~write_count[8]), src_rdy_int, dst_rdy_int, write_count[8:0]};
   
endmodule // gpif_wr
