//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Package: cic_test_pkg
//
// Description:
//
//   Package containing utilities and simulation models for CIC multisample TBs.
//
// Parameters:
//
//   SPC     : Number of samples per clock cycle on DUT
//   ACCUM_W : Width of a DUT input sample. Equivalent to the accumulator width.
//   COMP_W  : Width of I and Q components in the input samples. This is
//             used to generate test inputs and should be less than or equal to
//             ACCUM_W/2.
//

`default_nettype none
package cic_test_pkg;
  import cic_utils_pkg::*;
  import PkgRandom::*;
  import PkgAxiStreamBfm::*;

  //---------------------------------------------------------------------------
  // Test utilities
  //---------------------------------------------------------------------------
  class cic_test_utils #(
    parameter int SPC        = 2,
    parameter int ACCUM_W    = 32,
    parameter int COMP_W     = 16
  );


    //---------------------------------------------------------------------------
    // Parametrized type definitions
    //---------------------------------------------------------------------------
    typedef cic_utils#(
      .SPC(SPC),
      .SAMP_W(ACCUM_W)
    ) util_c;

    typedef util_c::sample_t                             sample_t;
    typedef util_c::comp_t                               comp_t;
    typedef util_c::word_t                               word_t;
    typedef sample_t                                     sample_queue_t[$];

    typedef AxiStreamPacket #(.DATA_WIDTH(ACCUM_W*SPC)) axis_pkt_t;
    typedef AxiStreamPacket #(.DATA_WIDTH(ACCUM_W))      axis_single_sample_pkt_t;
    typedef axis_pkt_t                                   pkt_burst_t[$];
    typedef axis_single_sample_pkt_t                     pkt_burst_single_sample_t[$];

    //-----------------------------------------------------------------------
    // Local parameters
    //-----------------------------------------------------------------------
    localparam bit VERBOSE = 1'b0;

    //------------------------------------------------------------------------
    // Constructor/Parameter checks
    //------------------------------------------------------------------------
    function new();
      if (ACCUM_W >= 64) begin
        $fatal(1, {"ACCUM_W parameter (%0d) must be less than or equal to 64",
                " due to sim model longint accumulator width limits"},
               ACCUM_W);
      end
    endfunction

    //-----------------------------------------------------------------------
    // Generate random IQ samples in COMP_W range
    //-----------------------------------------------------------------------
    static function automatic sample_t gen_iq_width_random_sample();
      comp_t i_value = $urandom_range(-2**(COMP_W-1), 2**(COMP_W-1)-1);
      comp_t q_value = $urandom_range(-2**(COMP_W-1), 2**(COMP_W-1)-1);
      return {i_value, q_value};
    endfunction : gen_iq_width_random_sample

    //-----------------------------------------------------------------------
    // Generate samples for a given set of I and Q values.
    //-----------------------------------------------------------------------
    static function automatic sample_t gen_sample_data(
      comp_t i_value = '0,
      comp_t q_value = '1,
      bit random_data  = 1'b0,
      bit fullscale  = 1'b0   // 0 => generate samples in IQ_W range.
                              // 1 => generate samples in ACCUM_W range.
    );
      sample_t data_word;
      if (!random_data) begin
        data_word = {i_value, q_value};
      end else begin
        if (fullscale) begin
          data_word = sample_t'(Rand#()::rand_bit());
        end else begin
          data_word = gen_iq_width_random_sample();
        end
      end
      return data_word;
    endfunction : gen_sample_data

    //-----------------------------------------------------------------------
    // Generate data word, SPC samples wide, specify I and q for all samples.
    //-----------------------------------------------------------------------
    static function automatic word_t gen_data_word(
      comp_t i_value = '0,
      comp_t q_value = '1,
      bit random_data = 1'b0
    );
      word_t ramp_word;
      for (int idx = 0; idx < SPC; idx++) begin
        ramp_word[idx] = gen_sample_data(i_value, q_value, random_data);
      end
      return ramp_word;
    endfunction : gen_data_word

    //-----------------------------------------------------------------------
    // Generate samples of Q values of alternating 1s and -1s, I = 0.
    //-----------------------------------------------------------------------
    static function automatic word_t gen_alternating_ones_array();
      word_t alt_word;
      for (int idx = 0; idx < SPC; idx++) begin
        alt_word[idx] = (idx % 2 == 0) ? gen_sample_data(0, -1) : gen_sample_data(0, 1);
        if (VERBOSE) begin
          $display("Generated alternating ones array sample %0d: %0d", idx, alt_word[idx]);
        end
      end
      return alt_word;
    endfunction : gen_alternating_ones_array

    //-------------------------------------------------------------------------
    // Convert axis_pkt_t payload to to sample queue.
    //-------------------------------------------------------------------------
    static function automatic sample_queue_t pkt_to_samples(axis_pkt_t pkt);
      sample_queue_t queue;
      while (pkt.data.size() > 0) begin
        logic [ACCUM_W*SPC-1:0] data_word = pkt.data.pop_front();
        for (int idx = 0; idx < SPC; idx++) begin
          queue.push_back(data_word[ACCUM_W*idx +: ACCUM_W]);
        end
      end
      return queue;
    endfunction : pkt_to_samples

    //-------------------------------------------------------------------------
    // generate_random_packet: generate an axis_pkt_t with num_words random
    // words covering the full ACCUM_W*SPC bit range. Suitable for DUTs that
    // do not saturate (e.g. linear comb or integrator filters).
    //   num_words : number of data words to generate.
    //   returns   : packet with num_words fully-random data words.
    //-------------------------------------------------------------------------
    static function automatic axis_pkt_t generate_random_packet(int num_words);
      automatic axis_pkt_t pkt = new();
      repeat (num_words) begin
        pkt.data.push_back(Rand#(ACCUM_W * SPC)::rand_bit());
      end
      return pkt;
    endfunction : generate_random_packet

    //-------------------------------------------------------------------------
    // generate_ramp_packet: generate an axis_pkt_t with a linearly increasing
    // sample pattern. Each sample is {i_value, q_value} where both I and Q
    // start at start_val and increment by incr for each successive sample.
    // The ramp is applied identically to both I and Q components.
    //   num_words : number of SPC-wide data words to generate.
    //   start_val : starting value for both I and Q (default 1).
    //   incr      : increment per sample (default 1).
    //   returns   : packet with num_words*SPC ramping samples.
    //-------------------------------------------------------------------------
    static function automatic axis_pkt_t generate_ramp_packet(
      int num_words,
      comp_t start_val = 1,
      comp_t incr      = 1
    );
      automatic axis_pkt_t pkt = new();
      automatic comp_t val = start_val;
      repeat (num_words) begin
        word_t word;
        foreach (word[idx]) begin
          word[idx] = {val, val};
          val += incr;
        end
        pkt.data.push_back(word);
      end
      return pkt;
    endfunction : generate_ramp_packet

    //-------------------------------------------------------------------------
    // generate_constant_packet: generate an axis_pkt_t where every sample has
    // the same I and Q component value.
    //   num_words : number of SPC-wide data words to generate.
    //   i_value   : value used for  I components of every sample.
    //   q_value   : value used for  Q components of every sample. If not
    //                 specified, defaults to the same value as i_value.
    //   returns   : packet with num_words*SPC identical samples.
    //-------------------------------------------------------------------------
    static function automatic axis_pkt_t generate_constant_packet(
      int num_words,
      comp_t i_value,
      comp_t q_value = i_value
    );
      automatic axis_pkt_t pkt = new();
      automatic word_t word = gen_data_word(i_value, q_value);
      repeat (num_words) begin
        pkt.data.push_back(word);
      end
      return pkt;
    endfunction : generate_constant_packet

    //-------------------------------------------------------------------------
    // generate_impulse_packet: generate an axis_pkt_t where only the first
    // sample has I=Q=1 and all remaining samples are zero.
    //   num_words : number of SPC-wide data words to generate.
    //   returns   : packet with a single impulse at sample index 0.
    //-------------------------------------------------------------------------
    static function automatic axis_pkt_t generate_impulse_packet(int num_words);
      automatic axis_pkt_t pkt = new();
      automatic word_t first_word;
      automatic word_t zero_word = gen_data_word(0, 0);
      // if num_words is zero, return an empty packet.
      if (num_words > 0) begin
        // First word: sample 0 is the impulse, rest are zero
        first_word = zero_word;
        first_word[0] = {comp_t'(1), comp_t'(1)};
        pkt.data.push_back(first_word);
        // Remaining words are all zero
        repeat (num_words - 1) begin
          pkt.data.push_back(zero_word);
        end
      end
      return pkt;
    endfunction : generate_impulse_packet

  endclass

  //---------------------------------------------------------------------------
  // Simulation models
  //---------------------------------------------------------------------------
  //
  // Bit-true model of cic multisample integrator.
  //
  // Processes one sample at a time (SPC=1), maintaining a running cumulative
  // sum for I and Q channels independently. The accumulator wraps at the same
  // extended bit width used by the RTL, and the output is saturated to COMP_W
  // bits, matching the DUT's axi_round_and_clip_complex output path.
  //
  // Note: The integrator is only adding sample values together. The DUT does
  // not care about the fixed point representation, as it only ever adds samples
  // to each other so the binary point position does not affect the arithmetic
  // so the rounding sub-stage is bypassed; only saturation clipping applies.
  //
  class cic_integrator_stage_model #(
    int SAMP_W    = 32,
    int ACCUM_W   = SAMP_W,   // By default, no clip and round.
    int MAX_DECIM = 255,
    int MAX_DELAY = 8
  );

    //-----------------------------------------------------------------------
    // Parameters
    //-----------------------------------------------------------------------
    localparam int COMP_W     = SAMP_W / 2;

    //-----------------------------------------------------------------------
    // Type definitions
    //-----------------------------------------------------------------------
    typedef cic_utils#(
        .SAMP_W(SAMP_W)
    ) util_c;

    typedef util_c::sample_t    sample_t;
    typedef util_c::comp_t    comp_t;

    //-----------------------------------------------------------------------
    // Internal state (persistant until reset/clear)
    //-----------------------------------------------------------------------
    longint sum_i;
    longint sum_q;

    //-----------------------------------------------------------------------
    // Constructor
    //-----------------------------------------------------------------------
    function new();
      reset();
    endfunction

    //-----------------------------------------------------------------------
    // reset: reset accumulator state
    //-----------------------------------------------------------------------
    function void reset();
      sum_i = 0;
      sum_q = 0;
    endfunction

    //-----------------------------------------------------------------------
    // wrap_to_width: wrap a longint value to N-bit two's complement
    //-----------------------------------------------------------------------
    static function longint wrap_to_width(longint val, int width);
      longint mask;
      longint sign_bit;
      mask     = (longint'(1) << width) - 1;
      sign_bit = longint'(1) << (width - 1);
      val      = val & mask;
      // Sign-extend: if the sign bit of the N-bit value is set, extend it
      if (val & sign_bit)
        val = val | (~mask);
      return val;
    endfunction

    //-----------------------------------------------------------------------
    // saturate: clip a longint to COMP_W-bit signed range
    //
    // Models the axi_clip module behavior:
    //   overflow detected when upper bits are not all-same as sign bit
    //   positive overflow  -> max positive = {0, {COMP_W-1{1}}}
    //   negative overflow  -> min negative = {1, {COMP_W-1{0}}}
    //-----------------------------------------------------------------------
    static function comp_t saturate(longint val);
      localparam longint max_pos = (longint'(1) << (COMP_W - 1)) - 1;
      localparam longint min_neg = -(longint'(1) << (COMP_W - 1));
      if (val > max_pos)
        return comp_t'(max_pos);
      else if (val < min_neg)
        return comp_t'(min_neg);
      else
        return comp_t'(val);
    endfunction

    //-----------------------------------------------------------------------
    // integrate: process one sample, return clipped output
    //
    // Input sample layout: { I[COMP_W-1:0], Q[COMP_W-1:0] }
    //   I = upper half, Q = lower half
    //-----------------------------------------------------------------------
    function sample_t integrate(sample_t sample_in);
      comp_t in_i, in_q;
      comp_t out_i, out_q;

      // Split into signed I and Q components
      in_q = comp_t'(sample_in[COMP_W-1:0]);
      in_i = comp_t'(sample_in[SAMP_W-1:COMP_W]);

      // Accumulate and wrap to per-component bit width (matches RTL overflow)
      sum_q = wrap_to_width(sum_q + longint'(in_q), ACCUM_W / 2);
      sum_i = wrap_to_width(sum_i + longint'(in_i), ACCUM_W / 2);

      // Saturate to output width
      out_q = saturate(sum_q);
      out_i = saturate(sum_i);

      return {out_i, out_q};
    endfunction

    //-----------------------------------------------------------------------
    // integrate_packet: process a queue of samples, return queue of outputs
    //-----------------------------------------------------------------------
    function void integrate_packet(
      input  sample_t samples_in[$],
      output sample_t samples_out[$]
    );
      samples_out = {};
      foreach (samples_in[i]) begin
        samples_out.push_back(integrate(samples_in[i]));
      end
    endfunction

    //-----------------------------------------------------------------------
    // Accessors for full-precision accumulator state (debug / inspection)
    //-----------------------------------------------------------------------
    function longint get_sum_i();
      return sum_i;
    endfunction

    function longint get_sum_q();
      return sum_q;
    endfunction

  endclass

  //---------------------------------------------------------------------------
  // Bit-true simulation model of an N-th order CIC integrator.
  //
  // Cascades ORDER independent cic_integrator_stage_model stages, preserving
  // accumulator state across successive process_packet() calls (matching
  // the RTL which maintains state between AXI-Stream packets). Call reset()
  // to reset all stages, mirroring the DUT's rst/clr behavior.
  //
  // Parameters:
  //   SAMP_W  : Sample bit width (I+Q packed, each SAMP_W/2 bits). Must
  //             match the DUT's SAMP_W (= ACCUM_W at the DUT input port).
  //   ACCUM_W : Accumulator width used for wrapping arithmetic. Defaults
  //             to SAMP_W (no internal clip between stages).
  //   ORDER   : Number of cascaded integrator stages.
  //---------------------------------------------------------------------------
  class cic_integrator_model #(
    int SAMP_W  = 32,
    int ACCUM_W = SAMP_W,
    int ORDER   = 4
  );

    //-----------------------------------------------------------------------
    // Type definitions
    //-----------------------------------------------------------------------
    typedef cic_utils#(
      .SAMP_W(SAMP_W)
    ) util_c;

    typedef util_c::sample_t sample_t;

    //-----------------------------------------------------------------------
    // Internal state: ORDER persistent integrator stages
    //-----------------------------------------------------------------------
    cic_integrator_stage_model #(
      .SAMP_W (SAMP_W),
      .ACCUM_W(ACCUM_W)
    ) stages[ORDER];

    //-----------------------------------------------------------------------
    // Constructor: allocate and initialise all stage models
    //-----------------------------------------------------------------------
    function new();
      for (int i = 0; i < ORDER; i++) begin
        stages[i] = new();
      end
    endfunction

    //-----------------------------------------------------------------------
    // reset: reset all stage accumulators (mirrors rst/clr on the DUT)
    //-----------------------------------------------------------------------
    function void reset();
      for (int i = 0; i < ORDER; i++) begin
        stages[i].reset();
      end
    endfunction

    //-----------------------------------------------------------------------
    // process_packet: chain samples through all ORDER integrator stages.
    //
    // The output of stage i is fed as the input to stage i+1. Accumulator
    // state persists across calls, matching inter-packet RTL behaviour.
    //
    //   samples_in  : flat queue of input samples.
    //   samples_out : flat queue of output samples after all ORDER stages.
    //-----------------------------------------------------------------------
    function void process_packet(
      input  sample_t samples_in[$],
      output sample_t samples_out[$]
    );
      sample_t stage_in[$], stage_out[$];
      stage_in = samples_in;
      for (int i = 0; i < ORDER; i++) begin
        stages[i].integrate_packet(stage_in, stage_out);
        stage_in = stage_out;
      end
      samples_out = stage_in;
    endfunction

  endclass : cic_integrator_model

endpackage

`default_nettype wire
