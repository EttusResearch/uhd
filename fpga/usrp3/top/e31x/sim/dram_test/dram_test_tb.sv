//
// Copyright 2016 Ettus Research
//


`timescale 1ns/1ps
`define SIM_TIMEOUT_US 10000
`define NS_PER_TICK 1
`define NUM_TEST_CASES 3

`define SIM_RUNTIME_US  100

`include "sim_clks_rsts.vh"
`include "sim_exec_report.vh"

module dram_test_tb();
  `TEST_BENCH_INIT("dram_test_tb",`NUM_TEST_CASES,`NS_PER_TICK)

  // Define all clocks and resets
  `DEFINE_CLK(sys_clk, 10, 50)    //100MHz sys_clk to generate DDR3 clocking
  `DEFINE_CLK(ref_clk, 5, 50)     //200MHz ref_clk to generate DDR3 clocking
  `DEFINE_RESET(sys_rst, 0, 250000)  //100ns for GSR to deassert

  // Initialize DUT
  wire calib_complete;

  wire [15:0]   ddr3_dq;      // Data pins. Input for Reads; Output for Writes.
  wire [1:0]    ddr3_dqs_n;   // Data Strobes. Input for Reads; Output for Writes.
  wire [1:0]    ddr3_dqs_p;
  wire [14:0]   ddr3_addr;    // Address
  wire [2:0]    ddr3_ba;      // Bank Address
  wire          ddr3_ras_n;   // Row Address Strobe.
  wire          ddr3_cas_n;   // Column address select
  wire          ddr3_we_n;    // Write Enable
  wire          ddr3_reset_n; // SDRAM reset pin.
  wire [0:0]    ddr3_ck_p;    // Differential clock
  wire [0:0]    ddr3_ck_n;
  wire [0:0]    ddr3_cke;     // Clock Enable
  wire [0:0]    ddr3_cs_n;    // Chip Select
  wire [3:0]    ddr3_dm;      // Data Mask [3] = UDM.U26; [2] = LDM.U26;
  wire [0:0]    ddr3_odt;     // On-Die termination enable.

  ddr3_model #(
    .DEBUG(1)   //Disable verbose prints
  ) sdram_i0 (
    .rst_n    (ddr3_reset_n),
    .ck       (ddr3_ck_p),
    .ck_n     (ddr3_ck_n),
    .cke      (ddr3_cke),
    .cs_n     (1'b0),
    .ras_n    (ddr3_ras_n),
    .cas_n    (ddr3_cas_n),
    .we_n     (ddr3_we_n),
    .dm_tdqs  (ddr3_dm[1:0]),
    .ba       (ddr3_ba),
    .addr     (ddr3_addr),
    .dq       (ddr3_dq[15:0]),
    .dqs      (ddr3_dqs_p[1:0]),
    .dqs_n    (ddr3_dqs_n[1:0]),
    .tdqs_n   (), // Unused on x16
    .odt      (ddr3_odt)
  );

  example_top inst_example_top
  (
    .ddr3_dq                       (ddr3_dq),
    .ddr3_dqs_n                    (ddr3_dqs_n),
    .ddr3_dqs_p                    (ddr3_dqs_p),
    .ddr3_addr                     (ddr3_addr),
    .ddr3_ba                       (ddr3_ba),
    .ddr3_ras_n                    (ddr3_ras_n),
    .ddr3_cas_n                    (ddr3_cas_n),
    .ddr3_we_n                     (ddr3_we_n),
    .ddr3_reset_n                  (ddr3_reset_n),
    .ddr3_ck_p                     (ddr3_ck_p),
    .ddr3_ck_n                     (ddr3_ck_n),
    .ddr3_cke                      (ddr3_cke),
    .ddr3_dm                       (ddr3_dm),
    .ddr3_odt                      (ddr3_odt),
    .sys_clk_i                     (sys_clk),
    .clk_ref_i                     (ref_clk),
    .tg_compare_error              (tg_compare_error),
    .init_calib_complete           (calib_complete),
    .sys_rst                       (sys_rst)
  );

  //
  //Make sure we catch the error condition
  //
  reg tg_compare_error_reg;
  always @ (posedge sys_clk)
    if (sys_rst)
      tg_compare_error_reg = 1'b0;
    else
      tg_compare_error_reg = tg_compare_error | tg_compare_error_reg;

  //------------------------------------------
  //Main thread for testbench execution
  //------------------------------------------

  initial begin : tb_main

    `TEST_CASE_START("Wait for reset");
    while (sys_rst) @(posedge sys_clk);
    `TEST_CASE_DONE((~sys_rst));

    repeat (200) @(posedge sys_clk);

    `TEST_CASE_START("Wait for initial calibration to complete");
    while (calib_complete !== 1'b1) @(posedge sys_clk);
    `TEST_CASE_DONE(calib_complete);

    `TEST_CASE_START("Run for a while, then check for error");
    repeat (2_000_000) @(posedge sys_clk);
    `ASSERT_ERROR(tg_compare_error_reg == 1'b0, "Test generator reported error");
    `TEST_CASE_DONE(1'b1);
    `TEST_BENCH_DONE;

  end

endmodule
