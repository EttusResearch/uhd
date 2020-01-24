//
// Copyright 2015 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Expects scaled radians fixed point input format of the form Q2.#,
// Example: WIDTH_IN=8 then input format: Q2.5 (sign bit, 2 integer bits, 5 fraction bits)
module phase_accum #(
  parameter REVERSE_ROTATION = 0, // Negate phase increment value
  parameter WIDTH_ACCUM = 16,
  parameter WIDTH_IN = 16,
  parameter WIDTH_OUT = 16)
(
  input clk, input reset, input clear,
  input [WIDTH_IN-1:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
  output [WIDTH_OUT-1:0] o_tdata, output o_tlast, output o_tvalid, input o_tready
);

  reg signed [WIDTH_ACCUM-1:0] accum, accum_next, phase_inc;
  // Scaled radians. Restrict range from +1 to -1.
  wire signed [WIDTH_ACCUM-1:0] POS_ROLLOVER = 2**(WIDTH_ACCUM-3);
  wire signed [WIDTH_ACCUM-1:0] NEG_ROLLOVER = -(2**(WIDTH_ACCUM-3));

  wire [WIDTH_OUT-1:0] output_round_tdata;
  wire output_round_tvalid, output_round_tready, output_round_tlast;

  // Phase accumulator, can rotate in either direction
  always @(posedge clk) begin
    if (reset | clear) begin
      accum      <= 'd0;
      accum_next <= 'd0;
      phase_inc  <= 'd0;
    end else if (i_tready & i_tvalid) begin
      if (i_tlast) begin
        accum       <= {WIDTH_ACCUM{1'b0}};
        accum_next  <= REVERSE_ROTATION ? -$signed(i_tdata) : $signed(i_tdata);
        phase_inc   <= REVERSE_ROTATION ? -$signed(i_tdata) : $signed(i_tdata);
      end else begin
        if (accum_next >= POS_ROLLOVER) begin
          accum_next <= accum_next + phase_inc - 2*POS_ROLLOVER;
          accum      <= accum + phase_inc - 2*POS_ROLLOVER;
        end else if (accum_next <= NEG_ROLLOVER) begin
          accum_next <= accum_next + phase_inc - 2*NEG_ROLLOVER;
          accum      <= accum + phase_inc - 2*NEG_ROLLOVER;
        end else begin
          accum_next <= accum_next + phase_inc;
          accum      <= accum + phase_inc;
        end
      end
    end
  end

  generate
    // Bypass rounding if accumulator width is same as output width
    if (WIDTH_ACCUM == WIDTH_OUT) begin
      assign o_tdata  = accum;
      assign o_tvalid = i_tvalid;
      assign o_tlast  = i_tlast;
      assign i_tready = o_tready;
    end else begin
      axi_round #(
        .WIDTH_IN(WIDTH_ACCUM),
        .WIDTH_OUT(WIDTH_OUT))
      axi_round (
        .clk(clk), .reset(reset),
        .i_tdata(accum), .i_tlast(i_tlast), .i_tvalid(i_tvalid), .i_tready(i_tready),
        .o_tdata(o_tdata), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready));
    end
  endgenerate

endmodule
