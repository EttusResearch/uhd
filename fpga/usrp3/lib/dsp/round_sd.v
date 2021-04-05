//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module round_sd #(
  parameter WIDTH_IN   = 18,
  parameter WIDTH_OUT  = 16,
  parameter DISABLE_SD = 0
) (
  input                      clk,
  input                      reset,
  input      [ WIDTH_IN-1:0] in,
  input                      strobe_in,
  output reg [WIDTH_OUT-1:0] out         = 0,
  output reg                 strobe_out  = 0
);

  localparam ERR_WIDTH = WIDTH_IN - WIDTH_OUT + 1;

  wire [ERR_WIDTH-1:0] err;
  wire [WIDTH_IN-1:0]  err_ext, err_ext_01, sum;
  wire [WIDTH_OUT-1:0] out_pre;
  wire                 strobe_pre;

  sign_extend #(
    .bits_in  (ERR_WIDTH),
    .bits_out (WIDTH_IN)
  ) ext_err (
    .in  (err),
    .out (err_ext)
  );

  assign err_ext_01 =
    //synthesis translate_off
    // Disallow X from getting stuck on the err_ext feedback path in simulation
    (^err_ext === 1'bX) ? 0 :
    //synthesis translate_on
    err_ext;

  add2_and_clip_reg #(
    .WIDTH (WIDTH_IN)
  ) add2_and_clip_reg (
    .clk        (clk),
    .rst        (reset),
    .in1        (in),
    .in2        ((DISABLE_SD == 0) ? err_ext_01 : {WIDTH_IN{1'b0}}),
    .strobe_in  (strobe_in),
    .sum        (sum),
    .strobe_out (strobe_pre)
  );

  round #(
    .bits_in  (WIDTH_IN),
    .bits_out (WIDTH_OUT)
  ) round_sum (
    .in  (sum),
    .out (out_pre),
    .err (err)
  );

  always @(posedge clk) begin
    if (reset) begin
      strobe_out <= 1'b0;
    end else begin
      strobe_out <= strobe_pre;
      if (strobe_pre) begin
        out <= out_pre;
      end
    end
  end

endmodule // round_sd
