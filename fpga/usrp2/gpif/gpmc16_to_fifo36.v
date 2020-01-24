//
// Copyright 2012 Ettus Research LLC
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

module gpmc16_to_fifo36
#(
    parameter FIFO_SIZE = 9,

    //not ready until minimum xfers of space available
    parameter MIN_SPACE16 = 128
)
(
    //input interface
    input gpif_clk, input gpif_rst,
    input [15:0] in_data,
    input valid,
    output reg ready,

    //output fifo interface
    input fifo_clk, input fifo_rst,
    output [35:0] out_data,
    output out_src_rdy,
    input out_dst_rdy
);

    wire [35:0] data_int;
    wire src_rdy_int, dst_rdy_int;
    wire [18:0] refr_data;
    wire refr_src_rdy, refr_dst_rdy;

    wire [15:0] fifo_space;

    always @(posedge gpif_clk)
        ready <= (fifo_space >= MIN_SPACE16/2);

   packet_reframer packet_reframer 
     (.clk(gpif_clk), .reset(gpif_rst), .clear(1'b0),
      .data_i(in_data), .src_rdy_i(valid), .dst_rdy_o(),
      .data_o(refr_data), .src_rdy_o(refr_src_rdy), .dst_rdy_i(refr_dst_rdy));

   fifo19_to_fifo36 #(.LE(1)) f19_to_f36
     (.clk(gpif_clk), .reset(gpif_rst), .clear(1'b0),
      .f19_datain(refr_data), .f19_src_rdy_i(refr_src_rdy), .f19_dst_rdy_o(refr_dst_rdy),
      .f36_dataout(data_int), .f36_src_rdy_o(src_rdy_int), .f36_dst_rdy_i(dst_rdy_int));

   fifo_2clock_cascade #(.WIDTH(36), .SIZE(FIFO_SIZE)) fifo_2clk
     (.wclk(gpif_clk), .datain(data_int), .src_rdy_i(src_rdy_int), .dst_rdy_o(dst_rdy_int), .space(fifo_space),
      .rclk(fifo_clk), .dataout(out_data), .src_rdy_o(out_src_rdy), .dst_rdy_i(out_dst_rdy), .occupied(),
      .arst(fifo_rst | gpif_rst));

endmodule //fifo_to_gpmc16
