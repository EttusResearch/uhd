//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Description:
//   This module is instantiated in parallel with a FIFO with AXI4-STREAM interfaces.
//   It tracks how many complete packets are contained within the FIFO, and also indicates
//   when the first word of a packet is presented on the FIFO outputs.

module axis_fifo_monitor #(
  parameter COUNT_W = 32
)(
  // Clocks and resets
  input  wire               clk,
  input  wire               reset,
  // FIFO Input      
  input  wire               i_tlast,
  input  wire               i_tvalid,
  input  wire               i_tready,
  // FIFO Output      
  input  wire               o_tlast,
  input  wire               o_tvalid,
  input  wire               o_tready,
  // FIFO Stats
  output wire               i_sop,
  output wire               i_eop,
  output wire               o_sop,
  output wire               o_eop,
  output wire [COUNT_W-1:0] occupied,
  output wire [COUNT_W-1:0] occupied_pkts
);

  wire [COUNT_W-1:0] i_pkt_count, o_pkt_count;
  wire [COUNT_W-1:0] i_xfer_count, o_xfer_count;

  axis_strm_monitor #(
    .WIDTH(1), .COUNT_W(COUNT_W),
    .PKT_LENGTH_EN(0), .PKT_CHKSUM_EN(0),
    .PKT_COUNT_EN(1), .XFER_COUNT_EN(1)
  ) input_monitor (
    .clk(clk), .reset(reset),
    .axis_tdata(1'b0), .axis_tlast(i_tlast), .axis_tvalid(i_tvalid), .axis_tready(i_tready),
    .sop(i_sop), .eop(i_eop),
    .pkt_length(), .pkt_chksum(),
    .pkt_count(i_pkt_count), .xfer_count(i_xfer_count)
  );

  axis_strm_monitor #(
    .WIDTH(1), .COUNT_W(COUNT_W),
    .PKT_LENGTH_EN(0), .PKT_CHKSUM_EN(0),
    .PKT_COUNT_EN(1), .XFER_COUNT_EN(1)
  ) output_monitor (
    .clk(clk), .reset(reset),
    .axis_tdata(1'b0), .axis_tlast(o_tlast), .axis_tvalid(o_tvalid), .axis_tready(o_tready),
    .sop(o_sop), .eop(o_eop),
    .pkt_length(), .pkt_chksum(),
    .pkt_count(o_pkt_count), .xfer_count(o_xfer_count)
  );

  // Count packets in FIFO.
  // No protection on counter wrap, 
  assign occupied      = (i_xfer_count - o_xfer_count);
  assign occupied_pkts = (i_pkt_count  - o_pkt_count);

 endmodule