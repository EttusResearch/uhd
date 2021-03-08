//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rx_dcoffset
//
// Description:
//
//   RX Offset DC Correction Module
//   ------------------------------
//
//   This module has two modes of operation:
//   - Automatic mode: In this case, this module acts as an IIR filter of the form
//
//        y[k] = x[k] - alpha * y[k-1]                                      (1)
//
//    The module thus implements a notch filter around DC.
//
//   - Fixed mode: In this case, a known DC offset is simply subtracted from the
//     input signal.
//
//   IQ Operation: To fix the DC offset of an IQ signal, this module is
//   typically instantiated twice, once for the I and Q signal separately.
//
//   Settings register:
//   This module implements a single settings bus register with configurable
//   address. The 32-bit payload of the register configures the module as
//   follows:
//   - Bit 31: When 1, use "Fixed Mode" (see above). When 0, use "Automatic Mode".
//   - Bit 30: When asserted, bits 29 through 0 are used to initialize the
//             accumulator.
//   - Bit 29:0: This sets the 30 MSBs for the accumulator. In Fixed Mode, the
//               accumulator is left unchanged and is directly subtracted from
//               the input signal.
//               In Automatic Mode, the accumulator can be primed using these
//               bits, but that is uncommon. Typically, the x[k-1] value is
//               assumed to be zero at the beginning.
//
//   Setting the register to zero is equivalent to enabling automatic mode with
//   no initial accumulator. It may be useful to set the accumulator to zero
//   between bursts if their relative DC offset is significantly different.
//
//   Alpha value: To avoid the usage of a multiplier in this value, the alpha
//   value is limited to powers of two, and is not runtime-configurable.
//
// Parameters:
//   WIDTH       : Input signal width
//   ADDR        : Settings bus address
//   ALPHA_SHIFT : -log2(desired_alpha), where desired_alpha is the alpha value
//                 in equation (1) and must be a power of two.

`default_nettype none
module rx_dcoffset #(
  parameter WIDTH        = 16,
  parameter ADDR         = 8'd0,
  parameter ALPHA_SHIFT  = 20
) (
  input wire clk,
  input wire rst,
  // Settings bus input
  input wire set_stb,
  input wire [7:0] set_addr,
  input wire [31:0] set_data,
  // Input signal
  input wire in_stb,
  input wire [WIDTH-1:0] in,
  // Output signal
  output wire out_stb,
  output wire [WIDTH-1:0] out
);

  localparam int_width = WIDTH + ALPHA_SHIFT;

  wire                set_now = set_stb & (ADDR == set_addr);
  reg                 fixed;  // uses fixed offset
  reg [int_width-1:0] integrator;
  reg                 integ_in_stb;
  wire [WIDTH-1:0]    quantized;

  always @(posedge clk) begin
    if (rst) begin
      integ_in_stb <= 0;
      fixed <= 0;
      integrator <= {int_width{1'b0}};
    end else if(set_now) begin
      fixed <= set_data[31];
      if (set_data[30]) begin
        integrator <= {set_data[29:0],{(int_width-30){1'b0}}};
      end
    end else if(~fixed & in_stb) begin
      integrator <= integrator + {{(ALPHA_SHIFT){out[WIDTH-1]}},out};
    end
    integ_in_stb <= in_stb;
  end

  round_sd #(
    .WIDTH_IN(int_width),
    .WIDTH_OUT(WIDTH)
  ) round_sd (
    .clk(clk),
    .reset(rst),
    .in(integrator),
    .strobe_in(integ_in_stb),
    .out(quantized),
    .strobe_out()
  );

  add2_and_clip_reg #(
    .WIDTH(WIDTH)
  ) add2_and_clip_reg (
    .clk(clk),
    .rst(rst),
    .in1(in),
    .in2(-quantized),
    .strobe_in(in_stb),
    .sum(out),
    .strobe_out(out_stb)
  );

endmodule // rx_dcoffset
`default_nettype wire
