//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: clock_en_control_tb
//
// Description:
//  Testbench for clock_en_control and clock_div modules. Tasks defined for ctrlport write and
//  read transactions. Tests clock enable control with ctrlport reads and writes.
//
`timescale 1ns / 1ps
`define NS_PER_TICK 1
`define NUM_TEST_CASES 4
`include "sim_exec_report.vh"
`include "sim_clks_rsts.vh"


module clock_en_control_tb();
  import PkgRandom::*;
  PkgRandom::Rand #(5) rand_clk_count;
  `TEST_BENCH_INIT("clock_en_control_tb", `NUM_TEST_CASES, `NS_PER_TICK);
  //sets up clock
  localparam CLK_PERIOD = $ceil(1e9/50.0e6);
  localparam CLK_PERIOD_10 = $ceil(1e9/10.0e6);
  `DEFINE_CLK(clk50, CLK_PERIOD, 50);
  `DEFINE_CLK(clk10, CLK_PERIOD_10, 50);
  localparam N = 2;
  localparam N_DIV2 = N/2;

  reg reset_clk50;
  reg clk10_rst;
  reg        s_ctrlport_req_wr;
  reg        s_ctrlport_req_rd;
  reg [19:0] s_ctrlport_req_addr;
  reg [31:0] s_ctrlport_req_data;

  // Response
  wire         s_ctrlport_resp_ack;
  wire  [ 1:0] s_ctrlport_resp_status;
  wire  [31:0] s_ctrlport_resp_data;


  wire clk5, clk5_buf;
  `include "../../regmap/clock_en_regmap_utils.vh"

  task cp_write;
    input [19:0] a;
    input [31:0] d;
    begin
      s_ctrlport_req_addr = a;
      s_ctrlport_req_data = d;
      @(posedge clk50);
      s_ctrlport_req_wr = 1;
      @(posedge clk50);
      while(~s_ctrlport_resp_ack) @(posedge clk50);
      s_ctrlport_req_wr = 0;
      @(posedge clk50);
    end
  endtask

  task cp_read;
    input [19:0] a;
    input [31:0] d;
    input debug;
    begin
      s_ctrlport_req_addr = a;
      @(posedge clk50);
      s_ctrlport_req_rd = 1;
      @(posedge clk50);
      while(~s_ctrlport_resp_ack) @(posedge clk50);
      s_ctrlport_req_rd = 0;
      @(posedge clk50);
      if(debug) begin
        $display("status: read addr %h at %t value = %h", a, $time, s_ctrlport_resp_data);
      end
      `ASSERT_ERROR( s_ctrlport_resp_data == d, "CtrlPort read returned unexpected value.");
    end
  endtask

  clock_div #(
    .N(N)
  ) clock_div_5mhz (
    .clk_in     (clk10),
    .clk_in_rst (clk10_rst),
    .clk_out    (clk5)
  );

  clock_en_control #(
    .BASE_ADDRESS(0)
  ) clock_en_control_inst (
    .ctrlport_clk              (clk50),
    .ctrlport_rst              (reset_clk50),
    .s_ctrlport_req_wr         (s_ctrlport_req_wr),
    .s_ctrlport_req_rd         (s_ctrlport_req_rd),
    .s_ctrlport_req_addr       (s_ctrlport_req_addr),
    .s_ctrlport_req_data       (s_ctrlport_req_data),
    .s_ctrlport_resp_ack       (s_ctrlport_resp_ack),
    .s_ctrlport_resp_status    (s_ctrlport_resp_status),
    .s_ctrlport_resp_data      (s_ctrlport_resp_data),
    .clk_in                    (clk5),
    .clk_out                   (clk5_buf)
  );

  // clock counters to verify clock division
  reg counter_reset;
  reg [8:0] clock_counter;
  reg [10:0] div_clock_counter;
  reg [10:0] expected_clock_count;
  reg [8:0] expected_div_clock_count;

  always @(posedge clk10) begin
    if(counter_reset) begin
      clock_counter <= 0;
    end else begin
      clock_counter <= clock_counter + 1;
    end
  end

  always @(posedge clk5_buf) begin
    if(counter_reset) begin
      div_clock_counter <= 0;
    end else begin
      div_clock_counter <= div_clock_counter + 1;
    end
  end

  // main test cases
  initial begin : tb_main
    `TEST_CASE_START("initialize UUT");
    reset_clk50 = 1;
    clk10_rst = 1;
    s_ctrlport_req_wr = 0;
    s_ctrlport_req_rd = 0;
    s_ctrlport_req_addr = 0;
    s_ctrlport_req_data = 0;
    #200 //wait a cycle for the 10mhz clk rst
    reset_clk50 = 0;
    clk10_rst = 0;
    $display("status: %t done reset here", $time);
    `TEST_CASE_DONE(~reset_clk50);
    // trigger setup
    `TEST_CASE_START("Check that clock_out is not running");
    @(posedge clk5);
    @(posedge clk10);
    `ASSERT_ERROR( clk5_buf == 0, "clk5_buf should be zero now.");
    repeat (N) @(posedge clk10);
    `TEST_CASE_DONE(clk5_buf == 0 && clk5 == 1);
    `TEST_CASE_START("Enable clock with clock_en_control enable register");
    cp_read(CLK_EN_CONTROL, 0, 1); //initial value should be zero
    cp_write(CLK_EN_CONTROL, CLK_EN_CONTROL_MASK);
    cp_read(CLK_EN_CONTROL, CLK_EN_CONTROL_MASK, 1);
    @(posedge clk5_buf);
    repeat (N_DIV2+1) @(posedge clk10);
    `ASSERT_ERROR( clk5_buf == 0, "clk5_buf should be zero now.");
    repeat (N_DIV2) @(posedge clk10);
    `TEST_CASE_DONE(clk5_buf == 1 && s_ctrlport_resp_data == CLK_EN_CONTROL_MASK);
    `TEST_CASE_START("Verify expected clock division.");
    expected_div_clock_count = 5 + rand_clk_count.rand_bit();
    expected_clock_count = N*expected_div_clock_count;
    $display("Reset clock counters @ time %t and then count main clock &d times and divided clock %d times", $time, expected_clock_count, expected_div_clock_count);
    @(posedge clk5_buf);
    counter_reset = 1;
    @(posedge clk5_buf);
    counter_reset = 0;
    repeat (expected_div_clock_count) @(posedge clk5_buf);
    @(posedge clk10);
    `TEST_CASE_DONE(div_clock_counter == expected_div_clock_count && clock_counter == expected_clock_count);
    `TEST_BENCH_DONE;
end
endmodule

