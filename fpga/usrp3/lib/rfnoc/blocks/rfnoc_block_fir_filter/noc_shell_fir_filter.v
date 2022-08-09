//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: noc_shell_fir_filter
//
// Description:
//
//   This is a tool-generated NoC-shell for the fir_filter block.
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


module noc_shell_fir_filter #(
  parameter [9:0] THIS_PORTID     = 10'd0,
  parameter       CHDR_W          = 64,
  parameter [5:0] MTU             = 10,
  parameter       NUM_PORTS       = 1,
  parameter       COEFF_WIDTH     = 16,
  parameter       NUM_COEFFS      = 41,
  parameter       COEFFS_VEC      = { 16'h7FFF, {640{1'b0}} },
  parameter       RELOADABLE_COEFFS = 1,
  parameter       SYMMETRIC_COEFFS = 0,
  parameter       SKIP_ZERO_COEFFS = 0,
  parameter       USE_EMBEDDED_REGS_COEFFS = 1
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
  input  wire [(0+NUM_PORTS)*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [(0+NUM_PORTS)-1:0]        s_rfnoc_chdr_tlast,
  input  wire [(0+NUM_PORTS)-1:0]        s_rfnoc_chdr_tvalid,
  output wire [(0+NUM_PORTS)-1:0]        s_rfnoc_chdr_tready,
  // AXIS-CHDR Output Ports (to framework)
  output wire [(0+NUM_PORTS)*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [(0+NUM_PORTS)-1:0]        m_rfnoc_chdr_tlast,
  output wire [(0+NUM_PORTS)-1:0]        m_rfnoc_chdr_tvalid,
  input  wire [(0+NUM_PORTS)-1:0]        m_rfnoc_chdr_tready,

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

  // AXI-Stream Payload Context Clock and Reset
  output wire               axis_data_clk,
  output wire               axis_data_rst,
  // Payload Stream to User Logic: in
  output wire [NUM_PORTS*32*1-1:0]   m_in_payload_tdata,
  output wire [NUM_PORTS*1-1:0]      m_in_payload_tkeep,
  output wire [NUM_PORTS-1:0]        m_in_payload_tlast,
  output wire [NUM_PORTS-1:0]        m_in_payload_tvalid,
  input  wire [NUM_PORTS-1:0]        m_in_payload_tready,
  // Context Stream to User Logic: in
  output wire [NUM_PORTS*CHDR_W-1:0] m_in_context_tdata,
  output wire [NUM_PORTS*4-1:0]      m_in_context_tuser,
  output wire [NUM_PORTS-1:0]        m_in_context_tlast,
  output wire [NUM_PORTS-1:0]        m_in_context_tvalid,
  input  wire [NUM_PORTS-1:0]        m_in_context_tready,
  // Payload Stream from User Logic: out
  input  wire [NUM_PORTS*32*1-1:0]   s_out_payload_tdata,
  input  wire [NUM_PORTS*1-1:0]      s_out_payload_tkeep,
  input  wire [NUM_PORTS-1:0]        s_out_payload_tlast,
  input  wire [NUM_PORTS-1:0]        s_out_payload_tvalid,
  output wire [NUM_PORTS-1:0]        s_out_payload_tready,
  // Context Stream from User Logic: out
  input  wire [NUM_PORTS*CHDR_W-1:0] s_out_context_tdata,
  input  wire [NUM_PORTS*4-1:0]      s_out_context_tuser,
  input  wire [NUM_PORTS-1:0]        s_out_context_tlast,
  input  wire [NUM_PORTS-1:0]        s_out_context_tvalid,
  output wire [NUM_PORTS-1:0]        s_out_context_tready
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
    .NOC_ID        (32'hF1120000),
    .NUM_DATA_I    (0+NUM_PORTS),
    .NUM_DATA_O    (0+NUM_PORTS),
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

  for (i = 0; i < NUM_PORTS; i = i + 1) begin: gen_input_in
    chdr_to_axis_pyld_ctxt #(
      .CHDR_W              (CHDR_W),
      .ITEM_W              (32),
      .NIPC                (1),
      .SYNC_CLKS           (0),
      .CONTEXT_FIFO_SIZE   ($clog2(2)),
      .PAYLOAD_FIFO_SIZE   ($clog2(32)),
      .CONTEXT_PREFETCH_EN (1)
    ) chdr_to_axis_pyld_ctxt_in_in (
      .axis_chdr_clk         (rfnoc_chdr_clk),
      .axis_chdr_rst         (rfnoc_chdr_rst),
      .axis_data_clk         (axis_data_clk),
      .axis_data_rst         (axis_data_rst),
      .s_axis_chdr_tdata     (s_rfnoc_chdr_tdata[((0+i)*CHDR_W)+:CHDR_W]),
      .s_axis_chdr_tlast     (s_rfnoc_chdr_tlast[0+i]),
      .s_axis_chdr_tvalid    (s_rfnoc_chdr_tvalid[0+i]),
      .s_axis_chdr_tready    (s_rfnoc_chdr_tready[0+i]),
      .m_axis_payload_tdata  (m_in_payload_tdata[(32*1)*i+:(32*1)]),
      .m_axis_payload_tkeep  (m_in_payload_tkeep[1*i+:1]),
      .m_axis_payload_tlast  (m_in_payload_tlast[i]),
      .m_axis_payload_tvalid (m_in_payload_tvalid[i]),
      .m_axis_payload_tready (m_in_payload_tready[i]),
      .m_axis_context_tdata  (m_in_context_tdata[CHDR_W*i+:CHDR_W]),
      .m_axis_context_tuser  (m_in_context_tuser[4*i+:4]),
      .m_axis_context_tlast  (m_in_context_tlast[i]),
      .m_axis_context_tvalid (m_in_context_tvalid[i]),
      .m_axis_context_tready (m_in_context_tready[i]),
      .flush_en              (data_i_flush_en),
      .flush_timeout         (data_i_flush_timeout),
      .flush_active          (data_i_flush_active[0+i]),
      .flush_done            (data_i_flush_done[0+i])
    );
  end

  //---------------------
  // Output Data Paths
  //---------------------

  for (i = 0; i < NUM_PORTS; i = i + 1) begin: gen_output_out
    axis_pyld_ctxt_to_chdr #(
      .CHDR_W              (CHDR_W),
      .ITEM_W              (32),
      .NIPC                (1),
      .SYNC_CLKS           (0),
      .CONTEXT_FIFO_SIZE   ($clog2(2)),
      .PAYLOAD_FIFO_SIZE   ($clog2(32)),
      .MTU                 (MTU),
      .CONTEXT_PREFETCH_EN (1)
    ) axis_pyld_ctxt_to_chdr_out_out (
      .axis_chdr_clk         (rfnoc_chdr_clk),
      .axis_chdr_rst         (rfnoc_chdr_rst),
      .axis_data_clk         (axis_data_clk),
      .axis_data_rst         (axis_data_rst),
      .m_axis_chdr_tdata     (m_rfnoc_chdr_tdata[(0+i)*CHDR_W+:CHDR_W]),
      .m_axis_chdr_tlast     (m_rfnoc_chdr_tlast[0+i]),
      .m_axis_chdr_tvalid    (m_rfnoc_chdr_tvalid[0+i]),
      .m_axis_chdr_tready    (m_rfnoc_chdr_tready[0+i]),
      .s_axis_payload_tdata  (s_out_payload_tdata[(32*1)*i+:(32*1)]),
      .s_axis_payload_tkeep  (s_out_payload_tkeep[1*i+:1]),
      .s_axis_payload_tlast  (s_out_payload_tlast[i]),
      .s_axis_payload_tvalid (s_out_payload_tvalid[i]),
      .s_axis_payload_tready (s_out_payload_tready[i]),
      .s_axis_context_tdata  (s_out_context_tdata[CHDR_W*i+:CHDR_W]),
      .s_axis_context_tuser  (s_out_context_tuser[4*i+:4]),
      .s_axis_context_tlast  (s_out_context_tlast[i]),
      .s_axis_context_tvalid (s_out_context_tvalid[i]),
      .s_axis_context_tready (s_out_context_tready[i]),
      .framer_errors         (),
      .flush_en              (data_o_flush_en),
      .flush_timeout         (data_o_flush_timeout),
      .flush_active          (data_o_flush_active[0+i]),
      .flush_done            (data_o_flush_done[0+i])
    );
  end

endmodule // noc_shell_fir_filter


`default_nettype wire
