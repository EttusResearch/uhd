//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_integrator
//
// Description:
//
//   N-th order CIC integrator for SPC parallel complex IQ samples per clock.
//   Accepts an AXI-Stream of SPC packed SAMP_W-bit samples, splits them into
//   independent I and Q components (each SAMP_W/2 bits wide), integrates each
//   through two parallel cic_integrator_core pipelines, then reassembles the
//   packed output word.
//
//   Signal chain:
//
//     data_in (SPC × SAMP_W)
//          │
//          ├──────────────────────────────┐
//          ▼                              ▼
//     [I component]                [Q component]
//     cic_integrator_core          cic_integrator_core
//     (ORDER stages)               (ORDER stages)
//          │                              │
//          └──────────────┬───────────────┘
//                         ▼
//                    output FIFO
//                         │
//                         ▼
//     data_out (SPC × SAMP_W)
//
//   Each cic_integrator_core implements ORDER integration stages using a
//   staircase input pipeline, ORDER stages of dsp48_prefix_accum adder
//   chains with running accumulators, and a destaircase output pipeline.
//
//   AXI-Stream handshake and pipeline flush:
//
//   The pipeline CE (en) is tied to fifo_in_ready so the entire data path
//   stalls in lock-step when the output FIFO is full.  valid_in is passed
//   separately to cic_integrator_core so that accumulators are frozen during
//   flush cycles (en=1, valid_in=0), preventing inter-packet corruption.
//
//   Three cases for (fifo_in_ready, data_in.tvalid):
//     (0, x) — FIFO full: pipeline frozen; data_in.tready=0 stalls upstream.
//     (1, 1) — Live data: pipeline and accumulators both advance.
//     (1, 0) — Flush/bubble: pipeline advances to drain in-flight data;
//              accumulators frozen; after CIC_LATENCY cycles the last word
//              and its tlast have traversed the pipeline.
//
// Parameters:
//
//   SAMP_W : Bit width of each sample (I+Q packed, each SAMP_W/2 bits). Set to
//            a value that ensures that the integrator stages do not overflow.
// Must be a multiple of 2.
//   SPC    : Number of samples per clock cycle (>= 1).
//   ORDER  : CIC filter order (number of integrator stages).
//

