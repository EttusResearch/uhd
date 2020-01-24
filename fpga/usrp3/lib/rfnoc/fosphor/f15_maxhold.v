/*
 * f15_maxhold.v
 *
 * Computes the max hold (with epsilon decay)
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

module f15_maxhold #(
	parameter integer Y_WIDTH = 12,
	parameter integer X_WIDTH = 16,
	parameter integer FRAC_WIDTH = 8
)(
	input  wire [Y_WIDTH-1:0] yin_0,
	input  wire [X_WIDTH-1:0] x_0,
	input  wire [15:0] rng_0,
	input  wire [15:0] epsilon_0,
	input  wire clear_0,
	output wire [Y_WIDTH-1:0] yout_4,
	input  wire clk,
	input  wire rst
);

	 localparam integer I_WIDTH = X_WIDTH + FRAC_WIDTH;

	// Signals
	reg [X_WIDTH-1:0] x_1;
	reg [I_WIDTH  :0] y_1;
	reg [Y_WIDTH  :0] d_1;
	reg clear_1;

	reg [Y_WIDTH-1:0] y_2;

	// Stage 1
	always @(posedge clk)
	begin
		x_1 <= x_0;
		y_1 <= { 1'b0, yin_0, rng_0[I_WIDTH-Y_WIDTH-1:0] } - epsilon_0;
		d_1 <= { 1'b0, yin_0 } - { 1'b0, x_0[X_WIDTH-1:X_WIDTH-Y_WIDTH] };
		clear_1 <= clear_0;
	end

	// Stage 2
	always @(posedge clk)
	begin
		if (clear_1)
			y_2 <= 0;
		else if (d_1[Y_WIDTH])
			// x is larger, use this
			y_2 <= x_1[X_WIDTH-1:X_WIDTH-Y_WIDTH];
		else
			// y is larger, take old y with small decay
			if (y_1[I_WIDTH])
				y_2 <= 0;
			else
				y_2 <= y_1[I_WIDTH-1:I_WIDTH-Y_WIDTH];
	end

	// Apply two more delay to match the avg block
	delay_bus #(2, Y_WIDTH) dl_y (y_2, yout_4, clk);

endmodule // f15_maxhold
