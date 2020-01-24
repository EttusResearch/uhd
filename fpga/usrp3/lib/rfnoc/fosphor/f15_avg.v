/*
 * f15_avg.v
 *
 * Applies the y(t+1) = alpha * y(t) + (1 - alpha) * x(t)
 * to compute an IIR average
 *
 * Copyright (C) 2015  Ettus Corporation LLC
 * Copyright 2018 Ettus Research, a National Instruments Company
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * vim: ts=4 sw=4
 */

`ifdef SIM
`default_nettype none
`endif

module f15_avg #(
	parameter integer Y_WIDTH = 12,
	parameter integer X_WIDTH = 16
)(
	input  wire [Y_WIDTH-1:0] yin_0,
	input  wire [X_WIDTH-1:0] x_0,
	input  wire [15:0] rng_0,
	input  wire [15:0] alpha_0,
	input  wire clear_0,
	output wire [Y_WIDTH-1:0] yout_4,
	input  wire clk,
	input  wire rst
);

	// Signals
	wire [X_WIDTH-1:0] x_2;
	wire clear_3;
	wire [47:0] pout_4;

	// Main DSP Instance
	DSP48E1 #(
		.A_INPUT("DIRECT"),
		.B_INPUT("DIRECT"),
		.USE_DPORT("TRUE"),
		.USE_MULT("MULTIPLY"),
		.AUTORESET_PATDET("NO_RESET"),
		.MASK(48'h3fffffffffff),
		.PATTERN(48'h000000000000),
		.SEL_MASK("MASK"),
		.SEL_PATTERN("PATTERN"),
		.USE_PATTERN_DETECT("PATDET"),
		.ACASCREG(1),
		.ADREG(1),
		.ALUMODEREG(1),
		.AREG(1),
		.BCASCREG(1),
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
	dsp_avg_I (
		.P(pout_4),
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
		.OPMODE(7'b0110101),	// X=M1, Y=M2, Z=C
		.RSTINMODE(rst),
		.A({{(30-X_WIDTH){1'b0}}, x_0}),
		.B({2'h0, alpha_0}),
		.C({{(32-X_WIDTH){1'b0}}, x_2, 16'h8000}),
		.CARRYIN(1'b0),
		.D({{(25-X_WIDTH){1'b0}}, yin_0, rng_0[X_WIDTH-Y_WIDTH-1:0]}),
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
		.RSTP(clear_3)
	);

	// Delay x for the C input
	delay_bus #(2, X_WIDTH) dl_x (x_0, x_2, clk);

	// Delay clear to use as reset for P
	delay_bit #(3) dl_clear (clear_0, clear_3, clk);

	// Map the output
	assign yout_4 = pout_4[X_WIDTH+15:X_WIDTH-Y_WIDTH+16];

endmodule // f15_avg
