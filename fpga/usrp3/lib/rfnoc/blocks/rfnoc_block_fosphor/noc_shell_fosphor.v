//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: noc_shell_fosphor
//
// Description:
//
//   This is a tool-generated NoC-shell for the fosphor block.
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


module noc_shell_fosphor #(
  parameter [9:0] THIS_PORTID     = 10'd0,
  parameter       CHDR_W          = 64,
  parameter [5:0] MTU             = 10
) (
  //---------------------
  // Framework Interface
  //---------------------

  // RFNoC Framework Clocks
  input  wire rfnoc_chdr_clk,
  input  wire rfnoc_ctrl_clk,
  input  wire ce_clk,

  // NoC Shell Generated Resets
  output wire rfnoc_chdr_rst,
  output wire rfnoc_ctrl_rst,
  output wire ce_rst,

  // RFNoC Backend Interface
  input  wire [511:0]          rfnoc_core_config,
  output wire [511:0]          rfnoc_core_status,

  // AXIS-CHDR Input Ports (from framework)
  input  wire [(1)*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [(1)-1:0]        s_rfnoc_chdr_tlast,
  input  wire [(1)-1:0]        s_rfnoc_chdr_tvalid,
  output wire [(1)-1:0]        s_rfnoc_chdr_tready,
  // AXIS-CHDR Output Ports (to framework)
  output wire [(2)*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [(2)-1:0]        m_rfnoc_chdr_tlast,
  output wire [(2)-1:0]        m_rfnoc_chdr_tvalid,
  input  wire [(2)-1:0]        m_rfnoc_chdr_tready,

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

  // AXI-Stream Data Clock and Reset
  output wire               axis_data_clk,
  output wire               axis_data_rst,
  // Data Stream to User Logic: fft_in
  output wire [32*1-1:0]    m_fft_in_axis_tdata,
  output wire [1-1:0]       m_fft_in_axis_tkeep,
  output wire               m_fft_in_axis_tlast,
  output wire               m_fft_in_axis_tvalid,
  input  wire               m_fft_in_axis_tready,
  output wire [63:0]        m_fft_in_axis_ttimestamp,
  output wire               m_fft_in_axis_thas_time,
  output wire [15:0]        m_fft_in_axis_tlength,
  output wire               m_fft_in_axis_teov,
  output wire               m_fft_in_axis_teob,
  // Data Stream from User Logic: hist
  input  wire [8*4-1:0]    s_hist_axis_tdata,
  input  wire [3:0]         s_hist_axis_tkeep,
  input  wire               s_hist_axis_tlast,
  input  wire               s_hist_axis_tvalid,
  output wire               s_hist_axis_tready,
  input  wire [63:0]        s_hist_axis_ttimestamp,
  input  wire               s_hist_axis_thas_time,
  input  wire [15:0]        s_hist_axis_tlength,
  input  wire               s_hist_axis_teov,
  input  wire               s_hist_axis_teob,
  // Data Stream from User Logic: wf
  input  wire [8*4-1:0]    s_wf_axis_tdata,
  input  wire [3:0]         s_wf_axis_tkeep,
  input  wire               s_wf_axis_tlast,
  input  wire               s_wf_axis_tvalid,
  output wire               s_wf_axis_tready,
  input  wire [63:0]        s_wf_axis_ttimestamp,
  input  wire               s_wf_axis_thas_time,
  input  wire [15:0]        s_wf_axis_tlength,
  input  wire               s_wf_axis_teov,
  input  wire               s_wf_axis_teob
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
    .NOC_ID        (32'h666F0000),
    .NUM_DATA_I    (1),
    .NUM_DATA_O    (2),
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
  //  Reset Generation
  //---------------------------------------------------------------------------

  wire ce_rst_pulse;

  pulse_synchronizer #(.MODE ("POSEDGE")) pulse_synchronizer_ce (
    .clk_a(rfnoc_chdr_clk), .rst_a(1'b0), .pulse_a (rfnoc_chdr_rst), .busy_a (),
    .clk_b(ce_clk), .pulse_b (ce_rst_pulse)
  );

  pulse_stretch_min #(.LENGTH(32)) pulse_stretch_min_ce (
    .clk(ce_clk), .rst(1'b0),
    .pulse_in(ce_rst_pulse), .pulse_out(ce_rst)
  );

  //---------------------------------------------------------------------------
  //  Control Path
  //---------------------------------------------------------------------------

  assign ctrlport_clk = ce_clk;
  assign ctrlport_rst = ce_rst;

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

  assign axis_data_clk = ce_clk;
  assign axis_data_rst = ce_rst;

  //---------------------
  // Input Data Paths
  //---------------------

  chdr_to_axis_data #(
    .CHDR_W         (CHDR_W),
    .ITEM_W         (32),
    .NIPC           (1),
    .SYNC_CLKS      (0),
    .INFO_FIFO_SIZE ($clog2(32)),
    .PYLD_FIFO_SIZE ($clog2(32))
  ) chdr_to_axis_data_in_fft_in (
    .axis_chdr_clk      (rfnoc_chdr_clk),
    .axis_chdr_rst      (rfnoc_chdr_rst),
    .axis_data_clk      (axis_data_clk),
    .axis_data_rst      (axis_data_rst),
    .s_axis_chdr_tdata  (s_rfnoc_chdr_tdata[(0)*CHDR_W+:CHDR_W]),
    .s_axis_chdr_tlast  (s_rfnoc_chdr_tlast[0]),
    .s_axis_chdr_tvalid (s_rfnoc_chdr_tvalid[0]),
    .s_axis_chdr_tready (s_rfnoc_chdr_tready[0]),
    .m_axis_tdata       (m_fft_in_axis_tdata),
    .m_axis_tkeep       (m_fft_in_axis_tkeep),
    .m_axis_tlast       (m_fft_in_axis_tlast),
    .m_axis_tvalid      (m_fft_in_axis_tvalid),
    .m_axis_tready      (m_fft_in_axis_tready),
    .m_axis_ttimestamp  (m_fft_in_axis_ttimestamp),
    .m_axis_thas_time   (m_fft_in_axis_thas_time),
    .m_axis_tlength     (m_fft_in_axis_tlength),
    .m_axis_teov        (m_fft_in_axis_teov),
    .m_axis_teob        (m_fft_in_axis_teob),
    .flush_en           (data_i_flush_en),
    .flush_timeout      (data_i_flush_timeout),
    .flush_active       (data_i_flush_active[0]),
    .flush_done         (data_i_flush_done[0])
  );

  //---------------------
  // Output Data Paths
  //---------------------

  axis_data_to_chdr #(
    .CHDR_W          (CHDR_W),
    .ITEM_W          (8),
    .NIPC            (4),
    .SYNC_CLKS       (0),
    .INFO_FIFO_SIZE  ($clog2(32)),
    .PYLD_FIFO_SIZE  ($clog2(32)),
    .MTU             (MTU),
    .SIDEBAND_AT_END (0)
  ) axis_data_to_chdr_out_hist (
    .axis_chdr_clk      (rfnoc_chdr_clk),
    .axis_chdr_rst      (rfnoc_chdr_rst),
    .axis_data_clk      (axis_data_clk),
    .axis_data_rst      (axis_data_rst),
    .m_axis_chdr_tdata  (m_rfnoc_chdr_tdata[(0)*CHDR_W+:CHDR_W]),
    .m_axis_chdr_tlast  (m_rfnoc_chdr_tlast[0]),
    .m_axis_chdr_tvalid (m_rfnoc_chdr_tvalid[0]),
    .m_axis_chdr_tready (m_rfnoc_chdr_tready[0]),
    .s_axis_tdata       (s_hist_axis_tdata),
    .s_axis_tkeep       (s_hist_axis_tkeep),
    .s_axis_tlast       (s_hist_axis_tlast),
    .s_axis_tvalid      (s_hist_axis_tvalid),
    .s_axis_tready      (s_hist_axis_tready),
    .s_axis_ttimestamp  (s_hist_axis_ttimestamp),
    .s_axis_thas_time   (s_hist_axis_thas_time),
    .s_axis_tlength     (s_hist_axis_tlength),
    .s_axis_teov        (s_hist_axis_teov),
    .s_axis_teob        (s_hist_axis_teob),
    .flush_en           (data_o_flush_en),
    .flush_timeout      (data_o_flush_timeout),
    .flush_active       (data_o_flush_active[0]),
    .flush_done         (data_o_flush_done[0])
  );

  axis_data_to_chdr #(
    .CHDR_W          (CHDR_W),
    .ITEM_W          (8),
    .NIPC            (4),
    .SYNC_CLKS       (0),
    .INFO_FIFO_SIZE  ($clog2(32)),
    .PYLD_FIFO_SIZE  ($clog2(32)),
    .MTU             (MTU),
    .SIDEBAND_AT_END (0)
  ) axis_data_to_chdr_out_wf (
    .axis_chdr_clk      (rfnoc_chdr_clk),
    .axis_chdr_rst      (rfnoc_chdr_rst),
    .axis_data_clk      (axis_data_clk),
    .axis_data_rst      (axis_data_rst),
    .m_axis_chdr_tdata  (m_rfnoc_chdr_tdata[(1)*CHDR_W+:CHDR_W]),
    .m_axis_chdr_tlast  (m_rfnoc_chdr_tlast[1]),
    .m_axis_chdr_tvalid (m_rfnoc_chdr_tvalid[1]),
    .m_axis_chdr_tready (m_rfnoc_chdr_tready[1]),
    .s_axis_tdata       (s_wf_axis_tdata),
    .s_axis_tkeep       (s_wf_axis_tkeep),
    .s_axis_tlast       (s_wf_axis_tlast),
    .s_axis_tvalid      (s_wf_axis_tvalid),
    .s_axis_tready      (s_wf_axis_tready),
    .s_axis_ttimestamp  (s_wf_axis_ttimestamp),
    .s_axis_thas_time   (s_wf_axis_thas_time),
    .s_axis_tlength     (s_wf_axis_tlength),
    .s_axis_teov        (s_wf_axis_teov),
    .s_axis_teob        (s_wf_axis_teob),
    .flush_en           (data_o_flush_en),
    .flush_timeout      (data_o_flush_timeout),
    .flush_active       (data_o_flush_active[1]),
    .flush_done         (data_o_flush_done[1])
  );

endmodule // noc_shell_fosphor


`default_nettype wire
