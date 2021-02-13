//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: gearbox_2x1_tb
//
// Description: Testbench for gearbox_2x1.
//

`default_nettype none


module gearbox_2x1_tb #(
  parameter IN_WORDS   = 2,
  parameter OUT_WORDS  = 1,
  parameter BIG_ENDIAN = 0,
  parameter PHASE      = 1
);

  `include "test_exec.svh"

  import PkgTestExec::*;

  localparam WORD_W = 4;

  // Clock periods
  localparam real SLOW_CLK_PER_NS = 10.0;
  localparam real FAST_CLK_PER_NS = SLOW_CLK_PER_NS / 2.0;
  localparam real CLK_IN_PER_NS   = (IN_WORDS > OUT_WORDS) ?
                                    SLOW_CLK_PER_NS : FAST_CLK_PER_NS;
  localparam real CLK_OUT_PER_NS  = (OUT_WORDS > IN_WORDS) ?
                                    SLOW_CLK_PER_NS : FAST_CLK_PER_NS;

  // Give the faster clock a T/2 phase offset
  localparam FAST_CLK_PHASE = PHASE * (FAST_CLK_PER_NS / 2.0);
  localparam real IN_PHASE  = (IN_WORDS > OUT_WORDS) ? 0.0 : FAST_CLK_PHASE;
  localparam real OUT_PHASE = (OUT_WORDS > IN_WORDS) ? 0.0 : FAST_CLK_PHASE;

  // Get gearbox total input and output widths
  localparam IN_WORD_W  = WORD_W * IN_WORDS;
  localparam OUT_WORD_W = WORD_W * OUT_WORDS;

  // Number of simulation cycles to run for
  localparam NUM_TEST_CYCLES = 50000;


  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit i_clk;
  bit o_clk;

  bit i_rst;
  bit o_rst;

  sim_clock_gen #(.PERIOD(CLK_IN_PER_NS), .AUTOSTART(0), .PHASE(IN_PHASE))
    clk_gen_in  (.clk(i_clk), .rst(i_rst));
  sim_clock_gen #(.PERIOD(CLK_OUT_PER_NS),.AUTOSTART(0), .PHASE(OUT_PHASE))
    clk_gen_out (.clk(o_clk), .rst(o_rst));


  //---------------------------------------------------------------------------
  // Device Under Test (DUT)
  //---------------------------------------------------------------------------

  bit [ IN_WORD_W-1:0] i_tdata  = { IN_WORD_W { 1'bX }};
  bit                  i_tvalid = 0;
  bit [OUT_WORD_W-1:0] o_tdata;
  bit                  o_tvalid;

  gearbox_2x1 #(
    .WORD_W     (WORD_W),
    .IN_WORDS   (IN_WORDS),
    .OUT_WORDS  (OUT_WORDS),
    .BIG_ENDIAN (BIG_ENDIAN)
  ) gearbox_2x1_i (
    .i_clk    (i_clk),
    .i_rst    (i_rst),
    .i_tdata  (i_tdata),
    .i_tvalid (i_tvalid),
    .o_clk    (o_clk),
    .o_rst    (o_rst),
    .o_tdata  (o_tdata),
    .o_tvalid (o_tvalid)
  );


  //---------------------------------------------------------------------------
  // Input Generator
  //---------------------------------------------------------------------------

  bit enable_input = 0;
  longint i_count;
  mailbox #(bit [IN_WORD_W-1:0]) i_queue = new;

  initial forever begin : gen_input
    clk_gen_in.clk_wait_r();
    if (i_rst || !enable_input) begin
      i_tdata  <= { IN_WORD_W { 1'bX }};
      i_tvalid <= 1'b0;
      continue;
    end
    // Input randomly to the gearbox
    if ($urandom() % 2 == 0) begin
      bit [ IN_WORD_W-1:0] next_input;
      next_input = $urandom();
      i_count  = i_count + 1;
      i_tdata  <= next_input;
      i_tvalid <= 1'b1;
      i_queue.put(next_input);
    end else begin
      i_tdata  <= { IN_WORD_W { 1'bX }};
      i_tvalid <= 1'b0;
    end
  end : gen_input


  //---------------------------------------------------------------------------
  // Packer/Unpacker
  //---------------------------------------------------------------------------
  //
  // Take the data from i_queue and repack it into o_queue with the correct
  // width and using the correct endianness.
  //
  //---------------------------------------------------------------------------

  mailbox #(bit [OUT_WORD_W-1:0]) o_queue = new;

  initial if (IN_WORDS > OUT_WORDS) begin : unpacker
    bit [IN_WORD_W-1:0] in_word;
    forever begin
      i_queue.get(in_word);
      if (BIG_ENDIAN) begin
        for (int i = IN_WORDS/OUT_WORDS-1; i >= 0; i--) begin
          o_queue.put(in_word[OUT_WORD_W*i +: OUT_WORD_W]);
        end
      end else begin
        for (int i = 0; i < IN_WORDS/OUT_WORDS; i++) begin
          o_queue.put(in_word[OUT_WORD_W*i +: OUT_WORD_W]);
        end
      end
    end
  end else begin : packer
    bit [OUT_WORD_W-1:0] out_word;
    forever begin
      if (BIG_ENDIAN) begin
        for (int i = OUT_WORDS/IN_WORDS-1; i >= 0; i--) begin
          i_queue.get(out_word[IN_WORD_W*i +: IN_WORD_W]);
        end
      end else begin
        for (int i = 0; i < OUT_WORDS/IN_WORDS; i++) begin
          i_queue.get(out_word[IN_WORD_W*i +: IN_WORD_W]);
        end
      end
      o_queue.put(out_word);
    end
  end


  //---------------------------------------------------------------------------
  // Output Checker
  //---------------------------------------------------------------------------

  longint o_count;
  bit [OUT_WORD_W-1:0] expected, actual;

  initial forever begin : check_output
    string msg;
    clk_gen_out.clk_wait_r();
    if (o_rst) begin
      continue;
    end
    if (o_tvalid) begin
      o_count++;
      actual = o_tdata;
      o_queue.get(expected);
      msg = $sformatf("Output didn't match expected value! Expected 0x%0X, received 0x%0X.",
        expected, actual);
      `ASSERT_ERROR(actual == expected, msg);
    end
  end : check_output


  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------

  initial begin : tb_main
    string msg;
    string tb_name;
    int min_out_words, max_out_words;

    // Initialize the test exec object for this testbench
    tb_name = $sformatf( {
      "gearbox_2x1_tb\n",
      "IN_WORDS   = %01d\n",
      "OUT_WORDS  = %01d\n",
      "BIG_ENDIAN = %01d\n",
      "PHASE      = %01d" },
      IN_WORDS, OUT_WORDS, BIG_ENDIAN, PHASE
    );
    test.start_tb(tb_name);

    // Don't start the clocks until after start_tb() returns. This ensures that
    // the clocks aren't toggling while other instances of this testbench are
    // running, which speeds up simulation time.
    clk_gen_in.start();
    clk_gen_out.start();

    //--------------------------------
    // Reset
    //--------------------------------

    test.start_test("Reset", 10us);
    clk_gen_in.reset();
    clk_gen_out.reset();
    if (i_rst) @i_rst;
    if (o_rst) @o_rst;
    test.end_test();

    //--------------------------------
    // Test Sequences
    //--------------------------------

    test.start_test("Random data", 10ms);

    enable_input <= 1;

    // Let it run for a while
    clk_gen_in.clk_wait_r(NUM_TEST_CYCLES);

    // Stop inputting and wait long enough for any data to propagate through
    enable_input <= 0;
    repeat (8*OUT_WORDS) @i_clk;
    repeat (8*IN_WORDS)  @o_clk;

    // Calculate how many words we expect to have received on the output. We
    // might be in the middle of a word, so it might be less than what was
    // input.
    if (OUT_WORDS > IN_WORDS) begin
      min_out_words = ((i_count * IN_WORDS) / OUT_WORDS) * OUT_WORDS;
      max_out_words = min_out_words;
    end else begin
      min_out_words = (i_count-1) * IN_WORDS;
      max_out_words = i_count * IN_WORDS;
    end

    // Make sure the word counts match
    msg = $sformatf("Word counts don't match. Input %0d, output %0d.",
      IN_WORDS*i_count, OUT_WORDS*o_count);
    `ASSERT_ERROR(o_count*OUT_WORDS >= min_out_words &&
                  o_count*OUT_WORDS <= max_out_words , msg);
    msg = $sformatf("Only %0d words input. Expected about %0d.",
      i_count*IN_WORDS, 0.5*NUM_TEST_CYCLES*IN_WORDS);
    `ASSERT_ERROR(i_count > 0.4*NUM_TEST_CYCLES, msg);

    $display("Tested %0d output words", o_count);

    test.end_test();

    //--------------------------------
    // Finish Up
    //--------------------------------

    // End the TB, but don't $finish, since we don't want to kill other
    // instances of this testbench that may be running.
    test.end_tb(0);

    // Kill the clocks to end this instance of the testbench
    clk_gen_in.kill();
    clk_gen_out.kill();

  end : tb_main

endmodule : gearbox_2x1_tb


`default_nettype wire
