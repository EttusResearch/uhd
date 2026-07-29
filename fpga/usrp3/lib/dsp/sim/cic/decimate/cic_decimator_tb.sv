//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_decimator_tb
//
// Description:
//    Testbench for the cic_decimator module.
//

`default_nettype none

module cic_decimator_tb #(
  parameter int SPC    = 8,   // Number of samples processed per clock cycle
  parameter int SAMP_W = 48,  // Input/output data width
  parameter int R_MAX  = 255  // Maximum decimation factor supported
);
  `include "test_exec.svh"  // Include the test execution framework

  import PkgTestExec::*;
  import PkgAxiStreamBfm::*;
  import cic_utils_pkg::*;
  import cic_test_pkg::*;

  //-----------------------------------------------------------------------------------------------
  // Local parameters and type definitions
  //-----------------------------------------------------------------------------------------------

  typedef cic_utils#(
    .SPC   (SPC),
    .SAMP_W(SAMP_W)
  ) util_c;

  typedef cic_test_utils #(
    .SPC    (SPC),
    .ACCUM_W(SAMP_W),
    .COMP_W (SAMP_W/2)
  ) test_util_c;

  localparam real CLK_PERIOD = 5.0;  // Clock period in nanoseconds
  localparam int SPC_LOG2 = (SPC == 1) ? 1 : $clog2(SPC);
  localparam int R_MAX_LOG2 = (R_MAX == 1) ? 1 : $clog2(R_MAX+1);

  // Testcase parameters
  localparam int NUM_TESTS = 10;  // Number of random test iterations
  localparam int MIN_PKTS = 1;  // Minimum number of packets to send in a test
  localparam int MIN_WORDS = 20;  // Minimum number of words per packet
  localparam int MAX_WORDS = 10 * R_MAX;  // Maximum number of words per packet
  localparam int MIN_STALL_PROB = 20;  // Min probability (in %) of BFM input stalls
  localparam int MAX_STALL_PROB = 50;  // Max probability (in %) of BFM input stalls
  localparam bit VERBOSE = 0;  // Verbose logging for debugging

  typedef util_c::sample_t            sample_t;  // Single sample (SAMP_W bits)
  typedef util_c::word_t              word_t;  // Packed array of SPC samples
  typedef test_util_c::sample_queue_t sample_queue_t; // Dynamic queue of samples
  typedef test_util_c::axis_pkt_t     axis_pkt_t;  // Packet type used by the test utilities
  typedef test_util_c::pkt_burst_t    pkt_burst_t;  // Burst of packets
  // Enumeration for different test modes
  typedef enum {
    TEST_MODE_PACKET,
    TEST_MODE_BURST,
    TEST_MODE_R_MIN,
    TEST_MODE_R_MAX
  } test_mode_t;

  //-----------------------------------------------------------------------------------------------
  // Clocks and Resets
  //-----------------------------------------------------------------------------------------------
  logic clk;
  logic rst;

  sim_clock_gen #(
    .PERIOD(CLK_PERIOD),
    .AUTOSTART(0)
  ) clk_gen (
    .clk(clk),
    .rst(rst)
  );

  //-----------------------------------------------------------------------------------------------
  // DUT interface
  //-----------------------------------------------------------------------------------------------

  // Inputs to the DUT
  AxiStreamIf #(.DATA_WIDTH(SPC * SAMP_W), .TKEEP(0), .TUSER(0)) i_data (
    .clk(clk),
    .rst(rst)
  );
  // Outputs from the DUT
  AxiStreamIf #(.DATA_WIDTH(SPC * SAMP_W), .TKEEP(0), .TUSER(0)) o_data (
    .clk(clk),
    .rst(rst)
  );
  // Decimation factor control
  logic [R_MAX_LOG2-1:0] decimation_factor;
  logic                  decimation_factor_changed;

  //-----------------------------------------------------------------------------------------------
  // Bus Functional Models
  //-----------------------------------------------------------------------------------------------

  // AxiStreamBfm instance for interfacing with the DUT.
  //   master → drives i_data (BFM TX → DUT input)
  //   slave  → reads  o_data (DUT output → BFM RX)
  AxiStreamBfm #(.DATA_WIDTH(SPC * SAMP_W), .TKEEP(0), .TUSER(0)) bfm = new(.master(i_data), .slave(o_data));

  //-----------------------------------------------------------------------------------------------
  // DUT
  //-----------------------------------------------------------------------------------------------
  logic clr = 1'b0;

  cic_decimator #(
    .SAMP_W(SAMP_W),
    .SPC(SPC)
  ) dut (
    .clk(clk),
    .rst(rst),
    .clr(clr),
    .data_in(i_data),
    .data_out(o_data),
    .decim_factor(decimation_factor),
    .decim_changed(decimation_factor_changed)
  );

  //-----------------------------------------------------------------------------------------------
  // Test Utilities
  //-----------------------------------------------------------------------------------------------
  // Reference model for the CIC decimator. This model will be used to compute the expected output
  // for a given input and decimation factor.
  // Given an array of input samples and a decimation factor R, the model will produce an array of
  // output samples which only includes every R-th sample from the input starting with the first sample.
  function automatic sample_queue_t sample_cic_decimate(input sample_queue_t input_samples,
                                                        input int R);
    sample_queue_t output_samples;
    foreach (input_samples[i]) begin
      if (i % R == 0) begin
        output_samples.push_back(input_samples[i]);
      end
    end
    return output_samples;
  endfunction : sample_cic_decimate

  // Utility task to set the decimation factor in the DUT. This task passes the given decimation
  // factor to the DUT and pulses the change signal to notify the DUT of the new factor.
  task automatic set_decimation_factor(input int R);
    decimation_factor = R;
    decimation_factor_changed = 1;  // Pulse the change signal to notify the DUT of the new factor
    clk_gen.clk_wait_r(1);  // Wait for a clock cycle to ensure the DUT registers the change
    decimation_factor_changed = 0;
    clk_gen.clk_wait_r(1);  // Wait for another clock cycle to allow the DUT to react to the change
  endtask : set_decimation_factor

  // Assert the DUT clear input and keeps it asserted for the specified number of full clock cycles.
  task automatic assert_clear(input int num_cycles);
    clk_gen.clk_wait_f();
    clr = 1'b1;
    clk_gen.clk_wait_r();
    clk_gen.clk_wait_r(num_cycles);
    clr = 1'b0;
  endtask : assert_clear

  //-----------------------------------------------------------------------------------------------
  // Testcase: Random Decimation Factor
  //-----------------------------------------------------------------------------------------------
  // This testcase generates an input packet of random length (length is at least MIN_WORDS) with
  // length being a multiple of the decimation factor. The packet is sent to the DUT and the output
  // is checked against a reference model. The decimation factor is randomly chosen for each test 
  // iteration and can range from 1 to R_MAX.
  task automatic test_random_decimation_factor(test_mode_t mode = TEST_MODE_PACKET);
    // Randomize decimation factor between 1 and R_MAX
    int R, min_multiples, max_multiples, pkt_length_words, samples_received, num_pkts;
    sample_queue_t input_samples, expected_output_samples, actual_output_samples;
    axis_pkt_t input_pkt;
    word_t word;
    pkt_burst_t input_burst;

    // Randomize packet length (at least MIN_WORDS and a multiple of R)
    case (mode)
      TEST_MODE_R_MIN: R = 1;  // Minimum decimation factor
      TEST_MODE_R_MAX: R = R_MAX;  // Maximum decimation factor
      default:         R = $urandom_range(1, R_MAX);  // Random decimation factor between 1 and R_MAX
    endcase
    set_decimation_factor(R);

    min_multiples = (MIN_WORDS + R - 1) / R;
    max_multiples = MAX_WORDS / R;

    `ASSERT_ERROR(max_multiples >= min_multiples,
                  $sformatf(
                      "No valid packet length exists for R=%0d (MIN_WORDS=%0d, MAX_WORDS=%0d)", R,
                      MIN_WORDS, MAX_WORDS))
    pkt_length_words = (min_multiples + $urandom_range(0, max_multiples - min_multiples)) * R;

    case (mode)
      TEST_MODE_PACKET: num_pkts = 1;
      TEST_MODE_BURST:  num_pkts = $urandom_range(2, 10);  // Randomize number of packets in burst
      default:          num_pkts = 1;
    endcase

    repeat (num_pkts) begin
      // Generate random input samples for the packet
      input_pkt = new();
      for (int i = 0; i < pkt_length_words; i++) begin
        word_t random_word = test_util_c::gen_data_word(.random_data(1));
        input_pkt.data.push_back(random_word);
      end
      input_samples = {input_samples, test_util_c::pkt_to_samples(input_pkt.copy())};
      input_burst.push_back(input_pkt);
      bfm.put(input_pkt);
    end

    // Compute expected output using reference model
    expected_output_samples = sample_cic_decimate(input_samples, R);

    // Read output packets from DUT and verify against expected output
    samples_received = 0;
    while (samples_received < expected_output_samples.size()) begin
      axis_pkt_t received_pkt;
      bfm.get(received_pkt);
      foreach (received_pkt.data[i]) begin
        word_t word = received_pkt.data[i];
        for (int j = 0; j < SPC; j++) begin
          if (samples_received < expected_output_samples.size()) begin
            sample_t expected_samp = expected_output_samples[samples_received];
            if (VERBOSE) begin
              $display("Expected: 0x%0h, Actual: 0x%0h", expected_samp, word[j]);
            end
            `ASSERT_ERROR(word[j] === expected_samp,
                          $sformatf("R=%0d, sample %0d: Expected 0x%0h, but got 0x%0h",
                                    R, samples_received, expected_samp, word[j]))
            samples_received++;
          end
        end
      end
    end
  endtask

  // Verify that clear resets the decimation phase after a partial input group.
  task automatic test_clear();
    localparam int CLEAR_TEST_R = 3;
    localparam int CONTAM_WORDS = CLEAR_TEST_R - 1;
    localparam int TEST_WORDS = 2 * CLEAR_TEST_R;

    axis_pkt_t contam_pkt;
    axis_pkt_t input_pkt;
    axis_pkt_t received_pkt;
    sample_queue_t input_samples, expected_output_samples;
    int samples_received;

    set_decimation_factor(CLEAR_TEST_R);

    contam_pkt = new();
    for (int i = 0; i < CONTAM_WORDS; i++) begin
      contam_pkt.data.push_back(test_util_c::gen_data_word(.random_data(1)));
    end
    bfm.put(contam_pkt);
    bfm.wait_complete();

    assert_clear(2);

    input_pkt = new();
    for (int i = 0; i < TEST_WORDS; i++) begin
      word_t random_word = test_util_c::gen_data_word(.random_data(1));
      input_pkt.data.push_back(random_word);
    end
    input_samples = test_util_c::pkt_to_samples(input_pkt.copy());
    expected_output_samples = sample_cic_decimate(input_samples, CLEAR_TEST_R);
    bfm.put(input_pkt);

    samples_received = 0;
    while (samples_received < expected_output_samples.size()) begin
      bfm.get(received_pkt);
      foreach (received_pkt.data[i]) begin
        word_t word = received_pkt.data[i];
        for (int j = 0; j < SPC; j++) begin
          if (samples_received < expected_output_samples.size()) begin
            `ASSERT_ERROR(word[j] === expected_output_samples[samples_received],
                          $sformatf("Clear test sample %0d: Expected 0x%0h, but got 0x%0h",
                                    samples_received, expected_output_samples[samples_received],
                                    word[j]))
            samples_received++;
          end
        end
      end
    end
  endtask : test_clear

  //-----------------------------------------------------------------------------------------------
  // Main Testbench task
  //-----------------------------------------------------------------------------------------------
  task automatic testbench_main();
    string tb_name = $sformatf(
      "CIC Multisample Decimate Testbench (SPC=%0d, SAMP_W=%0d)", SPC, SAMP_W
    );
    test.start_tb(tb_name, 10ms);
    // Start the clock
    clk_gen.start();
    // Start the BFM
    bfm.run();
    // Reset the DUT
    clk_gen.reset();
    @(negedge rst);  // Wait for reset to be deasserted

    test.start_test("Clear Test");
    test_clear();
    test.end_test();

    // Run the random decimation factor testcase for a number of iterations
    test.start_test("Random Decimation Factor Test - Single Packet");
    repeat (NUM_TESTS) begin
      test_random_decimation_factor(TEST_MODE_PACKET);
    end
    test.end_test();

    test.start_test("Random Decimation Factor Test - Burst");
    repeat (NUM_TESTS) begin
      test_random_decimation_factor(TEST_MODE_BURST);
    end
    test.end_test();

    test.start_test("Random Decimation Factor Test - R=1 (no decimation)");
    repeat (NUM_TESTS) begin
      test_random_decimation_factor(TEST_MODE_R_MIN);
    end
    test.end_test();

    test.start_test("Random Decimation Factor Test - R=R_MAX");
    repeat (NUM_TESTS) begin
      test_random_decimation_factor(TEST_MODE_R_MAX);
    end
    test.end_test();

    test.start_test("High input stall probability test");
    bfm.set_master_stall_prob(MAX_STALL_PROB);
    bfm.set_slave_stall_prob(MIN_STALL_PROB);
    repeat (NUM_TESTS) begin
      test_random_decimation_factor(TEST_MODE_PACKET);
    end
    test.end_test();

    test.start_test("High output stall probability test");
    bfm.set_master_stall_prob(MIN_STALL_PROB);
    bfm.set_slave_stall_prob(MAX_STALL_PROB);
    repeat (NUM_TESTS) begin
      test_random_decimation_factor(TEST_MODE_PACKET);
    end
    test.end_test();

    test.end_tb(0);
    clk_gen.kill();
  endtask

  //-----------------------------------------------------------------------------------------------
  // Test Execution
  //-----------------------------------------------------------------------------------------------
  initial begin
    testbench_main();
  end

endmodule

`default_nettype wire
