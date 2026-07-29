//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: dds_ms_tb
//
// Description: This is the testbench for dds_ms module.
//

module dds_ms_tb #(
    int SPC          = 8
);
  `include "test_exec.svh"

  import PkgTestExec::*;

  import PkgAxiStreamBfm::*;
  import ctrlport_pkg::*;
  import ctrlport_bfm_pkg::*;

  import PkgComplex::*;
  import PkgMath::*;
  import PkgRandom::*;

  localparam real CLK_PERIOD        = 4.0;
  localparam int  PHASE_WIDTH       = 24; // Fixed width of phase input to DDS LUT (IP specific)
  // Error tolerance for sample comparison - received sample = expected sample +/- tolerance.
  // Allow tolerance of upto 3 LSBs
  localparam sc16_t  ERR_TOLERANCE  = '{re: s16_t'(1<<2), im: s16_t'(1<<2)};
  // Minimum latency between two consecutive phase commands - derived from axi_tag_time_ms
  localparam int  CMD_LATENCY       = 4;

  //---------------------------------------------------------------------------
  // Testbench Configuration
  //---------------------------------------------------------------------------

  // DDS input phase limits (32-bit unsigned range on ctrlport)
  localparam logic unsigned [PHASE_WIDTH-1:0] MAX_PHASE = '1;

  // Number of words of SPC samples per packet
  localparam int PKT_LENGTH = 800;

  // Bit widths for our sample size
  localparam int SAMP_W = 32;           // Width of a complex sample
  localparam int COMP_W = SAMP_W/2;     // Width of just the imag/real part
  localparam int FRAC_W = COMP_W - 1;   // Number of fixed point fractional bits

  // Max possible value for the components of a sample
  localparam logic unsigned [FRAC_W-1:0] MAX_COMP = '1;

  // AXI-Stream data bus parameters
  localparam int USER_W = SPC+1; // used to indicate end of burst

  // Debug output display
  typedef enum {
    INFO,
    DEBUG,
    TRACE
  } log_level_t;

  localparam log_level_t LOG_LEVEL = INFO;

  //---------------------------------------------------------------------------
  // Clocks and Resets
  //---------------------------------------------------------------------------

  bit clk, rst;
  sim_clock_gen #(.PERIOD(CLK_PERIOD), .AUTOSTART(0)) clk_gen (clk, rst);

  //---------------------------------------------------------------------------
  // AXI-Stream BFM
  //---------------------------------------------------------------------------

  // AXI-Stream interfaces to/from DUT
  AxiStreamIf #(.DATA_WIDTH(SPC*SAMP_W), .USER_WIDTH(USER_W), .TKEEP(0))
    samples_to_dut (clk, rst);
  AxiStreamIf #(.DATA_WIDTH(SPC*SAMP_W), .USER_WIDTH(USER_W), .TKEEP(0))
    samples_from_dut (clk, rst);
  AxiStreamIf #(.DATA_WIDTH(PHASE_WIDTH), .USER_WIDTH(1), .TKEEP(0))
    phase_to_dut (clk, rst);

  // BFM for the AXI-Stream interface to DUT
  AxiStreamBfm #(.DATA_WIDTH(SPC*SAMP_W), .USER_WIDTH(USER_W), .TKEEP(0))
    axis_bfm = new(samples_to_dut, samples_from_dut);

  typedef AxiStreamPacket #(.DATA_WIDTH(SPC*SAMP_W), .USER_WIDTH(USER_W)) axis_pkt_t;
  typedef axis_pkt_t axis_pkt_queue_t[$];
  typedef axis_pkt_queue_t axis_burst_queue_t[$];

  //---------------------------------------------------------------------------
  // Control Port BFM
  //---------------------------------------------------------------------------
  ctrlport_if dds_ctrlport_if (
    .clk(clk),
    .rst(rst)
  );

  ctrlport_bfm dds_ctrlport_bfm = new(dds_ctrlport_if);

  //---------------------------------------------------------------------------
  // Test typedefs
  //---------------------------------------------------------------------------
  typedef logic signed   [COMP_W-1:0] comp_t;
  typedef logic [SPC-1:0][SAMP_W-1:0] iq_word_t;

  // Tone generation configuration
  // freq_norm: normalized frequency of the tone to generate, in range [-0.5, 0.5)
  // phase_offset: phase offset between each sample in the cycle
  // ampl: amplitude of the generated tone (in the range (0,1])
   typedef struct {
    real freq_norm;
    real phase_offset;
    real ampl;
  } tone_config_t;

  // Generation mode
  // CONST: generates constant samples (freq_norm = 0)
  // TONE: generates sinusoidal samples based on the provided frequency and
  //       phase offset configuration
  // RAMP: generates samples that linearly ramp up in amplitude from 0 to max
  // RAND: generates random complex samples
  //       (freq_norm and phase_offset need not be input in this mode)
  typedef enum {
    CONST,
    TONE,
    RAMP,
    RAND
  } gen_mode_t;

  // Stall probability configuration
  typedef struct packed {
    int master_stall_prob;
    int slave_stall_prob;
  } stall_config_t;

  // time tagging
  typedef struct {
    int              packet;
    int              word;
    logic [SPC-1:0]  tag;
  } time_tag_t;
  localparam time_tag_t NO_TAG = '{packet: 0, word: 0, tag: '0};

  //---------------------------------------------------------------------------
  // DUT
  //---------------------------------------------------------------------------
  logic [PHASE_WIDTH-1:0] phase_in_tdata;

  dds_ms #(
    .SPC                (SPC),
    .SAMP_W             (SAMP_W),
    .SAMP_FRAC_W        ((SAMP_W/2 - 1)),
    .PHASE_WIDTH        (PHASE_WIDTH)
  ) dut (
    .clk                      (clk),
    .rst                      (rst),
    .s_axis_din_tdata         (samples_to_dut.tdata),
    .s_axis_din_tlast         (samples_to_dut.tlast),
    .s_axis_din_tvalid        (samples_to_dut.tvalid),
    .s_axis_din_tready        (samples_to_dut.tready),
    .s_axis_din_tuser         (samples_to_dut.tuser),
    .m_axis_dout_tdata        (samples_from_dut.tdata),
    .m_axis_dout_tlast        (samples_from_dut.tlast),
    .m_axis_dout_tvalid       (samples_from_dut.tvalid),
    .m_axis_dout_tready       (samples_from_dut.tready),
    .m_axis_dout_tuser        (samples_from_dut.tuser[0]),
    .s_axis_phase_tdata       (phase_to_dut.tdata),
    .s_axis_phase_tready      (phase_to_dut.tready),
    .s_axis_phase_tvalid      (phase_to_dut.tvalid),
    .s_axis_phase_tuser       (phase_to_dut.tuser)
  );

  //---------------------------------------------------------------------------
  // Helper Functions
  //---------------------------------------------------------------------------

  function automatic sc16_t word_to_sc16(iq_word_t word, int spc_idx);
    sc16_t sample;
    sample.re = word[spc_idx][SAMP_W-1 : COMP_W];
    sample.im = word[spc_idx][COMP_W-1 : 0];
    return sample;
  endfunction : word_to_sc16

  // Convert sc16_t sample to a printable string "{re,im}"
  function automatic string sc16_to_str(sc16_t s);
    return $sformatf("{%1.15f,%1.15f}", real'(s.re)/(1<<FRAC_W), real'(s.im)/(1<<FRAC_W));
  endfunction : sc16_to_str

  // Convert a tone frequency to a normalized frequency in the range [-0.5, 0.5)
  // freq_norm = tone_freq / (2^data_width) - 0.5
  function automatic real gen_freq_norm(int tone_freq, int data_width);
    return real'(tone_freq) / real'(longint'(1) << data_width) - 0.5;
  endfunction : gen_freq_norm

  // Compute phase offset from a given phase word for the DDS LUT
  function automatic real compute_phase_offset(logic [PHASE_WIDTH-1:0] phase_word);
    logic [PHASE_WIDTH-1:0] phase_word_offset;

    phase_word_offset                 = phase_word;
    phase_word_offset[PHASE_WIDTH-1] = ~phase_word_offset[PHASE_WIDTH-1];
    return gen_freq_norm(int'(phase_word_offset), PHASE_WIDTH);
  endfunction : compute_phase_offset

  // -----------------------------------------------------------------------------
  // Generate a packet of sinusoidal samples corresponding to a
  // given initial phase, frequency, and amplitude.
  // parameters:
  // - length        number of words of SPC samples in the packet
  // - initial_phase starting phase for the first sample (normalized to [0,1))
  // - freq_norm     frequency of the tone to generate, normalized to
  //                 the sampling rate (i.e., in range [-0.5, 0.5))
  // - phase_offset  phase offset between each sample in the cycle
  // - ampl          amplitude of the generated tone
  //                 (in the range (0,1])
  // - eob           indicates the last packet of the burst by setting user for
  //                 all the samples in the packet to 1
  // returns:
  // - axis_pkt_t containing the generated samples
  // -------------------------------------------------------------------------------
  function automatic axis_pkt_t generate_tone_pkt(
    input int  length,
    inout real initial_phase,
    input tone_config_t tone_cfg  = '{freq_norm: 0.0,
                                    phase_offset: 0.0, ampl: 1.0},
    input bit         eob         = 0,
    input time_tag_t  time_tag    = NO_TAG,
    input log_level_t log_level   = INFO
  );
    static real phase; // contains phase accumulated value
    axis_pkt_t  samples;
    logic [SPC:0] user;

    phase   = initial_phase;

    samples = new();

    for(int word_idx = 0; word_idx < length; word_idx++) begin
      iq_word_t   iq_word;
      for (int sample_idx = 0; sample_idx < SPC; sample_idx++) begin
        comp_t    i, q;
        if (LOG_LEVEL == TRACE) begin
          $display("Word %0d, Sample %0d: phase = %f", word_idx, sample_idx, phase);
          $display("Phase Offset: %f", sample_idx * tone_cfg.phase_offset);
        end
        // SIN & COS values range [-1, 1)
        // Handling where SIN or COS value is exactly 1 since i & q are signed (sc16)
        if ($sin(2*PI*phase) >= (1.0 - 1e-4)) begin
          q = tone_cfg.ampl * MAX_COMP;
        end else begin
          q = (tone_cfg.ampl * $sin(2*PI*phase)) * (2**(FRAC_W));
        end
        if ($cos(2*PI*phase) >= (1.0 - 1e-4)) begin
          i = tone_cfg.ampl* MAX_COMP;
        end else begin
          i = (tone_cfg.ampl * $cos(2*PI*phase)) * (2**(FRAC_W));
        end

        // Arrange i and q in the format {i, q} for each sample in packet
        // to correspond to current DUT handling of the data
        iq_word[sample_idx][SAMP_W-1 : COMP_W] = i;
        iq_word[sample_idx][COMP_W-1 : 0]      = q;
        if (LOG_LEVEL == TRACE) begin
          $display("IQ sample {i,q} (%0d'd):", COMP_W);
          $display(sc16_to_str(word_to_sc16(iq_word, sample_idx)));
        end
        // offset phase for each sample in cycle
        phase += tone_cfg.phase_offset;
      end

      // Update phase for next word
      phase             = initial_phase + (word_idx+1)*tone_cfg.freq_norm;

      samples.data.push_back(iq_word);
      // Set user to indicate end of burst for the last packet
      user[0] = eob;
      user[SPC:1] = (time_tag.word == word_idx) ? time_tag.tag : '0;
      samples.user.push_back(user);
    end
    // Update initial phase with accumulated phase value at end of burst
    // for continuity across bursts
    initial_phase = phase;
    if (LOG_LEVEL == TRACE) $display("Generated %0d words", length);
    return samples;
  endfunction : generate_tone_pkt

  // -----------------------------------------------------------------------------
  // Generate a packet of random complex samples
  // parameters:
  // - length   number of words of SPC samples in the packet
  // - ampl     amplitude scaling factor for the generated random samples
  //            (in the range (0,1])
  // - eob      indicates the last packet of the burst by setting user for
  //            all the samples in the packet to 1
  // returns:
  // - axis_pkt_t containing the generated samples
  // -----------------------------------------------------------------------------
  function automatic axis_pkt_t generate_rand_sample_pkt(
    int         length,
    real        ampl     = 1.0,
    bit         eob      = 0,
    time_tag_t  time_tag = NO_TAG
  );
    axis_pkt_t samples;
    logic [SPC:0] user;

    samples = new();

    for(int word_idx = 0; word_idx < length; word_idx++) begin
      iq_word_t  iq_word;
      for (int sample_idx = 0; sample_idx < SPC; sample_idx++) begin
        iq_word[sample_idx] = Rand#(.WIDTH(SAMP_W))::rand_bit();
        if (LOG_LEVEL == TRACE) begin
          $display("IQ sample {i,q} (%0d'd):", COMP_W);
          $display(word_to_sc16(iq_word, sample_idx).re, word_to_sc16(iq_word, sample_idx).im);
        end
      end
      samples.data.push_back(iq_word);

      user[0] = eob;
      user[SPC:1] = (time_tag.word == word_idx) ? time_tag.tag : '0;
      samples.user.push_back(user);
    end
    if (LOG_LEVEL == TRACE) $display("Generated %0d random words", length);
    return samples;

  endfunction : generate_rand_sample_pkt

  // -----------------------------------------------------------------------------
  // Generate a packet of samples that linearly ramp up in amplitude from 0 to max
  // parameters:
  // - length   number of words of SPC samples in the packet
  // - ampl     amplitude scaling factor for the generated ramp samples
  //            (in the range (0,1])
  // - eob      indicates the last packet of the burst by setting user for
  //            all the samples in the packet to 1
  // returns:
  // - axis_pkt_t containing the generated samples
  // -----------------------------------------------------------------------------
  function automatic axis_pkt_t generate_ramp_sample_pkt(
    int         length,
    real        ampl     = 1.0,
    bit         eob      = 0,
    time_tag_t  time_tag = NO_TAG
  );
    axis_pkt_t samples;
    int        total_items;
    real       step;
    logic [SPC:0] user;

    samples     = new();
    total_items = length * SPC;
    // Step size to ramp I and Q linearly from 0 to total_items
    step = 1.0;

    for (int word_idx = 0; word_idx < length; word_idx++) begin
      iq_word_t iq_word;
      for (int sample_idx = 0; sample_idx < SPC; sample_idx++) begin
        int    global_idx;
        comp_t i, q;
        global_idx = word_idx * SPC + sample_idx;
        i = comp_t'(int'(step * global_idx));
        q = comp_t'(int'(step * global_idx));
        iq_word[sample_idx][SAMP_W-1: COMP_W] = i;
        iq_word[sample_idx][COMP_W-1: 0]      = q;
      end
      samples.data.push_back(iq_word);
      user[0] = eob;
      user[SPC:1] = (time_tag.word == word_idx) ? time_tag.tag : '0;
      samples.user.push_back(user);
    end
    if (LOG_LEVEL == TRACE) $display("Generated %0d ramp samples", length);
    return samples;
  endfunction : generate_ramp_sample_pkt

  // -----------------------------------------------------------------------------
  // Generate multiple packets of complex samples to create bursts of data
  // Random flag allows to pick between generating sinusoidal samples or
  // random samples for the bursts
  // parameters:
  // - num_pkts       number of packets in the burst
  // - pkt_length     number of words of SPC samples in each packet
  // - freq_norm      frequency of the tone to generate, normalized to
  //                  the sampling rate (i.e., in range [-0.5, 0.5))
  // - phase_offset   phase offset between each sample in cycle
  // - ampl           amplitude of the generated tone (in the range (0,1])
  // - random         flag to indicate whether to generate sinusoidal samples
  //                  or random samples
  // returns:
  // - axis_pkt_queue_t containing the generated packets for the burst
  // -----------------------------------------------------------------------------
  function automatic axis_pkt_queue_t generate_burst(
    int           num_pkts,
    int           pkt_length,
    tone_config_t tone_cfg = '{freq_norm: 0.0,
                              phase_offset: 0.0, ampl: 1.0},
    time_tag_t    time_tag = '{tag: '0, packet: 0, word: 0},
    gen_mode_t    gen_mode = TONE
  );
    axis_pkt_queue_t bursts;
    real initial_phase = 0.0;

    if (LOG_LEVEL == DEBUG)
      $display("Generating tag for packet %0d, word %0d: tag = %b",
            time_tag.packet, time_tag.word, time_tag.tag);

    for (int burst_num = 0; burst_num < num_pkts; burst_num++) begin
      axis_pkt_t gen_pkt;
      // For the first burst, start with an initial phase of 0.0.
      // For subsequent bursts, use the accumulated phase value
      // from the end of the previous burst to ensure continuity.
      if (gen_mode == RAND) begin
        gen_pkt = generate_rand_sample_pkt(pkt_length, tone_cfg.ampl, (burst_num == num_pkts-1),
                                           (burst_num == time_tag.packet) ? time_tag : NO_TAG);
        bursts.push_back(gen_pkt);
      end else if (gen_mode == RAMP) begin
        gen_pkt = generate_ramp_sample_pkt(pkt_length, tone_cfg.ampl, (burst_num == num_pkts-1),
                                           (burst_num == time_tag.packet) ? time_tag : NO_TAG);
        bursts.push_back(gen_pkt);
      end else begin
        gen_pkt = generate_tone_pkt(pkt_length, initial_phase, tone_cfg,
                                   (burst_num == num_pkts-1),
                                   (burst_num == time_tag.packet) ? time_tag : NO_TAG,
                                   INFO);
        bursts.push_back(gen_pkt);
      end
      if (LOG_LEVEL == DEBUG)
        $display("Tagged packet %0d with time tag: %p", burst_num, gen_pkt.user[SPC:0]);
    end
    $display("Generated burst of %0d packets", num_pkts);
    return bursts;
  endfunction : generate_burst

  // -----------------------------------------------------------------------------
  // Generate expected output samples by applying the expected frequency shift to
  // the input samples. Here, the generate_tone_pkt function is reused to generate
  // the NCO LUT samples for the frequency shift which is then multiplied with the
  // input samples to produce the expected output.
  // parameters:
  // - num_pkts         number of packets in the burst
  // - pkt_length       number of words of SPC samples in each packet
  // - initial_phase    starting phase for the first sample (normalized to [0,1))
  // - freq_norm        frequency of the tone to generate for the frequency shift,
  //                    normalized to the sampling rate (i.e., in range [-0.5, 0.5))
  // - phase_offset     phase offset between each sample in cycle
  //                    (accumulated over the cycle to simulate the effect of
  //                    different LUTs for each channel in the DDS)
  // - ampl             amplitude scaling factor to apply to the expected output samples
  //                    (in the range (0,1])
  // - input_pkt_queue  the original input samples to the DUT for which we want to
  //                    generate the expected output after frequency shifting
  // returns:
  // - axis_pkt_queue_t containing the generated expected output packets for the burst
  // Note: Currently the testbench only uses samples of width 32 bits
  //       (16 bits for real and 16 bits for imag)
  // -----------------------------------------------------------------------------
  function automatic axis_pkt_queue_t generate_expected_output(
    int              num_pkts,
    int              pkt_length,
    real             initial_phase,
    tone_config_t    tone_cfg = '{freq_norm: 0.0,
                                phase_offset: 0.0, ampl: 1.0},
    axis_pkt_queue_t input_pkt_queue
  );
    axis_pkt_queue_t expected_pkt_queue;

    axis_pkt_t input_pkt, freq_shift_pkt, expected_pkt;
    real       phase_acc_init = initial_phase;

    for (int pkt_num = 0; pkt_num < num_pkts; pkt_num++) begin
      if (LOG_LEVEL == DEBUG) begin
        $display("Generating expected output for Packet %0d:", pkt_num);
      end
      expected_pkt = new();
      freq_shift_pkt = new();
      input_pkt = input_pkt_queue.pop_front();
      freq_shift_pkt = generate_tone_pkt(pkt_length, phase_acc_init, tone_cfg,
                                        (pkt_num == num_pkts-1), NO_TAG, INFO);
      foreach (input_pkt.data[word_idx]) begin
        iq_word_t input_word = input_pkt.data[word_idx];
        iq_word_t expected_word;
        for (int sample_idx = 0; sample_idx < SPC; sample_idx++) begin
          sc16_t sample_a, sample_b, sample_product;
          sample_a = word_to_sc16(input_word, sample_idx);
          sample_b = word_to_sc16(freq_shift_pkt.data[word_idx], sample_idx);
          if (LOG_LEVEL == TRACE) begin
            $display("Packet %0d, Word %0d, Sample %0d:", pkt_num, word_idx, sample_idx);
            $display("Input Sample A (i,q) = %s", sc16_to_str(sample_a));
            $display("NCO LUT Sample B (i,q) = %s", sc16_to_str(sample_b));
          end
          sample_product            = mul_sc16(sample_a, sample_b);
          expected_word[sample_idx] = sample_product;
        end
        expected_pkt.data.push_back(expected_word);
        // Propagate user signal from input packet to expected output packet
        expected_pkt.user.push_back((pkt_num == num_pkts-1) ? '1 : '0);
      end
      expected_pkt_queue.push_back(expected_pkt);
    end
    $display("Generated expected output for %0d packets", expected_pkt_queue.size());
    return expected_pkt_queue;
  endfunction : generate_expected_output

  // -----------------------------------------------------------------------------
  // Generate expected output samples for a timed DDS test where the NCO
  // configuration switches at a tagged sample in the input packet queue.
  // Before the tagged sample, tone_cfg_pre is applied; at the tagged sample and
  // all subsequent samples, tone_cfg_post is applied.
  // The tag is detected by matching user[SPC:1] against time_tag.tag.
  // parameters:
  // - pkt_length       number of words of SPC samples in each packet
  // - tone_cfg_pre     NCO configuration applied before the time tag
  // - tone_cfg_post    NCO configuration applied at and after the time tag
  // - time_tag         time tag marking the configuration switch point
  // - input_pkt_queue  input sample packets to the DUT
  // returns:
  // - axis_pkt_queue_t containing the expected output packets
  // -----------------------------------------------------------------------------
  function automatic axis_pkt_queue_t generate_timed_expected_output(
    int              pkt_length,
    tone_config_t    tone_cfg_pre,
    tone_config_t    tone_cfg_post,
    time_tag_t       time_tag,
    axis_pkt_queue_t input_pkt_queue
  );
    axis_pkt_queue_t expected_pkt_queue;
    axis_pkt_t       input_pkt, expected_pkt;
    real             phase_pre  = 0.0;
    real             phase_post = 0.0;
    bit              tag_found  = 0;
    int              num_pkts   = input_pkt_queue.size();

    for (int pkt_num = 0; pkt_num < num_pkts; pkt_num++) begin
      if (LOG_LEVEL == DEBUG)
        $display("Generating timed expected output for Packet %0d:", pkt_num);
      expected_pkt = new();
      input_pkt    = input_pkt_queue.pop_front();

      for (int word_idx = 0; word_idx < pkt_length; word_idx++) begin
        iq_word_t     input_word = input_pkt.data[word_idx];
        iq_word_t     nco_word;
        iq_word_t     expected_word;

        logic [SPC:0] user = input_pkt.user[word_idx];

        // Detect the time tag: switch to post-tag config at the tagged sample
        if (!tag_found && (time_tag.tag != '0) && (user[SPC:1] == time_tag.tag))
          tag_found = 1;

        // Compute NCO samples for this word using the active tone config
        for (int sample_idx = 0; sample_idx < SPC; sample_idx++) begin
          comp_t nco_i, nco_q;
          real   nco_phase;
          if (tag_found) begin
            nco_phase = phase_post + sample_idx * tone_cfg_post.phase_offset;
            if ($sin(2*PI*nco_phase) >= (1.0 - 1e-4))
              nco_q = tone_cfg_post.ampl * MAX_COMP;
            else
              nco_q = (tone_cfg_post.ampl * $sin(2*PI*nco_phase)) * (2**(FRAC_W));
            if ($cos(2*PI*nco_phase) >= (1.0 - 1e-4))
              nco_i = tone_cfg_post.ampl * MAX_COMP;
            else
              nco_i = (tone_cfg_post.ampl * $cos(2*PI*nco_phase)) * (2**(FRAC_W));
          end else begin
            nco_phase = phase_pre + sample_idx * tone_cfg_pre.phase_offset;
            if ($sin(2*PI*nco_phase) >= (1.0 - 1e-4))
              nco_q = tone_cfg_pre.ampl * MAX_COMP;
            else
              nco_q = (tone_cfg_pre.ampl * $sin(2*PI*nco_phase)) * (2**(FRAC_W));
            if ($cos(2*PI*nco_phase) >= (1.0 - 1e-4))
              nco_i = tone_cfg_pre.ampl * MAX_COMP;
            else
              nco_i = (tone_cfg_pre.ampl * $cos(2*PI*nco_phase)) * (2**(FRAC_W));
          end
          nco_word[sample_idx][SAMP_W-1: COMP_W] = nco_i;
          nco_word[sample_idx][COMP_W-1: 0]      = nco_q;
        end

        // Advance the active phase accumulator by freq_norm (one sample word)
        if (tag_found)
          phase_post += tone_cfg_post.freq_norm;
        else
          phase_pre  += tone_cfg_pre.freq_norm;

        // Multiply input samples with NCO samples
        for (int sample_idx = 0; sample_idx < SPC; sample_idx++) begin
          sc16_t sample_in, sample_nco, sample_product;
          sample_in      = word_to_sc16(input_word, sample_idx);
          sample_nco     = word_to_sc16(nco_word, sample_idx);
          if (LOG_LEVEL == TRACE) begin
            $display("Packet %0d, WORD %0d, SAMP %0d (tag_found=%0b):",
                     pkt_num, word_idx, sample_idx, tag_found);
            $display("  Input  (i,q) = %s", sc16_to_str(sample_in));
            $display("  NCO    (i,q) = %s", sc16_to_str(sample_nco));
          end
          sample_product            = mul_sc16(sample_in, sample_nco);
          expected_word[sample_idx] = sample_product;
        end

        expected_pkt.data.push_back(expected_word);
        expected_pkt.user.push_back((pkt_num == num_pkts-1) ? '1 : '0);
      end
      expected_pkt_queue.push_back(expected_pkt);
    end
    $display("Generated timed expected output for %0d packets", expected_pkt_queue.size());
    return expected_pkt_queue;
  endfunction : generate_timed_expected_output

  // Compare sc16 samples (allow small rounding error)
  // parameters:
  // - a, b          the two sc16 samples to compare
  // - tolerance     the allowable error in LSBs for the sample comparison
  //                 due to rounding in multiplication
  // returns:
  // - 0 if the samples are within the specified tolerance
  // - 1 if they deviate more than the tolerance
  // TODO: Consider adding to PkgComplex as a utility function
  function automatic bit compare_samples(sc16_t a, sc16_t b,
                                         sc16_t tolerance = '{re: s16_t'(0), im: s16_t'(0)});
    Math #(s16_t) m;
    sc16_t diff = sub_sc16(a, b);
    if (m.abs(diff.re) > tolerance.re || m.abs(diff.im) > tolerance.im) begin
      $display("Deviation: %s", sc16_to_str(diff));
      return 1;
    end
    return 0;
  endfunction : compare_samples

  // Dequeue received samples and compare with expected samples
  // Assert error on mismatches
  function automatic void dequeue_and_compare_cmplx (
    axis_burst_queue_t expected_bursts,
    axis_burst_queue_t received_bursts,
    axis_burst_queue_t sent_bursts
  );
    axis_pkt_queue_t expected_pkt_queue, received_pkt_queue, sent_pkt_queue;
    axis_pkt_t       expected_pkt, recvd_pkt, sent_pkt;
    iq_word_t        expected_word, recvd_word, sent_word;

    `ASSERT_ERROR(received_bursts.size() == expected_bursts.size(),
                $sformatf("Received bursts size mismatch: expected %0d, got %0d",
                expected_bursts.size(), received_bursts.size()));

    begin : burst_loop
      int num_bursts = expected_bursts.size();
      for (int burst_idx = 0; burst_idx < num_bursts; burst_idx++) begin
        expected_pkt_queue = expected_bursts.pop_front();
        received_pkt_queue = received_bursts.pop_front();
        sent_pkt_queue     = sent_bursts.pop_front();
        `ASSERT_ERROR(received_pkt_queue.size() == expected_pkt_queue.size(),
                    $sformatf("Burst %0d: Expected %0d packets, got %0d",
                    burst_idx, expected_pkt_queue.size(), received_pkt_queue.size()));
        begin : pkt_loop
          int num_pkts = expected_pkt_queue.size();
          for (int pkt_idx = 0; pkt_idx < num_pkts; pkt_idx++) begin
            expected_pkt = expected_pkt_queue.pop_front();
            recvd_pkt    = received_pkt_queue.pop_front();
            `ASSERT_ERROR(recvd_pkt.data.size() == expected_pkt.data.size(),
                        $sformatf("Burst %0d, Packet %0d: Expected %0d samples, got %0d",
                        burst_idx, pkt_idx, expected_pkt.data.size(), recvd_pkt.data.size()));
            begin : word_loop
              int num_words = expected_pkt.data.size();
              for (int word_idx = 0; word_idx < num_words; word_idx++) begin
                expected_word = expected_pkt.data.pop_front();
                sent_word     = sent_pkt_queue[pkt_idx].data.pop_front();
                recvd_word    = recvd_pkt.data.pop_front();
                for (int sample_idx = 0; sample_idx < SPC; sample_idx++) begin
                  sc16_t expected, sent, recvd;
                  expected = word_to_sc16(expected_word, sample_idx);
                  sent     = word_to_sc16(sent_word, sample_idx);
                  recvd    = word_to_sc16(recvd_word, sample_idx);
                  if (LOG_LEVEL != INFO) begin
                      $display(
                        {"Burst %0d, Packet %0d, Word %0d, Sample %0d: Sent %s, ",
                         "Expected %s, Got %s"},
                        burst_idx, pkt_idx, word_idx, sample_idx,
                        sc16_to_str(sent), sc16_to_str(expected), sc16_to_str(recvd)
                      );
                  end
                  `ASSERT_ERROR(compare_samples(expected, recvd, ERR_TOLERANCE) == 0,
                      $sformatf(
                        {"Burst %0d, Packet %0d, Word %0d, Sample %0d: Sent %s, ",
                         "Expected %s, Got %s"},
                        burst_idx, pkt_idx, word_idx, sample_idx,
                        sc16_to_str(sent), sc16_to_str(expected), sc16_to_str(recvd)
                      ));
                end
              end
            end : word_loop
          end
        end : pkt_loop
      end
    end : burst_loop
  endfunction : dequeue_and_compare_cmplx

  //---------------------------------------------------------------------------
  // AXI-Stream Phase Write
  //---------------------------------------------------------------------------

  // Write a phase value to the DUT via the AXI-Stream phase interface.
  // Set timed=1 to assert tuser, indicating a timed phase update.
  task automatic write_phase_axis(
    input logic [PHASE_WIDTH-1:0] phase_data,
    input logic                   timed = 1'b0
  );
    @(posedge clk);
    phase_to_dut.tdata  <= phase_data;
    phase_to_dut.tuser  <= timed;
    phase_to_dut.tvalid <= 1'b1;
    do begin
      @(posedge clk);
    end while (!phase_to_dut.tready);
    phase_to_dut.tvalid <= 1'b0;
  endtask : write_phase_axis

  // Monitor the samples_to_dut AXI-Stream, count packet/word transfers, and
  // call write_phase_axis with timed=1 ONE transfer BEFORE the tagged position
  // so that the phase pulse is valid on s_axis_phase during the same clock
  // cycle as the tagged sample on s_axis_din, satisfying the DUT's
  // tag_phase_change condition.
  task automatic write_phase_timed(
    input logic [PHASE_WIDTH-1:0] phase_data,
    input time_tag_t              time_tag
  );
    int pkt_count  = 0;
    int word_count = 0;
    int next_pkt, next_word;

    // tag is on the very first word (0,0); write immediately so
    // the phase pulse coincides with that first transfer.
    if (time_tag.packet == 0 && time_tag.word == 0) begin
      write_phase_axis(phase_data, 1'b1);
      return;
    end

    forever begin
      @(posedge clk);
      if (samples_to_dut.tvalid && samples_to_dut.tready) begin
        // Compute counter values after this transfer completes
        if (samples_to_dut.tlast) begin
          next_pkt  = pkt_count + 1;
          next_word = 0;
        end else begin
          next_pkt  = pkt_count;
          next_word = word_count + 1;
        end
        // Write one transfer early: write_phase_axis holds the pulse for one
        // clock, so by the time it fires the tagged transfer is in progress.
        if (next_pkt == time_tag.packet && next_word >= (time_tag.word - 1)) begin
          write_phase_axis(phase_data, 1'b1);
          break;
        end
        pkt_count = next_pkt;
        word_count = next_word;
      end
    end
  endtask : write_phase_timed

  //---------------------------------------------------------------------------
  // Test Cases
  //---------------------------------------------------------------------------

  // Multiple phase writes test
  // Repeatedly writes random untimed frequency shift values to the DUT
  // Also includes a sanity check with one burst of samples to ensure that the DUT applies
  // last written frequency shift value to the input samples to produce the expected output.
  task automatic multi_wr_test();
    axis_burst_queue_t                      input_bursts, received_bursts, expected_bursts;
    axis_pkt_queue_t                        pkts_in, pkts_out, expected_pkts_out;
    axis_pkt_t                              get_pkt;
    logic             [PHASE_WIDTH-1:0]     phase_in_tdata;
    logic             [CTRLPORT_DATA_W-1:0] write_data;
    tone_config_t                           input_tone_cfg, dut_tone_cfg;
    shortreal                               smallest_phase_offset;
    test.start_test("Multiple Phase Writes Test");
    repeat (10) begin
      write_data = $urandom_range(MAX_PHASE);
      write_phase_axis(write_data[CTRLPORT_DATA_W-1 -: PHASE_WIDTH]);
    end
    if (LOG_LEVEL != INFO) begin
      $display("Register read/write verification passed");
      $display("Sanity check with one burst of samples to verify applied frequency shift...");
    end

    phase_in_tdata = write_data[CTRLPORT_DATA_W-1 -: PHASE_WIDTH];

    // Convert phase increment to frequency shift.
    // Per-lane phase offset = base_phase_inc (no /SPC: hardware uses freq_word directly)
    dut_tone_cfg.phase_offset    = compute_phase_offset(phase_in_tdata);
    // The accumulator advances by SPC*phase_in_tdata per word (clock cycle),
    // so freq_norm (per-word advance, normalized) must include the SPC factor.
    dut_tone_cfg.freq_norm       = SPC * dut_tone_cfg.phase_offset;
    dut_tone_cfg.ampl            = 1.0; // Set amplitude scaling for expected output

    // Random frequency for input tone
    input_tone_cfg.freq_norm    = gen_freq_norm(int'($urandom_range(MAX_PHASE)), CTRLPORT_DATA_W);
    input_tone_cfg.phase_offset = 0.0;
    input_tone_cfg.ampl         = 0.7;
    pkts_in            = generate_burst(.num_pkts(5), .pkt_length(PKT_LENGTH), .tone_cfg(input_tone_cfg),
                                        .gen_mode(TONE));
    expected_pkts_out  = generate_expected_output(.num_pkts(5), .pkt_length(PKT_LENGTH),
                                                  .initial_phase(0.0), .tone_cfg(dut_tone_cfg),
                                                  .input_pkt_queue(pkts_in));

    input_bursts.push_back(pkts_in);
    expected_bursts.push_back(expected_pkts_out);

    axis_bfm.set_master_stall_prob(10);
    axis_bfm.set_slave_stall_prob(0);

    foreach (pkts_in[in_idx]) begin
      axis_bfm.put(pkts_in[in_idx]);
      axis_bfm.get(get_pkt);
      pkts_out.push_back(get_pkt);
    end
    axis_bfm.wait_complete();
    received_bursts.push_back(pkts_out);

    dequeue_and_compare_cmplx(expected_bursts, received_bursts, input_bursts);

    test.end_test();
  endtask : multi_wr_test

  // Randomized test with multiple bursts of random complex samples and
  // random frequency shifts applied
  task automatic randomize_test (
    gen_mode_t     gen_mode,
    int            repeat_count = 3,
    stall_config_t stall_cfg    = '{master_stall_prob: 0, slave_stall_prob: 0}
  );
    axis_burst_queue_t input_bursts, received_bursts, expected_bursts;

    test.start_test("Randomized Test");

    repeat (repeat_count) begin
      axis_pkt_queue_t                      pkts_in, pkts_out, expected_pkts_out;
      axis_pkt_t                            get_pkt;
      logic           [PHASE_WIDTH-1:0]     phase_in_tdata;
      logic           [CTRLPORT_DATA_W-1:0] write_phase;
      tone_config_t                         input_tone_cfg, dut_tone_cfg;
      shortreal                             smallest_phase_offset;

      // Set phase increment for the DDS
      write_phase                  = int'($urandom_range(MAX_PHASE));
      phase_in_tdata               = write_phase[CTRLPORT_DATA_W-1 -: PHASE_WIDTH];
      // Convert phase increment to frequency shift.
      // Per-lane phase offset = base_phase_inc (no /SPC: hardware uses freq_word directly)
      dut_tone_cfg.phase_offset    = compute_phase_offset(phase_in_tdata);
      // The accumulator advances by SPC*phase_in_tdata per word (clock cycle),
      // so freq_norm (per-word advance, normalized) must include the SPC factor.
      dut_tone_cfg.freq_norm       = SPC * dut_tone_cfg.phase_offset;
      dut_tone_cfg.ampl            = 1.0;

      if (gen_mode == CONST) begin
        input_tone_cfg.freq_norm    = 0.0; // Generate constant samples for input
        input_tone_cfg.phase_offset = 0.0;
        input_tone_cfg.ampl         = 1.0; // Set amplitude for input tone
      end else if (gen_mode == TONE) begin
        // Random frequency for input tone
          input_tone_cfg.freq_norm  =
            gen_freq_norm(int'($urandom_range(MAX_PHASE)), CTRLPORT_DATA_W);
        input_tone_cfg.phase_offset = 0.0;
        input_tone_cfg.ampl         = 0.7;
      end else begin
        input_tone_cfg              = '{freq_norm: 0.0, phase_offset: 0.0, ampl: 0.7};
      end

      $display ($sformatf("Running in %s mode with Freq shift (norm.): %f",
                gen_mode.name(), dut_tone_cfg.freq_norm));

      pkts_in            = generate_burst(.num_pkts(5), .pkt_length(PKT_LENGTH), .tone_cfg(input_tone_cfg),
                                          .gen_mode(gen_mode));
      expected_pkts_out  = generate_expected_output(.num_pkts(5), .pkt_length(PKT_LENGTH),
                                                    .initial_phase(0.0), .tone_cfg(dut_tone_cfg),
                                                    .input_pkt_queue(pkts_in));

      input_bursts.push_back(pkts_in);
      expected_bursts.push_back(expected_pkts_out);

      axis_bfm.set_master_stall_prob(stall_cfg.master_stall_prob);
      axis_bfm.set_slave_stall_prob(stall_cfg.slave_stall_prob);
      write_phase_axis(phase_in_tdata);

      clk_gen.clk_wait_r(1);
      foreach (pkts_in[in_idx]) begin
        axis_bfm.put(pkts_in[in_idx]);
        axis_bfm.get(get_pkt);
        pkts_out.push_back(get_pkt);
      end
      axis_bfm.wait_complete();
      received_bursts.push_back(pkts_out);

      pkts_in.delete();
      pkts_out.delete();
      expected_pkts_out.delete();
    end

    // output validation
    dequeue_and_compare_cmplx(expected_bursts, received_bursts, input_bursts);

    test.end_test();
  endtask : randomize_test

  // Test to verify freq shift once set is applied consistently
  // across multiple bursts of samples unless changed.
  task automatic continuous_run (int repeat_count, stall_config_t stall_cfg);
    axis_burst_queue_t                    input_bursts, received_bursts, expected_bursts;
    axis_pkt_queue_t                      pkts_in, pkts_out, expected_pkts_out;
    tone_config_t                         input_tone_cfg, dut_tone_cfg;
    logic           [PHASE_WIDTH-1:0]     phase_in_tdata;
    logic           [CTRLPORT_DATA_W-1:0] write_phase;
    // Tracks the NCO phase continuously across bursts to match DUT accumulator state
    real                                  expected_phase = 0.0;

    test.start_test("Continuous Run");
    write_phase                    = $urandom_range(MAX_PHASE);
    phase_in_tdata                 = write_phase[CTRLPORT_DATA_W-1 -: PHASE_WIDTH];
    // Convert phase increment to frequency shift.
    // Per-lane phase offset = base_phase_inc (no /SPC: hardware uses freq_word directly)
    dut_tone_cfg.phase_offset      = compute_phase_offset(phase_in_tdata);
    // The accumulator advances by SPC*phase_in_tdata per word (clock cycle),
    // so freq_norm (per-word advance, normalized) must include the SPC factor.
    dut_tone_cfg.freq_norm         = SPC * dut_tone_cfg.phase_offset;
    dut_tone_cfg.ampl              = 1.0;
    $display($sformatf("Frequency shift (norm.): %f", dut_tone_cfg.freq_norm));
    if (LOG_LEVEL == DEBUG) begin
      $display($sformatf("Master stall: %f, Slave Stall: %f",
                        stall_cfg.master_stall_prob, stall_cfg.slave_stall_prob));
    end

    axis_bfm.set_master_stall_prob(stall_cfg.master_stall_prob);
    axis_bfm.set_slave_stall_prob(stall_cfg.slave_stall_prob);
    write_phase_axis(phase_in_tdata);

    repeat (repeat_count) begin
        input_tone_cfg.freq_norm       =
          gen_freq_norm(int'($urandom_range(MAX_PHASE)), CTRLPORT_DATA_W);
        input_tone_cfg.phase_offset    = 0.0;
        input_tone_cfg.ampl            = 0.7;

        pkts_in            = generate_burst(.num_pkts(3), .pkt_length(PKT_LENGTH),
                                            .tone_cfg(input_tone_cfg), .gen_mode(TONE));
        expected_pkts_out  = generate_expected_output(.num_pkts(3), .pkt_length(PKT_LENGTH),
                                                      .initial_phase(expected_phase), .tone_cfg(dut_tone_cfg),
                                                      .input_pkt_queue(pkts_in));
        // Advance expected_phase by the number of words consumed in this burst
        // (num_pkts * pkt_length words, each advancing phase by freq_norm)
        expected_phase += 3 * PKT_LENGTH * dut_tone_cfg.freq_norm;
        input_bursts.push_back(pkts_in);
        expected_bursts.push_back(expected_pkts_out);

        foreach (pkts_in[in_idx]) begin
          axis_bfm.put(pkts_in[in_idx]);
          axis_bfm.get(pkts_out[in_idx]);
        end
        axis_bfm.wait_complete();
        received_bursts.push_back(pkts_out);
        pkts_out.delete();
    end

    // output validation
    dequeue_and_compare_cmplx(expected_bursts, received_bursts, input_bursts);

    test.end_test();
  endtask : continuous_run

  task automatic sim_timed_test(stall_config_t stall_cfg);
    axis_burst_queue_t                    input_bursts, received_bursts, expected_bursts;
    axis_pkt_queue_t                      pkts_in, pkts_out, expected_pkts_out;
    axis_pkt_queue_t                      pkts_in_pre_tag, pkts_in_post_tag;
    axis_pkt_t                            send_pkt, get_pkt;
    tone_config_t                         input_tone_cfg, dut_tone_cfg, timed_dut_tone_cfg;
    logic           [PHASE_WIDTH-1:0]     phase_in_tdata;
    logic           [CTRLPORT_DATA_W-1:0] write_phase, timed_write_phase;
    time_tag_t                            data_in_tag;

    int num_pkts   = 2;
    int pkt_length = PKT_LENGTH;
    // Phase accumulated by the post-tag NCO by the end of the first burst,
    // used as initial phase for the follow-up burst verification
    int  words_pre_tag       = 0;
    int  words_post_tag      = 0;
    real timed_initial_phase = 0.0;


    test.start_test($sformatf("Simulated timed DDS test, master stall: %0d, slave stall: %0d",
                              stall_cfg.master_stall_prob, stall_cfg.slave_stall_prob));
    // Set untimed phase increment for the DDS
    write_phase                  = $urandom_range(MAX_PHASE);
    phase_in_tdata               = write_phase[CTRLPORT_DATA_W-1 -: PHASE_WIDTH];
    // Convert phase increment to frequency shift.
    // Per-lane phase offset = base_phase_inc (no /SPC: hardware uses freq_word directly)
    dut_tone_cfg.phase_offset    = compute_phase_offset(phase_in_tdata);
    // The accumulator advances by SPC*phase_in_tdata per word (clock cycle),
    // so freq_norm (per-word advance, normalized) must include the SPC factor.
    dut_tone_cfg.freq_norm       = SPC * dut_tone_cfg.phase_offset;
    dut_tone_cfg.ampl            = 1.0;

    //Timed phase increment
    timed_write_phase               = $urandom_range(MAX_PHASE);
    phase_in_tdata                  = timed_write_phase[CTRLPORT_DATA_W-1 -: PHASE_WIDTH];
    // Convert phase increment to frequency shift.
    // Per-lane phase offset = base_phase_inc (no /SPC: hardware uses freq_word directly)
    timed_dut_tone_cfg.phase_offset = compute_phase_offset(phase_in_tdata);
    // The accumulator advances by SPC*phase_in_tdata per word (clock cycle),
    // so freq_norm (per-word advance, normalized) must include the SPC factor.
    timed_dut_tone_cfg.freq_norm    = SPC * timed_dut_tone_cfg.phase_offset;
    timed_dut_tone_cfg.ampl         = 1.0;

    input_tone_cfg.freq_norm    = 0.0; // Generate constant/ramp samples for input
    input_tone_cfg.phase_offset = 0.0;
    input_tone_cfg.ampl         = 1.0; // Set amplitude for input tone

    $display ($sformatf("Running in %s mode with Freq shift (norm.): %f",
              "CONST", dut_tone_cfg.freq_norm));

    // Tag random sample in the burst for timed phase update
    data_in_tag.packet = $urandom_range(num_pkts-1);
    data_in_tag.word   = $urandom_range(CMD_LATENCY, pkt_length-3);
    data_in_tag.tag    = 1 << $urandom_range(SPC-1);

    $display($sformatf("Time tag set for packet %0d, word %0d, tag = %0b",
            data_in_tag.packet, data_in_tag.word, data_in_tag.tag));

    pkts_in            = generate_burst(.num_pkts(num_pkts), .pkt_length(pkt_length),
                                        .tone_cfg(input_tone_cfg),
                                        .gen_mode(RAMP), .time_tag(data_in_tag));

    expected_pkts_out  = generate_timed_expected_output(.pkt_length(pkt_length),
                        .tone_cfg_pre    (dut_tone_cfg),
                        .tone_cfg_post   (timed_dut_tone_cfg),
                        .time_tag        (data_in_tag),
                        .input_pkt_queue (pkts_in));

    input_bursts.push_back(pkts_in);
    expected_bursts.push_back(expected_pkts_out);

    axis_bfm.set_master_stall_prob(stall_cfg.master_stall_prob);
    axis_bfm.set_slave_stall_prob(stall_cfg.slave_stall_prob);
    write_phase_axis(write_phase[CTRLPORT_DATA_W-1 -: PHASE_WIDTH]);

    clk_gen.clk_wait_r(1);
    fork
      begin : axis_bfm_thread
        foreach(pkts_in[in_idx]) begin
          axis_bfm.put(pkts_in[in_idx]);
          axis_bfm.get(pkts_out[in_idx]);
        end
        axis_bfm.wait_complete();
        received_bursts.push_back(pkts_out);
      end
      begin : update_freq_thread
        write_phase_timed(phase_in_tdata, data_in_tag);
        $display($sformatf("Updated Frequency shift (norm.): %f",
                SPC * timed_dut_tone_cfg.freq_norm));
      end
    join

    pkts_in.delete();
    pkts_out.delete();
    expected_pkts_out.delete();

    // output validation
    dequeue_and_compare_cmplx(expected_bursts, received_bursts, input_bursts);
    input_bursts.delete();
    expected_bursts.delete();
    received_bursts.delete();

    // Ensure that the DUT applies the last written frequency shift value if
    // new phase increment is sent but not valid
    phase_to_dut.tdata = '0;
    // Derive the post-tag NCO phase at the end of the previous burst:
    // the post-tag config becomes active at data_in_tag and runs for the
    // remaining words in the burst.
    words_pre_tag      = data_in_tag.packet * pkt_length + data_in_tag.word;
    words_post_tag     = num_pkts * pkt_length - words_pre_tag;
    timed_initial_phase = words_post_tag * timed_dut_tone_cfg.freq_norm;
    pkts_in = generate_burst(.num_pkts(num_pkts), .pkt_length(pkt_length),
                        .tone_cfg(input_tone_cfg),
                        .gen_mode(RAMP));
    expected_pkts_out  = generate_expected_output(.num_pkts(num_pkts), .pkt_length(pkt_length),
                        .initial_phase(timed_initial_phase), .tone_cfg(timed_dut_tone_cfg),
                        .input_pkt_queue(pkts_in));
    input_bursts.push_back(pkts_in);
    expected_bursts.push_back(expected_pkts_out);

    foreach(pkts_in[in_idx]) begin
      axis_bfm.put(pkts_in[in_idx]);
      axis_bfm.get(pkts_out[in_idx]);
    end
    axis_bfm.wait_complete();
    received_bursts.push_back(pkts_out);

    dequeue_and_compare_cmplx(expected_bursts, received_bursts, input_bursts);

    test.end_test();
  endtask : sim_timed_test

  initial begin : main
    test.start_tb($sformatf("DDS Multisample Testbench - SPC=%0d", SPC));
    clk_gen.start();
    dds_ctrlport_bfm.run();
    axis_bfm.run();
    phase_to_dut.tvalid = 1'b0;
    phase_to_dut.tdata  = '0;
    phase_to_dut.tuser  = 1'b0;

    // Delay reset assertion to avoid warnings from the DDS LUT IP
    clk_gen.clk_wait_f(2);
    clk_gen.reset(2);
    // Wait for reset deassertion
    @(negedge rst);

    multi_wr_test();

    randomize_test(.gen_mode(CONST), .repeat_count(1),
                   .stall_cfg('{master_stall_prob: 0, slave_stall_prob: 0}));
    randomize_test(.gen_mode(TONE), .repeat_count(1),
                   .stall_cfg('{master_stall_prob: 20, slave_stall_prob: 80}));
    randomize_test(.gen_mode(TONE), .repeat_count(1),
                   .stall_cfg('{master_stall_prob: 80, slave_stall_prob: 20}));
    randomize_test(.gen_mode(TONE), .repeat_count(1),
                   .stall_cfg('{master_stall_prob: 50, slave_stall_prob: 50}));
    randomize_test(.gen_mode(RAND), .repeat_count(3),
                   .stall_cfg('{master_stall_prob: 0, slave_stall_prob: 0}));
    continuous_run(.repeat_count(10), .stall_cfg('{master_stall_prob: $urandom_range(80),
                                                slave_stall_prob: $urandom_range(80)}));

    sim_timed_test(.stall_cfg('{master_stall_prob: 0, slave_stall_prob: 0}));
    sim_timed_test(.stall_cfg('{master_stall_prob: $urandom_range(80),
                              slave_stall_prob: $urandom_range(80)}));
    test.end_tb(0);
    clk_gen.kill();
  end

endmodule
