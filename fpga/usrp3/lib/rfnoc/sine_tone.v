//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: sine_tone
//
// Description:
//
//   Sine tone generator. This block uses the Xilinx CORDIC IP configured to
//   perform the rotate function in units of scaled radians. See the CORDIC IP
//   Product Guide (PG105) for details.
//
//   This block outputs the X/I/real component in the most-significant bits and
//   the Y/Q/imaginary component in the least-significant bits. This is
//   opposite from the Xilinx IP but matches RFNoC.
//
//   The SR_PHASE_INC register controls the phase increment, in scaled radians,
//   for the sine waveform generator. It is a 16-bit signed fixed-point phase
//   value with 3 integer bits and 13 fractional bits. This is the amount by
//   which REG_CARTESIAN is rotated counter-clockwise each clock cycle. In
//   other words, it controls the rate of rotation, or the frequency, of the
//   sine wave. In scaled radians, the phase value range -1 to +1 corresponds
//   to -Pi to Pi in radians.
//
//   The SR_CARTESIAN register sets the sets the (X,Y) Cartesian coordinate
//   that will be rotated to generate the sine output. Both X and Y are 16-bit
//   signed fixed-point values with 2 integer bits and 14 fractional bits.
//   X/I/real is in the upper 16-bits and Y/Q/imaginary is in the lower 16-bits.
//
//   In addition to rotation, the SR_CARTESIAN input vector is also scaled by
//   a "CORDIC scale factor" that equals about 1.1644 (that is, the product of
//   sqrt(1 + 2^(-2i)) for i = 1 to n, where n = 14, the number of fractional
//   bits).
//
// Parameters:
//
//   SR_PHASE_INC_ADDR : The address to use for SR_PHASE_INC.
//   SR_CARTESIAN_ADDR : The address to use for SR_CARTESIAN.
//


module sine_tone #(
  parameter WIDTH             = 32,
  parameter SR_PHASE_INC_ADDR = 129,
  parameter SR_CARTESIAN_ADDR = 130
) (
  input              clk,
  input              reset,
  input              clear,
  input              enable,

  // Settings bus
  input              set_stb,
  input  [WIDTH-1:0] set_data,
  input  [      7:0] set_addr,

  // Output sinusoid
  output [WIDTH-1:0] o_tdata,
  output             o_tlast,
  output             o_tvalid,
  input              o_tready
);

  wire [15:0] phase_in_tdata;
  wire        phase_in_tlast;
  wire        phase_in_tvalid;
  wire        phase_in_tready;

  wire [15:0] phase_out_tdata;
  wire        phase_out_tlast;
  wire        phase_out_tvalid;
  wire        phase_out_tready;

  wire [WIDTH-1:0] cartesian_tdata;
  wire             cartesian_tlast;
  wire             cartesian_tvalid;
  wire             cartesian_tready;

  wire [WIDTH-1:0] sine_out_tdata;
  wire             sine_out_tlast;
  wire             sine_out_tvalid;
  wire             sine_out_tready;

  // AXI settings bus for phase values
  axi_setting_reg #(
    .ADDR        (SR_PHASE_INC_ADDR),
    .AWIDTH      (8),
    .WIDTH       (16),
    .STROBE_LAST (1),
    .REPEATS     (1)
  ) set_phase_acc (
    .clk       (clk),
    .reset     (reset),
    .error_stb (),
    .set_stb   (set_stb),
    .set_addr  (set_addr),
    .set_data  (set_data),
    .o_tdata   (phase_in_tdata),
    .o_tlast   (phase_in_tlast),
    .o_tvalid  (phase_in_tvalid),
    .o_tready  (phase_in_tready & enable)
  );

  // AXI settings bus for Cartesian values
  axi_setting_reg #(
    .ADDR    (SR_CARTESIAN_ADDR),
    .AWIDTH  (8),
    .WIDTH   (32),
    .REPEATS (1)
  ) set_axis_cartesian (
    .clk       (clk),
    .reset     (reset),
    .error_stb (),
    .set_stb   (set_stb),
    .set_addr  (set_addr),
    .set_data  (set_data),
    .o_tdata   (cartesian_tdata),
    .o_tlast   (),
    .o_tvalid  (cartesian_tvalid),
    .o_tready  (cartesian_tready & enable)
  );

  assign cartesian_tlast = 1;

  // Phase accumulator
  phase_accum phase_acc (
    .clk      (clk),
    .reset    (reset),
    .clear    (clear),
    .i_tdata  (phase_in_tdata),
    .i_tlast  (phase_in_tlast),
    .i_tvalid (1'b1),
    .i_tready (phase_in_tready),
    .o_tdata  (phase_out_tdata),
    .o_tlast  (phase_out_tlast),
    .o_tvalid (phase_out_tvalid),
    .o_tready (phase_out_tready & enable)
  );

  // CORDIC. Swap I and Q to match what the Xilinx IP expects.
  cordic_rotator cordic_inst (
    .aclk                    (clk),
    .aresetn                 (~(reset|clear)),
    .s_axis_phase_tdata      (phase_out_tdata),
    .s_axis_phase_tvalid     (phase_out_tvalid & cartesian_tvalid & enable),
    .s_axis_phase_tready     (phase_out_tready),
    .s_axis_cartesian_tdata  ({cartesian_tdata[      0 +: WIDTH/2],   // Q
                               cartesian_tdata[WIDTH/2 +: WIDTH/2]}), // I
    .s_axis_cartesian_tlast  (cartesian_tlast),
    .s_axis_cartesian_tvalid (phase_out_tvalid & cartesian_tvalid & enable),
    .s_axis_cartesian_tready (cartesian_tready),
    .m_axis_dout_tdata       ({sine_out_tdata[      0 +: WIDTH/2],    // Q
                               sine_out_tdata[WIDTH/2 +: WIDTH/2]}),  // I
    .m_axis_dout_tlast       (sine_out_tlast),
    .m_axis_dout_tvalid      (sine_out_tvalid),
    .m_axis_dout_tready      (sine_out_tready & enable)
  );

  assign o_tdata         = sine_out_tdata;
  assign o_tlast         = sine_out_tlast;
  assign o_tvalid        = sine_out_tvalid;
  assign sine_out_tready = o_tready;

endmodule  // sine_tone
