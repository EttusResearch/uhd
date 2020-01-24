//
// Copyright 2015 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// 10MHz master_clock_rate
always #50 clk <= ~clk;

   initial
     begin
	reset <= 1'b0;
	i_in <= 0;
	q_in <= 0;
	run <= 0;
	set_stb <= 0;
	set_addr <= 0;
	set_data <= 0;


	@(posedge clk);
	// Into Reset...
	reset <= 1'b1;
	repeat(10) @(posedge clk);
	// .. and back out of reset.
	reset <= 1'b0;
	repeat(10) @(posedge clk);
	// Now program DSP configuration via settings regs.
	write_setting_bus(SR_DSP_RX_FREQ,42949672); // 100kHz @ 10MHz MCR
	//  (1 << 15) * std::pow(2, ceil_log2(rate_pow))*2./(1.648*rate_pow)
	write_setting_bus(SR_DSP_RX_SCALE_IQ, 39767); // Should include CORDIC and CIC gain compensation.
	write_setting_bus(SR_DSP_RX_DECIM, 1<<9|1); // Decim = 2
	write_setting_bus(SR_DSP_RX_MUX, 0);
	write_setting_bus(SR_DSP_RX_COEFFS,0);
	repeat(10) @(posedge clk);

	// Set complex data inputs to DC unit circle position.
	i_in <= 12'h7ff;
	q_in <= 12'h0;
	run <= 1'b1;
	repeat(100) @(posedge clk);
	// Set complex data inputs to simulate ADC saturation of front end
	i_in <= 12'h7ff;
	q_in <= 12'h100;
	repeat(1000) @(posedge clk);
	i_in <= 12'h7ff;
	q_in <= 12'h200;
	repeat(1000) @(posedge clk);
	i_in <= 12'h7ff;
	q_in <= 12'h300;
	repeat(1000) @(posedge clk);
	i_in <= 12'h7ff;
	q_in <= 12'h400;
	repeat(1000) @(posedge clk);
	i_in <= 12'h7ff;
	q_in <= 12'h500;
	repeat(1000) @(posedge clk);
	i_in <= 12'h7ff;
	q_in <= 12'h600;
	repeat(1000) @(posedge clk);
	i_in <= 12'h7ff;
	q_in <= 12'h700;
	repeat(1000) @(posedge clk);
 	i_in <= 12'h7ff;
	q_in <= 12'h7FF;
	// Now test small signal performance
	repeat(1000) @(posedge clk);
	i_in <= 12'h001;
	q_in <= 12'h000;
	repeat(1000) @(posedge clk);
 	i_in <= 12'h000;
	q_in <= 12'h001;
	repeat(1000) @(posedge clk);
	i_in <= 12'hfff;
	q_in <= 12'h000;
	repeat(1000) @(posedge clk);
 	i_in <= 12'h000;
	q_in <= 12'hfff;

	repeat(100000) @(posedge clk);
	$finish();

     end // initial begin
