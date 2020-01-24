/*
 * rng.v
 *
 * Very simple 32-bits PRNG using a few underlying LFSR.
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

// ---------------------------------------------------------------------------
// Main RNG
// ---------------------------------------------------------------------------

module rng(
	output reg  [31:0] out,
	input  wire clk,
	input  wire rst
);

	// Signals
	wire [4:0] out5, out5rev;
	wire [7:0] out8;
	wire [11:0] out12;
	wire [15:0] out16;

	// Instanciate 4 LFSRs of different lengths
	lfsr #(.WIDTH( 5), .POLY( 5'b01001)) lfsr5  (.out(out5),  .clk(clk), .rst(rst));
	lfsr #(.WIDTH( 8), .POLY( 8'h71   )) lfsr8  (.out(out8),  .clk(clk), .rst(rst));
	lfsr #(.WIDTH(12), .POLY(12'hc11  )) lfsr12 (.out(out12), .clk(clk), .rst(rst));
	lfsr #(.WIDTH(16), .POLY(16'h6701 )) lfsr16 (.out(out16), .clk(clk), .rst(rst));

	// Reverse the 5 bit LFSR output
	genvar i;
	generate
		for (i=0; i<5; i=i+1)
			assign out5rev[i] = out5[4-i];
	endgenerate

	// Combine the outputs 'somehow'
	always @(posedge clk)
		out <= {
			out16[15:11] ^ out5rev,		// 5 bits
			out16[10:2],				// 9 bits
			out16[1:0] ^ out12[11:10],	// 2 bits
			out12[9:2],					// 8 bits
			out12[1:0] ^ out8[7:6],		// 2 bits
			out8[5:0]					// 6 bits
		};

endmodule // rng


// ---------------------------------------------------------------------------
// LFSR sub module
// ---------------------------------------------------------------------------

module lfsr #(
	parameter integer WIDTH = 8,
	parameter POLY = 8'h71
)(
	output reg  [WIDTH-1:0] out,
	input  wire clk,
	input  wire rst
);

	// Signals
	wire fb;

	// Linear Feedback
	assign fb = ^(out & POLY);

	// Register
	always @(posedge clk)
		if (rst)
			out <= { {(WIDTH-1){1'b0}}, 1'b1 };
		else
			out <= { fb, out[WIDTH-1:1] };

endmodule // lfsr
