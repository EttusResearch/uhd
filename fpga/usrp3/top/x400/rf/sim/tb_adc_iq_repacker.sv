//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: tb_adc_iq_repacker
//
// Description:
//
// This testbench mainly tests tb_adc_iq_repacker. It runs
// 3 different tests:
//   - test data propagation when not swapping data
//   - test data propagation when swapping data
//   - check that data valid does not propagate when block
//     is not enabled.
//

`timescale 1ns/1ps
`define NS_PER_TICK 1
`define NUM_TEST_CASES 3

`include "sim_clks_rsts.vh"
`include "sim_exec_report.vh"

module tb_adc_iq_repacker();
  import PkgRandom::*;

  `TEST_BENCH_INIT("tb_adc_iq_repacker", `NUM_TEST_CASES, `NS_PER_TICK);
  //sets up clock
  localparam CLK_PERIOD = $ceil(1e9/100.0e6);
  `DEFINE_CLK(clk, CLK_PERIOD, 50);
  //declare parameters
  localparam SPC = 1;
  localparam SAMPLE_WIDTH = 16;
  
  //declare wires and regs

  reg enable;
  reg valid_in;
  reg swap_iq;
  reg [SPC*SAMPLE_WIDTH-1:0] adc_q_in;
  reg [SPC*SAMPLE_WIDTH-1:0] adc_i_in;
  reg StopSim;

  wire [SPC*SAMPLE_WIDTH*2-1:0] data_out_tdata;
  wire data_out_tvalid;
  
  PkgRandom::Rand #(SPC*SAMPLE_WIDTH) rand_i;
  PkgRandom::Rand #(SPC*SAMPLE_WIDTH) rand_q;
  
  // instance adc_iq_repacker
  adc_iq_repacker #( 
    .SPC(SPC), 
    .SAMPLE_WIDTH(SAMPLE_WIDTH)
  ) adc_iq_repacker_inst (
    .clk(clk),
    // Data in
    .adc_q_in(adc_q_in),
    .adc_i_in(adc_i_in),
    .valid_in(valid_in),
    .enable(enable),
    // Data is packed [Q,I] (I in LSBs) when swap_iq is '0', and [I,Q] otherwise
    .swap_iq(swap_iq),
    // Data out
    .data_out_tdata(data_out_tdata),
    .data_out_tvalid(data_out_tvalid)
  );


  // initial statement with test cases
  initial begin : tb_main
  
    // test data propagation when not swapping data
    `TEST_CASE_START("test data propagation, no swap, core enabled");
    repeat (10) @(posedge clk);
    swap_iq = 0;
    enable = 1;
    StopSim = 0;
    valid_in = 1;
    for (int i = 0; i < 10; i++) begin 
      adc_q_in = rand_q.rand_bit();
      adc_i_in = rand_i.rand_bit();
      repeat (3) @(posedge clk);
      `ASSERT_ERROR(data_out_tdata[SAMPLE_WIDTH*2-1:0] == {adc_q_in[SAMPLE_WIDTH-1:0], adc_i_in[SAMPLE_WIDTH-1:0]}, "Output data is wrong");
      `ASSERT_ERROR(data_out_tvalid == 1, "output valid not asserted");
    end
    `TEST_CASE_DONE(1);
    // test data propagation when swapping data
    `TEST_CASE_START("test data propagation w/ swap_iq=1");
    swap_iq = 1;
    for (int i = 0; i < 10; i++) begin 
      adc_q_in = rand_q.rand_bit();
      adc_i_in = rand_i.rand_bit();
      repeat (3) @(posedge clk);
      `ASSERT_ERROR(data_out_tdata[SAMPLE_WIDTH*2-1:0] == {adc_i_in[SAMPLE_WIDTH-1:0], adc_q_in[SAMPLE_WIDTH-1:0]}, "Output data is wrong");
      `ASSERT_ERROR(data_out_tvalid == 1, "output valid not asserted");
    end
    `TEST_CASE_DONE(1);
    // check that data valid does not propagate when block
    // is not enabled.
    `TEST_CASE_START("test data valid when enable = 0");
    enable = 0;
    repeat (10) @(posedge clk);
    `ASSERT_ERROR(data_out_tvalid == 0, "block should be disabled");
    `TEST_CASE_DONE(1);
    StopSim = 1;

  end
endmodule : tb_adc_iq_repacker
