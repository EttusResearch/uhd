/*
 * f15_line_mem.v
 *
 * Memory for a single line to compute max-hold / average
 * Read latency is 2 and if read is not enabled, output data is forced
 * to zero.
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

module f15_line_mem #(
	parameter integer AWIDTH = 12,
    parameter integer DWIDTH = 18
)(
	input  wire [AWIDTH-1:0] rd_addr,
	output reg  [DWIDTH-1:0] rd_data,
	input  wire rd_ena,

	input  wire [AWIDTH-1:0] wr_addr,
	input  wire [DWIDTH-1:0] wr_data,
	input  wire wr_ena,

	input  wire clk,
	input  wire rst
);

	// Signals
	reg [DWIDTH-1:0] ram [(1<<AWIDTH)-1:0];
	reg [DWIDTH-1:0] rd_data_r;
	reg rd_ena_r;

`ifdef SIM
	integer i;
	initial
		for (i=0; i<(1<<AWIDTH); i=i+1)
			ram[i] = 0;
`endif

	always @(posedge clk)
	begin
		// Read
		rd_data_r <= ram[rd_addr];

		// Write
		if (wr_ena)
			ram[wr_addr] <= wr_data;

		// Register the enable flag
		rd_ena_r <= rd_ena;

		// Final read register
		if (rd_ena_r)
			rd_data <= rd_data_r;
		else
			rd_data <= 0;
	end

endmodule // f15_line_mem
