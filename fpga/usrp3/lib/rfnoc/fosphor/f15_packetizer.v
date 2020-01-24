/*
 * f15_packetizer.v
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

module f15_packetizer #(
	parameter integer BIN_WIDTH = 6,
	parameter integer DECIM_WIDTH = 10
)(
	input  wire [BIN_WIDTH-1:0] in_bin_addr,
	input  wire in_bin_last,
	input  wire [7:0] in_histo,
	input  wire [7:0] in_spectra_max,
	input  wire [7:0] in_spectra_avg,
	input  wire in_last,
	input  wire in_valid,

	output reg [31:0] out_data,
	output reg out_last,
	output reg out_eob,
	output reg out_valid,

	input  wire [DECIM_WIDTH-1:0] cfg_decim,
	input  wire cfg_decim_changed,

	input  wire clk,
	input  wire rst
);

	// FSM
	localparam
		ST_WAIT = 0,
		ST_SEND_HISTO = 1,
		ST_SEND_MAX = 2,
		ST_SEND_AVG = 3;

	reg [1:0] state;

	// Signals
	reg [DECIM_WIDTH:0] decim_cnt;
	reg [1:0] bcnt;

	// 1-in-N decimation counter
	always @(posedge clk)
	begin
		if (rst)
			decim_cnt <= 0;
		else if (cfg_decim_changed)
			// Force Reload
			decim_cnt <= { 1'b0, cfg_decim };
		else if (in_valid & in_bin_last & in_last)
			if (decim_cnt[DECIM_WIDTH])
				// Reload
				decim_cnt <= { 1'b0, cfg_decim };
			else
				// Just decrement
				decim_cnt <= decim_cnt - 1;
	end

	// FSM
	always @(posedge clk)
	begin
		if (rst)
			state <= ST_WAIT;
		else if (in_valid & in_last)
			case (state)
				ST_WAIT:
					if (in_bin_last & decim_cnt[DECIM_WIDTH])
						state <= ST_SEND_HISTO;

				ST_SEND_HISTO:
					if (in_bin_last)
						state <= ST_SEND_MAX;

				ST_SEND_MAX:
					state <= ST_SEND_AVG;

				ST_SEND_AVG:
					state <= ST_WAIT;
			endcase
	end

	// Byte counter
	always @(posedge clk)
	begin
		if (rst)
			bcnt <= 2'b00;
		else if (in_valid)
			if (in_last | (bcnt == 2'b11))
				bcnt <= 2'b00;
			else
				bcnt <= bcnt + 1;
	end

	// Input mux & shift register
	always @(posedge clk)
	begin
		if (in_valid)
		begin
			// Shift
			out_data[31:8] <= out_data[23:0];

			// New LSBs
			case (state)
				ST_SEND_HISTO: out_data[7:0] <= in_histo;
				ST_SEND_MAX:   out_data[7:0] <= in_spectra_max;
				ST_SEND_AVG:   out_data[7:0] <= in_spectra_avg;
			endcase
		end
	end

	// Output last, eob, valid
	always @(posedge clk)
	begin
		if (rst) begin
			out_last  <= 1'b0;
			out_eob   <= 1'b0;
			out_valid <= 1'b0;
		end else begin
			out_last  <= in_last;
			out_eob   <= (state == ST_SEND_AVG);
			out_valid <= in_valid & (in_last | bcnt == 2'b11) & (state != ST_WAIT);
		end
	end

endmodule // f15_packetizer
