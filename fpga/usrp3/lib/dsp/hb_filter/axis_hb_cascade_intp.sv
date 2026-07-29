//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_hb_cascade_intp
//
// Description:
//   AXI4-Stream halfband interpolation cascade.
//   The data-path is organized as two readable cascades:
//   1) A width-converter chain that creates each stage's direct-path input width.
//   2) A halfband interpolation stage chain consisting of NUM_HB halfband FIR stages.
//
//   Example topology for SPC=8, NUM_HB=3:
//
//     s_axis_* (input)                                                                        +----\
//         |                                                                                   |     \
//       x8/-------------------------------------------------------------------------------/-->|      \
//         |                                                                               8   |      |
//     +-------+                                                                               |      |
//     | wc_2  |                                                                               |      |
//     +-------+                                                                               |      |
//         |                                                                                   |      |
//       x4/------------------------------------------------------+                            |      |
//         |                                                      |                            |      |
//     +-------+                                                  |                            |      |
//     | wc_1  |                                                  |                            |      |
//     +-------+                                                  |                            |      |
//         |                                                      |                            |      |--/-->
//       x2/---------------------------+                          |                            |      |  x8
//         |                           |                          |                            |      |
//     +-------+                       |                          |                            |      |
//     | wc_0  |                       |                          |                            |      |
//     +-------+                       |                          |                            |      |
//         |                           |                          |                            |      |
//       x1/                           |                          |                            |      |
//         |   +-----------------+     |  +-----------------+     |  +-----------------+       |      |
//         |   |      hbi_0      |     |  |      hbi_1      |     |  |      hbi_2      |       |      |
//         +-->| direct in (x1)  |     +->| direct in (x2)  |     +->| direct in (x4)  |       |      |
//          X->| prev in   (x1)  |--/---->| prev in   (x2)  |--/---->| prev in   (x4)  |---/-->|      /
//          0->| input_select    | x2  +->| input_select    | x4  +->| input_select    |  x8   |     /
//             +-----------------+     |  +-----------------+     |  +-----------------+       +----/
//                                     |                          |                              ^
//                                     |                          |                              |
//
//   input_select controls inside stages:
//     hbi_2.input_select = 1 (input from hbi_1 output)    when num_stages > 1
//     hbi_1.input_select = 1 (input from hbi_0 output)    when num_stages > 2
//     out_mux.select     = 1 (pick data from hbi cascade) when num_stages > 0
//
//   The example shows 3 stages hbi_0, hbi_1, hbi_2 with output SPC of 2, 4, 8 respectively.
//   The number offilter coefficients for each stage are defined by the HB_NUM_COEFFS array,
//   HB_NUM_COEFFS[0] → hbi_0, HB_NUM_COEFFS[1] → hbi_1, HB_NUM_COEFFS[2] → hbi_2.
//   The num_stages input activates a subset of the stages starting from the rightmost:
//     - num_stages=1 activates hbi_2
//     - num_stages=2 activates hbi_1 + hbi_2
//     - num_stages=3 activates hbi_0 + hbi_1 + hbi_2
//
// Parameters:
//   SAMP_W: Sample width in bits.
//   SPC:    Samples per cycle (input/output axis bus width in number of samples).
//   NUM_HB: Number of halfband stages to cascade.
//           Limited to 0...HB_INTP_MAX_NUM_HB.
//           (The implementation of this module is generic and can support any
//            number of stages. This is more a practical limitation.)
//           For NUM_HB = 0, the module simply bypasses the input to the output.
//   HB_NUM_COEFFS:  Optional array that allows to configure the number of coefficients in each
//                   halfband stage to be 47 or 63 (to use predefined filter coeff sets with 47
//                   or 63 coeffs). Default is 47 coeffs for all stages.
//                   Fixed size array with HB_INTP_MAX_NUM_HB elements, but only the first
//                   NUM_HB elements are used.
//                   HB_NUM_COEFFS[i] maps directly to gen_stages[i]: index 0 is the
//                   leftmost/input stage (hbi_0), index NUM_HB-1 is the rightmost/output
//                   stage (hbi_{NUM_HB-1}).
//                   (Sizing as HB_NUM_COEFFS [NUM_HB] won't work, because NUM_HB can be zero,
//                    and zero-sized arrays are not allowed in SystemVerilog.)
//   PRELOAD_ZEROES: If 1, the hb interpolation FIR is instantiated with zero preload enabled.
//                   This means the FIR will be flushed upon clear/reset such that it behaves
//                   as after cold power-up with all-zero state.
//                   This is the main use case since it allows deterministic behavior for
//                   processing of successive bursts but requires extra cycles after clear/reset
//                   before accepting new data.
//

