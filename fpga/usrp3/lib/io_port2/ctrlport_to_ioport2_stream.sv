//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_to_ioport2_stream
//
// Description:
//
//   Bridges a CtrlPort slave interface to the ioport2 AXI-Stream message
//   interface.
//
//

module ctrlport_to_ioport2_stream (
  // CtrlPort slave interface
  ctrlport_if.slave   s_ctrlport,

  // ioport2 AXI-Stream request port (to ioport2 slave)
  output logic [63:0] regi_tdata,
  output logic        regi_tvalid,
  input  logic        regi_tready,

  // ioport2 AXI-Stream response port (from ioport2 slave)
  input  logic [63:0] rego_tdata,
  input  logic        rego_tvalid,
  output logic        rego_tready
);

  import ioport2_msg_pkg::*;

  // ioport2 message encoding (see ioport2_msg_codec.v):
  //   msg[63]    = rd_response = 0 (this is a request)
  //   msg[62]    = wr_request
  //   msg[61]    = rd_request
  //   msg[60]    = half_word = 0 (always 32-bit)
  //   msg[59:52] = reserved
  //   msg[51:32] = address
  //   msg[31:0]  = data
  ioport2_msg_t encoded_msg;
  assign encoded_msg = '{
    rd_response: 1'b0,
    wr_request : s_ctrlport.req.wr,
    rd_request : s_ctrlport.req.rd,
    half_word  : 1'b0,
    reserved   : 8'h00,
    address    : s_ctrlport.req.addr,
    data       : s_ctrlport.req.data
  };

  // Latch the incoming one-cycle ctrlport request pulse into a held valid
  // that drives regi_tvalid until the slave accepts it.
  logic        pending;
  ioport2_msg_t msg_r;
  logic        held_rd;

  logic accepted;
  assign held_rd  = msg_r.rd_request;
  assign accepted = regi_tvalid & regi_tready & (!held_rd || rego_tvalid);

  // Handle incoming requests and hold until accepted. Ignore any new request
  // pulses while pending is high so the held message is preserved until the
  // downstream side accepts it.
  always_ff @(posedge s_ctrlport.clk) begin
    if (s_ctrlport.rst) begin
      pending <= 1'b0;
      msg_r   <= '0;
    end else begin
      if (accepted) begin
        pending <= 1'b0;
      end else if (!pending && (s_ctrlport.req.wr || s_ctrlport.req.rd)) begin
        pending <= 1'b1;
        msg_r   <= encoded_msg;
      end
    end
  end

  assign regi_tdata  = msg_r;
  assign regi_tvalid = pending;

  // regi_tready is expected to be asserted combinatorially as soon as
  // regi_tvalid is seen. Reads also wait for rego_tvalid before the bridge
  // treats the transaction as accepted.
  assign rego_tready = 1'b1;

  // Ack fires the same cycle the slave consumes the message. Reads additionally
  // require rego_tvalid so resp.data is only returned with valid response data.
  // For writes: rego_tvalid=0; resp_data is a don't-care (ctrlport master
  //             ignores resp_data for writes).
  assign s_ctrlport.resp.ack    = accepted;
  assign s_ctrlport.resp.status = ctrlport_pkg::STS_OKAY;
  assign s_ctrlport.resp.data   = rego_tdata[31:0];

endmodule: ctrlport_to_ioport2_stream
