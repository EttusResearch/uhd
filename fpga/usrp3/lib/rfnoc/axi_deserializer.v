//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module axi_deserializer #(
  parameter WIDTH = 32)
(
  input clk, input rst, input reverse_output,
  input i_tdata, input  i_tlast, input i_tvalid,  output i_tready,
  output [WIDTH-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready
);

  reg flop_tlast, flop_tlast_latch;
  reg  [WIDTH-1:0] flop_tdata;
  wire [WIDTH-1:0] flop_tdata_reverse, flop_tdata_int;
  reg flop_tvalid;
  reg [$clog2(WIDTH)-1:0] i;

  always @(posedge clk) begin
    if (rst) begin
      flop_tdata       <= 'd0;
      flop_tvalid      <= 1'b0;
      flop_tlast       <= 1'b0;
      flop_tlast_latch <= 1'b0;
      i                <= WIDTH-1;
    end else begin
      flop_tvalid      <= 1'b0;
      if (i_tvalid & i_tready) begin
        flop_tdata[i]  <= i_tdata;
        if (i_tlast) begin
          flop_tlast_latch <= 1'b1;
        end
        if (i == 0) begin
          flop_tvalid      <= 1'b1;
          flop_tlast       <= flop_tlast_latch;
          flop_tlast_latch <= 1'b0;
          i                <= WIDTH-1;
        end else begin
          i                <= i - 1;
        end
      end
    end
  end

  // Reverse flop_tdata
  genvar k;
  generate
    for (k = 0; k < WIDTH; k = k + 1) assign flop_tdata_reverse[WIDTH-1-k] = flop_tdata;
  endgenerate

  assign flop_tdata_int = reverse_output ? flop_tdata_reverse : flop_tdata;

  axi_fifo_flop2 #(.WIDTH(WIDTH)) axi_fifo_flop (
    .clk(clk), .reset(rst), .clear(1'b0),
    .i_tdata({flop_tlast,flop_tdata_int}), .i_tvalid(flop_tvalid), .i_tready(i_tready),
    .o_tdata({o_tlast,o_tdata}),       .o_tvalid(o_tvalid),    .o_tready(o_tready),
    .space(), .occupied());

endmodule