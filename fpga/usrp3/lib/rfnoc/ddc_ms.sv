//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ddc_ms
//
// Description:
//   DDC signal processing module composed of the
//     - DDS
//     - CIC Filter
//     - HBF Cascade
//     - IQ scaling
//
//
// Parameters:
//   SPC           : Number of samples processed per clock cycle.
//                   This determines the width of the input and output data streams,
//                   which are SAMP_W*SPC bits wide.
//                   The DDC will process SPC samples in parallel every clock cycle.
//   NUM_HB        : Number of halfband filter stages to implement in each DDC chain.
//                   This determines the maximum decimation rate of the halfband filters,
//                   which is 2^(NUM_HB-1).
//   CIC_MAX_DECIM : Maximum decimation rate of the CIC filter in each DDC chain.
//                   The total maximum decimation rate of the DDC is
//                   CIC_MAX_DECIM * 2^(NUM_HB-1).
//   SAMP_W        : Width of a  I+Q sample.
//

`default_nettype none

module ddc_ms
  import ctrlport_pkg::*;
#(
  parameter int SPC           = 1,
  parameter int NUM_HB        = 3,
  parameter int CIC_MAX_DECIM = 255,
  parameter int SAMP_W        = 32,
  parameter int PHASE_W       = 24
) (
  input  wire logic clk,
  input  wire logic reset,
  input  wire logic clear,

  // CTRL port requests
  input  wire logic                          s_ctrlport_req_wr,
  input  wire logic                          s_ctrlport_req_rd,
  input  wire logic [   CTRLPORT_ADDR_W-1:0] s_ctrlport_req_addr,
  input  wire logic [   CTRLPORT_DATA_W-1:0] s_ctrlport_req_data,
  input  wire logic [CTRLPORT_BYTE_EN_W-1:0] s_ctrlport_req_byte_en,
  input  wire logic                          s_ctrlport_req_has_time,
  input  wire logic [   CTRLPORT_TIME_W-1:0] s_ctrlport_req_time,
  output      logic                          s_ctrlport_resp_ack,
  output      logic [    CTRLPORT_STS_W-1:0] s_ctrlport_resp_status,
  output      logic [   CTRLPORT_DATA_W-1:0] s_ctrlport_resp_data,

  // Sample stream
  input  wire logic [SPC-1:0][SAMP_W-1:0]  sample_in_tdata,
  input  wire logic                        sample_in_tvalid,
  input  wire logic                        sample_in_tlast,
  output      logic                        sample_in_tready,
  // Bits [SPC:1] - tag sample corresponding to timed command
  // Bit 0        - End of burst indicator (EoB) for the input sample stream
  input  wire logic [              SPC:0]  sample_in_tuser,

  output      logic [SPC-1:0][SAMP_W-1:0]  sample_out_tdata,
  output      logic                        sample_out_tvalid,
  output      logic                        sample_out_tlast,
  input  wire logic                        sample_out_tready,

  // Phase (freq shift) stream
  input  wire logic [PHASE_W-1:0] phase_in_tdata,
  input  wire logic               phase_in_tvalid,
  input  wire logic               phase_in_tuser,
  output      logic               phase_in_tready
);

  import ddc_ms_regs_pkg::*;

  //---------------------------------------------------------------------------
  // Localparams
  //---------------------------------------------------------------------------
  // Scale factor of signed #Q2.15 format
  localparam int SCALING_WIDTH = REG_SR_SCALE_IQ_W;
  localparam int SCALE_FRAC_W  = SCALING_WIDTH - 3;
  localparam int HB_NUM_COEFFS [axis_hb_utils_pkg::HB_DECIM_MAX_NUM_HB] = '{
    2: axis_hb_utils_pkg::HB63_NUM_COEFFS,
    default: axis_hb_utils_pkg::HB47_NUM_COEFFS
  };

  //---------------------------------------------------------------------------
  // Handle SR register accesses directly in this block (no separate SRC module)
  //---------------------------------------------------------------------------
  logic [ SCALING_WIDTH-1:0] reg_scale_iq;
  logic [REG_SR_DECIM_W-1:0] reg_decim;

  always @(posedge clk) begin
    if (reset) begin
      s_ctrlport_resp_ack    <= 1'b0;
      s_ctrlport_resp_status <= CTRL_STS_OKAY;
      s_ctrlport_resp_data   <= 32'h0;
      reg_scale_iq           <= {SCALING_WIDTH{1'b0}};
      reg_decim              <= REG_SR_DECIM_W'(1); // hb_rate=0, cic_decim_rate=1
    end else begin
      s_ctrlport_resp_ack    <= 1'b0;
      s_ctrlport_resp_status <= CTRL_STS_OKAY;
      s_ctrlport_resp_data   <= 32'h0;
      if (s_ctrlport_req_wr) begin
        case (s_ctrlport_req_addr)
          REG_SR_DECIM_ADDR: begin
            reg_decim           <= s_ctrlport_req_data[REG_SR_DECIM_W-1:0];
            s_ctrlport_resp_ack <= 1'b1;
          end
          REG_SR_MUX_ADDR: begin
            // Legacy register for IQ swap & realmode,
            // functionality no longer required for MS DDC.
            // Keeping register for backwards compatibility,
            // but return an error if written to.
            s_ctrlport_resp_status <= CTRL_STS_CMDERR;
            s_ctrlport_resp_ack <= 1'b1;
          end
          REG_SR_SCALE_IQ_ADDR: begin
            reg_scale_iq        <= s_ctrlport_req_data[SCALING_WIDTH-1:0];
            s_ctrlport_resp_ack <= 1'b1;
          end
          default: begin
            s_ctrlport_resp_status <= CTRL_STS_CMDERR; // unsupported address
            s_ctrlport_resp_ack    <= 1'b1;
          end
        endcase
      end
      if (s_ctrlport_req_rd) begin
        case (s_ctrlport_req_addr)
          REG_SR_DECIM_ADDR: begin
            s_ctrlport_resp_data <= reg_decim;
            s_ctrlport_resp_ack  <= 1'b1;
          end
          REG_SR_MUX_ADDR: begin
            // Legacy register for IQ swap & realmode,
            // functionality no longer required for MS DDC.
            // Keeping register for backwards compatibility,
            // but return an error if read.
            s_ctrlport_resp_status <= CTRL_STS_CMDERR;
            s_ctrlport_resp_ack  <= 1'b1;
          end
          REG_SR_SCALE_IQ_ADDR: begin
            s_ctrlport_resp_data <= reg_scale_iq;
            s_ctrlport_resp_ack  <= 1'b1;
          end
          default: begin
            s_ctrlport_resp_status <= CTRL_STS_CMDERR; // unsupported address
            s_ctrlport_resp_ack    <= 1'b1;
          end
        endcase
      end
    end
  end

  //-------------------------------------------------------------------------
  // AXI stream interfaces between DDC DSP components
  //-------------------------------------------------------------------------

  // DDS
  logic [SPC*SAMP_W-1:0]  dds_out_tdata;
  logic                   dds_out_tvalid, dds_out_tready;
  logic                   dds_out_tlast, dds_out_tuser;
  // HBF
  wire                    hbf_out_tvalid, hbf_out_tready, hbf_out_tlast;
  wire  [SPC*SAMP_W-1:0]  hbf_out_tdata;
  // CIC
  AxiStreamIf #(SPC*SAMP_W) decim_in (
    .clk    (clk),
    .rst    (reset)
  );

  AxiStreamIf #(SPC*SAMP_W) decim_out (
    .clk    (clk),
    .rst    (reset)
  );

  //--------------------------------------------------------------------------
  // Latch configuration during burst processing to avoid corrupted data caused
  // by changing the decimation rate or scaling factor mid-burst.
  //--------------------------------------------------------------------------
  logic [REG_SR_DECIM_W-1:0] decim_rate_latched;
  logic [   SCALING_WIDTH-1:0] scale_iq_latched;
  logic latched_rate_changed;
  logic active_burst;

  // Track whether input samples are currently being processed as a burst.
  always_ff @(posedge clk) begin
    if (reset || clear) begin
      active_burst <= 1'b0;
    end else if (!active_burst && sample_in_tvalid && sample_in_tready) begin
      active_burst <= 1'b1;
    end
  end

  // Apply new configuration while idle and hold it through the burst.
  // Changes written during a burst are applied after the EoB clear pulse.
  always_ff @(posedge clk) begin
    latched_rate_changed <= 1'b0;
    if (reset) begin
      decim_rate_latched <= REG_SR_DECIM_W'(1);
      scale_iq_latched   <= 1 << SCALE_FRAC_W; // 1.0 in Q2.15 format
    end else if (clear || !active_burst) begin
      scale_iq_latched <= reg_scale_iq;
      decim_rate_latched  <= reg_decim;
      if (clear || (reg_decim != decim_rate_latched)) begin
        latched_rate_changed <= 1'b1;
      end
    end
  end

  // -------------------------------------------------------------------------
  // Multisample DDS frequency shift
  // Note:
  // - The tuning word is passed over the control port from the host. This module
  // includes a phase accumulator (increments by the tuning word) that uses the
  // EoB flag from the input stream to reset the phase at the start of each burst.
  // - The output sample stream indicates EoB with the tuser signal, which is
  // used by the downstream decimator to know when to reset its decimation phase.
  // - IQ sample data (in the range [-1, 1)) is represented in #Q0.*_FRAC_W
  // fixed-point format ({1 sign bit, fractional bits}).
  // -------------------------------------------------------------------------

  dds_ms #(
    .SPC               (SPC),
    .SAMP_W            (SAMP_W),
    .SAMP_FRAC_W       ((SAMP_W/2)-1),
    .PHASE_WIDTH       (PHASE_W)
  ) dds_ms_i (
    .clk                      (clk),
    .rst                      (reset),
    .s_axis_din_tdata         (sample_in_tdata),
    .s_axis_din_tlast         (sample_in_tlast),
    .s_axis_din_tvalid        (sample_in_tvalid),
    .s_axis_din_tready        (sample_in_tready),
    .s_axis_din_tuser         (sample_in_tuser),
    .s_axis_phase_tdata       (phase_in_tdata),
    .s_axis_phase_tvalid      (phase_in_tvalid),
    .s_axis_phase_tuser       (phase_in_tuser),
    .s_axis_phase_tready      (phase_in_tready),
    .m_axis_dout_tdata        (dds_out_tdata),
    .m_axis_dout_tvalid       (dds_out_tvalid),
    .m_axis_dout_tlast        (dds_out_tlast),
    .m_axis_dout_tready       (dds_out_tready),
    .m_axis_dout_tuser        (dds_out_tuser)
  );

  // -------------------------------------------------------------------------
  // CIC decimator
  // -------------------------------------------------------------------------
  logic [7:0] cic_rate;
  logic [1:0] hb_enables;
  assign cic_rate   = decim_rate_latched[7:0];
  assign hb_enables = decim_rate_latched[9:8];

  // DDS drain gate: dds_ms has MULTIPLY_LATENCY=6 pipeline cycles.
  // When ARC fires clear, up to 6 in-flight beats drain out of the DDS. These
  // must not reach the CIC after it has been reset, or they corrupt state for
  // the next burst. A 6-bit shift register is loaded with 1s on clear and
  // shifts right one bit per cycle (when dds_out_tready=1), masking
  // dds_out_tvalid for exactly 6 cycles so drain beats are silently discarded.
  localparam int DDS_LATENCY = 6; // matches MULTIPLY_LATENCY in dds_ms
  logic [DDS_LATENCY-1:0] dds_drain_mask;
  always_ff @(posedge clk) begin
    if (reset || clear)
      dds_drain_mask <= {DDS_LATENCY{1'b1}};  // arm: block next 6 beats
    else if (dds_out_tready)
      dds_drain_mask <= {1'b0, dds_drain_mask[DDS_LATENCY-1:1]}; // drain one per cycle
  end
  wire dds_out_tvalid_gated = dds_out_tvalid & ~dds_drain_mask[0];

  assign decim_in.tdata  = dds_out_tdata;
  assign decim_in.tvalid = dds_out_tvalid_gated;
  assign decim_in.tlast  = dds_out_tlast;
  assign dds_out_tready  = decim_in.tready;

  cic_filter_decim #(
    .SPC       (SPC),
    .SAMP_W    (SAMP_W),
    .MAX_DECIM (CIC_MAX_DECIM),
    .ORDER     (4)
  ) cic_filter_decim_i (
    .clk            (clk),
    .rst            (reset),
    .clear          (clear),
    .data_in        (decim_in),
    .data_out       (decim_out),
    .decim_factor   (cic_rate),
    .config_changed (latched_rate_changed)
  );

  // -------------------------------------------------------------------------
  // HBF cascade decimator
  // -------------------------------------------------------------------------
  axis_hb_cascade_decim #(
    .SAMP_W         (SAMP_W),
    .SPC            (SPC),
    .NUM_HB         (NUM_HB),
    .HB_NUM_COEFFS  (HB_NUM_COEFFS),
    .PRELOAD_ZEROES (1)
  ) hbf_cascade_i (
    .clk           (clk),
    .rst           (reset),
    .clear         (clear),
    .s_axis_tdata  (decim_out.tdata),
    .s_axis_tvalid (decim_out.tvalid),
    .s_axis_tready (decim_out.tready),
    .s_axis_tlast  (decim_out.tlast),
    .m_axis_tdata  (hbf_out_tdata),
    .m_axis_tvalid (hbf_out_tvalid),
    .m_axis_tready (hbf_out_tready),
    .m_axis_tlast  (hbf_out_tlast),
    .m_axis_tkeep  (), // tkeep not used upstream
    .num_stages    (hb_enables)
  );

  //-------------------------------------------------------------------------
  // Scale the decimated IQ samples by the latched scaling factor.
  //-------------------------------------------------------------------------
  wire [SPC-1:0] scale_iq_tlast, scale_iq_tvalid, scale_iq_tready;

  for (genvar spc_idx = 0; spc_idx < SPC; spc_idx++) begin: gen_iq_scaling
    AxiStreamIf #(
      .DATA_WIDTH (2*SCALING_WIDTH),
      .TUSER      (0),
      .TKEEP      (0),
      .USER_WIDTH (0)
    ) scale_factor_in (
      .clk (clk),
      .rst (reset)
    );

    AxiStreamIf #(
      .DATA_WIDTH (SAMP_W),
      .TUSER      (0),
      .TKEEP      (0),
      .USER_WIDTH (0)
    ) iq_in (
      .clk (clk),
      .rst (reset)
    );

    AxiStreamIf #(
      .DATA_WIDTH (SAMP_W),
      .TUSER      (0),
      .TKEEP      (0),
      .USER_WIDTH (0)
    ) iq_scaled_out (
      .clk (clk),
      .rst (reset)
    );

    // Convert real scale factor to complex format {q, i}
    assign scale_factor_in.tdata     = {{SCALING_WIDTH{1'b0}}, scale_iq_latched};
    assign scale_factor_in.tvalid    = 1'b1;
    assign scale_factor_in.tlast     = 1'b0;

    // Flip i & q to match the expected input format of complex_multiply_iq
    assign iq_in.tdata               = {
      hbf_out_tdata[(spc_idx*SAMP_W) +: SAMP_W/2],           //Q
      hbf_out_tdata[(spc_idx*SAMP_W)+(SAMP_W/2) +: SAMP_W/2] //I
    };
    assign iq_in.tvalid              = hbf_out_tvalid;
    assign iq_in.tlast               = hbf_out_tlast;

    assign scale_iq_tready[spc_idx] = iq_in.tready && scale_factor_in.tready;
    assign iq_scaled_out.tready      = sample_out_tready;

    complex_multiply_iq #(
      .FRACTIONAL_BITS_A       ((SAMP_W/2)-1),
      .FRACTIONAL_BITS_B       (SCALE_FRAC_W),
      .FRACTIONAL_BITS_PRODUCT ((SAMP_W/2)-1)
    ) complex_multiply_iq_i (
      .factor_a (iq_in),
      .factor_b (scale_factor_in),
      .product  (iq_scaled_out)
    );

    // Switch back the order of I and Q to match the output format {i, q}
    assign sample_out_tdata[spc_idx] = { << (SAMP_W/2) {iq_scaled_out.tdata}};
    assign scale_iq_tlast[spc_idx]  = iq_scaled_out.tlast;
    assign scale_iq_tvalid[spc_idx] = iq_scaled_out.tvalid;

  end

  assign hbf_out_tready    = &scale_iq_tready; // backpressure HBF
  assign sample_out_tlast  = &scale_iq_tlast;
  assign sample_out_tvalid = &scale_iq_tvalid;

endmodule

`default_nettype wire
