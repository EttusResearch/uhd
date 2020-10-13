//
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Description:
//   A module to monitor a an AXI-Stream link and gather various
//   metric about packets and the stream in general

module axis_strm_monitor #(
  parameter WIDTH         = 64,
  parameter COUNT_W       = 32,
  parameter PKT_LENGTH_EN = 0,
  parameter PKT_CHKSUM_EN = 0,
  parameter PKT_COUNT_EN  = 0,
  parameter XFER_COUNT_EN = 0
)(
  // Clocks and resets
  input  wire               clk,
  input  wire               reset,
  // Stream to monitor      
  input  wire [WIDTH-1:0]   axis_tdata,
  input  wire               axis_tlast,
  input  wire               axis_tvalid,
  input  wire               axis_tready,
  // Packet Stats
  output wire               sop,
  output wire               eop,
  output reg  [15:0]        pkt_length = 16'd0,
  output wire [WIDTH-1:0]   pkt_chksum,
  // Stream Stats
  output reg  [COUNT_W-1:0] pkt_count  = {COUNT_W{1'b0}},
  output reg  [COUNT_W-1:0] xfer_count = {COUNT_W{1'b0}}
);

  //----------------------------
  // Packet specific
  //----------------------------

  reg pkt_head = 1'b1;
  wire xfer = axis_tvalid & axis_tready;

  assign sop = pkt_head & xfer;
  assign eop = xfer & axis_tlast;

  always @(posedge clk) begin
    if (reset) begin
      pkt_head <= 1'b1;
    end else begin
      if (pkt_head) begin
        if (xfer)
          pkt_head <= ~eop;
      end else begin
        if (eop)
          pkt_head <= 1'b0;
      end
    end
  end

  generate if (PKT_LENGTH_EN) begin
    // Count the number of lines (transfers) in a packet
    always @(posedge clk) begin
      if (reset | eop) begin
        pkt_length <= 16'd1;
      end else if (xfer) begin
        pkt_length <= pkt_length + 1'b1;
      end
    end
  end else begin
    // Default packet length is 0
    always @(*) pkt_length <= 16'd0;
  end endgenerate

  generate if (PKT_CHKSUM_EN) begin
    // Compute the XOR checksum of the lines in a packet
    reg [WIDTH-1:0] chksum_prev = {WIDTH{1'b0}};
    always @(posedge clk) begin
      if (reset) begin
        chksum_prev <= {WIDTH{1'b0}};
      end else if (xfer) begin
        chksum_prev <= pkt_chksum;
      end
    end
    assign pkt_chksum = chksum_prev ^ axis_tdata;
  end else begin
    // Default checksum is 0
    assign pkt_chksum = {WIDTH{1'b0}};
  end endgenerate

  //----------------------------
  // Stream specific
  //----------------------------

  always @(posedge clk) begin
    if (reset | (PKT_COUNT_EN == 0)) begin
      pkt_count <= {COUNT_W{1'b0}};
    end else if (eop) begin
      pkt_count <= pkt_count + 1'b1;
    end
  end

  always @(posedge clk) begin
    if (reset | (XFER_COUNT_EN == 0)) begin
      xfer_count <= {COUNT_W{1'b0}};
    end else if (xfer) begin
      xfer_count <= xfer_count + 1'b1;
    end
  end

 endmodule