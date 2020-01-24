//
// Copyright 2018-2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_raw_data_to_chdr
// Description:
//   A simple adapter for when CHDR data is requested as an
//   interface to user logic.
//
// Parameters:
//   - CHDR_W: Width of the input CHDR bus in bits
//
// Signals:
//   - s_axis_chdr_* : Input CHDR stream (AXI-Stream)
//   - m_axis_chdr_* : Output CHDR stream (AXI-Stream)
//   - flush_* : Signals for flush control and status
//

module chdr_to_chdr_data #(
  parameter CHDR_W = 256
)(
  // Clock, reset and settings
  input  wire              axis_chdr_clk,
  input  wire              axis_chdr_rst,
  // CHDR in (AXI-Stream)
  input  wire [CHDR_W-1:0] s_axis_chdr_tdata,
  input  wire              s_axis_chdr_tlast,
  input  wire              s_axis_chdr_tvalid,
  output wire              s_axis_chdr_tready,
  // CHDR in (AXI-Stream)
  output wire [CHDR_W-1:0] m_axis_chdr_tdata,
  output wire              m_axis_chdr_tlast,
  output wire              m_axis_chdr_tvalid,
  input  wire              m_axis_chdr_tready,
  // Flush signals
  input  wire              flush_en,
  input  wire [31:0]       flush_timeout,
  output wire              flush_active,
  output wire              flush_done
);

  axis_packet_flush #(
    .WIDTH(CHDR_W), .FLUSH_PARTIAL_PKTS(0), .TIMEOUT_W(32), .PIPELINE("OUT")
  ) chdr_flusher_i (
    .clk(axis_chdr_clk), .reset(axis_chdr_rst),
    .enable(flush_en), .timeout(flush_timeout),
    .flushing(flush_active), .done(flush_done),
    .s_axis_tdata(s_axis_chdr_tdata), .s_axis_tlast(s_axis_chdr_tlast),
    .s_axis_tvalid(s_axis_chdr_tvalid), .s_axis_tready(s_axis_chdr_tready),
    .m_axis_tdata(m_axis_chdr_tdata), .m_axis_tlast(m_axis_chdr_tlast),
    .m_axis_tvalid(m_axis_chdr_tvalid), .m_axis_tready(m_axis_chdr_tready)
  );

endmodule // chdr_to_chdr_data
