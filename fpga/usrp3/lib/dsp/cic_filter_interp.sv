//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_filter_interp
//
// Description:
//
//   CIC interpolation filter for multiple samples per clock cycle,
//   implementing the classic Hogenauer (CIC) interpolation topology:
//
//                            data_in
//                               │
//                               v
//               ┌───────────────────────────────┐
//               │  ┌────────────────────────┐   │
//               │  │        Comb 0          │   │
//               │  └───────────┬────────────┘   │ ORDER stages
//               │              │                │ (input rate)
//               │             ...               │
//               │              │                │
//               │  ┌────────────────────────┐   │
//               │  │     Comb ORDER-1       │   │
//               │  └───────────┬────────────┘   │
//               └──────────────┼────────────────┘
//                              │
//                  ┌────────────────────────┐
//    interp_factor─┤    Interpolator (×R)   │
//                  └───────────┬────────────┘
//                              │
//               ┌──────────────┼────────────────┐
//               │  ┌────────────────────────┐   │
//               │  │      Integrator 0      │   │
//               │  └───────────┬────────────┘   │
//               │              │                │ ORDER stages
//               │             ···               │ (full output rate)
//               │              │                │
//               │  ┌────────────────────────┐   │
//               │  │   Integrator ORDER-1   │   │
//               │  └───────────┬────────────┘   │
//               └──────────────┼────────────────┘
//                              │
//                              v
//                           data_out
//
//   ORDER comb stages (y[n] = x[n] - x[n-D]) operate at the full input rate.
//   The single interpolator then produces R output words for each input word.
//   Finally, ORDER integrator stages accumulate partial sums across SPC
//   parallel sample lanes at the full output rate.
//
//   Internally, all stages operate at ACCUM_W bits, which is automatically
//   computed to accommodate the full CIC bit growth of N*log2(R_max*D_max).
//   No internal clipping or rounding is performed. The output is normalized
//   back to SAMP_W at the output.
//
//   The interpolation factor may be changed at runtime by asserting
//   config_changed for one clock cycle. Configuration must only be changed
//   when the module is idle (not actively processing data).
//
// Parameters:
//
//   SPC        : Number of samples per clock cycle (must be power of 2, >= 1).
//   SAMP_W     : Bit width of each sample (I + Q packed, each SAMP_W/2 bits).
//   MAX_INTERP : Maximum interpolation factor supported. Sizes internal registers.
//   ORDER      : CIC filter order (number of comb and integrator stages).
//
// Signals:
//
//   clk            : Clock input.
//   rst            : Synchronous reset input.
//   clear          : State clear input.
//   interp_factor  : Runtime-configurable interpolation ratio (1..MAX_INTERP).
//   config_changed : Pulse high for 1 cycle after changing interp_factor.
//                    Must only be asserted when the module is idle.
//   data_in        : Input AXI-Stream bus carrying SPC packed SAMP_W-bit
//                    samples per word.
//   data_out       : Output AXI-Stream bus carrying SPC packed SAMP_W-bit
//                    samples per word.
//

