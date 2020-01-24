/*
 * delay.v
 *
 * Generates a delay line/bus using a combination of SRL and Register
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
// Single line delay
// ---------------------------------------------------------------------------

module delay_bit #(
	parameter integer DELAY = 1
)(
	input  wire d,
	output wire q,
	input  wire clk
);

	// Signals
	wire [4:0] addr = DELAY - 2;
	wire ff_in;

	// Generate SRL if needed (or bypass if not)
	generate
		if (DELAY > 17) begin
			SRLC32E srl_I (
				.Q(ff_in),
				.A(addr),
				.CE(1'b1),
				.CLK(clk),
				.D(d)
			);
		end else if (DELAY > 1) begin
			SRL16E srl_I (
				.Q(ff_in),
				.A0(addr[0]),
				.A1(addr[1]),
				.A2(addr[2]),
				.A3(addr[3]),
				.CE(1'b1),
				.CLK(clk),
				.D(d)
			);
		end else begin
			assign ff_in = d;
		end
	endgenerate

	// Generate flip-flop if needed (or bypass if not)
	generate
		if (DELAY > 0) begin
			FDRE ff_I (
				.Q(q),
				.C(clk),
				.CE(1'b1),
				.D(ff_in),
				.R(1'b0)
			);
		end else begin
			assign q = ff_in;
		end
	endgenerate

endmodule // delay_bit


// ---------------------------------------------------------------------------
// Bus delay
// ---------------------------------------------------------------------------

module delay_bus #(
	parameter integer DELAY = 1,
	parameter integer WIDTH = 1
)(
	input  wire [WIDTH-1:0] d,
	output wire [WIDTH-1:0] q,
	input  wire clk
);
	genvar i;

	// Variables / Signals
	wire [4:0] addr = DELAY - 2;
	wire [WIDTH-1:0] ff_in;

	// Generate SRL if needed (or bypass if not)
	generate
		if (DELAY > 17) begin
			for (i=0; i<WIDTH; i=i+1)
				SRLC32E srl_I (
					.Q(ff_in[i]),
					.A(addr),
					.CE(1'b1),
					.CLK(clk),
					.D(d[i])
				);
		end else if (DELAY > 1) begin
			for (i=0; i<WIDTH; i=i+1)
				SRL16E srl_I (
					.Q(ff_in[i]),
					.A0(addr[0]),
					.A1(addr[1]),
					.A2(addr[2]),
					.A3(addr[3]),
					.CE(1'b1),
					.CLK(clk),
					.D(d[i])
				);
		end else begin
			assign ff_in = d;
		end
	endgenerate

	// Generate flip-flop if needed (or bypass if not)
	generate
		if (DELAY > 0) begin
			for (i=0; i<WIDTH; i=i+1)
				FDRE ff_I (
					.Q(q[i]),
					.C(clk),
					.CE(1'b1),
					.D(ff_in[i]),
					.R(1'b0)
				);
		end else begin
			assign q = ff_in;
		end
	endgenerate

endmodule // delay_bus
