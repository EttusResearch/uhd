//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_window
//
// Description:
//
//   Windowing module for use with FFT block. This block can be loaded with up
//   to MAX_WINDOW_SIZE coefficients. Each input packet is then multiplied by
//   the preloaded coefficients before being output.
//
//   The first sample of the first packet is multiplied by the first
//   coefficient, then the second sample is multiplied by the second
//   coefficient, and so on. After "window size" coefficients, processing
//   continues with the first coefficient again, and the process repeats.
//
//   See rfnoc_block_window_regs.vh for register descriptions.
//
// Parameters:
//
//   THIS_PORTID     : Control crossbar port to which this block is connected
//   CHDR_W          : AXIS-CHDR data bus width
//   MTU             : Maximum transmission unit (i.e., maximum packet size in
//                     CHDR words is 2**MTU).
//   NUM_PORTS       : Number of window instances to include.
//   MAX_WINDOW_SIZE : Maximum window size to support, in number of samples.
//                     This is ideally a power of 2.
//

`default_nettype none


module rfnoc_block_window #(
  parameter [9:0] THIS_PORTID     = 10'd0,
  parameter       CHDR_W          = 64,
  parameter [5:0] MTU             = 10,
  parameter       NUM_PORTS       = 1,
  parameter       MAX_WINDOW_SIZE = 4096
) (
  // RFNoC Framework Clocks and Resets
  input  wire                        rfnoc_chdr_clk,
  input  wire                        rfnoc_ctrl_clk,
  input  wire                        ce_clk,
  // RFNoC Backend Interface
  input  wire [               511:0] rfnoc_core_config,
  output wire [               511:0] rfnoc_core_status,
  // AXIS-CHDR Input Ports (from framework)
  input  wire [NUM_PORTS*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tlast,
  input  wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tvalid,
  output wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tready,
  // AXIS-CHDR Output Ports (to framework)
  output wire [NUM_PORTS*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [       NUM_PORTS-1:0] m_rfnoc_chdr_tlast,
  output wire [       NUM_PORTS-1:0] m_rfnoc_chdr_tvalid,
  input  wire [       NUM_PORTS-1:0] m_rfnoc_chdr_tready,
  // AXIS-Ctrl Input Port (from framework)
  input  wire [                31:0] s_rfnoc_ctrl_tdata,
  input  wire                        s_rfnoc_ctrl_tlast,
  input  wire                        s_rfnoc_ctrl_tvalid,
  output wire                        s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Output Port (to framework)
  output wire [                31:0] m_rfnoc_ctrl_tdata,
  output wire                        m_rfnoc_ctrl_tlast,
  output wire                        m_rfnoc_ctrl_tvalid,
  input  wire                        m_rfnoc_ctrl_tready
);

  `include "rfnoc_block_window_regs.vh"

  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  // CtrlPort Master
  wire                        m_ctrlport_req_wr;
  wire                        m_ctrlport_req_rd;
  wire [19:0]                 m_ctrlport_req_addr;
  wire [31:0]                 m_ctrlport_req_data;
  wire                        m_ctrlport_resp_ack;
  wire [31:0]                 m_ctrlport_resp_data;
  // Payload Stream to User Logic: in
  wire [NUM_PORTS*32*1-1:0]   m_in_payload_tdata;
  wire [NUM_PORTS-1:0]        m_in_payload_tlast;
  wire [NUM_PORTS-1:0]        m_in_payload_tvalid;
  wire [NUM_PORTS-1:0]        m_in_payload_tready;
  // Context Stream to User Logic: in
  wire [NUM_PORTS*CHDR_W-1:0] m_in_context_tdata;
  wire [NUM_PORTS*4-1:0]      m_in_context_tuser;
  wire [NUM_PORTS-1:0]        m_in_context_tlast;
  wire [NUM_PORTS-1:0]        m_in_context_tvalid;
  wire [NUM_PORTS-1:0]        m_in_context_tready;
  // Payload Stream to User Logic: out
  wire [NUM_PORTS*32*1-1:0]   s_out_payload_tdata;
  wire [NUM_PORTS-1:0]        s_out_payload_tlast;
  wire [NUM_PORTS-1:0]        s_out_payload_tvalid;
  wire [NUM_PORTS-1:0]        s_out_payload_tready;
  // Context Stream to User Logic: out
  wire [NUM_PORTS*CHDR_W-1:0] s_out_context_tdata;
  wire [NUM_PORTS*4-1:0]      s_out_context_tuser;
  wire [NUM_PORTS-1:0]        s_out_context_tlast;
  wire [NUM_PORTS-1:0]        s_out_context_tvalid;
  wire [NUM_PORTS-1:0]        s_out_context_tready;

  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  wire ce_rst;

  noc_shell_window #(
    .CHDR_W      (CHDR_W),
    .THIS_PORTID (THIS_PORTID),
    .MTU         (MTU),
    .NUM_PORTS   (NUM_PORTS)
  ) noc_shell_window_i (
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

    // AXI-Stream Payload Context Clock and Reset
    .axis_data_clk        (),
    .axis_data_rst        (),
    // Payload Stream to User Logic: in
    .m_in_payload_tdata   (m_in_payload_tdata),
    .m_in_payload_tkeep   (),
    .m_in_payload_tlast   (m_in_payload_tlast),
    .m_in_payload_tvalid  (m_in_payload_tvalid),
    .m_in_payload_tready  (m_in_payload_tready),
    // Context Stream to User Logic: in
    .m_in_context_tdata   (m_in_context_tdata),
    .m_in_context_tuser   (m_in_context_tuser),
    .m_in_context_tlast   (m_in_context_tlast),
    .m_in_context_tvalid  (m_in_context_tvalid),
    .m_in_context_tready  (m_in_context_tready),
    // Payload Stream from User Logic: out
    .s_out_payload_tdata  (s_out_payload_tdata),
    .s_out_payload_tkeep  ({NUM_PORTS{1'b1}}),
    .s_out_payload_tlast  (s_out_payload_tlast),
    .s_out_payload_tvalid (s_out_payload_tvalid),
    .s_out_payload_tready (s_out_payload_tready),
    // Context Stream from User Logic: out
    .s_out_context_tdata  (s_out_context_tdata),
    .s_out_context_tuser  (s_out_context_tuser),
    .s_out_context_tlast  (s_out_context_tlast),
    .s_out_context_tvalid (s_out_context_tvalid),
    .s_out_context_tready (s_out_context_tready)
  );

  // Input packets have the same properties as output packets, so pass through
  // the header information unchanged.
  assign s_out_context_tdata  = m_in_context_tdata;
  assign s_out_context_tuser  = m_in_context_tuser;
  assign s_out_context_tlast  = m_in_context_tlast;
  assign s_out_context_tvalid = m_in_context_tvalid;
  assign m_in_context_tready  = s_out_context_tready;


  //---------------------------------------------------------------------------
  // CtrlPort Splitter
  //---------------------------------------------------------------------------

  // Create a CtrlPort bus for each port instance

  wire [ 1*NUM_PORTS-1:0] ctrlport_req_wr;
  wire [ 1*NUM_PORTS-1:0] ctrlport_req_rd;
  wire [20*NUM_PORTS-1:0] ctrlport_req_addr;
  wire [32*NUM_PORTS-1:0] ctrlport_req_data;
  wire [ 1*NUM_PORTS-1:0] ctrlport_resp_ack;
  wire [32*NUM_PORTS-1:0] ctrlport_resp_data;

  ctrlport_decoder #(
    .NUM_SLAVES   (NUM_PORTS),
    .BASE_ADDR    (0),
    .SLAVE_ADDR_W (WINDOW_ADDR_W)
  ) ctrlport_decoder_i (
    .ctrlport_clk            (ce_clk),
    .ctrlport_rst            (ce_rst),
    .s_ctrlport_req_wr       (m_ctrlport_req_wr),
    .s_ctrlport_req_rd       (m_ctrlport_req_rd),
    .s_ctrlport_req_addr     (m_ctrlport_req_addr),
    .s_ctrlport_req_data     (m_ctrlport_req_data),
    .s_ctrlport_req_byte_en  (4'hF),
    .s_ctrlport_req_has_time (1'b0),
    .s_ctrlport_req_time     (64'b0),
    .s_ctrlport_resp_ack     (m_ctrlport_resp_ack),
    .s_ctrlport_resp_status  (),
    .s_ctrlport_resp_data    (m_ctrlport_resp_data),
    .m_ctrlport_req_wr       (ctrlport_req_wr),
    .m_ctrlport_req_rd       (ctrlport_req_rd),
    .m_ctrlport_req_addr     (ctrlport_req_addr),
    .m_ctrlport_req_data     (ctrlport_req_data),
    .m_ctrlport_req_byte_en  (),
    .m_ctrlport_req_has_time (),
    .m_ctrlport_req_time     (),
    .m_ctrlport_resp_ack     (ctrlport_resp_ack),
    .m_ctrlport_resp_status  ({NUM_PORTS{2'b0}}),
    .m_ctrlport_resp_data    (ctrlport_resp_data)
  );

  //---------------------------------------------------------------------------
  // Port Instances
  //---------------------------------------------------------------------------

  genvar port;
  generate
    for (port = 0; port < NUM_PORTS; port = port+1) begin : gen_ports

      rfnoc_window_core #(
        .MAX_WINDOW_SIZE (MAX_WINDOW_SIZE),
        .COEFF_WIDTH     (REG_LOAD_COEFF_LEN)
      ) rfnoc_window_core_i (
        .clk                  (ce_clk),
        .rst                  (ce_rst),
        .s_ctrlport_req_wr    (ctrlport_req_wr      [port* 1 +:  1]),
        .s_ctrlport_req_rd    (ctrlport_req_rd      [port* 1 +:  1]),
        .s_ctrlport_req_addr  (ctrlport_req_addr    [port*20 +: 20]),
        .s_ctrlport_req_data  (ctrlport_req_data    [port*32 +: 32]),
        .s_ctrlport_resp_ack  (ctrlport_resp_ack    [port* 1 +:  1]),
        .s_ctrlport_resp_data (ctrlport_resp_data   [port*32 +: 32]),
        .s_tdata              (m_in_payload_tdata   [port*32 +: 32]),
        .s_tlast              (m_in_payload_tlast   [port* 1 +:  1]),
        .s_tvalid             (m_in_payload_tvalid  [port* 1 +:  1]),
        .s_tready             (m_in_payload_tready  [port* 1 +:  1]),
        .m_tdata              (s_out_payload_tdata  [port*32 +: 32]),
        .m_tlast              (s_out_payload_tlast  [port* 1 +:  1]),
        .m_tvalid             (s_out_payload_tvalid [port* 1 +:  1]),
        .m_tready             (s_out_payload_tready [port* 1 +:  1])
      );

    end
  endgenerate

endmodule

`default_nettype wire