`default_nettype none

module cic_filter_interp #(
  parameter int SPC        = 4,
  parameter int SAMP_W     = 32,
  parameter int MAX_INTERP = 255,
  parameter int ORDER      = 4,

  localparam int FACTOR_W = $clog2(MAX_INTERP+1)
)(
  input wire                clk,
  input wire                rst,
  input wire                clear,
  input wire [FACTOR_W-1:0] interp_factor,
  input wire                config_changed,
  AxiStreamIf.slave         data_in,
  AxiStreamIf.master        data_out
);
  //--------------------------------------------------------------------------
  // Local parameters
  //--------------------------------------------------------------------------
  localparam int MAX_DELAY = 1;  // Maximum comb delay in samples

  // Accumulator width to accommodate max CIC growth without overflow:
  localparam int ACCUM_W = 2 * (SAMP_W/2 + $clog2(MAX_INTERP*MAX_DELAY)*ORDER);
  localparam int COMP_W  = ACCUM_W/2; // Width of each I/Q component in the accumulator

  //---------------------------------------------------------------------------
  // Internal AXI-Stream buses
  //---------------------------------------------------------------------------
  // Comb chain: ORDER+1 buses connecting comb filter inputs/outputs.
  // comb_bus[0] is driven by the sign-extended input.
  // comb_bus[i] for i > 0 carries the output of comb stage i-1.
  AxiStreamIf #(SPC * ACCUM_W) comb_bus [ORDER+1] (.clk(clk), .rst(rst));

  // Integrator chain: ORDER+1 buses connecting integrator outputs.
  // integ_bus[0] is driven by the interpolator output.
  // integ_bus[i] for i > 0 carries the output of integrator stage i-1.
  AxiStreamIf #(SPC * ACCUM_W) integ_bus [2] (.clk(clk), .rst(rst));

  //---------------------------------------------------------------------------
  // Register configuration parameters
  //---------------------------------------------------------------------------
  logic [FACTOR_W-1:0] interp_factor_reg;
  logic                config_changed_reg;

  always_ff @(posedge clk) begin
    config_changed_reg <= '0; // Default to no config change
    if (config_changed) begin
      interp_factor_reg  <= interp_factor;
      config_changed_reg <= '1; // Pulse to indicate config change
    end
    if (rst) begin
      interp_factor_reg  <= 1; // Default to no interpolation
      config_changed_reg <= 1'b0;
    end
  end


  //--------------------------------------------------------------------------
  // Sign extension to ACCUM_W bits for the input samples
  //
  // Each sample in data_in is SAMP_W bits wide ({I, Q}). Sign-extend each I
  // and Q component independently from SAMP_W/2 to COMP_W bits, then pack into
  // comb_bus[0].tdata as ACCUM_W-wide samples.
  //--------------------------------------------------------------------------

  assign comb_bus[0].tvalid = data_in.tvalid;
  assign comb_bus[0].tlast  = data_in.tlast;
  assign data_in.tready     = comb_bus[0].tready;

  for (genvar samp_idx = 0; samp_idx < SPC; samp_idx++) begin : gen_sign_ext
    // Q component (lower half of each sample)
    sign_extend #(
      .bits_in (SAMP_W/2),
      .bits_out(COMP_W)
    ) sign_ext_q (
      .in (data_in.tdata[SAMP_W*samp_idx +: SAMP_W/2]),
      .out(comb_bus[0].tdata[ACCUM_W*samp_idx +: COMP_W])
    );
    // I component (upper half of each sample)
    sign_extend #(
      .bits_in (SAMP_W/2),
      .bits_out(COMP_W)
    ) sign_ext_i (
      .in (data_in.tdata[SAMP_W*samp_idx + SAMP_W/2 +: SAMP_W/2]),
      .out(comb_bus[0].tdata[ACCUM_W*samp_idx + COMP_W +: COMP_W])
    );
  end : gen_sign_ext


  //---------------------------------------------------------------------------
  // Comb chain: ORDER stages at input rate
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
  end : gen_combs


  //---------------------------------------------------------------------------
  // Interpolator: increases sample rate by interp_factor
  //---------------------------------------------------------------------------
  cic_interpolator #(
    .SAMP_W        (ACCUM_W),
    .SPC           (SPC),
    .R_MAX         (MAX_INTERP)
  ) cic_interpolator_i (
    .clk           (clk),
    .rst           (rst || clear),
    .data_in       (comb_bus[ORDER]),
    .data_out      (integ_bus[0]),
    .interp_factor (interp_factor_reg),
    .interp_changed(config_changed_reg)
  );


  //---------------------------------------------------------------------------
  // Integrator chain: ORDER stages at full output rate
  //---------------------------------------------------------------------------
  cic_integrator #(
    .SAMP_W(ACCUM_W),
    .SPC   (SPC),
    .ORDER (ORDER)
  ) cic_integrator_i (
    .clk     (clk),
    .rst     (rst),
    .clr     (clear),
    .data_in (integ_bus[0]),
    .data_out(integ_bus[1])
  );


  // Barrel-shift each I/Q component and extract SAMP_W/2 output bits.
  cic_barrel_shift #(
    .SPC       (SPC),
    .IN_WIDTH  (ACCUM_W),
    .OUT_WIDTH (SAMP_W),
    .MAX_RATE  (MAX_INTERP),
    .ORDER     (ORDER-1) // An Nth order CIC interpolation filter has a Gain of
                         // R^(N-1) at DC, so we shift by ceil(log2(R^(N-1))) bits.
  ) cic_barrel_shift_i (
    .clk         (clk),
    .rst         (rst),
    .clear       (clear),
    .rate_factor (interp_factor_reg),
    .data_in     (integ_bus[1]),
    .data_out    (data_out)
  );

endmodule : cic_filter_interp

`default_nettype wire
