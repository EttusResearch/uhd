//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_keep_one_in_n
//
// Description:
//  The Keep One in N block has two modes: sample mode and packet mode.
//  In sample mode, the first sample is kept and then N-1 samples are dropped.
//  Packet mode is similar to sample mode, except the first packet of samples
//  is kept and then N-1 packets are dropped. The packet size is determined
//  automatically from tlast.
//
// Parameters:
//
//   WIDTH_N     : Bit width of N parameter, must be 31 bits or less
//   THIS_PORTID : Control crossbar port to which this block is connected
//   CHDR_W      : AXIS-CHDR data bus width
//   NUM_PORTS   : Number of block instances
//   MTU         : Maximum transmission unit (i.e., maximum packet size in
//                 CHDR words is 2**MTU).
//
`default_nettype none

module rfnoc_block_keep_one_in_n #(
  parameter       WIDTH_N     = 24, // Must be 31 bits or less
  parameter [9:0] THIS_PORTID = 10'd0,
  parameter       CHDR_W      = 64,
  parameter       NUM_PORTS   = 1,
  parameter [5:0] MTU         = 10
)(
  // RFNoC Framework Clocks and Resets
  input  wire                         rfnoc_chdr_clk,
  input  wire                         rfnoc_ctrl_clk,
  input  wire                         ce_clk,
  // RFNoC Backend Interface
  input  wire [511:0]                 rfnoc_core_config,
  output wire [511:0]                 rfnoc_core_status,
  // AXIS-CHDR Input Ports (from framework)
  input  wire [CHDR_W*NUM_PORTS-1:0]  s_rfnoc_chdr_tdata,
  input  wire [       NUM_PORTS-1:0]  s_rfnoc_chdr_tlast,
  input  wire [       NUM_PORTS-1:0]  s_rfnoc_chdr_tvalid,
  output wire [       NUM_PORTS-1:0]  s_rfnoc_chdr_tready,
  // AXIS-CHDR Output Ports (to framework)
  output wire [CHDR_W*NUM_PORTS-1:0]  m_rfnoc_chdr_tdata,
  output wire [       NUM_PORTS-1:0]  m_rfnoc_chdr_tlast,
  output wire [       NUM_PORTS-1:0]  m_rfnoc_chdr_tvalid,
  input  wire [       NUM_PORTS-1:0]  m_rfnoc_chdr_tready,
  // AXIS-Ctrl Input Port (from framework)
  input  wire [31:0]                  s_rfnoc_ctrl_tdata,
  input  wire                         s_rfnoc_ctrl_tlast,
  input  wire                         s_rfnoc_ctrl_tvalid,
  output wire                         s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Output Port (to framework)
  output wire [31:0]                  m_rfnoc_ctrl_tdata,
  output wire                         m_rfnoc_ctrl_tlast,
  output wire                         m_rfnoc_ctrl_tvalid,
  input  wire                         m_rfnoc_ctrl_tready
);

  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  // Clocks and Resets
  wire                     ctrlport_clk;
  wire                     ctrlport_rst;
  // CtrlPort Master
  wire                     m_ctrlport_req_wr;
  wire                     m_ctrlport_req_rd;
  wire [19:0]              m_ctrlport_req_addr;
  wire [31:0]              m_ctrlport_req_data;
  wire                     m_ctrlport_resp_ack;
  wire [31:0]              m_ctrlport_resp_data;
  // Data Stream to User Logic: in
  wire [32*NUM_PORTS-1:0]  m_in_axis_tdata;
  wire [   NUM_PORTS-1:0]  m_in_axis_tlast;
  wire [   NUM_PORTS-1:0]  m_in_axis_tvalid;
  wire [   NUM_PORTS-1:0]  m_in_axis_tready;
  wire [64*NUM_PORTS-1:0]  m_in_axis_ttimestamp;
  wire [   NUM_PORTS-1:0]  m_in_axis_thas_time;
  wire [16*NUM_PORTS-1:0]  m_in_axis_tlength;
  wire [   NUM_PORTS-1:0]  m_in_axis_teov;
  wire [   NUM_PORTS-1:0]  m_in_axis_teob;
  // Data Stream from User Logic: out
  wire [32*NUM_PORTS-1:0]  s_out_axis_tdata;
  wire [   NUM_PORTS-1:0]  s_out_axis_tlast;
  wire [   NUM_PORTS-1:0]  s_out_axis_tvalid;
  wire [   NUM_PORTS-1:0]  s_out_axis_tready;
  wire [64*NUM_PORTS-1:0]  s_out_axis_ttimestamp;
  wire [   NUM_PORTS-1:0]  s_out_axis_thas_time;
  wire [   NUM_PORTS-1:0]  s_out_axis_teov;
  wire [   NUM_PORTS-1:0]  s_out_axis_teob;

  wire                     ce_rst;

  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  noc_shell_keep_one_in_n #(
    .CHDR_W      (CHDR_W),
    .THIS_PORTID (THIS_PORTID),
    .NUM_PORTS   (NUM_PORTS),
    .MTU         (MTU)
  ) noc_shell_keep_one_in_n_i (
    //// Framework Interface
    // Clock Inputs
    .rfnoc_chdr_clk        (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk        (rfnoc_ctrl_clk),
    .ce_clk                (ce_clk),
    // Reset Outputs
    .rfnoc_chdr_rst        (),
    .rfnoc_ctrl_rst        (),
    .ce_rst                (ce_rst),
    // RFNoC Backend Interface
    .rfnoc_core_config     (rfnoc_core_config),
    .rfnoc_core_status     (rfnoc_core_status),
    // CHDR Input Ports  (from framework)
    .s_rfnoc_chdr_tdata    (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast    (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid   (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready   (s_rfnoc_chdr_tready),
    // CHDR Output Ports (to framework)
    .m_rfnoc_chdr_tdata    (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast    (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid   (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready   (m_rfnoc_chdr_tready),
    // AXIS-Ctrl Input Port (from framework)
    .s_rfnoc_ctrl_tdata    (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast    (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid   (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready   (s_rfnoc_ctrl_tready),
    // AXIS-Ctrl Output Port (to framework)
    .m_rfnoc_ctrl_tdata    (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast    (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid   (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready   (m_rfnoc_ctrl_tready),
    //// Client Interface
    // CtrlPort Clock and Reset
    .ctrlport_clk          (ctrlport_clk),
    .ctrlport_rst          (ctrlport_rst),
    // CtrlPort Master
    .m_ctrlport_req_wr     (m_ctrlport_req_wr),
    .m_ctrlport_req_rd     (m_ctrlport_req_rd),
    .m_ctrlport_req_addr   (m_ctrlport_req_addr),
    .m_ctrlport_req_data   (m_ctrlport_req_data),
    .m_ctrlport_resp_ack   (m_ctrlport_resp_ack),
    .m_ctrlport_resp_data  (m_ctrlport_resp_data),
    // AXI-Stream Payload Context Clock and Reset
    .axis_data_clk         (),
    .axis_data_rst         (),
    // Data Stream to User Logic: in
    .m_in_axis_tdata       (m_in_axis_tdata),
    .m_in_axis_tkeep       (),
    .m_in_axis_tlast       (m_in_axis_tlast),
    .m_in_axis_tvalid      (m_in_axis_tvalid),
    .m_in_axis_tready      (m_in_axis_tready),
    .m_in_axis_ttimestamp  (m_in_axis_ttimestamp),
    .m_in_axis_thas_time   (m_in_axis_thas_time),
    .m_in_axis_tlength     (m_in_axis_tlength),
    .m_in_axis_teov        (m_in_axis_teov),
    .m_in_axis_teob        (m_in_axis_teob),
    // Data Stream from User Logic: out
    .s_out_axis_tdata      (s_out_axis_tdata),
    .s_out_axis_tkeep      ({(NUM_PORTS){1'b1}}),
    .s_out_axis_tlast      (s_out_axis_tlast),
    .s_out_axis_tvalid     (s_out_axis_tvalid),
    .s_out_axis_tready     (s_out_axis_tready),
    .s_out_axis_ttimestamp (s_out_axis_ttimestamp),
    .s_out_axis_thas_time  (s_out_axis_thas_time),
    .s_out_axis_teov       (s_out_axis_teov),
    .s_out_axis_teob       (s_out_axis_teob)
  );

  wire [ 8*NUM_PORTS-1:0] set_addr;
  wire [32*NUM_PORTS-1:0] set_data;
  wire [   NUM_PORTS-1:0] set_stb;
  wire [ 8*NUM_PORTS-1:0] rb_addr;
  reg  [64*NUM_PORTS-1:0] rb_data;
  wire [   NUM_PORTS-1:0] rb_stb;

  ctrlport_to_settings_bus # (
    .NUM_PORTS (NUM_PORTS)
  ) ctrlport_to_settings_bus_i (
    .ctrlport_clk             (ctrlport_clk),
    .ctrlport_rst             (ctrlport_rst),
    .s_ctrlport_req_wr        (m_ctrlport_req_wr),
    .s_ctrlport_req_rd        (m_ctrlport_req_rd),
    .s_ctrlport_req_addr      (m_ctrlport_req_addr),
    .s_ctrlport_req_data      (m_ctrlport_req_data),
    .s_ctrlport_req_has_time  (1'b0),
    .s_ctrlport_req_time      (64'd0),
    .s_ctrlport_resp_ack      (m_ctrlport_resp_ack),
    .s_ctrlport_resp_data     (m_ctrlport_resp_data),
    .set_data                 (set_data),
    .set_addr                 (set_addr),
    .set_stb                  (set_stb),
    .set_time                 (),
    .set_has_time             (),
    .rb_stb                   (rb_stb),
    .rb_addr                  (rb_addr),
    .rb_data                  (rb_data),
    .timestamp                ());

  //---------------------------------------------------------------------------
  // User Logic
  //---------------------------------------------------------------------------

  `include "rfnoc_keep_one_in_n_regs.vh"

  wire [REG_N_LEN*NUM_PORTS-1:0] n;
  wire [REG_MODE_LEN*NUM_PORTS-1:0] mode;

  genvar i;
  for (i = 0; i < NUM_PORTS; i = i+1) begin

    setting_reg #(
      .my_addr  (REG_N),
      .awidth   (8),
      .width    (REG_N_LEN))
    inst_setting_reg_n (
      .clk      (ce_clk),
      .rst      (ce_rst),
      .strobe   (set_stb[i]),
      .addr     (set_addr[8*(i+1)-1:8*i]),
      .in       (set_data[32*(i+1)-1:32*i]),
      .out      (n[WIDTH_N*(i+1)-1:WIDTH_N*i]),
      .changed  ());

    setting_reg #(
      .my_addr  (REG_MODE),
      .awidth   (8),
      .width    (REG_MODE_LEN))
    inst_setting_reg_mode (
      .clk      (ce_clk),
      .rst      (ce_rst),
      .strobe   (set_stb[i]),
      .addr     (set_addr[8*(i+1)-1:8*i]),
      .in       (set_data[32*(i+1)-1:32*i]),
      .out      (mode[i]),
      .changed  ());

    // Readback
    assign rb_stb[i] = 1'b1;
    always @*
      case (rb_addr[8*(i+1)-1:8*i])
        REG_N           : rb_data[64*(i+1)-1:64*i] <= {{(64-REG_N_LEN){1'b0}}, n[WIDTH_N*(i+1)-1:WIDTH_N*i]};
        REG_MODE        : rb_data[64*(i+1)-1:64*i] <= {{(64-REG_MODE_LEN){1'b0}}, mode[i]};
        REG_WIDTH_N     : rb_data[64*(i+1)-1:64*i] <= {{(64-REG_WIDTH_N){1'b0}}, WIDTH_N};
        default         : rb_data[64*(i+1)-1:64*i] <= 64'h0BADC0DE0BADC0DE;
    endcase

    rfnoc_keep_one_in_n #(
      .WIDTH      (32),
      .WIDTH_N    (WIDTH_N))
    inst_rfnoc_keep_one_in_n (
      .clk               (ce_clk),
      .reset             (ce_rst),
      .mode              (mode[i]),
      .n                 (n[WIDTH_N*(i+1)-1:WIDTH_N*i]),
      .s_axis_tdata      (m_in_axis_tdata[32*(i+1)-1:32*i]),
      .s_axis_tlast      (m_in_axis_tlast[i]),
      .s_axis_tvalid     (m_in_axis_tvalid[i]),
      .s_axis_tready     (m_in_axis_tready[i]),
      .s_axis_ttimestamp (m_in_axis_ttimestamp[64*(i+1)-1:64*i]),
      .s_axis_thas_time  (m_in_axis_thas_time[i]),
      .s_axis_tlength    (m_in_axis_tlength[16*(i+1)-1:16*i]),
      .s_axis_teov       (m_in_axis_teov[i]),
      .s_axis_teob       (m_in_axis_teob[i]),
      .m_axis_tdata      (s_out_axis_tdata[32*(i+1)-1:32*i]),
      .m_axis_tlast      (s_out_axis_tlast[i]),
      .m_axis_tvalid     (s_out_axis_tvalid[i]),
      .m_axis_tready     (s_out_axis_tready[i]),
      .m_axis_ttimestamp (s_out_axis_ttimestamp[64*(i+1)-1:64*i]),
      .m_axis_thas_time  (s_out_axis_thas_time[i]),
      .m_axis_teov       (s_out_axis_teov[i]),
      .m_axis_teob       (s_out_axis_teob[i]));
  end

endmodule // rfnoc_block_keep_one_in_n

`default_nettype wire
