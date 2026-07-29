//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_integrator_core
//
// Description:
//
//   N-th order CIC integrator core for SPC parallel real-valued samples per
//   clock.  Used by cic_integrator as the per-component (I or Q) integration
//   engine.
//
//   Signal chain:
//
//     in_data[SPC]
//          │
//          ▼
//     cic_staircase_pipeline
//       Lane k is delayed by k clock cycles to stagger the prefix-sum input.
//          │
//          ▼ dsp_chain[0]
//     ┌──────────────────────────────────────────────────────────────────────┐
//     │  For i = 0 .. ORDER-1:                                               │
//     │                                                                      │
//     │  dsp48_prefix_accum (USE_PCIN=1)                                     │
//     │    Computes the i-th order running prefix sum across all SPC lanes   │
//     │    using a DSP48 PCOUT→PCIN cascade; simultaneously accumulates the  │
//     │    cross-lane total into accu_out[i].  accu_in = 0 for i=0,          │
//     │    accu_out[i-1] for i > 0.                                          │
//     │         │                                                            │
//     │  spc_delay (DELAY = SPC-1)                                           │
//     │    Delays the prefix-sum output by SPC-1 cycles so that accu_out[i] │
//     │    settles before it is consumed as accu_in by the next stage.       │
//     │         │                                                            │
//     └──────────────────────────────────────────────────────────────────────┘
//          │
//          ▼ dsp_chain[2*ORDER]
//     dsp48_prefix_accum (USE_PCIN=0, final stage)
//       Computes the ORDER-th order prefix sum; accumulator output unused.
//          │
//          ▼ dsp_chain[2*ORDER+1]
//     cic_destaircase_pipeline
//       Re-aligns all SPC lanes to the same clock cycle.
//          │
//          ▼ out_data[SPC]
//
//   All lanes are processed at the full input clock rate.  Inputs are
//   sign-extended to a fixed internal precision of INTERNAL_W=48 bits;
//   outputs are truncated back to DATA_W bits.
//
// Parameters:
//
//   DATA_W : Bit width of each input/output lane element.
//   SPC    : Number of parallel lanes (samples per clock, power of 2 >= 1).
//   ORDER  : CIC filter order (number of integration stages).
//

