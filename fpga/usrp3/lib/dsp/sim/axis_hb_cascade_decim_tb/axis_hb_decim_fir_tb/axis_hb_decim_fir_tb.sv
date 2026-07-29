//
// Copyright 2026 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_hb_decim_fir_tb.sv
//
// Description: Testbench wrapper to for the AXI halfband generic FIR filter module
//

`default_nettype none

module axis_hb_decim_fir_tb #(
  parameter int  SAMP_W         = 48,   // Input/output data width
  parameter int  SPC            = 8,    // Number of interleaved samples per clock
  parameter int  NUM_COEFFS     = 47,   // Number of filter coefficients, either 47 or 63
  parameter int  COEFF_W        = 18,   // Coefficient width
  parameter bit  PRELOAD_ZEROES = 1'b0  // Enable zero-preload on reset/clear
) ();

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgAxiStreamBfm::*;
  import PkgRandom::*;

  import axis_hb_test_pkg::*;

  //---------------------------------------------------------------------------
  // Local parameters
  //---------------------------------------------------------------------------
  localparam int CLK_PERIOD_NS = 10;
  localparam int NUM_TESTS = 10;

  localparam int SCALING_W = SAMP_W/2 - COEFF_W;  // Difference between sample width and coefficient width

  // Pipeline delay minus one for the optimized halfband FIR filter implementation.
  // The additional +1 cycle is applied later when calculating the output-sample delay.
  // Comprised of:
  //  1(center tap)
  //  + ceil((NUM_COEFFS -1)/4) (save half of the non-center taps for symmetry and another half for every other coefficient 0)
  //  + 5 (additional fixed pipeline registers in the design)
  localparam int FILTER_PIPELINE_DELAY = 1 + ((NUM_COEFFS - 1) / 4) + 5;

  localparam bit VERBOSE = 1'b0;

  localparam int SPC_OUT = (SPC==1) ? SPC : SPC / 2;  // Number of interleaved samples in output stream after decimation by 2
  localparam int MAXIMUM_I_Q_VALUE = (1 << (SAMP_W/2 - 1)) - 1;  // Maximum positive value for I/Q samples, used for impulse generation
  localparam int MIN_PACKET_LEN_WORDS = 50;  // Minimum number of data words in randomly generated packets for testing

  //-----------------------------------------------------------------
  // Local type definitions
  //-----------------------------------------------------------------
  typedef FilterHBTestUtils#(
    .SAMP_W  (SAMP_W),
    .SPC_IN  (SPC),
    .SPC_OUT (SPC_OUT)
  ) filter_utils_t;

  typedef filter_utils_t::axis_input_pkt_t                axis_input_pkt_t;
  typedef filter_utils_t::axis_output_pkt_t               axis_output_pkt_t;
  typedef filter_utils_t::axis_pkt_single_sample_t        axis_pkt_single_sample_t;
  typedef filter_utils_t::axis_pkt_single_sample_mbox_t   axis_pkt_single_sample_mbox_t;
  typedef filter_utils_t::axis_pkt_queue_t                axis_pkt_queue_t;
  typedef filter_utils_t::axis_input_pkt_mbox_t           axis_input_pkt_mbox_t;
  typedef filter_utils_t::axis_output_pkt_mbox_t          axis_output_pkt_mbox_t;

  //---------------------------------------------------------------------------
  // Clock and reset
  //---------------------------------------------------------------------------
  logic clk;
  logic rst;
  logic clear = 1'b0;
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
    .DATA_WIDTH(SPC_OUT * SAMP_W)
  ) from_dut (
    .clk(clk),
    .rst(rst)
  );

  AxiStreamBfm #(
    .DATA_WIDTH(SPC * SAMP_W),
    .RESET_BEHAVIOR_MASTER(PkgAxiStreamBfm::DISCARD_PACKET)
  ) axi_bfm_in = new(
    .master(to_dut),
    .slave(null)  // BFM is only driving input stream, so slave interface is not used
  );

  AxiStreamBfm #(
    .DATA_WIDTH(SPC_OUT * SAMP_W),
    .RESET_BEHAVIOR_SLAVE(PkgAxiStreamBfm::DISCARD_PACKET)
  ) axi_bfm_out = new(
    .master(null),  // BFM is only monitoring output stream, so master interface is not used
    .slave(from_dut)
  );


  //---------------------------------------------------------------------------
  // DUT instantiation
  //---------------------------------------------------------------------------
  axis_hb_decim_fir #(
    .SAMP_W        (SAMP_W),
    .SPC_IN        (SPC),
    .SPC_OUT       (SPC_OUT),
    .NUM_COEFFS    (NUM_COEFFS),
    .PRELOAD_ZEROES(PRELOAD_ZEROES)
  ) dut (
    .clk          (clk),
    .rst          (rst),
    .clear        (clear),
    .s_axis_tdata (to_dut.tdata),
    .s_axis_tvalid(to_dut.tvalid),
    .s_axis_tlast (to_dut.tlast),
    .s_axis_tready(to_dut.tready),
    .m_axis_tdata (from_dut.tdata),
    .m_axis_tvalid(from_dut.tvalid),
    .m_axis_tlast (from_dut.tlast),
    .m_axis_tready(from_dut.tready),
    .enable       (1'b1)
  );

  //---------------------------------------------------------------------------
  // Testcase logic
  //---------------------------------------------------------------------------

  // Test impulse response of the filter and compare against expected values,
  // in this case the filter coefficients, computed from the filter model.
  task automatic test_filter_response(bit test_impulse = 1'b0);
    // Number of words in the impulse packet
    localparam int IMPULSE_LEN_WORDS  = FILTER_PIPELINE_DELAY + NUM_COEFFS + 1;
    localparam int TESTDATA_LEN_WORDS = FILTER_PIPELINE_DELAY + 64;
    axis_input_pkt_t input_pkt;
    axis_input_pkt_t flush_pkt;
    axis_output_pkt_t received_pkt;
    axis_pkt_single_sample_t expected_sample_pkt;
    axis_input_pkt_mbox_t input_pkts = new();
    axis_pkt_single_sample_mbox_t input_samples = new(), expected_samples = new();
    axis_pkt_single_sample_t received_sample_pkt;
    axis_output_pkt_mbox_t received_pkts = new();
    int pipeline_delay_cycles = FILTER_PIPELINE_DELAY;  // FIR filter delay in cycles
    logic [SAMP_W-1:0] received_sample;
    int received_sample_count;
    // FIR filter delay in number of output samples, which is different from
    // the number of cycles for SPC > 1 due to decimation and multiple samples per word.
    int pipeline_delay_samples = (SPC == 1)
      ? (((pipeline_delay_cycles * SPC_OUT) + 1) / 2)
      : ( (pipeline_delay_cycles + 1) * SPC_OUT);

    AxiFirHBDecimModel #(
      .SAMP_W(SAMP_W),
      .NUM_COEFFS(NUM_COEFFS)
    ) hbmodel = new();

    // Generate impulse input packet
    if (test_impulse) begin
      input_pkt = filter_utils_t::generate_impulse_packet(
          IMPULSE_LEN_WORDS
      );
      // Generate random input packet
    end else begin
      input_pkt = filter_utils_t::generate_random_packet(
          MIN_PACKET_LEN_WORDS, TESTDATA_LEN_WORDS
      );
    end
    input_pkts.put(input_pkt.copy());

    // Include the zero-flush packet in the expected stream so the model matches the DUT output.
    flush_pkt = filter_utils_t::generate_zero_packet(
      TESTDATA_LEN_WORDS
    );
    input_pkts.put(flush_pkt.copy());

    // Compute expected output samples
    filter_utils_t::collect_and_serialize_packets(
        input_pkts, input_samples
    );

    hbmodel.process_samples(input_samples, expected_samples);

    // Send input packet to DUT
    axi_bfm_in.put(input_pkt);

    // Flush filter for next run test run by sending all zeroes.
    axi_bfm_in.put(flush_pkt);

    // Wait for enough of the Flush packet to be processed to ensure filter is fully flushed.
    // The internal DUT data shift register is expected to filled with only zeroes before the next test.
    axi_bfm_in.wait_complete();


    // Check output packets from DUT and compare against expected samples

    // Collect output packets from DUT
    // Check if we expect to receive more samples from the dut
    if (expected_samples.num() > 0) begin
      axi_bfm_out.get(received_pkt);
      received_sample_pkt = filter_utils_t::serialize_packet(
          received_pkt
      );
      received_sample_count = received_sample_pkt.data.size();
      repeat (pipeline_delay_samples) begin
        received_sample = received_sample_pkt.data.pop_front();
        `ASSERT_ERROR(received_sample === '0, $sformatf(
                      "Expected samples to be 0 during pipeline delay cycles, but received sample %0h",
                      received_sample
                      ));
      end
      // Count only valid samples (after removing the initial pipeline-delay samples).
      received_sample_count = received_sample_pkt.data.size();
      // Align to the first expected sample to contain the impulse response
      foreach (received_sample_pkt.data[sample]) begin
        // For each sample in the word, check against expected data
        received_sample = received_sample_pkt.data[sample];
        if (VERBOSE) begin
          $display("Received sample %0d: 0x%0h", sample, received_sample);
        end
        if (!expected_samples.try_get(expected_sample_pkt)) begin
          `ASSERT_ERROR(0, $sformatf(
                        "Not enough expected samples for received data at sample %0d", sample));
        end else begin
          logic [SAMP_W-1:0] expected_sample;
          expected_sample = expected_sample_pkt.data[0];
          `ASSERT_ERROR(received_sample == expected_sample, $sformatf(
                        "Data mismatch at sample %0d: expected 0x%0h, got 0x%0h",
                        sample,
                        expected_sample_pkt.data[0],
                        received_sample
                        ));
        end
      end
      // Additional check to ensure we received enough samples to validate the full impulse
      // response, which is NUM_COEFFS/2 samples due to decimation by 2.
      if (test_impulse) begin
        `ASSERT_ERROR(received_sample_count >= (NUM_COEFFS / 2), $sformatf(
                      {
                        "Did not receive enough output samples to validate the full impulse response.",
                        " Expected at least %0d samples, but received only %0d samples."
                      },
                      NUM_COEFFS / 2,
                      received_sample_count
                      ));
      end
    end
  endtask : test_filter_response


  // Test that after a `clear` pulse (without full reset) the PRELOAD_ZEROES
  // mechanism restores a clean filter state, producing an impulse response
  // identical to what follows a hard reset (both start from zero tap history).
  //
  // Test flow:
  //   1. Contaminate filter history by sending a burst of random non-zero data
  //      and draining its output (output content is discarded — we only care
  //      that the filter tap history is non-zero after this step).
  //   2. Assert `clear` for a few cycles (no rst).
  //   3. Send an impulse immediately after deasserting clear.
  //      PRELOAD_ZEROES stalls the impulse input for PIPELINE_DELAY beats while
  //      zeroing the shift register, then lets the impulse through.
  //   4. Verify:
  //      a) The first pipeline_delay_samples output words are zero.
  //      b) The subsequent samples match the reference impulse response from
  //         the software filter model (zero-initial-state).
  //
  // Model strategy:
  //   The contam_pkt is intentionally NOT fed to the software model because its
  //   output is fully discarded by the DUT drain (axi_bfm_out.get() after
  //   wait_complete()). After the clear+PRELOAD_ZEROES sequence, the DUT's tap
  //   history is known to be all-zero. The model is instantiated fresh (new()),
  //   which also starts with all-zero tap history — this implicitly represents
  //   the post-clear state without needing to pass a clear signal to the model.
  //   Therefore the model only needs to process the impulse packet to produce
  //   the expected DUT output.
  task automatic test_preload_after_clear();
    // Enough words to fill the entire FIR data shift register with non-zero data:
    //   SHIFT_REG_WIDTH = (SPC+1)*NUM_COEFFS entries; each word contributes SPC entries.
    //   Using 2*NUM_COEFFS+1 words ensures an odd packet length so that the last
    //   output always falls on a "kept" toggle-sample (toggle=1), preventing a
    //   pending_tlast deadlock in axi_bfm_out.get().
    localparam int CONTAM_LEN_WORDS  = 2 * NUM_COEFFS + 1;
    localparam int IMPULSE_LEN_WORDS = FILTER_PIPELINE_DELAY + NUM_COEFFS + 1;
    // Number of DSP-pipeline output samples to skip before checking impulse data.
    // Matches the formula used in test_filter_response.
    int pipeline_delay_samples = (SPC == 1)
      ? (((FILTER_PIPELINE_DELAY * SPC_OUT) + 1) / 2)
      : ((FILTER_PIPELINE_DELAY + 1) * SPC_OUT);

    axis_input_pkt_t  contam_pkt;
    axis_input_pkt_t  impulse_pkt;
    axis_input_pkt_t  flush_pkt;
    axis_output_pkt_t received_pkt;
    axis_input_pkt_mbox_t       pkts_for_model  = new();
    axis_pkt_single_sample_mbox_t input_samples = new();
    axis_pkt_single_sample_mbox_t expected_samples = new();
    axis_pkt_single_sample_t expected_sample_pkt;

    AxiFirHBDecimModel #(
      .SAMP_W(SAMP_W), .NUM_COEFFS(NUM_COEFFS)
    ) hb_model = new();

    // Build expected output for an impulse starting from zero initial state.
    impulse_pkt = filter_utils_t::generate_impulse_packet(IMPULSE_LEN_WORDS);
    flush_pkt   = filter_utils_t::generate_zero_packet(IMPULSE_LEN_WORDS);
    pkts_for_model.put(impulse_pkt.copy());
    pkts_for_model.put(flush_pkt.copy());
    filter_utils_t::collect_and_serialize_packets(pkts_for_model, input_samples);
    hb_model.process_samples(input_samples, expected_samples);

    // --- Step 1: Contaminate filter history ---
    // Send the contam packet. Wait for full TX completion before draining output:
    // this guarantees no residual contam words remain queued in axi_bfm_in before
    // the clear pulse, so data_shift_reg is not re-dirtied after the preload.
    // Use an odd packet length to guarantee the contam tlast falls on a "kept"
    // toggle-sample slot (toggle_sample=1 at test start; odd length means the
    // last word arrives on an even beat index, i.e., toggle=1 -> tlast fires
    // immediately instead of setting pending_tlast, which would deadlock get()).
    // Use a fixed odd length (not random) so the property holds regardless of
    // test ordering or RNG seed.
    contam_pkt = filter_utils_t::generate_random_packet(
                    CONTAM_LEN_WORDS, CONTAM_LEN_WORDS);
    axi_bfm_in.put(contam_pkt);
    axi_bfm_in.wait_complete();    // wait for ALL contam words to be accepted by DUT
    axi_bfm_out.get(received_pkt); // drain the COMPLETE contam output packet: get() blocks
                                   // until it sees tlast on the output, which is propagated
                                   // from the contam input's tlast through the filter.
                                   // This consumes every contam output word already queued
                                   // in the BFM as well as any still draining from the pipeline.
                                   // After this returns, no contam output remains.

    // --- Step 2: Assert clear (no rst), then deassert ---
    // PRELOAD_ZEROES triggers a new fill+drain sequence, overwriting all FIR
    // state (data_shift_reg positions 0..NUM_COEFFS) with zeros and flushing
    // the DSP pipeline before the output gate opens.
    clk_gen.clk_wait_f();
    clear = 1'b1;
    clk_gen.clk_wait_f();
    clear = 1'b0;

    // --- Step 3: Send impulse immediately after clear ---
    // PRELOAD_ZEROES stalls this until the shift register is flushed with zeros.
    axi_bfm_in.put(impulse_pkt.copy());
    axi_bfm_in.put(flush_pkt.copy());
    axi_bfm_in.wait_complete();
    axi_bfm_out.get(received_pkt);

    // --- Step 4: Verify output ---
    begin
      axis_pkt_single_sample_t received_sample_pkt;
      logic [SAMP_W-1:0] received_sample;

      received_sample_pkt = filter_utils_t::serialize_packet(received_pkt);

      // 4a: Leading pipeline-delay outputs must be zero (preload filled history)
      repeat (pipeline_delay_samples) begin
        received_sample = received_sample_pkt.data.pop_front();
        `ASSERT_ERROR(received_sample === '0, $sformatf(
          "After clear+preload: expected 0 during pipeline startup, got 0x%0h",
          received_sample));
      end

      // 4b: Remaining samples must match the model's impulse response
      foreach (received_sample_pkt.data[i]) begin
        received_sample = received_sample_pkt.data[i];
        if (!expected_samples.try_get(expected_sample_pkt)) begin
          `ASSERT_ERROR(0, $sformatf(
            "After clear+preload: no expected sample at index %0d", i));
        end else begin
          `ASSERT_ERROR(received_sample == expected_sample_pkt.data[0], $sformatf(
            "After clear+preload: mismatch at sample %0d: expected 0x%0h, got 0x%0h",
            i, expected_sample_pkt.data[0], received_sample));
        end
      end
    end
  endtask : test_preload_after_clear

  //---------------------------------------------------------------------------
  // Test execution
  //---------------------------------------------------------------------------
  initial begin : main
    localparam string test_name = $sformatf(
        "axis_hb_decim_fir_tb: SAMP_W:%02d, SPC:%02d, NUM_COEFFS:%02d, PRELOAD_ZEROES:%b",
        SAMP_W,
        SPC,
        NUM_COEFFS,
        PRELOAD_ZEROES
    );

    test.start_tb(test_name, 10ms);
    // Initialization
    clk_gen.start();
    axi_bfm_in.run();
    axi_bfm_out.run();
    clk_gen.reset(10);
    @(negedge rst);

    // Testcase 1: Impulse response test
    test.start_test("Impulse Response Test", 200us);
    if (VERBOSE) begin
      $display("--------------------------------------------------");
      $display("Starting impulse response test iteration");
    end
    test_filter_response(1'b1);
    clk_gen.reset();
    @(negedge rst);
    test.end_test();

    // Testcase 2: Random data test
    test.start_test("Random Data Test", NUM_TESTS * 200us);
    repeat (NUM_TESTS) begin
      if (VERBOSE) begin
        $display("--------------------------------------------------");
        $display("Starting random data test iteration");
      end
      test_filter_response(1'b0);
      clk_gen.reset();
      @(negedge rst);
    end
    test.end_test();

    // Testcase 3: PRELOAD_ZEROES on clear
    // Verifies that asserting `clear` (without full reset) followed by a new
    // burst produces output identical to starting from a hard-reset state,
    // i.e., PRELOAD_ZEROES successfully re-initialises the tap history.
    if (PRELOAD_ZEROES) begin
      test.start_test("Preload After Clear Test", 2ms);
      test_preload_after_clear();
      clk_gen.reset();
      @(negedge rst);
      test.end_test();
    end

    clk_gen.kill();
    test.end_tb(0);
  end : main

endmodule : axis_hb_decim_fir_tb

`default_nettype wire
