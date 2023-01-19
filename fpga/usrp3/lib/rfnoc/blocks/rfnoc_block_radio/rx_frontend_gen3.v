//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2020 Ettus Research, a National Instruments Brand
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rx_frontend_gen3
//
// Description:
//
//   RX Frontend Correction Module
//   -----------------------------
//
//   This module will perform the following modifications of the signal, in this
//   order:
//
//   1) I/Q Swapping/Reordering: The I and Q values from the ADC can be remapped
//      arbitrarily and can be inverted. The behaviour of this IQ mux is
//      controlled by writing to the settings register at SR_IQ_MAPPING.
//
//      This register uses the following bits:
//
//      Bit 0:  Set to 1 to swap I and Q.
//      Bit 1:  Set to 1 to enable real mode. If it is 1, then
//              the Q input signal is ignored and assumed to be zero.
//      Bit 2:  Set to 1 to invert the Q input signal
//      Bit 3:  Set to 1 to invert the I input signal
//      Bit 4:  Set to 1 to enable the quarter-rate downconverter (only relevant
//              when BYPASS_HETERODYNE is set to 0, see below).
//      Bit 7:  Disable all corrections in this module.
//
//   2) DC offset correction. See the rx_dcoffset module for details. This is
//      either a fixed DC offset, or a notch filter around DC. The behaviour of
//      this correction step is controlled by writing to settings registers at
//      SR_OFFSET_I and SR_OFFSET_Q (they get forwarded to rx_dcoffset).
//      Set BYPASS_DC_OFFSET_CORR to 1 to not synthesize this step.
//
//   3) IQ imbalance correction. This implements a simple, one-shot IQ imbalance
//      correction. It will modify the I and Q signals as follows:
//           _  _     _         _   _   _
//          | I' |   | A/64+1  0 | |  I  |
//          |    | = |           | |     |
//          | Q' |   | B/64    1 | |  Q  |
//           ‾  ‾     ‾         ‾   ‾   ‾
//
//      Here, A is the value written to the register at SR_MAG_CORRECTION, and
//      B is the value written to the register at SR_PHASE_CORRECTION.
//      Set BYPASS_IQ_COMP to 1 to not synthesize this step.
//
//   4) Heterodyne conversion. The converter is only enabled when the
//      "downconvert" bit in the SR_IQ_MAPPING register is asserted. In this
//      case, it enables a quarter-rate mixer. The direction of this mixer is
//      controlled by the SR_HET_PHASE_INCR register (a 0 in this register
//      rotates by pi/2 every clock cycle, a 1 in this register rotates by -pi/2).
//
//      The mixer is followed by a non-decimating FIR filter.
//
//      Set BYPASS_HETERODYNE to 1 to not synthesize this step.
//
//
// UHD Developers Note: This module is typically controlled by rx_frontend_core_3000 in UHD,
// and is also described in the calibration.dox manual page. When modifying this file, make
// sure to also modify those files if necessary.
//
// Parameters:
//   SR_MAG_CORRECTION     : Settings register address for the IQ correction "MAG" value
//   SR_PHASE_CORRECTION   : Settings register address for the IQ correction "PHASE" value
//   SR_OFFSET_I           : Settings register address for DC offset I correction
//                           value (goes to rx_dcoffset)
//   SR_OFFSET_Q           : Settings register address for DC offset Q correction
//                           value (goes to rx_dcoffset)
//   SR_IQ_MAPPING         : Settings register address for IQ mapping value
//   SR_HET_PHASE_INCR     : Settings register address for the real mode phase
//                           increment value
//   BYPASS_DC_OFFSET_CORR : Set to 1 to disable DC offset correction
//   BYPASS_IQ_COMP        : Set to 1 to disable IQ offset correction
//   BYPASS_HETERODYNE     : Set to 1 to disable heterodyne conversion
//   DEVICE                : Unused.
//
module rx_frontend_gen3 #(
  parameter SR_MAG_CORRECTION = 0,
  parameter SR_PHASE_CORRECTION = 1,
  parameter SR_OFFSET_I = 2,
  parameter SR_OFFSET_Q = 3,
  parameter SR_IQ_MAPPING = 4,
  parameter SR_HET_PHASE_INCR = 5,
  parameter BYPASS_DC_OFFSET_CORR = 0,
  parameter BYPASS_IQ_COMP = 0,
  parameter BYPASS_HETERODYNE = 0,
  parameter DEVICE = "7SERIES"
)(
  input clk, input reset, input sync_in,
  input set_stb, input [7:0] set_addr, input [31:0] set_data,
  input adc_stb, input [15:0] adc_i, input [15:0] adc_q,
  output rx_stb, output [15:0] rx_i, output [15:0] rx_q
);

  wire               realmode;
  wire               swap_iq;
  wire               invert_i;
  wire               invert_q;
  wire               downconvert;
  wire               bypass_all;
  wire [1:0]         iq_map_reserved;
  wire [17:0]        mag_corr, phase_corr;
  wire               phase_dir;
  wire               phase_sync;

  reg  [23:0]        adc_i_mux, adc_q_mux;
  reg                adc_mux_stb;
  wire [23:0]        adc_i_ofs, adc_q_ofs, adc_i_comp, adc_q_comp;
  wire               adc_ofs_stb, adc_comp_stb;
  reg  [1:0]         adc_ofs_stb_dly;
  wire [23:0]        adc_i_dsp, adc_q_dsp;
  wire               adc_dsp_stb;
  wire [15:0]        rx_i_out, rx_q_out;

  /********************************************************
  ** Settings Bus Registers
  ********************************************************/
  setting_reg #(.my_addr(SR_MAG_CORRECTION),.width(18)) sr_mag_corr (
    .clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
    .in(set_data),.out(mag_corr),.changed());

  setting_reg #(.my_addr(SR_PHASE_CORRECTION),.width(18)) sr_phase_corr (
    .clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
    .in(set_data),.out(phase_corr),.changed());

  setting_reg #(.my_addr(SR_IQ_MAPPING), .width(8)) sr_mux_sel (
    .clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
    .in(set_data),
    .out({bypass_all,iq_map_reserved,downconvert,invert_i,invert_q,realmode,swap_iq}),
    .changed());

  // Setting reg: 1 bit to set phase direction: default to 0:
  //   direction bit == 0: the phase is increased by pi/2 (counter clockwise)
  //   direction bit == 1: the phase is increased by -pi/2 (clockwise)
  setting_reg #(.my_addr(SR_HET_PHASE_INCR), .width(1)) sr_phase_dir (
    .clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
    .in(set_data),.out(phase_dir),.changed(phase_sync));

  /********************************************************
  ** IQ Mapping (swapping, inversion, real-mode)
  ********************************************************/
  // MUX so we can do realmode signals on either input
  always @(posedge clk) begin
    if (swap_iq) begin
      adc_i_mux[23:8] <= invert_q ? ~adc_q   : adc_q;
      adc_q_mux[23:8] <= realmode ? 16'd0 : invert_i ? ~adc_i : adc_i;
    end else begin
      adc_i_mux[23:8] <= invert_i ? ~adc_i   : adc_i;
      adc_q_mux[23:8] <= realmode ? 16'd0 : invert_q ? ~adc_q : adc_q;
    end
    adc_mux_stb <= adc_stb;
    adc_i_mux[7:0] <= 8'd0;
    adc_q_mux[7:0] <= 8'd0;
  end

  /********************************************************
  ** DC offset Correction
  ********************************************************/
  generate
    if (BYPASS_DC_OFFSET_CORR == 0) begin

      rx_dcoffset #(.WIDTH(24),.ADDR(SR_OFFSET_I)) rx_dcoffset_i (
        .clk(clk),.rst(reset),.set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
        .in_stb(adc_mux_stb),.in(adc_i_mux),
        .out_stb(adc_ofs_stb),.out(adc_i_ofs));
      rx_dcoffset #(.WIDTH(24),.ADDR(SR_OFFSET_Q)) rx_dcoffset_q (
        .clk(clk),.rst(reset),.set_stb(set_stb),.set_addr(set_addr),.set_data(set_data),
        .in_stb(adc_mux_stb),.in(adc_q_mux),
        .out_stb(),.out(adc_q_ofs));

    end else begin
      assign adc_ofs_stb = adc_mux_stb;
      assign adc_i_ofs   = adc_i_mux;
      assign adc_q_ofs   = adc_q_mux;
    end
  endgenerate

  /********************************************************
  ** IQ Imbalance Compensation
  ********************************************************/
  generate
    if (BYPASS_IQ_COMP == 0) begin

      mult_add_clip #(
        .WIDTH_A(18),
        .BIN_PT_A(17),
        .WIDTH_B(18),
        .BIN_PT_B(17),
        .WIDTH_C(24),
        .BIN_PT_C(23),
        .WIDTH_O(24),
        .BIN_PT_O(23),
        .LATENCY(2)
      ) mult_i (
        .clk(clk),
        .reset(reset),
        .CE(1'b1),
        .A(adc_i_ofs[23:6]),
        .B(mag_corr),
        .C(adc_i_ofs),
        .O(adc_i_comp)
      );

      mult_add_clip #(
        .WIDTH_A(18),
        .BIN_PT_A(17),
        .WIDTH_B(18),
        .BIN_PT_B(17),
        .WIDTH_C(24),
        .BIN_PT_C(23),
        .WIDTH_O(24),
        .BIN_PT_O(23),
        .LATENCY(2)
      ) mult_q (
        .clk(clk),
        .reset(reset),
        .CE(1'b1),
        .A(adc_i_ofs[23:6]),
        .B(phase_corr),
        .C(adc_q_ofs),
        .O(adc_q_comp)
      );

      // Delay to match path latencies
      always @(posedge clk) begin
        if (reset) begin
          adc_ofs_stb_dly <= 2'b0;
        end else begin
          adc_ofs_stb_dly <= {adc_ofs_stb_dly[0], adc_ofs_stb};
        end
      end

      assign adc_comp_stb = adc_ofs_stb_dly[1];

    end else begin
      assign adc_comp_stb = adc_ofs_stb;
      assign adc_i_comp   = adc_i_ofs;
      assign adc_q_comp   = adc_q_ofs;
    end
  endgenerate

  /********************************************************
  ** Realmode DSP:
  *  - Heterodyne frequency translation
  *  - Realmode decimation (by 2)
  ********************************************************/
  generate
    if (BYPASS_HETERODYNE == 0) begin

      wire [23:0] adc_i_dsp_cout, adc_q_dsp_cout;
      wire [23:0] adc_i_filt, adc_q_filt;
      wire        adc_dsp_cout_stb;
      wire        adc_filt_stb;

      // 90 degree mixer
      quarter_rate_downconverter #(.WIDTH(24)) qr_dc_i(
        .clk(clk), .reset(reset || sync_in), .phase_sync(phase_sync),
        .i_tdata({adc_i_comp, adc_q_comp}), .i_tlast(1'b1), .i_tvalid(adc_comp_stb), .i_tready(),
        .o_tdata({adc_i_dsp_cout, adc_q_dsp_cout}), .o_tlast(), .o_tvalid(adc_dsp_cout_stb), .o_tready(1'b1),
        .dirctn(phase_dir));

      // Double FIR block
      localparam HB_COEFS = {-18'd62, 18'd0, 18'd194, 18'd0, -18'd440, 18'd0, 18'd855, 18'd0, -18'd1505, 18'd0, 18'd2478, 18'd0,
        -18'd3900, 18'd0, 18'd5990, 18'd0, -18'd9187, 18'd0, 18'd14632, 18'd0, -18'd26536, 18'd0, 18'd83009, 18'd131071, 18'd83009,
        18'd0, -18'd26536, 18'd0, 18'd14632, 18'd0, -18'd9187, 18'd0, 18'd5990, 18'd0, -18'd3900, 18'd0, 18'd2478, 18'd0, -18'd1505,
        18'd0, 18'd855, 18'd0, -18'd440, 18'd0, 18'd194, 18'd0, -18'd62};

      // FIR filter for real part
      axi_fir_filter #(.IN_WIDTH(24), .COEFF_WIDTH(18), .OUT_WIDTH(24), .NUM_COEFFS(47), .COEFFS_VEC(HB_COEFS),
        .RELOADABLE_COEFFS(0), .BLANK_OUTPUT(0), .SYMMETRIC_COEFFS(1), .SKIP_ZERO_COEFFS(1), .USE_EMBEDDED_REGS_COEFFS(0)
      ) hbfir0(
        .clk(clk),
        .reset(reset),
        .clear(reset),
        .s_axis_data_tdata(adc_i_dsp_cout),
        .s_axis_data_tlast(1'b1),
        .s_axis_data_tvalid(adc_dsp_cout_stb),
        .s_axis_data_tready(),
        .m_axis_data_tdata(adc_i_filt),
        .m_axis_data_tlast(),
        .m_axis_data_tvalid(adc_filt_stb),
        .m_axis_data_tready(1'b1),
        .s_axis_reload_tdata(18'd0),
        .s_axis_reload_tvalid(1'b0),
        .s_axis_reload_tlast(1'b0),
        .s_axis_reload_tready()
      );

      // FIR filter for imag. part
      axi_fir_filter #(.IN_WIDTH(24), .COEFF_WIDTH(18), .OUT_WIDTH(24), .NUM_COEFFS(47), .COEFFS_VEC(HB_COEFS),
        .RELOADABLE_COEFFS(0), .BLANK_OUTPUT(0), .SYMMETRIC_COEFFS(1), .SKIP_ZERO_COEFFS(1), .USE_EMBEDDED_REGS_COEFFS(0)
      ) hbfir1(
        .clk(clk),
        .reset(reset),
        .clear(reset),
        .s_axis_data_tdata(adc_q_dsp_cout),
        .s_axis_data_tlast(1'b1),
        .s_axis_data_tvalid(adc_dsp_cout_stb),
        .s_axis_data_tready(),
        .m_axis_data_tdata(adc_q_filt),
        .m_axis_data_tlast(),
        .m_axis_data_tvalid(),
        .m_axis_data_tready(1'b1),
        .s_axis_reload_tdata(18'd0),
        .s_axis_reload_tvalid(1'b0),
        .s_axis_reload_tlast(1'b0),
        .s_axis_reload_tready()
      );

      assign adc_dsp_stb = downconvert ? adc_filt_stb : adc_comp_stb;
      assign adc_i_dsp   = downconvert ? adc_i_filt : adc_i_comp;
      assign adc_q_dsp   = downconvert ? adc_q_filt : adc_q_comp;

    end else begin
      assign adc_dsp_stb = adc_comp_stb;
      assign adc_i_dsp   = adc_i_comp;
      assign adc_q_dsp   = adc_q_comp;
    end
  endgenerate

  // Round to short complex (sc16)
  round_sd #(.WIDTH_IN(24),.WIDTH_OUT(16)) round_i (
    .clk(clk),.reset(reset), .in(adc_i_dsp),.strobe_in(adc_dsp_stb), .out(rx_i_out), .strobe_out(rx_stb));
  round_sd #(.WIDTH_IN(24),.WIDTH_OUT(16)) round_q (
    .clk(clk),.reset(reset), .in(adc_q_dsp),.strobe_in(adc_dsp_stb), .out(rx_q_out), .strobe_out());

  assign rx_i = bypass_all ? adc_i : rx_i_out;
  assign rx_q = bypass_all ? adc_q : rx_q_out;

endmodule
