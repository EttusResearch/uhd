//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_monitor
//
// Description:
//
//   This module monitors an AXI-Stream CHDR bus and outputs some signals
//   useful for debugging.
//
// Parameters:
//
//   CHDR_W  : The CHDR width, which is also the width of i_tdata.
//   COUNT_W : The width in bits of the counter outputs.
//
// Signals:
//
//   i_t*        : The AXI-Stream bus to monitor.
//   xfer        : Transfer indicator. This output will be high in the clock
//                 cycles during which a transfer occurs on the bus (i.e., when
//                 i_tvalid and i_tready are both asserted).
//   sop         : Start-of-packet indicator. Asserts high for one clock cycle
//                 during the first transfer of the packet.
//   eop         : End-of-packet indicator. Asserts high for one clock cycle
//                 during the last transfer of the packet.
//   sob         : Start-of-burst indicator. Pulses high for one clock cycle
//                 during the first transfer of the CHDR burst.
//   eob         : End-of-burst indicator. Pulses high for one clock cycle
//                 during the last transfer of the CHDR burst.
//   xfer_count  : Counts the number of transfers that have occurred.
//   pkt_count   : Counts the number of packets that have been transferred.
//   burst_count : Counts the number of bursts that have been transferred.
//   timestamp   : The timestamp for the current packet. Outputs X between
//                 packets, when the packet has no timestamp, and during the
//                 beginning of a CHDR-64 packet before the timestamp is known.
//   chdr_header : The CHDR header for the current packet. Outputs X between
//                 packets.
//


`default_nettype none


module chdr_monitor
  import rfnoc_chdr_utils_pkg::*;
#(
  parameter int CHDR_W  = 64,
  parameter int COUNT_W = 32
) (
  input  wire                              clk,
  input  wire                              rst,

  input  wire       [          CHDR_W-1:0] i_tdata,
  input  wire                              i_tlast,
  input  wire                              i_tvalid,
  input  wire                              i_tready,

  output      logic                        xfer,
  output      logic                        sop,
  output      logic                        eop,
  output      logic                        sob,
  output      logic                        eob,
  output      logic [         COUNT_W-1:0] xfer_count,
  output      logic [         COUNT_W-1:0] pkt_count,
  output      logic [         COUNT_W-1:0] burst_count,
  output      logic [CHDR_TIMESTAMP_W-1:0] timestamp,
  output      chdr_header_t                chdr_header
);

  logic pkt_is_sob   = 1'b1;
  logic pkt_is_eob   = 1'b0;
  logic pkt_is_timed = 1'b0;

  chdr_header_t header_reg = 'X;

  logic sop_done = 1'b0;
  logic second_xfer;

  logic has_time;

  logic [CHDR_TIMESTAMP_W-1:0] timestamp_reg = 'X;

  axis_monitor #(
    .COUNT_W(COUNT_W)
  ) axis_monitor_i (
    .clk       (clk       ),
    .rst       (rst       ),
    .i_tlast   (i_tlast   ),
    .i_tvalid  (i_tvalid  ),
    .i_tready  (i_tready  ),
    .xfer      (xfer      ),
    .sop       (sop       ),
    .eop       (eop       ),
    .xfer_count(xfer_count),
    .pkt_count (pkt_count )
  );

  assign sob = sop && pkt_is_sob;
  assign eob = xfer && eop && (                  // Last transfer of packet
               pkt_is_eob ||                     // Multi-transfer packet with EOB set
               (sop && chdr_get_eob(i_tdata)));  // Single-transfer packet with EOB set

  assign second_xfer = xfer && sop_done;

  assign has_time = chdr_get_pkt_type(i_tdata) == CHDR_PKT_TYPE_DATA_TS;

  assign chdr_header = sop ? i_tdata[0+:CHDR_HEADER_W] : header_reg;

  // Always output the timestamp for the current packet. Output X if we're
  // between packets, the packet doesn't have a timestamp, or if there's a
  // timestamp, but we don't know what it is yet.
  always_comb begin
    if (CHDR_W == 64) begin
      if (sop && has_time) begin
        // It has a timestamp, but we don't know what it is yet
        timestamp = 'X;
      end else if (second_xfer && pkt_is_timed) begin
        timestamp = i_tdata[0+:CHDR_TIMESTAMP_W];
      end else begin
        // Output the timestamp that was latched at the start of the packet
        timestamp = timestamp_reg;
      end
    end else begin
      if (sop && has_time) begin
        // It has a timestamp, so output that
        timestamp = i_tdata[CHDR_HEADER_W+:CHDR_TIMESTAMP_W];
      end else begin
        // Output the timestamp that was latched at the start of the packet
        timestamp = timestamp_reg;
      end
    end
  end

  always_ff @(posedge clk) begin
    if (xfer) begin
      sop_done <= 1'b0;
    end

    if (sop) begin
      pkt_is_eob   <= chdr_get_eob(i_tdata);
      header_reg   <= i_tdata[0+:CHDR_HEADER_W];
      pkt_is_sob   <= 1'b0;
      pkt_is_timed <= has_time;
      if (CHDR_W == 64) begin
        if (has_time) timestamp_reg <= 'X;
        else timestamp_reg <= 'X;
      end else begin
        if (has_time) begin
          timestamp_reg <= i_tdata[CHDR_HEADER_W+:CHDR_TIMESTAMP_W];
        end else begin
          timestamp_reg <= 'X;
        end
      end
      sop_done <= 1'b1;
    end

    if (CHDR_W == 64 && second_xfer && pkt_is_timed) begin
      timestamp_reg <= i_tdata[0+:CHDR_TIMESTAMP_W];
    end

    if (eop) begin
      timestamp_reg <= 'X;
      header_reg    <= 'X;
    end

    if (eob) begin
      burst_count   <= burst_count + 1;
      pkt_is_sob    <= 1'b1;
      sop_done      <= 1'b0;
    end

    if (rst) begin
      burst_count   <= '0;
      timestamp_reg <= 'X;
      sop_done      <= 1'b0;
      pkt_is_sob    <= 1'b1;
      pkt_is_eob    <= 1'b0;
      pkt_is_timed  <= 1'b0;
      header_reg    <= 'X;
    end
  end

endmodule


`default_nettype wire
