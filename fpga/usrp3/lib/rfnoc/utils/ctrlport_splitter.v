//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_splitter
//
// Description:
//
// This block splits a single control port interface into multiple. It is used
// when you have a single master that needs to access multiple slaves. For
// example, a NoC block where the registers are implemented in multiple
// submodules that must be read/written by a single NoC shell.
//
// Note that this block does not do any address decoding, so the connected
// slaves must use non-overlapping address spaces.
//
// This module takes the request received by its single slave interface and
// outputs it on all its master interfaces. In the opposite direction, it takes
// the responses received by its multiple master interfaces and combines them
// into a single response on its slave interface. This is done by using the ack
// bit of each response to mask the other bits of the response, then OR'ing all
// of the masked responses together onto a single response bus. This is valid
// because only one block is allowed to respond to a single request.
//
// Parameters:
//
//   NUM_SLAVES : The number of slaves you want to connect to a master.
//


module ctrlport_splitter #(
  parameter NUM_SLAVES = 2
) (
  input wire ctrlport_clk,
  input wire ctrlport_rst,

  // Slave Interface
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  input  wire [ 3:0] s_ctrlport_req_byte_en,
  input  wire        s_ctrlport_req_has_time,
  input  wire [63:0] s_ctrlport_req_time,
  output reg         s_ctrlport_resp_ack = 1'b0,
  output reg  [ 1:0] s_ctrlport_resp_status,
  output reg  [31:0] s_ctrlport_resp_data,

  // Master Interfaces
  output wire [   NUM_SLAVES-1:0] m_ctrlport_req_wr,
  output wire [   NUM_SLAVES-1:0] m_ctrlport_req_rd,
  output wire [20*NUM_SLAVES-1:0] m_ctrlport_req_addr,
  output wire [32*NUM_SLAVES-1:0] m_ctrlport_req_data,
  output wire [ 4*NUM_SLAVES-1:0] m_ctrlport_req_byte_en,
  output wire [   NUM_SLAVES-1:0] m_ctrlport_req_has_time,
  output wire [64*NUM_SLAVES-1:0] m_ctrlport_req_time,
  input  wire [   NUM_SLAVES-1:0] m_ctrlport_resp_ack,
  input  wire [ 2*NUM_SLAVES-1:0] m_ctrlport_resp_status,
  input  wire [32*NUM_SLAVES-1:0] m_ctrlport_resp_data
);

  //---------------------------------------------------------------------------
  // Split the requests among the slaves
  //---------------------------------------------------------------------------

  generate
    genvar i;
    for (i = 0; i < NUM_SLAVES; i = i+1) begin : gen_split
      // No special logic is required to split the requests from the master among
      // multiple slaves.
      assign m_ctrlport_req_wr[i]           = s_ctrlport_req_wr;
      assign m_ctrlport_req_rd[i]           = s_ctrlport_req_rd;
      assign m_ctrlport_req_addr[20*i+:20]  = s_ctrlport_req_addr;
      assign m_ctrlport_req_data[32*i+:32]  = s_ctrlport_req_data;
      assign m_ctrlport_req_byte_en[4*i+:4] = s_ctrlport_req_byte_en;
      assign m_ctrlport_req_has_time[i]     = s_ctrlport_req_has_time;
      assign m_ctrlport_req_time[64*i+:64]  = s_ctrlport_req_time;
    end
  endgenerate

  //---------------------------------------------------------------------------
  // Decode the responses
  //---------------------------------------------------------------------------

  reg [31:0] data;
  reg [ 1:0] status;
  reg        ack = 0;

  // Take the responses and mask them with ack, then OR them together
  always @(*) begin : comb_decode
    integer s;
    data   = 0;
    status = 0;
    ack    = 0;
    for (s = 0; s < NUM_SLAVES; s = s+1) begin
      data   = data   | (m_ctrlport_resp_data  [s*32 +: 32] & {32{m_ctrlport_resp_ack[s]}});
      status = status | (m_ctrlport_resp_status[s* 2 +:  2] & { 2{m_ctrlport_resp_ack[s]}});
      ack    = ack    | m_ctrlport_resp_ack[s];
    end
  end

  // Register the output to break combinatorial path
  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      s_ctrlport_resp_ack  <= 0;
    end else begin
      s_ctrlport_resp_data   <= data;
      s_ctrlport_resp_status <= status;
      s_ctrlport_resp_ack    <= ack;
    end
  end

endmodule
