//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: noc_shell_switchboard
//
// Description:
//
//   This is a tool-generated NoC-shell for the switchboard block.
//   See the RFNoC specification for more information about NoC shells.
//
// Parameters:
//
//   THIS_PORTID : Control crossbar port to which this block is connected
//   CHDR_W      : AXIS-CHDR data bus width
//   MTU         : Maximum transmission unit (i.e., maximum packet size in
//                 CHDR words is 2**MTU).
//

`default_nettype none


module noc_shell_switchboard #(
  parameter [9:0] THIS_PORTID     = 10'd0,
  parameter       CHDR_W          = 64,
  parameter [5:0] MTU             = 10,
  parameter       NUM_INPUTS      = 1,
  parameter       NUM_OUTPUTS     = 1
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

  // AXIS-CHDR Input Ports (from framework)
  input  wire [(NUM_INPUTS)*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [(NUM_INPUTS)-1:0]        s_rfnoc_chdr_tlast,
  input  wire [(NUM_INPUTS)-1:0]        s_rfnoc_chdr_tvalid,
  output wire [(NUM_INPUTS)-1:0]        s_rfnoc_chdr_tready,
  // AXIS-CHDR Output Ports (to framework)
  output wire [(NUM_OUTPUTS)*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [(NUM_OUTPUTS)-1:0]        m_rfnoc_chdr_tlast,
  output wire [(NUM_OUTPUTS)-1:0]        m_rfnoc_chdr_tvalid,
  input  wire [(NUM_OUTPUTS)-1:0]        m_rfnoc_chdr_tready,

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
  input  wire [31:0]        m_ctrlport_resp_data,

  // AXIS-CHDR Clock and Reset
  output wire               axis_chdr_clk,
  output wire               axis_chdr_rst,
  // Framework to User Logic: in
  output wire [NUM_INPUTS*CHDR_W-1:0]  m_in_chdr_tdata,
  output wire [NUM_INPUTS-1:0]         m_in_chdr_tlast,
  output wire [NUM_INPUTS-1:0]         m_in_chdr_tvalid,
  input  wire [NUM_INPUTS-1:0]         m_in_chdr_tready,
  // User Logic to Framework: out
  input  wire [NUM_OUTPUTS*CHDR_W-1:0]  s_out_chdr_tdata,
  input  wire [NUM_OUTPUTS-1:0]         s_out_chdr_tlast,
  input  wire [NUM_OUTPUTS-1:0]         s_out_chdr_tvalid,
  output wire [NUM_OUTPUTS-1:0]         s_out_chdr_tready
);

  //---------------------------------------------------------------------------
  //  Backend Interface
  //---------------------------------------------------------------------------

  wire         data_i_flush_en;
  wire [31:0]  data_i_flush_timeout;
  wire [63:0]  data_i_flush_active;
  wire [63:0]  data_i_flush_done;
  wire         data_o_flush_en;
  wire [31:0]  data_o_flush_timeout;
  wire [63:0]  data_o_flush_active;
  wire [63:0]  data_o_flush_done;

  backend_iface #(
    .NOC_ID        (32'hBE110000),
    .NUM_DATA_I    (NUM_INPUTS),
    .NUM_DATA_O    (NUM_OUTPUTS),
    .CTRL_FIFOSIZE ($clog2(32)),
    .MTU           (MTU)
  ) backend_iface_i (
    .rfnoc_chdr_clk       (rfnoc_chdr_clk),
    .rfnoc_chdr_rst       (rfnoc_chdr_rst),
    .rfnoc_ctrl_clk       (rfnoc_ctrl_clk),
    .rfnoc_ctrl_rst       (rfnoc_ctrl_rst),
    .rfnoc_core_config    (rfnoc_core_config),
    .rfnoc_core_status    (rfnoc_core_status),
    .data_i_flush_en      (data_i_flush_en),
    .data_i_flush_timeout (data_i_flush_timeout),
    .data_i_flush_active  (data_i_flush_active),
    .data_i_flush_done    (data_i_flush_done),
    .data_o_flush_en      (data_o_flush_en),
    .data_o_flush_timeout (data_o_flush_timeout),
    .data_o_flush_active  (data_o_flush_active),
    .data_o_flush_done    (data_o_flush_done)
  );

  //---------------------------------------------------------------------------
  //  Control Path
  //---------------------------------------------------------------------------

  assign ctrlport_clk = rfnoc_chdr_clk;
  assign ctrlport_rst = rfnoc_chdr_rst;

  ctrlport_endpoint #(
    .THIS_PORTID      (THIS_PORTID),
    .SYNC_CLKS        (0),
    .AXIS_CTRL_MST_EN (0),
    .AXIS_CTRL_SLV_EN (1),
    .SLAVE_FIFO_SIZE  ($clog2(32))
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
    .m_ctrlport_resp_status    (2'b0),
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

  //---------------------------------------------------------------------------
  //  Data Path
  //---------------------------------------------------------------------------

  genvar i;

  assign axis_chdr_clk = rfnoc_chdr_clk;
  assign axis_chdr_rst = rfnoc_chdr_rst;

  //---------------------
  // Input Data Paths
  //---------------------

  for (i = 0; i < NUM_INPUTS; i = i + 1) begin: gen_input_in
    chdr_to_chdr_data #(
      .CHDR_W (CHDR_W)
    ) chdr_to_chdr_data_in_in (
      .axis_chdr_clk      (rfnoc_chdr_clk),
      .axis_chdr_rst      (rfnoc_chdr_rst),
      .s_axis_chdr_tdata  (s_rfnoc_chdr_tdata[(i)*CHDR_W+:CHDR_W]),
      .s_axis_chdr_tlast  (s_rfnoc_chdr_tlast[i]),
      .s_axis_chdr_tvalid (s_rfnoc_chdr_tvalid[i]),
      .s_axis_chdr_tready (s_rfnoc_chdr_tready[i]),
      .m_axis_chdr_tdata  (m_in_chdr_tdata[i*CHDR_W+:CHDR_W]),
      .m_axis_chdr_tlast  (m_in_chdr_tlast[i]),
      .m_axis_chdr_tvalid (m_in_chdr_tvalid[i]),
      .m_axis_chdr_tready (m_in_chdr_tready[i]),
      .flush_en           (data_i_flush_en),
      .flush_timeout      (data_i_flush_timeout),
      .flush_active       (data_i_flush_active[i]),
      .flush_done         (data_i_flush_done[i])
    );
  end

  //---------------------
  // Output Data Paths
  //---------------------

  for (i = 0; i < NUM_OUTPUTS; i = i + 1) begin: gen_output_out
    chdr_to_chdr_data #(
      .CHDR_W (CHDR_W)
    ) chdr_to_chdr_data_out_out (
      .axis_chdr_clk      (rfnoc_chdr_clk),
      .axis_chdr_rst      (rfnoc_chdr_rst),
      .m_axis_chdr_tdata  (m_rfnoc_chdr_tdata[(i)*CHDR_W+:CHDR_W]),
      .m_axis_chdr_tlast  (m_rfnoc_chdr_tlast[i]),
      .m_axis_chdr_tvalid (m_rfnoc_chdr_tvalid[i]),
      .m_axis_chdr_tready (m_rfnoc_chdr_tready[i]),
      .s_axis_chdr_tdata  (s_out_chdr_tdata[i*CHDR_W+:CHDR_W]),
      .s_axis_chdr_tlast  (s_out_chdr_tlast[i]),
      .s_axis_chdr_tvalid (s_out_chdr_tvalid[i]),
      .s_axis_chdr_tready (s_out_chdr_tready[i]),
      .flush_en           (data_o_flush_en),
      .flush_timeout      (data_o_flush_timeout),
      .flush_active       (data_o_flush_active[i]),
      .flush_done         (data_o_flush_done[i])
    );
  end

endmodule // noc_shell_switchboard


`default_nettype wire
