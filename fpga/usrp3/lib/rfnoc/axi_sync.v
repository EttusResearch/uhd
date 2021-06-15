//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_sync
//
// Description:
//
//   Synchronizes AXI stream buses so data is released on every port
//   simultaneously. Multiple inputs/outputs are supported by concatenating the
//   bus signals together. The number and size of each input/output bus is
//   controlled using parameters.
//
//    **WARNING**: This module violates the AXI4-Stream specification by not
//                 asserting TVALID until it receives TREADY. This will not
//                 work if downstream logic waits for TVALID before asserting
//                 TREADY, which is common. Use with care.
//
// Parameters:
//
//   SIZE      : The number of inputs streams to synchronize.
//   WIDTH     : The width of TDATA on the input streams, if they are all the
//               same width. If they are different widths, then use WIDTH_VEC
//               instead.
//   WIDTH_VEC : A vector of widths corresponding to each stream's TDATA width.
//               Each number in this vector must be 32 bits wide. This defaults
//               to WIDTH bits for all inputs.
//   FIFO_SIZE : Log2 the size of the FIFO to use internally for each stream.
//

module axi_sync #(
  parameter               SIZE      = 2,
  parameter               WIDTH     = 32,
  parameter [32*SIZE-1:0] WIDTH_VEC = {SIZE{WIDTH[31:0]}},
  parameter               FIFO_SIZE = 0
) (
  input  clk,
  input  reset,
  input  clear,

  // Input streams
  input  [len(SIZE)-1:0] i_tdata,
  input  [     SIZE-1:0] i_tlast,
  input  [     SIZE-1:0] i_tvalid,
  output [     SIZE-1:0] i_tready,

  // Output streams
  output [len(SIZE)-1:0] o_tdata,
  output [     SIZE-1:0] o_tlast,
  output [     SIZE-1:0] o_tvalid,
  input  [     SIZE-1:0] o_tready
);

  // Helper function to calculate the combined length of the lower 'n' ports
  // based on widths stored in WIDTH_VEC. Note: If n is negative, returns 0.
  function automatic integer len(input integer n);
    integer i, total;
  begin
    total = 0;
    if (n >= 0) begin
      for (i = 0; i <= n; i = i + 1) begin
        total = total + ((WIDTH_VEC >> 32*i) & 32'hFFFF);
      end
    end
    len = total;
  end
  endfunction

  wire [len(SIZE)-1:0] int_tdata;
  wire [     SIZE-1:0] int_tlast;
  wire [     SIZE-1:0] int_tvalid;
  wire [     SIZE-1:0] int_tready;

  // Generate a FIFO for each stream
  genvar i;
  generate
    for (i = 0; i < SIZE; i = i + 1) begin
      axi_fifo #(
        .WIDTH (len(i)-len(i-1)+1),
        .SIZE  (FIFO_SIZE)
      ) axi_fifo (
        .clk      (clk),
        .reset    (reset),
        .clear    (clear),
        .i_tdata  ({ i_tlast[i], i_tdata[len(i)-1 : len(i-1)] }),
        .i_tvalid (i_tvalid[i]),
        .i_tready (i_tready[i]),
        .o_tdata  ({ int_tlast[i], int_tdata[len(i)-1 : len(i-1)] }),
        .o_tvalid (int_tvalid[i]),
        .o_tready (int_tready[i]),
        .space    (),
        .occupied ()
      );
    end
  endgenerate

  // We allow a transfer and consume the outputs of the FIFOs when all
  // downstream blocks are ready to accept a transfer (o_tready is true for all
  // streams) and all FIFOs have data ready (int_tvalid is true for all FIFOs).
  wire consume = (&int_tvalid) & (&o_tready);

  assign int_tready = {SIZE{consume}};
  assign o_tvalid   = {SIZE{consume}};
  assign o_tdata    = int_tdata;
  assign o_tlast    = int_tlast;

endmodule
