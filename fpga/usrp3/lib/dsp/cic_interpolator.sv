//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_interpolator
//
// Description:
//
//   A 1-to-N CIC interpolator stage that produces N output words for every
//   input word. This interpolation factor can be changed at runtime.
//
//   The interpolation factor can be safely updated between packets while the
//   module is idle, or after reset is asserted and before data arrives.
//   Updating the interpolation factor takes a few clock cycles to complete
//   internally and no data should be input during that time. Updating the
//   interpolation factor while the module is processing data results in
//   undefined output.
//
//   The interpolation factor is changed by strobing the interp_changed
//   signal with the new rate input on interp_factor.
//
// Parameters:
//
//   SAMP_W : Bit width of each input/output sample.
//   SPC    : Number of samples processed per clock cycle.
//   R_MAX  : Maximum interpolation factor supported. Used to size internal
//            registers.
//
// Signals:
//
//   clk            : Clock
//   rst            : Reset
//   data_in        : AXI-Stream data input
//   data_out       : AXI-Stream data output
//   interp_factor  : Interpolation factor to load
//   interp_changed : Assert for one clock cycle to load a new interpolation
//                    factor from interp_factor.
//

`default_nettype none


module cic_interpolator #(
  parameter int SAMP_W = 32,
  parameter int SPC    = 4,
  parameter int R_MAX  = 255,

  localparam int FACTOR_W = $clog2(R_MAX + 1)
) (
  input wire clk,
  input wire rst,

  AxiStreamIf.slave  data_in,
  AxiStreamIf.master data_out,

  input wire [FACTOR_W-1:0] interp_factor,
  input wire                interp_changed
);
  `include "usrp_utils.svh"

  localparam int WORD_IDX_W = `MAX(1, $clog2(R_MAX));
  localparam int SAMP_IDX_W = `MAX(1, $clog2(SPC));


  //---------------------------------------------------------------------------
  // Assertions
  //---------------------------------------------------------------------------

  if (SPC < 1 || SPC != 2**$clog2(SPC)) begin : gen_spc_check
    $error(
      "cic_interpolator: SPC must be a power of 2 and at least 1. Got SPC=%0d",
      SPC
    );
  end

  if (R_MAX <= 1) begin : gen_rate_check
    $error("cic_interpolator: R_MAX must be at least 2. Got R_MAX=%0d", R_MAX);
  end


  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  // Create a version of the output bus we can index
  logic                       gen_tvalid;
  logic                       gen_tready;
  logic [SPC-1:0][SAMP_W-1:0] gen_tdata;
  logic                       gen_tlast;

  // Input register
  logic                       input_busy = 1'b0;
  logic                       input_last;
  logic [SPC-1:0][SAMP_W-1:0] input_samps;


  //---------------------------------------------------------------------------
  // Interpolation Factor
  //---------------------------------------------------------------------------

  logic [FACTOR_W-1:0] factor        = 1;  // Current interpolation factor
  logic [FACTOR_W-1:0] factor_m2     = 0;  // Current factor minus 2
  logic                factor_is_one = 1;  // High when factor == 1

  always_ff @(posedge clk) begin
    if (rst) begin
      factor        <= 1;
      factor_m2     <= 0;
      factor_is_one <= 1'b1;
    end else begin
      if (interp_changed) begin
        factor <= interp_factor;
      end
      factor_m2     <= factor - 2;
      factor_is_one <= (factor == 1);
    end
  end


  //---------------------------------------------------------------------------
  // Sample and Word Indices
  //---------------------------------------------------------------------------
  //
  // Determine the output word and sample indices into which each input sample
  // go after interpolation.
  //
  //---------------------------------------------------------------------------

  localparam int ACCUM_W  = WORD_IDX_W + SAMP_IDX_W;

  // An array of indices that stores the mapping from input sample to output
  // sample.
  logic [ACCUM_W-1:0] accum [SPC];

  // This array contains the output word index for each input sample
  logic [WORD_IDX_W-1:0] word_idx [SPC];
  // This array contains the output sample index for each input sample
  logic [SAMP_IDX_W-1:0] samp_idx [SPC];

  always_ff @(posedge clk) begin
    for (int idx = 0; idx < SPC; idx++) begin
      accum[idx] <= idx * factor;
    end
  end

  // Extract from accumulator the word and sample indices.
  always_comb begin
    for (int idx = 0; idx < SPC; idx++) begin
      word_idx[idx] = accum[idx][SAMP_IDX_W +: WORD_IDX_W];
      samp_idx[idx] = accum[idx][         0 +: SAMP_IDX_W];
    end
  end


  //---------------------------------------------------------------------------
  // Word Counter
  //---------------------------------------------------------------------------
  //
  // For each one input word we output factor words. This counter tracks the
  // number of output words that have been output for each input word.
  //
  //---------------------------------------------------------------------------

  logic [WORD_IDX_W-1:0] word_cnt;
  logic                  word_cnt_last;  // Assert on last word of output group

  always_ff @(posedge clk) begin
    if (rst) begin
      word_cnt      <= '0;
      word_cnt_last <= 1'b1;
    end else if (!input_busy) begin
      word_cnt      <= '0;
      word_cnt_last <= factor_is_one;
    end else if (gen_tvalid && gen_tready) begin
      if (word_cnt_last) begin
        word_cnt      <= '0;
        word_cnt_last <= factor_is_one;
      end else begin
        word_cnt      <= word_cnt + 1;
        word_cnt_last <= (word_cnt == factor_m2);
      end
    end
  end


  //---------------------------------------------------------------------------
  // Input Word Logic
  //---------------------------------------------------------------------------
  //
  // Captures the input word when accepted and holds it until all corresponding
  // output words are generated.
  //
  // The input_busy flag tracks when we're holding a loaded input word. It
  // starts low after reset so the first word is accepted immediately. It sets
  // when a word is accepted and clears on the last output transfer for the
  // current input word.
  //
  //---------------------------------------------------------------------------

  // Assert data_in.tready when not busy, or combinationally on the cycle the
  // last output word of the current interpolation group transfers.
  assign data_in.tready = !input_busy ||
    (gen_tvalid && gen_tready && word_cnt_last);

  // Output is valid whenever we have a loaded input word to work from.
  assign gen_tvalid = input_busy;

  // Assert tlast on the last output word of the group when the input was last.
  assign gen_tlast = input_last && word_cnt_last;

  always_ff @(posedge clk) begin
    if (rst) begin
      input_busy  <= 1'b0;
      input_last  <= 'X;
      input_samps <= 'X;
    end else begin
      if (data_in.tvalid && data_in.tready) begin
        // Accepted a new input word; busy until end of interpolation group.
        input_busy  <= 1'b1;
        input_last  <= data_in.tlast;
        input_samps <= data_in.tdata;
      end else if (gen_tvalid && gen_tready && word_cnt_last) begin
        // Last output word transferred with no new input; no longer busy.
        input_busy <= 1'b0;
      end
    end
  end



  //---------------------------------------------------------------------------
  // Output Word Generation
  //---------------------------------------------------------------------------
  //
  // For each output word, we look at the current word_cnt and use that to
  // determine which input samples, if any, should be output for each sample
  // position. We pull the sample values from the stored input_samps register.
  // We decide where those input samples go on the output by looking up the
  // word_idx and samp_idx for that sample. All other sample positions are set
  // to 0.
  //
  //---------------------------------------------------------------------------

  // Intermediate results to pipeline the array indexing
  // Stage 1: Determine which input samples match the current word count
  logic [SPC-1:0] word_idx_match;

  always_ff @(posedge clk) begin
    if (gen_tready) begin
      for (int src_samp = 0; src_samp < SPC; src_samp++) begin
        word_idx_match[src_samp] <= (word_idx[src_samp] == word_cnt);
      end
    end
  end

  // Stage 2: Generate the sample index for each output sample position
  logic [SAMP_IDX_W-1:0] out_mux_select [SPC];
  logic [SPC-1:0]        word_idx_en;
  always_ff @(posedge clk) begin
    automatic logic [SAMP_IDX_W-1:0] next_out_mux_select [SPC] = '{default: '0};
    automatic logic [SPC-1:0]        next_word_idx_en = '0;

    for (int src_samp = 0; src_samp < SPC; src_samp++) begin
      if (word_idx_match[src_samp]) begin
        next_out_mux_select[samp_idx[src_samp]] |= SAMP_IDX_W'(src_samp);
        next_word_idx_en[samp_idx[src_samp]] = '1;
      end
    end

    if (gen_tready) begin
      out_mux_select <= next_out_mux_select;
      word_idx_en    <= next_word_idx_en;
    end
  end

  // create shift register for tdata, tvalid and tlast to pipeline them to the output
  // FIFO
  logic [1:0] tvalid_pipe;
  logic [1:0] tlast_pipe;
  logic [1:0][SPC-1:0][SAMP_W-1:0] tdata_pipe;
  always_ff @(posedge clk) begin
    if (rst) begin
      tvalid_pipe <= 2'b00;
      tlast_pipe  <= 'X;
      tdata_pipe  <= 'X;
    end else if (gen_tready) begin
      tvalid_pipe <= {tvalid_pipe[0], gen_tvalid};
      tlast_pipe  <= {tlast_pipe[0],  gen_tlast };
      tdata_pipe  <= {tdata_pipe[0],  input_samps};
    end
  end


  always_comb begin
    // All samples are set to zero by default
    gen_tdata = '0;

    // Iterate over each sample and see if the word count and sample position
    // match what we precomputed in samp_idx and word_idx.
    for (int out_samp = 0; out_samp < SPC; out_samp++) begin
      if (word_idx_en[out_samp]) begin
        gen_tdata[out_samp] = tdata_pipe[1][out_mux_select[out_samp]];
      end
    end
  end

  //---------------------------------------------------------------------------
  // Output Register
  //---------------------------------------------------------------------------
  axi_fifo #(
    .WIDTH (SPC*SAMP_W + 1  ),
    .SIZE  (1               )
  ) axi_fifo_i (
    .clk      (clk                   ),
    .reset    (rst                   ),
    .clear    (1'b0                  ),
    .i_tdata  ({tlast_pipe[1], gen_tdata}),
    .i_tvalid (tvalid_pipe[1]        ),
    .i_tready (gen_tready            ),
    .o_tdata  ({data_out.tlast, data_out.tdata}),
    .o_tvalid (data_out.tvalid       ),
    .o_tready (data_out.tready       ),
    .space    (                      ),
    .occupied (                      )
  );

endmodule : cic_interpolator

`default_nettype wire
