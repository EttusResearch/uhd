//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_destaircase_pipeline
//
// Description:
//
//   Reverses the stagger introduced by cic_staircase_pipeline so that all
//   lanes are re-aligned to the same clock cycle.
//
//   cic_staircase_pipeline delays lane k by k cycles.
//   This module delays lane k by (SPC-1-k) cycles, giving every lane a
//   uniform total latency of (SPC-1) clock-enabled cycles.
//
//     out_data[0] = in_data[0] delayed SPC-1 cycles
//     out_data[k] = in_data[k] delayed SPC-1-k cycles
//     out_data[SPC-1] = in_data[SPC-1]   (wire, 0 FFs)
//
//   valid_in and valid_out are delayed according to lane 0. This is effectively
//   a delay by SPC-1 cycles.
//
// Parameters:
//   DATA_W : Bit width of each element.
//   SPC    : Number of parallel lanes (samples per clock).
//

module cic_destaircase_pipeline #(
  parameter int DATA_W = 16,
  parameter int SPC    = 8
) (
  input  logic                        clk,
  input  logic                        rst,
  input  logic                        en,
  input  logic                        valid_in,
  input  logic [SPC-1:0][DATA_W-1:0]  in_data,
  output logic                        valid_out,
  output logic [SPC-1:0][DATA_W-1:0]  out_data
);

  // --------------------------------------------------------------------
  // Valid pipeline
  // --------------------------------------------------------------------
  // valid_pipeline[d] = valid_in delayed by d clock-enabled cycles.
  // valid_pipeline[0] is a wire (no register).
  logic valid_pipeline [SPC];
  assign valid_pipeline[0] = valid_in;
  for (genvar d = 1; d < SPC; d++) begin : gen_valid_pipeline
    always_ff @(posedge clk) begin : valid_ff
      if (rst) begin
        valid_pipeline[d] <= 1'b0;
      end else if (en) begin
        valid_pipeline[d] <= valid_pipeline[d-1];
      end
    end : valid_ff
  end : gen_valid_pipeline
  assign valid_out = valid_pipeline[SPC-1];

  // --------------------------------------------------------------------
  // Data pipeline
  // --------------------------------------------------------------------
  // pipeline[k][d] = in_data[k] delayed by d clock-enabled cycles.
  // pipeline[k][0] is a wire (no register).
  logic [DATA_W-1:0] pipeline [SPC][SPC];
  for (genvar k = 0; k < SPC; k++) begin : gen_pipeline_in
    assign pipeline[k][0] = in_data[k];
  end : gen_pipeline_in

  // Lane k needs (SPC-1-k) registers; lane SPC-1 needs 0 (wire only).
  for (genvar k = 0; k < SPC-1; k++) begin : gen_pipeline_lane
    for (genvar d = 1; d <= SPC-1-k; d++) begin : gen_pipeline_depth
      always_ff @(posedge clk) begin : pipeline_ff
        if (en) begin
          pipeline[k][d] <= pipeline[k][d-1];
        end
      end : pipeline_ff
    end : gen_pipeline_depth
  end : gen_pipeline_lane

  // Output: lane k taps the d = SPC-1-k entry.
  for (genvar k = 0; k < SPC; k++) begin : gen_out
    assign out_data[k] = pipeline[k][SPC-1-k];
  end : gen_out

endmodule : cic_destaircase_pipeline
