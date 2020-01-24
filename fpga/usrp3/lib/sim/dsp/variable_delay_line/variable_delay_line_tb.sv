//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

`timescale 1ns/1ps
`define NS_PER_TICK 1
`define NUM_TEST_CASES 4

`include "sim_exec_report.vh"
`include "sim_clks_rsts.vh"
`include "sim_axis_lib.svh"

module variable_delay_line_tb();
  `TEST_BENCH_INIT("variable_delay_line_tb", `NUM_TEST_CASES, `NS_PER_TICK);
  localparam CLK_PERIOD = $ceil(1e9/166.67e6);
  `DEFINE_CLK(clk, CLK_PERIOD, 50);
  `DEFINE_RESET(reset, 0, 100);

  localparam             WIDTH                = 32;
  localparam [WIDTH-1:0] DEFAULT              = 32'hDEADBEEF;
  localparam integer     NUM_INST             = 24;
  localparam integer     DELAYS[0:NUM_INST-1] = {3, 14, 15, 16, 17, 30, 31, 32, 33, 127, 256, 511,
                                                 3, 14, 15, 16, 17, 30, 31, 32, 33, 127, 256, 511};

  logic             in_stb   [0:NUM_INST-1];
  logic [WIDTH-1:0] in_samp  [0:NUM_INST-1];
  wire  [WIDTH-1:0] out_samp [0:NUM_INST-1];
  logic [31:0]      out_delay[0:NUM_INST-1];

  task push_samp(logic [WIDTH-1:0] word, integer inst);
    begin
      //$display("D[%0d]: push(0x%08x)", inst, word);
      in_samp[inst] = word;
      in_stb[inst] = 1'b1;
      @(posedge clk);
      in_stb[inst] = 1'b0;
    end
  endtask

  task change_delay(logic [31:0] delay, integer inst);
    begin
      out_delay[inst] = delay;
      @(posedge clk);               // First stage
      if (inst/(NUM_INST/2) == 1) @(posedge clk); // Pipeline stage
      @(negedge clk);
      //$display("D[%0d]: cd(%0d) = 0x%08x", inst, delay, out_samp[inst]);
    end
  endtask

  genvar d;
  generate for (d = 0; d < NUM_INST; d=d+1) begin: inst
    variable_delay_line #(
      .WIDTH(WIDTH), .DEPTH(DELAYS[d]),
      .DYNAMIC_DELAY(1), .DEFAULT_DATA(DEFAULT), .OUT_REG(d / (NUM_INST/2))
    ) dut (
      .clk(clk), .clk_en(1'b1), .reset(reset),
      .data_in(in_samp[d]), .stb_in(in_stb[d]),
      .delay(out_delay[d]), .data_out(out_samp[d])
    );
  end endgenerate

  initial begin : tb_main
    string s;

    `TEST_CASE_START("Wait for Reset");
      for (int k = 0; k < NUM_INST; k=k+1) begin
        in_stb[k]    = 1'b0;
        out_delay[k] = 0;
      end
      while (reset) @(posedge clk);
    `TEST_CASE_DONE(~reset);

    `TEST_CASE_START("Check startup state of delay line");
      for (int k = 0; k < NUM_INST; k=k+1) begin
        $display("Validating delay line with DEPTH=%0d, OUT_REG=%0d...", DELAYS[k], k/(NUM_INST/2));
        for (int j = 0; j < DELAYS[k]; j=j+1) begin
          change_delay(j, k);
          `ASSERT_ERROR(out_samp[k] == DEFAULT, "Transient delay value incorrect");
        end
      end
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Check partially filled delay line");
      for (int k = 0; k < NUM_INST; k=k+1) begin
        $display("Validating delay line with DEPTH=%0d, OUT_REG=%0d...", DELAYS[k], k/(NUM_INST/2));
        for (int i = 0; i < DELAYS[k]; i=i+1) begin
          push_samp(i, k);
          for (int j = 0; j < DELAYS[k]; j=j+1) begin
            change_delay(j, k);
            $sformat(s, "Incorrect data. Samp = %0d, Delay = %0d, Data = %08x", i, j, out_samp[k]);
            `ASSERT_ERROR(out_samp[k] == (j>i ? DEFAULT : i - j), s);
          end
        end
      end
    `TEST_CASE_DONE(1);

    `TEST_CASE_START("Check steady state delay line");
      for (int k = 0; k < NUM_INST; k=k+1) begin
        $display("Validating delay line with DEPTH=%0d, OUT_REG=%0d...", DELAYS[k], k/(NUM_INST/2));
        for (int i = 0; i < DELAYS[k]; i=i+1) begin
          push_samp(i + DELAYS[k], k);
          for (int j = 0; j < DELAYS[k]; j=j+1) begin
            change_delay(j, k);
            $sformat(s, "Incorrect data. Samp = %0d, Delay = %0d, Data = %08x", i, j, out_samp[k]);
            `ASSERT_ERROR(out_samp[k] == DELAYS[k] + i - j, s);
          end
        end
      end
    `TEST_CASE_DONE(1);


    `TEST_BENCH_DONE;

  end
endmodule
