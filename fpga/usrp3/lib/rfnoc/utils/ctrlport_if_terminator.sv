//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_if_terminator
//
// Description:
//
//   Returns an error for all ctrlport requests in given address range.
//
// Parameters:
//
//  BASE_ADDRESS : First address to generate response for.
//  WINDOW_SIZE  : Size of the address space.
//                 Last address in range will be BASE_ADDRESS + WINDOW_SIZE - 1.
//

module ctrlport_if_terminator #(
  int BASE_ADDRESS = 0,
  int WINDOW_SIZE  = 32
)(
  ctrlport_if.slave s_ctrlport
);

  import ctrlport_pkg::*;

  // drive acknowledgement on requests but not on reset
  always_ff @(posedge s_ctrlport.clk) begin
    if (s_ctrlport.rst) begin
      s_ctrlport.resp.ack <= 1'b0;
    end else if ((s_ctrlport.req.addr >= BASE_ADDRESS) &&
      (s_ctrlport.req.addr < BASE_ADDRESS + WINDOW_SIZE)) begin
      s_ctrlport.resp.ack <= s_ctrlport.req.wr | s_ctrlport.req.rd;
    end else begin
      s_ctrlport.resp.ack <= 1'b0;
    end
  end

  // other outputs are fixed
  assign s_ctrlport.resp.status = STS_CMDERR;
  assign s_ctrlport.resp.data = '0;

endmodule
