//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_comb_filter_tb
//
// Description:
//
//   Testbench for cic_comb_filter (fixed delay D=1).
//
// Parameters:
//
//   SPC           : Number of samples per clock cycle. Must be a power of 2.
//   ACCUM_W       : Accumulator width in bits, used as bus width for the DUT.
//   SAMP_W        : Original sample width in bits. Each sample contains I and
//                   Q components, each SAMP_W/2 bits wide. Used for test
//                   input generation.
//

`default_nettype none

module cic_comb_filter_tb #(
  int SPC           = 2,
  int ACCUM_W       = 96,
  int SAMP_W        = 32
);

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"

  import PkgTestExec::*;

  import PkgAxiStreamBfm::*;
  import PkgRandom::*;

  import cic_utils_pkg::*;
  import cic_test_pkg::*;
  import cic_comb_filter_test_pkg::*;

  `include "usrp_utils.svh"

  //---------------------------------------------------------------------------
  // Type definitions
  //---------------------------------------------------------------------------
  typedef cic_comb_filter_test_utils #(
    .SPC(SPC),
    .ACCUM_W(ACCUM_W),
    .COMP_W(SAMP_W/2)
  ) utils_t;

  typedef cic_test_utils #(
    .SPC    (SPC),
    .ACCUM_W(ACCUM_W),
    .COMP_W (SAMP_W/2)
  ) ctest_utils_t;

  typedef utils_t::axis_packet_t                      axis_packet_t;
  typedef utils_t::axis_single_sample_packet_t        axis_single_sample_packet_t;
  typedef utils_t::axis_packet_queue_t                axis_packet_queue_t;
  typedef utils_t::axis_single_sample_packet_queue_t  axis_single_sample_packet_queue_t;
  typedef utils_t::sample_t                           sample_t;
  typedef sample_t                                    sample_queue_t[$];

  //---------------------------------------------------------------------------
  // Testbench parameters
  //---------------------------------------------------------------------------
  localparam real CLK_PERIOD = 10.0;
  localparam int  STALL_PROB_LOW = 10;
  localparam int  STALL_PROB_DEFAULT = 50;
  localparam int  STALL_PROB_HIGH = 80;
  localparam int  NUM_TESTS = 10;

  // Packet length range (in SPC-wide words). With D=1 the history is just
  // one sample, so a modest minimum suffices.
  localparam int  MIN_WORDS = SPC + 2;
  localparam int  MAX_WORDS = MIN_WORDS + 200;
  localparam int  MAX_PKTS  = 10;

  localparam bit  VERBOSE = 0;

  //---------------------------------------------------------------------------
  // Testbench signals
  //---------------------------------------------------------------------------
  logic clk;
  logic rst;
  logic clr = 1'b0;

  //---------------------------------------------------------------------------
  // Clock generator
  //---------------------------------------------------------------------------
  sim_clock_gen #(
    .PERIOD(CLK_PERIOD)
  ) clk_gen (
    .clk(clk),
    .rst(rst)
  );

  //---------------------------------------------------------------------------
  // AXI-Stream interfaces
  //---------------------------------------------------------------------------
  AxiStreamIf #(
    .DATA_WIDTH(ACCUM_W * SPC)
  ) to_dut (
    .clk(clk),
    .rst(rst)
  );

  AxiStreamIf #(
    .DATA_WIDTH(ACCUM_W * SPC)
  ) from_dut (
    .clk(clk),
    .rst(rst)
  );

  //---------------------------------------------------------------------------
  // AXI-Stream BFM
  //---------------------------------------------------------------------------
  AxiStreamBfm #(
    .DATA_WIDTH     (ACCUM_W * SPC),
    .RESET_BEHAVIOR_SLAVE (PkgAxiStreamBfm::DISCARD_PACKET)
  ) axi_bfm = new(
    .master(to_dut),
    .slave (from_dut)
  );

  //---------------------------------------------------------------------------
  // DUT instantiation
  //---------------------------------------------------------------------------
  cic_comb_filter #(
    .ACCUM_W(ACCUM_W),
    .SPC    (SPC)
  ) dut (
    .clk     (clk),
    .rst     (rst),
    .clr     (clr),
    .data_in (to_dut),
    .data_out(from_dut)
  );

  //---------------------------------------------------------------------------
  // Testcase logic
  //---------------------------------------------------------------------------

  // test_comb_filter: drives the DUT and a bit-true simulation model with
  // identical data, then compares every output sample.
  //
  // When use_random_data is 1 (default), input samples are fully random.
  // When use_random_data is 0, every sample has I=i_value, Q=q_value.
  // num_packets controls how many back-to-back packets are sent per
  // invocation (default 1). The model retains state across packets.
  task automatic test_comb_filter(
    bit use_random_data = 1'b1,
    int i_value = 0,
    int q_value = 0,
    int num_packets = 1
  );
    int                         num_words;
    axis_packet_t               input_pkt;
    axis_packet_t               rx_pkt;
    axis_single_sample_packet_t serialized_pkt;
    axis_packet_queue_t         pkt_queue;
    sample_queue_t              input_samples;
    sample_queue_t              expected_samples;
    sample_queue_t              dut_samples;

    // Bit-true simulation model (delay=1).
    cic_comb_filter_test_model #(
      .ACCUM_W(ACCUM_W)
    ) model = new();

    // Generate all input packets.
    for (int pkt_idx = 0; pkt_idx < num_packets; pkt_idx++) begin
      num_words = $urandom_range(MIN_WORDS, MAX_WORDS);
      if (use_random_data) begin
        input_pkt = ctest_utils_t::generate_random_packet(num_words);
      end else begin
        input_pkt = new();
        repeat (num_words) begin
          input_pkt.data.push_back(ctest_utils_t::gen_data_word(
            .i_value(i_value), .q_value(q_value), .random_data(0)));
        end
      end
      pkt_queue.push_back(input_pkt);

      if (VERBOSE) begin
        if (use_random_data) begin
          $display("  pkt[%0d/%0d] num_words=%0d  (random data)",
                   pkt_idx, num_packets, num_words);
        end else begin
          $display("  pkt[%0d/%0d] num_words=%0d  (I=%0d, Q=%0d)",
                   pkt_idx, num_packets, num_words, i_value, q_value);
        end
      end
    end

    // Run all packets through the simulation model.
    input_samples = utils_t::serialize_packets_to_sample_queue(pkt_queue);
    model.process_samples(input_samples, expected_samples);

    // Drive each packet through the DUT and collect output samples.
    foreach (pkt_queue[pkt_idx]) begin
      axi_bfm.put(pkt_queue[pkt_idx]);
      axi_bfm.wait_complete();
      axi_bfm.get(rx_pkt);

      serialized_pkt = utils_t::serialize_packet(rx_pkt);
      dut_samples = { dut_samples, serialized_pkt.data }; 
    end

    // Check total sample count.
    `ASSERT_ERROR(dut_samples.size() == expected_samples.size(),
      $sformatf("sample count mismatch: expected %0d, got %0d",
                expected_samples.size(), dut_samples.size()));

    // Compare every sample.
    for (int samp_idx = 0; samp_idx < dut_samples.size(); samp_idx++) begin
      if (VERBOSE) begin
        $display("  sample[%0d]: expected 0x%0h, got 0x%0h",
                 samp_idx, expected_samples[samp_idx], dut_samples[samp_idx]);
      end
      `ASSERT_ERROR(dut_samples[samp_idx] === expected_samples[samp_idx],
        $sformatf("sample[%0d]: expected 0x%0h, got 0x%0h",
                  samp_idx, expected_samples[samp_idx], dut_samples[samp_idx]));
    end
    clk_gen.reset();
    @(negedge rst);
  endtask : test_comb_filter

  //---------------------------------------------------------------------------
  // Test execution
  //---------------------------------------------------------------------------
  task automatic run_testcases();

    // Initialize testbench infrastructure.
    clk_gen.start();
    axi_bfm.run();
    axi_bfm.set_master_stall_prob(STALL_PROB_DEFAULT);
    axi_bfm.set_slave_stall_prob(STALL_PROB_DEFAULT);

    // Release DUT from reset.
    clk_gen.reset();
    @(negedge rst);

    // Testcase: random data.
    test.start_test("Random data", NUM_TESTS * 1ms);
    repeat (NUM_TESTS) begin
      test_comb_filter();
    end
    test.end_test();

    // Testcase: multiple back-to-back packets without reset.
    test.start_test("Multiple packets per test", NUM_TESTS * 5ms);
    repeat (3) begin
      test_comb_filter(.num_packets(MAX_PKTS));
    end
    test.end_test();

    // Testcase: high master stall probability.
    axi_bfm.set_master_stall_prob(STALL_PROB_HIGH);
    axi_bfm.set_slave_stall_prob(STALL_PROB_LOW);
    test.start_test("Random data: high master stall probability", NUM_TESTS * 5ms);
    repeat (NUM_TESTS) begin
      test_comb_filter();
    end;
    test.end_test();

    // Testcase: high slave stall probability.
    axi_bfm.set_master_stall_prob(STALL_PROB_LOW);
    axi_bfm.set_slave_stall_prob(STALL_PROB_HIGH);
    test.start_test("Random data: high slave stall probability", NUM_TESTS * 5ms);
    repeat (NUM_TESTS) begin
      test_comb_filter();
    end;
    test.end_test();

  endtask : run_testcases

  initial begin
    localparam string test_name = $sformatf(
        "cic_comb_filter_tb: SPC:%0d, DUT_SAMP_W:%0d", SPC, ACCUM_W
    );
    test.start_tb(test_name);
    run_testcases();
    test.end_tb(0);
    clk_gen.kill();
  end

endmodule

`default_nettype wire