module cic_integrator_core #(
  int DATA_W = 16,
  int SPC    = 8,
  int ORDER  = 4
) (
  input  logic                       clk,
  input  logic                       rst,
  input  logic                       en,

  input  logic                       valid_in,
  input  logic [SPC-1:0][DATA_W-1:0] in_data,

  output logic                       valid_out,
  output logic [SPC-1:0][DATA_W-1:0] out_data
);
  // Internal pipeline width: always 48 bits (full DSP48 precision).
  // Inputs are sign-extended to INTERNAL_W; outputs are truncated back to DATA_W.
  localparam int INTERNAL_W = 48;

  if (DATA_W > INTERNAL_W) begin
    $error("cic_integrator_core: DATA_W (%0d) must be <= INTERNAL_W (%0d)", DATA_W, INTERNAL_W);
  end

  //--------------------------------------------------------------------------
  // Input sign-extension: widen each lane from DATA_W to INTERNAL_W bits
  //--------------------------------------------------------------------------
  logic [SPC-1:0][INTERNAL_W-1:0] in_data_ext;
  for (genvar k = 0; k < SPC; k++) begin : gen_sign_ext
    assign in_data_ext[k] = INTERNAL_W'(signed'(in_data[k]));
  end : gen_sign_ext

  //--------------------------------------------------------------------------
  // Staircase pipeline: lane k carries in_data[k] delayed by k cycles
  //--------------------------------------------------------------------------
  logic [SPC-1:0][INTERNAL_W-1:0] dsp_chain [2*ORDER+2];
  logic [2*ORDER+1:0] valid_chain;

  cic_staircase_pipeline #(
    .DATA_W (INTERNAL_W),
    .SPC    (SPC)
  ) staircase_i (
    .clk      (clk),
    .en       (en),
    .valid_in (valid_in),
    .in_data  (in_data_ext),
    .valid_out(valid_chain[0]),
    .out_data (dsp_chain[0])
  );

  //--------------------------------------------------------------------------
  // Prefix-sum adder chain + accumulator.
  //   dsp_chain[2*i]   → data_in port of prefix_accum stage i
  //   PCOUT→PCIN cascade wired inside dsp48_prefix_accum across all SPC lanes
  //   accu_out[i]      → running cross-lane accumulator for order i
  //--------------------------------------------------------------------------
  logic [INTERNAL_W-1:0]          accu_out [ORDER];

  for (genvar i = 0; i < ORDER; i++) begin : gen_dsp_chain
    dsp48_prefix_accum #(
      .DATA_W (INTERNAL_W),
      .SPC    (SPC),
      .USE_PCIN(1)
    ) prefix_accum_i (
      .clk      (clk),
      .rst      (rst),
      .en       (en),
      .data_in  (dsp_chain[2*i]),
      .accu_in  ((i == 0) ? '0 : accu_out[i-1]),
      .valid_in (valid_chain[2*i]),
      .data_out (dsp_chain[2*i+1]),
      .accu_out (accu_out[i]),
      .valid_out(valid_chain[2*i+1])
    );

    // Delay {data, valid} by SPC-1 cycles to allow accu_out[i] to settle
    if (SPC > 1) begin : gen_delay
      logic [SPC*INTERNAL_W:0] shift_reg [SPC-1];
      always_ff @(posedge clk) begin
        if (en) begin
          shift_reg[0] <= {dsp_chain[2*i+1], valid_chain[2*i+1]};
          for (int d = 1; d < SPC-1; d++) begin
            shift_reg[d] <= shift_reg[d-1];
          end
        end
        // reset valid chain only
        if (rst) begin
           for (int d = 0; d < SPC-1; d++) begin
             shift_reg[d][0] <= '0;
           end
        end
      end
      assign {dsp_chain[2*i+2], valid_chain[2*i+2]} = shift_reg[SPC-2];
    end else begin : gen_passthrough
      assign dsp_chain[2*i+2]   = dsp_chain[2*i+1];
      assign valid_chain[2*i+2] = valid_chain[2*i+1];
    end
  end : gen_dsp_chain

  // Final prefix-sum stage: ORDER-th order summation; accumulator output unused.
  dsp48_prefix_accum #(
      .DATA_W (INTERNAL_W),
      .SPC    (SPC),
      .USE_PCIN(0)
    ) prefix_accum_i (
      .clk      (clk),
      .rst      (rst),
      .en       (en),
      .data_in  (dsp_chain[2*ORDER]),
      .accu_in  (accu_out[ORDER-1]),
      .valid_in (valid_chain[2*ORDER]),
      .data_out (dsp_chain[2*ORDER+1]),
      .accu_out (),
      .valid_out(valid_chain[2*ORDER+1])
    );

  //--------------------------------------------------------------------------
  // De-staircase: re-align all lanes to the same clock cycle.
  //--------------------------------------------------------------------------
  logic [SPC-1:0][INTERNAL_W-1:0] out_data_int;

  cic_destaircase_pipeline #(
    .DATA_W (INTERNAL_W),
    .SPC    (SPC)
  ) destaircase_i (
    .clk      (clk),
    .rst      (rst),
    .en       (en),
    .in_data  (dsp_chain[2*ORDER+1]),
    .valid_in (valid_chain[2*ORDER+1]),
    .valid_out(valid_out),
    .out_data (out_data_int)
  );

  //--------------------------------------------------------------------------
   // Output truncation: keep the least-significant DATA_W bits of each lane
   // (wrap-around / modulo arithmetic) and discard the upper
   // (INTERNAL_W-DATA_W) bits.
  //--------------------------------------------------------------------------
  for (genvar k = 0; k < SPC; k++) begin : gen_trunc
    assign out_data[k] = out_data_int[k][DATA_W-1:0];
  end : gen_trunc

endmodule : cic_integrator_core
