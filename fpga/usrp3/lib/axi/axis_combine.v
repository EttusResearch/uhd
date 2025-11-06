//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_combine
//
// Description:
//
//   Combines AXI stream buses into one output bus with concatenated data signals.
//
// Parameters:
//
//   SIZE           : The number of inputs streams to combine.
//   WIDTH          : The width of TDATA on the input streams, if they are all the
//                    same width. If they are different widths, then use WIDTH_VEC
//                    instead.
//   WIDTH_VEC      : A vector of widths corresponding to each stream's TDATA width.
//                    Each number in this vector must be 32 bits wide. This defaults
//                    to WIDTH bits for all inputs.
//   USER_WIDTH     : The width of TUSER on the input streams, if they are all the
//                    same width. If they are different widths, then use USER_WIDTH_VEC
//                    instead.
//   USER_WIDTH_VEC : A vector of widths corresponding to each stream's TUSER width.
//                    Each number in this vector must be 32 bits wide. This defaults
//                    to USER_WIDTH bits for all inputs.
//   FIFO_SIZE_LOG2 : Log2 the size of the FIFO to use internally for each stream.
//

`default_nettype none

module axis_combine #(
  parameter               SIZE           = 2,
  parameter               WIDTH          = 32,
  parameter               USER_WIDTH     = 1,
  parameter [32*SIZE-1:0] WIDTH_VEC      = {SIZE{WIDTH[31:0]}},
  parameter [32*SIZE-1:0] USER_WIDTH_VEC = {SIZE{USER_WIDTH[31:0]}},
  parameter               FIFO_SIZE_LOG2 = 0
) (
  input  wire clk,
  input  wire reset,

  // Input streams
  input  wire [len(SIZE-1,     WIDTH_VEC)-1:0] s_axis_tdata,
  input  wire [len(SIZE-1,USER_WIDTH_VEC)-1:0] s_axis_tuser,
  input  wire [                      SIZE-1:0] s_axis_tlast,
  input  wire [                      SIZE-1:0] s_axis_tvalid,
  output wire [                      SIZE-1:0] s_axis_tready,

  // Output streams
  output wire [len(SIZE-1,     WIDTH_VEC)-1:0] m_axis_tdata,
  output wire [len(SIZE-1,USER_WIDTH_VEC)-1:0] m_axis_tuser,
  output wire                                  m_axis_tlast,
  output wire                                  m_axis_tvalid,
  input  wire                                  m_axis_tready
);

  // Helper function to calculate the combined length of the lower 'n' ports
  // based on the concatenated widths (each 32-bits wide) stored in width_vector.
  // Note: If n is negative, returns 0.
  function automatic integer len(input integer n, input [32*SIZE-1:0] width_vector);
    integer i, total;
  begin
    total = 0;
    if (n >= 0) begin
      for (i = 0; i <= n; i = i + 1) begin
        total = total + width_vector[32*i +: 32];
      end
    end
    len = total;
  end
  endfunction

  wire [len(SIZE-1,     WIDTH_VEC)-1:0] int_tdata;
  wire [len(SIZE-1,USER_WIDTH_VEC)-1:0] int_tuser;
  wire [                      SIZE-1:0] int_tlast;
  wire [                      SIZE-1:0] int_tvalid;
  wire [                      SIZE-1:0] int_tready;

  // Generate a FIFO for each stream
  genvar i;
  generate
    for (i = 0; i < SIZE; i = i + 1) begin
      axi_fifo #(
        .WIDTH (WIDTH_VEC[32*i +: 32] + USER_WIDTH_VEC[32*i +: 32] + 1),
        .SIZE  (FIFO_SIZE_LOG2)
      ) axi_fifo (
        .clk      (clk),
        .reset    (reset),
        .clear    (),
        .i_tdata  ({ s_axis_tlast[i],
                     s_axis_tuser[len(i,USER_WIDTH_VEC)-1 : len(i-1,USER_WIDTH_VEC)],
                     s_axis_tdata[len(i,     WIDTH_VEC)-1 : len(i-1,     WIDTH_VEC)] }),
        .i_tvalid (s_axis_tvalid[i]),
        .i_tready (s_axis_tready[i]),
        .o_tdata  ({ int_tlast[i],
                     int_tuser[len(i,USER_WIDTH_VEC)-1 : len(i-1,USER_WIDTH_VEC)],
                     int_tdata[len(i,     WIDTH_VEC)-1 : len(i-1,     WIDTH_VEC)] }),
        .o_tvalid (int_tvalid[i]),
        .o_tready (int_tready[i]),
        .space    (),
        .occupied ()
      );
    end
  endgenerate

  assign int_tready      = { SIZE {m_axis_tvalid & m_axis_tready} };

  assign m_axis_tvalid   = &int_tvalid;
  assign m_axis_tdata    = int_tdata;
  assign m_axis_tuser    = int_tuser;
  assign m_axis_tlast    = int_tlast;

endmodule

`default_nettype wire
