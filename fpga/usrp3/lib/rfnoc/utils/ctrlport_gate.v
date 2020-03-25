//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_gate
//
// Description:
//
// This block forwards or blocks a control-port interface request based on the
// enable input.
//

module ctrlport_gate (
  input wire ctrlport_clk,
  input wire ctrlport_rst,

  // Control interface
  input  wire enable,

  // Slave interface
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [ 9:0] s_ctrlport_req_portid,
  input  wire [15:0] s_ctrlport_req_rem_epid,
  input  wire [ 9:0] s_ctrlport_req_rem_portid,
  input  wire [31:0] s_ctrlport_req_data,
  input  wire [ 3:0] s_ctrlport_req_byte_en,
  input  wire        s_ctrlport_req_has_time,
  input  wire [63:0] s_ctrlport_req_time,
  output reg         s_ctrlport_resp_ack,
  output reg  [ 1:0] s_ctrlport_resp_status,
  output reg  [31:0] s_ctrlport_resp_data,

  // Master interface
  output reg         m_ctrlport_req_wr,
  output reg         m_ctrlport_req_rd,
  output reg  [19:0] m_ctrlport_req_addr,
  output reg  [ 9:0] m_ctrlport_req_portid,
  output reg  [15:0] m_ctrlport_req_rem_epid,
  output reg  [ 9:0] m_ctrlport_req_rem_portid,
  output reg  [31:0] m_ctrlport_req_data,
  output reg  [ 3:0] m_ctrlport_req_byte_en,
  output reg         m_ctrlport_req_has_time,
  output reg  [63:0] m_ctrlport_req_time,
  input  wire        m_ctrlport_resp_ack,
  input  wire [ 1:0] m_ctrlport_resp_status,
  input  wire [31:0] m_ctrlport_resp_data
);

  `include "../core/ctrlport.vh"

  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      m_ctrlport_req_wr   <= 1'b0;
      m_ctrlport_req_rd   <= 1'b0;
      s_ctrlport_resp_ack <= 1'b0;
    end else begin
      // Forward all signals by default
      m_ctrlport_req_wr         <= s_ctrlport_req_wr;
      m_ctrlport_req_rd         <= s_ctrlport_req_rd;
      m_ctrlport_req_addr       <= s_ctrlport_req_addr;
      m_ctrlport_req_portid     <= s_ctrlport_req_portid;
      m_ctrlport_req_rem_epid   <= s_ctrlport_req_rem_epid;
      m_ctrlport_req_rem_portid <= s_ctrlport_req_rem_portid;
      m_ctrlport_req_data       <= s_ctrlport_req_data;
      m_ctrlport_req_byte_en    <= s_ctrlport_req_byte_en;
      m_ctrlport_req_has_time   <= s_ctrlport_req_has_time;
      m_ctrlport_req_time       <= s_ctrlport_req_time;

      s_ctrlport_resp_ack    <= m_ctrlport_resp_ack;
      s_ctrlport_resp_status <= m_ctrlport_resp_status;
      s_ctrlport_resp_data   <= m_ctrlport_resp_data;

      // Overwrite default assignments in case of disabled interface
      if (m_ctrlport_req_rd || m_ctrlport_req_wr) begin
        if (~enable) begin
          // Block forwarding of request
          m_ctrlport_req_wr <= 1'b0;
          m_ctrlport_req_rd <= 1'b0;

          // Issue error as response
          s_ctrlport_resp_ack    <= 1'b1;
          s_ctrlport_resp_status <= CTRL_STS_CMDERR;
        end
      end
    end
  end

endmodule
