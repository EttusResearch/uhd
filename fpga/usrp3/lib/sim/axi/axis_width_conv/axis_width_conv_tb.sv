//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

`timescale 1ns/1ps
`define NS_PER_TICK 1
`define NUM_TEST_CASES 13

`include "sim_exec_report.vh"
`include "sim_clks_rsts.vh"
`include "sim_axis_lib.svh"

module axis_width_conv_tb();
  `TEST_BENCH_INIT("axis_width_conv_tb", `NUM_TEST_CASES, `NS_PER_TICK);

  `DEFINE_CLK(clk50,  20.000, 50);
  `DEFINE_CLK(clk200,  5.000, 50);
  `DEFINE_LATE_START_CLK(clk166, 6.000, 50, 0.37, 0.01)
  `DEFINE_RESET(areset, 0, 100);

  localparam         WORD_W               = 8;
  localparam integer NINST                = 6;
  localparam integer IN_WORDS[0:NINST-1]  = {1, 4, 4, 8, 8, 17};
  localparam integer OUT_WORDS[0:NINST-1] = {4, 1, 3, 2, 8, 19};
  localparam integer MAX_IN_WORDS         = 20;
  localparam integer MAX_OUT_WORDS        = 20;
  localparam integer MAX_SPP              = 100;

  axis_master #(.DWIDTH((WORD_W+1)*IN_WORDS[0]))  m0 (.clk(clk50));
  axis_master #(.DWIDTH((WORD_W+1)*IN_WORDS[1]))  m1 (.clk(clk50));
  axis_master #(.DWIDTH((WORD_W+1)*IN_WORDS[2]))  m2 (.clk(clk50));
  axis_master #(.DWIDTH((WORD_W+1)*IN_WORDS[3]))  m3 (.clk(clk50));
  axis_master #(.DWIDTH((WORD_W+1)*IN_WORDS[4]))  m4 (.clk(clk50));
  axis_master #(.DWIDTH((WORD_W+1)*IN_WORDS[5]))  m5 (.clk(clk50));

  axis_slave  #(.DWIDTH((WORD_W+1)*OUT_WORDS[0])) s0 (.clk(clk50));
  axis_slave  #(.DWIDTH((WORD_W+1)*OUT_WORDS[1])) s1 (.clk(clk50));
  axis_slave  #(.DWIDTH((WORD_W+1)*OUT_WORDS[2])) s2 (.clk(clk200));
  axis_slave  #(.DWIDTH((WORD_W+1)*OUT_WORDS[3])) s3 (.clk(clk166));
  axis_slave  #(.DWIDTH((WORD_W+1)*OUT_WORDS[4])) s4 (.clk(clk166));
  axis_slave  #(.DWIDTH((WORD_W+1)*OUT_WORDS[5])) s5 (.clk(clk50));

  // An integer 1-to-4 upsizer
  axis_width_conv #(
    .WORD_W(WORD_W), .IN_WORDS(IN_WORDS[0]), .OUT_WORDS(OUT_WORDS[0]),
    .SYNC_CLKS(1), .PIPELINE("INOUT")
  ) dut0 (
    .s_axis_aclk  (clk50),
    .s_axis_rst   (areset),
    .s_axis_tdata (m0.axis.tdata[(IN_WORDS[0]*WORD_W)-1:0]), 
    .s_axis_tkeep (m0.axis.tdata[IN_WORDS[0]*WORD_W+:IN_WORDS[0]]),
    .s_axis_tlast (m0.axis.tlast),
    .s_axis_tvalid(m0.axis.tvalid),
    .s_axis_tready(m0.axis.tready),
    .m_axis_aclk  (clk50),
    .m_axis_rst   (areset),
    .m_axis_tdata (s0.axis.tdata[(OUT_WORDS[0]*WORD_W)-1:0]),
    .m_axis_tkeep (s0.axis.tdata[OUT_WORDS[0]*WORD_W+:OUT_WORDS[0]]),
    .m_axis_tlast (s0.axis.tlast),
    .m_axis_tvalid(s0.axis.tvalid),
    .m_axis_tready(s0.axis.tready)
  );

  // An integer 4-to-1 downsizer
  axis_width_conv #(
    .WORD_W(WORD_W), .IN_WORDS(IN_WORDS[1]), .OUT_WORDS(OUT_WORDS[1]),
    .SYNC_CLKS(1), .PIPELINE("IN")
  ) dut1 (
    .s_axis_aclk  (clk50),
    .s_axis_rst   (areset),
    .s_axis_tdata (m1.axis.tdata[(IN_WORDS[1]*WORD_W)-1:0]), 
    .s_axis_tkeep (m1.axis.tdata[IN_WORDS[1]*WORD_W+:IN_WORDS[1]]),
    .s_axis_tlast (m1.axis.tlast),
    .s_axis_tvalid(m1.axis.tvalid),
    .s_axis_tready(m1.axis.tready),
    .m_axis_aclk  (clk50),
    .m_axis_rst   (areset),
    .m_axis_tdata (s1.axis.tdata[(OUT_WORDS[1]*WORD_W)-1:0]),
    .m_axis_tkeep (s1.axis.tdata[OUT_WORDS[1]*WORD_W+:OUT_WORDS[1]]),
    .m_axis_tlast (s1.axis.tlast),
    .m_axis_tvalid(s1.axis.tvalid),
    .m_axis_tready(s1.axis.tready)
  );

  // A rational integer 4-to-3 downsizer
  axis_width_conv #(
    .WORD_W(WORD_W), .IN_WORDS(IN_WORDS[2]), .OUT_WORDS(OUT_WORDS[2]),
    .SYNC_CLKS(0), .PIPELINE("OUT")
  ) dut2 (
    .s_axis_aclk  (clk50),
    .s_axis_rst   (areset),
    .s_axis_tdata (m2.axis.tdata[(IN_WORDS[2]*WORD_W)-1:0]), 
    .s_axis_tkeep (m2.axis.tdata[IN_WORDS[2]*WORD_W+:IN_WORDS[2]]),
    .s_axis_tlast (m2.axis.tlast),
    .s_axis_tvalid(m2.axis.tvalid),
    .s_axis_tready(m2.axis.tready),
    .m_axis_aclk  (clk200),
    .m_axis_rst   (areset),
    .m_axis_tdata (s2.axis.tdata[(OUT_WORDS[2]*WORD_W)-1:0]),
    .m_axis_tkeep (s2.axis.tdata[OUT_WORDS[2]*WORD_W+:OUT_WORDS[2]]),
    .m_axis_tlast (s2.axis.tlast),
    .m_axis_tvalid(s2.axis.tvalid),
    .m_axis_tready(s2.axis.tready)
  );

  // An integer 4-to-1 downsizer but with 2 words per cycle
  axis_width_conv #(
    .WORD_W(WORD_W), .IN_WORDS(IN_WORDS[3]), .OUT_WORDS(OUT_WORDS[3]),
    .SYNC_CLKS(0), .PIPELINE("NONE")
  ) dut3 (
    .s_axis_aclk  (clk50),
    .s_axis_rst   (areset),
    .s_axis_tdata (m3.axis.tdata[(IN_WORDS[3]*WORD_W)-1:0]), 
    .s_axis_tkeep (m3.axis.tdata[IN_WORDS[3]*WORD_W+:IN_WORDS[3]]),
    .s_axis_tlast (m3.axis.tlast),
    .s_axis_tvalid(m3.axis.tvalid),
    .s_axis_tready(m3.axis.tready),
    .m_axis_aclk  (clk166),
    .m_axis_rst   (areset),
    .m_axis_tdata (s3.axis.tdata[(OUT_WORDS[3]*WORD_W)-1:0]),
    .m_axis_tkeep (s3.axis.tdata[OUT_WORDS[3]*WORD_W+:OUT_WORDS[3]]),
    .m_axis_tlast (s3.axis.tlast),
    .m_axis_tvalid(s3.axis.tvalid),
    .m_axis_tready(s3.axis.tready)
  );

  // A passthrough module (no up/down sizing) but with 8 words per cycle
  axis_width_conv #(
    .WORD_W(WORD_W), .IN_WORDS(IN_WORDS[4]), .OUT_WORDS(OUT_WORDS[4]),
    .SYNC_CLKS(0), .PIPELINE("INOUT")
  ) dut4 (
    .s_axis_aclk  (clk50),
    .s_axis_rst   (areset),
    .s_axis_tdata (m4.axis.tdata[(IN_WORDS[4]*WORD_W)-1:0]), 
    .s_axis_tkeep (m4.axis.tdata[IN_WORDS[4]*WORD_W+:IN_WORDS[4]]),
    .s_axis_tlast (m4.axis.tlast),
    .s_axis_tvalid(m4.axis.tvalid),
    .s_axis_tready(m4.axis.tready),
    .m_axis_aclk  (clk166),
    .m_axis_rst   (areset),
    .m_axis_tdata (s4.axis.tdata[(OUT_WORDS[4]*WORD_W)-1:0]),
    .m_axis_tkeep (s4.axis.tdata[OUT_WORDS[4]*WORD_W+:OUT_WORDS[4]]),
    .m_axis_tlast (s4.axis.tlast),
    .m_axis_tvalid(s4.axis.tvalid),
    .m_axis_tready(s4.axis.tready)
  );

  // A rational integer 17-to-19 upsizer
  axis_width_conv #(
    .WORD_W(WORD_W), .IN_WORDS(IN_WORDS[5]), .OUT_WORDS(OUT_WORDS[5]),
    .SYNC_CLKS(1), .PIPELINE("INOUT")
  ) dut5 (
    .s_axis_aclk  (clk50),
    .s_axis_rst   (areset),
    .s_axis_tdata (m5.axis.tdata[(IN_WORDS[5]*WORD_W)-1:0]), 
    .s_axis_tkeep (m5.axis.tdata[IN_WORDS[5]*WORD_W+:IN_WORDS[5]]),
    .s_axis_tlast (m5.axis.tlast),
    .s_axis_tvalid(m5.axis.tvalid),
    .s_axis_tready(m5.axis.tready),
    .m_axis_aclk  (clk50),
    .m_axis_rst   (areset),
    .m_axis_tdata (s5.axis.tdata[(OUT_WORDS[5]*WORD_W)-1:0]),
    .m_axis_tkeep (s5.axis.tdata[OUT_WORDS[5]*WORD_W+:OUT_WORDS[5]]),
    .m_axis_tlast (s5.axis.tlast),
    .m_axis_tvalid(s5.axis.tvalid),
    .m_axis_tready(s5.axis.tready)
  );

  // Push a test ramp packet into the specific instance of the module
  // - words: The size of the packet in words
  // - inst: The instance number of the module to send to
  // - gaps: If 1 then insert bubble cycles randomly in the stream
  task automatic push_test_pkt(input integer words, input integer inst, input logic gaps);
    begin
      logic [(MAX_IN_WORDS*WORD_W)-1:0] data = 0;
      logic [MAX_IN_WORDS-1:0]          keep = 0;
      logic                             last = 0;
      integer nspc = IN_WORDS[inst];
      integer lines = words/nspc;
      integer residue = words % nspc;
      if (residue != 0) begin
        lines = lines + 1;
      end
      for (int l = 0; l < lines; l=l+1) begin
        last = (l == lines-1);
        for (int p = 0; p < nspc; p=p+1) begin
          data[p*WORD_W+:WORD_W] = (l*nspc)+p;
        end
        keep = (last && (residue != 0)) ? (1<<residue)-1 : (1<<nspc)-1;
        if (inst == 0)
          m0.push_word({keep[IN_WORDS[0]-1:0], data[(IN_WORDS[0]*WORD_W)-1:0]}, last);
        else if (inst == 1)
          m1.push_word({keep[IN_WORDS[1]-1:0], data[(IN_WORDS[1]*WORD_W)-1:0]}, last);
        else if (inst == 2)
          m2.push_word({keep[IN_WORDS[2]-1:0], data[(IN_WORDS[2]*WORD_W)-1:0]}, last);
        else if (inst == 3)
          m3.push_word({keep[IN_WORDS[3]-1:0], data[(IN_WORDS[3]*WORD_W)-1:0]}, last);
        else if (inst == 4)
          m4.push_word({keep[IN_WORDS[4]-1:0], data[(IN_WORDS[4]*WORD_W)-1:0]}, last);
        else
          m5.push_word({keep[IN_WORDS[5]-1:0], data[(IN_WORDS[5]*WORD_W)-1:0]}, last);

        // Keep tvalid deasserted randomly introduce gaps
        if (gaps) begin
          integer r = $urandom_range(0, 100);
          if (r < 20) repeat(r) @(posedge clk50);
        end
//        $display("PUSH(%02d)[%03d]: D{%0x}, K{%0b}, L{%0b}", inst, l, data, keep, last);
      end
    end
  endtask

  // Pull a test ramp packet from the specific instance of the module
  // - words: The size of the packet in words
  // - inst: The instance number of the module to send to
  // - gaps: If 1 then insert bubble cycles randomly in the stream
  // - ok: If 1 then all sanity checks have passed
  task automatic pull_test_pkt(input integer words, input integer inst, input logic gaps, output logic ok);
    begin
      logic [(MAX_OUT_WORDS*WORD_W)-1:0] pull_data = 0;
      logic [MAX_OUT_WORDS-1:0]          pull_keep = 0;
      logic                              pull_last = 0;

      logic [(MAX_OUT_WORDS*WORD_W)-1:0] data = 0;
      logic [MAX_OUT_WORDS-1:0]          keep = 0;
      logic                              last = 0;
      logic [(MAX_OUT_WORDS*WORD_W)-1:0] mask = 0;
      integer nspc = OUT_WORDS[inst];
      integer lines = words/nspc;
      integer residue = words % nspc;
      if (residue != 0) begin
        lines = lines + 1;
      end
      for (int l = 0; l < lines; l=l+1) begin
        last = (l == lines-1);
        for (int p = 0; p < nspc; p=p+1) begin
          data[p*WORD_W+:WORD_W] = (l*nspc)+p;
        end
        keep = (last && (residue != 0)) ? (1<<residue)-1 : (1<<nspc)-1;
        mask = (last && (residue != 0)) ? (1<<(residue*WORD_W))-1 : (1<<(nspc*WORD_W))-1;
        if (inst == 0)
          s0.pull_word({pull_keep[OUT_WORDS[0]-1:0], pull_data[(OUT_WORDS[0]*WORD_W)-1:0]}, pull_last);
        else if (inst == 1)
          s1.pull_word({pull_keep[OUT_WORDS[1]-1:0], pull_data[(OUT_WORDS[1]*WORD_W)-1:0]}, pull_last);
        else if (inst == 2)
          s2.pull_word({pull_keep[OUT_WORDS[2]-1:0], pull_data[(OUT_WORDS[2]*WORD_W)-1:0]}, pull_last);
        else if (inst == 3)
          s3.pull_word({pull_keep[OUT_WORDS[3]-1:0], pull_data[(OUT_WORDS[3]*WORD_W)-1:0]}, pull_last);
        else if (inst == 4)
          s4.pull_word({pull_keep[OUT_WORDS[4]-1:0], pull_data[(OUT_WORDS[4]*WORD_W)-1:0]}, pull_last);
        else
          s5.pull_word({pull_keep[OUT_WORDS[5]-1:0], pull_data[(OUT_WORDS[5]*WORD_W)-1:0]}, pull_last);

        // Keep tready deasserted randomly introduce gaps
        if (gaps) begin
          integer r = $urandom_range(0, 100);
          if (r < 20) repeat(r) @(posedge clk50);
        end
        // Check data, keep and last
        ok = ((data&mask) == (pull_data&mask)) && (keep == (pull_keep&((1<<nspc)-1))) && (last == pull_last);
//        $display("PULL(%02d)[%03d]: D{%0x =? %0x}, K{%0b =? %0b}, L{%0b =? %0b} ... %s",
//          inst, l, (data&mask), (pull_data&mask), keep, (pull_keep&((1<<nspc)-1)), last, pull_last, (ok?"OK":"FAIL"));
      end
    end
  endtask

  initial begin : tb_main
    string s;

    `TEST_CASE_START("Wait for Reset");
      m0.reset();
      m1.reset();
      m2.reset();
      m3.reset();
      m4.reset();
      m5.reset();
      s0.reset();
      s1.reset();
      s2.reset();
      s3.reset();
      s4.reset();
      s5.reset();
      while (areset) @(posedge clk50);
    `TEST_CASE_DONE(~areset);

    // Iterate through two modes:
    // - g == 0: Gapless send and receive. Transfer as fast as possible
    // - g == 1: Sender and receiver AXI streams will have random bubbles
    for (int g = 0; g < 2; g=g+1) begin
      // Iterate through the various instances of the module
      for (int i = 0; i < NINST; i=i+1) begin
        $sformat(s, "Testing Instance %0d (%0s gaps)", i, (g==0?"without":"with"));
        `TEST_CASE_START(s);
          fork
            begin
              // Send packets of increasing size (up to MAX_SPP) to test all
              // configurations of tkeep and tlast.
              for (int n = 1; n <= MAX_SPP; n=n+1) begin
                push_test_pkt(n, i, g);
              end
            end
            begin
              logic ok;
              // Receive packets of increasing size and validate tlast, tkeep and tdata
              for (int n = 1; n <= MAX_SPP; n=n+1) begin
                pull_test_pkt(n, i, g, ok);
                $sformat(s, "Packet Validation Failed! (Size=%0d)", n);
                `ASSERT_ERROR(ok, s);
              end
            end
          join
          // Gaps to separate the tests in the wfm viewer
          repeat (100) @(posedge clk50);
        `TEST_CASE_DONE(1'b1);
      end
    end

    `TEST_BENCH_DONE;
  end
endmodule