`default_nettype none

module axis_hb_cascade_intp #(
  localparam int MAX_NUM_HB     = axis_hb_utils_pkg::HB_INTP_MAX_NUM_HB,
  parameter int  SAMP_W         = 32,
  parameter int  SPC            = 8,
  parameter int  NUM_HB         = 3,
  parameter int  HB_NUM_COEFFS [MAX_NUM_HB] = '{default: axis_hb_utils_pkg::HB47_NUM_COEFFS},
  parameter bit  PRELOAD_ZEROES = 1
) (
  input  wire                  clk,
  input  wire                  rst,
  input  wire                  clear,
  input  wire [SPC*SAMP_W-1:0] s_axis_tdata,
  input  wire                  s_axis_tvalid,
  output wire                  s_axis_tready,
  input  wire                  s_axis_tlast,
  output wire [SPC*SAMP_W-1:0] m_axis_tdata,
  output wire                  m_axis_tvalid,
  input  wire                  m_axis_tready,
  output wire                  m_axis_tlast,
  input  wire [           1:0] num_stages
);
  // Utility function to compute per-stage SPC values.
  // Index 0 refers to the leftmost/input stage (narrowest SPC); index NUM_HB-1 is
  // the rightmost/output stage (SPC).
  typedef int spc_arr_t [0:MAX_NUM_HB-1];
  function automatic spc_arr_t spc_per_stage(input int spc_out);
    spc_arr_t tmp;
    int cur_spc;
    cur_spc = spc_out;
    // rightmost stage has SPC_OUT = SPC, and each subsequent stage to the
    // left has half the SPC of the previous stage, down to a minimum of 1.
    for (int i = MAX_NUM_HB-1; i >= 0; i--) begin
      if (i < NUM_HB) begin
        tmp[i] = cur_spc;
        cur_spc = (cur_spc > 1) ? (cur_spc / 2) : 1;
      end
    end
    return tmp;
  endfunction

  if (NUM_HB > MAX_NUM_HB || NUM_HB < 0) begin : gen_num_hb_assertion
    $error("NUM_HB must be less than or equal to MAX_NUM_HB (%0d), but got NUM_HB=%0d",
      MAX_NUM_HB, NUM_HB);
  end

  localparam spc_arr_t SPC_PER_STAGE = spc_per_stage(SPC);

  // Internal signal for selecting the filter stages ensuring num_stages does not exceed NUM_HB.
  logic [1:0] num_stages_int;
  assign num_stages_int = (num_stages > NUM_HB) ? NUM_HB : num_stages;

  // Output signals of the hb cascade.
  // Driven by gen_stages[NUM_HB-1] (the rightmost/output stage) when NUM_HB > 0.
  // Selected at the module output when num_stages_int != 0 (see bottom assigns).
  // When NUM_HB = 0, these are never driven and never selected (num_stages_int is
  // always 0 in that case, so s_axis_* pass through directly).
  logic [SPC*SAMP_W-1:0] cascade_tdata;
  logic                  cascade_tvalid;
  logic                  cascade_tlast;
  logic                  cascade_tready;
  logic                  s_axis_chain_tready;

  // Generate width-converter cascade aligned with the filter stage indices.
  // gen_wconv[i] feeds gen_stages[i] with the direct-path input (WC_OUT = SPC[i]/2).
  //
  // Index ordering (matching the stage indexing):
  //   gen_wconv[NUM_HB-1]: takes s_axis_* module input directly
  //   gen_wconv[NUM_HB-2]: takes gen_wconv[NUM_HB-1] output
  //   ...
  //   gen_wconv[0]:        narrowest output (feeds the leftmost stage)
  //
  // The two generate loops (gen_wconv and gen_stages) are wired together via
  // hierarchical cross-references at matching index i:
  //   gen_wconv[i].tdata/tvalid/tlast --> gen_stages[i].s_axis (direct path input)
  //   gen_stages[i].direct_tready     --> gen_wconv[i].tready  (backpressure)
  //
  // chain_tready propagates backpressure upstream when a lower-index stage is the
  // active output: gen_wconv[i].tready is set to gen_wconv[i].chain_tready, which
  // is driven by the downstream gen_wconv[i-1] s_axis_tready so the whole chain
  // keeps flowing toward the module input.
  for (genvar i = 0; i < NUM_HB; i++) begin : gen_wconv
    // WC_IN_WORDS: gen_wconv[NUM_HB-1] takes SPC words from s_axis_*;
    //             all others take the output of the next converter to the left (i+1).
    localparam int WC_IN_WORDS  = (i == NUM_HB-1) ? SPC
                                : (SPC_PER_STAGE[i+1] > 1) ? (SPC_PER_STAGE[i+1] / 2) : 1;
    localparam int WC_OUT_WORDS = (SPC_PER_STAGE[i] > 1) ? (SPC_PER_STAGE[i] / 2) : 1;

    logic [WC_OUT_WORDS*SAMP_W-1:0] tdata;
    logic                           tvalid;
    logic                           tready;
    logic                           tlast;
    logic                           chain_tready;

    if (i == NUM_HB-1) begin : wc_from_input
      axis_width_conv #(
        .WORD_W    (SAMP_W),
        .IN_WORDS  (WC_IN_WORDS),
        .OUT_WORDS (WC_OUT_WORDS),
        .SYNC_CLKS (1),
        .PIPELINE  ("INOUT")
      ) width_conv_i (
        .s_axis_aclk  (clk),
        .s_axis_rst   (rst | clear),
        .s_axis_tdata (s_axis_tdata),
        .s_axis_tkeep ('1),
        .s_axis_tvalid(s_axis_tvalid && (num_stages_int != 2'd0)),
        .s_axis_tready(s_axis_chain_tready),
        .s_axis_tlast (s_axis_tlast),
        .m_axis_aclk  (clk),
        .m_axis_rst   (rst | clear),
        .m_axis_tdata (tdata),
        .m_axis_tkeep (),
        .m_axis_tvalid(tvalid),
        .m_axis_tready(tready),
        .m_axis_tlast (tlast)
      );
    end else begin : wc_from_prev
      axis_width_conv #(
        .WORD_W    (SAMP_W),
        .IN_WORDS  (WC_IN_WORDS),
        .OUT_WORDS (WC_OUT_WORDS),
        .SYNC_CLKS (1),
        .PIPELINE  ("INOUT")
      ) width_conv_i (
        .s_axis_aclk  (clk),
        .s_axis_rst   (rst | clear),
        .s_axis_tdata (gen_wconv[i+1].tdata[WC_IN_WORDS*SAMP_W-1:0]),
        .s_axis_tkeep ('1),
        .s_axis_tvalid(gen_wconv[i+1].tvalid && (num_stages_int > (NUM_HB-1-i))),
        .s_axis_tready(gen_wconv[i+1].chain_tready),
        .s_axis_tlast (gen_wconv[i+1].tlast),
        .m_axis_aclk  (clk),
        .m_axis_rst   (rst | clear),
        .m_axis_tdata (tdata),
        .m_axis_tkeep (),
        .m_axis_tvalid(tvalid),
        .m_axis_tready(tready),
        .m_axis_tlast (tlast)
      );
    end

    // Narrowest converter (i==0) has no downstream converter; tie chain_tready
    // to 0 to avoid undriven-net.
    if (i == 0) begin : wc_last
      assign chain_tready = 1'b0;
    end

    // Route tready from exactly one consumer, selected by num_stages_int:
    //   num_stages_int == NUM_HB-i: gen_stages[i] is the active output stage
    //                               → tready from its direct_tready
    //   num_stages_int >  NUM_HB-i: a deeper (lower-index) stage is active
    //                               → tready from chain_tready (next converter left)
    //   num_stages_int <  NUM_HB-i: this converter is unused → tready = 0
    assign tready = (num_stages_int == (NUM_HB - i)) ? gen_stages[i].direct_tready :
                    ((num_stages_int >  (NUM_HB - i)) ? chain_tready : 1'b0);
  end : gen_wconv

  // Inter-stage AXI-S connections: gen_stg_links[k] carries the output of
  // stage k into the s_prev_* input of stage k+1.
  // Bus width: SPC_PER_STAGE[k] samples (the output SPC of stage k).
  for (genvar k = 0; k < NUM_HB-1; k++) begin : gen_stg_links
    localparam int LINK_SPC = SPC_PER_STAGE[k];
    logic [LINK_SPC*SAMP_W-1:0] tdata;
    logic                       tvalid;
    logic                       tready;
    logic                       tlast;
  end : gen_stg_links

  // Generate HB interpolation stage cascade (NUM_HB instances).
  // Stage i=0 is the leftmost (input) stage; i=NUM_HB-1 is the rightmost
  // (output) stage.
  //
  // Each stage selects its input at runtime via input_select:
  //   input_select = 0 (num_stages_int == NUM_HB-i): direct path from gen_wconv[i]
  //   input_select = 1 (num_stages_int >  NUM_HB-i): prev path from gen_stg_links[i-1]
  //                                          (output of the adjacent left stage i-1)
  //
  // Stage output routing:
  //   i == NUM_HB-1: drives cascade_* (the module's cascade output signals)
  //   i <  NUM_HB-1: drives gen_stg_links[i] (feeds the adjacent right stage i+1)
  //
  // The leftmost stage (i == 0) has no upstream stage, so its prev inputs are
  // tied to 0 (no_prev block) to avoid X propagation.
  for (genvar i = 0; i < NUM_HB; i++) begin : gen_stages
    localparam int STG_SPC    = SPC_PER_STAGE[i];
    localparam int STG_IN_SPC = (STG_SPC > 1) ? (STG_SPC / 2) : 1;

    logic [STG_IN_SPC*SAMP_W-1:0] prev_tdata;
    logic                         prev_tvalid;
    logic                         prev_tready;
    logic                         prev_tlast;
    logic                         direct_tready;
    logic [STG_SPC*SAMP_W-1:0]    stg_tdata;
    logic                         stg_tvalid;
    logic                         stg_tready;
    logic                         stg_tlast;

    if (i > 0) begin : with_prev
      assign prev_tdata                = gen_stg_links[i-1].tdata;
      assign prev_tvalid               = (num_stages_int > (NUM_HB - i)) ? gen_stg_links[i-1].tvalid : 1'b0;
      assign prev_tlast                = gen_stg_links[i-1].tlast;
      assign gen_stg_links[i-1].tready = prev_tready;
    end else begin : no_prev
      assign prev_tdata  = '0;
      assign prev_tvalid = 1'b0;
      assign prev_tlast  = 1'b0;
    end

    axis_hb_intp_stage #(
      .SAMP_W        (SAMP_W),
      .SPC_OUT       (STG_SPC),
      .NUM_COEFFS    (HB_NUM_COEFFS[i]),
      .PRELOAD_ZEROES(PRELOAD_ZEROES)
    ) stage_i (
      .clk          (clk),
      .rst          (rst),
      .clear        (clear),
      .input_select ((num_stages_int > (NUM_HB - i)) ? 1'b1 : 1'b0),
      .s_axis_tdata (gen_wconv[i].tdata[STG_IN_SPC*SAMP_W-1:0]),
      .s_axis_tvalid((num_stages_int == (NUM_HB - i)) ? gen_wconv[i].tvalid : 1'b0),
      .s_axis_tready(direct_tready),
      .s_axis_tlast ((num_stages_int == (NUM_HB - i)) ? gen_wconv[i].tlast : 1'b0),
      .s_prev_tdata (prev_tdata),
      .s_prev_tvalid(prev_tvalid),
      .s_prev_tready(prev_tready),
      .s_prev_tlast (prev_tlast),
      .m_axis_tdata (stg_tdata),
      .m_axis_tvalid(stg_tvalid),
      .m_axis_tready(stg_tready),
      .m_axis_tlast (stg_tlast)
    );

    if (i == NUM_HB-1) begin : to_module_output
      assign cascade_tdata  = stg_tdata;
      assign cascade_tvalid = stg_tvalid;
      assign cascade_tlast  = stg_tlast;
      assign stg_tready     = cascade_tready;
    end else begin : to_next_link
      assign gen_stg_links[i].tdata  = stg_tdata;
      assign gen_stg_links[i].tvalid = stg_tvalid;
      assign gen_stg_links[i].tlast  = stg_tlast;
      assign stg_tready              = gen_stg_links[i].tready;
    end
  end : gen_stages

  // cascade_tready is always connected to m_axis_tready so the output stage
  // can accept data whenever the downstream is ready.
  assign cascade_tready = m_axis_tready;

  // Output mux: when num_stages_int == 0 the module is a pass-through and s_axis_*
  // is wired directly to m_axis_*. Otherwise the cascade output is selected.
  // s_axis_tready is sourced from the width-converter chain (s_axis_chain_tready)
  // so backpressure propagates all the way back to the input.
  assign s_axis_tready = (num_stages_int == 2'd0) ? m_axis_tready : s_axis_chain_tready;
  assign m_axis_tdata  = (num_stages_int == 2'd0) ? s_axis_tdata  : cascade_tdata;
  assign m_axis_tvalid = (num_stages_int == 2'd0) ? s_axis_tvalid : cascade_tvalid;
  assign m_axis_tlast  = (num_stages_int == 2'd0) ? s_axis_tlast  : cascade_tlast;

endmodule : axis_hb_cascade_intp

`default_nettype wire
