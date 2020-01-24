//
// Copyright 2019 Ettus Research, A National Instruments Company
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
//
//   PORT_BASE   : Base addresses to use fore each slave. This is a 
//                 concatenation of 20-bit addresses, where the right-most 
//                 (least-significant) 20 bits corresponds to slave 0. Each 
//                 address must be a multiple of 2**PORT_ADDR_W, where 
//                 PORT_ADDR_W is the number of address bits allocated to that 
//                 slave.
//
//   PORT_ADDR_W : Number of address bits to allocate to each slave. This is a 
//                 concatenation of 32-bit integers, where the right-most 
//                 (least-significant) 32 bits corresponds to the address space 
//                 for slave 0.
//

module ctrlport_decoder_param #(
  parameter NUM_SLAVES  = 4,
  parameter PORT_BASE   = { 20'h300, 20'h200, 20'h100, 20'h000 },
  parameter PORT_ADDR_W = {   32'd8,   32'd8,   32'd8,   32'd8 }
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

  //---------------------------------------------------------------------------
  // Address Decode Logic
  //---------------------------------------------------------------------------
  //
  // Check if the upper bits of the request address match each slave. If the
  // address matches, set the corresponding dec_mask[] bit.
  //
  //---------------------------------------------------------------------------

  wire [NUM_SLAVES-1:0] dec_mask;  // Address decoder mask

  genvar i;

  for (i = 0; i < NUM_SLAVES; i = i+1) begin : gen_dec_mask
    localparam [19:0] BASE_ADDR = PORT_BASE  [i*20 +: 20];
    localparam [31:0] ADDR_W    = PORT_ADDR_W[i*32 +: 32];
    assign dec_mask[i] = ~|((s_ctrlport_req_addr ^ BASE_ADDR) & ((~0) << ADDR_W));
  end


  //---------------------------------------------------------------------------
  // Split the requests among the slaves
  //---------------------------------------------------------------------------

  for (i = 0; i < NUM_SLAVES; i = i+1) begin : gen_split
    localparam [31:0] ADDR_W = PORT_ADDR_W[i*32 +: 32];

    always @(posedge ctrlport_clk) begin
      if (ctrlport_rst) begin
        m_ctrlport_req_wr[i] <= 1'b0;
        m_ctrlport_req_rd[i] <= 1'b0;
      end else begin
        // Mask WR and RD based on address decoding
        m_ctrlport_req_wr[i] <= s_ctrlport_req_wr & dec_mask[i];
        m_ctrlport_req_rd[i] <= s_ctrlport_req_rd & dec_mask[i];

        // Other values pass through to all slaves, but should be ignored
        // unless WR or RD is asserted.
        m_ctrlport_req_data    [32*i +: 32] <= s_ctrlport_req_data;
        m_ctrlport_req_byte_en [4*i +: 4]   <= s_ctrlport_req_byte_en;
        m_ctrlport_req_has_time[i]          <= s_ctrlport_req_has_time;
        m_ctrlport_req_time    [64*i +: 64] <= s_ctrlport_req_time;

        // Mask the address bits to that of the slaves address space.
        m_ctrlport_req_addr[20*i +: 20]     <= 20'b0;
        m_ctrlport_req_addr[20*i +: ADDR_W] <= s_ctrlport_req_addr[ADDR_W-1 : 0];
      end
    end
  end


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
