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
//                  ┌────────────────────────┐   tready
//    interp_factor─┤    Interpolator (×R)   │<──────────────┐
//                  └───────────┬────────────┘               │
//                              │                            │
//               ┌──────────────┼────────────────┐           │
//               │  ┌────────────────────────┐   │           │
//               │  │      Integrator 0      │   │           │
//               │  └────────────────────────┘   │           │
//               │              │                │ ORDER     │
//               │             ···               │ stages    │
//               │              │                │           │
//               │  ┌────────────────────────┐   │           │
//               │  │   Integrator ORDER-1   │   │           │
//               │  └────────────────────────┘   │           │
//               └──────────────┼────────────────┘           │
//                              │                            │
//                              v                            │
//               ┌───────────────────────────────┐           │
//               │         Barrel shift          │           │
//               └──────────────┬────────────────┘           │
//                              │                            │
//                              v                            │
//               ┌───────────────────────────────┐           │
//               │          Output FIFO          │───────────┘
//               └───────────────────────────────┘
//                              │
//                              v
//                           data_out
//
//   ORDER comb stages (y[n] = x[n] - x[n-D]) operate at the full input rate.
//   The single interpolator then produces R output words for each input word.
//   Finally, ORDER integrator stages accumulate partial sums across SPC
//   parallel sample lanes at the full output rate.
//
//   Comb and interpolator stages use COMB_W bits. COMB_W provides ORDER bits
//   of growth per I/Q component for the comb cascade. Integrator stages use
//   ACCUM_W bits, automatically computed to provide
//   ORDER*ceil(log2(MAX_INTERP*MAX_DELAY)) bits of growth per component.
//   Internal stages do not clip or round. The output is normalized back to
//   SAMP_W with signed round-to-nearest quantization.
//
//   The interpolation factor is driven directly and must remain stable while
//   the module is actively processing data.
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
  AxiStreamIf.slave         data_in,
  AxiStreamIf.master        data_out
);
  `include "../axi4s_sv/axi4s.vh"

  //--------------------------------------------------------------------------
  // Local parameters
  //--------------------------------------------------------------------------
  localparam int MAX_DELAY = 1;  // Maximum comb delay in samples

  // Accumulator width for the maximum integrator growth without overflow.
  localparam int ACCUM_W = 2 * (SAMP_W/2 + $clog2(MAX_INTERP*MAX_DELAY)*ORDER);

  // A cascade of ORDER comb stages has coefficients whose absolute sum is
  // 2**ORDER, so each component needs ORDER bits of growth. The comb stages
  // precede the interpolator and can therefore use less width than the full
  // accumulation path.
  localparam int COMB_W  = 2 * (SAMP_W/2 + ORDER);

  //---------------------------------------------------------------------------
  // Internal AXI-Stream buses
  //---------------------------------------------------------------------------
  // Comb chain: ORDER+1 buses connecting comb filter inputs/outputs.
  // comb_bus[0] is driven by the sign-extended input.
  // comb_bus[i] for i > 0 carries the output of comb stage i-1.
  AxiStreamIf #(SPC * COMB_W) comb_bus [ORDER+1] (.clk(clk), .rst(rst));

  // The interpolator operates at COMB_W bits. Its output is sign-extended to
  // ACCUM_W bits before entering the integrator chain.
  AxiStreamIf #(SPC * COMB_W) interp_bus (.clk(clk), .rst(rst));
  AxiStreamIf #(SPC * ACCUM_W) interp_bus_extended (.clk(clk), .rst(rst));

  // Integrator path: two buses connecting the interpolator to the integrator
  // chain and carrying the chain output.
  // integ_bus[0] is driven by the interpolator.
  // integ_bus[1] carries the output of the ORDER-stage integrator chain.
  AxiStreamIf #(SPC * ACCUM_W) integ_bus [2] (.clk(clk), .rst(rst));

  // Barrel Shift output bus
  AxiStreamIf #(data_out.DATA_WIDTH) barrel_shift_out (.clk(clk), .rst(rst));
  AxiStreamIf #(data_out.DATA_WIDTH) output_fifo_in (.clk(clk), .rst(rst));


  //--------------------------------------------------------------------------
  // Sign extension to COMB_W bits for the input samples
  //
  // Each sample in data_in is SAMP_W bits wide ({I, Q}). Sign-extend each I
  // and Q component independently from SAMP_W/2 to COMB_W/2 bits, then pack
  // into comb_bus[0].tdata as COMB_W-wide samples.
  //--------------------------------------------------------------------------

  assign comb_bus[0].tvalid = data_in.tvalid;
  assign comb_bus[0].tlast  = data_in.tlast;
  assign data_in.tready     = comb_bus[0].tready;

  for (genvar samp_idx = 0; samp_idx < SPC; samp_idx++) begin : gen_sign_ext
    // Q component (lower half of each sample)
    sign_extend #(
      .bits_in (SAMP_W/2),
      .bits_out(COMB_W/2)
    ) sign_ext_q (
      .in (data_in.tdata[SAMP_W*samp_idx +: SAMP_W/2]),
      .out(comb_bus[0].tdata[COMB_W*samp_idx +: COMB_W/2])
    );
    // I component (upper half of each sample)
    sign_extend #(
      .bits_in (SAMP_W/2),
      .bits_out(COMB_W/2)
    ) sign_ext_i (
      .in (data_in.tdata[SAMP_W*samp_idx + SAMP_W/2 +: SAMP_W/2]),
      .out(comb_bus[0].tdata[COMB_W*samp_idx + COMB_W/2 +: COMB_W/2])
    );
  end : gen_sign_ext


  //---------------------------------------------------------------------------
  // Comb chain: ORDER stages at input rate
  //---------------------------------------------------------------------------
  for (genvar stage = 0; stage < ORDER; stage++) begin : gen_combs
    cic_comb_filter #(
      .ACCUM_W  (COMB_W),
      .SPC      (SPC),
      .FIFO_SIZE((stage == ORDER-1)? 1 : 0)
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
    .SAMP_W        (COMB_W),
    .SPC           (SPC),
    .R_MAX         (MAX_INTERP)
  ) cic_interpolator_i (
    .clk           (clk),
    .rst           (rst || clear),
    .data_in       (comb_bus[ORDER]),
    .data_out      (interp_bus),
    .interp_factor (interp_factor)
  );

  //--------------------------------------------------------------------------
  // Sign extension from COMB_W to ACCUM_W bits.
  //
  // The interpolator does not increase the numeric width. The integrator
  // stages require ACCUM_W bits to accommodate their accumulated growth.
  //--------------------------------------------------------------------------

  assign interp_bus_extended.tvalid = interp_bus.tvalid;
  assign interp_bus_extended.tlast  = interp_bus.tlast;
  assign interp_bus.tready   = interp_bus_extended.tready;

  for (genvar samp_idx = 0; samp_idx < SPC; samp_idx++) begin : gen_interp_ext
    sign_extend #(
      .bits_in (COMB_W/2),
      .bits_out(ACCUM_W/2)
    ) sign_ext_q (
      .in (interp_bus.tdata[COMB_W*samp_idx +: COMB_W/2]),
      .out(interp_bus_extended.tdata[ACCUM_W*samp_idx +: ACCUM_W/2])
    );
    sign_extend #(
      .bits_in (COMB_W/2),
      .bits_out(ACCUM_W/2)
    ) sign_ext_i (
      .in (interp_bus.tdata[COMB_W*samp_idx + COMB_W/2 +: COMB_W/2]),
      .out(interp_bus_extended.tdata[ACCUM_W*samp_idx + ACCUM_W/2 +: ACCUM_W/2])
    );
  end : gen_interp_ext


  //---------------------------------------------------------------------------
  // Integrator chain: ORDER stages at full output rate
  //---------------------------------------------------------------------------
  // Starting here until the output FIFO the data flow is controlled by the
  // output FIFO backpressure.
  logic fifo_ready;
  logic input_blocked;

  // Align input locking with AXI transfer.
  always_ff @(posedge clk) begin
    if (rst || clear) begin
      input_blocked <= 1'b0;
    end else begin
      if (fifo_ready) begin
        input_blocked <= 1'b0;
      end else if (interp_bus_extended.tvalid && interp_bus_extended.tready) begin
        input_blocked <= 1'b1;
      end
    end
  end

  always_comb begin
    `AXI4S_ASSIGN(integ_bus[0], interp_bus_extended)

    // overwrite handshake signal to block input when FIFO is full
    interp_bus_extended.tready = integ_bus[0].tready & ~input_blocked;
    integ_bus[0].tvalid = interp_bus_extended.tvalid & ~input_blocked;
  end

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
    .MAX_RATE  (MAX_INTERP),
    .ORDER     (ORDER-1) // An Nth order CIC interpolation filter has a Gain of
                         // R^(N-1) at DC, so we shift by ceil(log2(R^(N-1))) bits.
  ) cic_barrel_shift_i (
    .clk         (clk),
    .rst         (rst),
    .clear       (clear),
    .rate_factor (interp_factor),
    .data_in     (integ_bus[1]),
    .data_out    (barrel_shift_out)
  );

  //---------------------------------------------------------------------------
  // Output FIFO to absorb latency of bypassed modules
  //---------------------------------------------------------------------------
  import cic_utils_pkg::*;
  localparam int LATENCY = integrator_latency(SPC, ORDER)
                          + barrel_shift_latency(ACCUM_W, SAMP_W)
                          + 1  // fifo_ready register
                          + 1  // input_blocked register
                          + 1; // output FIFO output register offset for space calculation


  // assign the barrel shift output to the FIFO input, and always ready to
  // accept data. The FIFO will backpressure the integrator chain when it is full.
  always_comb begin
    `AXI4S_ASSIGN(output_fifo_in, barrel_shift_out)
    barrel_shift_out.tready = 1'b1;
  end

  // Allow the integrator chain to continue processing as long as the FIFO has
  // enough space for the entire latency of the bypassed modules.
  logic [15:0] fifo_space;
  always_ff @(posedge clk) begin
    if (rst || clear) begin
      fifo_ready <= 1'b0;
    end else begin
      fifo_ready <= (fifo_space > LATENCY);
    end
  end

  // FIFO depth is the next power of two at or above twice the calculated
  // latency, ensuring sufficient buffering for constant data flow downstream
  // after stalls.
  localparam int FIFO_WIDTH = data_out.DATA_WIDTH + 1;
  logic [FIFO_WIDTH-1:0] fifo_i_tdata;
  logic [FIFO_WIDTH-1:0] fifo_o_tdata;

  assign fifo_i_tdata   = {output_fifo_in.tlast, output_fifo_in.tdata};
  assign {data_out.tlast, data_out.tdata} = fifo_o_tdata;

  axi_fifo #(
    .WIDTH(FIFO_WIDTH),
    .SIZE ($clog2(2*LATENCY))
  ) output_fifo (
    .clk      (clk),
    .reset    (rst | clear),
    .clear    (1'b0),
    .i_tdata  (fifo_i_tdata),
    .i_tvalid (output_fifo_in.tvalid),
    .i_tready (output_fifo_in.tready),
    .o_tdata  (fifo_o_tdata),
    .o_tvalid (data_out.tvalid),
    .o_tready (data_out.tready),
    .space    (fifo_space),
    .occupied ()
  );

endmodule : cic_filter_interp

`default_nettype wire
