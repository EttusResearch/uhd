//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: duc_ms
//
// Description:
//   DUC signal processing module composed of the
//
//     - HBF Cascade
//     - CIC Filter
//     - IQ scaling
//
//
// Parameters:
//   SPC           : Number of samples processed per clock cycle.
//                   This determines the width of the input and output data streams,
//                   which are SAMP_W*SPC bits wide.
//                   The DUC will process SPC samples in parallel every clock cycle.
//   NUM_HB        : Number of halfband filter stages to implement in each DUC chain.
//                   This determines the maximum interpolation rate of the halfband filters,
//                   which is 2^NUM_HB.
//   CIC_MAX_INTERP: Maximum interpolation rate of the CIC filter in each DUC chain.
//                   The total maximum interpolation rate of the DUC is
//                   CIC_MAX_INTERP * 2^NUM_HB.
//   SAMP_W        : Width of a  I+Q sample.
//
//

`default_nettype none

module duc_ms
  import ctrlport_pkg::*;
#(
  parameter int SPC             = 1,
  parameter int NUM_HB          = 3,
  parameter int CIC_MAX_INTERP  = 255,
  parameter int SAMP_W          = 32
) (
  input  wire clk,
  input  wire reset,
  input  wire clear,

  // CTRL port requests
  input  wire                          s_ctrlport_req_wr,
  input  wire                          s_ctrlport_req_rd,
  input  wire [   CTRLPORT_ADDR_W-1:0] s_ctrlport_req_addr,
  input  wire [   CTRLPORT_DATA_W-1:0] s_ctrlport_req_data,
  input  wire [CTRLPORT_BYTE_EN_W-1:0] s_ctrlport_req_byte_en,
  input  wire                          s_ctrlport_req_has_time,
  input  wire [   CTRLPORT_TIME_W-1:0] s_ctrlport_req_time,
  output logic                         s_ctrlport_resp_ack,
  output logic [    CTRLPORT_STS_W-1:0] s_ctrlport_resp_status,
  output logic [   CTRLPORT_DATA_W-1:0] s_ctrlport_resp_data,

  // Sample stream
  input  wire [SPC-1:0][SAMP_W-1:0]  sample_in_tdata,
  input  wire                        sample_in_tvalid,
  input  wire                        sample_in_tlast,
  output wire                        sample_in_tready,
  output wire [SPC-1:0][SAMP_W-1:0]  sample_out_tdata,
  output wire                        sample_out_tvalid,
  input  wire                        sample_out_tready,
  output wire                        sample_out_tlast
);
  import duc_ms_regs_pkg::*;

  //---------------------------------------------------------------------------
  // Localparams
  //---------------------------------------------------------------------------
  // Scale factor of signed #Q2.15 format
  localparam int SCALING_WIDTH = REG_SR_SCALE_IQ_W;
  localparam int SCALE_FRAC_W  = SCALING_WIDTH - 3;


  //---------------------------------------------------------------------------
  // Handle SR register accesses directly in this block (no separate SRC module)
  //---------------------------------------------------------------------------
  logic [    SCALING_WIDTH-1:0] reg_scale_iq;
  logic [  REG_SR_INTERP_W-1:0] reg_interp;
  logic [REG_SR_CIC_RATE_W-1:0] cic_rate;
  logic [   REG_SR_HB_EN_W-1:0] hb_enables;

  always_ff @(posedge clk) begin
    if (reset) begin
      s_ctrlport_resp_ack    <= 1'b0;
      s_ctrlport_resp_status <= CTRL_STS_OKAY;
      s_ctrlport_resp_data   <= 32'h0;
      reg_scale_iq           <= {SCALING_WIDTH{1'b0}};
      reg_interp             <= REG_SR_INTERP_W'(1); // hb_rate=0, cic_interp_rate=1
    end else begin
      s_ctrlport_resp_ack  <= 1'b0;
      s_ctrlport_resp_status <= CTRL_STS_OKAY;
      s_ctrlport_resp_data <= 32'h0;
      if (s_ctrlport_req_wr) begin
        case (s_ctrlport_req_addr)
          REG_SR_INTERP_ADDR: begin
            reg_interp <= s_ctrlport_req_data[REG_SR_INTERP_W-1:0];
            s_ctrlport_resp_ack <= 1'b1;
          end
          REG_SR_MUX_ADDR: begin
            // Legacy register for IQ swap & realmode,
            // functionality no longer required for MS DUC.
            // Keeping register for backwards compatibility,
            // but return an error if written to.
            s_ctrlport_resp_status <= CTRL_STS_CMDERR;
            s_ctrlport_resp_ack <= 1'b1;
          end
          REG_SR_SCALE_IQ_ADDR: begin
            reg_scale_iq <= s_ctrlport_req_data[REG_SR_SCALE_IQ_W-1:0];
            s_ctrlport_resp_ack <= 1'b1;
          end
          default: begin
            s_ctrlport_resp_status <= CTRL_STS_CMDERR; // unsupported address
            s_ctrlport_resp_ack  <= 1'b1;
          end
        endcase
      end
      if (s_ctrlport_req_rd) begin
        case (s_ctrlport_req_addr)
          REG_SR_INTERP_ADDR: begin
            s_ctrlport_resp_data <= reg_interp;
            s_ctrlport_resp_ack  <= 1'b1;
          end
          REG_SR_MUX_ADDR: begin
            // Legacy register for IQ swap & realmode,
            // functionality no longer required for MS DUC.
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
            s_ctrlport_resp_ack  <= 1'b1;
          end
        endcase
      end
    end
  end

  //-------------------------------------------------------------------------
  // AXI stream interfaces between DUC DSP components
  //-------------------------------------------------------------------------
  // HBF output → CIC input (plain logic; cic_in is driven from these after the HBF instance)
  logic                       hbf_out_tvalid, hbf_out_tready, hbf_out_tlast;
  logic [SPC-1:0][SAMP_W-1:0] hbf_out_tdata;
  // CIC
  AxiStreamIf #(SPC*SAMP_W) cic_in (
    .clk    (clk),
    .rst    (reset)
  );

  AxiStreamIf #(SPC*SAMP_W) cic_out (
    .clk    (clk),
    .rst    (reset)
  );


  //-------------------------------------------------------------------------
  // Latch configuration during burst processing to avoid corrupted data caused
  // by changing the interpolation rate or scaling factor mid-burst.
  //-------------------------------------------------------------------------
  logic [REG_SR_INTERP_W-1:0] interp_rate_latched;
  logic [  SCALING_WIDTH-1:0] scale_iq_latched;
  logic                       latched_rate_changed;
  logic                       active_burst;

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
      interp_rate_latched <= REG_SR_INTERP_W'(1);
      scale_iq_latched    <= 1 << SCALE_FRAC_W; // 1.0 in Q2.15 format
    end else if (clear || !active_burst) begin
      scale_iq_latched <= reg_scale_iq;
      interp_rate_latched <= reg_interp;
      if (clear || (reg_interp != interp_rate_latched)) begin
        latched_rate_changed <= 1'b1;
      end
    end
  end

  assign cic_rate   = interp_rate_latched[7:0];
  assign hb_enables = interp_rate_latched[9:8];



  // -------------------------------------------------------------------------
  // HBF cascade interpolator
  // -------------------------------------------------------------------------
  axis_hb_cascade_intp #(
    .SAMP_W         (SAMP_W),
    .SPC            (SPC),
    .NUM_HB         (NUM_HB),
    .PRELOAD_ZEROES (1)
  ) hbf_cascade_i (
    .clk           (clk),
    .rst           (reset),
    .clear         (clear),
    .s_axis_tdata  (sample_in_tdata),
    .s_axis_tvalid (sample_in_tvalid),
    .s_axis_tready (sample_in_tready),
    .s_axis_tlast  (sample_in_tlast),
    .m_axis_tdata  (hbf_out_tdata),
    .m_axis_tvalid (hbf_out_tvalid),
    .m_axis_tready (hbf_out_tready),
    .m_axis_tlast  (hbf_out_tlast),
    .num_stages    (hb_enables)
  );

  // Connect HBF output to CIC input.
  assign cic_in.tdata   = hbf_out_tdata;
  assign cic_in.tvalid  = hbf_out_tvalid;
  assign cic_in.tlast   = hbf_out_tlast;
  assign hbf_out_tready = cic_in.tready;

  //-------------------------------------------------------------------------
  // CIC interpolating filter
  //-------------------------------------------------------------------------
  if (CIC_MAX_INTERP > 0) begin : gen_cic_filter_interp
    cic_filter_interp #(
      .SPC       (SPC),
      .SAMP_W    (SAMP_W),
      .MAX_INTERP(CIC_MAX_INTERP),
      .ORDER     (4)
    ) cic_filter_interp_i (
      .clk            (clk),
      .rst            (reset),
      .clear          (clear),
      .data_in        (cic_in),
      .data_out       (cic_out),
      .interp_factor  (cic_rate),
      .config_changed (latched_rate_changed)
    );
  end else begin : gen_cic_filter_bypass
    // Bypass the CIC filter if CIC_MAX_INTERP is set to 0 or lower.
    assign cic_out.tdata  = cic_in.tdata;
    assign cic_out.tvalid = cic_in.tvalid;
    assign cic_out.tlast  = cic_in.tlast;
    assign cic_in.tready  = cic_out.tready;
  end

  // Unpack CIC output tdata into a per-sample 2D array for the scaler.
  logic [SPC-1:0][SAMP_W-1:0] cic_out_tdata_arr;
  assign cic_out_tdata_arr = cic_out.tdata;

  //-------------------------------------------------------------------------
  // Scale the interpolated IQ samples by the latched scaling factor.
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
      cic_out_tdata_arr[spc_idx][SAMP_W/2-1:0],     //Q
      cic_out_tdata_arr[spc_idx][SAMP_W-1:SAMP_W/2] //I
    };
    assign iq_in.tvalid              = cic_out.tvalid;
    assign iq_in.tlast               = cic_out.tlast;

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
    assign sample_out_tdata[spc_idx] = {
      iq_scaled_out.tdata[(SAMP_W/2)-1 : 0],     //I
      iq_scaled_out.tdata[SAMP_W-1 : (SAMP_W/2)] //Q
    };
    assign scale_iq_tlast[spc_idx]  = iq_scaled_out.tlast;
    assign scale_iq_tvalid[spc_idx] = iq_scaled_out.tvalid;

  end

  assign cic_out.tready    = &scale_iq_tready; // backpressure CIC output
  assign sample_out_tlast  = &scale_iq_tlast;
  assign sample_out_tvalid = &scale_iq_tvalid;

endmodule

`default_nettype wire
