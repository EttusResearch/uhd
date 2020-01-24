//
// Copyright 2018-2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_endpoint
// Description:
//   A bidirectional AXIS-Control to Control-Port converter.
//   Use this module in noc_shell to interface between the user
//   logic (using ctrlport) and the rfnoc infrastructure (axis_ctrl)
//
// Parameters:
//   - THIS_PORTID: The 10-bit ID of the control XB port that is
//                  connected to this converter.
//   - SYNC_CLKS: Is rfnoc_ctrl_clk and ctrlport_clk the same clock?
//   - AXIS_CTRL_MST_EN: Enable an AXIS-Ctrl master
//   - AXIS_CTRL_SLV_EN: Enable an AXIS-Ctrl slave
//   - SLAVE_FIFO_SIZE: FIFO depth for the slave port
//
// Signals:
//   - *_rfnoc_ctrl_* : Input/output AXIS-Control stream (AXI-Stream)
//   - *_ctrlport_*   : Input/output control-port bus

module ctrlport_endpoint #(
  parameter [9:0] THIS_PORTID       = 10'd0,
  parameter       SYNC_CLKS         = 0,
  parameter [0:0] AXIS_CTRL_MST_EN  = 1,
  parameter [0:0] AXIS_CTRL_SLV_EN  = 1,
  parameter       SLAVE_FIFO_SIZE   = 5
)(
  // Clocks, Resets, Misc
  input  wire         rfnoc_ctrl_clk,
  input  wire         rfnoc_ctrl_rst,
  input  wire         ctrlport_clk,
  input  wire         ctrlport_rst,
  // AXIS-Control Bus
  input  wire [31:0]  s_rfnoc_ctrl_tdata,
  input  wire         s_rfnoc_ctrl_tlast,
  input  wire         s_rfnoc_ctrl_tvalid,
  output wire         s_rfnoc_ctrl_tready,
  output wire [31:0]  m_rfnoc_ctrl_tdata,
  output wire         m_rfnoc_ctrl_tlast,
  output wire         m_rfnoc_ctrl_tvalid,
  input  wire         m_rfnoc_ctrl_tready,
  // Control Port Master (Request)
  output wire         m_ctrlport_req_wr,
  output wire         m_ctrlport_req_rd,
  output wire [19:0]  m_ctrlport_req_addr,
  output wire [31:0]  m_ctrlport_req_data,
  output wire [3:0]   m_ctrlport_req_byte_en,
  output wire         m_ctrlport_req_has_time,
  output wire [63:0]  m_ctrlport_req_time,
  // Control Port Master (Response)
  input  wire         m_ctrlport_resp_ack,
  input  wire [1:0]   m_ctrlport_resp_status,
  input  wire [31:0]  m_ctrlport_resp_data,
  // Control Port Slave (Request)
  input  wire         s_ctrlport_req_wr,
  input  wire         s_ctrlport_req_rd,
  input  wire [19:0]  s_ctrlport_req_addr,
  input  wire [9:0]   s_ctrlport_req_portid,
  input  wire [15:0]  s_ctrlport_req_rem_epid,
  input  wire [9:0]   s_ctrlport_req_rem_portid,
  input  wire [31:0]  s_ctrlport_req_data,
  input  wire [3:0]   s_ctrlport_req_byte_en,
  input  wire         s_ctrlport_req_has_time,
  input  wire [63:0]  s_ctrlport_req_time,
  // Control Port Slave (Response)
  output wire         s_ctrlport_resp_ack,
  output wire [1:0]   s_ctrlport_resp_status,
  output wire [31:0]  s_ctrlport_resp_data
);

  // ---------------------------------------------------
  //  RFNoC Includes
  // ---------------------------------------------------
  `include "rfnoc_chdr_utils.vh"
  `include "rfnoc_axis_ctrl_utils.vh"

  // ---------------------------------------------------
  //  Clock Crossing
  // ---------------------------------------------------

  wire [31:0] i_ctrl_tdata,  o_ctrl_tdata;
  wire        i_ctrl_tlast,  o_ctrl_tlast;
  wire        i_ctrl_tvalid, o_ctrl_tvalid;
  wire        i_ctrl_tready, o_ctrl_tready;

  generate
    if (SYNC_CLKS) begin : gen_sync_fifos
      axi_fifo #(.WIDTH(32+1), .SIZE(1)) in_fifo_i (
        .clk(ctrlport_clk), .reset(ctrlport_rst), .clear(1'b0),
        .i_tdata({s_rfnoc_ctrl_tlast, s_rfnoc_ctrl_tdata}),
        .i_tvalid(s_rfnoc_ctrl_tvalid), .i_tready(s_rfnoc_ctrl_tready),
        .o_tdata({i_ctrl_tlast, i_ctrl_tdata}),
        .o_tvalid(i_ctrl_tvalid), .o_tready(i_ctrl_tready),
        .space(), .occupied()
      );

      axi_fifo #(.WIDTH(32+1), .SIZE(1)) out_fifo_i (
        .clk(ctrlport_clk), .reset(ctrlport_rst), .clear(1'b0),
        .i_tdata({o_ctrl_tlast, o_ctrl_tdata}),
        .i_tvalid(o_ctrl_tvalid), .i_tready(o_ctrl_tready),
        .o_tdata({m_rfnoc_ctrl_tlast, m_rfnoc_ctrl_tdata}),
        .o_tvalid(m_rfnoc_ctrl_tvalid), .o_tready(m_rfnoc_ctrl_tready),
        .space(), .occupied()
      );
    end else begin : gen_async_fifos
      axi_fifo_2clk #(.WIDTH(32+1), .SIZE(1), .PIPELINE("IN")) in_fifo_i (
        .reset(rfnoc_ctrl_rst),
        .i_aclk(rfnoc_ctrl_clk),
        .i_tdata({s_rfnoc_ctrl_tlast, s_rfnoc_ctrl_tdata}),
        .i_tvalid(s_rfnoc_ctrl_tvalid), .i_tready(s_rfnoc_ctrl_tready),
        .o_aclk(ctrlport_clk),
        .o_tdata({i_ctrl_tlast, i_ctrl_tdata}),
        .o_tvalid(i_ctrl_tvalid), .o_tready(i_ctrl_tready)
      );

      axi_fifo_2clk #(.WIDTH(32+1), .SIZE(1), .PIPELINE("OUT")) out_fifo_i (
        .reset(ctrlport_rst),
        .i_aclk(ctrlport_clk),
        .i_tdata({o_ctrl_tlast, o_ctrl_tdata}),
        .i_tvalid(o_ctrl_tvalid), .i_tready(o_ctrl_tready),
        .o_aclk(rfnoc_ctrl_clk),
        .o_tdata({m_rfnoc_ctrl_tlast, m_rfnoc_ctrl_tdata}),
        .o_tvalid(m_rfnoc_ctrl_tvalid), .o_tready(m_rfnoc_ctrl_tready)
      );

    end
  endgenerate

  // ---------------------------------------------------
  //  MUXING
  // ---------------------------------------------------
  wire [31:0] mst_req_tdata,  mst_resp_tdata ;
  wire        mst_req_tlast,  mst_resp_tlast ;
  wire        mst_req_tvalid, mst_resp_tvalid;
  wire        mst_req_tready, mst_resp_tready;

  wire [31:0] slv_req_tdata,  slv_req_fifo_tdata,  slv_resp_tdata ;
  wire        slv_req_tlast,  slv_req_fifo_tlast,  slv_resp_tlast ;
  wire        slv_req_tvalid, slv_req_fifo_tvalid, slv_resp_tvalid;
  wire        slv_req_tready, slv_req_fifo_tready, slv_resp_tready;

  generate
    if (AXIS_CTRL_MST_EN == 1'b1 && AXIS_CTRL_SLV_EN == 1'b1) begin : gen_mst_slv_muxing
      wire [31:0] in_hdr;
      axi_demux #(
        .WIDTH(32), .SIZE(2), .PRE_FIFO_SIZE(0), .POST_FIFO_SIZE(0)
      ) demux_i (
        .clk(ctrlport_clk), .reset(ctrlport_rst), .clear(1'b0),
        .header(in_hdr), .dest(axis_ctrl_get_is_ack(in_hdr)),
        .i_tdata (i_ctrl_tdata ),
        .i_tlast (i_ctrl_tlast ),
        .i_tvalid(i_ctrl_tvalid),
        .i_tready(i_ctrl_tready),
        .o_tdata ({mst_resp_tdata,  slv_req_tdata }),
        .o_tlast ({mst_resp_tlast,  slv_req_tlast }),
        .o_tvalid({mst_resp_tvalid, slv_req_tvalid}),
        .o_tready({mst_resp_tready, slv_req_tready})
      );

      axi_mux #(
        .WIDTH(32), .SIZE(2), .PRIO(0), .PRE_FIFO_SIZE(0), .POST_FIFO_SIZE(0)
      ) mux_i (
        .clk(ctrlport_clk), .reset(ctrlport_rst), .clear(1'b0),
        .i_tdata ({mst_req_tdata,  slv_resp_tdata }),
        .i_tlast ({mst_req_tlast,  slv_resp_tlast }),
        .i_tvalid({mst_req_tvalid, slv_resp_tvalid}),
        .i_tready({mst_req_tready, slv_resp_tready}),
        .o_tdata (o_ctrl_tdata ),
        .o_tlast (o_ctrl_tlast ),
        .o_tvalid(o_ctrl_tvalid),
        .o_tready(o_ctrl_tready)
      );

    end else if (AXIS_CTRL_MST_EN == 1'b1) begin : gen_mst_muxing

      assign mst_resp_tdata  = i_ctrl_tdata;
      assign mst_resp_tlast  = i_ctrl_tlast;
      assign mst_resp_tvalid = i_ctrl_tvalid;
      assign i_ctrl_tready   = mst_resp_tready;

      assign o_ctrl_tdata    = mst_req_tdata;
      assign o_ctrl_tlast    = mst_req_tlast;
      assign o_ctrl_tvalid   = mst_req_tvalid;
      assign mst_req_tready  = o_ctrl_tready;

    end else begin : gen_no_mst_muxing

      assign slv_req_tdata   = i_ctrl_tdata;
      assign slv_req_tlast   = i_ctrl_tlast;
      assign slv_req_tvalid  = i_ctrl_tvalid;
      assign i_ctrl_tready   = slv_req_tready;

      assign o_ctrl_tdata    = slv_resp_tdata;
      assign o_ctrl_tlast    = slv_resp_tlast;
      assign o_ctrl_tvalid   = slv_resp_tvalid;
      assign slv_resp_tready = o_ctrl_tready;

    end
  endgenerate

  // ---------------------------------------------------
  //  AXIS Control Master and Slave
  // ---------------------------------------------------

  generate
    if (AXIS_CTRL_MST_EN == 1'b1) begin : gen_ctrl_master
      axis_ctrl_master #( .THIS_PORTID(THIS_PORTID) ) axis_ctrl_mst_i (
        .clk                    (ctrlport_clk),
        .rst                    (ctrlport_rst),
        .s_axis_ctrl_tdata      (mst_resp_tdata),
        .s_axis_ctrl_tlast      (mst_resp_tlast),
        .s_axis_ctrl_tvalid     (mst_resp_tvalid),
        .s_axis_ctrl_tready     (mst_resp_tready),
        .m_axis_ctrl_tdata      (mst_req_tdata),
        .m_axis_ctrl_tlast      (mst_req_tlast),
        .m_axis_ctrl_tvalid     (mst_req_tvalid),
        .m_axis_ctrl_tready     (mst_req_tready),
        .ctrlport_req_wr        (s_ctrlport_req_wr),
        .ctrlport_req_rd        (s_ctrlport_req_rd),
        .ctrlport_req_addr      (s_ctrlport_req_addr),
        .ctrlport_req_portid    (s_ctrlport_req_portid),
        .ctrlport_req_rem_epid  (s_ctrlport_req_rem_epid),
        .ctrlport_req_rem_portid(s_ctrlport_req_rem_portid),
        .ctrlport_req_data      (s_ctrlport_req_data),
        .ctrlport_req_byte_en   (s_ctrlport_req_byte_en),
        .ctrlport_req_has_time  (s_ctrlport_req_has_time),
        .ctrlport_req_time      (s_ctrlport_req_time),
        .ctrlport_resp_ack      (s_ctrlport_resp_ack),
        .ctrlport_resp_status   (s_ctrlport_resp_status),
        .ctrlport_resp_data     (s_ctrlport_resp_data)
      );
    end else begin : gen_no_ctrl_master
      assign mst_resp_tready = 1'b1;
      assign mst_req_tlast = 1'b0;
      assign mst_req_tvalid = 1'b0;
      assign s_ctrlport_resp_ack = 1'b0;
    end

    if (AXIS_CTRL_SLV_EN == 1'b1) begin : gen_ctrl_slave
      axi_fifo #(.WIDTH(32+1), .SIZE(SLAVE_FIFO_SIZE)) slv_fifo_i (
        .clk(ctrlport_clk), .reset(ctrlport_rst), .clear(1'b0),
        .i_tdata({slv_req_tlast, slv_req_tdata}),
        .i_tvalid(slv_req_tvalid), .i_tready(slv_req_tready),
        .o_tdata({slv_req_fifo_tlast, slv_req_fifo_tdata}),
        .o_tvalid(slv_req_fifo_tvalid), .o_tready(slv_req_fifo_tready),
        .space(), .occupied()
      );

      axis_ctrl_slave axis_ctrl_slv_i (
        .clk                  (ctrlport_clk),
        .rst                  (ctrlport_rst),
        .s_axis_ctrl_tdata    (slv_req_fifo_tdata),
        .s_axis_ctrl_tlast    (slv_req_fifo_tlast),
        .s_axis_ctrl_tvalid   (slv_req_fifo_tvalid),
        .s_axis_ctrl_tready   (slv_req_fifo_tready),
        .m_axis_ctrl_tdata    (slv_resp_tdata),
        .m_axis_ctrl_tlast    (slv_resp_tlast),
        .m_axis_ctrl_tvalid   (slv_resp_tvalid),
        .m_axis_ctrl_tready   (slv_resp_tready),
        .ctrlport_req_wr      (m_ctrlport_req_wr),
        .ctrlport_req_rd      (m_ctrlport_req_rd),
        .ctrlport_req_addr    (m_ctrlport_req_addr),
        .ctrlport_req_data    (m_ctrlport_req_data),
        .ctrlport_req_byte_en (m_ctrlport_req_byte_en),
        .ctrlport_req_has_time(m_ctrlport_req_has_time),
        .ctrlport_req_time    (m_ctrlport_req_time),
        .ctrlport_resp_ack    (m_ctrlport_resp_ack),
        .ctrlport_resp_status (m_ctrlport_resp_status),
        .ctrlport_resp_data   (m_ctrlport_resp_data)
      );
    end else begin : gen_no_ctrl_slave
      assign slv_req_fifo_tready = 1'b1;
      assign slv_resp_tlast = 1'b0;
      assign slv_resp_tvalid = 1'b0;
      assign m_ctrlport_req_wr = 1'b0;
      assign m_ctrlport_req_rd = 1'b0;
    end
  endgenerate

endmodule // ctrlport_endpoint

