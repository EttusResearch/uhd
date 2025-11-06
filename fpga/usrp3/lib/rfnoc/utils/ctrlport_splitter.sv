//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_splitter
//
// Description:
//
//   This block splits a single control port interface into multiple. It is used
//   when you have a single master that needs to access multiple slaves. For
//   example, a NoC block where the registers are implemented in multiple
//   submodules that must be read/written by a single NoC shell.
//
//   Note that this block does not do any address decoding, so the connected
//   slaves must use non-overlapping address spaces.
//
//   This module takes the request received by its single slave interface and
//   outputs it on all its master interfaces. In the opposite direction, it takes
//   the responses received by its multiple master interfaces and combines them
//   into a single response on its slave interface. This is done by using the ack
//   bit of each response to mask the other bits of the response, then OR'ing all
//   of the masked responses together onto a single response bus. This is valid
//   because only one block is allowed to respond to a single request.
//
// Parameters:
//
//   NUM_SLAVES : The number of slaves you want to connect to a master.
//


module ctrlport_splitter
  import ctrlport_pkg::*;
#(
  int NUM_SLAVES = 2
) (
  input logic ctrlport_clk,
  input logic ctrlport_rst,

  // Slave Interface
  input  logic                          s_ctrlport_req_wr,
  input  logic                          s_ctrlport_req_rd,
  input  logic [   CTRLPORT_ADDR_W-1:0] s_ctrlport_req_addr,
  input  logic [   CTRLPORT_DATA_W-1:0] s_ctrlport_req_data,
  input  logic [CTRLPORT_BYTE_EN_W-1:0] s_ctrlport_req_byte_en,
  input  logic                          s_ctrlport_req_has_time,
  input  logic [   CTRLPORT_TIME_W-1:0] s_ctrlport_req_time,
  output logic                          s_ctrlport_resp_ack,
  output logic [    CTRLPORT_STS_W-1:0] s_ctrlport_resp_status,
  output logic [   CTRLPORT_DATA_W-1:0] s_ctrlport_resp_data,

  // Master Interfaces
  output logic [                   NUM_SLAVES-1:0] m_ctrlport_req_wr,
  output logic [                   NUM_SLAVES-1:0] m_ctrlport_req_rd,
  output logic [   CTRLPORT_ADDR_W*NUM_SLAVES-1:0] m_ctrlport_req_addr,
  output logic [   CTRLPORT_DATA_W*NUM_SLAVES-1:0] m_ctrlport_req_data,
  output logic [CTRLPORT_BYTE_EN_W*NUM_SLAVES-1:0] m_ctrlport_req_byte_en,
  output logic [                   NUM_SLAVES-1:0] m_ctrlport_req_has_time,
  output logic [   CTRLPORT_TIME_W*NUM_SLAVES-1:0] m_ctrlport_req_time,
  input  logic [                   NUM_SLAVES-1:0] m_ctrlport_resp_ack,
  input  logic [    CTRLPORT_STS_W*NUM_SLAVES-1:0] m_ctrlport_resp_status,
  input  logic [   CTRLPORT_DATA_W*NUM_SLAVES-1:0] m_ctrlport_resp_data
);

  // Define interfaces
  ctrlport_if s_ctrlport_if(.clk(ctrlport_clk), .rst(ctrlport_rst));
  ctrlport_if m_ctrlport_if[NUM_SLAVES](.clk(ctrlport_clk), .rst(ctrlport_rst));

  // Map existing ports to ctrlport_if
  always_comb begin
    s_ctrlport_if.req = '0;
    s_ctrlport_if.req.wr = s_ctrlport_req_wr;
    s_ctrlport_if.req.rd = s_ctrlport_req_rd;
    s_ctrlport_if.req.addr = s_ctrlport_req_addr;
    s_ctrlport_if.req.data = s_ctrlport_req_data;
    s_ctrlport_if.req.byte_en = s_ctrlport_req_byte_en;
    s_ctrlport_if.req.has_time = s_ctrlport_req_has_time;
    s_ctrlport_if.req.timestamp = s_ctrlport_req_time;

    s_ctrlport_resp_ack = s_ctrlport_if.resp.ack;
    s_ctrlport_resp_status = s_ctrlport_if.resp.status;
    s_ctrlport_resp_data = s_ctrlport_if.resp.data;
  end

  // Instantiate ctrlport_if_splitter module
  ctrlport_if_splitter #(
    .NUM_SLAVES(NUM_SLAVES)
  ) splitter_inst (
    .s_ctrlport(s_ctrlport_if),
    .m_ctrlport(m_ctrlport_if)
  );

  // Unpack ctrlport_if to output port
  for (genvar i = 0; i < NUM_SLAVES; i++) begin : output_gen
    always_comb begin
      m_ctrlport_req_wr[i] = m_ctrlport_if[i].req.wr;
      m_ctrlport_req_rd[i] = m_ctrlport_if[i].req.rd;
      m_ctrlport_req_addr[i*CTRLPORT_ADDR_W +: CTRLPORT_ADDR_W] = m_ctrlport_if[i].req.addr;
      m_ctrlport_req_data[i*CTRLPORT_DATA_W +: CTRLPORT_DATA_W] = m_ctrlport_if[i].req.data;
      m_ctrlport_req_byte_en[i*CTRLPORT_BYTE_EN_W +: CTRLPORT_BYTE_EN_W] = m_ctrlport_if[i].req.byte_en;
      m_ctrlport_req_has_time[i] = m_ctrlport_if[i].req.has_time;
      m_ctrlport_req_time[i*CTRLPORT_TIME_W +: CTRLPORT_TIME_W] = m_ctrlport_if[i].req.timestamp;

      m_ctrlport_if[i].resp.ack = m_ctrlport_resp_ack[i];
      m_ctrlport_if[i].resp.status = ctrlport_status_t'(m_ctrlport_resp_status[i*CTRLPORT_STS_W +: CTRLPORT_STS_W]);
      m_ctrlport_if[i].resp.data = m_ctrlport_resp_data[i*CTRLPORT_DATA_W +: CTRLPORT_DATA_W];
    end
  end

endmodule
