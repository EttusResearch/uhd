//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_hb_cascade_intp_tb
//
// Description: Skeleton testbench for AXI halfband filter cascade interpolator.
//

`default_nettype none

module axis_hb_cascade_intp_tb #(
  parameter int SAMP_W         = 48,
  parameter int SPC            = 8,
  parameter int NUM_HB         = 3,
  parameter bit PRELOAD_ZEROES = 0
) ();

  `include "test_exec.svh"

  import PkgTestExec::*;
  import PkgAxiStreamBfm::*;

  import axis_hb_test_pkg::*;

  localparam int CLK_PERIOD_NS = 10;
  localparam bit VERBOSE       = 0;

  // Generate filter coefficient array in left-to-right stage order.
  // Fixed size array with HB_INTP_MAX_NUM_HB elements,
  // but only the first NUM_HB elements are used.
  // (fixed size is required to support zero-stage configurations,
  //  since unpacked arrays cannot have zero dimension).
  localparam int HB_NUM_COEFFS [axis_hb_utils_pkg::HB_INTP_MAX_NUM_HB] = '{default: 47};
  //localparam int HB_NUM_COEFFS [axis_hb_utils_pkg::HB_INTP_MAX_NUM_HB] = '{0: 63, default: 47};
  
  // Array of stage sweep values to run the tests on.
  // 0...NUM_HB (i.e. cascade with 0, 1, 2, ..., NUM_HB stages).
  typedef int stage_sweep_arr_t [NUM_HB+1];
  function automatic stage_sweep_arr_t gen_stage_sweep();
    stage_sweep_arr_t sweep;
    foreach (sweep[i]) sweep[i] = i;
    return sweep;
  endfunction : gen_stage_sweep
  localparam int TB_STAGE_SWEEP [NUM_HB+1] = gen_stage_sweep();

  typedef AxiStreamPacket#(.DATA_WIDTH(SPC * SAMP_W)) axis_pkt_t;
  typedef AxiStreamPacket#(.DATA_WIDTH(SAMP_W)) axis_sample_pkt_t;
  typedef mailbox#(axis_sample_pkt_t) axis_sample_mbox_t;
  typedef mailbox#(axis_pkt_t) axis_pkt_mbox_t;

  typedef FilterHBTestUtils #(
    .SAMP_W(SAMP_W),
    .SPC_IN(SPC),
    .SPC_OUT(SPC)
  ) filter_utils_t;

  typedef filter_utils_t::axis_input_pkt_mbox_t axis_input_pkt_mbox_t;

  // --------------------------------------------------------------------------
  // Clock and reset
  // --------------------------------------------------------------------------
  logic clk;
  logic rst;
  logic clear = 1'b0;

  sim_clock_gen #(
    .PERIOD(CLK_PERIOD_NS)
  ) clk_gen (
    .clk(clk),
    .rst(rst)
  );

  // --------------------------------------------------------------------------
  // AXI interfaces and BFM
  // --------------------------------------------------------------------------
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

  // --------------------------------------------------------------------------
  // DUT configuration
  // --------------------------------------------------------------------------
  logic [1:0] num_stages;

  // --------------------------------------------------------------------------
  // DUT
  // --------------------------------------------------------------------------
  axis_hb_cascade_intp #(
    .SAMP_W        (SAMP_W),
    .SPC           (SPC),
    .NUM_HB        (NUM_HB),
    .HB_NUM_COEFFS (HB_NUM_COEFFS),
    .PRELOAD_ZEROES(PRELOAD_ZEROES)
  ) dut (
    .clk          (clk),
    .rst          (rst),
    .clear        (clear),
    .s_axis_tdata (to_dut.tdata),
    .s_axis_tvalid(to_dut.tvalid),
    .s_axis_tready(to_dut.tready),
    .s_axis_tlast (to_dut.tlast),
    .m_axis_tdata (from_dut.tdata),
    .m_axis_tvalid(from_dut.tvalid),
    .m_axis_tready(from_dut.tready),
    .m_axis_tlast (from_dut.tlast),
    .num_stages   (num_stages)
  );

  // --------------------------------------------------------------------------
  // model smoke: instantiate and run cascade interpolation reference model.
  // Validates:
  //   - Reference model construction for stage counts 0..3.
  //   - Stage chaining path executes without mailbox underrun/overrun.
  //   - Output sample count follows 2^N growth relative to input sample count.
  // Note: This is model-only and does not exercise the DUT datapath.
  // --------------------------------------------------------------------------
  task automatic test_model_smoke();
    localparam int MODEL_SMOKE_SAMPLES = 100;
    axis_sample_mbox_t in_samples;
    axis_sample_mbox_t out_samples;
    axis_sample_pkt_t  sample_pkt;
    int out_count;
    AxiFirHBCascadeIntpModel #(
      .SAMP_W        (SAMP_W),
      .NUM_HB        (NUM_HB),
      .HB_NUM_COEFFS (HB_NUM_COEFFS)
    ) cascade_model;

    foreach (TB_STAGE_SWEEP[i]) begin
      cascade_model = new();

      in_samples = new();
      for (int n = 0; n < MODEL_SMOKE_SAMPLES; n++) begin
        sample_pkt = new();
        sample_pkt.data.push_back(n + 1);
        in_samples.put(sample_pkt);
      end

      cascade_model.process_samples(in_samples, out_samples, TB_STAGE_SWEEP[i]);

      out_count = 0;
      while (out_samples.try_get(sample_pkt)) begin
        out_count++;
      end

      `ASSERT_ERROR(out_count == (MODEL_SMOKE_SAMPLES << TB_STAGE_SWEEP[i]), $sformatf(
        "Model smoke: unexpected output sample count for stages=%0d. expected=%0d got=%0d",
        TB_STAGE_SWEEP[i],
        (MODEL_SMOKE_SAMPLES << TB_STAGE_SWEEP[i]),
        out_count
      ));
    end
  endtask : test_model_smoke

  // --------------------------------------------------------------------------
  // Bypass smoke test (num_stages = 0)
  // Validates:
  //   - Pure pass-through behavior in bypass mode.
  //   - Packet length preservation and packet boundary propagation (tlast).
  //   - Exact word ordering and sample-lane ordering.
  // --------------------------------------------------------------------------
  task automatic test_bypass_smoke();
    localparam int PKT_LEN_WORDS = 100;
    axis_pkt_t input_pkt;
    axis_pkt_t output_pkt;
    logic [SPC*SAMP_W-1:0] word;

    num_stages = 2'd0;

    input_pkt = new();
    for (int word_idx = 0; word_idx < PKT_LEN_WORDS; word_idx++) begin
      word = '0;
      for (int lane = 0; lane < SPC; lane++) begin
        word[lane*SAMP_W +: SAMP_W] = word_idx * SPC + lane;
      end
      input_pkt.data.push_back(word);
    end

    axi_bfm.put(input_pkt);
    axi_bfm.wait_complete();
    axi_bfm.get(output_pkt);

    `ASSERT_ERROR(output_pkt.data.size() == input_pkt.data.size(), $sformatf(
      "Bypass length mismatch: expected %0d words, got %0d words",
      input_pkt.data.size(), output_pkt.data.size()
    ));

    foreach (input_pkt.data[idx]) begin
      `ASSERT_ERROR(output_pkt.data[idx] == input_pkt.data[idx], $sformatf(
        "Bypass data mismatch at word %0d: expected 0x%0h, got 0x%0h",
        idx, input_pkt.data[idx], output_pkt.data[idx]
      ));
    end
  endtask : test_bypass_smoke

  // --------------------------------------------------------------------------
  // Model-only impulse area: impulse response sum follows interpolation growth.
  // Validates:
  //   - Impulse response area of the cascade reference model grows by 2^N for
  //     stages 0..NUM_HB, matching the output sample-rate increase.
  //   - Sum of all impulse response output samples equals input impulse amplitude
  //     times the interpolation factor within fixed-point rounding tolerance.
  // Note: This is model-only and does not exercise the DUT datapath. But
  //       since other tests do a sample-by-sample comparison between DUT and model,
  //       this test indirectly validates the DUT impulse-response scaling as well.
  // --------------------------------------------------------------------------
  task automatic test_model_impulse_area();
    // Enough scalar input samples for the full impulse response of a NUM_HB-stage
    // cascade to fit within the packet.
    localparam int    NUM_INPUT_SAMPLES = 512;
    // Max positive signed value for a SAMP_W/2-bit component (impulse amplitude).
    localparam longint IMPULSE_AMP = longint'({1'b0, {(SAMP_W/2-1){1'b1}}});
    // Allow 0.1% gain error due to rounding effects.
    localparam longint GAIN_TOL = IMPULSE_AMP / 1000;

    axis_sample_mbox_t in_samples;
    axis_sample_mbox_t out_samples;
    axis_sample_pkt_t  sample_pkt;
    longint            sum_i;
    longint            sum_q;
    longint            expected_sum;
    longint            gain_tol;
    longint            err_i;
    longint            err_q;

    // Sweep stages 0..NUM_HB (0 = bypass pass-through, NUM_HB = full cascade).
    for (int stages = 0; stages <= NUM_HB; stages++) begin
      AxiFirHBCascadeIntpModel #(
        .SAMP_W        (SAMP_W),
        .NUM_HB        (NUM_HB),
        .HB_NUM_COEFFS (HB_NUM_COEFFS)
      ) cascade_model = new();

      // Build scalar-sample input: one max-amplitude impulse followed by zeros.
      // I and Q components are both set to max positive amplitude.
      in_samples = new();
      for (int n = 0; n < NUM_INPUT_SAMPLES; n++) begin
        logic [SAMP_W-1:0] sample_val;
        sample_pkt = new();
        sample_val = (n == 0) ? {2{filter_utils_t::MAXIMUM_I_Q_VALUE}} : '0;
        sample_pkt.data.push_back(sample_val);
        in_samples.put(sample_pkt);
      end

      cascade_model.process_samples(in_samples, out_samples, stages);

      // Each sample packs I in the upper half and Q in the lower half. Accumulate
      // each signed component across the impulse response. For an interpolating
      // cascade, the impulse-response sum grows with the output sample-rate factor.
      sum_i = 0;
      sum_q = 0;
      while (out_samples.try_get(sample_pkt)) begin
        logic signed [SAMP_W/2-1:0] si, sq;
        si    = sample_pkt.data[0][SAMP_W-1 -: SAMP_W/2];
        sq    = sample_pkt.data[0][SAMP_W/2-1 -: SAMP_W/2];
        sum_i += longint'(si);
        sum_q += longint'(sq);
      end

      expected_sum = IMPULSE_AMP << stages;
      gain_tol     = expected_sum / 1000;

      // Error is the signed residual from the ideal impulse-area result. A positive
      // value means the model gained amplitude; a negative value means it lost it.
      err_i = sum_i - expected_sum;
      err_q = sum_q - expected_sum;
      if (VERBOSE) begin
        $display(
          {"Model impulse area: stages=%0d sum_i=%0d sum_q=%0d expected=%0d ",
           "err_i=%0d err_q=%0d tol=%0d"},
          stages, sum_i, sum_q, expected_sum, err_i, err_q, gain_tol
        );
      end
      `ASSERT_ERROR((err_i >= -gain_tol) && (err_i <= gain_tol), $sformatf(
        "Impulse area (I) stages=%0d: sum=%0d expected=%0d err=%0d tol=%0d",
        stages, sum_i, expected_sum, err_i, gain_tol
      ));
      `ASSERT_ERROR((err_q >= -gain_tol) && (err_q <= gain_tol), $sformatf(
        "Impulse area (Q) stages=%0d: sum=%0d expected=%0d err=%0d tol=%0d",
        stages, sum_q, expected_sum, err_q, gain_tol
      ));
    end
  endtask : test_model_impulse_area

  // --------------------------------------------------------------------------
  // Model-only DC unity gain: steady-state constant input retains amplitude.
  // Validates:
  //   - DC gain of the cascade reference model is unity for stages 0..NUM_HB.
  //   - Output sample amplitude is stable after startup transient despite the
  //     interpolation-rate impulse-area growth checked above.
  // Note: This is model-only and does not exercise the DUT datapath. But
  //       since other tests do a sample-by-sample comparison between DUT and model,
  //       this test indirectly validates the DUT steady-state DC gain as well.
  // --------------------------------------------------------------------------
  task automatic test_model_dc_unity_gain();
    localparam int                      NUM_INPUT_SAMPLES = 1024;
    localparam int                      NUM_CHECK_SAMPLES = 256;
    localparam logic [SAMP_W/2-1:0]     DC_COMPONENT      = {1'b0, {(SAMP_W/2-3){1'b1}}, 2'b00};
    localparam longint                  DC_AMP            = longint'(DC_COMPONENT);
    localparam longint                  GAIN_TOL          = DC_AMP / 1000;

    axis_sample_mbox_t in_samples;
    axis_sample_mbox_t out_samples;
    axis_sample_pkt_t  sample_pkt;
    int                out_count;
    int                check_start;
    int                sample_idx;
    longint            err_i;
    longint            err_q;

    for (int stages = 0; stages <= NUM_HB; stages++) begin
      AxiFirHBCascadeIntpModel #(
        .SAMP_W        (SAMP_W),
        .NUM_HB        (NUM_HB),
        .HB_NUM_COEFFS (HB_NUM_COEFFS)
      ) cascade_model = new();

      in_samples = new();
      for (int n = 0; n < NUM_INPUT_SAMPLES; n++) begin
        sample_pkt = new();
        sample_pkt.data.push_back({DC_COMPONENT, DC_COMPONENT});
        in_samples.put(sample_pkt);
      end

      cascade_model.process_samples(in_samples, out_samples, stages);

      out_count   = out_samples.num();
      check_start = out_count - NUM_CHECK_SAMPLES;
      sample_idx  = 0;

      while (out_samples.try_get(sample_pkt)) begin
        logic signed [SAMP_W/2-1:0] si, sq;
        if (sample_idx >= check_start) begin
          si    = sample_pkt.data[0][SAMP_W-1 -: SAMP_W/2];
          sq    = sample_pkt.data[0][SAMP_W/2-1 -: SAMP_W/2];
          err_i = longint'(si) - DC_AMP;
          err_q = longint'(sq) - DC_AMP;
          `ASSERT_ERROR((err_i >= -GAIN_TOL) && (err_i <= GAIN_TOL), $sformatf(
            "DC unity gain (I) stages=%0d sample=%0d value=%0d expected=%0d err=%0d tol=%0d",
            stages, sample_idx, si, DC_AMP, err_i, GAIN_TOL
          ));
          `ASSERT_ERROR((err_q >= -GAIN_TOL) && (err_q <= GAIN_TOL), $sformatf(
            "DC unity gain (Q) stages=%0d sample=%0d value=%0d expected=%0d err=%0d tol=%0d",
            stages, sample_idx, sq, DC_AMP, err_q, GAIN_TOL
          ));
        end
        sample_idx++;
      end
    end
  endtask : test_model_dc_unity_gain

  // --------------------------------------------------------------------------
  // Shared helper: deterministic packet DUT-vs-model stage sweep with configurable stalls.
  // This is used by impulse response, random data, and backpressure stress tests.
  // When check_throughput=1 and both stall probabilities are 0, a background monitor
  // verifies that from_dut.tvalid is asserted every cycle while from_dut.tready=1
  // (i.e. no idle output cycles, indicating full-rate throughput).
  // --------------------------------------------------------------------------
  task automatic run_stage(
    input string tc_name,
    input int stages_req        = 0,
    input int num_pkts          = 1,
    input int min_pkt_len_words = 128,
    input int max_pkt_len_words = 128,
    input int master_stall_prob = 0,
    input int slave_stall_prob  = 0,
    input bit impulse           = 0,
    input bit check_throughput  = 0
  );
    axis_pkt_t input_pkt;
    axis_pkt_t output_pkt;
    axis_input_pkt_mbox_t input_pkts_mbox = new();
    axis_sample_mbox_t input_samples_mbox = new();
    axis_sample_mbox_t expected_samples_mbox = new();
    axis_sample_pkt_t expected_sample_pkt;
    int pipeline_delay_samples;
    int expected_total_samples;
    int expected_comparable_samples;
    int received_total_samples;
    int skipped_delay_samples;
    int compared_samples;
    int pkt_idx;
    // Throughput monitor state variables (used when check_throughput=1)
    int tput_mon_gap_cycles;  // cycles where tvalid=0 within a packet output window
    bit tput_mon_done;        // set by monitor thread when done

    // Reference model construction
    AxiFirHBCascadeIntpModel #(
      .SAMP_W        (SAMP_W),
      .NUM_HB        (NUM_HB),
      .HB_NUM_COEFFS (HB_NUM_COEFFS)
    ) cascade_model;

    axi_bfm.set_master_stall_prob(master_stall_prob);
    axi_bfm.set_slave_stall_prob(slave_stall_prob);

    // Drive DUT stage select from this test invocation.
    num_stages = stages_req[1:0];

    // Generate and send multiple packets
    input_samples_mbox = new();
    for (pkt_idx = 0; pkt_idx < num_pkts; pkt_idx++) begin
      if (impulse) begin
        input_pkt = filter_utils_t::generate_impulse_packet(max_pkt_len_words);
      end else begin
        input_pkt = filter_utils_t::generate_random_packet(
          min_pkt_len_words,
          max_pkt_len_words
        );
      end
      input_pkts_mbox.put(input_pkt.copy());
      axi_bfm.put(input_pkt);
    end

    // Collect and serialize all input packets
    filter_utils_t::collect_and_serialize_packets(input_pkts_mbox, input_samples_mbox);

    cascade_model = new();
    cascade_model.process_samples(input_samples_mbox, expected_samples_mbox, stages_req);

    pipeline_delay_samples = get_pipeline_delay_samples(stages_req);

    expected_total_samples = expected_samples_mbox.num();
    expected_comparable_samples = expected_total_samples - pipeline_delay_samples;
    received_total_samples = 0;
    skipped_delay_samples = 0;
    compared_samples = 0;

    // Start throughput monitor before wait_complete so it sees output as it arrives.
    // Monitor DUT m_axis_* ports directly (not via BFM interface), without a tready
    // guard: a full-rate DUT must keep m_axis_tvalid=1 on every cycle within a packet
    // once output has started, regardless of downstream backpressure.
    tput_mon_gap_cycles = 0;
    tput_mon_done       = 1'b0;
    if (check_throughput && master_stall_prob == 0 && slave_stall_prob == 0) begin
      fork
        begin : throughput_monitor
          for (int pkt = 0; pkt < num_pkts; pkt++) begin
            // Wait for the first output word of this packet.
            @(posedge clk iff dut.m_axis_tvalid);
            // Count idle cycles within this packet until the valid tlast.
            while (!(dut.m_axis_tvalid && dut.m_axis_tlast)) begin
              @(posedge clk);
              if (!dut.m_axis_tvalid)
                tput_mon_gap_cycles++;
            end
          end
          tput_mon_done = 1'b1;
        end
      join_none
    end else begin
      tput_mon_done = 1'b1;
    end

    axi_bfm.wait_complete();

    // Receive and validate all output packets
    for (pkt_idx = 0; pkt_idx < num_pkts; pkt_idx++) begin
      axi_bfm.get(output_pkt);

      foreach (output_pkt.data[word_idx]) begin
        logic [SPC*SAMP_W-1:0] word;
        word = output_pkt.data[word_idx];
        for (int lane = 0; lane < SPC; lane++) begin
          logic [SAMP_W-1:0] received_sample;
          received_sample = word[lane*SAMP_W +: SAMP_W];
          received_total_samples++;
          if (pipeline_delay_samples > 0) begin
            `ASSERT_ERROR(received_sample == '0, $sformatf(
              "%s expected zero during pipeline delay pkt=%0d stages=%0d word=%0d lane=%0d: got=0x%0h",
              tc_name,
              pkt_idx,
              stages_req,
              word_idx,
              lane,
              received_sample
            ));
            pipeline_delay_samples--;
            skipped_delay_samples++;
            continue;
          end
          if (!expected_samples_mbox.try_get(expected_sample_pkt)) begin
            `ASSERT_ERROR(0, $sformatf(
              "%s expected-data underrun pkt=%0d word=%0d lane=%0d",
              tc_name,
              pkt_idx,
              word_idx,
              lane
            ));
          end else begin
            compared_samples++;
            `ASSERT_ERROR(received_sample == expected_sample_pkt.data[0], $sformatf(
              "%s data mismatch pkt=%0d stages=%0d word=%0d lane=%0d: exp=0x%0h got=0x%0h",
              tc_name,
              pkt_idx,
              stages_req,
              word_idx,
              lane,
              expected_sample_pkt.data[0],
              received_sample
            ));
          end
        end
      end
    end

    if (VERBOSE) begin
      $display(
        "  >> %s accounting: received=%0d skipped_delay=%0d compared=%0d expected=%0d",
        tc_name,
        received_total_samples,
        skipped_delay_samples,
        compared_samples,
        expected_total_samples
      );
    end

    `ASSERT_ERROR(compared_samples == expected_comparable_samples, $sformatf(
      "%s compare-count mismatch: compared=%0d expected_comparable=%0d (expected_total=%0d)",
      tc_name,
      compared_samples,
      expected_comparable_samples,
      expected_total_samples
    ));

    // Wait for the throughput monitor to finish and check the result.
    wait (tput_mon_done);
    if (check_throughput && master_stall_prob == 0 && slave_stall_prob == 0) begin
      `ASSERT_ERROR(tput_mon_gap_cycles == 0, $sformatf(
        "%s stages=%0d: throughput violation: %0d idle output cycle(s) detected (tvalid=0 while tready=1)",
        tc_name, stages_req, tput_mon_gap_cycles
      ));
    end

    axi_bfm.set_master_stall_prob(0);
    axi_bfm.set_slave_stall_prob(0);
  endtask : run_stage
  
  // Ensure DUT in clean state before each test run.
  task automatic reset_dut_state();
    axis_pkt_t flush_pkt;
    axis_pkt_t flush_out_pkt;
    if (PRELOAD_ZEROES == '0) begin
      // Filter instance without preload zeroes relies on explicit
      // flush packets of zeroes to clear internal state between tests.
      flush_pkt = FilterHBTestUtils#(
        .SAMP_W(SAMP_W),
        .SPC_IN(SPC)
      )::generate_zero_packet(512 / SPC);
      axi_bfm.put(flush_pkt);
      axi_bfm.wait_complete();
      // Consume corresponding flush output so it cannot pollute this run.
      axi_bfm.get(flush_out_pkt);
    end else begin
      // Use HB intp filter clear signal to reset internal state.
      clk_gen.clk_wait_f();
      clear = 1'b1;
      clk_gen.clk_wait_r();
      clk_gen.clk_wait_f();
      clear = 1'b0;
    end
  endtask : reset_dut_state

  // --------------------------------------------------------------------------
  // Baseline deterministic DUT-vs-model impulse sweep.
  // Validates:
  //   - Packet stimulus handling across multiple iterations.
  //   - Deterministic stage sweep (caller-provided stage_values).
  //   - Per-stage state preparation behavior:
  //       PRELOAD_ZEROES==0 -> explicit zero flush + drain.
  //       PRELOAD_ZEROES==1 -> clear pulse.
  //   - Total sample-count accounting with delay-offset hook.
  //   - Exact sample-by-sample DUT-vs-model equivalence.
  // --------------------------------------------------------------------------
  task automatic test_impulse_response(
    input int stage_values[$] = '{0, 1, 2, 3}
  );
    // TC constants
    localparam int NUM_TESTS = 4; // one test per stage count requested
    localparam int NUM_PKTS  = 1;
    localparam int PKT_LEN   = 512 / SPC; // some large number impulse response should fit into

    repeat (NUM_TESTS) begin
      foreach (stage_values[i]) begin
        // Ensure DUT in clean state for this stage run.
        reset_dut_state();
        // Run the test (creates test data, runs the DUT and compares the data)
        run_stage(
          .tc_name("Impulse Response"),
          .stages_req(stage_values[i]),
          .num_pkts(NUM_PKTS),
          .min_pkt_len_words(PKT_LEN),
          .max_pkt_len_words(PKT_LEN),
          .impulse(1'b1),
          .check_throughput(1'b1)
        );
      end
    end
  endtask : test_impulse_response

  // --------------------------------------------------------------------------
  // Random Data: Baseline deterministic DUT-vs-model random data sweep.
  // Validates:
  //   - Packet stimulus handling across multiple iterations.
  //   - Deterministic stage sweep (caller-provided stage_values).
  //   - Per-stage state preparation behavior:
  //       PRELOAD_ZEROES==0 -> explicit zero flush + drain.
  //       PRELOAD_ZEROES==1 -> clear pulse.
  //   - Total sample-count accounting with delay-offset hook.
  //   - Exact sample-by-sample DUT-vs-model equivalence.
  // --------------------------------------------------------------------------
  task automatic test_random_data(
    input int stage_values[$] = '{0, 1, 2, 3}
  );
    // TC constants
    localparam int NUM_TESTS = 4;
    localparam int NUM_PKTS  = 3;
    localparam int MIN_PKT_LEN = 17;
    localparam int MAX_PKT_LEN = 211;

    repeat (NUM_TESTS) begin
      foreach (stage_values[i]) begin
        // Ensure DUT in clean state for this stage run.
        reset_dut_state();
        // Run the test (creates test data, runs the DUT and compares the data)
        run_stage(
          .tc_name("Random data"),
          .stages_req(stage_values[i]),
          .num_pkts(NUM_PKTS),
          .min_pkt_len_words(MIN_PKT_LEN),
          .max_pkt_len_words(MAX_PKT_LEN),
          .impulse(1'b0),
          .check_throughput(1'b1)
        );
      end
    end
  endtask : test_random_data

  // Backpressure stress test (optional, can be enabled if time permits)
  // Validates:
  //   - DUT behavior under master/slave backpressure with random stalls.
  //   - Exact sample-by-sample DUT-vs-model equivalence under backpressure.
  task automatic test_backpressure_stress(
    input int stage_values[$] = '{0, 1, 2, 3}
  );
    // TC constants
    localparam int NUM_PKTS = 3;
    localparam int PKT_LEN  = 31;
    localparam int MASTER_STALL_PROB = 20; // 20% chance to stall master (input)
    localparam int SLAVE_STALL_PROB  = 20; // 20% chance to stall slave (output)

    foreach (stage_values[i]) begin
      // Ensure DUT in clean state for this stage run.
      reset_dut_state();
      // Run the test (creates test data, runs the DUT and compares the data)
      run_stage(
        .tc_name("Backpressure Stress"),
        .stages_req(stage_values[i]),
        .num_pkts(NUM_PKTS),
        .min_pkt_len_words(PKT_LEN),
        .max_pkt_len_words(PKT_LEN),
        .master_stall_prob(MASTER_STALL_PROB),
        .slave_stall_prob(SLAVE_STALL_PROB),
        .impulse(1'b0)
      );
    end
  endtask : test_backpressure_stress

  // --------------------------------------------------------------------------
  // Test procedure
  // --------------------------------------------------------------------------
  initial begin : main
    localparam string TEST_NAME = $sformatf(
      "axis_hb_cascade_intp_tb: SAMP_W:%0d, NUM_HB:%0d, SPC:%0d, PRELOAD_ZEROES:%0d",
      SAMP_W, NUM_HB, SPC, PRELOAD_ZEROES
    );
    test.start_tb(TEST_NAME, 100ms);

    clk_gen.start();
    axi_bfm.run();
    clk_gen.reset(10);
    @(negedge rst);

    test.start_test("Model-Only Smoke: model chaining and stage-count growth", 200us);
    test_model_smoke();
    test.end_test();

    test.start_test("Model-Only Impulse Area: impulse sum follows interpolation growth", 200us);
    test_model_impulse_area();
    test.end_test();

    test.start_test("Model-Only DC Unity Gain: steady-state amplitude is preserved", 200us);
    test_model_dc_unity_gain();
    test.end_test();

    test.start_test("Bypass Smoke: pass-through data/order/tlast", 200us);
    test_bypass_smoke();
    test.end_test();

    test.start_test("Impulse Response: impulse shape, ordering, and count", 10ms);
    test_impulse_response(TB_STAGE_SWEEP);
    test.end_test();

    test.start_test("Random Data: random packets, stage selection, reset, model compare",
            10ms);
    test_random_data(TB_STAGE_SWEEP);
    test.end_test();

    test.start_test("Backpressure Stress: output throttling with model compare",
      10ms);
    test_backpressure_stress(TB_STAGE_SWEEP);
    test.end_test();

    test.end_tb(0);
    clk_gen.kill();
  end : main

  // Calculate the expected pipeline delay in scalar samples for a given number of
  // stages in the cascade based on the per-stage filter coefficient counts and the
  // internal axi_fir_multisample filter pipeline delay formula.
  // This is used to determine how many output samples to skip in the test before
  // starting to compare against the model output, since the model does not include
  // the pipeline delay.
  function automatic int get_pipeline_delay_samples(input int stages);
    int hb_num_coeffs_reversed           [];
    int hb_pipeline_delay_words_reversed [];
    int hb_spc_out_reversed              [];
    int total_delay_scalar_samples   = 0;
    hb_num_coeffs_reversed           = new[stages];
    hb_pipeline_delay_words_reversed = new[stages];
    hb_spc_out_reversed              = new[stages];

    if (stages == 0) return 0;

    // Create array of number of hb filter coefficients in right-to-left stage order
    // (i.e. reversed relative to processing order).
    for (int i = 0; i < stages; i++) begin
      hb_num_coeffs_reversed[i] = HB_NUM_COEFFS[NUM_HB - 1 - i];
    end

    // Calculate multisample fir pipeline delay for each stage.
    // NOTE: the used formula is NUM_COEFFS/4 + 5 words, which is specific to the
    //       axi_fir_multisample_filter implementation with optimizations for
    //       symmetric halfband coefficients.
    for (int i = 0; i < stages; i++) begin
      hb_pipeline_delay_words_reversed[i] = (hb_num_coeffs_reversed[i] + 5) / 4 + 5;
    end

    // Calculate SPC at the output of each stage in the cascade (the SPC the internal
    // axi_fir_multisample_filter is processing at).
    // This is needed to convert the pipeline delay from words to scalar samples.
    // The SPC is halved at each stage (from right to left, index 0 to stages-1),
    // but cannot go below 1.
    hb_spc_out_reversed[0] = SPC;
    for (int i = 1; i < stages; i++) begin
      hb_spc_out_reversed[i] =
        (hb_spc_out_reversed[i-1] > 1) ? hb_spc_out_reversed[i-1] / 2 : 1; 
    end

    // Calculate total pipeline delay of the cascaded stages in scalar samples
    // by summing the per-stage pipeline delays considering upsampling of 2 in
    // each stage that is,
    // for 1 stage:  delay_samples = pd_words[0] * spc[0]
    // for 2 stages: delay_samples = pd_words[0] * spc[0] + 2 * pd_words[1] * spc[1]
    // for 3 stages: delay_samples = pd_words[0] * spc[0] + 2 * pd_words[1] * spc[1]
    //                                                    + 4 * pd_words[2] * spc[2]
    // etc.
    total_delay_scalar_samples = 0;
    for (int i = 0; i < stages; i++) begin
      total_delay_scalar_samples +=
        (1 << i) * hb_pipeline_delay_words_reversed[i] * hb_spc_out_reversed[i];
    end

    if (VERBOSE) begin
      $display("get_pipeline_delay_samples: stages=%0d total_delay_scalar_samples=%0d",
      stages, total_delay_scalar_samples);
      // Print the per-stage pipeline delay details in processing order
      // (left to right, stage 0 to stages-1).
      for (int i = stages - 1; i >= 0; i--) begin
        $display("  >> stage %0d: num_coeffs=%0d pd_words=%0d spc_out=%0d",
          stages - 1 - i, hb_num_coeffs_reversed[i],
          hb_pipeline_delay_words_reversed[i],
          hb_spc_out_reversed[i]);
      end
    end
    return total_delay_scalar_samples;
  endfunction : get_pipeline_delay_samples

endmodule : axis_hb_cascade_intp_tb

`default_nettype wire
