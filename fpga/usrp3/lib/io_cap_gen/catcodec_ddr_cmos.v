//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module catcodec_ddr_cmos
#(
  parameter DEVICE = "SPARTAN6"  // "7SERIES" or "SPARTAN6", determines device specific implementation of clock divider
)
(
  //output source sync clock for baseband data
  output radio_clk,

  //async reset for clocking
  input arst,

  //control mimo mode
  input mimo,

  //baseband sample interface
  output reg [31:0] rx1,
  output reg [31:0] rx2,
  input [31:0] tx1,
  input [31:0] tx2,

  //capture interface
  input rx_clk,
  input rx_frame,
  input [11:0] rx_d,

  //generate interface
  output tx_clk,
  output tx_frame,
  output [11:0] tx_d
);

  wire clk0, clkdv;
  wire codec_clk, half_clk;
  wire radio_clk_locked;

  // Synchronize MIMO signal into codec_clk domain
  wire mimo_r;
  synchronizer mimo_sync (
    .clk(codec_clk),
    .rst(1'b0),
    .in(mimo),
    .out(mimo_r));

  generate
    if (DEVICE == "SPARTAN6") begin
      DCM_SP #(
        .CLKDV_DIVIDE(2),
        .CLK_FEEDBACK("1X"))
      DCM_SP_codec_clk (
        .RST(arst),
        .CLKIN(rx_clk), .CLKFB(clk0),
        .CLK0(clk0), .CLKDV(clkdv),
        .LOCKED(radio_clk_locked));
      BUFG BUFG_codec_clk(.I(clk0), .O(codec_clk));
      BUFG BUFG_half_clk(.I(clkdv), .O(half_clk));
      BUFGMUX BUFGMUX_radio_clk (.I0(codec_clk), .I1(half_clk), .S(mimo_r), .O(radio_clk));
    end
    else if (DEVICE == "7SERIES") begin
      wire rx_clk_ibufg, clkfb_out, clkfb_in;
      // Create clocks for source synchronous interface
      // Goal is to create a capture clock (codec_clk) and a sample clock (radio_clk).
      // - Capture clock's and source clock's (rx_clk) phase are aligned due to
      //   the MMCM's deskew ability (see the BUFG in the feedback clock path).
      // - BUFGCTRL muxes between the 1x and 1/2x clocks depending on MIMO mode. In MIMO mode, the 1/2x
      //   clock is used, because the sample clock rate is half the source clock rate.
      // - Locked signal is used to ensure the BUFG's output is disabled if the MMCM is not locked.
      // - Avoided cascading BUFGs to ensure minimal skew between codec_clk and radio_clk.
      catcodec_mmcm inst_catcodec_mmcm (
        .CLK_IN1(rx_clk_ibufg),
        .CLK_OUT(clk0),
        .CLK_OUT_DIV2(clkdv),
        .CLKFB_IN(clkfb_in),
        .CLKFB_OUT(clkfb_out),
        .RESET(arst),
        .LOCKED(radio_clk_locked));
      IBUFG (.I(rx_clk), .O(rx_clk_ibufg));
      BUFG (.I(clkfb_out), .O(clkfb_in));
      BUFGCE (.I(clk0), .O(codec_clk), .CE(radio_clk_locked));
      BUFGCTRL BUFGCTRL_radio_clk (.I0(clk0), .I1(clkdv), .S0(~mimo_r), .S1(mimo_r), .CE0(radio_clk_locked), .CE1(radio_clk_locked), .O(radio_clk));
    end
  endgenerate

  //assign baseband sample interfaces
  //all samples are registered on strobe
  wire rx_strobe, tx_strobe;
  wire [11:0] rx_i0, rx_q0, rx_i1, rx_q1;
  reg [11:0] tx_i0, tx_q0, tx_i1, tx_q1;
  //tx mux to feed single channel mode from either input
  wire [31:0] txm = (mimo_r || (tx1 != 32'b0))? tx1: tx2;
  always @(posedge codec_clk) begin
    if (rx_strobe) rx2 <= {rx_i1, 4'b0, rx_q1, 4'b0};
    if (rx_strobe) rx1 <= {rx_i0, 4'b0, rx_q0, 4'b0};
    if (tx_strobe) {tx_i0, tx_q0} <= {txm[31:20], txm[15:4]};
    if (tx_strobe) {tx_i1, tx_q1} <= {tx2[31:20], tx2[15:4]};
  end

  // CMOS Data interface to AD9361
  catcap_ddr_cmos #(
    .DEVICE(DEVICE))
  catcap (
    .data_clk(codec_clk), .mimo(mimo_r),
    .rx_frame(rx_frame), .rx_d(rx_d),
    .rx_clk(/*out*/), .rx_strobe(rx_strobe),
    .i0(rx_i0), .q0(rx_q0),
    .i1(rx_i1), .q1(rx_q1));

   catgen_ddr_cmos #(
    .DEVICE(DEVICE))
   catgen (
    .data_clk(tx_clk), .mimo(mimo_r),
    .tx_frame(tx_frame), .tx_d(tx_d),
    .tx_clk(codec_clk), .tx_strobe(tx_strobe),
    .i0(tx_i0), .q0(tx_q0),
    .i1(tx_i1), .q1(tx_q1));

endmodule // catcodec_ddr_cmos
