//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_hb_cascade_decim
//
// Description:
//   AXI4-Stream halfband decimation cascade.
//   The data-path is organized as two cascades:
//   1) A cascade of NUM_HB halfband decimation FIR stages (gen_stages), each halving the
//      sample rate.  Index 0 is the leftmost/input stage (widest SPC); index NUM_HB-1 is
//      the rightmost (narrowest SPC before the WC chain).
//   2) A width-converter chain (gen_wconv) that expands the narrow decimated output back to
//      the full SPC width for the 2-input output mux.
//
//   Example topology for SPC=8, NUM_HB=3:
//
//     s_axis_* (input)                                                                               +----\
//         |                                                                                          |     \
//       x8/--------------------------------------------------------------------------------------/-->|      \  m_axis_* (output)
//         |                                                                                     x8   |      |--/-->
//         |                                                                         +-------+        |      |  x8
//         |                                                                         | wc_0  |----/-->|      /
//         |                                                                         +-------+   x8   |     /
//         |                                                                             |            +----/
//         |                          +------------------------------------------------->/x4            |
//         |                          |                                                  |              |
//         |                          |                                              +-------+          |
//         |                          |                                              | wc_1  |          |
//         |                          |                                              +-------+          |
//         |                          |                                                  |              |
//         |                          |                         +----------------------->/x2            |
//         |                          |                         |                        |              |
//         |                          |                         |                    +-------+          |
//         |                          |                         |                    | wc_2  |          |
//         |                          |                         |                    +-------+          |
//         |                          |                         |                        |              |
//       x8/                          |                         |                        |              |
//         |   +-----------------+    |  +-----------------+    |  +-----------------+   |              |
//         |   |      hbi_0      |    |  |      hbi_1      |    |  |      hbi_2      |   |              |
//         |   |      direct out |----+  |      direct out |----+  |      direct out |---+              |
//         +-->| in     next_out |--/--->| in     next_out |--/--->| in     next_out |                  |
//          +->| output_select   | x4 +->| output_select   | x2 +->| output_select   |                  |
//          |  +-----------------+    |  +-----------------+    |  +-----------------+                  |
//          |                         |                         |                                       |
//          |                         |                         |                                       |
//          +-------------------------+-------------------------+---------------------------------------+
//                                                  |
//                                             num_stages
//
//   Signal flow per num_stages (NUM_HB=3 example):
//     num_stages=0: s_axis                                            -> out_mux[bypass] -> m_axis
//     num_stages=1: s_axis -> hbi_0 -> wc_0                                   -> out_mux -> m_axis
//     num_stages=2: s_axis -> hbi_0 -> hbi_1 -> wc_1 -> wc_0                  -> out_mux -> m_axis
//     num_stages=3: s_axis -> hbi_0 -> hbi_1 -> hbi_2 -> wc_2 -> wc_1 -> wc_0 -> out_mux -> m_axis
//
//   The hbi_i output_select port steers the FIR output to either the WC chain
//   (m_direct, when this is the last active stage) or the next stage.
//
// Parameters:
//   SAMP_W:        Sample width in bits.
//   SPC:           Samples per cycle (input/output axis bus width in number of samples).
//   NUM_HB:        Number of halfband stages to cascade (0..HB_DECIM_MAX_NUM_HB).
//                  For NUM_HB=0 the module bypasses input directly to output.
//   HB_NUM_COEFFS: Per-stage coefficient count array (47 or 63 per stage).
//                  Fixed size HB_DECIM_MAX_NUM_HB; only first NUM_HB elements used.
//                  HB_NUM_COEFFS[0] → gen_stages[0] (leftmost/input stage).
//   PRELOAD_ZEROES: If 1, pre-fill HBF tap history on clear for deterministic burst behavior.
//
// Notes:
//   tkeep: The m_axis_tkeep output reflects partial last-word padding introduced by the
//          width-converter (axis_width_conv) chain that re-packs the narrow per-stage
//          output back  to the full SPC width. It arises only when the number of output
//          samples produced by the last active HB decimation stage is not an integer
//          multiple of SPC.
//          The underlying multi-sample FIR filter (axis_hb_decim_fir) does not produce
//          or consume tkeep; it always operates on complete SPC-wide words and relies
//          on the caller to send only fully-populated words.
//


