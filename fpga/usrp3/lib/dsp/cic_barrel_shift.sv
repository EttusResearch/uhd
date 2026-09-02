//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_barrel_shift
//
// Description:
//
//   Rate-dependent output scaling stage shared by the CIC decimation and
//   interpolation filters. Accepts SPC I/Q samples at the full accumulator
//   width (IN_WIDTH bits per sample) and right-shifts each I and Q component
//   to normalize the CIC gain, then rounds the result to OUT_WIDTH/2 bits per
//   component.
//
//   Each component enters the shifter scaled up by one bit, so the most
//   significant bit that the shift would otherwise discard survives as the LSB
//   of the shifted value. The final quantization is then an ordinary
//   axi_round_and_clip stage that removes that single bit with
//   round-to-nearest. Rounding this way keeps the quantization error zero-mean
//   instead of biasing every sample towards negative infinity as plain
//   truncation does.
//
//   The required shift for rate factor R is ceil(log2((R*M)^N)), where M=1
//   (differential delay) and N=ORDER. A compile-time lookup table
//   (shift_lut[1:MAX_RATE]) pre-computes this value for every supported rate.
//
//   The shift itself is a SHIFT_W-stage binary barrel shifter: stage k
//   conditionally shifts right by 2^k positions when shift_amount[k] is set.
//   Pipeline depth is SHIFT_W = ceil(log2(IN_WIDTH/2 - OUT_WIDTH/2 + 1)) cycles
//   plus the two register stages of the round and clip.
//
// Parameters:
//
//   SPC       : Number of samples per clock cycle (power of 2, >= 1).
//   OUT_WIDTH : Output sample width in bits (I + Q packed, each OUT_WIDTH/2 bits).
//               data_out.tdata must be SPC * OUT_WIDTH bits wide.
//   IN_WIDTH  : Input sample width in bits (I + Q packed, each IN_WIDTH/2 bits).
//               data_in.tdata must be SPC * IN_WIDTH bits wide.
//   MAX_RATE : Maximum rate factor (decimation or interpolation ratio).
//              Sizes the compile-time shift lookup table.
//   ORDER    : CIC filter order (number of integrator/comb stages).
//              Used to compute the per-rate shift value.
//
// Ports:
//
//   rate_factor : Active rate factor (1..MAX_RATE). Must be stable while
//                 data is in flight. Sampled through a 2-stage pipeline to
//                 align the shift with data latency through upstream stages.
//   data_in     : AXI-Stream input; tdata is SPC * IN_WIDTH bits wide,
//                 packed as {I[IN_WIDTH/2-1:0], Q[IN_WIDTH/2-1:0]} per sample.
//   data_out    : AXI-Stream output; tdata is SPC * OUT_WIDTH bits wide,
//                 packed as {I[OUT_WIDTH/2-1:0], Q[OUT_WIDTH/2-1:0]} per sample.
//

