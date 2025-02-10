//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_if_clk_cross
//
// Description:
//
//   Crosses control-port interface between different clock domains.
//


module ctrlport_if_clk_cross
(
  ctrlport_if.slave s_ctrlport,
  ctrlport_if.master m_ctrlport

);
  import ctrlport_pkg::*;

  //---------------------------------------------------------------------------
  // Slave to Master Clock Crossing (Request)
  //---------------------------------------------------------------------------
  ctrlport_request_t m_req_hs;
  logic              m_req_hs_valid;

  // Busy flag can be ignored as the response handshake takes at least the same
  // amount of cycles to transfer the response as this handshake instance needs
  // to release the busy flag as they are configured with the same amount of
  // synchronization stages. Furthermore the ctrlport protocol just allows for
  // one transaction to be active at the same time. A request can only be issued
  // once the response is provided.
  handshake #(
    .WIDTH($bits(ctrlport_request_t))
  ) req_handshake_inst (
    .clk_a(s_ctrlport.clk),
    .rst_a(s_ctrlport.rst),
    .valid_a((s_ctrlport.req.rd | s_ctrlport.req.wr) & ~s_ctrlport.rst),
    .data_a(s_ctrlport.req),
    .busy_a(),
    .clk_b(m_ctrlport.clk),
    .valid_b(m_req_hs_valid),
    .data_b(m_req_hs)
  );

  always_comb begin
    m_ctrlport.req = m_req_hs;
    // mask read and write flags
    m_ctrlport.req.wr = m_req_hs.wr & m_req_hs_valid & ~m_ctrlport.rst;
    m_ctrlport.req.rd = m_req_hs.rd & m_req_hs_valid & ~m_ctrlport.rst;
  end

  //---------------------------------------------------------------------------
  // Master to Slave Clock Crossing (Response)
  //---------------------------------------------------------------------------
  ctrlport_response_t s_resp_hs;
  logic               s_resp_hs_valid;

  // Busy flag can be ignored as the request handshake takes at least the same
  // amount of cycles to transfer the request as this handshake instance needs
  // to release the busy flag as they are configured with the same amount of
  // synchronization stages. Furthermore the ctrlport protocol just allows for
  // one transaction to be active at the same time. A response can only be
  // issued once the request is available.
  handshake #(
    .WIDTH($bits(ctrlport_response_t))
  ) resp_handshake_inst (
    .clk_a(m_ctrlport.clk),
    .rst_a(m_ctrlport.rst),
    .valid_a(m_ctrlport.resp.ack & ~m_ctrlport.rst),
    .data_a(m_ctrlport.resp),
    .busy_a(),
    .clk_b(s_ctrlport.clk),
    .valid_b(s_resp_hs_valid),
    .data_b(s_resp_hs)
  );

  always_comb begin
    s_ctrlport.resp = s_resp_hs;
    s_ctrlport.resp.ack = s_resp_hs.ack & s_resp_hs_valid & ~s_ctrlport.rst;
  end

endmodule
