/*
 * f15_binmap.v
 *
 * Maps a log pwr value to an histogram bin
 *
 * Copyright (C) 2014  Ettus Corporation LLC
 * Copyright 2018 Ettus Research, a National Instruments Company
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * vim: ts=4 sw=4
 */

`ifdef SIM
`default_nettype none
`endif

module f15_binmap #(
	parameter integer BIN_WIDTH = 6,
	parameter integer SCALE_FRAC_BITS = 8
)(
	input  wire [15:0] in_0,
	input  wire [15:0] offset_0,		// unsigned
	input  wire [15:0] scale_0,			// unsigned
	output reg  [BIN_WIDTH-1:0] bin_5,	// bin number
	output reg  sat_ind_5,				// saturation indicator
	input  wire clk,
	input  wire rst
);
	localparam integer TBI = 15 + SCALE_FRAC_BITS;	// Top-Bit-Index

	// Signals
	wire [47:0] dsp_pout_4;
	wire dsp_pat_match_4;


	// Main DSP
	// --------
	// computes (in - cfg_offset) * cfg_scale

	DSP48E1 #(
		.A_INPUT("DIRECT"),
		.B_INPUT("DIRECT"),
		.USE_DPORT("TRUE"),
		.USE_MULT("MULTIPLY"),
		.AUTORESET_PATDET("NO_RESET"),
		.MASK({1'b1, {(46-TBI){1'b0}}, {(TBI+1){1'b1}}}),
		.PATTERN(48'h000000000000),
		.SEL_MASK("MASK"),
		.SEL_PATTERN("PATTERN"),
		.USE_PATTERN_DETECT("PATDET"),
		.ACASCREG(1),
		.ADREG(1),
		.ALUMODEREG(1),
		.AREG(1),
		.BCASCREG(2),
		.BREG(2),
		.CARRYINREG(1),
		.CARRYINSELREG(1),
		.CREG(1),
		.DREG(1),
		.INMODEREG(1),
		.MREG(1),
		.OPMODEREG(1),
		.PREG(1),
		.USE_SIMD("ONE48")
	)
	dsp_binmap_I (
		.PATTERNDETECT(dsp_pat_match_4),
		.P(dsp_pout_4),
		.ACIN(30'h0),
		.BCIN(18'h0),
		.CARRYCASCIN(1'h0),
		.MULTSIGNIN(1'h0),
		.PCIN(48'h000000000000),
		.ALUMODE(4'b0000),		// Z + X + Y + CIN
		.CARRYINSEL(3'h0),
		.CEINMODE(1'b1),
		.CLK(clk),
		.INMODE(5'b01100),		// B=B2, A=D-A2
		.OPMODE(7'b0000101),	// X=M1, Y=M2, Z=0
		.RSTINMODE(rst),
		.A({14'h0, offset_0}),
		.B({ 2'h0, scale_0}),
		.C({48'h0}),
		.CARRYIN(1'b0),
		.D({ 9'h0, in_0}),
		.CEA1(1'b0),
		.CEA2(1'b1),
		.CEAD(1'b1),
		.CEALUMODE(1'b1),
		.CEB1(1'b1),
		.CEB2(1'b1),
		.CEC(1'b1),
		.CECARRYIN(1'b1),
		.CECTRL(1'b1),
		.CED(1'b1),
		.CEM(1'b1),
		.CEP(1'b1),
		.RSTA(rst),
		.RSTALLCARRYIN(rst),
		.RSTALUMODE(rst),
		.RSTB(rst),
		.RSTC(rst),
		.RSTCTRL(rst),
		.RSTD(rst),
		.RSTM(rst),
		.RSTP(rst)
	);


	// Post-DSP mapping & saturation
	// -----------------------------

	always @(posedge clk)
	begin
		if (rst == 1) begin
			bin_5 <= 0;
			sat_ind_5 <= 0;
		end else begin
			// Undeflow
			if (dsp_pout_4[47] == 1) begin
				bin_5 <= {BIN_WIDTH{1'b0}};
				sat_ind_5 <= 1;

			// Overflow
			end else if (dsp_pat_match_4 == 0) begin
				bin_5 <= {BIN_WIDTH{1'b1}};
				sat_ind_5 <= 1;

			// In-range
			end else begin
				bin_5 <= dsp_pout_4[TBI:TBI-BIN_WIDTH+1];
				sat_ind_5 <= 0;
			end
		end
	end

endmodule // f15_binmap
