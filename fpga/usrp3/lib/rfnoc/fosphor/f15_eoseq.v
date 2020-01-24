/*
 * f15_eoseq.v
 *
 * Resequence a data flow with data/valid/last ensuring EVEN/ODD
 * sequencing (even data on even cycles, odd data on odd cycles)
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

module f15_eoseq #(
	parameter integer WIDTH = 16
)(
	input  wire [WIDTH-1:0] in_data,
	input  wire in_valid,
	input  wire in_last,
	output reg  [WIDTH-1:0] out_data,
	output reg  out_valid,
	output reg  out_last,
	input  wire clk,
	input  wire rst
);

	// Signals
	reg [WIDTH-1:0] buf_data;
	reg buf_valid;
	reg buf_last;

	wire flip;
	reg odd;
	reg sel;

	// Control
	always @(posedge clk)
		if (rst)
			odd <= 1'b0;
		else
			odd <= ~(in_last & in_valid) & (odd ^ in_valid);

	always @(posedge clk)
		if (rst)
			sel <= 1'b0;
		else if (flip)
			sel <= ~sel;

	assign flip = ~in_valid | (in_last & ~odd);

	// Buffer
	always @(posedge clk)
	begin
		buf_data  <= in_data;
		buf_valid <= in_valid;
		buf_last  <= in_last;
	end

	// Output
	always @(posedge clk)
	begin
		if (sel) begin
			out_data  <= buf_data;
			out_valid <= buf_valid;
			out_last  <= buf_last;
		end else begin
			out_data  <= in_data;
			out_valid <= in_valid;
			out_last  <= in_last;
		end
	end

endmodule // f15_eoseq
