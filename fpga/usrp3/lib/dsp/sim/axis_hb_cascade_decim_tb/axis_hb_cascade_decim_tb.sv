//
// Copyright 2026 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_hb_cascade_decim_tb.sv
//
// Description: Testbench for AXI halfband filter cascade decimator
//


`default_nettype none

module axis_hb_cascade_decim_tb #(
  parameter int  SAMP_W = 48,
  parameter int  SPC    = 8,
  parameter int  NUM_HB = 3,        // Number of halfband filter stages in cascade
  parameter int  PRELOAD_ZEROES = 0 // Instantiate DUT with zero preload (flush) support.
) ();

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgAxiStreamBfm::*;
  import PkgRandom::*;

  import axis_hb_test_pkg::*;

  //-----------------------------------------------------------------
  // Local parameters
  //-----------------------------------------------------------------
  localparam int CLK_PERIOD_NS     = 10;
  localparam int NUM_TESTS         = 10;
  localparam int MIN_PKT_LEN_WORDS = (SPC > 1) ? 200 : 400;
  localparam int MAX_PKT_LEN_WORDS = (SPC > 1) ? 250 : 500;
  localparam int MAX_PKTS_PER_TEST = 5;
  localparam bit VERBOSE           = 0;

  // Generate filter coefficient array with HB_DECIM_MAX_NUM_HB entries,
  // set HB_NUM_COEFFS[2] to 63, such that for NUM_HB = 3 we will get
  // [47, 47, 63] which is the current decimation cascade configuration.
  localparam int HB_NUM_COEFFS [axis_hb_utils_pkg::HB_DECIM_MAX_NUM_HB]
                               = '{2: 63, default: 47};

  //-----------------------------------------------------------------
  // Local type definitions
  //-----------------------------------------------------------------
  typedef FilterHBTestUtils#(
    .SAMP_W(SAMP_W),
    .SPC_IN(SPC)
  ) filter_utils_t;
  typedef filter_utils_t::axis_input_pkt_t axis_pkt_t;
  typedef filter_utils_t::axis_pkt_single_sample_t axis_pkt_single_sample_t;
  typedef filter_utils_t::axis_pkt_single_sample_mbox_t axis_pkt_single_sample_mbox_t;
  typedef filter_utils_t::axis_pkt_queue_t axis_pkt_queue_t;
  typedef filter_utils_t::axis_input_pkt_mbox_t axis_pkt_mbox_t;

  //-----------------------------------------------------------------
  // Local helper functions
  //-----------------------------------------------------------------
  // Calculate the expected pipeline delay in scalar samples for a given number of
  // stages in the cascade based on the per-stage filter coefficient counts and the
  // internal axi_fir_multisample filter pipeline delay formula.
  // This is used to determine how many output samples to skip in the test before
  // starting to compare against the model output, since the model does not include
  // the pipeline delay.
  function automatic int get_pipeline_delay_samples(input int stages);
    int pd_words [];           // per-stage pipeline delay in clock cycles
    int spc_in   [];           // per-stage input SPC
    int cascade_delay_cycles;  // running cascade delay in clock cycles
    int spc_out;               // output SPC of the last active stage

    if (stages == 0) return 0;

    pd_words = new[stages];
    spc_in   = new[stages];

    // Calculate per-stage input SPC and pipeline delay (in clock cycles / words).
    // Stage 0 runs at the full SPC; each subsequent stage runs at half the SPC,
    // clamped to 1 (time-domain decimation when SPC reaches 1).
    // NOTE: the used formula is (NUM_COEFFS + 5) / 4 + 5 words, which is specific to the
    //       axi_fir_multisample_filter implementation with optimizations for
    //       symmetric halfband coefficients (pipeline depth = 5, filter slices = (N+5)/4).
    spc_out = SPC;
    for (int i = 0; i < stages; i++) begin
      spc_in[i]   = spc_out;
      pd_words[i] = (HB_NUM_COEFFS[i] + 5) / 4 + 5;
      if (spc_out > 1) spc_out = spc_out / 2;
    end

    // Compute cascade delay using the iterative formula:
    //   width-domain stage (spc_in > 1):  cascade_delay_cycles += pd_words[i]
    //   time-domain stage  (spc_in == 1): cascade_delay_cycles = (cascade_delay_cycles + pd_words[i]) / 2
    // The result is expressed in clock cycles of the last active stage.
    cascade_delay_cycles = 0;
    for (int i = 0; i < stages; i++) begin
      if (spc_in[i] == 1) begin
        cascade_delay_cycles = (cascade_delay_cycles + pd_words[i]) / 2;
      end else begin
        cascade_delay_cycles = cascade_delay_cycles + pd_words[i];
      end
    end

    if (VERBOSE) begin
      $display("get_pipeline_delay_samples: stages=%0d total_delay_scalar_samples=%0d",
               stages, cascade_delay_cycles * spc_out);
      // Print per-stage details in processing order (left to right, stage 0 to stages-1).
      for (int i = 0; i < stages; i++) begin
        $display("  >> stage %0d: num_coeffs=%0d pd_words=%0d spc_in=%0d",
                 i, HB_NUM_COEFFS[i], pd_words[i], spc_in[i]);
      end
    end

    // Convert cascade delay (clock cycles) to scalar samples using the
    // output SPC of the last stage
    // (last stage produces spc_out samples per clock cycle).
    return cascade_delay_cycles * spc_out;
  endfunction : get_pipeline_delay_samples

  //-----------------------------------------------------------------
  // Clock and reset
  //-----------------------------------------------------------------
  logic clk;
  logic rst;
  sim_clock_gen #(
      .PERIOD(CLK_PERIOD_NS)
  ) clk_gen (
      .clk(clk),
      .rst(rst)
  );

  //-----------------------------------------------------------------
  // AXI BFM
  //-----------------------------------------------------------------
  AxiStreamIf #(
      .DATA_WIDTH(SPC * SAMP_W)
  ) to_dut (
      .clk(clk),
      .rst(rst)
  );
  AxiStreamIf #(
      .DATA_WIDTH(SPC * SAMP_W)
  ) from_dut (
      .clk(clk),
      .rst(rst)
  );
  AxiStreamBfm #(
    .DATA_WIDTH(SPC * SAMP_W),
    .RESET_BEHAVIOR_SLAVE(PkgAxiStreamBfm::DISCARD_PACKET)
  ) axi_bfm = new(
    .master(to_dut),
    .slave(from_dut)
  );

  //-----------------------------------------------------------------
  // DUT Instantiation
  //-----------------------------------------------------------------
  logic [1:0] num_stages = 2'd0;
  axis_hb_cascade_decim #(
    .SAMP_W        (SAMP_W),
    .SPC           (SPC),
    .NUM_HB        (NUM_HB),
    .HB_NUM_COEFFS (HB_NUM_COEFFS),
    .PRELOAD_ZEROES(PRELOAD_ZEROES)
  ) dut (
    // Clocking
    .clk          (clk),
    .rst          (rst),
    .clear        (1'b0),
    // AXI Stream input
    .s_axis_tdata (to_dut.tdata),
    .s_axis_tvalid(to_dut.tvalid),
    .s_axis_tready(to_dut.tready),
    .s_axis_tlast (to_dut.tlast),
    // AXI Stream output
    .m_axis_tdata (from_dut.tdata),
    .m_axis_tvalid(from_dut.tvalid),
    .m_axis_tready(from_dut.tready),
    .m_axis_tlast (from_dut.tlast),
    .m_axis_tkeep (from_dut.tkeep[SPC-1:0]), // AxiStreamIf tkeep: 1 bit per byte
                                             // DUT tkeep: 1 bit per sample
    // Configuration
    .num_stages   (num_stages)
  );

  // --------------------------------------------------------------------------
  // run_stage: Send random packets through the DUT with a given stage count,
  // compare DUT output sample-by-sample against the reference model.
  // Assumes the DUT is in a clean state before being called.
  // --------------------------------------------------------------------------
  task automatic run_stage(
    input string tc_name,
    input int stages_req        = 0,
    input int num_pkts          = MAX_PKTS_PER_TEST,
    input int min_pkt_len_words = MIN_PKT_LEN_WORDS,
    input int max_pkt_len_words = MAX_PKT_LEN_WORDS
  );
    axis_pkt_queue_t test_data;
    axis_pkt_single_sample_mbox_t expected_data = new();
    axis_pkt_mbox_t  input_data = new();
    axis_pkt_t received_pkt;
    axis_pkt_single_sample_mbox_t serialized_input_pkts = new();
    axis_pkt_single_sample_t expected_sample_pkt, received_sample_pkt;
    logic [SAMP_W-1:0] received_sample;
    logic [SAMP_W-1:0] expected_sample;
    bit first_sample_received = 1'b0;
    int num_samples_compared  = 0;
    // Worst-case flush input words to drain the full pipeline such that we get the
    // tlast signal on the last output packet.
    // Derived from worst case SPC = 1 (all decimation in time) which would require
    // ∑(pd_words[i] * 2^i) for i=0..NUM_HB-1, SPC=1, NUM_HB=4, all 63-tap stages.
    localparam int FLUSH_WORDS = 400; // 330 + margin for safety

    // Reference model construction
    AxiFirHBCascadeDecimModel #(
      .SAMP_W        (SAMP_W),
      .NUM_HB        (NUM_HB),
      .HB_NUM_COEFFS (HB_NUM_COEFFS)
    ) cascade_model;

    // Clear any late-arriving flush packet left over from the previous run_stage().
    // The reset between runs discards active RX transfers, but it does not clear the
    // BFM receive mailbox.
    begin
      automatic axis_pkt_t drain_tmp;
      while (axi_bfm.try_get(drain_tmp)) begin end
    end

    // Generate random test data
    test_data = FilterHBTestUtils#(
        .SAMP_W(SAMP_W),
        .SPC_IN(SPC)
    )::generate_random_test_data(
        num_pkts, min_pkt_len_words, max_pkt_len_words
    );

    // Configure DUT before sending any data.
    if (stages_req < 0) begin
      num_stages = 2'd0;
    end else if (stages_req > NUM_HB) begin
      num_stages = NUM_HB[1:0];
    end else begin
      num_stages = stages_req[1:0];
    end

    // Send packets to DUT and feed a copy into the reference model.
    foreach (test_data[i]) begin
      if (VERBOSE) $display("Sending packet %0d with %0d words", i, test_data[i].data.size());
      input_data.put(test_data[i].copy());  // For Filter sim model, removes packet boundaries
      axi_bfm.put(test_data[i]);
    end
    //   send additional flush packet to the DUT to force the remaining output tail through
    //   cascade pipeline such that we get the tlast signal on the last output packet.
    axi_bfm.put(filter_utils_t::generate_zero_packet(FLUSH_WORDS));

    axi_bfm.wait_complete();

    // Serialize input data and run reference model to get expected output.
    filter_utils_t::collect_and_serialize_packets(input_data, serialized_input_pkts);
    cascade_model = new();
    cascade_model.process_samples(serialized_input_pkts, expected_data, stages_req);
    expected_data.put(null); // add end of data marker

    // Check DUT output against expected data
    repeat (test_data.size()) begin
      // The first sample is expected after pipeline_delay_samples samples have been received.
      // pipeline_delay_samples is 0 for subsequent packets since the pipeline is already full.
      int pipeline_delay_samples = first_sample_received ? 0 : get_pipeline_delay_samples(num_stages);

      // Check DUT output against expected data
      axi_bfm.get(received_pkt);
      if (VERBOSE) $display("Received packet with %0d words", received_pkt.data.size());
      received_sample_pkt = FilterHBTestUtils#(
        .SAMP_W(SAMP_W),
        .SPC_IN(SPC),
        .SPC_OUT(SPC)
      )::serialize_packet(
          received_pkt
      );
      repeat (pipeline_delay_samples) begin
        logic [SAMP_W/8-1:0] temp_keep;
        received_sample = received_sample_pkt.data.pop_front();
        temp_keep = received_sample_pkt.keep.pop_front();
        if (VERBOSE) $display("Discard sample received during pipeline delay: 0x%0h", received_sample);
        `ASSERT_ERROR(received_sample === '0, $sformatf(
                    "Expected samples to be 0 during pipeline delay cycles, but received sample %0h",
                    received_sample
                    ));
      end
      first_sample_received = 1'b1;
      // Compare received packet to expected data
      foreach (received_sample_pkt.data[sample]) begin
        // For each sample in the word, check against expected data
        received_sample = received_sample_pkt.data[sample];
        if (!expected_data.try_peek(expected_sample_pkt)) begin
          `ASSERT_ERROR(0, $sformatf(
                        "Not enough expected samples for received data at sample %0d", sample));
        end else begin
          if (received_sample_pkt.keep[sample] == 0) begin
            if (VERBOSE)
              $display(
                  "Received sample %0d is marked invalid by tkeep, skipping comparison",
                  sample
              );
            continue;  // Skip comparison for invalid samples
          end
          expected_data.get(expected_sample_pkt);  // Move expected data forward
          expected_sample = expected_sample_pkt.data[0];
          if (VERBOSE) begin
          $display("Received sample %0d: 0x%0h. Expected: 0x%0h", sample, received_sample, expected_sample);
          end
          if (received_sample != expected_sample) begin
            // Debugging output for data mismatch
            if (VERBOSE) begin
              // Print lookahead: next 10 received samples (from the already-serialized array)
              $display("  >> Lookahead of next received samples:");
              for (int la = 1; la <= 10; la++) begin
                int la_idx = sample + la;
                if (la_idx < received_sample_pkt.data.size()) begin
                  $display("    received[%0d] = 0x%0h", la_idx, received_sample_pkt.data[la_idx]);
                end
              end
              // Print lookahead: drain up to 10 more expected samples (consuming them,
              // test will fail anyway, so this is for diagnosis only)
              $display("  >> Lookahead of next expected samples:");
              for (int la = 1; la <= 10; la++) begin
                automatic axis_pkt_single_sample_t la_pkt;
                if (expected_data.try_get(la_pkt)) begin
                  $display("    expected[%0d] = 0x%0h", la, la_pkt.data[0]);
                end else begin
                  $display("    expected[%0d] = (no more samples)", la);
                  break;
                end
              end
            end
            // Assertion failure for data mismatch
            `ASSERT_ERROR(received_sample == expected_sample, $sformatf(
                          "Data mismatch at sample %0d: expected 0x%0h, got 0x%0h",
                          sample,
                          expected_sample,
                          received_sample
                          ));
          end
          num_samples_compared++;
        end
      end
    end

    $display("run_stage: %s completed successfully with %0d stages, %0d packets, %0d samples compared",
             tc_name, stages_req, num_pkts, num_samples_compared);
  endtask : run_stage

  // --------------------------------------------------------------------------
  // Random Data: Baseline deterministic DUT-vs-model random data sweep.
  // Validates:
  //   - Deterministic stage sweep (caller-provided stage_values).
  //   - Exact sample-by-sample DUT-vs-model equivalence.
  // --------------------------------------------------------------------------
  task automatic test_random_data(
    input int num_tests = NUM_HB + 1  // Default: one test for num_stage = 0..NUM_HB
  );
    // TC constants
    int test_num_pkts;
    int test_num_stages;

    for (int test_iter = 0; test_iter < num_tests; test_iter++) begin
      test_num_stages = test_iter % (NUM_HB + 1); // Cycle through 0..NUM_HB stages
      test_num_pkts = $urandom_range(1, MAX_PKTS_PER_TEST);
      // Reset DUT.
      clk_gen.reset();
      @(negedge rst);
      clk_gen.clk_wait_r(100);
      // Run the test (creates test data, runs the DUT and compares the data)
      run_stage(
        .tc_name("Random data"),
        .stages_req(test_num_stages),
        .num_pkts(test_num_pkts),
        .min_pkt_len_words(MIN_PKT_LEN_WORDS),
        .max_pkt_len_words(MAX_PKT_LEN_WORDS)
      );
    end
  endtask : test_random_data

  //-----------------------------------------------------------------
  // Test procedure
  //-----------------------------------------------------------------
  initial begin
    localparam string test_name = $sformatf(
        "axis_hb_cascade_decim_tb: NUM_HB:%0d, SPC:%0d, SAMP_W:%0d, PRELOAD_ZEROES:%0d",
        NUM_HB, SPC, SAMP_W, PRELOAD_ZEROES
    );

    test.start_tb(test_name, 10ms);
    // Initialization
    clk_gen.start();
    axi_bfm.run();
    clk_gen.reset(10);
    @(negedge rst);

    // Testcase: Random data test (with default stall probabilities)
    test.start_test("Random Data Test", NUM_TESTS * 250us);
    test_random_data(.num_tests(NUM_TESTS));
    test.end_test();

    test.end_tb(0);
    clk_gen.kill();
  end

endmodule : axis_hb_cascade_decim_tb
