//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_comb_filter
//
// Description:
//
//   A CIC comb filter stage that processes multiple samples per clock cycle.
//   Implements the difference equation y[n] = x[n] - x[n-1] with a fixed
//   delay of 1 sample.
//
//   With D=1 the delayed sample x[n-1] is always either:
//     - lane (lane_idx-1) of the current input word (for lanes 1..SPC-1), or
//     - lane (SPC-1) of the previous input word (for lane 0).
//
//   Only a single register (prev_last_sample) is needed to hold the last
//   sample of the previous word.
//
//   I and Q components within each sample (each COMP_W bits wide) are
//   subtracted independently to prevent borrow propagation across the
//   half-word boundary.
//
//   Pipeline depth is 1 clock cycle (subtraction registered on input
//   transfer). The output is buffered through a single-stage axi_fifo to
//   break the combinational backpressure path.
//
//   data_in ------+-------------------------+
//   (AXI-S)       |                         |
//                 v                         v
//      +-------------------+  +----------------------------+
//      | prev_last_sample  |  |    Subtraction Stage       |
//      |  (registered)     +->| Lane 0  : x[0] - prev      |
//      +-------------------+  | Lane idx: x[idx] - x[idx-1]|
//                             | (I/Q subtracted sep.)      |
//                             +-------------+--------------+
//                                           |
//                                           v
//                             +-------------+-------------+
//                             |   output_buffer (flop)    |
//                             +-------------+-------------+
//                                           |
//                                           v
//                                      data_out
//                                      (AXI-S)
//
//
// Parameters:
//    ACCUM_W       : Bit width of each input/output sample, accounting for
//                    any register growth of the CIC filter this module is
//                    used in. Must have at least one bit of headroom
//                    compared to the  actual input Sample width to
//                    prevent overflow in the subtraction within this module.
//
//    SPC           : Number of samples processed per clock cycle. Must be a
//                    power of 2 and at least 1.
//


`default_nettype none

module cic_comb_filter #(
  parameter int ACCUM_W        = 96,
  parameter int SPC            = 4
) (
  input wire              clk,
  input wire              rst,
  // Clear comb filter (resets previous-sample register)
  input wire              clr,
  // Data in
  AxiStreamIf.slave       data_in,
  // Data out
  AxiStreamIf.master      data_out
);

  import cic_utils_pkg::*;

  //---------------------------------------------------------------------------
  // Type definitions
  //---------------------------------------------------------------------------

  typedef cic_utils#(
    .SPC   (SPC),
    .SAMP_W(ACCUM_W)
  ) util_c;

  typedef util_c::sample_t sample_t; // Single sample (ACCUM_W bits).

  //---------------------------------------------------------------------------
  // Local parameters
  //---------------------------------------------------------------------------
  // Width of each I/Q component within a accumulator-width sample.
  localparam int COMP_W = ACCUM_W / 2;

  //---------------------------------------------------------------------------
  // Synthesis-time checks
  //---------------------------------------------------------------------------

  // SPC must be a power of 2 and at least 1.
  if (SPC < 1 || (SPC & (SPC - 1)) != 0) begin : gen_check_nspc
    $error("cic_comb_filter: SPC must be a power of 2 and at least 1, got SPC=%0d", SPC);
  end

  // AXI stream interface width checks.
  if (data_in.DATA_WIDTH != ACCUM_W*SPC) begin : gen_check_data_in_width
    $error("cic_comb_filter: data_in DATA_WIDTH (%0d) must equal ACCUM_W*SPC (%0d)",
      data_in.DATA_WIDTH, ACCUM_W*SPC);
  end
  if (data_out.DATA_WIDTH != ACCUM_W*SPC) begin : gen_check_data_out_width
    $error("cic_comb_filter: data_out DATA_WIDTH (%0d) must equal ACCUM_W*SPC (%0d)",
      data_out.DATA_WIDTH, ACCUM_W*SPC);
  end

  // ACCUM_W must be even to allow separate I and Q components.
  if (ACCUM_W % 2 != 0) begin : gen_check_samp_w
    $error({"cic_comb_filter: ACCUM_W must be even to allow separate I and Q components,",
    " got ACCUM_W=%0d"}, ACCUM_W);
  end

  //---------------------------------------------------------------------------
  // Internal signal declarations
  //---------------------------------------------------------------------------

  // Input handshake
  logic input_transfer;
  assign input_transfer = data_in.tvalid & data_in.tready;

  // Previous word's last sample (lane SPC-1). For lane 0 of the current
  // word, this provides x[n-1]. Cleared to zero on reset/clear, which
  // naturally provides the zero initial condition for the first word.
  sample_t prev_last_sample;

  // Subtraction stage output (combinational, before output FIFO).
  logic [ACCUM_W*SPC-1:0] sub_tdata;

  //---------------------------------------------------------------------------
  // Previous-sample register
  //---------------------------------------------------------------------------

  always_ff @(posedge clk) begin
    if (rst || clr) begin
      prev_last_sample <= '0;
    end else if (input_transfer) begin
      prev_last_sample <= data_in.tdata[ACCUM_W*(SPC-1) +: ACCUM_W];
    end
  end

  //---------------------------------------------------------------------------
  // Per-component subtraction (combinational)
  //
  // Concatenate {data_in.tdata, prev_last_sample} to form (SPC+1) samples
  // worth of COMP_W-wide component slots. Then for each output slot k:
  //   sub_tdata[k] = shifted_data[k+2] - shifted_data[k]
  // where +2 skips one full sample (I and Q components).
  //
  // This implements y[n] = x[n] - x[n-1] with I and Q subtracted
  // independently to prevent borrow propagation across the half-word boundary.
  //---------------------------------------------------------------------------

  logic [ACCUM_W*(SPC+1)-1:0] shifted_data;

  always_comb begin
    shifted_data = {data_in.tdata, prev_last_sample};
  end

  // Iterate over all I and Q components of the input word, starting at the first
  // sample of the current input word (index 2 to skip prev_last_sample).
  for (genvar comp_idx = 0; comp_idx < SPC*2; comp_idx++) begin : gen_output_lanes
    always_comb begin
      sub_tdata[COMP_W*comp_idx +: COMP_W] =
        data_in.tdata[COMP_W*comp_idx +: COMP_W] -
        shifted_data[COMP_W*comp_idx +: COMP_W];
    end
  end : gen_output_lanes


  //---------------------------------------------------------------------------
  // Output FIFO (single stage)
  //
  // Breaks the combinational path from data_out.tready to data_in.tready.
  //---------------------------------------------------------------------------

  logic [SPC*ACCUM_W:0] fifo_out_tdata;

  axi_fifo #(
    .WIDTH(SPC * ACCUM_W + 1),
    .SIZE (1)
  ) output_buffer (
    .clk     (clk),
    .reset   (rst),
    .clear   (clr),
    .i_tdata ({data_in.tlast, sub_tdata}),
    .i_tvalid(data_in.tvalid),
    .i_tready(data_in.tready),
    .o_tdata (fifo_out_tdata),
    .o_tvalid(data_out.tvalid),
    .o_tready(data_out.tready),
    .space   (),
    .occupied()
  );

  assign data_out.tdata = fifo_out_tdata[SPC*ACCUM_W-1:0];
  assign data_out.tlast = fifo_out_tdata[SPC*ACCUM_W];

endmodule : cic_comb_filter

`default_nettype wire
