//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_window
//
// Description:
//
//   Copy requests from slave to master interface when s_ctrlport_req_addr is in
//   address range specified by BASE_ADDRESS and WINDOW_SIZE. The modules does
//   not use any registers and therefore does not need ctrlport_clk and
//   ctrlport_rst.
//
// Parameters:
//
//   BASE_ADDRESS: Base address of the memory window.
//   WINDOW_SIZE:  Size of the memory window.
//

module ctrlport_window
  import ctrlport_pkg::*;
#(
  int BASE_ADDRESS = 0,
  int WINDOW_SIZE  = 32
) (
  // Slave Interface
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

  // Master Interface
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

  // Define interfaces
  logic dummy_clk = '0;
  logic dummy_rst = '0;
  ctrlport_if s_ctrlport_if(.clk(dummy_clk), .rst(dummy_rst));
  ctrlport_if m_ctrlport_if(.clk(dummy_clk), .rst(dummy_rst));;

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
  ctrlport_if_window #(
    .BASE_ADDRESS(BASE_ADDRESS),
    .WINDOW_SIZE(WINDOW_SIZE)
  ) window_inst (
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
