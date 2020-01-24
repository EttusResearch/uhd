//
// Copyright 2016 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cat_io_lvds
//
// Description:
//
// This is an LVDS interface for the AD9361 (Catalina). It consists of of an
// input module for Rx and an output module for Tx. See the AD9361 Interface
// Specification and the AD9361 data sheet for details.
//
// This module assumes a dual-port, full-duplex topology, in 2R2T timing mode.
// The mimo signal allows you to support one (mimo = 0) or two channels (mimo =
// 1). When mimo = 0, the data from one channel is simply ignored and radio_clk
// frequency remains equal to rx_clk_p / 4.
//

module cat_io_lvds #(
  parameter INVERT_FRAME_RX    = 0,
  parameter INVERT_DATA_RX     = 6'b00_0000,
  parameter INVERT_FRAME_TX    = 0,
  parameter INVERT_DATA_TX     = 6'b00_0000,
  parameter USE_CLOCK_IDELAY   = 1,
  parameter USE_DATA_IDELAY    = 1,
  parameter DATA_IDELAY_MODE   = "VAR_LOAD",
  parameter CLOCK_IDELAY_MODE  = "VAR_LOAD",
  parameter INPUT_CLOCK_DELAY  = 16,
  parameter INPUT_DATA_DELAY   = 0,
  parameter USE_CLOCK_ODELAY   = 0,
  parameter USE_DATA_ODELAY    = 0,
  parameter DATA_ODELAY_MODE   = "VAR_LOAD",
  parameter CLOCK_ODELAY_MODE  = "VAR_LOAD",
  parameter OUTPUT_CLOCK_DELAY = 16,
  parameter OUTPUT_DATA_DELAY  = 0,
  parameter USE_BUFG           = 1
) (
  input rst,
  input clk200,

  // Data and frame timing (synchronous to radio_clk)
  input mimo,
  input frame_sample,

  // Delay Control Interface
  input       ctrl_clk,
  input [4:0] ctrl_in_data_delay,
  input [4:0] ctrl_in_clk_delay,
  input       ctrl_ld_in_data_delay,
  input       ctrl_ld_in_clk_delay,
  input [4:0] ctrl_out_data_delay,
  input [4:0] ctrl_out_clk_delay,
  input       ctrl_ld_out_data_delay,
  input       ctrl_ld_out_clk_delay,

  // Baseband sample interface
  output        radio_clk,
  output        radio_clk_2x,
  output        rx_aligned,
  output [11:0] rx_i0,
  output [11:0] rx_q0,
  output [11:0] rx_i1,
  output [11:0] rx_q1,
  input  [11:0] tx_i0,
  input  [11:0] tx_q0,
  input  [11:0] tx_i1,
  input  [11:0] tx_q1,

  // Catalina LVDS interface
  input        rx_clk_p,
  input        rx_clk_n,
  input        rx_frame_p,
  input        rx_frame_n,
  input  [5:0] rx_d_p,
  input  [5:0] rx_d_n,
  output       tx_clk_p,
  output       tx_clk_n,
  output       tx_frame_p,
  output       tx_frame_n,
  output [5:0] tx_d_p,
  output [5:0] tx_d_n
);

  wire sdr_clk, ddr_clk;

  //---------------------------------------------------------------------------
  // Input (Rx) Interface
  //---------------------------------------------------------------------------

  cat_input_lvds #(
    .INVERT_FRAME_RX  (INVERT_FRAME_RX),
    .INVERT_DATA_RX   (INVERT_DATA_RX),
    .USE_CLOCK_DELAY  (USE_CLOCK_IDELAY),
    .USE_DATA_DELAY   (USE_DATA_IDELAY),
    .CLOCK_DELAY_MODE (CLOCK_IDELAY_MODE),
    .DATA_DELAY_MODE  (DATA_IDELAY_MODE),
    .CLOCK_DELAY      (INPUT_CLOCK_DELAY),
    .DATA_DELAY       (INPUT_DATA_DELAY),
    .WIDTH            (6),
    .GROUP            ("CATALINA"),
    .USE_BUFG         (USE_BUFG)
  ) cat_input_lvds_i0 (
    .clk200 (clk200),
    .rst    (rst),

    // Data and frame timing
    .mimo         (mimo),
    .frame_sample (frame_sample),

    // Region local Clocks for I/O cells.
    .ddr_clk (ddr_clk),
    .sdr_clk (sdr_clk),

    // Source Synchronous external input clock
    .ddr_clk_p (rx_clk_p),
    .ddr_clk_n (rx_clk_n),

    // Source Synchronous data lines
    .ddr_data_p  (rx_d_p),
    .ddr_data_n  (rx_d_n),
    .ddr_frame_p (rx_frame_p),
    .ddr_frame_n (rx_frame_n),

    // Delay control interface
    .ctrl_clk           (ctrl_clk),
    .ctrl_data_delay    (ctrl_in_data_delay),
    .ctrl_clk_delay     (ctrl_in_clk_delay),
    .ctrl_ld_data_delay (ctrl_ld_in_data_delay),
    .ctrl_ld_clk_delay  (ctrl_ld_in_clk_delay),

    // SDR output clock(s)
    .radio_clk    (radio_clk),
    .radio_clk_2x (radio_clk_2x),

    // SDR Data buses
    .i0         (rx_i0),
    .q0         (rx_q0),
    .i1         (rx_i1),
    .q1         (rx_q1),
    .rx_aligned (rx_aligned)
  );


  //---------------------------------------------------------------------------
  // Output (Tx) Interface
  //---------------------------------------------------------------------------

  cat_output_lvds #(
    .INVERT_FRAME_TX  (INVERT_FRAME_TX),
    .INVERT_DATA_TX   (INVERT_DATA_TX),
    .USE_CLOCK_DELAY  (USE_CLOCK_ODELAY),
    .USE_DATA_DELAY   (USE_DATA_ODELAY),
    .CLOCK_DELAY_MODE (CLOCK_ODELAY_MODE),
    .DATA_DELAY_MODE  (DATA_ODELAY_MODE),
    .CLOCK_DELAY      (OUTPUT_CLOCK_DELAY),
    .DATA_DELAY       (OUTPUT_DATA_DELAY),
    .WIDTH            (6),
    .GROUP            ("CATALINA")
  ) cat_output_lvds_i0 (
    .clk200 (clk200),
    .rst    (rst),

    // Two samples per frame period (frame_sample=0; e.g., for two-channel
    // mode) or one sample per frame (frame_sample=1)
    .frame_sample(frame_sample),

    // Region local Clocks for I/O cells.
    .ddr_clk (ddr_clk),
    .sdr_clk (sdr_clk),

    // Source Synchronous external input clock
    .ddr_clk_p (tx_clk_p),
    .ddr_clk_n (tx_clk_n),

    // Source Synchronous data lines
    .ddr_data_p  (tx_d_p),
    .ddr_data_n  (tx_d_n),
    .ddr_frame_p (tx_frame_p),
    .ddr_frame_n (tx_frame_n),

    // Delay control interface
    .ctrl_clk           (ctrl_clk),
    .ctrl_data_delay    (ctrl_out_data_delay),
    .ctrl_clk_delay     (ctrl_out_clk_delay),
    .ctrl_ld_data_delay (ctrl_ld_out_data_delay),
    .ctrl_ld_clk_delay  (ctrl_ld_out_clk_delay),

    // SDR global input clock
    .radio_clk (radio_clk),

    // SDR Data buses
    .i0 (tx_i0),
    .q0 (tx_q0),
    .i1 (tx_i1),
    .q1 (tx_q1)
  );


endmodule  // cat_io_lvds
