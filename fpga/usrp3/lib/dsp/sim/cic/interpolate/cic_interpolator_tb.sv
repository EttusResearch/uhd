//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_interpolator_tb
//
// Description:
//
//    Testbench for the cic_interpolator module.
//
// Parameters:
//
//    SPC:    Number of samples processed per clock cycle.
//    SAMP_W: Input/output data width.
//    R_MAX:  Maximum interpolation factor supported.
//

`default_nettype none


module cic_interpolator_tb #(
  parameter int SPC    = 4,
  parameter int SAMP_W = 32,
  parameter int R_MAX  = 15
);
   // Test execution framework
  `include "test_exec.svh"
  import PkgTestExec::*;

  import PkgRandom::*;
  import PkgAxiStreamBfm::*;
  import cic_utils_pkg::*;
  import cic_test_pkg::*;


  //---------------------------------------------------------------------------
  // Local parameters and type definitions
  //---------------------------------------------------------------------------

  typedef cic_utils #(
    .SPC   (SPC),
    .SAMP_W(SAMP_W)
  ) util_c;

  typedef cic_test_utils #(
    .SPC    (SPC),
    .ACCUM_W(SAMP_W),
    .COMP_W (SAMP_W/2)
  ) test_util_c;

  localparam real CLK_PERIOD = 5.0;  // Clock period in nanoseconds
  localparam int  FACTOR_W   = dut.FACTOR_W;
  localparam int  WORD_W     = SPC * SAMP_W;

  // Testcase parameters
  localparam int NUM_TESTS      = 400;  // Number of random test iterations
  localparam int MAX_PKT_LEN    = 16;   // Max number of input words per packet
  localparam int STALL_PROB     = 38;   // Default stall probability percentage
  localparam bit VERBOSE        = 0;    // Verbose logging for debugging
  localparam bit USE_RANDOM     = 0;    // Use random vs. sequential data

  typedef util_c::sample_t            sample_t;       // Single sample (SAMP_W bits)
  typedef util_c::word_t              word_t;         // Packed array of SPC samples
  typedef test_util_c::sample_queue_t sample_queue_t; // Dynamic queue of samples
  typedef test_util_c::axis_pkt_t     axis_pkt_t;     // Packet type used by the test utilities


  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  logic clk;
  logic rst;

  sim_clock_gen #(
    .PERIOD   (CLK_PERIOD),
    .AUTOSTART(0)
  ) clk_gen (
    .clk(clk),
    .rst(rst)
  );


  //---------------------------------------------------------------------------
  // DUT Interface
  //---------------------------------------------------------------------------

  AxiStreamIf #(WORD_W) i_data (.clk, .rst);  // Input to the DUT
  AxiStreamIf #(WORD_W) o_data (.clk, .rst);  // Output from the DUT

  // Interpolation factor control
  logic [FACTOR_W-1:0] interp_factor;
  logic                interp_changed;


  //---------------------------------------------------------------------------
  // Bus Functional Models
  //---------------------------------------------------------------------------

  AxiStreamBfm #(WORD_W) bfm = new(.master(i_data), .slave(o_data));


  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------

  cic_interpolator #(
    .SAMP_W (SAMP_W),
    .SPC    (SPC   ),
    .R_MAX  (R_MAX )
  ) dut (
    .clk            (clk           ),
    .rst            (rst           ),
    .data_in        (i_data        ),
    .data_out       (o_data        ),
    .interp_factor  (interp_factor ),
    .interp_changed (interp_changed)
  );


  //---------------------------------------------------------------------------
  // Test Utilities
  //---------------------------------------------------------------------------

  // Builds the expected interpolated sample stream for a given input queue.
  //
  // Args:
  //   input_samps: Input samples in time order.
  //   factor: Interpolation factor. Each input sample is placed at
  //     output[input_sample_idx * factor].
  //
  // Returns:
  //   A sample queue of length input_samps.size() * factor, with inserted
  //   zero-valued samples between the original input samples.
  //
  function automatic sample_queue_t interpolate(
    sample_queue_t input_samps,
    int            factor
  );
    sample_queue_t output_samps;

    // Initialize all samples to 0
    repeat (input_samps.size() * factor) output_samps.push_back('0);

    // For each input sample, set the corresponding output sample
    foreach (input_samps[idx]) begin
      output_samps[idx * factor] = input_samps[idx];
    end
    return output_samps;
  endfunction : interpolate

  // Updates the DUT interpolation factor and pulses the change indicator.
  //
  // Args:
  //   factor: Interpolation factor value to drive into the DUT.
  //
  task automatic set_factor(int factor);
    @(posedge clk);
    interp_factor <= FACTOR_W'(factor);
    interp_changed <= 1;
    @(posedge clk);
    interp_changed <= 0;
    @(posedge clk);
  endtask : set_factor

  // Prints one word with its starting sample index.
  //
  // Args:
  //   idx: Starting sample index for the word being printed.
  //   word: Word to print, left-most sample first.
  //
  function automatic void print_word(int idx, word_t word);
    sample_t sample;
    $write("%04d: ", idx);
    foreach (word[samp_idx]) $write("%h ",  word[samp_idx]);
    $write("\n");
  endfunction : print_word

  
  // Waits for reset deassertion, then allows a few extra cycles for the DUT
  // and BFMs to settle before starting a test.
  task automatic reset_dut();
    clk_gen.reset(1);
    clk_gen.clk_wait_r(3);
  endtask : reset_dut


  //---------------------------------------------------------------------------
  // Test Tasks
  //---------------------------------------------------------------------------

  // Test basic functionality using a single packet. Verifies the interpolated
  // output stream.
  //
  // Args:
  //   num_words: Number of input words to generate.
  //   factor:    Interpolation factor to apply in the DUT and reference model.
  //              Set to 0 to use default value of the DUT.
  //
  task automatic test_basic(int num_words, int factor = 0);
    sample_queue_t input_samps;
    sample_queue_t exp_samps;
    axis_pkt_t     in_pkt;
    axis_pkt_t     out_pkt;
    int            active_factor;
    int            samp_count;

    string factor_desc;

    if (factor) begin
      set_factor(factor);
      active_factor = factor;
    end else begin
      active_factor = dut.factor;
    end

    if (factor) factor_desc = "Random";
    else factor_desc = "No Change";

    test.start_test($sformatf("test_basic: num_words=%0d, factor=%0d (%s)",
      num_words, active_factor, factor_desc));

    // Generate input packet
    in_pkt = new();
    samp_count = 0;
    repeat(num_words) begin
      word_t input_word;
      for (int samp_idx = 0; samp_idx < SPC; samp_idx++) begin
        input_word[samp_idx] = USE_RANDOM ? Rand#(SAMP_W)::rand_bit() :
                                            sample_t'(samp_count++);
        input_samps.push_back(input_word[samp_idx]);
      end
      in_pkt.data.push_back(input_word);
    end

    if (VERBOSE) begin
      $display("--- Input Packet: %0d word(s) ---", in_pkt.data.size());
      foreach (in_pkt.data[word_idx]) begin
        print_word(word_idx, in_pkt.data[word_idx]);
      end
    end

    bfm.put(in_pkt);

    // Compute expected output using reference model.
    exp_samps = interpolate(input_samps, active_factor);

    // Collect and verify output words.
    if (VERBOSE) begin
      $display("--- Output Packet: expecting %0d word(s) ---",
               exp_samps.size() / SPC);
    end

    // Process output packets
    samp_count = 0;
    while (samp_count < exp_samps.size()) begin
      bfm.get(out_pkt);
      foreach (out_pkt.data[word_idx]) begin
        word_t word = out_pkt.data[word_idx];

        if (VERBOSE) print_word(word_idx, word);

        for (int samp_idx = 0; samp_idx < SPC; samp_idx++) begin
          sample_t expected = exp_samps[samp_count];
          `ASSERT_ERROR(
            word[samp_idx] == expected,
            $sformatf(
              "Sample %0d: expected 0x%0h, received 0x%0h",
              samp_count, expected, word[samp_idx])
          );
          samp_count++;
        end
      end
    end

    test.end_test();
  endtask : test_basic


  // Generates random input packets, sends them to the DUT, and checks the
  // output against the reference model.
  //
  // Args:
  //   num_iter: Number of test iterations to run, each with a different random
  //             factor.
  //   factor:   Set to 0 to get a random factor, otherwise this factor will be
  //             used.
  //
  task automatic test_random(int num_iter = 1, int factor = 0);
    const int MAX_NUM_PKT = 4;

    test.start_test($sformatf("test_random: num_iter=%0d, factor=%s",
      num_iter, factor ? $sformatf("%0d", factor) : "RANDOM"));

    repeat (num_iter) begin : iter_loop
      int            active_factor;
      int            num_words;
      int            num_pkts;
      int            samp_count;
      sample_queue_t input_pkts [];
      sample_queue_t exp_pkts [];

      active_factor = (factor > 0) ? factor : $urandom_range(1, R_MAX);
      set_factor(active_factor);

      num_pkts  = $urandom_range(1, MAX_NUM_PKT);
      num_words = $urandom_range(1, MAX_PKT_LEN);

      input_pkts = new [num_pkts];
      exp_pkts   = new [num_pkts];

      // Send input packets
      samp_count = 0;
      for (int pkt_idx = 0; pkt_idx < num_pkts; pkt_idx++) begin
        axis_pkt_t in_pkt;
        in_pkt = new();
        repeat (num_words) begin
          word_t input_word;
          for (int samp_idx = 0; samp_idx < SPC; samp_idx++) begin
            input_word[samp_idx] = USE_RANDOM ? Rand#(SAMP_W)::rand_bit() :
                                                sample_t'(samp_count++);
          end
          in_pkt.data.push_back(input_word);
        end

        // Compute the expected output
        exp_pkts[pkt_idx] = interpolate(
          test_util_c::pkt_to_samples(in_pkt.copy()), active_factor);

        if (VERBOSE) begin
          $display("--- Input Packet %0d: %0d word(s) (factor=%0d) ---",
                   pkt_idx, in_pkt.data.size(), active_factor);
          foreach (in_pkt.data[word_idx]) begin
            print_word(word_idx, in_pkt.data[word_idx]);
          end
        end

        bfm.put(in_pkt);
      end

      // Read output packets from DUT and verify against expected output.
      for (int pkt_idx = 0; pkt_idx < num_pkts; pkt_idx++) begin : verify_loop
        axis_pkt_t out_pkt;
        bfm.get(out_pkt);

        if (VERBOSE) begin
          $display("--- Output Packet %0d: %0d word(s) (factor=%0d) ---",
                   pkt_idx, out_pkt.data.size(), active_factor);
        end

        samp_count = 0;

        // Iterate over each output word
        foreach (out_pkt.data[word_idx]) begin : word_loop
          word_t recv_word = out_pkt.data[word_idx];

          if (VERBOSE) begin
            print_word(word_idx, recv_word);
          end

          // Check each sample
          for (int samp_idx = 0; samp_idx < SPC; samp_idx++) begin : sample_loop
            sample_t exp_samp = exp_pkts[pkt_idx][samp_count];
            `ASSERT_ERROR(
              recv_word[samp_idx] == exp_samp,
              $sformatf(
                "Packet %0d, Word %0d, Sample %0d: expected 0x%0h, received 0x%0h",
                pkt_idx, word_idx, samp_count, exp_samp,
                recv_word[samp_idx])
            );
            samp_count++;
          end : sample_loop
        end : word_loop
      end : verify_loop
    end : iter_loop

    test.end_test();
  endtask : test_random


  //---------------------------------------------------------------------------
  // Main Testbench
  //---------------------------------------------------------------------------

  initial begin : main
    string tb_name;

    typedef struct {
      int mst;
      int slv;
    } stall_prob_t;

    // Stall probabilities to test
    stall_prob_t stall_probs [] = '{
      '{ STALL_PROB, STALL_PROB },
      '{         10,         90 },
      '{         90,         10 },
      '{          0,          0 }
    };

    tb_name = $sformatf(
      "cic_interpolator_tb (SPC=%0d, SAMP_W=%0d, R_MAX=%0d)",
      SPC, SAMP_W, R_MAX
    );

    test.start_tb(tb_name, 100ms);

    // Start the clock, BFM, then reset
    clk_gen.start();
    bfm.run();
    clk_gen.reset();
    reset_dut();

    //---------------------------------
    // Tests
    //---------------------------------

    test_basic(.num_words(16), .factor(4));  // Basic sanity check

    foreach (stall_probs[idx]) begin
      $display("Setting input/output stall prob to %0d/%0d",
        stall_probs[idx].mst, stall_probs[idx].slv);
      bfm.set_master_stall_prob(stall_probs[idx].mst);
      bfm.set_slave_stall_prob(stall_probs[idx].slv);

      reset_dut();

      // Check the default interpolation factor
      test.start_test("Verify default interpolation factor");
      `ASSERT_ERROR(dut.factor == 1, "Default factor is not 1!");
      test.end_test();
      test_basic(.num_words(16), .factor(0));  // Test using default factor

      test_random(1, 1);       // Test min interpolation
      test_random(1, R_MAX);   // Test max interpolation
      test_random(NUM_TESTS);  // Test random interpolation
    end

    test.end_tb(0);
    clk_gen.kill();
  end : main

endmodule : cic_interpolator_tb


`default_nettype wire
