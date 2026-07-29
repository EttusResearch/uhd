//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_decimator
//
// Description:
//
//    A N-to-1 CIC decimator stage that produces one output per N input samples.
//    This module is intended for usage in the multisample CIC filter. This
//    module supports changing the decimation factor at runtime, but changing
//    the decimation factor while the module is actively processing data may
//    cause unexpected results and corrupted output data.
//
//     The decimation process is as follows:
//     1. On each clock cycle, we receive SPC input samples (one word). A word
//        counter tracks the position within the current decimation group
//        (0 to decimation_factor-1).
//     2. Each output slot independently knows which word position and sample
//        within that word contains its needed sample. When the matching word
//        arrives, the sample is grabbed directly into the output register.
//     3. Once all decimation_factor words have been received (the output word
//        is fully assembled), compute_output fires and the output FIFO
//        captures the completed output word on the next clock cycle.
//
// Parameters:
//    SAMP_W        : Bit width of each input/output samples.
//    SPC           : Number of samples processed per clock cycle.
//    R_MAX         : Maximum decimation factor supported. Used to size internal
//                    registers.
//
// NOTE:
//    - This module always expects to receive enough input words to produce an
//      output word. This greatly reduces the complexity of this module as we
//      do not have to worry about not having enough input words to produce an
//      output when the end of a burst is reached. The reason we can make this
//      assumption is because this module is intended to be used in the
//      multisample CIC filter, which includes the axi_rate_change module at
//      the beginning of the chain to ensure that groups of words (SPC
//      samples) are only passed to the chain in increments of SPC. The
//      axi_rate_change module also ensures that the end of a burst is aligned
//      with the end of a group, discarding words of a partial group at the
//      end of a burst if necessary. This allows this downstream module to
//      operate without needing to worry about partial groups of input samples.
//    - The decimation factor can be changed at runtime by pulsing the
//      decimation_factor_changed signal, but will result in incorrect output
//      data if the decimation factor is changed in the middle of processing a
//      group of input words. It is the responsibility of the user to only
//      change the decimation factor when the module is idle (not processing
//      input words).