`default_nettype none

module axis_hb_cascade_decim #(
  localparam int MAX_NUM_HB    = axis_hb_utils_pkg::HB_DECIM_MAX_NUM_HB,
  parameter int  SAMP_W        = 48,
  parameter int  SPC           = 8,
  parameter int  NUM_HB        = 3,
  parameter int  HB_NUM_COEFFS [MAX_NUM_HB] = '{default: axis_hb_utils_pkg::HB47_NUM_COEFFS},
  parameter bit  PRELOAD_ZEROES = 0
) (
  // Clock and reset
  input  wire                        clk,
  input  wire                        rst,
  input  wire                        clear,
  // AXI4-Stream input
  input  wire  [SPC-1:0][SAMP_W-1:0] s_axis_tdata,
  input  wire                        s_axis_tvalid,
  output logic                       s_axis_tready,
  input  wire                        s_axis_tlast,
  // AXI4-Stream output
  output logic [SPC-1:0][SAMP_W-1:0] m_axis_tdata,
  output logic                       m_axis_tvalid,
  input  wire                        m_axis_tready,
  output logic                       m_axis_tlast,
  output logic [SPC-1:0]             m_axis_tkeep,
  // Runtime stage select (0 = bypass, up to NUM_HB)
  input  wire  [1:0]                 num_stages
);

  //---------------------------------------------------------------------------
  // Parameter validation
  //---------------------------------------------------------------------------
  if (NUM_HB > MAX_NUM_HB || NUM_HB < 0) begin : gen_num_hb_assertion
    $error("NUM_HB must be 0..MAX_NUM_HB (%0d), got NUM_HB=%0d", MAX_NUM_HB, NUM_HB);
  end

  //---------------------------------------------------------------------------
  // Helper functions
  //---------------------------------------------------------------------------
  // SPC_PER_STAGE[i] = SPC_IN of gen_stages[i].
  // Index 0 is the leftmost/input stage (SPC_IN = SPC, widest);
  // index NUM_HB-1 is the rightmost stage (narrowest, before the WC chain).
  // Each stage halves the SPC of the previous stage, down to a minimum of 1.
  typedef int spc_arr_t [0:MAX_NUM_HB-1];
  function automatic spc_arr_t spc_per_stage(input int spc_in);
    spc_arr_t tmp;
    int cur_spc;
    cur_spc = spc_in;
    for (int i = 0; i < MAX_NUM_HB; i++) begin
      if (i < NUM_HB) begin
        tmp[i] = cur_spc;
        cur_spc = (cur_spc > 1) ? (cur_spc / 2) : 1;
      end else begin
        tmp[i] = 1; // unused entries
      end
    end
    return tmp;
  endfunction

  localparam spc_arr_t SPC_PER_STAGE = spc_per_stage(SPC);

  // DECIM_PHASE_PER_STAGE[i] = DECIMATION_PHASE for gen_stages[i].
  //
  // Two modes of decimation exist depending on the SPC of a stage:
  //   Lane-domain  (SPC_PER_STAGE[i] > 1): The FIR picks one output lane out of SPC_IN
  //                lanes. Decimation phase is always 0 (lane 0); the output clock rate is
  //                unchanged.
  //   Time-domain  (SPC_PER_STAGE[i] == 1): The FIR decimates in time (SPC_IN=SPC_OUT=1).
  //                The output clock rate is halved. The decimation phase must be chosen to
  //                align the overall cascade output with the reference (pure-lane) phase.
  //
  // Purpose of this function: compute the DECIMATION_PHASE (either 0 or 1) for each
  // time-domain decimation stage such that the output of the overall cascade is identical
  // irrespective of the parallelism (SPC) of the input.
  // (Algorithmically this is: always select the same out of the 2^num_stages equally possible
  // and valid decimation phases.)
  //
  // The phase for a time-domain stage i is determined by the parity of the total FIR
  // pipeline delay accumulated at its input:
  //   PHASE[i] = (acc_delay + PD[i]) % 2
  // where PD[i] = (HB_NUM_COEFFS[i] + 5) / 4 + DSP_PIPELINE_DELAY
  // and acc_delay evolves as:
  //   Lane-domain stage: acc_delay += PD[i]          (same clock rate, delay adds directly)
  //   Time-domain stage: acc_delay = (acc_delay + PD[i]) / 2  (output at half rate)
  localparam int DSP_PIPELINE_DELAY = 5;
  typedef bit phase_arr_t [0:MAX_NUM_HB-1];
  function automatic phase_arr_t decim_phase_per_stage(
    input spc_arr_t spc_per_stg,
    input int       num_coeffs [MAX_NUM_HB]
  );
    phase_arr_t phases;
    int acc_delay;
    int pd;
    acc_delay = 0;
    for (int i = 0; i < MAX_NUM_HB; i++) begin
      if (i < NUM_HB) begin
        pd = (num_coeffs[i] + 5) / 4 + DSP_PIPELINE_DELAY;
        if (spc_per_stg[i] == 1) begin
          // Time-domain decimation: pick the phase that aligns with accumulated delay.
          phases[i] = bit'((acc_delay + pd) % 2);
          acc_delay = (acc_delay + pd) / 2;
        end else begin
          // Lane-domain decimation: always pick lane 0.
          phases[i] = 1'b0;
          acc_delay = acc_delay + pd;
        end
      end else begin
        phases[i] = 1'b0; // Unused stage slot.
      end
    end
    return phases;
  endfunction

  localparam phase_arr_t DECIM_PHASE_PER_STAGE = decim_phase_per_stage(SPC_PER_STAGE, HB_NUM_COEFFS);

  //---------------------------------------------------------------------------
  // Internal signals
  //---------------------------------------------------------------------------
  // Clamp num_stages to NUM_HB so out-of-range values are handled gracefully.
  logic [1:0] num_stages_int;
  assign num_stages_int = (num_stages > NUM_HB[1:0]) ? NUM_HB[1:0] : num_stages;

  //---------------------------------------------------------------------------
  // Output signals of cascade (driven by gen_wconv[0] when NUM_HB > 0)
  //---------------------------------------------------------------------------
  logic [SPC-1:0][SAMP_W-1:0] cascade_tdata;
  logic [SPC-1:0]             cascade_tkeep; // tkeep per sample at cascade output SPC
  logic                       cascade_tvalid;
  logic                       cascade_tlast;
  wire                        cascade_tready;
  assign cascade_tready = m_axis_tready;

  //---------------------------------------------------------------------------
  // gen_stg_links: Inter-stage AXI-S links.
  // gen_stg_links[k] carries m_next of stage k to s_axis of stage k+1.
  // Bus width: SPC_PER_STAGE[k+1] samples (= SPC_IN of stage k+1 = SPC_OUT of stage k).
  //---------------------------------------------------------------------------
  for (genvar k = 0; k < NUM_HB-1; k++) begin : gen_stg_links
    localparam int LINK_SPC = SPC_PER_STAGE[k+1];
    logic [LINK_SPC-1:0][SAMP_W-1:0] tdata;
    logic                            tvalid;
    logic                            tready;
    logic                            tlast;
  end : gen_stg_links

  //---------------------------------------------------------------------------
  // gen_stages: NUM_HB halfband decimation FIR stages.
  // Index 0 = leftmost/input stage (SPC_IN = SPC, widest).
  // Index NUM_HB-1 = rightmost stage (narrowest, before the WC chain).
  //
  // output_select routing per stage i:
  //   0 (num_stages_int == i+1): FIR output -> m_direct -> gen_wconv[i] (this is the last stage)
  //   1 (num_stages_int  > i+1): FIR output -> m_next   -> gen_stg_links[i] -> stage i+1
  //
  // m_direct_tready is driven by gen_wconv[i].stg_m_direct_tready (cross-reference).
  //---------------------------------------------------------------------------
  for (genvar i = 0; i < NUM_HB; i++) begin : gen_stages
    localparam int STG_SPC     = SPC_PER_STAGE[i];
    localparam int STG_OUT_SPC = (STG_SPC > 1) ? (STG_SPC / 2) : 1;

    logic [STG_SPC-1:0][SAMP_W-1:0]     stg_s_tdata;
    logic                               stg_s_tvalid;
    logic                               stg_s_tready;
    logic                               stg_s_tlast;
    logic [STG_OUT_SPC-1:0][SAMP_W-1:0] m_direct_tdata;
    logic                               m_direct_tvalid;
    wire                                m_direct_tready;  // driven by gen_wconv[i].stg_m_direct_tready
    logic                               m_direct_tlast;
    logic [STG_OUT_SPC-1:0][SAMP_W-1:0] m_next_tdata;
    logic                               m_next_tvalid;
    logic                               m_next_tready;
    logic                               m_next_tlast;

    // Drive m_direct_tready from the width-converter chain (cross-reference)
    assign m_direct_tready = gen_wconv[i].stg_m_direct_tready;

    // Input routing
    if (i == 0) begin : from_input
      assign stg_s_tdata  = s_axis_tdata;
      assign stg_s_tvalid = s_axis_tvalid & (num_stages_int != 2'd0);
      assign stg_s_tlast  = s_axis_tlast;
      // s_axis_tready is assigned in gen_output below
    end else begin : from_prev_link
      assign stg_s_tdata  = gen_stg_links[i-1].tdata;
      assign stg_s_tvalid = (num_stages_int > i) ? gen_stg_links[i-1].tvalid : 1'b0;
      assign stg_s_tlast  = gen_stg_links[i-1].tlast;
      assign gen_stg_links[i-1].tready = stg_s_tready;
    end

    axis_hb_decim_stage #(
      .SAMP_W          (SAMP_W),
      .SPC_IN          (STG_SPC),
      .NUM_COEFFS      (HB_NUM_COEFFS[i]),
      .DECIMATION_PHASE(DECIM_PHASE_PER_STAGE[i]),
      .PRELOAD_ZEROES  (PRELOAD_ZEROES)
    ) stage_i (
      .clk            (clk),
      .rst            (rst),
      .clear          (clear),
      .output_select  (num_stages_int > (i+1) ? 1'b1 : 1'b0),
      .s_axis_tdata   (stg_s_tdata),
      .s_axis_tvalid  (stg_s_tvalid),
      .s_axis_tready  (stg_s_tready),
      .s_axis_tlast   (stg_s_tlast),
      .m_direct_tdata (m_direct_tdata),
      .m_direct_tvalid(m_direct_tvalid),
      .m_direct_tready(m_direct_tready),
      .m_direct_tlast (m_direct_tlast),
      .m_next_tdata   (m_next_tdata),
      .m_next_tvalid  (m_next_tvalid),
      .m_next_tready  (m_next_tready),
      .m_next_tlast   (m_next_tlast)
    );

    // m_next routing
    if (i < NUM_HB-1) begin : to_next_link
      assign gen_stg_links[i].tdata  = m_next_tdata;
      assign gen_stg_links[i].tvalid = m_next_tvalid;
      assign gen_stg_links[i].tlast  = m_next_tlast;
      assign m_next_tready           = gen_stg_links[i].tready;
    end else begin : no_next_link
      assign m_next_tready = 1'b0;   // last stage never uses m_next
    end

  end : gen_stages

  //---------------------------------------------------------------------------
  // gen_wconv: Width-converter chain — expands narrow stage output back to SPC.
  //
  // gen_wconv[i] widens from SPC_OUT of stage i (WC_IN_SPC) to SPC_PER_STAGE[i] (WC_OUT_SPC).
  //
  // Chain direction: gen_wconv[NUM_HB-1] -> gen_wconv[NUM_HB-2] -> ... -> gen_wconv[0] -> cascade
  //
  // Input muxing:
  //   i == NUM_HB-1: always from gen_stages[NUM_HB-1].m_direct (no mux)
  //   i <  NUM_HB-1: mux — stages[i].m_direct (num_stages_int == i+1)
  //                         OR gen_wconv[i+1] output (num_stages_int  > i+1)
  //
  // stg_m_direct_tready: signal exported for gen_stages[i].m_direct_tready.
  //   Active (= wc_s_tready) only when stage i is the last active stage; else 0.
  //
  // tready of this WC (m_axis_tready):
  //   i == 0: cascade_tready (= m_axis_tready)
  //   i  > 0: gen_wconv[i-1].wc_s_tready when num_stages_int > i; else 0
  //---------------------------------------------------------------------------
  for (genvar i = 0; i < NUM_HB; i++) begin : gen_wconv
    localparam int WC_IN_SPC  = (SPC_PER_STAGE[i] > 1) ? (SPC_PER_STAGE[i] / 2) : 1;
    localparam int WC_OUT_SPC = SPC_PER_STAGE[i];

    // Signals for axis_width_conv
    logic [WC_IN_SPC-1:0][SAMP_W-1:0]  wc_s_tdata;
    logic [WC_IN_SPC-1:0]              wc_s_tkeep;  // from upstream WC or '1 for stage direct
    logic                              wc_s_tvalid;
    logic                              wc_s_tready; // driven by axis_width_conv s_axis_tready
    logic                              wc_s_tlast;
    logic [WC_OUT_SPC-1:0][SAMP_W-1:0] tdata;
    logic [WC_OUT_SPC-1:0]             tkeep;       // tkeep per sample at this WC's output SPC
    logic                              tvalid;
    logic                              tready;      // m_axis_tready of this WC
    logic                              tlast;
    // Exported to gen_stages[i].m_direct_tready
    logic                              stg_m_direct_tready;

    // Input source
    if (i == NUM_HB-1) begin : from_last_stage
      assign wc_s_tdata  = gen_stages[i].m_direct_tdata;
      assign wc_s_tkeep  = '1;                            // stage output is always fully valid
      assign wc_s_tvalid = gen_stages[i].m_direct_tvalid;
      assign wc_s_tlast  = gen_stages[i].m_direct_tlast;
      assign stg_m_direct_tready = (num_stages_int == (i+1)) ? wc_s_tready : 1'b0;
    end else begin : from_mux
      // Mux: deeper WC chain output (higher stages active) vs. this stage's m_direct
      wire mux_sel;
      assign mux_sel = (num_stages_int > (i+1));
      assign wc_s_tdata  = mux_sel ? gen_wconv[i+1].tdata  : gen_stages[i].m_direct_tdata;
      // Propagate partial-last-word tkeep from the upstream WC so this WC
      // receives the correct keep mask and can produce a valid output tkeep.
      // When drawing directly from a stage m_direct, tkeep is always '1, since
      // the stage output doesn't have tkeep information and is always fully valid.
      assign wc_s_tkeep  = mux_sel ? gen_wconv[i+1].tkeep  : '1;
      assign wc_s_tvalid = mux_sel ? gen_wconv[i+1].tvalid : gen_stages[i].m_direct_tvalid;
      assign wc_s_tlast  = mux_sel ? gen_wconv[i+1].tlast  : gen_stages[i].m_direct_tlast;
      assign stg_m_direct_tready = mux_sel ? 1'b0 : wc_s_tready;
    end

    // Output routing
    if (i == 0) begin : to_cascade
      assign cascade_tdata  = tdata;
      assign cascade_tkeep  = tkeep;
      assign cascade_tvalid = tvalid;
      assign cascade_tlast  = tlast;
      assign tready         = cascade_tready;
    end else begin : to_prev_wc
      // Backpressure from gen_wconv[i-1]'s WC when this WC is in the active path
      assign tready = (num_stages_int > i) ? gen_wconv[i-1].wc_s_tready : 1'b0;
    end

    axis_width_conv #(
      .WORD_W    (SAMP_W),
      .IN_WORDS  (WC_IN_SPC),
      .OUT_WORDS (WC_OUT_SPC),
      .SYNC_CLKS (1),
      .PIPELINE  ("INOUT")
    ) width_conv_i (
      .s_axis_aclk  (clk),
      .s_axis_rst   (rst | clear),
      .s_axis_tdata (wc_s_tdata),
      .s_axis_tkeep (wc_s_tkeep),
      .s_axis_tvalid(wc_s_tvalid),
      .s_axis_tready(wc_s_tready),
      .s_axis_tlast (wc_s_tlast),
      .m_axis_aclk  (clk),
      .m_axis_rst   (rst | clear),
      .m_axis_tdata (tdata),
      .m_axis_tkeep (tkeep),
      .m_axis_tvalid(tvalid),
      .m_axis_tready(tready),
      .m_axis_tlast (tlast)
    );

  end : gen_wconv

  //---------------------------------------------------------------------------
  // Module output mux and s_axis_tready
  //   num_stages_int == 0: bypass — s_axis passes directly to m_axis
  //   num_stages_int  > 0: cascade path — gen_wconv[0] -> cascade -> m_axis
  //---------------------------------------------------------------------------
  if (NUM_HB > 0) begin : gen_output
    assign s_axis_tready = (num_stages_int == 2'd0) ? m_axis_tready : gen_stages[0].stg_s_tready;
    assign m_axis_tdata  = (num_stages_int == 2'd0) ? s_axis_tdata  : cascade_tdata;
    assign m_axis_tvalid = (num_stages_int == 2'd0) ? s_axis_tvalid : cascade_tvalid;
    assign m_axis_tlast  = (num_stages_int == 2'd0) ? s_axis_tlast  : cascade_tlast;
    assign m_axis_tkeep  = (num_stages_int == 2'd0) ? '1            : cascade_tkeep;
  end else begin : gen_bypass
    assign s_axis_tready = m_axis_tready;
    assign m_axis_tdata  = s_axis_tdata;
    assign m_axis_tvalid = s_axis_tvalid;
    assign m_axis_tlast  = s_axis_tlast;
    assign m_axis_tkeep  = '1;
  end

endmodule : axis_hb_cascade_decim

`default_nettype wire
