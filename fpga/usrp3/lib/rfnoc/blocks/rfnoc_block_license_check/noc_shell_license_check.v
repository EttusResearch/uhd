//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: noc_shell_license_check
//
// Description:
//
//   This is a tool-generated NoC-shell for the license_check block.
//   See the RFNoC specification for more information about NoC shells.
//
// Parameters:
//
//   THIS_PORTID  : Control crossbar port to which this block is connected
//   CHDR_W       : AXIS-CHDR data bus width
//   CTRL_CLK_IDX : The index of the control clock for this block. This is used
//                  to populate the backend interface, from where UHD can query
//                  the clock index and thus auto-deduct which clock is used.
//   TB_CLK_IDX   : The index of the timebase clock for this block. This is used
//                  to populate the backend interface, from where UHD can query
//                  the clock index and thus auto-deduct which clock is used.
//   MTU          : Maximum transmission unit (i.e., maximum packet size in
//                  CHDR words is 2**MTU).
//

`default_nettype none

module noc_shell_license_check #(
  parameter [9:0] THIS_PORTID     = 10'd0,
  parameter       CHDR_W          = 64,
  parameter [5:0] CTRL_CLK_IDX    = 6'h3F,
  parameter [5:0] TB_CLK_IDX      = 6'h3F,
  parameter [5:0] MTU             = 10
) (
  //---------------------
  // Framework Interface
  //---------------------

  // RFNoC Framework Clocks
  input  wire rfnoc_chdr_clk,
  input  wire rfnoc_ctrl_clk,

  // NoC Shell Generated Resets
  output wire rfnoc_chdr_rst,
  output wire rfnoc_ctrl_rst,

  // RFNoC Backend Interface
  input  wire [511:0]          rfnoc_core_config,
  output wire [511:0]          rfnoc_core_status,

  // AXIS-Ctrl Control Input Port (from framework)
  input  wire [31:0]           s_rfnoc_ctrl_tdata,
  input  wire                  s_rfnoc_ctrl_tlast,
  input  wire                  s_rfnoc_ctrl_tvalid,
  output wire                  s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Control Output Port (to framework)
  output wire [31:0]           m_rfnoc_ctrl_tdata,
  output wire                  m_rfnoc_ctrl_tlast,
  output wire                  m_rfnoc_ctrl_tvalid,
  input  wire                  m_rfnoc_ctrl_tready,

  //---------------------
  // Client Interface
  //---------------------

  // CtrlPort Clock and Reset
  output wire               ctrlport_clk,
  output wire               ctrlport_rst,
  // CtrlPort Master
  output wire               m_ctrlport_req_wr,
  output wire               m_ctrlport_req_rd,
  output wire [19:0]        m_ctrlport_req_addr,
  output wire [31:0]        m_ctrlport_req_data,
  input  wire               m_ctrlport_resp_ack,
  input  wire [1:0]         m_ctrlport_resp_status,
  input  wire [31:0]        m_ctrlport_resp_data
);

  //---------------------------------------------------------------------------
  //  Backend Interface
  //---------------------------------------------------------------------------

  backend_iface #(
    .NOC_ID        (32'h11C0CECC),
    .NUM_DATA_I    (0),
    .NUM_DATA_O    (0),
    .CTRL_FIFOSIZE ($clog2(64)),
    .MTU           (MTU)
  ) backend_iface_i (
    .rfnoc_chdr_clk       (rfnoc_chdr_clk),
    .rfnoc_chdr_rst       (rfnoc_chdr_rst),
    .rfnoc_ctrl_clk       (rfnoc_ctrl_clk),
    .rfnoc_ctrl_rst       (rfnoc_ctrl_rst),
    .data_i_flush_en      (),
    .data_i_flush_timeout (),
    .data_i_flush_active  (64'b0),
    .data_i_flush_done    (64'b1),
    .data_o_flush_en      (),
    .data_o_flush_timeout (),
    .data_o_flush_active  (64'b0),
    .data_o_flush_done    (64'b1),
    .rfnoc_core_config    (rfnoc_core_config),
    .rfnoc_core_status    (rfnoc_core_status)
  );

  //---------------------------------------------------------------------------
  //  Control Path
  //---------------------------------------------------------------------------

  assign ctrlport_clk = rfnoc_ctrl_clk;
  assign ctrlport_rst = rfnoc_ctrl_rst;

  ctrlport_endpoint #(
    .THIS_PORTID      (THIS_PORTID),
    .SYNC_CLKS        (1),
    .AXIS_CTRL_MST_EN (0),
    .AXIS_CTRL_SLV_EN (1),
    .SLAVE_FIFO_SIZE  ($clog2(64))
  ) ctrlport_endpoint_i (
    .rfnoc_ctrl_clk            (rfnoc_ctrl_clk),
    .rfnoc_ctrl_rst            (rfnoc_ctrl_rst),
    .ctrlport_clk              (ctrlport_clk),
    .ctrlport_rst              (ctrlport_rst),
    .s_rfnoc_ctrl_tdata        (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast        (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid       (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready       (s_rfnoc_ctrl_tready),
    .m_rfnoc_ctrl_tdata        (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast        (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid       (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready       (m_rfnoc_ctrl_tready),
    .m_ctrlport_req_wr         (m_ctrlport_req_wr),
    .m_ctrlport_req_rd         (m_ctrlport_req_rd),
    .m_ctrlport_req_addr       (m_ctrlport_req_addr),
    .m_ctrlport_req_data       (m_ctrlport_req_data),
    .m_ctrlport_req_byte_en    (),
    .m_ctrlport_req_has_time   (),
    .m_ctrlport_req_time       (),
    .m_ctrlport_resp_ack       (m_ctrlport_resp_ack),
    .m_ctrlport_resp_status    (m_ctrlport_resp_status),
    .m_ctrlport_resp_data      (m_ctrlport_resp_data),
    .s_ctrlport_req_wr         (1'b0),
    .s_ctrlport_req_rd         (1'b0),
    .s_ctrlport_req_addr       (20'b0),
    .s_ctrlport_req_portid     (10'b0),
    .s_ctrlport_req_rem_epid   (16'b0),
    .s_ctrlport_req_rem_portid (10'b0),
    .s_ctrlport_req_data       (32'b0),
    .s_ctrlport_req_byte_en    (4'hF),
    .s_ctrlport_req_has_time   (1'b0),
    .s_ctrlport_req_time       (64'b0),
    .s_ctrlport_resp_ack       (),
    .s_ctrlport_resp_status    (),
    .s_ctrlport_resp_data      ()
  );

endmodule // noc_shell_license_check


`default_nettype wire
