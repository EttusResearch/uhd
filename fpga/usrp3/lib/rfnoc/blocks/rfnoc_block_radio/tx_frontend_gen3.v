//
// Copyright 2015 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module tx_frontend_gen3 #(
  parameter SR_OFFSET_I = 0,
  parameter SR_OFFSET_Q = 1,
  parameter SR_MAG_CORRECTION = 2,
  parameter SR_PHASE_CORRECTION = 3,
  parameter SR_MUX = 4,
  parameter BYPASS_DC_OFFSET_CORR = 0,
  parameter BYPASS_IQ_COMP = 0,
  parameter DEVICE = "7SERIES"
)(
  input clk, input reset,
  input set_stb, input [7:0] set_addr, input [31:0] set_data,
  input tx_stb, input [15:0] tx_i, input [15:0] tx_q,
  output reg dac_stb, output reg [15:0] dac_i, output reg [15:0] dac_q
);

  wire [23:0]        i_dco, q_dco;
  wire [7:0]         mux_ctrl;
  wire [17:0]        mag_corr, phase_corr;

  wire [35:0]        corr_i, corr_q;
  reg  [1:0]         tx_stb_dly;
  reg  [23:0]        tx_i_dly, tx_q_dly;
  wire               tx_comp_stb, tx_ofs_stb;
  wire [23:0]        tx_i_comp, tx_q_comp, tx_i_ofs, tx_q_ofs;
  wire               tx_round_stb;
  wire [15:0]        tx_i_round, tx_q_round;

  /********************************************************
  ** Settings Registers
  ********************************************************/
  setting_reg #(.my_addr(SR_OFFSET_I), .width(24)) sr_i_dc_offset (
    .clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
    .in(set_data),.out(i_dco),.changed());

  setting_reg #(.my_addr(SR_OFFSET_Q), .width(24)) sr_q_dc_offset (
    .clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
    .in(set_data),.out(q_dco),.changed());

  setting_reg #(.my_addr(SR_MAG_CORRECTION),.width(18)) sr_mag_corr (
    .clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
    .in(set_data),.out(mag_corr),.changed());

  setting_reg #(.my_addr(SR_PHASE_CORRECTION),.width(18)) sr_phase_corr (
    .clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
    .in(set_data),.out(phase_corr),.changed());

  setting_reg #(.my_addr(SR_MUX), .width(8), .at_reset(8'h10)) sr_mux_ctrl (
    .clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
    .in(set_data),.out(mux_ctrl),.changed());

  /********************************************************
  ** DSP
  ********************************************************/
  // I/Q compensation with option to bypass
  generate
    if (BYPASS_IQ_COMP == 0) begin

      mult_add_clip #(
        .WIDTH_A(16),
        .BIN_PT_A(15),
        .WIDTH_B(18),
        .BIN_PT_B(17),
        .WIDTH_C(16),
        .BIN_PT_C(15),
        .WIDTH_O(24),
        .BIN_PT_O(23),
        .LATENCY(2)
      ) mult_i (
        .clk(clk),
        .reset(reset),
        .CE(1'b1),
        .A(tx_i),
        .B(mag_corr),
        .C(tx_i),
        .O(tx_i_comp)
      );

      mult_add_clip #(
        .WIDTH_A(16),
        .BIN_PT_A(15),
        .WIDTH_B(18),
        .BIN_PT_B(17),
        .WIDTH_C(16),
        .BIN_PT_C(15),
        .WIDTH_O(24),
        .BIN_PT_O(23),
        .LATENCY(2)
      ) mult_q (
        .clk(clk),
        .reset(reset),
        .CE(1'b1),
        .A(tx_i),
        .B(phase_corr),
        .C(tx_q),
        .O(tx_q_comp)
      );

      // Delay to match path latencies
      always @(posedge clk) begin
        if (reset) begin
          tx_stb_dly <= 2'b0;
        end else begin
          tx_stb_dly <= {tx_stb_dly[0], tx_stb};
        end
      end

      assign tx_comp_stb = tx_stb_dly[1];

    end else begin
      assign tx_comp_stb = tx_stb;
      assign tx_i_comp = {tx_i,8'd0};
      assign tx_q_comp = {tx_q,8'd0};
    end
  endgenerate

  // DC offset correction
  generate
    if (BYPASS_DC_OFFSET_CORR == 0) begin
      add2_and_clip_reg #(.WIDTH(24)) add_dco_i (
        .clk(clk), .rst(reset), .in1(i_dco), .in2(tx_i_comp), .strobe_in(tx_comp_stb), .sum(tx_i_ofs), .strobe_out(tx_ofs_stb));
      add2_and_clip_reg #(.WIDTH(24)) add_dco_q (
        .clk(clk), .rst(reset), .in1(q_dco), .in2(tx_q_comp), .strobe_in(tx_comp_stb), .sum(tx_q_ofs), .strobe_out());
    end else begin
      assign tx_ofs_stb = tx_comp_stb;
      assign tx_i_ofs = tx_i_comp;
      assign tx_q_ofs = tx_q_comp;
    end
  endgenerate

  // Round to short complex (sc16)
  round_sd #(.WIDTH_IN(24),.WIDTH_OUT(16)) round_i (
    .clk(clk),.reset(reset), .in(tx_i_ofs),.strobe_in(tx_ofs_stb), .out(tx_i_round), .strobe_out(tx_round_stb));
  round_sd #(.WIDTH_IN(24),.WIDTH_OUT(16)) round_q (
    .clk(clk),.reset(reset), .in(tx_q_ofs),.strobe_in(tx_ofs_stb), .out(tx_q_round), .strobe_out());

  // Mux
  // Muxing logic matches that in tx_frontend.v, and what tx_frontend_core_200.cpp expects.
  //
  // mux_ctrl ! 0+0  ! 0+16 ! 1+0  ! 1+16
  // =========!======!======!======!========
  // DAC_I    ! tx_i ! tx_i ! tx_q ! tx_q
  // DAC_Q    ! tx_i ! tx_q ! tx_i ! tx_q
  //
  // Most daughterboards will thus use 0x01 or 0x10 as the mux_ctrl value.
  always @(posedge clk) begin
    if (reset) begin
      dac_stb <= 1'b0;
      dac_i   <= 16'd0;
      dac_q   <= 16'd0;
    end else begin
      dac_stb <= tx_round_stb;
      case(mux_ctrl[3:0])
        0 : dac_i <= tx_i_round;
        1 : dac_i <= tx_q_round;
        default : dac_i <= 0;
      endcase
      case(mux_ctrl[7:4])
        0 : dac_q <= tx_i_round;
        1 : dac_q <= tx_q_round;
        default : dac_q <= 0;
      endcase
    end
  end

endmodule