`default_nettype none

module cic_integrator #(
  parameter int SAMP_W = 32,
  parameter int SPC    = 4,
  parameter int ORDER  = 4
) (
  input  wire        clk,
  input  wire        rst,
  input  wire        clr,
  AxiStreamIf.slave  data_in,
  AxiStreamIf.master data_out
);
  // compile time check
  if (SAMP_W % 2 != 0) begin
      $error("cic_integrator: SAMP_W must be a multiple of 2");
  end

  localparam int COMP_W = SAMP_W / 2;
  // CIC_LATENCY: pipeline depth of cic_integrator_max from data_in to data_out:
  //   staircase                         : 0 cycles (valid is a wire)
  //   ORDER × (prefix_accum + spc_delay): ORDER × (2 + SPC-1) cycles
  //   final prefix_accum                : 2 cycles
  //   de-staircase                      : SPC-1 cycles
  //   Total = ORDER*(SPC+1) + SPC + 1 cycles.
  localparam int CIC_LATENCY = ORDER * (SPC + 2-1) + SPC + 1;
  // OUT_FIFO_SIZE: The pipeline is gated by fifo_in_ready so it stalls in
  // lock-step with data_in.tready; the FIFO only needs to absorb the one
  // word in transit between the pipeline output register and the FIFO input.
  localparam int OUT_FIFO_SIZE = 1;

  //--------------------------------------------------------------------------
  // I/Q split
  //
  //   lane k word: data_in.tdata[SAMP_W*k +: SAMP_W]
  //   Q component: bits [COMP_W-1:0]
  //   I component: bits [SAMP_W-1:COMP_W]
  //--------------------------------------------------------------------------
  logic [SPC-1:0][COMP_W-1:0] in_i;
  logic [SPC-1:0][COMP_W-1:0] in_q;

  for (genvar k = 0; k < SPC; k++) begin : gen_iq_split
    assign in_q[k] = data_in.tdata[SAMP_W*k        +: COMP_W];
    assign in_i[k] = data_in.tdata[SAMP_W*k+COMP_W +: COMP_W];
  end : gen_iq_split

  //--------------------------------------------------------------------------
  // Parallel I and Q integrator branches (cic_integrator_max)
  //
  //   en        = fifo_in_ready — pipeline CE; keeps draining on flush cycles
  //               so tlast is pushed through to the FIFO.
  //   valid_in  = data_in.tvalid — gates accumulator updates internally;
  //               flush cycles (en=1, valid_in=0) leave accumulators frozen.
  //   valid_out = pipeline-fill indicator; drives FIFO i_tvalid.
  //--------------------------------------------------------------------------
  logic [SPC-1:0][COMP_W-1:0] out_i;
  logic [SPC-1:0][COMP_W-1:0] out_q;
  logic                        fifo_in_ready;  // pipeline CE and FIFO write-side ready
  logic                        valid_out_i;    // CIC pipeline valid output (I branch)

  cic_integrator_core #(
    .DATA_W (COMP_W),
    .SPC    (SPC),
    .ORDER  (ORDER)
  ) branch_i (
    .clk      (clk),
    .rst      (rst || clr),
    .en       (fifo_in_ready),
    .valid_in (data_in.tvalid),
    .in_data  (in_i),
    .valid_out(valid_out_i),
    .out_data (out_i)
  );

  cic_integrator_core #(
    .DATA_W (COMP_W),
    .SPC    (SPC),
    .ORDER  (ORDER)
  ) branch_q (
    .clk      (clk),
    .rst      (rst || clr),
    .en       (fifo_in_ready),
    .valid_in (data_in.tvalid),
    .in_data  (in_q),
    .valid_out(),
    .out_data (out_q)
  );

  //--------------------------------------------------------------------------
  // tlast delay pipeline — CIC_LATENCY deep so tlast arrives at the FIFO in
  // the same cycle as the last output word.
  //
  //   CE = fifo_in_ready — identical to the data pipeline CE.
  //--------------------------------------------------------------------------
  logic [CIC_LATENCY-1:0] last_pipe;

  always_ff @(posedge clk) begin
    if (rst || clr) begin
      last_pipe <= '0;
    end else if (fifo_in_ready) begin
      last_pipe <= {last_pipe[CIC_LATENCY-2:0], data_in.tlast & data_in.tvalid};
    end
  end

  //--------------------------------------------------------------------------
  // Output pack: repack I/Q components into per-lane tdata words
  //--------------------------------------------------------------------------
  logic [SAMP_W*SPC-1:0] out_tdata;

  for (genvar k = 0; k < SPC; k++) begin : gen_output_pack
    assign out_tdata[SAMP_W*k        +: COMP_W] = out_q[k];
    assign out_tdata[SAMP_W*k+COMP_W +: COMP_W] = out_i[k];
  end : gen_output_pack

  //--------------------------------------------------------------------------
  // Output FIFO
  //
  // Decouples data_in.tready from data_out.tready.
  //--------------------------------------------------------------------------
  logic [SAMP_W*SPC:0] fifo_out_packed;

  axi_fifo #(
    .WIDTH(SAMP_W * SPC + 1),
    .SIZE (OUT_FIFO_SIZE)
  ) output_fifo (
    .clk      (clk),
    .reset    (rst || clr),
    .clear    (rst || clr),
    .i_tdata  ({last_pipe[CIC_LATENCY-1], out_tdata}),
    .i_tvalid (valid_out_i),
    .i_tready (fifo_in_ready),
    .o_tdata  (fifo_out_packed),
    .o_tvalid (data_out.tvalid),
    .o_tready (data_out.tready),
    .space    (),
    .occupied ()
  );

  assign data_out.tdata = fifo_out_packed[SAMP_W*SPC-1:0];
  assign data_out.tlast = fifo_out_packed[SAMP_W*SPC];

  //--------------------------------------------------------------------------
  // Input ready: accept when the FIFO has space.  The pipeline stalls in
  // lock-step so there is no in-flight overshoot to worry about.
  //--------------------------------------------------------------------------
  assign data_in.tready = fifo_in_ready;

endmodule : cic_integrator

`default_nettype wire
