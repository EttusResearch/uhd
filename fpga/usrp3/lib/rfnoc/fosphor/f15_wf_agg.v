/*
 * f15_wf_agg.v
 *
 * Watefall Aggregation
 *
 * Copyright (C) 2016  Ettus Corporation LLC
 *
 * vim: ts=4 sw=4
 */

`ifdef SIM
`default_nettype none
`endif

module f15_wf_agg #(
	parameter integer Y_WIDTH = 12,
	parameter integer X_WIDTH = 16,
	parameter integer DECIM_WIDTH = 8
)(
	input wire [Y_WIDTH-1:0] yin_0,
	input wire [X_WIDTH-1:0] x_0,
	input wire valid_0,
	input wire last_0,
	input wire [15:0] rng_0,

	output wire [Y_WIDTH-1:0] yout_3,
	output wire [7:0] zout_3,
	output wire zvalid_3,

	input wire [1:0] cfg_div,
	input wire cfg_mode,		// 0=MaxHold, 1=Average
	input wire [DECIM_WIDTH-1:0] cfg_decim,
	input wire cfg_decim_changed,

	input wire clk,
	input wire rst
);

	localparam integer R_WIDTH = X_WIDTH + 9;

	// Signals
		// Data pah
	reg [R_WIDTH-1:0] xe_1;
	reg [R_WIDTH-1:0] ye_1;

	wire over_2;
	reg [R_WIDTH-1:0] r_2;
	reg [Y_WIDTH-1:0] x_2;
	reg [Y_WIDTH-1:0] y_2;

	reg [Y_WIDTH-1:0] y_3;

		// Control
	reg [DECIM_WIDTH:0] decim_cnt;
	reg init_0;
	wire init_2;
	reg init_force_0;
	reg flush_0;
	reg zvalid_1;


	// Datapath
	// --------

	// X predivision mux
	always @(posedge clk)
	begin
		case (cfg_div)
			2'b00:
				xe_1 <= { 1'd0, x_0, 8'd0 };	// 1:1

			2'b01:
				xe_1 <= { 4'd0, x_0, 5'd0 };	// 1:8

			2'b10:
				xe_1 <= { 7'd0, x_0, 2'd0 };	// 1:64

			2'b11:
				xe_1 <= { 9'd0, x_0 };			// 1:256
		endcase
	end

	// Y register
	always @(posedge clk)
	begin
		if (cfg_mode)
			// Average
			ye_1 <= { 1'b0, yin_0, rng_0[R_WIDTH-Y_WIDTH-2:0] };
		else
			// Max Hold
			ye_1 <= { 1'b0, yin_0, {(R_WIDTH-Y_WIDTH-1){1'b0}} };
	end

	// Adder / Substractor
	always @(posedge clk)
	begin
		if (cfg_mode)
			// Average
			r_2 <= ye_1 + xe_1;
		else
			// Max-Hold
			r_2 <= ye_1 - xe_1;
	end

	assign over_2 = r_2[R_WIDTH-1];

	// Registers for the two branches.
	always @(posedge clk)
	begin
		x_2 <= xe_1[R_WIDTH-2:R_WIDTH-Y_WIDTH-1];
		y_2 <= ye_1[R_WIDTH-2:R_WIDTH-Y_WIDTH-1];
	end

	// Output mux
	always @(posedge clk)
	begin
		// If first : take x_2
		// If average :
		//  - If overflow = 0 -> take r_2
		//  - If overflow = 1 -> sature to all 1's
		// If max-hold
		//  - If overflow = 0 -> take y_2
		//  - If overflow = 1 -> take x_2
		if (init_2)
			y_3 <= x_2;
		else if (cfg_mode)
			y_3 <= over_2 ? { (Y_WIDTH){1'b1} } : r_2[R_WIDTH-2:R_WIDTH-Y_WIDTH-1];
		else
			y_3 <= over_2 ? x_2 : y_2;
	end

	assign yout_3 = y_3;
	assign zout_3 = y_3[Y_WIDTH-1:Y_WIDTH-8];


	// Control
	// -------

	// 1-in-N decimation counter
	always @(posedge clk)
	begin
		if (rst)
			decim_cnt <= 0;
		else if (cfg_decim_changed)
			// Force Reload
			decim_cnt <= { 1'b0, cfg_decim };
		else if (valid_0 & last_0)
			if (decim_cnt[DECIM_WIDTH])
				// Reload
				decim_cnt <= { 1'b0, cfg_decim };
			else
				// Just decrement
				decim_cnt <= decim_cnt - 1;
	end

	// Decimation flush & init states
	always @(posedge clk)
	begin
		if (rst) begin
			// Initial state
			flush_0 <= 1'b0;
			init_0  <= 1'b1;
			init_force_0 <= 1'b0;
		end else begin
			if (valid_0 & last_0) begin
				// Flushing
				flush_0 <= decim_cnt[DECIM_WIDTH];

				// Init after flush or if forced
				init_0 <= flush_0 | init_force_0;
			end

			// Init forcing after a decim change
			if (cfg_decim_changed)
				init_force_0 <= 1'b1;
			else if (valid_0 & last_0)
				init_force_0 <= 1'b0;
		end
	end

	delay_bit #(2) dl_init(init_0, init_2, clk);

	// Z-output valid
	always @(posedge clk)
		zvalid_1 <= valid_0 & flush_0;

	delay_bit #(2) dl_zvalid(zvalid_1, zvalid_3, clk);

endmodule // f15_wf_agg
