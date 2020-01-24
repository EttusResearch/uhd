//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_resp_combine
//
// Description:
//
//   This module combines the control-port responses from multiple slave blocks 
//   into a single response for the master. This is done by using ack bit to 
//   mask all bits of the responses then ORing all the results together onto a 
//   single response bus. This is valid because only one block is allowed to 
//   respond to a single request.
//
//   Note that no special logic is required to split the requests from the 
//   master among multiple slaves. A single master request interface can be 
//   directly connected to all the slaves without issue.
//
// Parameters:
//
//   NUM_SLAVES : The number of slaves you want to connect to a master.
//


module ctrlport_resp_combine #(
  parameter NUM_SLAVES = 2
) (
  input wire ctrlport_clk,
  input wire ctrlport_rst,

  // Responses from multiple slaves
  input wire [   NUM_SLAVES-1:0] m_ctrlport_resp_ack,
  input wire [ 2*NUM_SLAVES-1:0] m_ctrlport_resp_status,
  input wire [32*NUM_SLAVES-1:0] m_ctrlport_resp_data,

  // Response to a single master
  output reg        s_ctrlport_resp_ack,
  output reg [ 1:0] s_ctrlport_resp_status,
  output reg [31:0] s_ctrlport_resp_data
);

  always @(posedge ctrlport_clk) begin
    if (ctrlport_rst) begin
      s_ctrlport_resp_data <= 0;
      s_ctrlport_resp_ack  <= 0;
    end else begin : or_reg_resp
      reg [31:0] data;
      reg [ 1:0] status;
      reg        ack;
      integer    s;

      // Take the responses and mask them with ack then OR them together
      data   = 0;
      status = 0;
      ack    = 0;
      for (s = 0; s < NUM_SLAVES; s = s+1) begin
        data   = data   | (m_ctrlport_resp_data  [s*32 +: 32] & {32{m_ctrlport_resp_ack[s]}});
        status = status | (m_ctrlport_resp_status[s* 2 +:  2] & { 2{m_ctrlport_resp_ack[s]}});
        ack    = ack    |  m_ctrlport_resp_ack[s];
      end

      // Register the output to break combinatorial path
      s_ctrlport_resp_data   <= data;
      s_ctrlport_resp_status <= status;
      s_ctrlport_resp_ack    <= ack;
    end
  end

endmodule
