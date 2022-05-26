//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi4s_fifo
//
// Description: System Verilog wrapper for axi_fifo that accepts an
// AxiStreamIfc with slave_user/master_uer interface.
//
// Parameters:
//   SIZE  - 2**SIZE words are stored

module axi4s_fifo #(
  int SIZE  = 1 // default size set to one to act as a pipe phase
 ) (
  // Clock domain: i.clk (o.clk is unused)
  input logic clear=1'b0,
  interface.slave  i,  // AxiStreamIf or AxiStreamPacketIf
  interface.master o,  // AxiStreamIf or AxiStreamPacketIf
  output logic [15:0] space,
  output logic [15:0] occupied
);

  `include "axi4s.vh"

  // Parameter Checks
  initial begin
    assert (i.DATA_WIDTH == o.DATA_WIDTH) else
      $fatal(1, "DATA_WIDTH mismatch");
    assert (i.USER_WIDTH == o.USER_WIDTH) else
      $fatal(1, "USER_WIDTH mismatch");
    assert (i.TDATA == o.TDATA) else
      $fatal(1, "TDATA present mismatch");
    assert (i.TUSER == o.TUSER) else
      $fatal(1, "TUSER present mismatch");
    assert (i.TKEEP == o.TKEEP) else
      $fatal(1, "TKEEP present mismatch");
    assert (i.TLAST == o.TLAST) else
      $fatal(1, "TLAST present mismatch");
  end

  AxiStreamPacketIf #(.DATA_WIDTH(i.DATA_WIDTH),.USER_WIDTH(i.USER_WIDTH),
    .TDATA(i.TDATA),.TKEEP(i.TKEEP),.TUSER(i.TUSER),.TLAST(i.TLAST),
    .MAX_PACKET_BYTES(i.MAX_PACKET_BYTES))
    s0(i.clk,i.rst);
  AxiStreamPacketIf #(.DATA_WIDTH(i.DATA_WIDTH),.USER_WIDTH(i.USER_WIDTH),
    .TDATA(i.TDATA),.TKEEP(i.TKEEP),.TUSER(i.TUSER),.TLAST(i.TLAST),
    .MAX_PACKET_BYTES(i.MAX_PACKET_BYTES))
    s1(i.clk,i.rst);

  // move from AxiStreamIfc to AxiStreamPacketIf
  always_comb begin
    `AXI4S_ASSIGN(s0,i)
  end
  // move from AxiStreamPacketIf to AxiStreamIfc
  always_comb begin
    `AXI4S_ASSIGN(o,s1)
  end

  logic [s0.PACKED_WIDTH-1:0] s0_data;
  logic [s1.PACKED_WIDTH-1:0] s1_data;
  always_comb s0_data = s0.pack();
  always_comb s1.unpack(s1_data);

  logic s0_ready, s1_valid;
  always_comb s0.tready = s0_ready;
  always_comb s1.tvalid = s1_valid;

  axi_fifo #(
    .WIDTH(s0.PACKED_WIDTH),.SIZE(SIZE)
  ) axi_fifo_i (
    .clk(s0.clk), .reset(s0.rst), .clear(clear),
    .i_tdata(s0_data),
    .i_tvalid(s0.tvalid),
    .i_tready(s0_ready),
    .o_tdata(s1_data),
    .o_tvalid(s1_valid),
    .o_tready(s1.tready),
    .space(space), .occupied(occupied)
  );

endmodule : axi4s_fifo
