//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

`timescale 1ns/1ps
`define NS_PER_TICK 1
`define NUM_TEST_CASES 8

`include "sim_exec_report.vh"
`include "sim_clks_rsts.vh"
`include "sim_axis_lib.svh"

module axis_shift_register_tb();
  `TEST_BENCH_INIT("axis_shift_register_tb", `NUM_TEST_CASES, `NS_PER_TICK);
  localparam CLK_PERIOD = $ceil(1e9/100.0e6);
  `DEFINE_CLK(clk, CLK_PERIOD, 50);
  `DEFINE_RESET(reset, 0, 100);

  localparam             WIDTH                = 32;
  localparam integer     NUM_INST             = 4;

  logic [WIDTH:0]   i_tdata[0:NUM_INST-1];
  logic             i_tlast[0:NUM_INST-1], i_tvalid[0:NUM_INST-1];
  wire              i_tready[0:NUM_INST-1];
  wire [WIDTH:0]    o_tdata[0:NUM_INST-1];
  wire              o_tlast[0:NUM_INST-1], o_tvalid[0:NUM_INST-1];
  logic             o_tready[0:NUM_INST-1];
  wire [15:0]       stage_stb[0:NUM_INST-1], stage_eop[0:NUM_INST-1];
  wire [WIDTH-1:0]  sb_dout, sb_din;
  wire              sb_keep;
  reg  [WIDTH-1:0]  sb_shreg[0:2];
  always @(posedge clk) begin
    if (stage_stb[2][0])
      sb_shreg[0] <= sb_dout;
    if (stage_stb[2][1])
      sb_shreg[1] <= sb_shreg[0];
    if (stage_stb[2][2])
      sb_shreg[2] <= sb_shreg[1];
  end
  assign sb_din = sb_shreg[2];

  axis_shift_register #(
    .WIDTH(WIDTH), .NSPC(1), .LATENCY(2),
    .SIDEBAND_DATAPATH(0), .GAPLESS(0), 
    .PIPELINE("NONE")
  ) inst_0 (
    .clk(clk), .reset(reset),
    .s_axis_tdata(i_tdata[0]), .s_axis_tkeep(i_tdata[0][WIDTH]), .s_axis_tlast(i_tlast[0]),
    .s_axis_tvalid(i_tvalid[0]), .s_axis_tready(i_tready[0]),
    .m_axis_tdata(o_tdata[0]), .m_axis_tkeep(o_tdata[0][WIDTH]), .m_axis_tlast(o_tlast[0]),
    .m_axis_tvalid(o_tvalid[0]), .m_axis_tready(o_tready[0]),
    .stage_stb(stage_stb[0]), .stage_eop(stage_eop[0]),
    .m_sideband_data(), .m_sideband_keep(), .s_sideband_data()
  );

  axis_shift_register #(
    .WIDTH(WIDTH), .NSPC(1), .LATENCY(9),
    .SIDEBAND_DATAPATH(0), .GAPLESS(0), 
    .PIPELINE("BOTH")
  ) inst_1 (
    .clk(clk), .reset(reset),
    .s_axis_tdata(i_tdata[1]), .s_axis_tkeep(i_tdata[1][WIDTH]), .s_axis_tlast(i_tlast[1]),
    .s_axis_tvalid(i_tvalid[1]), .s_axis_tready(i_tready[1]),
    .m_axis_tdata(o_tdata[1]), .m_axis_tkeep(o_tdata[1][WIDTH]), .m_axis_tlast(o_tlast[1]),
    .m_axis_tvalid(o_tvalid[1]), .m_axis_tready(o_tready[1]),
    .stage_stb(stage_stb[1]), .stage_eop(stage_eop[1]),
    .m_sideband_data(), .m_sideband_keep(), .s_sideband_data()
  );

  axis_shift_register #(
    .WIDTH(WIDTH/2), .NSPC(2), .LATENCY(3),
    .SIDEBAND_DATAPATH(1), .GAPLESS(0), 
    .PIPELINE("NONE")
  ) inst_2 (
    .clk(clk), .reset(reset),
    .s_axis_tdata(i_tdata[2]), .s_axis_tkeep(i_tdata[2][WIDTH]), .s_axis_tlast(i_tlast[2]),
    .s_axis_tvalid(i_tvalid[2]), .s_axis_tready(i_tready[2]),
    .m_axis_tdata(o_tdata[2]), .m_axis_tkeep(o_tdata[2][WIDTH]), .m_axis_tlast(o_tlast[2]),
    .m_axis_tvalid(o_tvalid[2]), .m_axis_tready(o_tready[2]),
    .stage_stb(stage_stb[2]), .stage_eop(stage_eop[2]),
    .m_sideband_data(sb_dout), .m_sideband_keep(sb_keep), .s_sideband_data(sb_din)
  );

  axis_shift_register #(
    .WIDTH(WIDTH/8), .NSPC(8), .LATENCY(14),
    .SIDEBAND_DATAPATH(0), .GAPLESS(1), 
    .PIPELINE("NONE")
  ) inst_3 (
    .clk(clk), .reset(reset),
    .s_axis_tdata(i_tdata[3]), .s_axis_tkeep(i_tdata[3][WIDTH]), .s_axis_tlast(i_tlast[3]),
    .s_axis_tvalid(i_tvalid[3]), .s_axis_tready(i_tready[3]),
    .m_axis_tdata(o_tdata[3]), .m_axis_tkeep(o_tdata[3][WIDTH]), .m_axis_tlast(o_tlast[3]),
    .m_axis_tvalid(o_tvalid[3]), .m_axis_tready(o_tready[3]),
    .stage_stb(stage_stb[3]), .stage_eop(stage_eop[3]),
    .m_sideband_data(), .m_sideband_keep(), .s_sideband_data()
  );

  logic [15:0] prev_stage_stb[0:NUM_INST-1];
  initial begin : tb_main
    string s;

    `TEST_CASE_START("Wait for Reset");
      for (int k = 0; k < NUM_INST; k=k+1) begin
        o_tready[k] = 1'b1;
        i_tvalid[k] = 1'b0;
        prev_stage_stb[k] = 0;
      end
      while (reset) @(posedge clk);
    `TEST_CASE_DONE(~reset);

    `TEST_CASE_START("Check module with LATENCY=2, PIPELINE=NONE");
      `ASSERT_ERROR(o_tvalid[0] == 1'b0, "Output incorrectly active when idle");
      `ASSERT_ERROR(i_tready[0] == 1'b1, "Input was not ready when idle");
      `ASSERT_ERROR(stage_stb[0][1:0] === 0, "Incorrect stage_stb when idle");
      for (int j = 0; j < 2; j=j+1) begin
        i_tdata[0] = j;
        i_tlast[0] = j % 2;
        i_tvalid[0] = 1'b1;
        @(posedge clk);
        i_tvalid[0] = 1'b0;
        prev_stage_stb[0][1:0] = {prev_stage_stb[0][0], 1'b1};
        `ASSERT_ERROR(stage_stb[0][1:0] == prev_stage_stb[0][1:0], "Incorrect stage_stb");
        `ASSERT_ERROR(i_tready[0] == 1'b1, "Input was not ready during fill-up");
        `ASSERT_ERROR(o_tvalid[0] == 1'b0, "Output incorrectly active during fill-up");
      end
      for (int j = 0; j < 2; j=j+1) begin
        @(posedge clk);
        `ASSERT_ERROR(o_tvalid[0] == 1'b1, "Output was not active");
        `ASSERT_ERROR(o_tlast[0] == j % 2, "Incorrect tlast");
        `ASSERT_ERROR(o_tdata[0] == j, "Incorrect tdata");
      end
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Check module with LATENCY=9, PIPELINE=BOTH");
      prev_stage_stb[1] = 0;
      `ASSERT_ERROR(o_tvalid[1] == 1'b0, "Output incorrectly active when idle");
      `ASSERT_ERROR(i_tready[1] == 1'b1, "Input was not ready when idle");
      `ASSERT_ERROR(stage_stb[1][1:0] === 0, "Incorrect stage_stb when idle");
      for (int j = 0; j < 9; j=j+1) begin
        i_tdata[1] = -j;
        i_tlast[1] = 1'b1;
        i_tvalid[1] = 1'b1;
        @(posedge clk);
        i_tvalid[1] = 1'b0;
        prev_stage_stb[1][8:0] = {prev_stage_stb[1][7:0], 1'b1};
        `ASSERT_ERROR(stage_eop[1][8:0] == prev_stage_stb[1][8:0], "Incorrect stage_eop");
        `ASSERT_ERROR(stage_stb[1][8:0] == prev_stage_stb[1][8:0], "Incorrect stage_stb");
        `ASSERT_ERROR(i_tready[1] == 1'b1, "Input was not ready during fill-up");
        `ASSERT_ERROR(o_tvalid[1] == 1'b0, "Output incorrectly active during fill-up");
      end
      for (int j = 0; j < 9; j=j+1) begin
        @(posedge clk);
        `ASSERT_ERROR(o_tvalid[1] == 1'b1, "Output was not active");
        `ASSERT_ERROR(o_tlast[1] == 1'b1, "Incorrect tlast");
        `ASSERT_ERROR(o_tdata[1] == -j, "Incorrect tdata");
      end
      @(posedge clk);
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Check module with LATENCY=9, PIPELINE=BOTH (backpressure)");
      prev_stage_stb[1] = 0;
      o_tready[1] = 1'b0;
      `ASSERT_ERROR(o_tvalid[1] == 1'b0, "Output incorrectly active when idle");
      `ASSERT_ERROR(i_tready[1] == 1'b1, "Input was not ready when idle");
      `ASSERT_ERROR(stage_stb[1][1:0] === 0, "Incorrect stage_stb when idle");
      for (int j = 0; j < 9; j=j+1) begin
        i_tdata[1] = -j;
        i_tlast[1] = 1'b1;
        i_tvalid[1] = 1'b1;
        @(posedge clk);
        i_tvalid[1] = 1'b0;
        prev_stage_stb[1][8:0] = {prev_stage_stb[1][7:0], 1'b1};
        `ASSERT_ERROR(stage_stb[1][8:0] == prev_stage_stb[1][8:0], "Incorrect stage_stb");
        `ASSERT_ERROR(stage_eop[1][8:0] == prev_stage_stb[1][8:0], "Incorrect stage_eop");
        `ASSERT_ERROR(i_tready[1] == 1'b1, "Input was not ready during fill-up");
        `ASSERT_ERROR(o_tvalid[1] == 1'b0, "Output incorrectly active during fill-up");
      end
      o_tready[1] = 1'b1;
      for (int j = 0; j < 9; j=j+1) begin
        @(posedge clk);
        `ASSERT_ERROR(o_tvalid[1] == 1'b1, "Output was not active");
        `ASSERT_ERROR(o_tlast[1] == 1'b1, "Incorrect tlast");
        `ASSERT_ERROR(o_tdata[1] == -j, "Incorrect tdata");
      end
      @(posedge clk);
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Check module with LATENCY=9, PIPELINE=BOTH (steady-state)");
      prev_stage_stb[1] = 0;
      fork
        begin
          for (int j = 0; j < 50; j=j+1) begin
            i_tdata[1] = j;
            i_tlast[1] = j % 2;
            i_tvalid[1] = 1'b1;
            @(posedge clk);
            i_tvalid[1] = 1'b0;
            prev_stage_stb[1][8:0] = {prev_stage_stb[1][7:0], 1'b1};
            `ASSERT_ERROR(stage_stb[1][8:0] == prev_stage_stb[1][8:0], "Incorrect stage_stb");
          end
        end
        begin
          o_tready[1] = 1'b1;
          repeat(9) @(posedge clk);
          for (int j = 0; j < 50; j=j+1) begin
            @(posedge clk);
            `ASSERT_ERROR(o_tvalid[1] == 1'b1, "Output was not active");
            `ASSERT_ERROR(o_tlast[1] == j % 2, "Incorrect tlast");
            `ASSERT_ERROR(o_tdata[1] == j, "Incorrect tdata");
          end
          @(posedge clk);
        end
      join
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Check module with LATENCY=9, PIPELINE=BOTH (gaps)");
      prev_stage_stb[1] = 0;
      fork
        begin
          for (int j = 0; j < 50; j=j+1) begin
            i_tdata[1] = j;
            i_tlast[1] = j % 2;
            i_tvalid[1] = 1'b1;
            @(posedge clk);
            while (~i_tready[1]) @(posedge clk);
            i_tvalid[1] = 1'b0;
          end
        end
        begin
          o_tready[1] = 1'b1;
          repeat(9) @(posedge clk);
          for (int j = 0; j < 20; j=j+1) begin
            @(posedge clk);
            `ASSERT_ERROR(o_tvalid[1] == 1'b1, "Output was not active");
            `ASSERT_ERROR(o_tlast[1] == j % 2, "Incorrect tlast");
            `ASSERT_ERROR(o_tdata[1] == j, "Incorrect tdata");
          end
          o_tready[1] = 1'b0;
          repeat(4) @(posedge clk);
          o_tready[1] = 1'b1;
          for (int j = 20; j < 50; j=j+1) begin
            @(posedge clk);
            `ASSERT_ERROR(o_tvalid[1] == 1'b1, "Output was not active");
            `ASSERT_ERROR(o_tlast[1] == j % 2, "Incorrect tlast");
            `ASSERT_ERROR(o_tdata[1] == j, "Incorrect tdata");
          end
          @(posedge clk);
        end
      join
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Check module with LATENCY=3, SIDEBAND=1 (gaps)");
      prev_stage_stb[2] = 0;
      fork
        begin
          for (int j = 0; j < 50; j=j+1) begin
            i_tdata[2] = j;
            i_tlast[2] = j % 4 == 0;
            i_tvalid[2] = 1'b1;
            @(posedge clk);
            while (~i_tready[2]) @(posedge clk);
            i_tvalid[2] = 1'b0;
          end
        end
        begin
          o_tready[2] = 1'b1;
          repeat(3) @(posedge clk);
          for (int j = 0; j < 20; j=j+1) begin
            @(posedge clk);
            `ASSERT_ERROR(o_tvalid[2] == 1'b1, "Output was not active");
            `ASSERT_ERROR(o_tlast[2] == j % 4 == 0, "Incorrect tlast");
            `ASSERT_ERROR(o_tdata[2] == j, "Incorrect tdata");
          end
          o_tready[2] = 1'b0;
          repeat(4) @(posedge clk);
          o_tready[2] = 1'b1;
          for (int j = 20; j < 50; j=j+1) begin
            @(posedge clk);
            `ASSERT_ERROR(o_tvalid[2] == 1'b1, "Output was not active");
            `ASSERT_ERROR(o_tlast[2] == j % 4 == 0, "Incorrect tlast");
            `ASSERT_ERROR(o_tdata[2] == j, "Incorrect tdata");
          end
          @(posedge clk);
        end
      join
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Check module with LATENCY=14, GAPLESS=1");
      prev_stage_stb[3] = 0;
      o_tready[3] = 1'b0;
      fork
        begin
          for (int j = 0; j < 50; j=j+1) begin
            i_tdata[3] = j;
            i_tlast[3] = j % 4 == 0;
            i_tvalid[3] = 1'b1;
            @(posedge clk);
            prev_stage_stb[3][13:0] = {prev_stage_stb[3][12:0], 1'b1};
            `ASSERT_ERROR(stage_stb[3][13:0] == prev_stage_stb[3][13:0], "Incorrect stage_stb");
            while (~i_tready[3]) @(posedge clk);
            i_tvalid[3] = 1'b0;
            if (j > 16) repeat($urandom_range(0, 15)) @(posedge clk);
            `ASSERT_ERROR(stage_stb[3][13:0] == prev_stage_stb[3][13:0] || stage_stb[3][13:0] == 14'd0, "Incorrect stage_stb");
          end
        end
        begin
          for (int j = 0; j < 50 - 14; j=j+1) begin
            while (~o_tvalid[3]) @(posedge clk);
            @(negedge clk);
            `ASSERT_ERROR(o_tvalid[3] == 1'b1, "Output was not active");
            `ASSERT_ERROR(o_tlast[3] == j % 4 == 0, "Incorrect tlast");
            `ASSERT_ERROR(o_tdata[3] == j, "Incorrect tdata");
            o_tready[3] = 1'b1;
            @(negedge clk);
            o_tready[3] = 1'b0;
            @(posedge clk);
          end
        end
      join
    `TEST_CASE_DONE(1);
    `TEST_BENCH_DONE;
  end
endmodule
