//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Synchronizes AXI stream buses so data is released on every port simultaneously.
//
// Note: If inputs have inequal bitwidths, use WIDTH_VEC instead of WIDTH to define
//       the individual bit widths. Each bit width is defined with 8-bits stuffed
//       into a vector of width 8*SIZE.
//

module axi_sync #(
  parameter               SIZE      = 2,
  parameter               WIDTH     = 32,
  parameter [32*SIZE-1:0] WIDTH_VEC = {SIZE{WIDTH[31:0]}},
  parameter               FIFO_SIZE = 0
)(
  input clk, input reset, input clear,
  input [msb(SIZE,WIDTH_VEC)-1:0] i_tdata, input  [SIZE-1:0] i_tlast, input  [SIZE-1:0] i_tvalid, output [SIZE-1:0] i_tready,
  output [msb(SIZE,WIDTH_VEC)-1:0] o_tdata, output [SIZE-1:0] o_tlast, output [SIZE-1:0] o_tvalid, input  [SIZE-1:0] o_tready
);

  // Helper function to calculate the MSB index based on widths stored in WIDTH_VEC.
  // Note: If n is negative, returns 0
  function automatic integer msb(input integer n, input [SIZE*32-1:0] bit_vec);
    integer i, total;
  begin
    total = 0;
    if (n >= 0) begin
      for (i = 0; i <= n; i = i + 1) begin
        total = total + ((bit_vec >> 32*i) & 32'hFF);
      end
    end
    msb = total;
  end
  endfunction

  wire [msb(SIZE,WIDTH_VEC)-1:0] int_tdata;
  wire [SIZE-1:0] int_tlast, int_tvalid, int_tready;

  genvar i;
  generate
    for (i = 0; i < SIZE; i = i + 1) begin
      axi_fifo #(.WIDTH(msb(i,WIDTH_VEC)-msb(i-1,WIDTH_VEC)+1), .SIZE(FIFO_SIZE)) axi_fifo (
        .clk(clk), .reset(reset), .clear(clear),
        .i_tdata({i_tlast[i],i_tdata[msb(i,WIDTH_VEC)-1:msb(i-1,WIDTH_VEC)]}),
        .i_tvalid(i_tvalid[i]), .i_tready(i_tready[i]),
        .o_tdata({int_tlast[i],int_tdata[msb(i,WIDTH_VEC)-1:msb(i-1,WIDTH_VEC)]}),
        .o_tvalid(int_tvalid[i]), .o_tready(int_tready[i]),
        .space(), .occupied());
    end
  endgenerate

  assign o_tdata    = int_tdata;
  assign o_tlast    = int_tlast;

  wire consume = (&int_tvalid) & (&o_tready);
  assign int_tready = {SIZE{consume}};
  assign o_tvalid   = {SIZE{consume}};

endmodule