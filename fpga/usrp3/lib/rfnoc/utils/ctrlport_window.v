//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_window
//
// Description:
//   Copy requests from slave to master interface when s_ctrlport_req_addr is in
//   address range specified by BASE_ADDRESS and WINDOW_SIZE. The modules does
//   not use any registers and therefore does not need ctrlport_clk and
//   ctrlport_rst.
//

`default_nettype none

module ctrlport_window #(
  parameter BASE_ADDRESS = 0,
  parameter WINDOW_SIZE  = 32
) (
  // Slave Interface
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

  // Master Interface
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

  // Mask write and read flag
  wire address_in_range = (s_ctrlport_req_addr >= BASE_ADDRESS) && (s_ctrlport_req_addr < BASE_ADDRESS + WINDOW_SIZE);
  assign m_ctrlport_req_wr = s_ctrlport_req_wr & address_in_range;
  assign m_ctrlport_req_rd = s_ctrlport_req_rd & address_in_range;

  // Forward all other signals untouched.
  assign m_ctrlport_req_addr       = s_ctrlport_req_addr;
  assign m_ctrlport_req_portid     = s_ctrlport_req_portid;
  assign m_ctrlport_req_rem_epid   = s_ctrlport_req_rem_epid;
  assign m_ctrlport_req_rem_portid = s_ctrlport_req_rem_portid;
  assign m_ctrlport_req_data       = s_ctrlport_req_data;
  assign m_ctrlport_req_byte_en    = s_ctrlport_req_byte_en;
  assign m_ctrlport_req_has_time   = s_ctrlport_req_has_time;
  assign m_ctrlport_req_time       = s_ctrlport_req_time;

  assign s_ctrlport_resp_ack    = m_ctrlport_resp_ack;
  assign s_ctrlport_resp_status = m_ctrlport_resp_status;
  assign s_ctrlport_resp_data   = m_ctrlport_resp_data;

endmodule

`default_nettype wire
