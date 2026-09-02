//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_filter_decim
//
// Description:
//
//   CIC decimation filter for SPC complex samples per clock cycle. The datapath
//   implements the classic Hogenauer topology and adds rate-dependent gain
//   normalization and an output FIFO:
//
//                            data_in
//                       (SPC x SAMP_W)
//                               │   tready
//                               v<─────────────────────────────────┐
//               ┌───────────────────────────────┐                  │
//               │      I/Q sign extension       │                  │
//               │       SAMP_W -> ACCUM_W       │                  │
//               └──────────────┬────────────────┘                  │
//                              │                                   │
//               ┌──────────────┼────────────────┐                  │
//               │  ┌────────────────────────┐   │                  │
//               │  │      Integrator 0      │   │                  │
//               │  └───────────┬────────────┘   │                  │
//               │              │                │ ORDER stages     │
//               │             ...               │ (full input rate)│
//               │              │                │                  │
//               │  ┌────────────────────────┐   │                  │
//               │  │   Integrator ORDER-1   │   │                  │
//               │  └───────────┬────────────┘   │                  │
//               └──────────────┼────────────────┘                  │
//                              │                                   │
//                  ┌────────────────────────┐                      │
//    decim_factor ─┤    Decimator (÷R)      │                      │
//                  └───────────┬────────────┘                      │
//                              │                                   │
//               ┌──────────────┼────────────────┐                  │
//               │  ┌────────────────────────┐   │                  │
//               │  │        Comb 0          │   │                  │
//               │  └───────────┬────────────┘   │                  │
//               │              │                │ ORDER stages     │
//               │             ...               │ (decimated rate) │
//               │              │                │                  │
//               │  ┌────────────────────────┐   │                  │
//               │  │     Comb ORDER-1       │   │                  │
//               │  └───────────┬────────────┘   │                  │
//               └──────────────┼────────────────┘                  │
//                              │                                   │
//               ┌───────────────────────────────┐                  │
//               │      Normalize and round      │                  │
//               │       ACCUM_W -> SAMP_W       │                  │
//               └──────────────┬────────────────┘                  │
//                              │                                   │
//               ┌───────────────────────────────┐   fifo_space     │
//               │          Output FIFO          │──────────────────┘
//               └──────────────┬────────────────┘
//                              │
//                              v
//                            data_out
//                       (SPC x SAMP_W)
//
//   Each input word contains SPC packed complex samples. The module sign-extends
//   each I and Q component to ACCUM_W, which includes the maximum CIC bit growth.
//   The ORDER integrator stages process every input sample. For decimation factor
//   R, the decimator consumes R input words and constructs one SPC-sample output
//   word by selecting every R-th integrated sample. The ORDER comb stages then
//   apply y[n] = x[n] - x[n-1] at the decimated rate.
//
//   The barrel shifter normalizes the CIC gain by right-shifting each accumulator
//   component by ceil(log2(R**ORDER)). It rounds each result to SAMP_W/2 bits
//   before packing the SPC output samples.
//
//   Downstream backpressure stops at the output FIFO. A registered credit gate
//   admits input words only while the FIFO has room for every possible output
//   already in flight, using the worst-case R=1 rate. Once accepted, a word can
//   traverse the datapath without stalling on data_out.tready. If the reserved
//   capacity runs low, data_in.tready deasserts until FIFO space becomes available.
//
//   The decimation factor is driven directly on decim_factor. It must be valid
//   before reset is released and remain stable while data is actively
//   processed.
//
//   NOTE: This module is expected to run within axi_rate_change, which ensures
//   that each packet contains a multiple of R input words and that output tlast
//   signals are correctly aligned.
//
// Parameters:
//
//   SPC       : Number of samples per clock cycle (must be power of 2, >= 1).
//   SAMP_W    : External sample width (I + Q packed, each SAMP_W/2 bits).
//   MAX_DECIM : Maximum decimation factor supported. Sizes internal registers.
//   ORDER     : CIC filter order (number of integrator and comb stages).
//
// Signals:
//   decim_factor   : Runtime-configurable decimation ratio (1..MAX_DECIM).
//

