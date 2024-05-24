//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_ofdm
//
// Description:
//
//   FFT/IFFT plus cyclic prefix insertion/removal.
//
// User Parameters:
//
//   THIS_PORTID              : Control crossbar port to which this block is connected
//   CHDR_W                   : AXIS-CHDR data bus width
//   MTU                      : Log2 of maximum transmission unit
//   NUM_PORTS                : Number of channels
//   MAX_FFT_SIZE_LOG2        : Log2 of maximum configurable FFT size
//   MAX_CP_LEN_LOG2          : Log2 of maximum cyclic prefix length
//   MAX_CP_LIST_LEN_INS_LOG2 : Log2 of max length of cyclic prefix insertion list
//   MAX_CP_LIST_LEN_REM_LOG2 : Log2 of max length of cyclic prefix removal list
//   CP_INSERTION_REPEAT      : Enable (1) or disable (0) CP insertion list FIFO loopback
//   CP_REMOVAL_REPEAT        : Enable (1) or disable (0) CP removal list FIFO loopback
//

`default_nettype none

module rfnoc_block_ofdm #(
  logic [9:0] THIS_PORTID              = 10'd0,
  int         CHDR_W                   = 64,
  logic [5:0] MTU                      = 6'd10,
  int         NUM_PORTS                = 2,
  int         MAX_FFT_SIZE_LOG2        = 12,
  int         MAX_CP_LEN_LOG2          = 12,
  int         MAX_CP_LIST_LEN_INS_LOG2 = 5,
  int         MAX_CP_LIST_LEN_REM_LOG2 = 5,
  int         CP_INSERTION_REPEAT      = 1,
  int         CP_REMOVAL_REPEAT        = 1
) (
  // RFNoC Framework Clocks and Resets
  input  wire                            rfnoc_chdr_clk,
  input  wire                            rfnoc_ctrl_clk,
  input  wire                            ce_clk,

  // RFNoC Backend Interface
  input  wire [                   511:0] rfnoc_core_config,
  output wire [                   511:0] rfnoc_core_status,

  // AXIS-CHDR Input Ports (from framework)
  input  wire [(0+NUM_PORTS)*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [       (0+NUM_PORTS)-1:0] s_rfnoc_chdr_tlast,
  input  wire [       (0+NUM_PORTS)-1:0] s_rfnoc_chdr_tvalid,
  output wire [       (0+NUM_PORTS)-1:0] s_rfnoc_chdr_tready,

  // AXIS-CHDR Output Ports (to framework)
  output wire [(0+NUM_PORTS)*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [       (0+NUM_PORTS)-1:0] m_rfnoc_chdr_tlast,
  output wire [       (0+NUM_PORTS)-1:0] m_rfnoc_chdr_tvalid,
  input  wire [       (0+NUM_PORTS)-1:0] m_rfnoc_chdr_tready,

  // AXIS-Ctrl Input Port (from framework)
  input  wire [                    31:0] s_rfnoc_ctrl_tdata,
  input  wire                            s_rfnoc_ctrl_tlast,
  input  wire                            s_rfnoc_ctrl_tvalid,
  output wire                            s_rfnoc_ctrl_tready,

  // AXIS-Ctrl Output Port (to framework)
  output wire [                    31:0] m_rfnoc_ctrl_tdata,
  output wire                            m_rfnoc_ctrl_tlast,
  output wire                            m_rfnoc_ctrl_tvalid,
  input  wire                            m_rfnoc_ctrl_tready
);

  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  // Clocks and Resets
  logic ce_rst;

  // CtrlPort Master
  logic        m_ctrlport_req_wr;
  logic        m_ctrlport_req_rd;
  logic [19:0] m_ctrlport_req_addr;
  logic [31:0] m_ctrlport_req_data;
  logic        m_ctrlport_resp_ack;
  logic [31:0] m_ctrlport_resp_data;

  logic [NUM_PORTS*32*1-1:0] m_in_axis_tdata;
  logic [NUM_PORTS*1-1:0]    m_in_axis_tkeep;
  logic [NUM_PORTS-1:0]      m_in_axis_tlast;
  logic [NUM_PORTS-1:0]      m_in_axis_tvalid;
  logic [NUM_PORTS-1:0]      m_in_axis_tready;
  logic [NUM_PORTS*64-1:0]   m_in_axis_ttimestamp;
  logic [NUM_PORTS-1:0]      m_in_axis_thas_time;
  logic [NUM_PORTS*16-1:0]   m_in_axis_tlength;
  logic [NUM_PORTS-1:0]      m_in_axis_teov;
  logic [NUM_PORTS-1:0]      m_in_axis_teob;

  logic [NUM_PORTS*32*1-1:0] s_out_axis_tdata;
  logic [NUM_PORTS*1-1:0]    s_out_axis_tkeep;
  logic [NUM_PORTS-1:0]      s_out_axis_tlast;
  logic [NUM_PORTS-1:0]      s_out_axis_tvalid;
  logic [NUM_PORTS-1:0]      s_out_axis_tready;
  logic [NUM_PORTS*64-1:0]   s_out_axis_ttimestamp;
  logic [NUM_PORTS-1:0]      s_out_axis_thas_time;
  logic [NUM_PORTS*16-1:0]   s_out_axis_tlength;
  logic [NUM_PORTS-1:0]      s_out_axis_teov;
  logic [NUM_PORTS-1:0]      s_out_axis_teob;

  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  noc_shell_ofdm #(
    .CHDR_W     (CHDR_W),
    .THIS_PORTID(THIS_PORTID),
    .MTU        (MTU),
    .NUM_PORTS  (NUM_PORTS)
  ) noc_shell_ofdm_i (
    //---------------------
    // Framework Interface
    //---------------------
    // Clock Inputs
    .rfnoc_chdr_clk       (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk       (rfnoc_ctrl_clk),
    .ce_clk               (ce_clk),
    // Reset Outputs
    .rfnoc_chdr_rst       (),
    .rfnoc_ctrl_rst       (),
    .ce_rst               (ce_rst),
    // RFNoC Backend Interface
    .rfnoc_core_config    (rfnoc_core_config),
    .rfnoc_core_status    (rfnoc_core_status),
    // CHDR Input Ports  (from framework)
    .s_rfnoc_chdr_tdata   (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast   (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid  (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready  (s_rfnoc_chdr_tready),
    // CHDR Output Ports (to framework)
    .m_rfnoc_chdr_tdata   (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast   (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid  (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready  (m_rfnoc_chdr_tready),
    // AXIS-Ctrl Input Port (from framework)
    .s_rfnoc_ctrl_tdata   (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast   (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid  (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready  (s_rfnoc_ctrl_tready),
    // AXIS-Ctrl Output Port (to framework)
    .m_rfnoc_ctrl_tdata   (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast   (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid  (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready  (m_rfnoc_ctrl_tready),
    //---------------------
    // Client Interface
    //---------------------
    // CtrlPort Clock and Reset
    .ctrlport_clk         (),
    .ctrlport_rst         (),
    // CtrlPort Master
    .m_ctrlport_req_wr    (m_ctrlport_req_wr),
    .m_ctrlport_req_rd    (m_ctrlport_req_rd),
    .m_ctrlport_req_addr  (m_ctrlport_req_addr),
    .m_ctrlport_req_data  (m_ctrlport_req_data),
    .m_ctrlport_resp_ack  (m_ctrlport_resp_ack),
    .m_ctrlport_resp_data (m_ctrlport_resp_data),
    // AXI-Stream Clock and Reset
    .axis_data_clk        (),
    .axis_data_rst        (),
    // Data Stream to User Logic: in
    .m_in_axis_tdata      (m_in_axis_tdata),
    .m_in_axis_tkeep      (m_in_axis_tkeep),
    .m_in_axis_tlast      (m_in_axis_tlast),
    .m_in_axis_tvalid     (m_in_axis_tvalid),
    .m_in_axis_tready     (m_in_axis_tready),
    .m_in_axis_ttimestamp (m_in_axis_ttimestamp),
    .m_in_axis_thas_time  (m_in_axis_thas_time),
    .m_in_axis_tlength    (m_in_axis_tlength),
    .m_in_axis_teov       (m_in_axis_teov),
    .m_in_axis_teob       (m_in_axis_teob),
    // Data Stream from User Logic: out
    .s_out_axis_tdata     (s_out_axis_tdata),
    .s_out_axis_tkeep     (s_out_axis_tkeep),
    .s_out_axis_tlast     (s_out_axis_tlast),
    .s_out_axis_tvalid    (s_out_axis_tvalid),
    .s_out_axis_tready    (s_out_axis_tready),
    .s_out_axis_ttimestamp(s_out_axis_ttimestamp),
    .s_out_axis_thas_time (s_out_axis_thas_time),
    .s_out_axis_tlength   (s_out_axis_tlength),
    .s_out_axis_teov      (s_out_axis_teov),
    .s_out_axis_teob      (s_out_axis_teob)
  );


  //---------------------------------------------------------------------------
  // FFT Core
  //---------------------------------------------------------------------------

  fft_core #(
    .CHDR_W                  (CHDR_W),
    .NUM_CHAN                (NUM_PORTS),
    .MAX_FFT_SIZE_LOG2       (MAX_FFT_SIZE_LOG2),
    .MAX_CP_LEN_LOG2         (MAX_CP_LEN_LOG2),
    .MAX_CP_LIST_LEN_INS_LOG2(MAX_CP_LIST_LEN_INS_LOG2),
    .MAX_CP_LIST_LEN_REM_LOG2(MAX_CP_LIST_LEN_REM_LOG2),
    .CP_INSERTION_REPEAT     (CP_INSERTION_REPEAT),
    .CP_REMOVAL_REPEAT       (CP_REMOVAL_REPEAT)
  ) fft_core_i (
    .ce_clk               (ce_clk),
    .ce_rst               (ce_rst),
    .m_ctrlport_req_wr    (m_ctrlport_req_wr),
    .m_ctrlport_req_rd    (m_ctrlport_req_rd),
    .m_ctrlport_req_addr  (m_ctrlport_req_addr),
    .m_ctrlport_req_data  (m_ctrlport_req_data),
    .m_ctrlport_resp_ack  (m_ctrlport_resp_ack),
    .m_ctrlport_resp_data (m_ctrlport_resp_data),
    .m_in_axis_tdata      (m_in_axis_tdata),
    .m_in_axis_tkeep      (m_in_axis_tkeep),
    .m_in_axis_tlast      (m_in_axis_tlast),
    .m_in_axis_tvalid     (m_in_axis_tvalid),
    .m_in_axis_tready     (m_in_axis_tready),
    .m_in_axis_ttimestamp (m_in_axis_ttimestamp),
    .m_in_axis_thas_time  (m_in_axis_thas_time),
    .m_in_axis_tlength    (m_in_axis_tlength),
    .m_in_axis_teov       (m_in_axis_teov),
    .m_in_axis_teob       (m_in_axis_teob),
    .s_out_axis_tdata     (s_out_axis_tdata),
    .s_out_axis_tkeep     (s_out_axis_tkeep),
    .s_out_axis_tlast     (s_out_axis_tlast),
    .s_out_axis_tvalid    (s_out_axis_tvalid),
    .s_out_axis_tready    (s_out_axis_tready),
    .s_out_axis_ttimestamp(s_out_axis_ttimestamp),
    .s_out_axis_thas_time (s_out_axis_thas_time),
    .s_out_axis_tlength   (s_out_axis_tlength),
    .s_out_axis_teov      (s_out_axis_teov),
    .s_out_axis_teob      (s_out_axis_teob)
  );

endmodule : rfnoc_block_ofdm

`default_nettype wire
