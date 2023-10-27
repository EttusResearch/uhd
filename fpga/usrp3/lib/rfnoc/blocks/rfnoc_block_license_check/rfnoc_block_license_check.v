//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_license_check
//
// Description:
//
//   This RFNoC block has no data ports. Its sole purpose is to take in a
//   license key via ControlPort transactions, and verify its integrity by
//   comparing it with a hash of the the device serial, a private key, and a
//   feature ID.
//
// Parameters:
//
//   THIS_PORTID : Control crossbar port to which this block is connected
//   CHDR_W      : AXIS-CHDR data bus width
//   MTU         : Maximum transmission unit (i.e., maximum packet size in
//                 CHDR words is 2**MTU).
//   NUM_FEATURES: Number of features this block can unlock. The width of the
//                 output bus 'feature_enable' is of the same width as this
//                 number. Every bit in 'feature_enable' represents the locked/
//                 unlocked state of one feature.
//   FEATURE_IDS : A list of 32-bit feature IDs. There must be as many feature IDs
//                 as NUM_FEATURES (e.g., if NUM_FEATURES == 3, then there need
//                 to be 3 32-bit feature IDs in here).
//   SERIAL_W    : Width of the device serial. This is tyically 96 bits for
//                 RFSoC devices, and 64 bits for 7-series devices. Note that
//                 when using the image builder, this will be automatically
//                 populated.
//   PKEY_W      : Width of the private key.
//   PRIVATE_KEY : The private key. Its length should match PKEY_W.
//

