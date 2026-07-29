//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_filter_interp_tb
//
// Description:
//
//   Testbench for cic_filter_interp module. Drives random input data through
//   the DUT at various interpolation factors, compares every output sample
//   against the bit-true cic_filter_interp_model.
//
// Parameters:
//
//   SPC        : Number of samples per clock cycle on DUT.
//   COMP_W     : Component width used to generate input samples (ACCUM_W/2 max).
//   ORDER      : CIC filter order (number of comb and integrator stages).
//   MAX_INTERP : Maximum interpolation factor supported.
//

`default_nettype none

module cic_filter_interp_tb #(
  int SPC        = 4,
  int COMP_W     = 16,
  int ORDER      = 4,
  int MAX_INTERP = 255
);

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"
  import PkgTestExec::*;

  import PkgAxiStreamBfm::*;

  import cic_utils_pkg::*;
  import cic_test_pkg::*;
  import cic_filter_test_pkg::*;

  //---------------------------------------------------------------------------
  // Testbench parameters and type definitions
  //---------------------------------------------------------------------------
  localparam real CLK_PERIOD         = 10.0;
  localparam int  STALL_PROB_LOW     = 10;
  localparam int  STALL_PROB_DEFAULT = 50;
  localparam int  STALL_PROB_HIGH    = 80;
  localparam int  NUM_TESTS          = 20;
  localparam bit  VERBOSE            = 0;

  // DUT I/O sample width (I + Q packed)
  localparam int SAMP_W = COMP_W * 2;
  // Internal accumulator width (must match DUT formula, MAX_DELAY=1)
  localparam int ACCUM_W = 2 * (SAMP_W/2 + $clog2(MAX_INTERP) * ORDER);
  localparam int MODEL_COMP_W = ACCUM_W / 2;
  // AXI-Stream data width at DUT I/O
  localparam int DATA_W = SAMP_W * SPC;

  // Input packet length constraints for input words. MAX_WORDS is kept small
  // because the output is R times as large as the input, so even modest packet
  // sizes produce a large number of output samples at high interpolation
  // factors.
  localparam int MIN_WORDS = ORDER;
  localparam int MAX_WORDS = 6*ORDER;

  // Test input data mode
  typedef enum {
    TEST_MODE_RANDOM,
    TEST_MODE_RAMP,
    TEST_MODE_CONSTANT,
    TEST_MODE_IMPULSE
  } test_mode_t;

  // I/O-level types (SAMP_W-wide samples for DUT interface)
  typedef cic_utils #(
    .SPC   (SPC),
    .SAMP_W(SAMP_W)
  ) util_c;

  typedef cic_test_utils #(
    .SPC    (SPC),
    .ACCUM_W(SAMP_W),
    .COMP_W (COMP_W)
  ) test_utils_c;

  // Model-level types (ACCUM_W-wide samples for internal computation)
  typedef cic_utils #(
    .SPC   (1),
    .SAMP_W(ACCUM_W)
  ) model_util_c;

  typedef util_c::sample_t       sample_t;
  typedef util_c::comp_t         comp_t;
  typedef util_c::word_t         word_t;
  typedef sample_t               sample_queue_t[$];
  typedef model_util_c::sample_t model_sample_t;
  typedef model_sample_t         model_sample_queue_t[$];

  // Max/min representable sample component values (signed COMP_W-bit)
  localparam comp_t MAX_SAMPLE_VAL = {1'b0, {(COMP_W-1){1'b1}}};
  localparam comp_t MIN_SAMPLE_VAL = {1'b1, {(COMP_W-1){1'b0}}};

  typedef AxiStreamPacket #(DATA_W) axis_pkt_t;

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
  // AXI-Stream interfaces
  //---------------------------------------------------------------------------
  AxiStreamIf #(DATA_W) i_data (.clk(clk), .rst(rst));
  AxiStreamIf #(DATA_W) o_data (.clk(clk), .rst(rst));

  //---------------------------------------------------------------------------
  // AXI-Stream BFM
  //---------------------------------------------------------------------------
  AxiStreamBfm #(DATA_W) bfm = new(.master(i_data), .slave(o_data));

  //---------------------------------------------------------------------------
  // DUT configuration signals
  //---------------------------------------------------------------------------
  localparam int INTERP_W = $clog2(MAX_INTERP + 1);

  logic [INTERP_W-1:0] interp_factor;
  logic                config_changed;
  logic                clr;

  //---------------------------------------------------------------------------
  // Device Under Test (DUT)
  //---------------------------------------------------------------------------
  cic_filter_interp #(
    .SPC       (SPC),
    .SAMP_W    (SAMP_W),
    .MAX_INTERP(MAX_INTERP),
    .ORDER     (ORDER)
  ) dut (
    .clk           (clk),
    .rst           (rst),
    .clear         (clr),
    .interp_factor (interp_factor),
    .config_changed(config_changed),
    .data_in       (i_data),
    .data_out      (o_data)
  );

  //---------------------------------------------------------------------------
  // Reference Model
  //---------------------------------------------------------------------------
  cic_filter_interp_model #(
    .ACCUM_W(ACCUM_W),
    .COMP_W (MODEL_COMP_W),
    .ORDER  (ORDER)
  ) model = new();

  //---------------------------------------------------------------------------
  // Utility: set interpolation factor on DUT
  //---------------------------------------------------------------------------
  task automatic set_interp_factor(input int R);
    interp_factor  <= R;
    config_changed <= 1;
    clk_gen.clk_wait_r(1);
    config_changed <= 0;
    clk_gen.clk_wait_r(1);
  endtask : set_interp_factor

  //---------------------------------------------------------------------------
  // sign_extend_sample: widen a SAMP_W sample to ACCUM_W for model input
  //---------------------------------------------------------------------------
  function automatic model_sample_t sign_extend_sample(sample_t samp_in);
    logic signed [COMP_W-1:0]       q_in = samp_in[COMP_W-1:0];
    logic signed [COMP_W-1:0]       i_in = samp_in[SAMP_W-1:COMP_W];
    logic signed [MODEL_COMP_W-1:0] q_ext = MODEL_COMP_W'(q_in);
    logic signed [MODEL_COMP_W-1:0] i_ext = MODEL_COMP_W'(i_in);
    return {i_ext, q_ext};
  endfunction : sign_extend_sample

  //---------------------------------------------------------------------------
  // shift_sample: normalize an ACCUM_W model output sample to SAMP_W
  //
  // Matches the DUT's barrel shifter behavior. Arithmetic-right-shifts each
  // MODEL_COMP_W-bit component by ceil(log2(R^(ORDER-1))) bits, then extracts
  // the lower COMP_W bits as the output.
  //---------------------------------------------------------------------------
  function automatic sample_t shift_sample(model_sample_t samp_in, int R);
    logic signed [MODEL_COMP_W-1:0] q_in  = samp_in[MODEL_COMP_W-1:0];
    logic signed [MODEL_COMP_W-1:0] i_in  = samp_in[ACCUM_W-1:MODEL_COMP_W];
    int shift_amount = $clog2(longint'(R) ** (ORDER-1));
    logic signed [MODEL_COMP_W-1:0] q_shifted, i_shifted;
    logic signed [COMP_W-1:0]       q_out, i_out;

    q_shifted = q_in >>> shift_amount;
    i_shifted = i_in >>> shift_amount;
    q_out = q_shifted[COMP_W-1:0];
    i_out = i_shifted[COMP_W-1:0];

    return {i_out, q_out};
  endfunction : shift_sample

  //---------------------------------------------------------------------------
  // Test task: generate input, run model, compare with DUT output
  //
  // Sends a single packet of data with a given interpolation factor R.
  // The output contains R times as many samples as the input.
  //
  // mode selects the input data pattern:
  //
  //   TEST_MODE_RANDOM   : fully random samples (default)
  //   TEST_MODE_RAMP     : deterministic ramp (I=Q=1,2,3,...)
  //   TEST_MODE_CONSTANT : every sample has I=Q=const_value
  //   TEST_MODE_IMPULSE  : first sample I=Q=1, all others zero
  //---------------------------------------------------------------------------
  task automatic test_cic_interp(
    input int         R,
    input test_mode_t mode        = TEST_MODE_RANDOM,
    input comp_t      const_value = 1
  );
    int num_words;
    int samples_received;
    axis_pkt_t input_pkt;
    sample_queue_t input_samples, expected_samples;
    model_sample_queue_t model_in, model_out;

    // Configure DUT interpolation factor
    set_interp_factor(R);

    // Any input length is valid for interpolation.
    num_words = $urandom_range(MIN_WORDS, MAX_WORDS);

    case (mode)
      TEST_MODE_RAMP: input_pkt =
        test_utils_c::generate_ramp_packet(num_words);
      TEST_MODE_CONSTANT: input_pkt =
        test_utils_c::generate_constant_packet(num_words, const_value, const_value);
      TEST_MODE_IMPULSE: input_pkt =
        test_utils_c::generate_impulse_packet(num_words);
      default: input_pkt =
        test_utils_c::generate_random_packet(num_words);
    endcase

    // Convert to sample queue at I/O width
    input_samples = test_utils_c::pkt_to_samples(input_pkt.copy());

    // Sign-extend input to ACCUM_W for model
    model_in = {};
    foreach (input_samples[i])
      model_in.push_back(sign_extend_sample(input_samples[i]));

    // Run model at full precision, then shift output to SAMP_W.
    // model_out contains R times as many samples as model_in.
    model.process(model_in, R, model_out);
    expected_samples = {};
    foreach (model_out[i])
      expected_samples.push_back(shift_sample(model_out[i], R));

    if (VERBOSE) begin
      $display(
        "%s: R=%0d, num_words=%0d, input_samples=%0d, expected_output=%0d",
        mode.name(), R, num_words,
        input_samples.size(), expected_samples.size());
    end

    // Send packet to DUT
    bfm.put(input_pkt);

    // Receive output and compare sample-by-sample
    samples_received = 0;
    while (samples_received < expected_samples.size()) begin
      axis_pkt_t rx_pkt;
      bfm.get(rx_pkt);
      foreach (rx_pkt.data[word_idx]) begin
        word_t word = rx_pkt.data[word_idx];
        for (int samp_idx = 0; samp_idx < SPC; samp_idx++) begin
          if (samples_received < expected_samples.size()) begin
            sample_t expected_samp = expected_samples[samples_received];
            if (VERBOSE) begin
              $display("  sample[%0d]: expected=0x%0h, actual=0x%0h",
                       samples_received, expected_samp, word[samp_idx]);
            end
            `ASSERT_ERROR(word[samp_idx] === expected_samp,
              $sformatf("R=%0d, sample %0d: expected 0x%0h, got 0x%0h",
                        R, samples_received, expected_samp, word[samp_idx]))
            samples_received++;
          end
        end
      end
    end

    // Reset DUT between iterations to clear comb/integrator state
    clk_gen.reset();
    @(negedge rst);
  endtask : test_cic_interp

  //---------------------------------------------------------------------------
  // Main Test Process
  //---------------------------------------------------------------------------
  initial begin
    string tb_name;
    tb_name = $sformatf({
      "CIC Multisample Interp Filter ",
      "(SPC=%0d, SAMP_W=%0d, ACCUM_W=%0d, ORDER=%0d, MAX_INTERP=%0d)"},
      SPC, SAMP_W, ACCUM_W, ORDER, MAX_INTERP
    );
    test.start_tb(tb_name, 20ms);

    // Initialize
    clr            <= 0;
    interp_factor  <= 1;
    config_changed <= 0;

    clk_gen.start();
    bfm.run();
    bfm.set_master_stall_prob(STALL_PROB_DEFAULT);
    bfm.set_slave_stall_prob(STALL_PROB_DEFAULT);

    // Reset
    clk_gen.reset();
    @(negedge rst);

    //-------------------------------------------------------------------------
    // Test: R=1 (no interpolation, pass-through of comb+integrator chain)
    //-------------------------------------------------------------------------
    test.start_test("CIC interp R=1 (pass-through)", NUM_TESTS * 10us);
    repeat (NUM_TESTS) begin
      test_cic_interp(1);
    end
    test.end_test();

    //-------------------------------------------------------------------------
    // Test: ramp input (I=Q=1,2,3,...) with random interpolation factor
    //-------------------------------------------------------------------------
    test.start_test("CIC interp ramp input, random R", NUM_TESTS * 50us);
    repeat (NUM_TESTS) begin
      test_cic_interp($urandom_range(1, MAX_INTERP), .mode(TEST_MODE_RAMP));
    end
    test.end_test();

    //-------------------------------------------------------------------------
    // Test: constant input (I=Q=1) with random interpolation factor
    //-------------------------------------------------------------------------
    test.start_test("CIC interp constant input, random R", NUM_TESTS * 50us);
    repeat (NUM_TESTS) begin
      test_cic_interp($urandom_range(1, MAX_INTERP), .mode(TEST_MODE_CONSTANT), .const_value(1));
    end
    test.end_test();

    //-------------------------------------------------------------------------
    // Test: impulse input (first sample=1, rest=0) with random interpolation
    //-------------------------------------------------------------------------
    test.start_test("CIC interp impulse input, random R", NUM_TESTS * 50us);
    repeat (NUM_TESTS) begin
      test_cic_interp($urandom_range(1, MAX_INTERP), .mode(TEST_MODE_IMPULSE));
    end
    test.end_test();

    //-------------------------------------------------------------------------
    // Test: max positive input (I=Q=max_pos) to verify accumulator width
    //-------------------------------------------------------------------------
    test.start_test("CIC interp max positive input, random R", NUM_TESTS * 50us);
    repeat (NUM_TESTS) begin
      test_cic_interp($urandom_range(1, MAX_INTERP), .mode(TEST_MODE_CONSTANT),
                      .const_value(MAX_SAMPLE_VAL));
    end
    test.end_test();

    //-------------------------------------------------------------------------
    // Test: max negative input (I=Q=min_neg) to verify accumulator width
    //-------------------------------------------------------------------------
    test.start_test("CIC interp max negative input, random R", NUM_TESTS * 50us);
    repeat (NUM_TESTS) begin
      test_cic_interp($urandom_range(1, MAX_INTERP), .mode(TEST_MODE_CONSTANT),
                      .const_value(MIN_SAMPLE_VAL));
    end
    test.end_test();

    //-------------------------------------------------------------------------
    // Test: small interpolation factors (R=2..SPC)
    //-------------------------------------------------------------------------
    test.start_test("CIC interp small R (2..SPC)", NUM_TESTS * 50us);
    repeat (NUM_TESTS) begin
      test_cic_interp($urandom_range(2, SPC));
    end
    test.end_test();

    //-------------------------------------------------------------------------
    // Test: random interpolation factor
    //-------------------------------------------------------------------------
    test.start_test("CIC interp random R", NUM_TESTS * 50us);
    repeat (NUM_TESTS) begin
      test_cic_interp($urandom_range(1, MAX_INTERP));
    end
    test.end_test();

    //-------------------------------------------------------------------------
    // Test: maximum interpolation factor
    //-------------------------------------------------------------------------
    test.start_test("CIC interp R=MAX_INTERP", NUM_TESTS * 100us);
    repeat (NUM_TESTS) begin
      test_cic_interp(MAX_INTERP);
    end
    test.end_test();

    //-------------------------------------------------------------------------
    // Test: high input stall
    //-------------------------------------------------------------------------
    bfm.set_master_stall_prob(STALL_PROB_HIGH);
    bfm.set_slave_stall_prob(STALL_PROB_LOW);
    test.start_test("CIC interp random R, high master stall", NUM_TESTS * MAX_INTERP * 125us);
    repeat (NUM_TESTS) begin
      test_cic_interp($urandom_range(1, MAX_INTERP));
    end
    test.end_test();

    //-------------------------------------------------------------------------
    // Test: high output stall
    //-------------------------------------------------------------------------
    bfm.set_master_stall_prob(STALL_PROB_LOW);
    bfm.set_slave_stall_prob(STALL_PROB_HIGH);
    test.start_test("CIC interp random R, high slave stall", NUM_TESTS * MAX_INTERP * 125us);
    repeat (NUM_TESTS) begin
      test_cic_interp($urandom_range(1, MAX_INTERP));
    end
    test.end_test();

    test.end_tb(0);
    clk_gen.kill();
  end

endmodule : cic_filter_interp_tb

`default_nettype wire
