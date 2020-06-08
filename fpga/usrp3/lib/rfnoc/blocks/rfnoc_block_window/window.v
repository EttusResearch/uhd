//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Description: 
//
//   Windowing module. Multiplies each input packet by the coefficients loaded
//   into an internal memory.
//
// Parameters:
//
//   WINDOW_SIZE : Sets the maximum window size to 2^WINDOW_SIZE samples.
//   COEFF_WIDTH : The bit width of coefficients.
//

module window #(
  parameter WINDOW_SIZE = 10,
  parameter COEFF_WIDTH = 16
) (
  input wire clk,
  input wire rst,

  // Window size to use
  input wire [WINDOW_SIZE:0] window_size,

  // Coefficient input
  input  wire [COEFF_WIDTH-1:0] m_axis_coeff_tdata,
  input  wire                   m_axis_coeff_tlast,
  input  wire                   m_axis_coeff_tvalid,
  output wire                   m_axis_coeff_tready,

  // Input data stream
  input  wire [31:0] i_tdata,
  input  wire        i_tlast,
  input  wire        i_tvalid,
  output wire        i_tready,

  // Output data stream
  output wire [31:0] o_tdata,
  output wire        o_tlast,
  output wire        o_tvalid,
  input  wire        o_tready
);

  // Sample data stream (from i_tdata)
  wire [           31:0] stream_tdata;
  wire                   stream_tlast;
  wire                   stream_tvalid;
  wire                   stream_tready;
  // RAM output data stream (saved coefficients)
  wire [           15:0] ram_tdata;
  wire                   ram_tvalid;
  wire                   ram_tready;
  // Counter data stream (RAM address for coefficient lookup)
  wire [WINDOW_SIZE-1:0] count_tdata;
  wire                   count_tvalid;
  wire                   count_tready;
  // Flow control stream. This keeps the counter stream in sync with the
  // sample data stream.
  wire                   flow_tvalid;
  wire                   flow_tready;

  wire clear_counter;

  // Restart the address counter whenever we load a new set of coefficients.
  assign clear_counter = m_axis_coeff_tlast & 
                         m_axis_coeff_tvalid &
                         m_axis_coeff_tready;

  // Split the incoming data stream into two
  split_stream_fifo #(
    .WIDTH       (32),
    .ACTIVE_MASK (4'b0011)
  ) split_stream_fifo_i (
    .clk       (clk),
    .reset     (rst),
    .clear     (1'b0),
    .i_tdata   (i_tdata),
    .i_tlast   (i_tlast),
    .i_tvalid  (i_tvalid),
    .i_tready  (i_tready),
    .o0_tdata  (stream_tdata),
    .o0_tlast  (stream_tlast),
    .o0_tvalid (stream_tvalid),
    .o0_tready (stream_tready),
    .o1_tdata  (),
    .o1_tlast  (),
    .o1_tvalid (flow_tvalid),
    .o1_tready (flow_tready),
    .o2_tdata  (),
    .o2_tlast  (),
    .o2_tvalid (),
    .o2_tready (1'b0),
    .o3_tdata  (),
    .o3_tlast  (),
    .o3_tvalid (),
    .o3_tready (1'b0)
  );

  // Address generation
  counter #(
    .WIDTH (WINDOW_SIZE)
  ) counter_i (
    .clk      (clk),
    .reset    (rst),
    .clear    (clear_counter),
    .max      (window_size),
    .i_tlast  (1'b0),
    .i_tvalid (flow_tvalid),
    .i_tready (flow_tready),
    .o_tdata  (count_tdata),
    .o_tlast  (),
    .o_tvalid (count_tvalid),
    .o_tready (count_tready)
  );

  // RAM to store window coefficients
  ram_to_fifo #(
    .DWIDTH (COEFF_WIDTH),
    .AWIDTH (WINDOW_SIZE)
  ) ram_to_fifo_i (
    .clk           (clk),
    .reset         (rst),
    .clear         (1'b0),
    .config_tdata  (m_axis_coeff_tdata),
    .config_tlast  (m_axis_coeff_tlast),
    .config_tvalid (m_axis_coeff_tvalid),
    .config_tready (m_axis_coeff_tready),
    .i_tdata       (count_tdata),
    .i_tlast       (1'b0),
    .i_tvalid      (count_tvalid),
    .i_tready      (count_tready),
    .o_tdata       (ram_tdata),
    .o_tlast       (),
    .o_tvalid      (ram_tvalid),
    .o_tready      (ram_tready)
  );

  // Real by complex multiplier. This multiplier is configured for signed
  // fixed point with 15 fractional bits. For N fractional bits (a right-shift
  // by N in the multiplication result), set:
  // DROP_TOP_P = (WIDTH_REAL + WIDTH_CPLX - WIDTH_P + 5) - N
  mult_rc #(
    .WIDTH_REAL (COEFF_WIDTH),
    .WIDTH_CPLX (16),
    .WIDTH_P    (16),
    .DROP_TOP_P (6)
  ) mult_rc_i (
    .clk         (clk),
    .reset       (rst),
    .real_tdata  (ram_tdata),
    .real_tlast  (1'b0),
    .real_tvalid (ram_tvalid),
    .real_tready (ram_tready),
    .cplx_tdata  (stream_tdata),
    .cplx_tlast  (stream_tlast),
    .cplx_tvalid (stream_tvalid),
    .cplx_tready (stream_tready),
    .p_tdata     (o_tdata),
    .p_tlast     (o_tlast),
    .p_tvalid    (o_tvalid),
    .p_tready    (o_tready)
  );

endmodule
