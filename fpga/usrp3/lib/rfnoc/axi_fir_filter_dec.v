//
// Copyright 2018 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later

// Generic FIR filter with decimator

module axi_fir_filter_dec #(
  parameter WIDTH         = 24,
  parameter COEFF_WIDTH  = 18,
  parameter NUM_COEFFS    = 47,
  parameter [NUM_COEFFS*COEFF_WIDTH-1:0] COEFFS_VEC =
      {{1'b0,{(COEFF_WIDTH-1){1'b1}}},{(COEFF_WIDTH*(NUM_COEFFS-1)){1'b0}}},
  parameter BLANK_OUTPUT  = 0
)(
  input clk,
  input reset,

  input [2*WIDTH-1:0] i_tdata,
  input i_tlast,
  input i_tvalid,
  output i_tready,

  output [2*WIDTH-1:0] o_tdata,
  output o_tlast,
  output o_tvalid,
  input o_tready
);

wire [WIDTH-1:0]  tdata_fir0_dec0;
wire              tvalid_fir0_dec0;
wire              tlast_fir0_dec0;
wire              tready_fir0_dec0;

wire [WIDTH-1:0]  tdata_fir1_dec1;
wire              tvalid_fir1_dec1;
wire              tlast_fir1_dec1;
wire              tready_fir1_dec1;

wire [WIDTH-1:0] tdata_fir0;
wire [WIDTH-1:0] tdata_fir1;
wire [WIDTH-1:0] tdata_dec0;
wire [WIDTH-1:0] tdata_dec1;

// Split input data into real and imag. part.
assign tdata_fir0 = i_tdata[2*WIDTH-1:WIDTH];
assign tdata_fir1 = i_tdata[WIDTH-1:0];

// FIR filter for real part
axi_fir_filter #(.IN_WIDTH(WIDTH), .COEFF_WIDTH(COEFF_WIDTH), .OUT_WIDTH(WIDTH), .NUM_COEFFS(NUM_COEFFS), .COEFFS_VEC(COEFFS_VEC),
  .RELOADABLE_COEFFS(0), .BLANK_OUTPUT(0), .SYMMETRIC_COEFFS(1), .SKIP_ZERO_COEFFS(1), .USE_EMBEDDED_REGS_COEFFS(0)
) hbfir0(
  .clk(clk), .reset(reset), .clear(reset),
  .s_axis_data_tdata(tdata_fir0), .s_axis_data_tlast(i_tlast), .s_axis_data_tvalid(i_tvalid), .s_axis_data_tready(i_tready),
  .m_axis_data_tdata(tdata_fir0_dec0), .m_axis_data_tlast(tlast_fir0_dec0), .m_axis_data_tvalid(tvalid_fir0_dec0), .m_axis_data_tready(tready_fir0_dec0),
  .s_axis_reload_tdata(18'd0), .s_axis_reload_tvalid(1'b0), .s_axis_reload_tlast(1'b0), .s_axis_reload_tready());

// FIR filter for imag. part
axi_fir_filter #(.IN_WIDTH(WIDTH), .COEFF_WIDTH(COEFF_WIDTH), .OUT_WIDTH(WIDTH), .NUM_COEFFS(NUM_COEFFS), .COEFFS_VEC(COEFFS_VEC),
  .RELOADABLE_COEFFS(0), .BLANK_OUTPUT(0), .SYMMETRIC_COEFFS(1), .SKIP_ZERO_COEFFS(1), .USE_EMBEDDED_REGS_COEFFS(0)
) hbfir1(
  .clk(clk), .reset(reset), .clear(reset),
  .s_axis_data_tdata(tdata_fir1), .s_axis_data_tlast(i_tlast), .s_axis_data_tvalid(i_tvalid), .s_axis_data_tready(),
  .m_axis_data_tdata(tdata_fir1_dec1), .m_axis_data_tlast(tlast_fir1_dec1), .m_axis_data_tvalid(tvalid_fir1_dec1), .m_axis_data_tready(tready_fir1_dec1),
  .s_axis_reload_tdata(18'd0), .s_axis_reload_tvalid(1'b0), .s_axis_reload_tlast(1'b0), .s_axis_reload_tready());

// Decimator for real part
keep_one_in_n #(.KEEP_FIRST(1), .WIDTH(WIDTH), .MAX_N(4)
) dec0 (
  .clk(clk), .reset(reset), .vector_mode(1'b0), .n(2),
  .i_tdata(tdata_fir0_dec0), .i_tlast(tlast_fir0_dec0), .i_tvalid(tvalid_fir0_dec0), .i_tready(tready_fir0_dec0),
  .o_tdata(tdata_dec0), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready));

// Decimator for imag. part
keep_one_in_n #(.KEEP_FIRST(1), .WIDTH(WIDTH), .MAX_N(4)
) dec1 (
  .clk(clk), .reset(reset), .vector_mode(1'b0), .n(2),
  .i_tdata(tdata_fir1_dec1), .i_tlast(tlast_fir1_dec1), .i_tvalid(tvalid_fir1_dec1), .i_tready(tready_fir1_dec1),
  .o_tdata(tdata_dec1), .o_tlast(), .o_tvalid(), .o_tready(o_tready));

assign o_tdata = {tdata_dec0, tdata_dec1};

endmodule // axi_fir_filter_dec
