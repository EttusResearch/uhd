//
// Copyright 2015 Ettus Research
//
// Test bench for E310 I/O interface to AD9361.

`timescale 1ns/1ps
`define SIM_TIMEOUT_US 20
`define NS_PER_TICK 1
`define NUM_TEST_CASES 6

`include "sim_clks_rsts.vh"
`include "sim_exec_report.vh"

module e310_io_tb();
  `TEST_BENCH_INIT("e310_io_tb",`NUM_TEST_CASES,`NS_PER_TICK)

  // Define all clocks and resets
  `DEFINE_CLK(rx_clk, 16.27, 50) // ~61.44 MHz clock from AD9361
  `DEFINE_RESET(areset, 0, 100)  // 100ns reset

  reg mimo;
  wire radio_clk, radio_rst;
  wire [11:0] rx_i0, rx_i1, rx_q0, rx_q1;
  wire rx_stb;
  reg [11:0] tx_i0, tx_i1, tx_q0, tx_q1;
  wire tx_stb;
  reg rx_frame;
  reg [11:0] rx_data;
  wire tx_clk;
  wire tx_frame;
  wire [11:0] tx_data;
  e310_io e310_io (
    .areset(areset),
    .mimo(mimo),
    .radio_clk(radio_clk),
    .radio_rst(radio_rst),
    .rx_i0(rx_i0),
    .rx_q0(rx_q0),
    .rx_i1(rx_i1),
    .rx_q1(rx_q1),
    .rx_stb(rx_stb),
    .tx_i0(tx_i0),
    .tx_q0(tx_q0),
    .tx_i1(tx_i1),
    .tx_q1(tx_q1),
    .tx_stb(tx_stb),
    .rx_clk(rx_clk),
    .rx_frame(rx_frame),
    .rx_data(rx_data),
    .tx_clk(tx_clk),
    .tx_frame(tx_frame),
    .tx_data(tx_data));

  /********************************************************
  ** Test Bench
  ********************************************************/
  initial begin : tb_main
    mimo     <= 1'b0;
    tx_i0    <= 'd0;
    tx_q0    <= 'd0;
    tx_i1    <= 'd0;
    tx_q1    <= 'd0;
    rx_data  <= 'd0;
    rx_frame <= 1'b0;
    `TEST_CASE_START("Wait for reset");
    while (areset) @(posedge radio_clk);
    `TEST_CASE_DONE((~areset));

    repeat (10) @(posedge radio_clk);

    `TEST_CASE_START("Test RX channel 0,1");
    mimo      <= 1'b0;
    rx_data   <= 'd0;
    repeat (10) @(posedge radio_clk);
    fork
      begin
        for (int i = 1; i < 64; i = i + 2) begin
          @(posedge radio_clk);
          rx_frame  <= 1'b1;
          rx_data   <= i;
          @(negedge radio_clk);
          rx_frame  <= 1'b0;
          rx_data   <= i+1;
        end
      end
      begin
        while ({rx_i0, rx_q0} == 24'd0) @(posedge radio_clk);
        for (int i = 1; i < 64; i = i + 2) begin
          // RX should be replicated across both ports
          `ASSERT_ERROR(rx_i0 == i,   "RX0 I incorrect!");
          `ASSERT_ERROR(rx_q0 == i+1, "RX0 Q incorrect!");
          `ASSERT_ERROR(rx_i1 == i,   "RX1 I incorrect!");
          `ASSERT_ERROR(rx_q1 == i+1, "RX1 Q incorrect!");
          @(posedge radio_clk);
        end
      end
    join
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Test RX channels 0 & 1 (MIMO mode)");
    mimo      <= 1'b1;
    rx_frame  <= 1'b0;
    rx_data   <= 'd0;
    repeat (10) @(posedge radio_clk);
    fork
      begin
        for (int i = 1; i < 64; i = i + 2) begin
          @(posedge radio_clk);
          rx_frame  <= ~rx_frame;
          rx_data   <= i;
          @(negedge radio_clk);
          rx_data   <= i+1;
        end
      end
      begin
        while ({rx_i0, rx_q0} == 24'd0) @(posedge radio_clk);
        for (int i = 1; i < 32; i = i + 4) begin
          // RX should be replicated across both ports
          `ASSERT_ERROR(rx_i0 == i,   "RX0 I incorrect!");
          `ASSERT_ERROR(rx_q0 == i+1, "RX0 Q incorrect!");
          @(posedge radio_clk);
          `ASSERT_ERROR(rx_i1 == i+2, "RX1 I incorrect!");
          `ASSERT_ERROR(rx_q1 == i+3, "RX1 Q incorrect!");
          @(posedge radio_clk);
        end
      end
    join
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Test TX channel 0");
    mimo  <= 1'b0;
    tx_i0 <= 'd0;
    tx_q0 <= 'd0;
    tx_i1 <= 'd0;
    tx_q1 <= 'd0;
    repeat (10) @(posedge radio_clk);
    // TX0
    fork
      begin
        for (int i = 1; i < 64; i = i + 2) begin
          tx_i0 <= i;
          tx_q0 <= i+1;
          @(posedge radio_clk);
        end
      end
      begin
        while (tx_data == 12'd0) @(posedge tx_clk);
        for (int i = 1; i < 64; i = i + 2) begin
          // RX should be replicated across both ports
          `ASSERT_ERROR(tx_data  == i, "TX0 I data incorrect!");
          `ASSERT_ERROR(tx_frame == 1'b1, "TX frame incorrect");
          @(negedge tx_clk);
          `ASSERT_ERROR(tx_data  == i+1, "TX0 Q data incorrect!");
          `ASSERT_ERROR(tx_frame == 1'b0, "TX frame incorrect");
          @(posedge tx_clk);
        end
      end
    join
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Test TX channel 1");
    mimo  <= 1'b0;
    tx_i0 <= 'd0;
    tx_q0 <= 'd0;
    tx_i1 <= 'd0;
    tx_q1 <= 'd0;
    repeat (10) @(posedge radio_clk);
    fork
      begin
        for (int i = 1; i < 64; i = i + 2) begin
          tx_i1 <= i;
          tx_q1 <= i+1;
          @(posedge radio_clk);
        end
      end
      begin
        while (tx_data == 12'd0) @(posedge tx_clk);
        for (int i = 1; i < 64; i = i + 2) begin
          `ASSERT_ERROR(tx_data  == i, "TX1 I data incorrect!");
          `ASSERT_ERROR(tx_frame == 1'b1, "TX frame incorrect");
          @(negedge tx_clk);
          `ASSERT_ERROR(tx_data  == i+1, "TX1 Q data incorrect!");
          `ASSERT_ERROR(tx_frame == 1'b0, "TX frame incorrect");
          @(posedge tx_clk);
        end
      end
    join
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Test TX channel 0 & 1 (MIMO)");
    mimo  <= 1'b1;
    tx_i0 <= 'd0;
    tx_q0 <= 'd0;
    tx_i1 <= 'd0;
    tx_q1 <= 'd0;
    repeat (10) @(posedge radio_clk);
    fork
      begin
        for (int i = 1; i < 32; i = i + 4) begin
          tx_i0 <= i;
          tx_q0 <= i+1;
          tx_i1 <= i+2;
          tx_q1 <= i+3;
          @(posedge radio_clk);
          while (tx_stb) @(posedge radio_clk);
        end
      end
      begin
        while (tx_data == 12'd0) @(posedge tx_clk);
        for (int i = 1; i < 32; i = i + 4) begin
          `ASSERT_ERROR(tx_data  == i, "TX0 I data incorrect!");
          `ASSERT_ERROR(tx_frame == 1'b1, "TX frame incorrect");
          @(negedge tx_clk);
          `ASSERT_ERROR(tx_data  == i+1, "TX0 Q data incorrect!");
          `ASSERT_ERROR(tx_frame == 1'b1, "TX frame incorrect");
          @(posedge tx_clk);
          `ASSERT_ERROR(tx_data  == i+2, "TX1 I data incorrect!");
          `ASSERT_ERROR(tx_frame == 1'b0, "TX frame incorrect");
          @(negedge tx_clk);
          `ASSERT_ERROR(tx_data  == i+3, "TX1 Q data incorrect!");
          `ASSERT_ERROR(tx_frame == 1'b0, "TX frame incorrect");
          @(posedge tx_clk);
        end
      end
    join
    `TEST_CASE_DONE(1);
    `TEST_BENCH_DONE;
  end

endmodule
