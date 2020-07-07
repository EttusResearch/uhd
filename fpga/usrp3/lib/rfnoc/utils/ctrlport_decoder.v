//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_decoder
//
// Description:
//
// This block splits a single control port interface into multiple. It is used
// when you have a single master that needs to access multiple slaves.  For
// example, a NoC block where the registers are implemented in multiple
// submodules that must be read/written by a single NoC shell.
//
// This version also implements address decoding. The request is passed to a
// slave only if the address falls within that slave's address space. Each
// slave is given an address space of 2**ADDR_W and the first slave starts at
// address BASE_ADDR. In other words, the request address is partitioned as
// shown below.
//
//   |---------------- 32-bit -----------------|
//   |    Base     | Port Num |  Slave Addr    |
//   |-----------------------------------------|
//
// When passed to the slave, the base address and port number bits are stripped
// from the request address and only the SLAVE_ADDR_W-bit address is passed
// through.
//
// Parameters:
//
//   NUM_SLAVES   : Number of slave devices that you want to connect to master.
//   BASE_ADDR    : Base address for slave 0. This should be a power-of-2
//                  multiple of the combined slave address spaces.
//   SLAVE_ADDR_W : Number of address bits to allocate to each slave.
//

module ctrlport_decoder #(
  parameter NUM_SLAVES   = 2,
  parameter BASE_ADDR    = 0,
  parameter SLAVE_ADDR_W = 8
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
  output reg  [   NUM_SLAVES-1:0] m_ctrlport_req_wr = 0,
  output reg  [   NUM_SLAVES-1:0] m_ctrlport_req_rd = 0,
  output reg  [20*NUM_SLAVES-1:0] m_ctrlport_req_addr = 0,
  output reg  [32*NUM_SLAVES-1:0] m_ctrlport_req_data,
  output reg  [ 4*NUM_SLAVES-1:0] m_ctrlport_req_byte_en,
  output reg  [   NUM_SLAVES-1:0] m_ctrlport_req_has_time,
  output reg  [64*NUM_SLAVES-1:0] m_ctrlport_req_time,
  input  wire [   NUM_SLAVES-1:0] m_ctrlport_resp_ack,
  input  wire [ 2*NUM_SLAVES-1:0] m_ctrlport_resp_status,
  input  wire [32*NUM_SLAVES-1:0] m_ctrlport_resp_data
);

  localparam        PORT_NUM_W      = $clog2(NUM_SLAVES);
  localparam        PORT_NUM_POS    = SLAVE_ADDR_W;
  localparam        BASE_ADDR_W     = 20 - (SLAVE_ADDR_W + PORT_NUM_W);
  localparam        BASE_ADDR_POS   = SLAVE_ADDR_W + PORT_NUM_W;
  localparam [19:0] BASE_ADDR_MASK  = { BASE_ADDR_W {1'b1}} << BASE_ADDR_POS;


  //---------------------------------------------------------------------------
  // Split the requests among the slaves
  //---------------------------------------------------------------------------

  wire [NUM_SLAVES-1:0] decoder;

  generate
    genvar i;
    for (i = 0; i < NUM_SLAVES; i = i+1) begin : gen_split
      // Check if the upper bits of the request address match each slave. If the
      // address matches, set the corresponding decoder[] bit.
      if (PORT_NUM_W == 0) begin
        // Only one port in this case, so there are no port number bits to check
        assign decoder[i] = ((s_ctrlport_req_addr & BASE_ADDR_MASK) == BASE_ADDR);
      end else begin
        assign decoder[i] = ((s_ctrlport_req_addr & BASE_ADDR_MASK) == BASE_ADDR) &&
                             (s_ctrlport_req_addr[PORT_NUM_POS +: PORT_NUM_W] == i);
      end

      always @(posedge ctrlport_clk) begin
        if (ctrlport_rst) begin
          m_ctrlport_req_wr[i] <= 1'b0;
          m_ctrlport_req_rd[i] <= 1'b0;
        end else begin
          // Mask WR and RD based on address decoding
          m_ctrlport_req_wr[i] <= s_ctrlport_req_wr & decoder[i];
          m_ctrlport_req_rd[i] <= s_ctrlport_req_rd & decoder[i];

          // Other values pass through to all slaves, but should be ignored
          // unless the corresponding WR or RD is not asserted.
          m_ctrlport_req_data    [32*i +: 32] <= s_ctrlport_req_data;
          m_ctrlport_req_byte_en [4*i +: 4]   <= s_ctrlport_req_byte_en;
          m_ctrlport_req_has_time[i]          <= s_ctrlport_req_has_time;
          m_ctrlport_req_time    [64*i +: 64] <= s_ctrlport_req_time;

          // Pass through only the relevant slave bits
          m_ctrlport_req_addr[20*i+:20]           <= 20'b0;
          m_ctrlport_req_addr[20*i+:SLAVE_ADDR_W] <= s_ctrlport_req_addr[SLAVE_ADDR_W-1:0];
        end
      end
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
