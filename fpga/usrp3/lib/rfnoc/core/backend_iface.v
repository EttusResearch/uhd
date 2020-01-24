//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: backend_iface
// Description:
//   A noc_shell interface to the backend infrastructure
//

module backend_iface #(
  parameter [31:0] NOC_ID              = 32'h0,
  parameter  [5:0] NUM_DATA_I          = 0,
  parameter  [5:0] NUM_DATA_O          = 0,
  parameter  [5:0] CTRL_FIFOSIZE       = 0,
  parameter  [7:0] CTRL_MAX_ASYNC_MSGS = 0,
  parameter  [5:0] MTU                 = 0
)(
  // Input clock
  input  wire         rfnoc_chdr_clk,
  input  wire         rfnoc_ctrl_clk,
  // Backend interface (sync. to rfnoc_ctrl_clk)
  input  wire [511:0] rfnoc_core_config,
  output wire [511:0] rfnoc_core_status,
  // Output reset
  output wire         rfnoc_chdr_rst,
  output wire         rfnoc_ctrl_rst,
  // Flush interface (sync. to rfnoc_chdr_clk)
  output wire         data_i_flush_en,
  output wire [31:0]  data_i_flush_timeout,
  input  wire [63:0]  data_i_flush_active,
  input  wire [63:0]  data_i_flush_done,
  output wire         data_o_flush_en,
  output wire [31:0]  data_o_flush_timeout,
  input  wire [63:0]  data_o_flush_active,
  input  wire [63:0]  data_o_flush_done
);
  localparam RESET_LENGTH = 32;

  `include "rfnoc_backend_iface.vh"

  // -----------------------------------
  // CONFIG: Infrastructure => Block
  // -----------------------------------
  wire [BEC_TOTAL_WIDTH-1:0] rfnoc_core_config_trim = rfnoc_core_config[BEC_TOTAL_WIDTH-1:0];

  reg  [31:0] flush_timeout_ctclk = 32'd0;
  reg         flush_en_ctclk      = 1'b0;
  reg         soft_ctrl_rst_ctclk = 1'b0;
  reg         soft_chdr_rst_ctclk = 1'b0;

  // Register logic before synchronizer
  always @(posedge rfnoc_ctrl_clk) begin
    flush_timeout_ctclk <= rfnoc_core_config_trim[BEC_FLUSH_TIMEOUT_OFFSET +: BEC_FLUSH_TIMEOUT_WIDTH];
    flush_en_ctclk      <= rfnoc_core_config_trim[BEC_FLUSH_EN_OFFSET      +: BEC_FLUSH_EN_WIDTH     ];
    soft_ctrl_rst_ctclk <= rfnoc_core_config_trim[BEC_SOFT_CTRL_RST_OFFSET +: BEC_SOFT_CTRL_RST_WIDTH];
    soft_chdr_rst_ctclk <= rfnoc_core_config_trim[BEC_SOFT_CHDR_RST_OFFSET +: BEC_SOFT_CHDR_RST_WIDTH];
  end

  // Synchronizer
  wire [31:0] flush_timeout_chclk;
  wire        flush_en_chclk;

  // Note: We are using a synchronizer to cross the 32-bit timeout bus
  // into a different clock domain. Typically we would use a 2clk FIFO
  // but it's OK to have the bits unsynchronized here because the value
  // is static and is set from SW long before it is actually used.

  synchronizer #(.WIDTH(33), .INITIAL_VAL(33'd0)) sync_ctrl_i (
    .clk(rfnoc_chdr_clk), .rst(1'b0),
    .in({flush_en_ctclk, flush_timeout_ctclk}),
    .out({flush_en_chclk, flush_timeout_chclk})
  );

  // Synchronize the reset to the CHDR and CTRL clock domains, and extend the 
  // reset pulse to make it long enough for most IP to reset correctly.

  wire rfnoc_ctrl_rst_pulse;
  wire rfnoc_chdr_rst_pulse;

  pulse_synchronizer #(.MODE("POSEDGE")) soft_ctrl_rst_sync_i (
    .clk_a(rfnoc_ctrl_clk), .rst_a(1'b0), .pulse_a(soft_ctrl_rst_ctclk), .busy_a(),
    .clk_b(rfnoc_ctrl_clk), .pulse_b(rfnoc_ctrl_rst_pulse)
  );

  pulse_synchronizer #(.MODE("POSEDGE")) soft_chdr_rst_sync_i (
    .clk_a(rfnoc_ctrl_clk), .rst_a(1'b0), .pulse_a(soft_chdr_rst_ctclk), .busy_a(),
    .clk_b(rfnoc_chdr_clk), .pulse_b(rfnoc_chdr_rst_pulse)
  );

  pulse_stretch_min #(.LENGTH(RESET_LENGTH)) soft_ctrl_rst_stretch_i (
    .clk(rfnoc_ctrl_clk), .rst(1'b0),
    .pulse_in(rfnoc_ctrl_rst_pulse), .pulse_out(rfnoc_ctrl_rst)
  );

  pulse_stretch_min #(.LENGTH(RESET_LENGTH)) soft_chdr_rst_stretch_i (
    .clk(rfnoc_chdr_clk), .rst(1'b0),
    .pulse_in(rfnoc_chdr_rst_pulse), .pulse_out(rfnoc_chdr_rst)
  );

  assign data_i_flush_timeout = flush_timeout_chclk;
  assign data_o_flush_timeout = flush_timeout_chclk;
  assign data_i_flush_en      = flush_en_chclk;
  assign data_o_flush_en      = flush_en_chclk;

  // -----------------------------------
  // STATUS: Block => Infrastructure
  // -----------------------------------

  reg  flush_active_chclk  = 1'b0;
  reg  flush_done_chclk    = 1'b0;

  // Register logic before synchronizer
  wire flush_active_ctclk;
  wire flush_done_ctclk;

  always @(posedge rfnoc_chdr_clk) begin
    flush_active_chclk <= (|data_i_flush_active[NUM_DATA_I-1:0]) | (|data_o_flush_active[NUM_DATA_O-1:0]);
    flush_done_chclk   <= (&data_i_flush_done  [NUM_DATA_I-1:0]) & (&data_o_flush_done  [NUM_DATA_O-1:0]);
  end

  // Synchronizer
  synchronizer #(.WIDTH(2), .INITIAL_VAL(2'd0)) sync_status_i (
    .clk(rfnoc_ctrl_clk), .rst(1'b0),
    .in({flush_active_chclk, flush_done_chclk}),
    .out({flush_active_ctclk, flush_done_ctclk})
  );

  assign rfnoc_core_status[BES_PROTO_VER_OFFSET          +:BES_PROTO_VER_WIDTH          ] = BACKEND_PROTO_VER;
  assign rfnoc_core_status[BES_NUM_DATA_I_OFFSET         +:BES_NUM_DATA_I_WIDTH         ] = NUM_DATA_I;
  assign rfnoc_core_status[BES_NUM_DATA_O_OFFSET         +:BES_NUM_DATA_O_WIDTH         ] = NUM_DATA_O;
  assign rfnoc_core_status[BES_CTRL_FIFOSIZE_OFFSET      +:BES_CTRL_FIFOSIZE_WIDTH      ] = CTRL_FIFOSIZE;
  assign rfnoc_core_status[BES_CTRL_MAX_ASYNC_MSGS_OFFSET+:BES_CTRL_MAX_ASYNC_MSGS_WIDTH] = CTRL_MAX_ASYNC_MSGS;
  assign rfnoc_core_status[BES_NOC_ID_OFFSET             +:BES_NOC_ID_WIDTH             ] = NOC_ID;
  assign rfnoc_core_status[BES_FLUSH_ACTIVE_OFFSET       +:BES_FLUSH_ACTIVE_WIDTH       ] = flush_active_ctclk;
  assign rfnoc_core_status[BES_FLUSH_DONE_OFFSET         +:BES_FLUSH_DONE_WIDTH         ] = flush_done_ctclk;
  assign rfnoc_core_status[BES_DATA_MTU_OFFSET           +:BES_DATA_MTU_WIDTH           ] = MTU;
  // Assign the rest to 0
  assign rfnoc_core_status[511:BES_TOTAL_WIDTH] = {(512-BES_TOTAL_WIDTH){1'b0}};

endmodule // backend_iface