`default_nettype none

module cic_barrel_shift #(
  parameter int SPC      = 4,
  parameter int OUT_WIDTH   = 32,
  parameter int IN_WIDTH   = 128,
  parameter int MAX_RATE = 255,
  parameter int ORDER    = 4
)(
  input wire clk,
  input wire rst,
  input wire clear,
  // Active rate factor (decimation or interpolation ratio, 1..MAX_RATE).
  input wire [$clog2(MAX_RATE+1)-1:0] rate_factor,
  // Data in: SPC samples, each IN_WIDTH bits wide ({I, Q} each IN_WIDTH/2 bits)
  AxiStreamIf.slave  data_in,
  // Data out: SPC samples, each OUT_WIDTH bits wide ({I, Q} each OUT_WIDTH/2 bits)
  AxiStreamIf.master data_out
);

  localparam int MAX_SHIFT = IN_WIDTH/2 - OUT_WIDTH/2;
  localparam int SHIFT_W   = $clog2(MAX_SHIFT + 1);
  localparam int IN_COMP_W = IN_WIDTH/2;
  localparam int OUT_COMP_W = OUT_WIDTH/2;
  // One extra LSB so the bit dropped by the shift is available for rounding.
  localparam int SHIFT_COMP_W = IN_COMP_W + 1;
  // The normalized result fits in the output range; retain the round bit.
  localparam int ROUND_COMP_W = OUT_COMP_W + 1;

  //---------------------------------------------------------------------------
  // Compile-time shift lookup table
  //
  // shift_lut[r] = ceil(log2((r * 1)^ORDER)) = ceil(log2(r**ORDER))
  // (differential delay M=1 is absorbed into the formula).
  //---------------------------------------------------------------------------
  logic [SHIFT_W-1:0] shift_lut [1:MAX_RATE];
  for (genvar r = 1; r <= MAX_RATE; r++) begin : gen_shift_lut
    localparam int SHIFT_R = $clog2((longint'(r)) ** ORDER);
    assign shift_lut[r] = SHIFT_W'(SHIFT_R);
  end : gen_shift_lut

  //---------------------------------------------------------------------------
  // Reading the shift amount from the LUT.
  //
  // The shift_lut is implemented in a BRAM. In order to improve timing a 2
  // stage pipeline without any reset is used to allow the address register and
  // the output register of the BRAM to be activated.
  // The rate_factor is assumed to be stable while data is in flight, so the shift amount is
  // guaranteed to be stable when the data arrives at the barrel shifter.
  //---------------------------------------------------------------------------
  logic [SHIFT_W-1:0] shift_amount [2];
  always_ff @(posedge clk) begin
    shift_amount[0] <= shift_lut[rate_factor];
    shift_amount[1] <= shift_amount[0];
  end

  // Lane signals aggregated from the per-sample generate block below.
  logic [SPC-1:0]              lane_tvalid;
  logic [SPC-1:0]              lane_tlast;
  logic [SPC-1:0]              lane_tready;
  logic [SPC-1:0][OUT_WIDTH-1:0] lane_tdata;

  // All lanes share the same tready (aligned by construction).
  assign data_in.tready = lane_tready[0];

  for (genvar samp_idx = 0; samp_idx < SPC; samp_idx++) begin : gen_shift_out
    // Arithmetic right-shift pipeline: stages shift right in descending bit
    // order, from the most significant set bit to the least significant one.
    logic signed [SHIFT_COMP_W-1:0] q_shifted[SHIFT_W+1];
    logic signed [SHIFT_COMP_W-1:0] i_shifted[SHIFT_W+1];
    logic        [SHIFT_W:0]        shifter_tvalid;
    logic                           shifter_tlast[SHIFT_W+1];

    logic stage_tready [SHIFT_W+1];

    // A rate change leading to a new shift amount is only legal while the
    // barrel-shifter pipeline is empty. The shift amount is shared by all
    // pipeline stages, so changing it with valid data in flight would apply the
    // new amount to part of an old word. This assumption is also used by the
    // barrel-shifter multicycle path in
    // fpga/usrp3/top/x400/constraints/timing/common.xdc.
    // The assertion is done even when reset or clear is present to ensure the
    // multicycle path is used correctly at all times.
    // synthesis translate_off
    assert property (
      @(posedge clk) $changed(shift_amount[1]) |-> (shifter_tvalid == 0)[*2]
    ) else
      $error("cic_barrel_shift: data processed before shift_amount was settled");
    // synthesis translate_on

    // Input stage: scale up by one bit so the shift keeps the round bit.
    assign q_shifted[SHIFT_W] = {data_in.tdata[IN_WIDTH*samp_idx             +: IN_COMP_W], 1'b0};
    assign i_shifted[SHIFT_W] = {data_in.tdata[IN_WIDTH*samp_idx + IN_COMP_W +: IN_COMP_W], 1'b0};
    assign shifter_tvalid[SHIFT_W] = data_in.tvalid;
    assign shifter_tlast[SHIFT_W]  = data_in.tlast;

    for (genvar shift_bit = SHIFT_W - 1; shift_bit >= 0; shift_bit--) begin : gen_shift_bits
      // Combinational next-stage values for this shift bit.
      logic signed [SHIFT_COMP_W-1:0] q_next;
      logic signed [SHIFT_COMP_W-1:0] i_next;
      always_comb begin
        if (shift_amount[1][shift_bit]) begin
          q_next = q_shifted[shift_bit + 1] >>> (2 ** shift_bit);
          i_next = i_shifted[shift_bit + 1] >>> (2 ** shift_bit);
        end else begin
          q_next = q_shifted[shift_bit + 1];
          i_next = i_shifted[shift_bit + 1];
        end
      end

      // Register every second stage and pass through the others. This places
      // a 4:1 mux before each registered stage, which fits LUT6 best for
      // Xilinx architectures.
      axi_fifo #(
        .WIDTH (2*SHIFT_COMP_W + 1), // {tlast, i, q}
        .SIZE  ((shift_bit % 2 == 1) ? 0 : -1)
      ) pipe_ff (
        .clk     (clk),
        .reset   (rst),
        .clear   (clear),
        .i_tdata ({shifter_tlast[shift_bit + 1], i_next, q_next}),
        .i_tvalid(shifter_tvalid[shift_bit + 1]),
        .i_tready(stage_tready[shift_bit + 1]),
        .o_tdata ({shifter_tlast[shift_bit], i_shifted[shift_bit], q_shifted[shift_bit]}),
        .o_tvalid(shifter_tvalid[shift_bit]),
        .o_tready(stage_tready[shift_bit]),
        .occupied(),
        .space()
      );

    end : gen_shift_bits

    // The upper shifted bits are redundant sign extension. Keep the output
    // component and the extra LSB needed for round-to-nearest.
    logic [OUT_WIDTH-1:0] rounded;
    logic                  rounded_tvalid;
    logic                  rounded_tlast;

    axi_round_and_clip_complex #(
      .WIDTH_IN (ROUND_COMP_W),
      .WIDTH_OUT(OUT_COMP_W),
      .CLIP_BITS(0),
      .FIFOSIZE (1)
    ) round_clip_complex (
      .clk     (clk),
      // axi_round_and_clip has no clear input, so flush it through reset.
      .reset   (rst | clear),
      .i_tdata ({i_shifted[0][ROUND_COMP_W-1:0], q_shifted[0][ROUND_COMP_W-1:0]}),
      .i_tlast (shifter_tlast[0]),
      .i_tvalid(shifter_tvalid[0]),
      .i_tready(stage_tready[0]),
      .o_tdata (rounded),
      .o_tlast (rounded_tlast),
      .o_tvalid(rounded_tvalid),
      .o_tready(data_out.tready)
    );

    assign lane_tready[samp_idx] = stage_tready[SHIFT_W];
    assign lane_tvalid[samp_idx] = rounded_tvalid;
    assign lane_tlast[samp_idx]  = rounded_tlast;
    assign lane_tdata[samp_idx]  = rounded;

  end : gen_shift_out

  assign data_out.tdata = lane_tdata;

  // All lanes are aligned; use lane 0 for the aggregate valid/last.
  assign data_out.tvalid = lane_tvalid[0];
  assign data_out.tlast  = lane_tlast[0];

endmodule : cic_barrel_shift

`default_nettype wire
