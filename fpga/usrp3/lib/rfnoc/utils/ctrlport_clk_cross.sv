//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_clk_cross
//
// Description:
//
//   Crosses a CTRL Port request and response between two clock domains.
//


module ctrlport_clk_cross
  import ctrlport_pkg::*;
(
  input logic rst, // Can be either clock domain, but must be glitch-free

  //---------------------------------------------------------------------------
  // Input Clock Domain (Slave Interface)
  //---------------------------------------------------------------------------

  input  logic                           s_ctrlport_clk,
  input  logic                           s_ctrlport_req_wr,
  input  logic                           s_ctrlport_req_rd,
  input  logic [    CTRLPORT_ADDR_W-1:0] s_ctrlport_req_addr,
  input  logic [  CTRLPORT_PORTID_W-1:0] s_ctrlport_req_portid,
  input  logic [CTRLPORT_REM_EPID_W-1:0] s_ctrlport_req_rem_epid,
  input  logic [  CTRLPORT_PORTID_W-1:0] s_ctrlport_req_rem_portid,
  input  logic [    CTRLPORT_DATA_W-1:0] s_ctrlport_req_data,
  input  logic [ CTRLPORT_BYTE_EN_W-1:0] s_ctrlport_req_byte_en,
  input  logic                           s_ctrlport_req_has_time,
  input  logic [    CTRLPORT_TIME_W-1:0] s_ctrlport_req_time,
  output logic                           s_ctrlport_resp_ack,
  output logic [     CTRLPORT_STS_W-1:0] s_ctrlport_resp_status,
  output logic [    CTRLPORT_DATA_W-1:0] s_ctrlport_resp_data,

  //---------------------------------------------------------------------------
  // Output Clock Domain (Master Interface)
  //---------------------------------------------------------------------------

  input  logic                           m_ctrlport_clk,
  output logic                           m_ctrlport_req_wr,
  output logic                           m_ctrlport_req_rd,
  output logic [    CTRLPORT_ADDR_W-1:0] m_ctrlport_req_addr,
  output logic [  CTRLPORT_PORTID_W-1:0] m_ctrlport_req_portid,
  output logic [CTRLPORT_REM_EPID_W-1:0] m_ctrlport_req_rem_epid,
  output logic [  CTRLPORT_PORTID_W-1:0] m_ctrlport_req_rem_portid,
  output logic [    CTRLPORT_DATA_W-1:0] m_ctrlport_req_data,
  output logic [ CTRLPORT_BYTE_EN_W-1:0] m_ctrlport_req_byte_en,
  output logic                           m_ctrlport_req_has_time,
  output logic [    CTRLPORT_TIME_W-1:0] m_ctrlport_req_time,
  input  logic                           m_ctrlport_resp_ack,
  input  logic [     CTRLPORT_STS_W-1:0] m_ctrlport_resp_status,
  input  logic [    CTRLPORT_DATA_W-1:0] m_ctrlport_resp_data
);

  import ctrlport_pkg::*;

  // Reset sync to both clock domains
  logic m_rst, s_rst;
  reset_sync slave_reset_sync_inst (
    .clk(s_ctrlport_clk), .reset_in(rst), .reset_out(s_rst)
  );
  reset_sync master_reset_sync_inst (
    .clk(m_ctrlport_clk), .reset_in(rst), .reset_out(m_rst)
  );

  // Define interfaces
  ctrlport_if s_ctrlport_if(.clk(s_ctrlport_clk), .rst(s_rst));
  ctrlport_if m_ctrlport_if(.clk(m_ctrlport_clk), .rst(m_rst));

  // Map existing ports to ctrlport_if
  always_comb begin
    s_ctrlport_if.req.wr = s_ctrlport_req_wr;
    s_ctrlport_if.req.rd = s_ctrlport_req_rd;
    s_ctrlport_if.req.addr = s_ctrlport_req_addr;
    s_ctrlport_if.req.port_id = s_ctrlport_req_portid;
    s_ctrlport_if.req.remote_epid = s_ctrlport_req_rem_epid;
    s_ctrlport_if.req.remote_portid = s_ctrlport_req_rem_portid;
    s_ctrlport_if.req.data = s_ctrlport_req_data;
    s_ctrlport_if.req.byte_en = s_ctrlport_req_byte_en;
    s_ctrlport_if.req.has_time = s_ctrlport_req_has_time;
    s_ctrlport_if.req.timestamp = s_ctrlport_req_time;

    s_ctrlport_resp_ack = s_ctrlport_if.resp.ack;
    s_ctrlport_resp_status = s_ctrlport_if.resp.status;
    s_ctrlport_resp_data = s_ctrlport_if.resp.data;
  end

  // Instantiate ctrlport_if_clk_cross module
  ctrlport_if_clk_cross clk_cross_inst (
    .s_ctrlport(s_ctrlport_if),
    .m_ctrlport(m_ctrlport_if)
  );

  // Unpack ctrlport_if to output port
  always_comb begin
    m_ctrlport_req_wr = m_ctrlport_if.req.wr;
    m_ctrlport_req_rd = m_ctrlport_if.req.rd;
    m_ctrlport_req_addr = m_ctrlport_if.req.addr;
    m_ctrlport_req_portid = m_ctrlport_if.req.port_id;
    m_ctrlport_req_rem_epid = m_ctrlport_if.req.remote_epid;
    m_ctrlport_req_rem_portid = m_ctrlport_if.req.remote_portid;
    m_ctrlport_req_data = m_ctrlport_if.req.data;
    m_ctrlport_req_byte_en = m_ctrlport_if.req.byte_en;
    m_ctrlport_req_has_time = m_ctrlport_if.req.has_time;
    m_ctrlport_req_time = m_ctrlport_if.req.timestamp;

    m_ctrlport_if.resp.ack = m_ctrlport_resp_ack;
    m_ctrlport_if.resp.status = ctrlport_status_t'(m_ctrlport_resp_status);
    m_ctrlport_if.resp.data = m_ctrlport_resp_data;
  end

endmodule
