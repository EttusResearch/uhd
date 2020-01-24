//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  ctrlport_clk_cross
//
// Description:
//
//   Crosses a CTRL Port request and response between two clock domains.
//


module ctrlport_clk_cross (

  input wire rst, // Can be either clock domain, but must be glitch-free

  //---------------------------------------------------------------------------
  // Input Clock Domain (Slave Interface)
  //---------------------------------------------------------------------------

  input  wire        s_ctrlport_clk,
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
  output wire        s_ctrlport_resp_ack,
  output wire [ 1:0] s_ctrlport_resp_status,
  output wire [31:0] s_ctrlport_resp_data,

  //---------------------------------------------------------------------------
  // Output Clock Domain (Master Interface)
  //---------------------------------------------------------------------------

  input  wire        m_ctrlport_clk,
  output wire        m_ctrlport_req_wr,
  output wire        m_ctrlport_req_rd,
  output wire [19:0] m_ctrlport_req_addr,
  output wire [ 9:0] m_ctrlport_req_portid,
  output wire [15:0] m_ctrlport_req_rem_epid,
  output wire [ 9:0] m_ctrlport_req_rem_portid,
  output wire [31:0] m_ctrlport_req_data,
  output wire [ 3:0] m_ctrlport_req_byte_en,
  output wire        m_ctrlport_req_has_time,
  output wire [63:0] m_ctrlport_req_time,
  input  wire        m_ctrlport_resp_ack,
  input  wire [ 1:0] m_ctrlport_resp_status,
  input  wire [31:0] m_ctrlport_resp_data
);

  //---------------------------------------------------------------------------
  // Slave to Master Clock Crossing (Request)
  //---------------------------------------------------------------------------

  localparam REQ_W = 
    1  + // ctrlport_req_wr
    1  + // ctrlport_req_rd
    20 + // ctrlport_req_addr
    10 + // ctrlport_req_portid
    16 + // ctrlport_req_rem_epid
    10 + // ctrlport_req_rem_portid
    32 + // ctrlport_req_data
    4  + // ctrlport_req_byte_en
    1  + // ctrlport_req_has_time
    64;  // ctrlport_req_time

  wire [ REQ_W-1:0] s_req_flat;
  wire [ REQ_W-1:0] m_req_flat;
  wire              m_req_flat_valid;
  wire              m_ctrlport_req_wr_tmp;
  wire              m_ctrlport_req_rd_tmp;

  assign s_req_flat = {
    s_ctrlport_req_wr,
    s_ctrlport_req_rd,
    s_ctrlport_req_addr,
    s_ctrlport_req_portid,
    s_ctrlport_req_rem_epid,
    s_ctrlport_req_rem_portid,
    s_ctrlport_req_data,
    s_ctrlport_req_byte_en,
    s_ctrlport_req_has_time,
    s_ctrlport_req_time
  };

  axi_fifo_2clk #(
    .WIDTH (REQ_W),
    .SIZE  (3)
  ) req_fifo (
    .reset    (rst),
    .i_aclk   (s_ctrlport_clk),
    .i_tdata  (s_req_flat),
    .i_tvalid (s_ctrlport_req_wr | s_ctrlport_req_rd),
    .i_tready (),
    .o_aclk   (m_ctrlport_clk),
    .o_tdata  (m_req_flat),
    .o_tready (1'b1),
    .o_tvalid (m_req_flat_valid)
  );

  assign {
    m_ctrlport_req_wr_tmp,
    m_ctrlport_req_rd_tmp,
    m_ctrlport_req_addr,
    m_ctrlport_req_portid,
    m_ctrlport_req_rem_epid,
    m_ctrlport_req_rem_portid,
    m_ctrlport_req_data,
    m_ctrlport_req_byte_en,
    m_ctrlport_req_has_time,
    m_ctrlport_req_time
  } = m_req_flat;

  assign m_ctrlport_req_wr = m_ctrlport_req_wr_tmp & m_req_flat_valid;
  assign m_ctrlport_req_rd = m_ctrlport_req_rd_tmp & m_req_flat_valid;


  //---------------------------------------------------------------------------
  // Master to Slave Clock Crossing (Response)
  //---------------------------------------------------------------------------

  localparam RESP_W = 
    1  + // ctrlport_resp_ack,
    2  + // ctrlport_resp_status,
    32;  // ctrlport_resp_data

  wire [RESP_W-1:0] m_resp_flat;
  wire [RESP_W-1:0] s_resp_flat;
  wire              s_resp_flat_valid;
  wire              s_ctrlport_resp_ack_tmp;

  assign m_resp_flat = {
    m_ctrlport_resp_ack,
    m_ctrlport_resp_status,
    m_ctrlport_resp_data
  };

  axi_fifo_2clk #(
    .WIDTH (RESP_W),
    .SIZE  (3)
  ) resp_fifo (
    .reset    (rst),
    .i_aclk   (m_ctrlport_clk),
    .i_tdata  (m_resp_flat),
    .i_tvalid (m_ctrlport_resp_ack),
    .i_tready (),
    .o_aclk   (s_ctrlport_clk),
    .o_tdata  (s_resp_flat),
    .o_tready (1'b1),
    .o_tvalid (s_resp_flat_valid)
  );

  assign {
    s_ctrlport_resp_ack_tmp,
    s_ctrlport_resp_status,
    s_ctrlport_resp_data
  } = s_resp_flat;

  assign s_ctrlport_resp_ack = s_ctrlport_resp_ack_tmp & s_resp_flat_valid;

endmodule
