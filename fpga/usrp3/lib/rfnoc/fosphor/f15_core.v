/*
 * f15_core.v
 *
 * Core of the fosphor IP
 *
 * Copyright (C) 2014,2015  Ettus Corporation LLC
 * Copyright 2018 Ettus Research, a National Instruments Company
 *
 * SPDX-License-Identifier: LGPL-3.0-or-later
 *
 * vim: ts=4 sw=4
 */

`ifdef SIM
`default_nettype none
`endif

module f15_core (
	input  clk, input reset,
	input  clear_req,
	input  [ 1:0] cfg_random,
	input  [15:0] cfg_offset, input [15:0] cfg_scale,
	input  [15:0] cfg_trise,  input [15:0] cfg_tdecay,
	input  [15:0] cfg_alpha,  input [15:0] cfg_epsilon,
	input  [11:0] cfg_decim, input cfg_decim_changed,
	input  [ 1:0] cfg_wf_div, input cfg_wf_mode,
	input  [ 7:0] cfg_wf_decim, input cfg_wf_decim_changed,
	input  [31:0] i_tdata, input  i_tlast, input  i_tvalid, output i_tready,
	output [31:0] o_hist_tdata, output o_hist_tlast, output o_hist_tvalid, input o_hist_tready, output o_hist_teob,
	output [31:0] o_wf_tdata,   output o_wf_tlast,   output o_wf_tvalid,   input o_wf_tready
);
	// Signals
	reg [31:0] in_data;
	reg in_last;
	reg in_valid;
	reg in_ready;

	wire [15:0] proc_real_0, proc_imag_0;
	wire [15:0] proc_logpwr_12, proc_logpwr_end;
	wire proc_last_0, proc_last_12, proc_last_end;
	wire proc_valid_0, proc_valid_12, proc_valid_end;

	reg [5:0] proc_binscan_addr_end;
	reg proc_binscan_last_end;
	reg proc_clear_end;
	reg clear_pending;

	wire rise_last_0, rise_last_15;
	wire rise_valid_0, rise_valid_15, rise_valid_24;
	wire [15:0] rise_logpwr_0;
	wire [5:0]  rise_pwrbin_5, rise_pwrbin_15;
	reg  [9:0]  rise_addr_lsb_15;
	wire [15:0] rise_addr_15, rise_addr_24;
	wire [8:0]  rise_intensity_18, rise_intensity_23;
	reg  [8:0]  rise_intensity_24;

	wire decay_last_0, decay_last_9;
	wire decay_valid_0, decay_valid_9;
	reg  [ 9:0] decay_addr_lsb_0;
	wire [15:0] decay_addr_0, decay_addr_9;
	wire [8:0]  decay_intensity_3, decay_intensity_8;
	reg  [8:0]  decay_intensity_9;
	wire decay_clear_0, decay_clear_9;

	reg  [10:0] sls_addr_0;
	wire [10:0] sls_addr_6;
	wire [35:0] sls_data_2, sls_data_6;
	wire sls_last_0;
	wire sls_valid_0, sls_valid_6;

	wire [15:0] avgmh_logpwr_0, avgmh_logpwr_2;
	wire avgmh_clear_0, avgmh_clear_2;
	wire [11:0] avgmh_avg_2, avgmh_avg_6, avgmh_avg_9;
	wire [11:0] avgmh_max_2, avgmh_max_6, avgmh_max_9;

	wire [5:0] out_binaddr_0, out_binaddr_9;
	wire out_binlast_0, out_binlast_9;
	wire [33:0] out_hist_fifo_di;
	wire out_hist_fifo_wren;
	wire out_hist_fifo_afull;
	wire [33:0] out_hist_fifo_do;
	wire out_hist_fifo_rden;
	wire out_hist_fifo_empty;

	wire [11:0] wf_data_2, wf_data_5, wf_data_6;
	wire [15:0] wf_logpwr_0, wf_logpwr_2;
	wire [ 7:0] wf_out_data_5;
	wire wf_last_0, wf_last_2, wf_out_last_5;
	wire wf_valid_0, wf_valid_2, wf_out_valid_5;

	reg  [ 1:0] out_wf_cnt;
	reg  [32:0] out_wf_fifo_di;
	reg  out_wf_fifo_wren;
	wire out_wf_fifo_afull;
	wire [32:0] out_wf_fifo_do;
	wire out_wf_fifo_rden;
	wire out_wf_fifo_empty;

	wire [31:0] rng;


	// -----------------------------------------------------------------------
	// Input
	// -----------------------------------------------------------------------

	always @(posedge clk)
	begin
		// Control
		if (reset) begin
			in_valid <= 1'b0;
			in_ready <= 1'b0;
		end else begin
			// Valid flag
			in_valid <= i_tvalid & i_tready;

			// We know we can get a sample if :
			//  - Both outputs consumed a sample
			//  - Both FIFOs have enough space
			in_ready <= (o_hist_tready & o_wf_tready) |
			            (~out_hist_fifo_afull & ~out_wf_fifo_afull);
		end

		// Data pipeline
		in_data <= i_tdata;
		in_last <= i_tlast;
	end

	assign i_tready = in_ready;


	// -----------------------------------------------------------------------
	// Processing chain
	// -----------------------------------------------------------------------

	// Input to this stage
	assign proc_real_0  = in_data[31:16];
	assign proc_imag_0  = in_data[15:0];
	assign proc_last_0  = in_last;
	assign proc_valid_0 = in_valid;

	// Log power
	f15_logpwr logpwr_I (
		.in_real_0(proc_real_0),
		.in_imag_0(proc_imag_0),
		.out_12(proc_logpwr_12),
		.rng(rng),
		.random_mode(cfg_random),
		.clk(clk),
		.rst(reset)
	);

	// Aggregation
		// Not supported ATM but this is where it would be

	// Flag propagation
	delay_bit #(12) dl_proc_last  (proc_last_0,  proc_last_12,  clk);
	delay_bit #(12) dl_proc_valid (proc_valid_0, proc_valid_12, clk);

	// Even/Odd resequencing
	f15_eoseq #(
		.WIDTH(16)
	) eoseq_I (
		.in_data(proc_logpwr_12),
		.in_valid(proc_valid_12),
		.in_last(proc_last_12),
		.out_data(proc_logpwr_end),
		.out_valid(proc_valid_end),
		.out_last(proc_last_end),
		.clk(clk),
		.rst(reset)
	);

	// Bin address counter and clear process
		// We do this here so we can propagate to every other stage with
		// just delay lines
	always @(posedge clk)
	begin
		if (reset) begin
			proc_binscan_addr_end <= 6'd0;
			proc_binscan_last_end <= 1'b0;
		end else if (proc_valid_end & proc_last_end) begin
			proc_binscan_addr_end <= proc_binscan_addr_end + 1;
			proc_binscan_last_end <= (proc_binscan_addr_end == 6'h3e);
		end
	end

	always @(posedge clk)
	begin
		if (reset) begin
			clear_pending  <= 1'b0;
			proc_clear_end <= 1'b0;
		end else begin
			if (proc_valid_end & proc_last_end & proc_binscan_last_end) begin
				clear_pending  <= 1'b0;
				proc_clear_end <= clear_pending;
			end else begin
				clear_pending  <= clear_pending | clear_req;
			end
		end
	end


	// -----------------------------------------------------------------------
	// Rise
	// -----------------------------------------------------------------------

	// Input of this stage
	assign rise_last_0   = proc_last_end;
	assign rise_valid_0  = proc_valid_end;
	assign rise_logpwr_0 = proc_logpwr_end;

	// Power Bin mapping
	f15_binmap #(
		.BIN_WIDTH(6),
		.SCALE_FRAC_BITS(8)
	) binmap_I (
		.in_0(rise_logpwr_0),
		.offset_0(cfg_offset),
		.scale_0(cfg_scale),
		.bin_5(rise_pwrbin_5),
		.sat_ind_5(),				// FIXME: Could be use to disable write ena (configurable)
		.clk(clk),
		.rst(reset)
	);

	// Delay
	// (We need to make sure rise doesn't conflict with decay)
	delay_bus #(10, 6) dl_pwrbin (rise_pwrbin_5, rise_pwrbin_15, clk);
	delay_bit #(15)    dl_valid  (rise_valid_0,  rise_valid_15,  clk);
	delay_bit #(15)    dl_last   (rise_last_0,   rise_last_15,   clk);

	// Address
	always @(posedge clk)
	begin
		if (reset)
			rise_addr_lsb_15[9:0] <= 9'd0;
		else if (rise_valid_15)
			if (rise_last_15)
				rise_addr_lsb_15 <= 9'd0;
			else
				rise_addr_lsb_15 <= rise_addr_lsb_15[9:0] + 1;
	end

	assign rise_addr_15 = { rise_pwrbin_15, rise_addr_lsb_15 };

	// Exponential rise
	f15_rise_decay #(
		.WIDTH(9)
	) rise_I (
		.in_0(rise_intensity_18),
		.out_5(rise_intensity_23),
		.k_0(cfg_trise),
		.ena_0(1'b1),
		.mode_0(1'b0),
		.rng(rng[15:0]),
		.clk(clk),
		.rst(reset)
	);

	// Need one more stage just for proper even/odd interlacing
	always @(posedge clk)
		rise_intensity_24 <= rise_intensity_23;

	// Propagate control
	delay_bit #(9)     dl_rise_valid2 (rise_valid_15, rise_valid_24, clk);
	delay_bus #(9, 16) dl_rise_addr2  (rise_addr_15,  rise_addr_24,  clk);


	// -----------------------------------------------------------------------
	// State storage
	// -----------------------------------------------------------------------

	f15_histo_mem #(
		.ADDR_WIDTH(16)
	) mem_I (
		// Rise readout
		.addr_AR(rise_addr_15),
		.data_AR(rise_intensity_18),
		.ena_AR(rise_valid_15),

		// Rise writeback
		.addr_AW(rise_addr_24),
		.data_AW(rise_intensity_24),
		.ena_AW(rise_valid_24),

		// Decay readout
		.addr_BR(decay_addr_0),
		.data_BR(decay_intensity_3),
		.ena_BR(decay_valid_0),

		// Decay writeback
		.addr_BW(decay_addr_9),
		.data_BW(decay_intensity_9),
		.ena_BW(decay_valid_9),

		// Common
		.clk(clk),
		.rst(reset)
	);


	// -----------------------------------------------------------------------
	// Decay & Clear
	// -----------------------------------------------------------------------

	// Input of this stage
	assign decay_last_0  = proc_last_end;
	assign decay_valid_0 = proc_valid_end;
	assign decay_clear_0 = proc_clear_end;

	// Address generation
	always @(posedge clk)
	begin
		if (reset)
			decay_addr_lsb_0 <= 10'd0;
		else if (decay_valid_0)
			if (decay_last_0)
				decay_addr_lsb_0 <= 10'd0;
			else
				decay_addr_lsb_0 <= decay_addr_lsb_0 + 1;
	end

	assign decay_addr_0 = { proc_binscan_addr_end, decay_addr_lsb_0 };

	// Exponential decay
	f15_rise_decay #(
		.WIDTH(9)
	) decay_I (
		.in_0(decay_intensity_3),
		.out_5(decay_intensity_8),
		.k_0(cfg_tdecay),
		.ena_0(1'b1),
		.mode_0(1'b1),
		.rng(rng[15:0]),
		.clk(clk),
		.rst(reset)
	);

	// Need one more stage just for proper even/odd interlacing
	// Also do the clear in there
	always @(posedge clk)
		if (decay_clear_9)
			decay_intensity_9 <= 9'd0;
		else
			decay_intensity_9 <= decay_intensity_8;

	// Propagate control
	delay_bit #(9)     dl_decay_valid (decay_valid_0, decay_valid_9, clk);
	delay_bit #(9)     dl_decay_last  (decay_last_0,  decay_last_9,  clk);
	delay_bit #(9)     dl_decay_clear (decay_clear_0, decay_clear_9, clk);
	delay_bus #(9, 16) dl_decay_addr  (decay_addr_0,  decay_addr_9,  clk);


	// -----------------------------------------------------------------------
	// Shared line-storage
	// -----------------------------------------------------------------------
		// This is shared between the average/max-hold spectrum lines and the
		// waterfall aggregation

	// Input of this stage
	assign sls_last_0   = proc_last_end;
	assign sls_valid_0  = proc_valid_end;

	// Address
	always @(posedge clk)
	begin
		if (reset)
			sls_addr_0 <= 11'd0;
		else if (sls_valid_0)
			if (sls_last_0)
				sls_addr_0 <= 11'd0;
			else
				sls_addr_0 <= sls_addr_0 + 1;
	end

	delay_bus #(6, 11) dl_sls_addr  (sls_addr_0,  sls_addr_6,  clk);
	delay_bit #(6)     dl_sls_valid (sls_valid_0, sls_valid_6, clk);

	// Storage
	f15_line_mem #(
		.AWIDTH(11),
		.DWIDTH(36)
	) line_mem_I (
		.rd_addr(sls_addr_0),
		.rd_data(sls_data_2),
		.rd_ena(sls_valid_0),
		.wr_addr(sls_addr_6),
		.wr_data(sls_data_6),
		.wr_ena(sls_valid_6),
		.clk(clk),
		.rst(reset)
	);

	// Data mapping
	assign avgmh_avg_2 = sls_data_2[11: 0];
	assign avgmh_max_2 = sls_data_2[23:12];
	assign wf_data_2   = sls_data_2[35:24];

	assign sls_data_6[11: 0] = avgmh_avg_6;
	assign sls_data_6[23:12] = avgmh_max_6;
	assign sls_data_6[35:24] = wf_data_6;


	// -----------------------------------------------------------------------
	// Average and Max-Hold
	// -----------------------------------------------------------------------

	// Input of this stage
	assign avgmh_logpwr_0 = proc_logpwr_end;
	assign avgmh_clear_0  = proc_clear_end;

	// Modify stage: Average
	f15_avg #(
		.Y_WIDTH(12),
		.X_WIDTH(16)
	) avg_I (
		.yin_0(avgmh_avg_2),
		.x_0(avgmh_logpwr_2),
		.rng_0(rng[15:0]),
		.alpha_0(cfg_alpha),
		.clear_0(avgmh_clear_2),
		.yout_4(avgmh_avg_6),
		.clk(clk),
		.rst(reset)
	);

	// Modify stage: Max Hold
	f15_maxhold #(
		.Y_WIDTH(12),
		.X_WIDTH(16),
		.FRAC_WIDTH(8)
	) maxhold_I (
		.yin_0(avgmh_max_2),
		.x_0(avgmh_logpwr_2),
		.rng_0(rng[15:0]),
		.epsilon_0(cfg_epsilon),
		.clear_0(avgmh_clear_2),
		.yout_4(avgmh_max_6),
		.clk(clk),
		.rst(reset)
	);

	// Delays
	delay_bus #(2, 16) dl_avgmh_logpwr (avgmh_logpwr_0, avgmh_logpwr_2, clk);
	delay_bit #(2)     dl_avgmh_clear  (avgmh_clear_0,  avgmh_clear_2,  clk);
	delay_bus #(3, 12) dl_avgmh_max    (avgmh_max_6,    avgmh_max_9,    clk);
	delay_bus #(3, 12) dl_avgmh_avg    (avgmh_avg_6,    avgmh_avg_9,    clk);


	// -----------------------------------------------------------------------
	// Histogram Output
	// -----------------------------------------------------------------------

		// For the 'tap' to work, we need avmh and decay blocks to have the
		// same number of pipeline stage and be right after proc.

	// Input of this stage
	assign out_binaddr_0 = proc_binscan_addr_end;
	assign out_binlast_0 = proc_binscan_last_end;

	// Delays
	delay_bus #(9, 6) dl_out_binaddr (out_binaddr_0, out_binaddr_9, clk);
	delay_bit #(9)    dl_out_binlast (out_binlast_0, out_binlast_9, clk);

	// Packetizer
	f15_packetizer #(
		.BIN_WIDTH(6),
		.DECIM_WIDTH(12)
	) packetizer_I (
		.in_bin_addr(out_binaddr_9),
		.in_bin_last(out_binlast_9),
		.in_histo(decay_intensity_9[8:1]),
		.in_spectra_max(avgmh_max_9[11:4]),
		.in_spectra_avg(avgmh_avg_9[11:4]),
		.in_last(decay_last_9),
		.in_valid(decay_valid_9),
		.out_data(out_hist_fifo_di[31:0]),
		.out_last(out_hist_fifo_di[32]),
		.out_eob(out_hist_fifo_di[33]),
		.out_valid(out_hist_fifo_wren),
		.cfg_decim(cfg_decim),
		.cfg_decim_changed(cfg_decim_changed),
		.clk(clk),
		.rst(reset)
	);

	// FIFO
	fifo_srl #(
		.WIDTH(34),
		.LOG2_DEPTH(6),
		.AFULL_LEVEL(20)
	) out_hist_fifo_I (
		.di(out_hist_fifo_di),
		.wren(out_hist_fifo_wren),
		.afull(out_hist_fifo_afull),
		.do(out_hist_fifo_do),
		.rden(out_hist_fifo_rden),
		.empty(out_hist_fifo_empty),
		.clk(clk),
		.rst(reset)
	);

	// AXI mapping
	assign o_hist_tdata = out_hist_fifo_do[31:0];
	assign o_hist_tlast = out_hist_fifo_do[32];
	assign o_hist_teob  = out_hist_fifo_do[33];
	assign o_hist_tvalid = ~out_hist_fifo_empty;
	assign out_hist_fifo_rden = ~out_hist_fifo_empty && o_hist_tready;


	// -----------------------------------------------------------------------
	// Waterfall Output
	// -----------------------------------------------------------------------

	// Input to this stage (synced to SLS)
	assign wf_logpwr_0 = proc_logpwr_end;
	assign wf_last_0   = proc_last_end;
	assign wf_valid_0  = proc_valid_end;

	// Delay some input signals
	delay_bus #(2, 16) dl_wf_logpwr (wf_logpwr_0, wf_logpwr_2, clk);
	delay_bit #(2)     dl_wf_last   (wf_last_0,   wf_last_2,   clk);
	delay_bit #(2)     dl_wf_valid  (wf_valid_0,  wf_valid_2,  clk);

	// Decimation / Aggregation
	f15_wf_agg #(
		.Y_WIDTH(12),
		.X_WIDTH(16),
		.DECIM_WIDTH(8)
	) dut_wf (
		.yin_0(wf_data_2),
		.x_0(wf_logpwr_2),
		.valid_0(wf_valid_2),
		.last_0(wf_last_2),
		.rng_0(rng[15:0]),
		.yout_3(wf_data_5),
		.zout_3(wf_out_data_5),
		.zvalid_3(wf_out_valid_5),
		.cfg_div(cfg_wf_div),
		.cfg_mode(cfg_wf_mode),
		.cfg_decim(cfg_wf_decim),
		.cfg_decim_changed(cfg_wf_decim_changed),
		.clk(clk),
		.rst(reset)
	);

	// Delay some output signals
	delay_bus #(1, 12) dl_wf_data     (wf_data_5, wf_data_6, clk);
	delay_bit #(3)     dl_wf_out_last (wf_last_2, wf_out_last_5, clk);

	// Pack into 32 bits words
	always @(posedge clk)
	begin
		if (reset) begin
			out_wf_fifo_di   <= 0;
			out_wf_fifo_wren <= 1'b0;
			out_wf_cnt       <= 2'b00;
		end else begin
			if (wf_out_valid_5) begin
				if (wf_out_last_5) begin
					out_wf_fifo_di   <= { 1'b1, out_wf_fifo_di[23:0], wf_out_data_5 };
					out_wf_fifo_wren <= 1'b1;
					out_wf_cnt       <= 2'b00;
				end else begin
					out_wf_fifo_di   <= { 1'b0, out_wf_fifo_di[23:0], wf_out_data_5 };
					out_wf_fifo_wren <= (out_wf_cnt == 2'b11);
					out_wf_cnt       <= out_wf_cnt + 1;
				end
			end else begin
				out_wf_fifo_wren <= 1'b0;
			end
		end
	end

	// FIFO
	fifo_srl #(
		.WIDTH(33),
		.LOG2_DEPTH(6),
		.AFULL_LEVEL(20)
	) out_wf_fifo_I (
		.di(out_wf_fifo_di),
		.wren(out_wf_fifo_wren),
		.afull(out_wf_fifo_afull),
		.do(out_wf_fifo_do),
		.rden(out_wf_fifo_rden),
		.empty(out_wf_fifo_empty),
		.clk(clk),
		.rst(reset)
	);

	// AXI mapping
	assign o_wf_tdata = out_wf_fifo_do[31:0];
	assign o_wf_tlast = out_wf_fifo_do[32];
	assign o_wf_tvalid = ~out_wf_fifo_empty;
	assign out_wf_fifo_rden = ~out_wf_fifo_empty && o_wf_tready;


	// -----------------------------------------------------------------------
	// Misc
	// -----------------------------------------------------------------------

	// RNG
`ifdef SIM
	assign rng = 0;
`else
	rng rng_I (rng, clk, reset);
`endif

endmodule // f15_core
