//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module strobed_to_axi #(
  parameter WIDTH = 32,
  parameter FIFO_SIZE = 1
)(
  input clk, input reset, input clear,
  input in_stb, input [WIDTH-1:0] in_data, input in_last,
  output [WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready
);

  axi_fifo #(.WIDTH(WIDTH+1), .SIZE(FIFO_SIZE)) axi_fifo (
    .clk(clk), .reset(reset), .clear(clear),
    .i_tdata({in_last,in_data}), .i_tvalid(in_stb), .i_tready(),
    .o_tdata({o_tlast,o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready),
    .space(), .occupied());
endmodule