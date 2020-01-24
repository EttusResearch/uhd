/*
 * fifo_srl.v
 *
 * Very small/light-weight FIFO using SRL.
 * Only for synchronous design. Has a fixed depth of 15 or 31 entries and
 * always work in the so-called first-word-fall-thru mode.
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

module fifo_srl #(
	parameter integer WIDTH = 4,
	parameter integer LOG2_DEPTH = 5,	// 4 or 5
	parameter integer AFULL_LEVEL = -1	// -1 -> No AFULL

)(
	input  wire [WIDTH-1:0] di,
	input  wire wren,
	output wire full,
	output wire afull,

	output reg  [WIDTH-1:0] do,
	input  wire rden,
	output reg  empty,

	input  wire clk,
	input  wire rst
);

	genvar i;

	// Signals
	wire [WIDTH-1:0] srl_q;
	reg  [LOG2_DEPTH-1:0] srl_addr;
	wire srl_addr_ce;

	wire srl_write;
	wire srl_read;

	wire srl_full;
	wire srl_afull;
	reg  srl_empty;
	wire srl_aempty;

	// Instanciate the SRLs
	generate
		if (LOG2_DEPTH == 6) begin
			wire [WIDTH-1:0] srl0_q31, srl0_q, srl1_q;

			for (i=0; i<WIDTH; i=i+1)
			begin : srl_64
				SRLC32E srl_I0 (
					.Q(srl0_q[i]),
					.Q31(srl0_q31[i]),
					.A(srl_addr[4:0]),
					.CE(srl_write),
					.CLK(clk),
					.D(di[i])
				);

				SRLC32E srl_I1 (
					.Q(srl1_q[i]),
					.A(srl_addr[4:0]),
					.CE(srl_write),
					.CLK(clk),
					.D(srl0_q31[i])
				);

				MUXF7 mux_I (
					.O(srl_q[i]),
					.I0(srl0_q[i]),
					.I1(srl1_q[i]),
					.S(srl_addr[5])
				);
			end
		end else if (LOG2_DEPTH == 5) begin
			for (i=0; i<WIDTH; i=i+1)
				SRLC32E srl_I (
					.Q(srl_q[i]),
					.A(srl_addr),
					.CE(srl_write),
					.CLK(clk),
					.D(di[i])
				);
		end else if (LOG2_DEPTH == 4) begin
			for (i=0; i<WIDTH; i=i+1)
				SRL16E srl_I (
					.Q(srl_q[i]),
					.A0(srl_addr[0]),
					.A1(srl_addr[1]),
					.A2(srl_addr[2]),
					.A3(srl_addr[3]),
					.CE(srl_write),
					.CLK(clk),
					.D(di[i])
				);
		end
	endgenerate

	// Address counter
	assign srl_addr_ce = srl_write ^ srl_read;

	always @(posedge clk)
	begin
		if (rst)
			srl_addr <= {LOG2_DEPTH{1'b1}};
		else if (srl_addr_ce) begin
			if (srl_write)
				srl_addr <= srl_addr + 1;
			else
				srl_addr <= srl_addr - 1;
		end
	end

	// SRL status
	assign srl_full = srl_addr == {{(LOG2_DEPTH-1){1'b1}}, 1'b0};

	generate
		if (AFULL_LEVEL != -1) begin
			assign srl_afull = (srl_addr >= AFULL_LEVEL) && ~&(srl_addr);
		end else begin
			assign srl_afull = 1'b0;
		end
	endgenerate

	assign srl_aempty = &(~srl_addr);

	always @(posedge clk)
	begin
		if (rst)
			srl_empty <= 1'b1;
		else if (srl_addr_ce)
			srl_empty <= srl_aempty & srl_read;
	end

	// Output register (to capture whatever comes out from SRL)
	always @(posedge clk)
	begin
		if (srl_read)
			do <= srl_q;
	end

	// Control and flag generation
		// Write/Full is easy
	assign srl_write = wren;
	assign full = srl_full;
	assign afull = srl_afull;

		// Read/Empty is tricky
	always @(posedge clk)
	begin
		if (rst)
			empty <= 1'b1;
		else if (rden | srl_read)
			empty <= srl_empty;
	end

	assign srl_read = (rden | empty) & ~srl_empty;

endmodule // fifo_srl
