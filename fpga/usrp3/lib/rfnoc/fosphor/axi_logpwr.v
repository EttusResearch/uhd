/*
 * axi_logpwr.v
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

module axi_logpwr #(
	parameter [1:0] RANDOM_MODE = 2'b11
)(
	input  clk, input reset,
	input  [31:0] i_tdata, input  i_tlast, input  i_tvalid, output i_tready,
	output [15:0] o_tdata, output o_tlast, output o_tvalid, input  o_tready
);

	// Signals
	reg ready;
	reg valid_1;
	wire valid_12;
	wire last_12;

	wire [31:0] rng;

	wire [15:0] in_real_0;
	wire [15:0] in_imag_0;
	wire [15:0] out_logpwr_12;

	wire [16:0] fifo_di;
	wire [16:0] fifo_do;
	wire fifo_wren;
	wire fifo_afull;
	wire fifo_rden;
	wire fifo_empty;

 	// Input control
	assign in_real_0 = i_tdata[31:16];
	assign in_imag_0 = i_tdata[15:0];

	always @(posedge clk)
	begin
		ready <= ~fifo_afull | o_tready;
		valid_1 <= i_tvalid & ready;
	end

	assign i_tready = ready;

	// Delays
	delay_bit #(11) dl_valid (valid_1, valid_12, clk);
	delay_bit #(12) dl_last  (i_tlast, last_12, clk);

	// RNG Instance
    rng rng_I (
		.out(rng),
		.clk(clk),
		.rst(reset)
	);

	// logpwr Instance
	f15_logpwr logpwr_I (
		.in_real_0(in_real_0),
		.in_imag_0(in_imag_0),
		.out_12(out_logpwr_12),
		.rng(rng),
		.random_mode(RANDOM_MODE),
		.clk(clk),
		.rst(reset)
	);

	// Output FIFO
	assign fifo_di   = { last_12, out_logpwr_12 };
	assign fifo_wren = { valid_12 };

	fifo_srl #(
		.WIDTH(17),
		.LOG2_DEPTH(6),
		.AFULL_LEVEL(49)
	) fifo_I (
		.di(fifo_di),
		.wren(fifo_wren),
		.afull(fifo_afull),
		.do(fifo_do),
		.rden(fifo_rden),
		.empty(fifo_empty),
		.clk(clk),
		.rst(reset)
	);

	assign o_tdata  = fifo_do[15:0];
	assign o_tlast  = fifo_do[16];
	assign o_tvalid = ~fifo_empty;

	assign fifo_rden = ~fifo_empty & o_tready;

endmodule // axi_logpwr
