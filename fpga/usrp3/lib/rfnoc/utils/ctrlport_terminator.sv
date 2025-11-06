//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_terminator.v
//
// Description:
//
//  Returns an error for all ctrlport requests in given address range.
//
// Parameters:
//
//   START_ADDRESS: First address to generate response for
//   LAST_ADDRESS:  Last address (including) to generate response for
//

module ctrlport_terminator
  import ctrlport_pkg::*;
#(
  int START_ADDRESS = 0,
  int LAST_ADDRESS  = 32
)(
  //---------------------------------------------------------------
  // ControlPort slave
  //---------------------------------------------------------------
  input  logic ctrlport_clk,
  input  logic ctrlport_rst,

  input  logic                       s_ctrlport_req_wr,
  input  logic                       s_ctrlport_req_rd,
  input  logic [CTRLPORT_ADDR_W-1:0] s_ctrlport_req_addr,
  input  logic [CTRLPORT_DATA_W-1:0] s_ctrlport_req_data,
  output logic                       s_ctrlport_resp_ack,
  output logic [ CTRLPORT_STS_W-1:0] s_ctrlport_resp_status,
  output logic [CTRLPORT_DATA_W-1:0] s_ctrlport_resp_data
);

// Define interfaces
  ctrlport_if s_ctrlport_if(.clk(ctrlport_clk), .rst(ctrlport_rst));

  // Map existing ports to ctrlport_if
  always_comb begin
    s_ctrlport_if.req.wr = s_ctrlport_req_wr;
    s_ctrlport_if.req.rd = s_ctrlport_req_rd;
    s_ctrlport_if.req.addr = s_ctrlport_req_addr;
    s_ctrlport_if.req.data = s_ctrlport_req_data;

    s_ctrlport_resp_ack = s_ctrlport_if.resp.ack;
    s_ctrlport_resp_status = s_ctrlport_if.resp.status;
    s_ctrlport_resp_data = s_ctrlport_if.resp.data;
  end

  // Instantiate ctrlport_if_clk_cross module
  ctrlport_if_terminator #(
    .BASE_ADDRESS(START_ADDRESS),
    .WINDOW_SIZE(LAST_ADDRESS - START_ADDRESS + 1)
  ) terminator_inst (
    .s_ctrlport(s_ctrlport_if)
  );

endmodule
