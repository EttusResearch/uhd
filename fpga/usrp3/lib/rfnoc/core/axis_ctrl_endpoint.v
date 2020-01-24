//
// Copyright 2018-2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_ctrl_endpoint
// Description:
//   A bidirectional AXIS-Control to AXIS-Control converter.
//   Use this module in noc_shell to interface between the user
//   logic and the rfnoc infrastructure when both interfaces use
//   AXIS-Control.
//
// Parameters:
//   - SYNC_CLKS: Is rfnoc_ctrl_clk and axis_ctrl_clk the same clock?
//   - SLAVE_FIFO_SIZE: The depth of the slave FIFO. Note that the 
//                      slave FIFO will also buffer master responses.
//
// Signals:
//   - *_rfnoc_ctrl_* : Input/output AXIS-Control from/to the framework
//   - *_axis_ctrl_*  : Input/output AXIS-Control from/to the user

module axis_ctrl_endpoint #(
  parameter       SYNC_CLKS       = 0,
  parameter       SLAVE_FIFO_SIZE = 5
)(
  // Clocks, Resets, Misc
  input  wire         rfnoc_ctrl_clk,
  input  wire         rfnoc_ctrl_rst,
  input  wire         axis_ctrl_clk,
  input  wire         axis_ctrl_rst,
  // AXIS-Control Bus (RFNoC infrastructure)
  input  wire [31:0]  s_rfnoc_ctrl_tdata,
  input  wire         s_rfnoc_ctrl_tlast,
  input  wire         s_rfnoc_ctrl_tvalid,
  output wire         s_rfnoc_ctrl_tready,
  output wire [31:0]  m_rfnoc_ctrl_tdata,
  output wire         m_rfnoc_ctrl_tlast,
  output wire         m_rfnoc_ctrl_tvalid,
  input  wire         m_rfnoc_ctrl_tready,
  // AXIS-Control Bus (User logic)
  input  wire [31:0]  s_axis_ctrl_tdata,
  input  wire         s_axis_ctrl_tlast,
  input  wire         s_axis_ctrl_tvalid,
  output wire         s_axis_ctrl_tready,
  output wire [31:0]  m_axis_ctrl_tdata,
  output wire         m_axis_ctrl_tlast,
  output wire         m_axis_ctrl_tvalid,
  input  wire         m_axis_ctrl_tready
);

  // ---------------------------------------------------
  //  RFNoC Includes
  // ---------------------------------------------------
  `include "rfnoc_chdr_utils.vh"
  `include "rfnoc_axis_ctrl_utils.vh"

  // ---------------------------------------------------
  //  Clock Crossing
  // ---------------------------------------------------

  wire [31:0] i_ctrl_tdata;
  wire        i_ctrl_tlast, i_ctrl_tvalid, i_ctrl_tready;

  generate
    if (SYNC_CLKS) begin
      axi_fifo #(.WIDTH(32+1), .SIZE(SLAVE_FIFO_SIZE)) in_fifo_i (
        .clk(axis_ctrl_clk), .reset(axis_ctrl_rst), .clear(1'b0),
        .i_tdata({s_rfnoc_ctrl_tlast, s_rfnoc_ctrl_tdata}),
        .i_tvalid(s_rfnoc_ctrl_tvalid), .i_tready(s_rfnoc_ctrl_tready),
        .o_tdata({i_ctrl_tlast, i_ctrl_tdata}),
        .o_tvalid(i_ctrl_tvalid), .o_tready(i_ctrl_tready),
        .space(), .occupied()
      );

      axi_fifo #(.WIDTH(32+1), .SIZE(1)) out_fifo_i (
        .clk(axis_ctrl_clk), .reset(axis_ctrl_rst), .clear(1'b0),
        .i_tdata({s_axis_ctrl_tlast, s_axis_ctrl_tdata}),
        .i_tvalid(s_axis_ctrl_tvalid), .i_tready(s_axis_ctrl_tready),
        .o_tdata({m_rfnoc_ctrl_tlast, m_rfnoc_ctrl_tdata}),
        .o_tvalid(m_rfnoc_ctrl_tvalid), .o_tready(m_rfnoc_ctrl_tready),
        .space(), .occupied()
      );
    end else begin
      axi_fifo_2clk #(.WIDTH(32+1), .SIZE(SLAVE_FIFO_SIZE), .PIPELINE("NONE")) in_fifo_i (
        .reset(rfnoc_ctrl_rst),
        .i_aclk(rfnoc_ctrl_clk),
        .i_tdata({s_rfnoc_ctrl_tlast, s_rfnoc_ctrl_tdata}),
        .i_tvalid(s_rfnoc_ctrl_tvalid), .i_tready(s_rfnoc_ctrl_tready),
        .o_aclk(axis_ctrl_clk),
        .o_tdata({i_ctrl_tlast, i_ctrl_tdata}),
        .o_tvalid(i_ctrl_tvalid), .o_tready(i_ctrl_tready)
      );

      axi_fifo_2clk #(.WIDTH(32+1), .SIZE(1), .PIPELINE("NONE")) out_fifo_i (
        .reset(axis_ctrl_rst),
        .i_aclk(axis_ctrl_clk),
        .i_tdata({s_axis_ctrl_tlast, s_axis_ctrl_tdata}),
        .i_tvalid(s_axis_ctrl_tvalid), .i_tready(s_axis_ctrl_tready),
        .o_aclk(rfnoc_ctrl_clk),
        .o_tdata({m_rfnoc_ctrl_tlast, m_rfnoc_ctrl_tdata}),
        .o_tvalid(m_rfnoc_ctrl_tvalid), .o_tready(m_rfnoc_ctrl_tready)
      );
    end
  endgenerate

  axi_fifo #(.WIDTH(32+1), .SIZE(1)) slv_pipe_i (
    .clk(axis_ctrl_clk), .reset(axis_ctrl_rst), .clear(1'b0),
    .i_tdata({i_ctrl_tlast, i_ctrl_tdata}),
    .i_tvalid(i_ctrl_tvalid), .i_tready(i_ctrl_tready),
    .o_tdata({m_axis_ctrl_tlast, m_axis_ctrl_tdata}),
    .o_tvalid(m_axis_ctrl_tvalid), .o_tready(m_axis_ctrl_tready),
    .space(), .occupied()
  );

endmodule // axis_ctrl_endpoint

