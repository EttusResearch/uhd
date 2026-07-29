//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: dsp48_prefix_accum
//
// Description:
//
//   SPC-wide DSP48E1 prefix-sum adder chain followed by a DSP48E1 accumulator.
//
//   Signal chain:
//
//     data_in[SPC]  (C ports, one per lane)
//     accu_in       (AB port at stage 0; cascaded via ACOUT/BCOUT to stages 1..SPC-1)
//          │
//          ▼
//     dsp48_add3[0]  : P = accu_in + C[0]              (PCIN=0,       AB=accu_in)
//          │ PCOUT / ACOUT / BCOUT
//     dsp48_add3[1]  : P = PCIN + accu_in + C[1]       (PCIN=P[0],    AB=cascade)
//          │ PCOUT / ACOUT / BCOUT
//          ...
//     dsp48_add3[SPC-1]: P = PCIN + accu_in + C[SPC-1] (PCIN=P[SPC-2], AB=cascade)
//          │ PCOUT
//     dsp48_accum    : accu <= accu + PCIN + 0
//
//   data_out[k] = registered P output of adder stage k
//   accu_out    = running accumulator (sum of all prefix-sum tails)
//
//   Valid signals are aligned with the sample in lane 0 on input and output.
//
//   All stages use explicit DSP48E1 primitives with hard-wired
//   PCOUT→PCIN cascade so no synthesis inference is required.
//
// Parameters:
//   DATA_W  : Bit width of each lane (default 48, matching DSP48 P width).
//             There are only addition operations. Internally 48 bits are used for
//             calculations due to DSP48 primitives. The output is just truncated back to
//             DATA_W bits. In context of the CIC filter DATA_W has to be set to a value
//             that is large enough to avoid overflow in the integrator stages.
//   SPC     : Number of parallel lanes.
//   USE_PCIN: Include PCIN in the sum (1 = yes, 0 = no).
//

module dsp48_prefix_accum #(
  int DATA_W   = 48,
  int SPC      = 8,
  bit USE_PCIN = 1
) (
  input  logic                        clk,
  input  logic                        rst,
  input  logic                        en,

  input  logic [SPC-1:0][DATA_W-1:0]  data_in,
  input  logic [DATA_W-1:0]           accu_in,
  input  logic                        valid_in,

  output logic [SPC-1:0][DATA_W-1:0]  data_out,
  output logic [DATA_W-1:0]           accu_out,
  output logic                        valid_out
);

  // Shift register for delay signal.
  // There are two receivers for the valid signal from different delay stages,
  // the valid_out and the accumulator at the end of the chain.
  localparam int MAX_DELAY = (SPC > 1) ? SPC : 2;
  logic [MAX_DELAY:0] valid_chain;
  assign valid_chain[0] = valid_in;
  for (genvar k = 1; k < $bits(valid_chain); k++) begin : gen_valid_chain
    always_ff @(posedge clk) begin
      if (rst) begin
        valid_chain[k] <= 1'b0;
      end else if (en) begin
        valid_chain[k] <= valid_chain[k-1];
      end
    end
  end
  // Sample 0 takes 2 clock cycles (C and P reg) for processing through the first DSP48E1
  // stage, so valid_out is delayed by 2 cycles.
  assign valid_out = valid_chain[2];


  logic [47:0] pcout_cas  [SPC];
  logic [47:0] abcout_cas [SPC];

  for (genvar k = 0; k < SPC; k++) begin : gen_dsp
    if (k == 0) begin : gen_first
      dsp48_add3 #(.AB_CASCADE(0), .USE_PCIN(0)) dsp_inst (
        .clk    (clk),
        .rst    (rst),
        .en     (en),
        .ab     (accu_in),
        .c      (data_in[0]),
        .pcin   (48'b0),
        .pcout  (pcout_cas[0]),
        .abcout (abcout_cas[0]),
        .p      (data_out[0])
      );
    end else begin : gen_chain
      dsp48_add3 #(.AB_CASCADE(1), .USE_PCIN(USE_PCIN)) dsp_inst (
        .clk    (clk),
        .rst    (rst),
        .en     (en),
        .ab     (abcout_cas[k-1]),
        .c      (data_in[k]),
        .pcin   (pcout_cas[k-1]),
        .pcout  (pcout_cas[k]),
        .abcout (abcout_cas[k]),
        .p      (data_out[k])
      );
    end : gen_chain
  end : gen_dsp

  //--------------------------------------------------------------------------
  // Accumulator: P <= P + PCIN  (C = 0)
  //--------------------------------------------------------------------------
  dsp48_accum accu_inst (
    .clk   (clk),
    .rst   (rst),
    .en    (en),
    .valid (valid_chain[SPC]),
    .c     (48'b0),
    .pcin  (pcout_cas[SPC-1]),
    .pcout (),
    .p     (accu_out)
  );

endmodule : dsp48_prefix_accum
