//
// Copyright 2026 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_hb_intp_fir_tb.sv
//
// Description:
//   Testbench for axis_hb_intp_fir — AXI halfband FIR x2 interpolator.
//

`default_nettype none

module axis_hb_intp_fir_tb #(
  parameter int  SAMP_W         = 48,  // Input/output sample word width (I+Q)
  parameter int  SPC_OUT        = 8,   // Output samples per clock (FIR parallelism)
  parameter int  NUM_COEFFS     = 47,  // Filter tap count: 47 or 63
  parameter int  COEFF_W        = 18,  // Coefficient width (informational)
  parameter bit  PRELOAD_ZEROES = 1'b1 // Pre-fill taps with zeros on rst/clear
) ();

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgAxiStreamBfm::*;
  import PkgRandom::*;
  import axis_hb_test_pkg::*;

  //---------------------------------------------------------------------------
  // Derived parameters
  //---------------------------------------------------------------------------

  // Input samples per clock: half of output rate (minimum 1).
  localparam int SPC_IN = (SPC_OUT > 1) ? SPC_OUT / 2 : 1;

  localparam int CLK_PERIOD_NS = 10;
  localparam int NUM_TESTS = 10;
  localparam int MIN_PACKET_LEN_WORDS = 50;

  // FIR pipeline delay in clock cycles — same formula as axis_hb_decim_fir_tb.sv.
  //   1  (center-tap latency)
  //   + (NUM_COEFFS-1)/4  (unique non-zero symmetric taps, after HBF zero-skip + symmetry)
  //   + 5  (fixed pipeline registers inside axi_fir_multisample_filter)
  localparam int FILTER_PIPELINE_DELAY = 1 + ((NUM_COEFFS - 1) / 4) + 5;

  // Packet lengths for TC1: impulse at word 0, zeros to flush the pipeline.
  localparam int IMPULSE_LEN_WORDS = FILTER_PIPELINE_DELAY + NUM_COEFFS + 1;
  localparam int FLUSH_LEN_WORDS   = IMPULSE_LEN_WORDS;

  //---------------------------------------------------------------------------
  // Type aliases via FilterHBTestUtils
  //---------------------------------------------------------------------------

  typedef FilterHBTestUtils#(
    .SAMP_W (SAMP_W),
    .SPC_IN (SPC_IN),
    .SPC_OUT(SPC_OUT)
  ) filter_utils_t;

  typedef filter_utils_t::axis_input_pkt_t              axis_input_pkt_t;
  typedef filter_utils_t::axis_output_pkt_t             axis_output_pkt_t;
  typedef filter_utils_t::axis_pkt_single_sample_t      axis_pkt_single_sample_t;
  typedef filter_utils_t::axis_pkt_single_sample_mbox_t axis_pkt_single_sample_mbox_t;
  typedef filter_utils_t::axis_input_pkt_mbox_t         axis_input_pkt_mbox_t;
  typedef filter_utils_t::axis_output_pkt_mbox_t        axis_output_pkt_mbox_t;

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

  //---------------------------------------------------------------------------
  // AXI BFMs
  //---------------------------------------------------------------------------

  AxiStreamIf #(
    .DATA_WIDTH(SPC_IN * SAMP_W)
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
    .DATA_WIDTH(SPC_IN * SAMP_W),
    .RESET_BEHAVIOR_MASTER(PkgAxiStreamBfm::DISCARD_PACKET)
  ) axi_bfm_in = new(
    .master(to_dut),
    .slave (null)
  );

  AxiStreamBfm #(
    .DATA_WIDTH(SPC_OUT * SAMP_W),
    .RESET_BEHAVIOR_SLAVE(PkgAxiStreamBfm::DISCARD_PACKET)
  ) axi_bfm_out = new(
    .master(null),
    .slave (from_dut)
  );

  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------

  axis_hb_intp_fir #(
    .SAMP_W        (SAMP_W),
    .SPC_IN        (SPC_IN),
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
  // Test tasks
  //---------------------------------------------------------------------------

  //---------------------------------------------------------------------------
  // test_filter_response
  //
  // Shared DUT-vs-model checker for impulse/random input packets.
  // For each run, feed one input packet and one zero-flush packet.
  //   Compare DUT output samples against AxiFirHBIntpModel output.
  //---------------------------------------------------------------------------
  task automatic test_filter_response(bit test_impulse = 1'b0);
    localparam int IMPULSE_LEN_WORDS_LOCAL  = FILTER_PIPELINE_DELAY + NUM_COEFFS + 1;
    localparam int TESTDATA_LEN_WORDS_LOCAL = FILTER_PIPELINE_DELAY + 64;
    axis_input_pkt_t              input_pkt, pkt_flush;
    axis_output_pkt_t             received_pkt;
    axis_pkt_single_sample_t      expected_sample_pkt;
    axis_pkt_single_sample_t      received_sample_pkt;
    axis_input_pkt_mbox_t         mbox_in         = new();
    axis_pkt_single_sample_mbox_t samples_in      = new();
    axis_pkt_single_sample_mbox_t samples_exp     = new();
    logic [SAMP_W-1:0]            received_sample;
    int                           received_count;
    int                           expected_count;
    int                           expected_words;
    int                           pipeline_delay_samples;

    AxiFirHBIntpModel #(
      .SAMP_W    (SAMP_W),
      .NUM_COEFFS(NUM_COEFFS)
    ) hb_model = new();

    if (test_impulse) begin
      input_pkt = filter_utils_t::generate_impulse_packet(IMPULSE_LEN_WORDS_LOCAL);
    end else begin
      input_pkt = filter_utils_t::generate_random_packet(
        MIN_PACKET_LEN_WORDS, TESTDATA_LEN_WORDS_LOCAL);
    end
    pkt_flush   = filter_utils_t::generate_zero_packet(FLUSH_LEN_WORDS);

    // Build expected scalar sample stream for the selected input packet only.
    // Flush packet is sent only to clean DUT history for the next testcase.
    mbox_in.put(input_pkt.copy());
    filter_utils_t::collect_and_serialize_packets(mbox_in, samples_in);
    hb_model.process_samples(samples_in, samples_exp);

    // Drive DUT with selected input followed by zeros to flush all FIR state.
    axi_bfm_in.put(input_pkt);
    axi_bfm_in.put(pkt_flush);
    axi_bfm_in.wait_complete();

    // Collect one full output packet (up to tlast) and compare against model.
    axi_bfm_out.get(received_pkt);

    // Packet length in words validates tlast placement for the selected input packet.
    // For interpolation-by-2, output words scale by (2*SPC_IN)/SPC_OUT.
    expected_words = (input_pkt.data.size() * (2 * SPC_IN)) / SPC_OUT;
    `ASSERT_ERROR(received_pkt.data.size() == expected_words, $sformatf(
      "Unexpected output packet length (tlast placement): expected %0d words, got %0d words",
      expected_words, received_pkt.data.size()));

    received_sample_pkt = filter_utils_t::serialize_packet(received_pkt);

    // Skip initial pipeline-delay samples and confirm they are zero.
    pipeline_delay_samples = (FILTER_PIPELINE_DELAY + 1) * SPC_OUT;
    repeat (pipeline_delay_samples) begin
      `ASSERT_ERROR(received_sample_pkt.data.size() > 0,
        "DUT output underflow while skipping pipeline-delay samples");
      if (received_sample_pkt.data.size() > 0) begin
        received_sample = received_sample_pkt.data.pop_front();
        `ASSERT_ERROR(received_sample === '0, $sformatf(
          "Expected zero during pipeline delay, got 0x%0h", received_sample));
      end
    end

    expected_count = samples_exp.num();
    received_count = received_sample_pkt.data.size();

    `ASSERT_ERROR((received_count + pipeline_delay_samples) == expected_count, $sformatf(
      "Sample-count mismatch: expected %0d total model samples, got %0d (%0d post-delay + %0d skipped)",
      expected_count, (received_count + pipeline_delay_samples), received_count,
      pipeline_delay_samples));

    if (test_impulse) begin
      `ASSERT_ERROR(received_count >= NUM_COEFFS, $sformatf(
        "Expected at least %0d post-delay samples for impulse validation, got %0d",
        NUM_COEFFS, received_count));
    end

    foreach (received_sample_pkt.data[sample]) begin
      received_sample = received_sample_pkt.data[sample];
      if (!samples_exp.try_get(expected_sample_pkt)) begin
        `ASSERT_ERROR(0, $sformatf(
          "Not enough expected samples for received sample %0d", sample));
      end else begin
        `ASSERT_ERROR(received_sample == expected_sample_pkt.data[0], $sformatf(
          "Data mismatch at sample %0d: expected 0x%0h, got 0x%0h",
          sample, expected_sample_pkt.data[0], received_sample));
      end
    end
    
  endtask : test_filter_response

  //---------------------------------------------------------------------------
  // test_preload_after_clear
  //
  //   1) Contaminate filter history with random data and drain output.
  //   2) Pulse clear (without reset).
  //   3) Send impulse and compare DUT output against a fresh model response.
  //---------------------------------------------------------------------------
  task automatic test_preload_after_clear();
    localparam int CONTAM_LEN_WORDS  = 2 * NUM_COEFFS + 1;
    localparam int IMPULSE_LEN_WORDS_LOCAL = FILTER_PIPELINE_DELAY + NUM_COEFFS + 1;

    axis_input_pkt_t              contam_pkt;
    axis_input_pkt_t              impulse_pkt;
    axis_output_pkt_t             received_pkt;
    axis_input_pkt_mbox_t         mbox_in         = new();
    axis_pkt_single_sample_mbox_t samples_in      = new();
    axis_pkt_single_sample_mbox_t samples_exp     = new();
    axis_pkt_single_sample_t      expected_sample_pkt;
    axis_pkt_single_sample_t      received_sample_pkt;
    logic [SAMP_W-1:0]            received_sample;
    int                           pipeline_delay_samples;

    AxiFirHBIntpModel #(
      .SAMP_W    (SAMP_W),
      .NUM_COEFFS(NUM_COEFFS)
    ) hb_model = new();

    // Build expected stream for an impulse from clean filter state.
    impulse_pkt = filter_utils_t::generate_impulse_packet(IMPULSE_LEN_WORDS_LOCAL);
    mbox_in.put(impulse_pkt.copy());
    filter_utils_t::collect_and_serialize_packets(mbox_in, samples_in);
    hb_model.process_samples(samples_in, samples_exp);

    // Step 1: Contaminate filter history and fully drain the resulting output packet.
    contam_pkt = filter_utils_t::generate_random_packet(CONTAM_LEN_WORDS, CONTAM_LEN_WORDS);
    axi_bfm_in.put(contam_pkt);
    axi_bfm_in.wait_complete();
    axi_bfm_out.get(received_pkt);

    // Step 2: Assert/deassert clear for 1 cycle without full reset.
    clk_gen.clk_wait_f();
    clear = 1'b1;
    clk_gen.clk_wait_f();
    clear = 1'b0;

    // Step 3: Send impulse after clear.
    axi_bfm_in.put(impulse_pkt.copy());
    axi_bfm_in.wait_complete();
    axi_bfm_out.get(received_pkt);
    received_sample_pkt = filter_utils_t::serialize_packet(received_pkt);

    // Step 4a: Leading pipeline delay samples must be zero.
    pipeline_delay_samples = (FILTER_PIPELINE_DELAY + 1) * SPC_OUT;
    repeat (pipeline_delay_samples) begin
      `ASSERT_ERROR(received_sample_pkt.data.size() > 0,
        "DUT output underflow while skipping clear+preload pipeline-delay samples");
      if (received_sample_pkt.data.size() > 0) begin
        received_sample = received_sample_pkt.data.pop_front();
        `ASSERT_ERROR(received_sample === '0, $sformatf(
          "After clear+preload: expected 0 during pipeline startup, got 0x%0h",
          received_sample));
      end
    end

    // Step 4b: Remaining samples must match the model's impulse response.
    foreach (received_sample_pkt.data[sample]) begin
      received_sample = received_sample_pkt.data[sample];
      if (!samples_exp.try_get(expected_sample_pkt)) begin
        `ASSERT_ERROR(0, $sformatf(
          "After clear+preload: no expected sample at sample %0d", sample));
      end else begin
        `ASSERT_ERROR(received_sample == expected_sample_pkt.data[0], $sformatf(
          "After clear+preload: mismatch at sample %0d: expected 0x%0h, got 0x%0h",
          sample, expected_sample_pkt.data[0], received_sample));
      end
    end
  endtask : test_preload_after_clear

  //---------------------------------------------------------------------------
  // Main testbench
  //---------------------------------------------------------------------------

  initial begin : main
    test.start_tb($sformatf(
      "axis_hb_intp_fir_tb: SAMP_W=%0d SPC_OUT=%0d NUM_COEFFS=%0d PRELOAD_ZEROES=%0b",
      SAMP_W, SPC_OUT, NUM_COEFFS, PRELOAD_ZEROES), 10ms);

    clk_gen.start();
    axi_bfm_in.run();
    axi_bfm_out.run();
    clk_gen.reset(10);
    @(negedge rst);

    test.start_test("TC1: Impulse Response Test", 100us);
    test_filter_response(1'b1);
    clk_gen.reset();
    @(negedge rst);
    test.end_test();

    test.start_test("TC2: Random Data Test", NUM_TESTS * 500us);
    repeat (NUM_TESTS) begin
      test_filter_response(1'b0);
      clk_gen.reset();
      @(negedge rst);
    end
    test.end_test();

    if (PRELOAD_ZEROES) begin
      test.start_test("TC3: Preload After Clear Test", 500us);
      test_preload_after_clear();
      clk_gen.reset();
      @(negedge rst);
      test.end_test();
    end

    clk_gen.kill();
    test.end_tb(0);
  end : main

endmodule : axis_hb_intp_fir_tb

`default_nettype wire
