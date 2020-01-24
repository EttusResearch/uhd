/*
 * f15_rise_decay.v
 *
 * Applies the rise or decay to a given value.
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

module f15_rise_decay #(
	parameter integer WIDTH = 9
)(
	input  wire [WIDTH-1:0] in_0,	// input
	output reg  [WIDTH-1:0] out_5,	// output
	input  wire [15:0] k_0,			// time constant
	input  wire ena_0,				// If ena=0, then output original value
	input  wire mode_0,				// 0=rise, 1=decay
	input  wire [15:0] rng,
	input  wire clk,
	input  wire rst
);

	// Signals
	reg mode_1;
	reg [4:0] inmode_1;

	wire [WIDTH-1:0] in_2;
	wire ena_2;
	wire [6:0] opmode_2;
	reg  [3:0] alumode_2;

	wire [47:0] pout_4;
	wire pmatch_4;


	// Main DSP
	// --------

	// Mode control
		// For rise  we have INMODE=00000 (A=A2,   B=B2), ALUMODE=0000 (C+M)
		// For decay we have INMODE=01100 (A=D-A2, B=B2), ALUMODE=0011 (C-M)
	always @(posedge clk)
	begin
		mode_1 <= mode_0;

		if (mode_0)
			inmode_1 <= 5'b00000;
		else
			inmode_1 <= 5'b01100;

		if (mode_1)
			alumode_2 <= 4'b0011;
		else
			alumode_2 <= 4'b0000;
	end

		// When not enabled, we use OPMODE to do pass-through
	delay_bit #(2) dl_ena (ena_0, ena_2, clk);
	assign opmode_2  = ena_2 ? 7'b0110101 : 7'b0110000;

	// Delay for input to C
	delay_bus #(2, WIDTH) dl_in (in_0, in_2, clk);

	// Instance
	DSP48E1 #(
		.A_INPUT("DIRECT"),
		.B_INPUT("DIRECT"),
		.USE_DPORT("TRUE"),
		.USE_MULT("MULTIPLY"),
		.AUTORESET_PATDET("NO_RESET"),
		.MASK({1'b1, {(31-WIDTH){1'b0}}, {(WIDTH+16){1'b1}}}),
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
	dsp_exp_I (
		.PATTERNDETECT(pmatch_4),
		.P(pout_4),
		.ACIN(30'h0),
		.BCIN(18'h0),
		.CARRYCASCIN(1'h0),
		.MULTSIGNIN(1'h0),
		.PCIN(48'h000000000000),
		.ALUMODE(alumode_2),
		.CARRYINSEL(3'h0),
		.CEINMODE(1'b1),
		.CLK(clk),
		.INMODE(inmode_1),
		.OPMODE(opmode_2),
		.RSTINMODE(rst),
		.A({{(30-WIDTH){1'b0}}, in_0}),
		.B({ 2'h0, k_0}),
		.C({{(32-WIDTH){1'b0}}, in_2, rng}),
		.CARRYIN(1'b0),
		.D({{(24-WIDTH){1'b0}}, 1'b1, {WIDTH{1'b0}}}),
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


	// Saturation
	// ----------

	always @(posedge clk)
	begin
		if (rst == 1)
			out_5 <= 0;
		else
			if (pout_4[47] == 1)
				out_5 <= {WIDTH{1'b0}};
			else if (pmatch_4 == 0)
				out_5 <= {WIDTH{1'b1}};
			else
				out_5 <= pout_4[WIDTH+15:16];
	end

endmodule // f15_rise_decay
