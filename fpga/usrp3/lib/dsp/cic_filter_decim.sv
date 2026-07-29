//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_filter_decim
//
// Description:
//
//   CIC decimation filter for multiple samples per clock cycle, implementing
//   the classic Hogenauer (CIC) decimation topology:
//
//                            data_in
//                               │
//                               v
//               ┌───────────────────────────────┐
//               │  ┌────────────────────────┐   │
//               │  │      Integrator 0      │   │
//               │  └───────────┬────────────┘   │
//               │              │                │
//               │             ···               │ ORDER stages
//               │              │                │ (full input rate)
//               │  ┌────────────────────────┐   │
//               │  │   Integrator ORDER-1   │   │
//               │  └───────────┬────────────┘   │
//               └──────────────┼────────────────┘
//                              │
//                  ┌────────────────────────┐
//     decim_factor─┤    Decimator (÷R)      │
//                  └───────────┬────────────┘
//                              │
//               ┌──────────────┼────────────────┐
//               │  ┌────────────────────────┐   │
//               │  │        Comb 0          │   │
//               │  └───────────┬────────────┘   │ ORDER stages
//               │              │                │ (decimated rate)
//               │             ...               │
//               │              │                │
//               │              │                │
//               │  ┌────────────────────────┐   │
//               │  │     Comb ORDER-1       │   │
//               │  └───────────┬────────────┘   │
//               └──────────────┼────────────────┘
//                              │
//                              v
//                           data_out
//
//   ORDER integrator stages run at the full input rate, accumulating partial
//   sums across SPC parallel sample lanes. The single decimator then selects
//   every R-th sample group (keeping SPC output samples per word). Finally,
//   ORDER comb stages (y[n] = x[n] - x[n-D]) operate at the decimated rate.
//
//   All stages use the same SAMP_W, which must be pre-sized to accommodate
//   the full CIC bit growth of N*log2(R_max*D_max) (no internal clipping or
//   rounding is performed).
//
//   The decimation factor may be changed at runtime by asserting
//   config_changed for one clock cycle. Configuration must only be changed
//   when the module is idle (not actively processing data).
//
//   NOTE: This module is expected to run within the context of the axi_rate_change
//   module, which ensures that the number of input words is always a multiple
//   of the decimation factor and that the output tlast signals are correctly
//   aligned.
//
// Parameters:
//
//   SPC      : Number of samples per clock cycle (must be power of 2, >= 1).
//   SAMP_W    : Bit width of each sample (I + Q packed, each SAMP_W/2 bits).
//   MAX_DECIM : Maximum decimation factor supported. Sizes internal registers.
//   ORDER     : CIC filter order (number of integrator and comb stages).
//
// Signals:
//   decim_factor   : Runtime-configurable decimation ratio (1..MAX_DECIM).
//   config_changed : Pulse high for 1 cycle after changing decim_factor.
//                    Must only be asserted when the module is idle.
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
  input wire                              config_changed,
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

  //---------------------------------------------------------------------------
  // Register configuration parameters
  //---------------------------------------------------------------------------
  logic [$clog2(MAX_DECIM+1)-1:0] decim_factor_reg;
  logic                           config_changed_reg;
  always_ff @(posedge clk) begin
    config_changed_reg <= '0; // Default to no config change
    if (config_changed) begin
      decim_factor_reg <= decim_factor;
      config_changed_reg <= '1; // Pulse to indicate config change
    end
    if (rst) begin
      decim_factor_reg   <= 1; // Default to no decimation
      config_changed_reg <= 1'b0;
    end
  end


  //--------------------------------------------------------------------------
  // Sign extension to ACCUM_W bits for the input samples
  //
  // Each sample in data_in is SAMP_W bits wide ({I[SAMP_W/2-1:0], Q[SAMP_W/2-1:0]}).
  // Sign-extend each I and Q component independently from SAMP_W/2 to COMP_W
  // bits, then pack into integ_bus[0].tdata as ACCUM_W-wide samples.
  //--------------------------------------------------------------------------

  assign integ_bus[0].tvalid = data_in.tvalid;
  assign integ_bus[0].tlast  = data_in.tlast;
  assign data_in.tready      = integ_bus[0].tready;

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
    .decim_factor  (decim_factor_reg),
    .decim_changed (config_changed_reg)
  );

  //---------------------------------------------------------------------------
  // Comb chain: ORDER stages at decimated rate
  //---------------------------------------------------------------------------
  for (genvar stage = 0; stage < ORDER; stage++) begin : gen_combs
    cic_comb_filter #(
      .ACCUM_W (ACCUM_W),
      .SPC     (SPC)
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
    .IN_WIDTH  (ACCUM_W),
    .OUT_WIDTH (SAMP_W),
    .MAX_RATE  (MAX_DECIM),
    .ORDER     (ORDER)
  ) cic_barrel_shift_i (
    .clk         (clk),
    .rst         (rst),
    .clear       (clear),
    .rate_factor (decim_factor_reg),
    .data_in     (comb_bus[ORDER]),
    .data_out    (data_out)
  );

endmodule : cic_filter_decim

`default_nettype wire
