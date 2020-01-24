//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_terminator.v
// Description:
// Returns an error for all ctrlport requests in given address range.

module ctrlport_terminator #(
  parameter START_ADDRESS = 0, // first address to generate response for
  parameter LAST_ADDRESS = 32  // last address (including) to generate response for
)(
  //---------------------------------------------------------------
  // ControlPort slave
  //---------------------------------------------------------------
  input  wire        ctrlport_clk,
  input  wire        ctrlport_rst,
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,

  output  reg        s_ctrlport_resp_ack,
  output wire [ 1:0] s_ctrlport_resp_status,
  output wire [31:0] s_ctrlport_resp_data
);

`include "../core/ctrlport.vh"
//vhook_nowarn s_ctrlport_req_addr
//vhook_nowarn s_ctrlport_req_data

// drive acknowledgement on requests but not on reset
always @(posedge ctrlport_clk) begin
  if (ctrlport_clk) begin
    if (ctrlport_rst) begin
      s_ctrlport_resp_ack <= 1'b0;
    end else if ((s_ctrlport_req_addr >= START_ADDRESS) && (s_ctrlport_req_addr <= LAST_ADDRESS)) begin
      s_ctrlport_resp_ack <= s_ctrlport_req_wr | s_ctrlport_req_rd;
    end else begin
      s_ctrlport_resp_ack <= 1'b0;
    end
  end
end

// other outputs are fixed
assign s_ctrlport_resp_status = CTRL_STS_CMDERR;
assign s_ctrlport_resp_data = { CTRLPORT_DATA_W {1'b0}};

endmodule