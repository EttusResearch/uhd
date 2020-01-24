//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Description:
//   A gatekeeper module for data packets entering and leaving
//   the user logic in an RFNoC block. This module keeps track
//   of the packet count for software to detect activity and 
//   provides a mechanism to flush packets from software. Useful
//   to prevent slow-moving or misbehaving noc blocks from clogging
//   up the infrastructure.

module datapath_gatekeeper #(
  parameter WIDTH   = 64,
  parameter COUNT_W = 16
)(
  // Clocks and resets
  input  wire               clk,
  input  wire               reset,
  // Input data stream      
  input  wire [WIDTH-1:0]   s_axis_tdata,
  input  wire               s_axis_tlast,
  input  wire               s_axis_tvalid,
  output wire               s_axis_tready,
  // Output data stream     
  output wire [WIDTH-1:0]   m_axis_tdata,
  output wire               m_axis_tlast,
  output wire               m_axis_tvalid,
  input  wire               m_axis_tready,
  // Settings and Status
  input  wire               flush,          // Drop all packets coming into module
  output wire               flushing,       // Is the module still dropping packets?
  output wire [COUNT_W-1:0] pkt_count       // Input packet counter (includes drops)
);

  axis_strm_monitor #(
    .WIDTH(1), .COUNT_W(COUNT_W),
    .PKT_LENGTH_EN(0), .PKT_CHKSUM_EN(0),
    .PKT_COUNT_EN(1), .XFER_COUNT_EN(0)
  ) monitor_i (
    .clk(clk), .reset(reset),
    .axis_tdata(1'b0), .axis_tlast(s_axis_tlast),
    .axis_tvalid(s_axis_tvalid), .axis_tready(s_axis_tready),
    .sop(), .eop(),
    .pkt_length(), .pkt_chksum(),
    .pkt_count(pkt_count), .xfer_count()
  );

  axis_packet_flush #(
    .WIDTH(WIDTH), .FLUSH_PARTIAL_PKTS(0),
    .TIMEOUT_W(1), .PIPELINE("NONE")
  ) flusher_i (
    .clk(clk), .reset(reset),
    .enable(flush), .timeout(1'b0), .flushing(flushing), .done(),
    .s_axis_tdata(s_axis_tdata), .s_axis_tlast(s_axis_tlast),
    .s_axis_tvalid(s_axis_tvalid), .s_axis_tready(s_axis_tready),
    .m_axis_tdata(m_axis_tdata), .m_axis_tlast(m_axis_tlast),
    .m_axis_tvalid(m_axis_tvalid), .m_axis_tready(m_axis_tready)
  );

endmodule