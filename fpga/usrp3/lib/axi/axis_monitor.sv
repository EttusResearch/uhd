//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_monitor
//
// Description:
//
//   This module monitors an AXI-Stream bus and outputs some signals useful for
//   debugging.
//
// Parameters:
//
//   COUNT_W : The width in bits of the counter outputs.
//
// Signals:
//
//   i_t*       : The AXI-Stream bus to monitor. Note that i_tdata is not
//                needed, so there is no port for it.
//   xfer       : Transfer indicator. This output will be high in the clock
//                cycles during which a transfer occurs on the bus (i.e., when
//                i_tvalid and i_tready are both asserted).
//   sop        : Start-of-packet indicator. Asserts high for one clock cycle
//                during the first transfer of the packet.
//   eop        : End-of-packet indicator. Asserts high for one clock cycle
//                during the last transfer of the packet.
//   xfer_count : Counts the number of transfers that have occurred.
//   pkt_count  : Counts the number of packets that have been transferred.
//

`default_nettype none


module axis_monitor #(
  parameter int COUNT_W = 32
) (
  input  wire                clk,
  input  wire                rst,

  input  wire                i_tlast,
  input  wire                i_tvalid,
  input  wire                i_tready,

  output logic               xfer,
  output logic               sop,
  output logic               eop,
  output logic [COUNT_W-1:0] xfer_count = '0,
  output logic [COUNT_W-1:0] pkt_count  = '0
);
  logic sop_reg = 1'b1;

  assign xfer = i_tvalid && i_tready;
  assign sop  = xfer && sop_reg;
  assign eop  = xfer && i_tlast;

  always_ff @(posedge clk) begin
    if (xfer) begin
      sop_reg    <= i_tlast;
      xfer_count <= xfer_count + 1;
      if (i_tlast) begin
        pkt_count  <= pkt_count + 1;
        xfer_count <= '0;
      end
    end

    if (rst) begin
      sop_reg    <= 1'b1;
      pkt_count  <= '0;
      xfer_count <= '0;
    end
  end
endmodule


`default_nettype wire
