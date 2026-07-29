//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_staircase_pipeline
//
// Description:
//
//   Produces a staircase-delayed version of the SPC-wide input data vector.
//
//   Lane k of out_data carries in_data[k] delayed by exactly k clock-enabled
//   cycles:
//     out_data[0] = in_data[0]                  (wire, 0 FFs)
//     out_data[1] = in_data[1] delayed 1 cycle  (1 FF)
//     out_data[k] = in_data[k] delayed k cycles (k FFs)
//
//   This stagger is required by the prefix-sum column stage so that c_in[k]
//   arrives in the same cycle that p_reg[k-1] holds the previous lane's
//   registered result — ensuring each column adds samples from the same batch.
//
// Parameters:
//   DATA_W : Bit width of each element.
//   SPC    : Number of parallel lanes (samples per clock).
//

module cic_staircase_pipeline #(
  parameter int DATA_W = 16,
  parameter int SPC    = 8
) (
  input  logic                       clk,
  input  logic                       en,

  input  logic [SPC-1:0][DATA_W-1:0] in_data,
  input  logic                       valid_in,

  output logic [SPC-1:0][DATA_W-1:0] out_data,
  output logic                       valid_out
);
  // Internal shift-register array.
  // pipeline[k][d] = in_data[k] delayed by d clock-enabled cycles.
  // pipeline[k][0] is a wire (no register).
  logic [DATA_W-1:0] pipeline [SPC][SPC];

  for (genvar k = 0; k < SPC; k++) begin : gen_pipeline_in
    assign pipeline[k][0] = in_data[k];
  end : gen_pipeline_in

  for (genvar k = 1; k < SPC; k++) begin : gen_pipeline_lane
    for (genvar d = 1; d <= k; d++) begin : gen_pipeline_depth
      always_ff @(posedge clk) begin : pipeline_ff
        if (en) pipeline[k][d] <= pipeline[k][d-1];
      end : pipeline_ff
    end : gen_pipeline_depth
  end : gen_pipeline_lane

  // Output: lane k taps the d=k entry
  for (genvar k = 0; k < SPC; k++) begin : gen_out
    assign out_data[k] = pipeline[k][k];
  end : gen_out

  // lane 0 is a wire, so valid_out is just a copy of valid_in
  assign valid_out = valid_in;

endmodule : cic_staircase_pipeline