`default_nettype none

module cic_filter_decim #(
    parameter int SPC       = 4,
    parameter int SAMP_W    = 32,
    parameter int MAX_DECIM = 255,
    parameter int ORDER     = 4
)(
  input wire                              clk,
  input wire                              rst,
  input wire                              clear,
  input wire [$clog2(MAX_DECIM+1)-1:0]    decim_factor,
  // Data in (SPC * SAMP_W wide)
  AxiStreamIf.slave                       data_in,
  // Data out (SPC * SAMP_W wide)
  AxiStreamIf.master                      data_out
);
  //----------------------------------------------------------------------------
  // Local parameters
  //----------------------------------------------------------------------------
  localparam int MAX_DELAY = 1;  // Maximum comb delay in samples

  // Accumulator width to accommodate max CIC growth without overflow, based on the formula:
  localparam int ACCUM_W = 2 * (SAMP_W/2 + $clog2(MAX_DECIM*MAX_DELAY)*ORDER);
  localparam int COMP_W  = ACCUM_W/2; // Width of each I/Q component in the accumulator

  import cic_utils_pkg::*;

  // Reserve for the worst-case R=1 path, where every input word produces an
  // output word, plus one cycle each for the registered FIFO-space decision
  // and input_blocked state.
  localparam int DATAPATH_LATENCY = integrator_latency(SPC, ORDER)
                                  + decimator_latency()
                                  + comb_filter_latency(ORDER)
                                  + barrel_shift_latency(ACCUM_W, SAMP_W)
                                  + 2; // Additional cycles for fifo_ready and input_blocked

  // Set the output FIFO size to twice the datapath latency to allow continuous
  // downstream traffic.
  localparam int OUTPUT_FIFO_SIZE = $clog2(2 * DATAPATH_LATENCY);
  localparam int OUTPUT_FIFO_DEPTH = 1 << OUTPUT_FIFO_SIZE;

  //---------------------------------------------------------------------------
  // Internal AXI-Stream buses
  //---------------------------------------------------------------------------
  // Integrator chain: 2 buses (input [0] and output [1] of cic_integrator).
  AxiStreamIf #(SPC * ACCUM_W) integ_bus [2] (.clk(clk), .rst(rst));
  // integ_bus[i] for i > 0 carries the output of integrator stage i-1.

  // Comb chain: ORDER+1 buses connecting comb filter inputs/outputs.
  // comb_bus[0] is driven by the decimator output.
  // comb_bus[i] for i > 0 carries the output of comb stage i-1.
  AxiStreamIf #(SPC * ACCUM_W) comb_bus [ORDER+1] (.clk(clk), .rst(rst));

  // The output of the barrel shift stage after the bus width is reduced back
  // to SAMP_W bits from ACCUM_W bits.
  AxiStreamIf #(SPC * SAMP_W) barrel_shift_out (.clk(clk), .rst(rst));


  //--------------------------------------------------------------------------
  // Sign extension to ACCUM_W bits for the input samples
  //
  // Each sample in data_in is SAMP_W bits wide ({I[SAMP_W/2-1:0], Q[SAMP_W/2-1:0]}).
  // Sign-extend each I and Q component independently from SAMP_W/2 to COMP_W
  // bits, then pack into integ_bus[0].tdata as ACCUM_W-wide samples.
  //--------------------------------------------------------------------------

  logic fifo_ready;
  logic input_blocked;

  // Hold the input blocked until FIFO credit is restored.
  always_ff @(posedge clk) begin
    if (rst || clear) begin
      input_blocked <= 1'b0;
    end else begin
      if (fifo_ready) begin
        input_blocked <= 1'b0;
      end else if (integ_bus[0].tvalid && integ_bus[0].tready) begin
        input_blocked <= 1'b1;
      end
    end
  end

  always_comb begin
    integ_bus[0].tvalid = data_in.tvalid & ~input_blocked;
    integ_bus[0].tlast  = data_in.tlast;
    data_in.tready      = integ_bus[0].tready & ~input_blocked;
  end

  for (genvar samp_idx = 0; samp_idx < SPC; samp_idx++) begin : gen_sign_ext
    // Q component (lower half of each sample)
    sign_extend #(
      .bits_in (SAMP_W/2),
      .bits_out(COMP_W)
    ) sign_ext_q (
      .in (data_in.tdata[SAMP_W*samp_idx +: SAMP_W/2]),
      .out(integ_bus[0].tdata[ACCUM_W*samp_idx +: COMP_W])
    );
    // I component (upper half of each sample)
    sign_extend #(
      .bits_in (SAMP_W/2),
      .bits_out(COMP_W)
    ) sign_ext_i (
      .in (data_in.tdata[SAMP_W*samp_idx + SAMP_W/2 +: SAMP_W/2]),
      .out(integ_bus[0].tdata[ACCUM_W*samp_idx + COMP_W +: COMP_W])
    );
  end : gen_sign_ext

  //---------------------------------------------------------------------------
  // Integrator chain: N-th order, ORDER stages at full input rate
  //---------------------------------------------------------------------------
  cic_integrator #(
    .SAMP_W(ACCUM_W),
    .SPC   (SPC),
    .ORDER (ORDER)
  ) cic_integrator_x (
    .clk     (clk),
    .rst     (rst),
    .clr     (clear),
    .data_in (integ_bus[0]),
    .data_out(integ_bus[1])
  );

  //---------------------------------------------------------------------------
  // Decimator: reduces sample rate by decim_factor
  //---------------------------------------------------------------------------
  cic_decimator #(
    .SAMP_W        (ACCUM_W),
    .SPC           (SPC),
    .R_MAX         (MAX_DECIM)
  ) cic_decimator_i (
    .clk           (clk),
    .rst           (rst),
    .clr           (clear),
    .data_in       (integ_bus[1]),
    .data_out      (comb_bus[0]),
    .decim_factor  (decim_factor)
  );

  //---------------------------------------------------------------------------
  // Comb chain: ORDER stages at decimated rate
  //---------------------------------------------------------------------------
  for (genvar stage = 0; stage < ORDER; stage++) begin : gen_combs
    cic_comb_filter #(
      .ACCUM_W  (ACCUM_W),
      .SPC      (SPC),
      .FIFO_SIZE((stage == ORDER-1)? 1 : 0)
    ) cic_comb_filter_i (
      .clk     (clk),
      .rst     (rst),
      .clr     (clear),
      .data_in (comb_bus[stage]),
      .data_out(comb_bus[stage+1])
    );
  end

  // Barrel-shift each I/Q component and extract SAMP_W/2 output bits.
  cic_barrel_shift #(
    .SPC       (SPC),
    .MAX_RATE  (MAX_DECIM),
    .ORDER     (ORDER)
  ) cic_barrel_shift_i (
    .clk         (clk),
    .rst         (rst),
    .clear       (clear),
    .rate_factor (decim_factor),
    .data_in     (comb_bus[ORDER]),
    .data_out    (barrel_shift_out)
  );

  // Downstream backpressure terminates at the output FIFO. The credit gate at
  // data_in reserves enough room for every word already in the datapath.
  assign barrel_shift_out.tready = 1'b1;

  localparam FIFO_WIDTH = data_out.DATA_WIDTH + 1;
  logic [15:0] fifo_space;
  logic        output_fifo_i_tready;
  logic [FIFO_WIDTH-1:0] fifo_data_in;
  logic [FIFO_WIDTH-1:0] fifo_data_out;

  assign fifo_data_in = {barrel_shift_out.tlast, barrel_shift_out.tdata};
  assign {data_out.tlast, data_out.tdata} = fifo_data_out;

  always_ff @(posedge clk) begin
    if (rst || clear) begin
      fifo_ready <= 1'b0;
    end else begin
      fifo_ready <= (fifo_space > DATAPATH_LATENCY);
    end
  end

  axi_fifo #(
    .WIDTH(FIFO_WIDTH),
    .SIZE (OUTPUT_FIFO_SIZE)
  ) output_fifo (
    .clk      (clk),
    .reset    (rst),
    .clear    (clear),
    .i_tdata  (fifo_data_in),
    .i_tvalid (barrel_shift_out.tvalid),
    .i_tready (output_fifo_i_tready),
    .o_tdata  (fifo_data_out),
    .o_tvalid (data_out.tvalid),
    .o_tready (data_out.tready),
    .space    (fifo_space),
    .occupied ()
  );

  // synthesis translate_off
  assert property (
    @(posedge clk) disable iff (rst || clear)
      barrel_shift_out.tvalid |-> output_fifo_i_tready
  ) else
    $error("cic_filter_decim: output FIFO not ready for valid data");
  // synthesis translate_on

endmodule : cic_filter_decim

`default_nettype wire
