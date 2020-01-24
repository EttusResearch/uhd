//
// Copyright 2016 Ettus Research
//

`timescale 1ns/1ps
`define SIM_TIMEOUT_US 1000
`define NS_PER_TICK 1
`define NUM_TEST_CASES 4

`include "sim_exec_report.vh"
`include "sim_clks_rsts.vh"
`include "sim_cvita_lib.svh"
`include "sim_axis_lib.svh"
`include "sim_set_rb_lib.svh"

module axi_fifo_tb();
  /*********************************************
  ** Setup Testbench
  *********************************************/
  `TEST_BENCH_INIT("axi_fifo_tb",`NUM_TEST_CASES,`NS_PER_TICK);
  localparam CLK_PERIOD = $ceil(1e9/166.67e6);
  `DEFINE_CLK(clk, CLK_PERIOD, 50);
  `DEFINE_RESET(reset, 0, 100);

  // 4 variants: axi_fifo_flop, axi_fifo_flop2, axi_fifo_short, axi_fifo_bram
  localparam NUM_FIFOS = 4;
  localparam integer FIFO_SIZES[NUM_FIFOS-1:0] = '{0,1,5,8};
  localparam NUM_ITERATIONS = 10000;

  /*********************************************
  ** DUTs
  ** - Instances of all variations of AXI FIFO
  *********************************************/
  reg  [NUM_FIFOS-1:0] clear;
  axis_master #(.DWIDTH(32), .NUM_STREAMS(NUM_FIFOS)) m_axis(.clk(clk));
  axis_slave #(.DWIDTH(32), .NUM_STREAMS(NUM_FIFOS)) s_axis(.clk(clk));

  genvar n;
  generate
    for (n = 0; n < NUM_FIFOS; n = n + 1) begin
      axi_fifo #(
        .SIZE(FIFO_SIZES[n]),
        .WIDTH(32))
      axi_fifo (
        .clk(clk), .reset(reset), .clear(clear[n]),
        .i_tdata(m_axis.axis.tdata[32*n +: 32]), .i_tvalid(m_axis.axis.tvalid[n]), .i_tready(m_axis.axis.tready[n]),
        .o_tdata(s_axis.axis.tdata[32*n +: 32]), .o_tvalid(s_axis.axis.tvalid[n]), .o_tready(s_axis.axis.tready[n]),
        .space(), .occupied());
    end
  endgenerate

  /*********************************************
  ** Testbench
  *********************************************/
  int write_word = 0;
  int read_word = 0;
  logic last;
  string s;

  initial begin
    clear = 'd0;

    /********************************************************
    ** Test 1 -- Reset
    ********************************************************/
    `TEST_CASE_START("Wait for Reset");
    m_axis.reset();
    s_axis.reset();
    while (reset) @(posedge clk);
    `TEST_CASE_DONE(~reset);

    /********************************************************
    ** Test 2 -- Check filling FIFOs
    ********************************************************/
    `TEST_CASE_START("Check filling FIFOs");
    for (int i = 0; i < NUM_FIFOS; i++) begin
      $display("Testing FIFO %0d, SIZE %0d",i,2**FIFO_SIZES[i]);
      for (int k = 0; k < 2**FIFO_SIZES[i]; k++) begin
        $sformat(s,"FIFO size should be %0d entries, but detected %0d!",2**FIFO_SIZES[i],k);
        `ASSERT_FATAL(m_axis.axis.tready[i],s);
        m_axis.push_word(k,0,i);
      end
      $sformat(s,"FIFO depth appears to be greater than %0d entries! Might be due to output registering.",2**FIFO_SIZES[i]);
      `ASSERT_WARN(~m_axis.axis.tready[i],s);
    end
    `TEST_CASE_DONE(1);

    /********************************************************
    ** Test 3 -- Check emptying FIFOs
    ********************************************************/
    `TEST_CASE_START("Check emptying FIFOs");
    for (int i = 0; i < NUM_FIFOS; i++) begin
      $display("Testing FIFO %0d, SIZE %0d",i,2**FIFO_SIZES[i]);
      for (int k = 0; k < 2**FIFO_SIZES[i]; k = k + 1) begin
        $sformat(s,"FIFO prematurely empty! Occured after %0d reads!",k);
        `ASSERT_FATAL(s_axis.axis.tvalid[i],s);
        s_axis.pull_word(read_word,last,i);
        $sformat(s,"Read invalid FIFO word! Expected: %0d, Actual: %0d",k,read_word);
        `ASSERT_FATAL(read_word == k,s);
      end
      `ASSERT_FATAL(~s_axis.axis.tvalid[i],"FIFO not empty after reading all entries!");
    end
    `TEST_CASE_DONE(1);

    /********************************************************
    ** Test 4 -- Randomized Write / Read Timing
    ********************************************************/
    `TEST_CASE_START("Randomized Write / Read");
    for (int i = 0; i < NUM_FIFOS; i++) begin
      $display("Testing FIFO %0d, SIZE %0d",i,2**FIFO_SIZES[i]);
      fork
      begin
        write_word = 0;
        for (int k = 0; k < NUM_ITERATIONS; k++) begin
          while ($signed($random()) > 0) @(posedge clk);
          m_axis.push_word(write_word,0,i);
          write_word++;
        end
      end
      begin
        for (int k = 0; k < NUM_ITERATIONS; k++) begin
          while ($signed($random()) > 0) @(posedge clk);
          s_axis.pull_word(read_word,last,i);
          $sformat(s,"Read invalid FIFO word! Expected: %0d, Actual: %0d",read_word,k);
          `ASSERT_FATAL(read_word == k,s);
        end
      end
      join
    end
    `TEST_CASE_DONE(1);
    `TEST_BENCH_DONE;
  end

endmodule
