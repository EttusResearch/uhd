//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_decoder_param
//
// Description:
//
//   This block splits a single control port interface into multiple. It is
//   used when you have a single master that needs to access multiple slaves.
//   For example, a NoC block where the registers are implemented in multiple
//   submodules that must be read/written by a single NoC shell.
//
//   This version also implements address decoding. The request is passed to a
//   slave only if the address falls within that slave's address space. Each
//   slave can have a unique base address and address space size. The address
//   space is broken up as follows.
//
//     PORT_BASE[0*20 +: 20] = Port 0 base address
//       │           ┐
//       │           ├── 2**PORT_ADDR_W[0*32 +: 32] bytes for slave 0
//       │           ┘
//       .
//       .
//     PORT_BASE[1*20 +: 20] = Port 1 base address
//       │           ┐
//       │           ├── 2**PORT_ADDR_W[1*32 +: 32] bytes for slave 1
//       │           ┘
//       .
//       .
//
//   When passed to the slave, the base address is stripped from the request
//   address so that only the PORT_ADDR_W-bit address is passed through.
//
// Parameters:
//
//   NUM_SLAVES  : The number of slaves to connect to a master.
//   PORT_BASE   : Base addresses to use fore each slave. This is a
//                 concatenation of 20-bit addresses, where the right-most
//                 (least-significant) 20 bits corresponds to slave 0. Each
//                 address must be a multiple of 2**PORT_ADDR_W, where
//                 PORT_ADDR_W is the number of address bits allocated to that
//                 slave.
//   PORT_ADDR_W : Number of address bits to allocate to each slave. This is a
//                 concatenation of 32-bit integers, where the right-most
//                 (least-significant) 32 bits corresponds to the address space
//                 for slave 0.
//

module ctrlport_decoder_param
  import ctrlport_pkg::*;
#(
  int NUM_SLAVES                                   = 4,
  bit [CTRLPORT_ADDR_W*NUM_SLAVES-1:0] PORT_BASE   = { 20'h300, 20'h200, 20'h100, 20'h000 },
  bit [             32*NUM_SLAVES-1:0] PORT_ADDR_W = {   32'd8,   32'd8,   32'd8,   32'd8 }
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

  import ctrlport_pkg::*;

  // calculate port base address and size
  typedef int int_array_t[NUM_SLAVES];

  function int_array_t calc_port_base();
    int_array_t base;
    for (int i = 0; i < NUM_SLAVES; i++) begin
      base[i] = PORT_BASE[i*20 +: 20];
    end
    return base;
  endfunction

  function int_array_t calc_port_size();
    int_array_t size;
    for (int i = 0; i < NUM_SLAVES; i++) begin
      size[i] = 2**PORT_ADDR_W[i*32 +: 32];
    end
    return size;
  endfunction

  localparam int PORT_BASE_INT [NUM_SLAVES] = calc_port_base();
  localparam int PORT_SIZE_INT [NUM_SLAVES] = calc_port_size();

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

  // Instantiate ctrlport_if_decoder module
  ctrlport_if_decoder #(
    .NUM_SLAVES(NUM_SLAVES),
    .PORT_BASE(PORT_BASE_INT),
    .PORT_SIZE(PORT_SIZE_INT)
  ) decoder_inst (
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
