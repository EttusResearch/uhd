//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_combiner
//
// Description:
//
// This block is an arbiter that merges control-port interfaces. This block is
// used when you have multiple control-port masters that need to access a
// single slave. For example, a NoC block with multiple submodules that each
// need to read and/or write registers outside of themselves.
//
// This module combines the control-port requests from multiple masters into a
// single request for one slave. Simultaneous requests are handled in the order
// specified by PRIORITY. The responding ACK is routed back to the requester.
//
// The module has been designed so that the latency through it is always the
// same when PRIORITY=1 and there is no contention, so that it can be used in
// applications where deterministic behavior is desired.
//
// Parameters:
//
//   NUM_MASTERS : The number of control-port masters to connect to a single
//                 control-port slave.
//   PRIORITY    : Use PRIORITY = 0 for round robin arbitration, PRIORITY = 1
//                 for priority arbitration (lowest number port serviced first).
//


module ctrlport_combiner
  import ctrlport_pkg::*;
#(
  int NUM_MASTERS = 2,
  bit PRIORITY    = 0
) (
  input logic ctrlport_clk,
  input logic ctrlport_rst,

  // Requests from multiple masters
  input  logic [                    NUM_MASTERS-1:0] s_ctrlport_req_wr,
  input  logic [                    NUM_MASTERS-1:0] s_ctrlport_req_rd,
  input  logic [    CTRLPORT_ADDR_W*NUM_MASTERS-1:0] s_ctrlport_req_addr,
  input  logic [  CTRLPORT_PORTID_W*NUM_MASTERS-1:0] s_ctrlport_req_portid,
  input  logic [CTRLPORT_REM_EPID_W*NUM_MASTERS-1:0] s_ctrlport_req_rem_epid,
  input  logic [  CTRLPORT_PORTID_W*NUM_MASTERS-1:0] s_ctrlport_req_rem_portid,
  input  logic [    CTRLPORT_DATA_W*NUM_MASTERS-1:0] s_ctrlport_req_data,
  input  logic [ CTRLPORT_BYTE_EN_W*NUM_MASTERS-1:0] s_ctrlport_req_byte_en,
  input  logic [                    NUM_MASTERS-1:0] s_ctrlport_req_has_time,
  input  logic [    CTRLPORT_TIME_W*NUM_MASTERS-1:0] s_ctrlport_req_time,
  // Responses to multiple masters
  output logic [                    NUM_MASTERS-1:0] s_ctrlport_resp_ack,
  output logic [     CTRLPORT_STS_W*NUM_MASTERS-1:0] s_ctrlport_resp_status,
  output logic [    CTRLPORT_DATA_W*NUM_MASTERS-1:0] s_ctrlport_resp_data,

  // Request to a single slave
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
  // Response from a single slave
  input  logic                           m_ctrlport_resp_ack,
  input  logic [     CTRLPORT_STS_W-1:0] m_ctrlport_resp_status,
  input  logic [    CTRLPORT_DATA_W-1:0] m_ctrlport_resp_data
);

  // Define interfaces
  ctrlport_if slave_ctrlport[NUM_MASTERS](.clk(ctrlport_clk), .rst(ctrlport_rst));
  ctrlport_if master_ctrlport(.clk(ctrlport_clk), .rst(ctrlport_rst));

  // Map existing ports to ctrlport_if
  for (genvar i=0; i<NUM_MASTERS; i++) begin : input_gen
    always_comb begin
      slave_ctrlport[i].req.wr = s_ctrlport_req_wr[i];
      slave_ctrlport[i].req.rd = s_ctrlport_req_rd[i];
      slave_ctrlport[i].req.addr = s_ctrlport_req_addr[CTRLPORT_ADDR_W*i +: CTRLPORT_ADDR_W];
      slave_ctrlport[i].req.port_id = s_ctrlport_req_portid[CTRLPORT_PORTID_W*i +: CTRLPORT_PORTID_W];
      slave_ctrlport[i].req.remote_epid = s_ctrlport_req_rem_epid[CTRLPORT_REM_EPID_W*i +: CTRLPORT_REM_EPID_W];
      slave_ctrlport[i].req.remote_portid = s_ctrlport_req_rem_portid[CTRLPORT_PORTID_W*i +: CTRLPORT_PORTID_W];
      slave_ctrlport[i].req.data = s_ctrlport_req_data[CTRLPORT_DATA_W*i +: CTRLPORT_DATA_W];
      slave_ctrlport[i].req.byte_en = s_ctrlport_req_byte_en[CTRLPORT_BYTE_EN_W*i +: CTRLPORT_BYTE_EN_W];
      slave_ctrlport[i].req.has_time = s_ctrlport_req_has_time[i];
      slave_ctrlport[i].req.timestamp = s_ctrlport_req_time[CTRLPORT_TIME_W*i +: CTRLPORT_TIME_W];

      s_ctrlport_resp_ack[i] = slave_ctrlport[i].resp.ack;
      s_ctrlport_resp_status[CTRLPORT_STS_W*i +: CTRLPORT_STS_W] = slave_ctrlport[i].resp.status;
      s_ctrlport_resp_data[CTRLPORT_DATA_W*i +: CTRLPORT_DATA_W] = slave_ctrlport[i].resp.data;
    end
  end

  // Instantiate ctrlport_if_combiner module
  ctrlport_if_combiner #(
    .NUM_MASTERS(NUM_MASTERS),
    .PRIORITY(PRIORITY)
  ) ctrlport_if_combiner_inst (
    .s_ctrlport(slave_ctrlport),
    .m_ctrlport(master_ctrlport)
  );

  // Unpack ctrlport_if to output port
  always_comb begin
    m_ctrlport_req_wr = master_ctrlport.req.wr;
    m_ctrlport_req_rd = master_ctrlport.req.rd;
    m_ctrlport_req_addr = master_ctrlport.req.addr;
    m_ctrlport_req_portid = master_ctrlport.req.port_id;
    m_ctrlport_req_rem_epid = master_ctrlport.req.remote_epid;
    m_ctrlport_req_rem_portid = master_ctrlport.req.remote_portid;
    m_ctrlport_req_data = master_ctrlport.req.data;
    m_ctrlport_req_byte_en = master_ctrlport.req.byte_en;
    m_ctrlport_req_has_time = master_ctrlport.req.has_time;
    m_ctrlport_req_time = master_ctrlport.req.timestamp;

    master_ctrlport.resp.ack = m_ctrlport_resp_ack;
    master_ctrlport.resp.status = ctrlport_status_t'(m_ctrlport_resp_status);
    master_ctrlport.resp.data = m_ctrlport_resp_data;
  end

endmodule
