//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_siggen_core
//
// Description:
//
//   This module contains the registers and core logic for a single RFNoC
//   Signal Generator module instance.
//


module rfnoc_siggen_core (
  input wire clk,
  input wire rst,

  // CtrlPort Slave
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  output reg         s_ctrlport_resp_ack,
  output reg  [31:0] s_ctrlport_resp_data,

  // Output data stream
  output wire [31:0] m_tdata,
  output wire        m_tlast,
  output wire        m_tvalid,
  input  wire        m_tready,
  output wire [15:0] m_tlength
);

  `include "rfnoc_block_siggen_regs.vh"


  //---------------------------------------------------------------------------
  // Registers
  //---------------------------------------------------------------------------

  // Define maximum fixed point value for the gain, equal to about 0.9999
  localparam MAX_GAIN = {REG_GAIN_LEN-1{1'b1}};

  reg [   REG_ENABLE_LEN-1:0] reg_enable    = 0;
  reg [      REG_SPP_LEN-1:0] reg_spp       = 16;
  reg [ REG_WAVEFORM_LEN-1:0] reg_waveform  = WAVE_CONST;
  reg [     REG_GAIN_LEN-1:0] reg_gain      = MAX_GAIN;
  reg [ REG_CONSTANT_LEN-1:0] reg_constant  = 0;
  reg [REG_PHASE_INC_LEN-1:0] reg_phase_inc;
  reg [REG_CARTESIAN_LEN-1:0] reg_cartesian;

  reg reg_phase_inc_stb;
  reg reg_cartesian_stb;

  always @(posedge clk) begin
    if (rst) begin
      reg_enable           <= 0;
      reg_spp              <= 16;
      reg_waveform         <= WAVE_CONST;
      reg_gain             <= MAX_GAIN;
      reg_constant         <= 0;
      reg_phase_inc        <= 'bX;
      reg_cartesian        <= 'bX;
      s_ctrlport_resp_ack  <= 1'b0;
      s_ctrlport_resp_data <= 'bX;
      reg_phase_inc_stb    <= 1'b0;
      reg_cartesian_stb    <= 1'b0;
    end else begin

      // Default assignments
      s_ctrlport_resp_ack  <= 1'b0;
      s_ctrlport_resp_data <= 0;
      reg_phase_inc_stb    <= 1'b0;
      reg_cartesian_stb    <= 1'b0;

      // Handle register writes
      if (s_ctrlport_req_wr) begin
        s_ctrlport_resp_ack <= 1;
        case (s_ctrlport_req_addr)
          REG_ENABLE    : reg_enable    <= s_ctrlport_req_data[REG_ENABLE_LEN-1:0];
          REG_SPP       : reg_spp       <= s_ctrlport_req_data[REG_SPP_LEN-1:0];
          REG_WAVEFORM  : reg_waveform  <= s_ctrlport_req_data[REG_WAVEFORM_LEN-1:0];
          REG_GAIN      : reg_gain      <= s_ctrlport_req_data[REG_GAIN_LEN-1:0];
          REG_CONSTANT  : reg_constant  <= s_ctrlport_req_data[REG_CONSTANT_LEN-1:0];
          REG_PHASE_INC : begin
            reg_phase_inc     <= s_ctrlport_req_data[REG_PHASE_INC_LEN-1:0];
            reg_phase_inc_stb <= 1'b1;
          end
          REG_CARTESIAN : begin
            reg_cartesian     <= s_ctrlport_req_data[REG_CARTESIAN_LEN-1:0];
            reg_cartesian_stb <= 1'b1;
          end
        endcase
      end

      // Handle register reads
      if (s_ctrlport_req_rd) begin
        s_ctrlport_resp_ack <= 1;
        case (s_ctrlport_req_addr)
          REG_ENABLE    : s_ctrlport_resp_data[REG_ENABLE_LEN-1:0]     <= reg_enable;
          REG_SPP       : s_ctrlport_resp_data[REG_SPP_LEN-1:0]        <= reg_spp;
          REG_WAVEFORM  : s_ctrlport_resp_data[REG_WAVEFORM_LEN-1:0]   <= reg_waveform;
          REG_GAIN      : s_ctrlport_resp_data[REG_GAIN_LEN-1:0]       <= reg_gain;
          REG_CONSTANT  : s_ctrlport_resp_data[REG_CONSTANT_LEN-1:0]   <= reg_constant;
          REG_PHASE_INC : s_ctrlport_resp_data[REG_PHASE_INC_LEN-1:0]  <= reg_phase_inc;
          REG_CARTESIAN : s_ctrlport_resp_data[REG_CARTESIAN_LEN-1:0]  <= reg_cartesian;
        endcase
      end
    end
  end


  //---------------------------------------------------------------------------
  // Waveform Generation
  //---------------------------------------------------------------------------

  wire [31:0]  axis_sine_tdata;
  wire         axis_sine_tvalid;
  wire         axis_sine_tready;
  wire [31:0]  axis_const_tdata;
  wire         axis_const_tvalid;
  wire         axis_const_tready;
  wire [31:0]  axis_noise_tdata;
  wire         axis_noise_tvalid;
  wire         axis_noise_tready;

  //------------------------------------
  // Sine waveform generation
  //------------------------------------

  // Convert the registers writes to settings bus transactions. Only one
  // register strobe will assert at a time.
  wire        sine_set_stb  = reg_cartesian_stb | reg_phase_inc_stb;
  wire [31:0] sine_set_data = reg_cartesian_stb ? reg_cartesian : reg_phase_inc;
  wire [ 7:0] sine_set_addr = reg_cartesian_stb;

  sine_tone #(
    .WIDTH             (32),
    .SR_PHASE_INC_ADDR (0),
    .SR_CARTESIAN_ADDR (1)
  ) sine_tone_i (
    .clk      (clk),
    .reset    (rst),
    .clear    (1'b0),
    .enable   (1'b1),
    .set_stb  (sine_set_stb),
    .set_data (sine_set_data),
    .set_addr (sine_set_addr),
    .o_tdata  (axis_sine_tdata),
    .o_tlast  (),
    .o_tvalid (axis_sine_tvalid),
    .o_tready (axis_sine_tready)
  );

  //------------------------------------
  // Constant waveform generation
  //------------------------------------

  assign axis_const_tdata  = reg_constant;
  assign axis_const_tvalid = 1'b1;

  //------------------------------------
  // Noise waveform generation
  //------------------------------------

  assign axis_noise_tvalid = 1'b1;

  // Random number generator
  rng rng_i (
    .clk (clk),
    .rst (rst),
    .out (axis_noise_tdata)
  );


  //---------------------------------------------------------------------------
  // Waveform Selection
  //---------------------------------------------------------------------------

  wire [31:0]  axis_mux_tdata;
  wire         axis_mux_tvalid;
  wire         axis_mux_tready;

  axi_mux_select #(
    .WIDTH          (32),
    .SIZE           (3),
    .SWITCH_ON_LAST (0)
  ) axi_mux_select_i (
    .clk      (clk),
    .reset    (rst),
    .clear    (1'b0),
    .select   (reg_waveform),
    .i_tdata  ({axis_noise_tdata, axis_sine_tdata, axis_const_tdata}),
    .i_tlast  ({3'd0}),   // Length controlled by SPP register
    .i_tvalid ({axis_noise_tvalid, axis_sine_tvalid, axis_const_tvalid}),
    .i_tready ({axis_noise_tready, axis_sine_tready, axis_const_tready}),
    .o_tdata  (axis_mux_tdata),
    .o_tlast  (),
    .o_tvalid (axis_mux_tvalid),
    .o_tready (axis_mux_tready)
  );


  //---------------------------------------------------------------------------
  // Gain
  //---------------------------------------------------------------------------

  wire [63:0]  axis_gain_tdata;
  wire         axis_gain_tvalid;
  wire         axis_gain_tready;
  wire [31:0]  axis_round_tdata;
  wire         axis_round_tvalid;
  wire         axis_round_tready;

  mult_rc #(
    .WIDTH_REAL (16),
    .WIDTH_CPLX (16),
    .WIDTH_P    (32),
    .DROP_TOP_P (5),
    .LATENCY    (4)
  ) mult_rc_i (
    .clk         (clk),
    .reset       (rst),
    .real_tdata  (reg_gain),
    .real_tlast  (1'b0),
    .real_tvalid (1'b1),
    .real_tready (),
    .cplx_tdata  (axis_mux_tdata),
    .cplx_tlast  (1'b0),
    .cplx_tvalid (axis_mux_tvalid),
    .cplx_tready (axis_mux_tready),
    .p_tdata     (axis_gain_tdata),
    .p_tlast     (),
    .p_tvalid    (axis_gain_tvalid),
    .p_tready    (axis_gain_tready)
  );

  axi_round_and_clip_complex #(
    .WIDTH_IN  (32),
    .WIDTH_OUT (16),
    .CLIP_BITS (1)
  ) axi_round_and_clip_complex_i (
    .clk      (clk),
    .reset    (rst),
    .i_tdata  (axis_gain_tdata),
    .i_tlast  (1'b0),
    .i_tvalid (axis_gain_tvalid),
    .i_tready (axis_gain_tready),
    .o_tdata  (axis_round_tdata),
    .o_tlast  (),
    .o_tvalid (axis_round_tvalid),
    .o_tready (axis_round_tready)
  );


  //---------------------------------------------------------------------------
  // Packet Length Control
  //---------------------------------------------------------------------------

  wire [REG_SPP_LEN-1:0] m_tlength_samples;

  assign m_tlength = { m_tlength_samples, 2'b0 };   // 4 bytes per sample

  axis_packetize #(
    .DATA_W  (32),
    .SIZE_W  (REG_SPP_LEN),
    .FLUSH   (1)
  ) axis_packetize_i (
    .clk      (clk),
    .rst      (rst),
    .gate     (~reg_enable),
    .size     (reg_spp),
    .i_tdata  (axis_round_tdata),
    .i_tvalid (axis_round_tvalid),
    .i_tready (axis_round_tready),
    .o_tdata  (m_tdata),
    .o_tlast  (m_tlast),
    .o_tvalid (m_tvalid),
    .o_tready (m_tready),
    .o_tuser  (m_tlength_samples)
  );

endmodule
