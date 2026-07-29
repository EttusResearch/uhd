// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  axis_hb_test_pkg.sv
//
// Description:
//   Test package for AXI halfband FIR filter testbenches — covers both the
//   decimation filter (axis_hb_decim_fir) and the interpolation filter
//   (axis_hb_intp_fir).
//
//   Classes provided:
//     FilterHBTestUtils  — direction-agnostic packet-gen/serialisation helper
//                          (set SPC_OUT = SPC_IN/2 for decimation,
//                           SPC_IN*2 for interpolation)
//
//     AxiFirHBBaseModel  — abstract base: coefficient loading, extract_samples,
//                          preload_filter, convolve (protected constructor)
//     AxiFirHBDecimModel — decimation reference model, extends AxiFirHBBaseModel
//     AxiFirHBIntpModel  — interpolation reference model, extends AxiFirHBBaseModel
//
//     AxiFirHBCascadeDecimModel — decimation cascade reference model
//     AxiFirHBCascadeIntpModel  — interpolation cascade reference model
//

package axis_hb_test_pkg;

  `include "test_exec.svh"

  import PkgAxiStreamBfm::*;
  import axis_hb_utils_pkg::*;
  import PkgTestExec::*;
  import PkgRandom::*;

  // --------------------------------------------------------------------------
  // FilterHBTestUtils
  //
  // Direction-agnostic packet-generation and serialisation helpers for HBF TBs.
  // The caller sets SPC_OUT explicitly:
  //   SPC_IN/2 for decimation (default, backward-compatible with decimation TBs),
  //   SPC_IN*2 for interpolation.
  // --------------------------------------------------------------------------
  class FilterHBTestUtils #(
    parameter int SAMP_W  = 32,         // Width of one complex I/Q sample (I + Q)
    parameter int SPC_IN  = 4,          // Input samples per clock cycle
    parameter int SPC_OUT = SPC_IN / 2  // Output samples per clock cycle
  );

    //-------------------------------------------------------------------------
    // Type aliases
    //-------------------------------------------------------------------------
    typedef AxiStreamPacket#(.DATA_WIDTH(SPC_IN  * SAMP_W)) axis_input_pkt_t;
    typedef AxiStreamPacket#(.DATA_WIDTH(SPC_OUT * SAMP_W)) axis_output_pkt_t;
    typedef AxiStreamPacket#(.DATA_WIDTH(SAMP_W))           axis_pkt_single_sample_t;

    typedef mailbox#(axis_pkt_single_sample_t) axis_pkt_single_sample_mbox_t;
    typedef axis_input_pkt_t                   axis_pkt_queue_t[$];
    typedef mailbox#(axis_input_pkt_t)         axis_input_pkt_mbox_t;
    typedef mailbox#(axis_output_pkt_t)        axis_output_pkt_mbox_t;

    //-------------------------------------------------------------------------
    // Constants
    //-------------------------------------------------------------------------
    // Maximum positive value for I/Q samples, used for impulse generation
    localparam logic [SAMP_W/2-1:0] MAXIMUM_I_Q_VALUE = {1'b0, {(SAMP_W/2-1){1'b1}}};

    //-------------------------------------------------------------------------
    // Packet generators
    //-------------------------------------------------------------------------

    // Generates a random packet with len in [min_len, max_len] input words.
    static function axis_input_pkt_t generate_random_packet(int min_len, int max_len);
      axis_input_pkt_t pkt = new();
      int len;
      `ASSERT_ERROR(max_len >= min_len,
        "Maximum packet length must be >= minimum packet length!");
      len = $urandom_range(min_len, max_len);
      repeat (len) begin
        pkt.data.push_back(Rand#(SPC_IN * SAMP_W)::rand_bit());
      end
      return pkt;
    endfunction : generate_random_packet

    // Generates an all-zero packet of exactly `len` input words.
    static function axis_input_pkt_t generate_zero_packet(int len);
      axis_input_pkt_t pkt = new();
      repeat (len) begin
        pkt.data.push_back('0);
      end
      return pkt;
    endfunction : generate_zero_packet

    // Impulse packet: first sample of first word = max positive I/Q; rest = 0.
    // len_words should include enough trailing zeros to flush the FIR pipeline.
    static function axis_input_pkt_t generate_impulse_packet(int len_words);
      axis_input_pkt_t pkt = new();
      // First sample is impulse (max positive for both I and Q), rest = 0.
      // Occupies lane 0 of the first word; remaining lanes and all other words are zero.
      pkt.data.push_back({2{MAXIMUM_I_Q_VALUE}});
      repeat (len_words - 1) begin
        pkt.data.push_back('0);
      end
      return pkt;
    endfunction : generate_impulse_packet

    // Generates a queue of a random number of packets with random data.
    static function axis_pkt_queue_t generate_random_test_data(
      int max_pkts, int min_len, int max_len, bit impulse = 0
    );
      automatic axis_pkt_queue_t pkt_queue;
      int num_pkts;
      num_pkts = $urandom_range(1, max_pkts);
      repeat (num_pkts) begin
        // for testing purposes, generate impulse packet.
        if (impulse) begin
          pkt_queue.push_back(generate_impulse_packet(max_len));
        end else begin
          pkt_queue.push_back(generate_random_packet(min_len, max_len));
        end
      end
      return pkt_queue;
    endfunction : generate_random_test_data

    //-------------------------------------------------------------------------
    // Serialisation helpers
    //-------------------------------------------------------------------------

    // Drains all packets from input_mbox and pushes each I/Q sample as a
    // single-sample packet into serialized_pkts.
    static task collect_and_serialize_packets(
      input  axis_input_pkt_mbox_t          input_mbox,
      inout  axis_pkt_single_sample_mbox_t  serialized_pkts
    );
      axis_input_pkt_t in_pkt;
      logic [SPC_IN*SAMP_W-1:0] word;

      // Collect all packets from the mailbox
      while (input_mbox.try_get(in_pkt)) begin
        // Process each word in the packet
        foreach (in_pkt.data[word_idx]) begin
          word = in_pkt.data[word_idx];
          // Extract SPC_IN samples from each word and add them individually
          for (int idx = 0; idx < SPC_IN; idx++) begin
            axis_pkt_single_sample_t sample_pkt = new();
            sample_pkt.data.push_back(word[idx*SAMP_W+:SAMP_W]);
            // Push each sample as its own packet into the serialized packets mailbox
            serialized_pkts.put(sample_pkt);
          end
        end
      end
    endtask : collect_and_serialize_packets

    // Unpacks one SPC_OUT-wide output packet into a flat single-sample packet.
    // Respects tkeep so partially-valid words are handled correctly.
    static function axis_pkt_single_sample_t serialize_packet(
      input axis_output_pkt_t in_pkt
    );
      automatic axis_pkt_single_sample_t out_pkt = new();
      logic [SPC_OUT*SAMP_W-1:0] word;
      foreach (in_pkt.data[word_idx]) begin
        word = in_pkt.data[word_idx];
        for (int spc = 0; spc < SPC_OUT; spc++) begin
          out_pkt.data.push_back(word[spc*SAMP_W+:SAMP_W]);
          if (in_pkt.keep[word_idx][spc] == 1'b1) begin
            out_pkt.keep.push_back('1);
          end else begin
            out_pkt.keep.push_back('0);
          end
        end
      end
      return out_pkt;
    endfunction : serialize_packet

  endclass : FilterHBTestUtils

  // --------------------------------------------------------------------------
  // AxiFirHBBaseModel
  //
  // Abstract base class for halfband FIR reference models. The constructor is
  // declared protected to prevent direct instantiation — use
  // AxiFirHBDecimModel (decimation) or AxiFirHBIntpModel (interpolation).
  //
  // Provides shared infrastructure: coefficient loading, extract_samples,
  // preload_filter, and convolve (FIR convolution + round + saturate).
  // --------------------------------------------------------------------------
  class AxiFirHBBaseModel #(
    parameter int SAMP_W                = 48,
    parameter int NUM_COEFFS            = 47,
    parameter int ACCUM_FRACTIONAL_BITS = COEFF_FRACTIONAL_BITS + COEFF_GAIN_BITS
  );

    //-------------------------------------------------------------------------
    // Constants
    //-------------------------------------------------------------------------
    // Accumulator headroom: sample bits + coefficient bits + log2(taps) - 1.
    localparam int ACCUM_WIDTH           = SAMP_W/2 + COEFF_WIDTH + $clog2(NUM_COEFFS) - 1;

    //-------------------------------------------------------------------------
    // Type aliases
    //-------------------------------------------------------------------------
    typedef AxiStreamPacket#(.DATA_WIDTH(SAMP_W)) axis_pkt_t;
    typedef mailbox#(axis_pkt_t)           axis_pkt_mbox_t;
    typedef logic signed [SAMP_W/2-1:0]    sample_component_t;
    typedef sample_component_t             sample_array_t[$];
    typedef logic signed [COEFF_WIDTH-1:0] coeff_t;
    typedef coeff_t                        coeff_array_t[NUM_COEFFS];

    //-------------------------------------------------------------------------
    // Coefficient storage
    //-------------------------------------------------------------------------
    protected coeff_array_t coeffs;

    //-------------------------------------------------------------------------
    // Constructor -- protected to prevent direct instantiation.
    // Selects HB47 or HB63 coefficient set based on NUM_COEFFS.
    //-------------------------------------------------------------------------
    protected function new();
      if (NUM_COEFFS == HB47_NUM_COEFFS) begin
        for (int i = 0; i < NUM_COEFFS; i++) coeffs[i] = HB47_COEFF_VEC[i];
      end else if (NUM_COEFFS == HB63_NUM_COEFFS) begin
        for (int i = 0; i < NUM_COEFFS; i++) coeffs[i] = HB63_COEFF_VEC[i];
      end else begin
        $error("AxiFirHBBaseModel: unsupported NUM_COEFFS = %0d", NUM_COEFFS);
      end
    endfunction : new

    //-------------------------------------------------------------------------
    // extract_samples -- drain the input mailbox into scalar I and Q arrays.
    // Convention: MSB half = I, LSB half = Q (RFNoC sample format).
    //-------------------------------------------------------------------------
    protected task extract_samples(
      input  axis_pkt_mbox_t  in_mbox,
      output sample_array_t   in_samples_i,
      output sample_array_t   in_samples_q
    );
      axis_pkt_t pkt;
      while (in_mbox.try_get(pkt)) begin
        sample_component_t sample_i, sample_q;
        {sample_i, sample_q} = pkt.data[0];
        in_samples_i.push_back(sample_i);
        in_samples_q.push_back(sample_q);
      end
    endtask : extract_samples

    //-------------------------------------------------------------------------
    // preload_filter -- prepend (NUM_COEFFS - 1) zeros to model filter startup
    // transient (tap history = 0 after reset/clear).
    //-------------------------------------------------------------------------
    protected function void preload_filter(
      inout sample_array_t samples_i,
      inout sample_array_t samples_q
    );
      repeat (NUM_COEFFS - 1) begin
        samples_i.push_front('0);
        samples_q.push_front('0);
      end
    endfunction : preload_filter

    //-------------------------------------------------------------------------
    // convolve -- FIR convolution with round-half-up and saturation.
    //
    // Iterates over in_i/in_q starting at index (NUM_COEFFS - 1) to skip the
    // preload zone. All results are written into out_i/out_q.
    //-------------------------------------------------------------------------
    protected task convolve(
      input  sample_array_t in_i,
      input  sample_array_t in_q,
      output sample_array_t out_i,
      output sample_array_t out_q
    );
      out_i = {};
      out_q = {};
      for (int i = NUM_COEFFS - 1; i < int'(in_i.size()); i++) begin
        logic signed [ACCUM_WIDTH-1:0] acc_i = '0;
        logic signed [ACCUM_WIDTH-1:0] acc_q = '0;

        for (int j = 0; j < NUM_COEFFS; j++) begin
          acc_i = acc_i + in_i[i-j] * signed'(coeffs[j]);
          acc_q = acc_q + in_q[i-j] * signed'(coeffs[j]);
        end

        // Round-half-up: add 2^(ACCUM_FRACTIONAL_BITS-1) then arithmetic right-shift.
        acc_i = acc_i + (1 << (ACCUM_FRACTIONAL_BITS - 1));
        acc_q = acc_q + (1 << (ACCUM_FRACTIONAL_BITS - 1));
        acc_i = acc_i >>> ACCUM_FRACTIONAL_BITS;
        acc_q = acc_q >>> ACCUM_FRACTIONAL_BITS;

        // Saturate to SAMP_W/2 bits.
        if (acc_i > $signed({1'b0, {(SAMP_W/2 - 1){1'b1}}}))
          acc_i = $signed({1'b0, {(SAMP_W/2 - 1){1'b1}}});
        else if (acc_i < $signed({1'b1, {(SAMP_W/2 - 1){1'b0}}}))
          acc_i = $signed({1'b1, {(SAMP_W/2 - 1){1'b0}}});

        if (acc_q > $signed({1'b0, {(SAMP_W/2 - 1){1'b1}}}))
          acc_q = $signed({1'b0, {(SAMP_W/2 - 1){1'b1}}});
        else if (acc_q < $signed({1'b1, {(SAMP_W/2 - 1){1'b0}}}))
          acc_q = $signed({1'b1, {(SAMP_W/2 - 1){1'b0}}});

        out_i.push_back(acc_i[SAMP_W/2-1:0]);
        out_q.push_back(acc_q[SAMP_W/2-1:0]);
      end
    endtask : convolve

    //-------------------------------------------------------------------------
    // process_samples -- virtual entry point; must be overridden by derived classes.
    //-------------------------------------------------------------------------
    virtual task process_samples(
      input  axis_pkt_mbox_t  in_mbox,
      output axis_pkt_mbox_t  out_mbox
    );
      $error("AxiFirHBBaseModel::process_samples called directly — use a derived class.");
    endtask : process_samples

  endclass : AxiFirHBBaseModel


  // --------------------------------------------------------------------------
  // AxiFirHBDecimModel
  //
  // Halfband FIR x2 decimation reference model. Extends AxiFirHBBaseModel.
  //
  // process_samples flow:
  //   extract → preload → convolve → pack (even-indexed samples only, x2 decim)
  // --------------------------------------------------------------------------
  class AxiFirHBDecimModel #(
    parameter int SAMP_W     = 48,
    parameter int NUM_COEFFS = 47
  ) extends AxiFirHBBaseModel #(.SAMP_W(SAMP_W), .NUM_COEFFS(NUM_COEFFS));

    function new();
      super.new();
    endfunction : new

    // Pack convolution results into output mailbox, emitting even-indexed
    // samples only to implement x2 decimation.
    local task insert_samples(
      input  sample_array_t   out_samples_i,
      input  sample_array_t   out_samples_q,
      output axis_pkt_mbox_t  out_mbox
    );
      axis_pkt_t pkt;
      out_mbox = new();
      assert (out_samples_i.size() == out_samples_q.size()) else
        $error("AxiFirHBDecimModel.insert_samples: size mismatch I=%0d Q=%0d",
               out_samples_i.size(), out_samples_q.size());
      for (int i = 0; i < int'(out_samples_i.size()); i++) begin
        if (i % 2 == 1) continue;  // Skip odd samples (x2 decimation).
        pkt = new();
        pkt.data.push_back({out_samples_i[i], out_samples_q[i]});
        out_mbox.put(pkt);
      end
    endtask : insert_samples

    task process_samples(
      input  axis_pkt_mbox_t  in_mbox,
      output axis_pkt_mbox_t  out_mbox
    );
      sample_array_t in_i, in_q, out_i, out_q;
      extract_samples(in_mbox, in_i, in_q);
      preload_filter(in_i, in_q);
      convolve(in_i, in_q, out_i, out_q);
      insert_samples(out_i, out_q, out_mbox);
    endtask : process_samples

  endclass : AxiFirHBDecimModel


  // --------------------------------------------------------------------------
  // AxiFirHBIntpModel
  //
  // Halfband FIR x2 interpolation reference model. Extends AxiFirHBBaseModel.
  //
  // process_samples flow:
  //   extract → upsample → preload → convolve → pack (all 2N samples)
  // --------------------------------------------------------------------------
  class AxiFirHBIntpModel #(
    parameter int SAMP_W     = 48,
    parameter int NUM_COEFFS = 47
  ) extends AxiFirHBBaseModel #(
    .SAMP_W                (SAMP_W),
    .NUM_COEFFS            (NUM_COEFFS),
    .ACCUM_FRACTIONAL_BITS (COEFF_FRACTIONAL_BITS)
  );

    function new();
      super.new();
    endfunction : new

    // Zero-insert upsample: N samples → 2N samples [x0, 0, x1, 0, ...].
    local function void upsample(
      input  sample_array_t in_i,  input  sample_array_t in_q,
      output sample_array_t up_i,  output sample_array_t up_q
    );
      up_i = {};
      up_q = {};
      foreach (in_i[k]) begin
        up_i.push_back(in_i[k]);
        up_i.push_back('0);
        up_q.push_back(in_q[k]);
        up_q.push_back('0);
      end
    endfunction : upsample

    // Pack all computed I/Q results into output mailbox (no decimation skip).
    local task insert_samples(
      input  sample_array_t   out_samples_i,
      input  sample_array_t   out_samples_q,
      output axis_pkt_mbox_t  out_mbox
    );
      axis_pkt_t pkt;
      out_mbox = new();
      assert (out_samples_i.size() == out_samples_q.size()) else
        $error("AxiFirHBIntpModel.insert_samples: size mismatch I=%0d Q=%0d",
               out_samples_i.size(), out_samples_q.size());
      for (int i = 0; i < int'(out_samples_i.size()); i++) begin
        pkt = new();
        pkt.data.push_back({out_samples_i[i], out_samples_q[i]});
        out_mbox.put(pkt);
      end
    endtask : insert_samples

    task process_samples(
      input  axis_pkt_mbox_t  in_mbox,
      output axis_pkt_mbox_t  out_mbox
    );
      sample_array_t in_i, in_q, up_i, up_q, out_i, out_q;
      extract_samples(in_mbox, in_i, in_q);
      upsample(in_i, in_q, up_i, up_q);
      preload_filter(up_i, up_q);
      convolve(up_i, up_q, out_i, out_q);
      insert_samples(out_i, out_q, out_mbox);
    endtask : process_samples

  endclass : AxiFirHBIntpModel

  // ------------------------------------------------------------------------
  // AxiFirHBCascadeDecimModel
  //
  // Software reference model for the HB FIR decimation cascade.
  // The parameter interface mirrors the DUT (axis_hb_cascade_decim):
  //   NUM_HB         — number of cascade stages (0..HB_DECIM_MAX_NUM_HB)
  //   HB_NUM_COEFFS  — per-stage coefficient count (47 or 63); fixed-size array
  //                    of HB_DECIM_MAX_NUM_HB elements, only [0..NUM_HB-1] used.
  //
  // Stage indexing follows the DUT left-to-right convention:
  //   index 0:        leftmost/input stage
  //   index NUM_HB-1: rightmost/output stage
  //
  // Input/output are scalar-sample mailboxes (SPC=1 model domain).
  // ------------------------------------------------------------------------
  class AxiFirHBCascadeDecimModel #(
    parameter int SAMP_W = 32,
    parameter int NUM_HB = 3,
    parameter int HB_NUM_COEFFS [axis_hb_utils_pkg::HB_DECIM_MAX_NUM_HB]
                         = '{default: axis_hb_utils_pkg::HB47_NUM_COEFFS}
  );

    localparam int MAX_NUM_HB = axis_hb_utils_pkg::HB_DECIM_MAX_NUM_HB;

    typedef AxiStreamPacket#(.DATA_WIDTH(SAMP_W)) axis_pkt_t;
    typedef mailbox#(axis_pkt_t) axis_pkt_mbox_t;

    // Parallel handle arrays — one per supported coefficient count.
    // For stage i, exactly one of {hb47_stage[i], hb63_stage[i]} is non-null,
    // selected by HB_NUM_COEFFS[i].  SV cannot hold handles of different
    // parameterized types in a single array, so we maintain one array per type.
    AxiFirHBDecimModel #(.SAMP_W(SAMP_W), .NUM_COEFFS(HB47_NUM_COEFFS)) hb47_stage[MAX_NUM_HB];
    AxiFirHBDecimModel #(.SAMP_W(SAMP_W), .NUM_COEFFS(HB63_NUM_COEFFS)) hb63_stage[MAX_NUM_HB];

    function new();
      for (int i = 0; i < NUM_HB; i++) begin
        if (HB_NUM_COEFFS[i] == HB47_NUM_COEFFS) begin
          hb47_stage[i] = new();
        end else if (HB_NUM_COEFFS[i] == HB63_NUM_COEFFS) begin
          hb63_stage[i] = new();
        end else begin
          $error("AxiFirHBCascadeDecimModel: HB_NUM_COEFFS[%0d]=%0d unsupported (use %0d or %0d)",
                 i, HB_NUM_COEFFS[i], HB47_NUM_COEFFS, HB63_NUM_COEFFS);
        end
      end
    endfunction : new

    // Run one decimation stage; dispatches to the HB47 or HB63 model.
    local task run_stage(
      input  int             stage_idx,
      input  axis_pkt_mbox_t in_mbox,
      output axis_pkt_mbox_t out_mbox
    );
      if (HB_NUM_COEFFS[stage_idx] == HB47_NUM_COEFFS) begin
        hb47_stage[stage_idx].process_samples(in_mbox, out_mbox);
      end else if (HB_NUM_COEFFS[stage_idx] == HB63_NUM_COEFFS) begin
        hb63_stage[stage_idx].process_samples(in_mbox, out_mbox);
      end else begin
        $error("AxiFirHBCascadeDecimModel: HB_NUM_COEFFS[%0d]=%0d unsupported (use %0d or %0d)",
               stage_idx, HB_NUM_COEFFS[stage_idx], HB47_NUM_COEFFS, HB63_NUM_COEFFS);
        out_mbox = new();
      end
    endtask : run_stage

    // num_stages: number of active stages (0..NUM_HB). Pass -1 (default) to use
    // all NUM_HB stages. Note: SV does not allow class parameters as default
    // argument values, so -1 is used as a sentinel.
    task process_samples(
      input  axis_pkt_mbox_t in_mbox,
      output axis_pkt_mbox_t out_mbox,
      input  int             num_stages = -1
    );
      // Chain of mailboxes for maximum number of stages + 1 (final output).
      axis_pkt_mbox_t chain[NUM_HB+1];
      int effective_num_stages;
      effective_num_stages = (num_stages < 0) ? NUM_HB : num_stages;

      // Special case: num_stages=0 means bypass all stages, copy input to output.
      if (effective_num_stages == 0) begin
        axis_pkt_t pkt;
        out_mbox = new();
        while (in_mbox.try_get(pkt)) begin
          out_mbox.put(pkt.copy());
        end
        return;
      end

      // Active stages: index 0 .. (effective_num_stages - 1).
      // DUT activation: num_stages=1 activates only the leftmost stage 0;
      // num_stages=NUM_HB activates all stages left-to-right.
      chain[0] = in_mbox;
      for (int i = 0; i < effective_num_stages; i++) begin
        run_stage(i, chain[i], chain[i+1]);
      end
      out_mbox = chain[effective_num_stages];
    endtask : process_samples

  endclass : AxiFirHBCascadeDecimModel

  // ------------------------------------------------------------------------
  // AxiFirHBCascadeIntpModel
  //
  // Software reference model for the HB FIR interpolation cascade.
  // The parameter interface mirrors the DUT (axis_hb_cascade_intp):
  //   NUM_HB         — number of cascade stages (0..HB_INTP_MAX_NUM_HB)
  //   HB_NUM_COEFFS  — per-stage coefficient count (47 or 63); fixed-size array
  //                    of HB_INTP_MAX_NUM_HB elements, only [0..NUM_HB-1] used.
  //
  // Stage indexing follows the DUT left-to-right convention:
  //   index 0:        leftmost/input stage (activated last, for highest num_stages)
  //   index NUM_HB-1: rightmost/output stage (activated first, for num_stages=1)
  //
  // Input/output are scalar-sample mailboxes (SPC=1 model domain).
  // ------------------------------------------------------------------------
  class AxiFirHBCascadeIntpModel #(
    parameter int SAMP_W = 32,
    parameter int NUM_HB = 3,
    parameter int HB_NUM_COEFFS [axis_hb_utils_pkg::HB_INTP_MAX_NUM_HB]
                         = '{default: axis_hb_utils_pkg::HB47_NUM_COEFFS}
  );

    localparam int MAX_NUM_HB = axis_hb_utils_pkg::HB_INTP_MAX_NUM_HB;

    typedef AxiStreamPacket#(.DATA_WIDTH(SAMP_W)) axis_pkt_t;
    typedef mailbox#(axis_pkt_t) axis_pkt_mbox_t;

    // Parallel handle arrays — one per supported coefficient count.
    // For stage i, exactly one of {hb47_stage[i], hb63_stage[i]} is non-null,
    // selected by HB_NUM_COEFFS[i].  SV cannot hold handles of different
    // parameterized types in a single array, so we maintain one array per type.
    AxiFirHBIntpModel #(.SAMP_W(SAMP_W), .NUM_COEFFS(HB47_NUM_COEFFS)) hb47_stage[MAX_NUM_HB];
    AxiFirHBIntpModel #(.SAMP_W(SAMP_W), .NUM_COEFFS(HB63_NUM_COEFFS)) hb63_stage[MAX_NUM_HB];

    function new();
      for (int i = 0; i < NUM_HB; i++) begin
        if (HB_NUM_COEFFS[i] == HB47_NUM_COEFFS) begin
          hb47_stage[i] = new();
        end else if (HB_NUM_COEFFS[i] == HB63_NUM_COEFFS) begin
          hb63_stage[i] = new();
        end else begin
          $error("AxiFirHBCascadeIntpModel: HB_NUM_COEFFS[%0d]=%0d unsupported (use %0d or %0d)",
                 i, HB_NUM_COEFFS[i], HB47_NUM_COEFFS, HB63_NUM_COEFFS);
        end
      end
    endfunction : new

    // Run one interpolation stage; dispatches to the HB47 or HB63 model.
    local task run_stage(
      input int          stage_idx,
      input  axis_pkt_mbox_t in_mbox,
      output axis_pkt_mbox_t out_mbox
    );
      if (HB_NUM_COEFFS[stage_idx] == HB47_NUM_COEFFS) begin
        hb47_stage[stage_idx].process_samples(in_mbox, out_mbox);
      end else begin
        hb63_stage[stage_idx].process_samples(in_mbox, out_mbox);
      end
    endtask : run_stage

    // num_stages: number of active stages (0..NUM_HB). Pass -1 (default) to use
    // all NUM_HB stages. Note: SV does not allow class parameters as default
    // argument values, so -1 is used as a sentinel.
    task process_samples(
      input  axis_pkt_mbox_t in_mbox,
      output axis_pkt_mbox_t out_mbox,
      input  int             num_stages = -1
    );
      // chain[i] is the input mailbox for stage i; chain[NUM_HB] is the final output.
      axis_pkt_mbox_t chain[NUM_HB+1];
      int first_stage;
      int effective_num_stages;
      effective_num_stages = (num_stages < 0) ? NUM_HB : num_stages;

      if (effective_num_stages == 0) begin
        axis_pkt_t pkt;
        out_mbox = new();
        while (in_mbox.try_get(pkt)) begin
          out_mbox.put(pkt.copy());
        end
        return;
      end

      // Active stages: index (NUM_HB - effective_num_stages) .. (NUM_HB - 1), matching the
      // DUT activation: num_stages=1 activates only the rightmost stage (NUM_HB-1);
      // num_stages=NUM_HB activates all stages left-to-right.
      first_stage        = NUM_HB - effective_num_stages;
      chain[first_stage] = in_mbox;
      for (int i = first_stage; i < NUM_HB; i++) begin
        run_stage(i, chain[i], chain[i+1]);
      end
      out_mbox = chain[NUM_HB];
    endtask : process_samples

  endclass : AxiFirHBCascadeIntpModel

endpackage : axis_hb_test_pkg
