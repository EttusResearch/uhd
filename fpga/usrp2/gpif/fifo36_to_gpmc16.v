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

module fifo36_to_gpmc16
#(
    parameter FIFO_SIZE = 9
)
(
    //input fifo interface
    input fifo_clk, input fifo_rst,
    input [35:0] in_data,
    input in_src_rdy,
    output in_dst_rdy,

    //output interface
    input gpif_clk, input gpif_rst,
    output [15:0] out_data,
    output valid,
    input enable,
    output eof
);

    wire [35:0] data_int;
    wire src_rdy_int, dst_rdy_int;

    fifo_2clock_cascade #(.WIDTH(36), .SIZE(FIFO_SIZE)) fifo_2clk
     (.wclk(fifo_clk), .datain(in_data), .src_rdy_i(in_src_rdy), .dst_rdy_o(in_dst_rdy), .space(),
      .rclk(gpif_clk), .dataout(data_int), .src_rdy_o(src_rdy_int), .dst_rdy_i(dst_rdy_int), .occupied(),
      .arst(fifo_rst | gpif_rst));

    wire [18:0] data18_int;
    fifo36_to_fifo19 #(.LE(1)) f36_to_f19
     (.clk(gpif_clk), .reset(gpif_rst), .clear(1'b0),
      .f36_datain(data_int), .f36_src_rdy_i(src_rdy_int), .f36_dst_rdy_o(dst_rdy_int),
      .f19_dataout(data18_int), .f19_src_rdy_o(valid), .f19_dst_rdy_i(enable) );

    assign out_data = data18_int[15:0];
    assign eof = data18_int[17];

endmodule //fifo_to_gpmc16