`default_nettype none

module cic_decimator #(
  parameter int SAMP_W        = 32,
  parameter int SPC           = 4,
  parameter int R_MAX         = 255
) (
  input wire                           clk,
  input wire                           rst,
  // Clear decimator
  input wire                           clr,
  // Data in
  AxiStreamIf.slave                    data_in,
  // Data out
  AxiStreamIf.master                   data_out,
  // Configuration interface
  input wire [($clog2(R_MAX+1) - 1):0] decim_factor,
  input wire                           decim_changed
);

  import cic_utils_pkg::*;

  //---------------------------------------------------------------------------
  // Parameter declarations and Type definitions
  //---------------------------------------------------------------------------
  // Module constants
  localparam int NSPC_LOG2 = (SPC == 1) ? 1 : $clog2(SPC);
  localparam int R_MAX_LOG2 = (R_MAX == 1) ? 1 : $clog2(R_MAX);
  // flat_idx needs to hold (SPC-1) * (R_MAX-1), so use R_MAX_LOG2 + NSPC_LOG2 bits.
  localparam int FLAT_IDX_W = R_MAX_LOG2 + NSPC_LOG2;

  typedef cic_utils#(
    .SPC   (SPC),
    .SAMP_W(SAMP_W)
  ) util_c;

  typedef util_c::sample_t sample_t;// Single sample packed logic type (SAMP_W bits).
  typedef util_c::word_t   word_t;  // Word type, packed array of SPC sample_t.

  //---------------------------------------------------------------------------
  // Synthesis-time checks
  //---------------------------------------------------------------------------
  // Check that SPC is a power of 2 and at least 1
  if (SPC < 1 || (SPC & (SPC - 1)) != 0) begin
    $error(
      {"cic_decimator: SPC must be a power of 2 and at least 1.",
      " Got SPC=%0d"},
      SPC
    );
  end
  // Only support R_MAX up to 255 to keep internal registers reasonably sized (R_MAX_LOG2 <= 8).
  if (R_MAX > 255 || R_MAX < 1) begin
    $error(
      {"cic_decimator: This module only supports maximum decimation factors between 1 and 255,",
      " but R_MAX=%0d was specified."},
      R_MAX
    );
  end

  //----------------------------------------------------------------------------
  // Internal signal declarations
  //----------------------------------------------------------------------------

  // Output word register: assembled incrementally as input words arrive.
  // No input buffer needed: each output slot grabs its sample directly from
  // the current input word when that word's position matches the slot's need.
  word_t output_word;

  // Word counter: counts input words received in the current decimation group.
  logic [R_MAX_LOG2-1:0] word_count;

  // Control signals
  logic compute_output_pre;  // Combinational: same cycle as the Rth input_xfer.
  logic compute_output;      // Registered: 1 cycle delayed assembled output_word.

  // tlast accumulation across the decimation group
  logic tlast_accum;

  // Output FIFO signals
  logic [SPC*SAMP_W-1:0] packed_output;

  // Input handshake
  logic input_xfer;
  assign input_xfer = data_in.tvalid & data_in.tready;

  //----------------------------------------------------------------------------
  // Internal logic
  //----------------------------------------------------------------------------
  // Unpack input tdata into a word_t
  word_t input_word;
  for (genvar i = 0; i < SPC; i++) begin : gen_unpack_input
    assign input_word[i] = data_in.tdata[SAMP_W*i+:SAMP_W];
  end

  //----------------------------------------------------------------------------
  // Phase 1: Word counter
  //----------------------------------------------------------------------------
  // Word counter: counts from 0 to decimation_factor-1
  always_ff @(posedge clk) begin
    if (rst || clr || decim_changed) begin
      word_count <= '0;
    end else if (input_xfer) begin
      if (word_count == decim_factor - 1) begin
        word_count <= '0;
      end else begin
        word_count <= word_count + 1;
      end
    end
  end

  // compute_output_pre indicates when word_count is at the last word of the current
  // decimation group. Combined with data_in.tvalid/tready, this drives compute_output
  // (1-cycle delayed) once the last word is accepted.
  assign compute_output_pre = (word_count == decim_factor - R_MAX_LOG2'(1));

  always_ff @(posedge clk) begin
    if (rst || clr) begin
      compute_output <= 1'b0;
    end else if (data_in.tready) begin
      compute_output <= compute_output_pre & data_in.tvalid;
    end
  end

  //----------------------------------------------------------------------------
  // Phase 2: Output selection logic
  //----------------------------------------------------------------------------
  // SPC parallel processing lanes, one per sample, each containing the logic
  // to select the correct sample from the input stream for that output slot.
  // Each lane calculates which input word contains the sample it needs, and at
  // what position within that word the sample is located.
  // When that word arrives, the sample is copied directly from the input
  // into the output register.
  //
  // For output sample i (0..SPC-1):
  //   flat_idx    = i * decimation_factor   (flat sample index in the group)
  //   needed_word = flat_idx / SPC          (which word in the group holds it)
  //   needed_samp = flat_idx % SPC          (which sample within that word)
  //
  // When word_count == needed_word and input_xfer, grab input_word[needed_samp].
  //
  // Example (SPC=4, R=3):
  //   output[0]: flat=0,  needed_word=0, needed_samp=0  -> grab from 1st word
  //   output[1]: flat=3,  needed_word=0, needed_samp=3  -> grab from 1st word
  //   output[2]: flat=6,  needed_word=1, needed_samp=2  -> grab from 2nd word
  //   output[3]: flat=9,  needed_word=2, needed_samp=1  -> grab from 3rd word
  //
  // This logic allows each output slot to grab its needed sample directly from
  // the input stream with no additional buffering.
  for (genvar i = 0; i < SPC; i++) begin : gen_output_select
    // Index computation (combinational)
    logic [FLAT_IDX_W-1:0] flat_idx = '0;
    logic [R_MAX_LOG2-1:0] needed_word;
    logic [ NSPC_LOG2-1:0] needed_samp;

    assign needed_word = flat_idx[NSPC_LOG2+:R_MAX_LOG2];
    assign needed_samp = flat_idx[NSPC_LOG2-1:0];

    // Grab the sample from the current input word when its position matches
    always_ff @(posedge clk) begin
      if (input_xfer && (word_count == needed_word)) begin
        output_word[i] <= input_word[needed_samp];
      end
      // Register flat_idx to ease timing. Decimation factor expected to be stable
      // for the duration of sample processing, and is expected to only change when
      // module is idle, so this should not cause issues.
      flat_idx <= FLAT_IDX_W'(i) * FLAT_IDX_W'(decim_factor);
    end
  end

  //----------------------------------------------------------------------------
  // Phase 3: tlast accumulation
  //----------------------------------------------------------------------------
  // OR-accumulate tlast across the decimation group.
  // At the compute_output cycle (1 cycle after last input_xfer), tlast_accum
  // holds the OR of all data_in.tlast values seen during the group. The FIFO
  // captures this value directly alongside packed_output on compute_output.
  //
  // Pipeline: input_xfer (accumulate) -> compute_output (FIFO captures tlast_accum)

  always_ff @(posedge clk) begin
    if (rst || clr || decim_changed) begin
      tlast_accum <= 1'b0;
    end else if (input_xfer) begin
      if (word_count == 0) begin
        tlast_accum <= data_in.tlast;
      end else begin
        tlast_accum <= tlast_accum | data_in.tlast;
      end
    end
  end

  //----------------------------------------------------------------------------
  // Phase 4: Pack output and output FIFO
  //----------------------------------------------------------------------------

  assign packed_output = output_word;

  // Output FIFO: buffers decimated output for AXI-Stream backpressure handling.
  // compute_output is used directly as FIFO valid, at this point output_word
  // and tlast_accum both contain the correct values for the completed group.
  axi_fifo #(
    .WIDTH(SPC * SAMP_W + 1),
    .SIZE (1)
  ) output_buffer (
    .clk     (clk),
    .reset   (rst),
    .clear   (clr),
    .i_tdata ({tlast_accum, packed_output}),
    .i_tvalid(compute_output),
    .i_tready(data_in.tready),
    .o_tdata ({data_out.tlast, data_out.tdata}),
    .o_tvalid(data_out.tvalid),
    .o_tready(data_out.tready),
    .space   (),
    .occupied()
  );

endmodule

`default_nettype wire