`default_nettype none

module rfnoc_block_license_check #(
  parameter [9:0] THIS_PORTID     = 10'd0,
  parameter       CHDR_W          = 64,
  parameter [5:0] MTU             = 10,
  parameter       NUM_FEATURES    = 1,
  parameter       FEATURE_IDS     = {32'h0000},
  parameter       SERIAL_W        = 96,
  parameter       PKEY_W          = 312,
  parameter       PRIVATE_KEY     = 312'b0
)(
  // RFNoC Framework Clocks and Resets
  input  wire                   rfnoc_chdr_clk,
  input  wire                   rfnoc_ctrl_clk,
  // RFNoC Backend Interface
  input  wire [511:0]           rfnoc_core_config,
  output wire [511:0]           rfnoc_core_status,
  // AXIS-Ctrl Input Port (from framework)
  input  wire [31:0]            s_rfnoc_ctrl_tdata,
  input  wire                   s_rfnoc_ctrl_tlast,
  input  wire                   s_rfnoc_ctrl_tvalid,
  output wire                   s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Output Port (to framework)
  output wire [31:0]            m_rfnoc_ctrl_tdata,
  output wire                   m_rfnoc_ctrl_tlast,
  output wire                   m_rfnoc_ctrl_tvalid,
  input  wire                   m_rfnoc_ctrl_tready,

  // The read-only device serial (device DNA)
  input  wire [SERIAL_W-1:0]    serial,

  output wire [NUM_FEATURES-1:0] feature_enable
);

  `include "rfnoc_block_license_check_regs.vh"

  localparam COMPAT_MAJOR  = 16'h0;
  localparam COMPAT_MINOR  = 16'h0;

  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  // Clocks and Resets
  wire               ctrlport_clk;
  wire               ctrlport_rst;
  // CtrlPort Master
  wire               m_ctrlport_req_wr;
  wire               m_ctrlport_req_rd;
  wire [19:0]        m_ctrlport_req_addr;
  wire [31:0]        m_ctrlport_req_data;
  wire               m_ctrlport_resp_ack;
  wire [1:0]         m_ctrlport_resp_status;
  wire [31:0]        m_ctrlport_resp_data;

  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  noc_shell_license_check #(
    .CHDR_W              (CHDR_W),
    .THIS_PORTID         (THIS_PORTID),
    .MTU                 (MTU)
  ) noc_shell_license_check_i (
    //---------------------
    // Framework Interface
    //---------------------

    // Clock Inputs
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
    // Reset Outputs
    .rfnoc_chdr_rst      (),
    .rfnoc_ctrl_rst      (),
    // RFNoC Backend Interface
    .rfnoc_core_config   (rfnoc_core_config),
    .rfnoc_core_status   (rfnoc_core_status),
    // AXIS-Ctrl Input Port (from framework)
    .s_rfnoc_ctrl_tdata  (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast  (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready (s_rfnoc_ctrl_tready),
    // AXIS-Ctrl Output Port (to framework)
    .m_rfnoc_ctrl_tdata  (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast  (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready (m_rfnoc_ctrl_tready),

    //---------------------
    // Client Interface
    //---------------------

    // CtrlPort Clock and Reset
    .ctrlport_clk              (ctrlport_clk),
    .ctrlport_rst              (ctrlport_rst),
    // CtrlPort Master
    .m_ctrlport_req_wr         (m_ctrlport_req_wr),
    .m_ctrlport_req_rd         (m_ctrlport_req_rd),
    .m_ctrlport_req_addr       (m_ctrlport_req_addr),
    .m_ctrlport_req_data       (m_ctrlport_req_data),
    .m_ctrlport_resp_ack       (m_ctrlport_resp_ack),
    .m_ctrlport_resp_status    (m_ctrlport_resp_status),
    .m_ctrlport_resp_data      (m_ctrlport_resp_data)

  );

  //---------------------------------------------------------------------------
  // User Logic
  //---------------------------------------------------------------------------

  localparam NUM_CTRLPORT_CLIENTS = 2;

  wire        m_ctrlport_lc_req_wr,      m_ctrlport_reg_req_wr;
  wire        m_ctrlport_lc_req_rd,      m_ctrlport_reg_req_rd;
  wire [19:0] m_ctrlport_lc_req_addr,    m_ctrlport_reg_req_addr;
  wire [31:0] m_ctrlport_lc_req_data,    m_ctrlport_reg_req_data;
  wire        m_ctrlport_lc_resp_ack;
  reg         m_ctrlport_reg_resp_ack    =  1'b0;
  wire [ 1:0] m_ctrlport_lc_resp_status, m_ctrlport_reg_resp_status;
  wire [31:0] m_ctrlport_lc_resp_data,   m_ctrlport_reg_resp_data;

  ctrlport_splitter #(
    .NUM_SLAVES(2)
  ) ctrl_split_i (
    .ctrlport_clk(ctrlport_clk),
    .ctrlport_rst(ctrlport_rst),

    .s_ctrlport_req_wr       (m_ctrlport_req_wr),
    .s_ctrlport_req_rd       (m_ctrlport_req_rd),
    .s_ctrlport_req_addr     (m_ctrlport_req_addr),
    .s_ctrlport_req_data     (m_ctrlport_req_data),
    .s_ctrlport_req_byte_en  (4'hF),
    .s_ctrlport_req_has_time (1'b0),
    .s_ctrlport_req_time     (64'h0),
    .s_ctrlport_resp_ack     (m_ctrlport_resp_ack),
    .s_ctrlport_resp_status  (m_ctrlport_resp_status),
    .s_ctrlport_resp_data    (m_ctrlport_resp_data),

    .m_ctrlport_req_wr       ({m_ctrlport_lc_req_wr,      m_ctrlport_reg_req_wr}),
    .m_ctrlport_req_rd       ({m_ctrlport_lc_req_rd,      m_ctrlport_reg_req_rd}),
    .m_ctrlport_req_addr     ({m_ctrlport_lc_req_addr,    m_ctrlport_reg_req_addr}),
    .m_ctrlport_req_data     ({m_ctrlport_lc_req_data,    m_ctrlport_reg_req_data}),
    .m_ctrlport_req_byte_en  (/* not connected */),
    .m_ctrlport_req_has_time (/* not connected */),
    .m_ctrlport_req_time     (/* not connected */),
    .m_ctrlport_resp_ack     ({m_ctrlport_lc_resp_ack,    m_ctrlport_reg_resp_ack}),
    .m_ctrlport_resp_status  ({m_ctrlport_lc_resp_status, m_ctrlport_reg_resp_status}),
    .m_ctrlport_resp_data    ({m_ctrlport_lc_resp_data,   m_ctrlport_reg_resp_data})
  );

  license_check #(
    .BASE_ADDR     (REG_FEATURE_ID),
    .PKEY_W        (PKEY_W        ),
    .SERIAL_W      (SERIAL_W      ),
    .NUM_FEATURES  (NUM_FEATURES  ),
    .FEATURE_IDS   (FEATURE_IDS   ),
    .PRIVATE_KEY   (PRIVATE_KEY   )
  ) license_check_i (
    .clk                   (ctrlport_clk),
    .rst                   (ctrlport_rst),

    .serial                (serial),

    .s_ctrlport_req_wr     (m_ctrlport_lc_req_wr     ),
    .s_ctrlport_req_rd     (m_ctrlport_lc_req_rd     ),
    .s_ctrlport_req_addr   (m_ctrlport_lc_req_addr   ),
    .s_ctrlport_req_data   (m_ctrlport_lc_req_data   ),
    .s_ctrlport_resp_ack   (m_ctrlport_lc_resp_ack   ),
    .s_ctrlport_resp_status(m_ctrlport_lc_resp_status),
    .s_ctrlport_resp_data  (m_ctrlport_lc_resp_data  ),

    .feature_enabled       (feature_enable)
  );

  // Compat reg
  always @(posedge ctrlport_clk) begin
    m_ctrlport_reg_resp_ack <= m_ctrlport_reg_req_rd && m_ctrlport_reg_req_addr == REG_COMPAT;
  end
  assign m_ctrlport_reg_resp_data = {COMPAT_MAJOR, COMPAT_MINOR};
  assign m_ctrlport_reg_resp_status = 2'b00;


endmodule // rfnoc_block_license_check


`default_nettype wire
