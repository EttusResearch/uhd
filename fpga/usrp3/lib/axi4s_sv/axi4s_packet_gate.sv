//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi4s_packet_gate
//
// Description:
//   System Verilog wrapper for axi_gate_packet that accepts
//   a AxiStreamIfc with slave_user/master_uer interface.
//
// Parameters:
//   TDATA        - store tData if 1
//   TUSER        - store tUser if 1
//   SIZE         - 2**SIZE words are stored
//   USE_AS_BUFF  - Allow the packet gate to be used as a buffer (uses more RAM)
//   MIN_PKT_SIZE - log2 of minimum valid packet size (rounded down, used to
//                  reduce addr fifo size)

module axi4s_packet_gate #(
  bit TDATA       = 1,
  bit TUSER       = 1,
  int SIZE        = 10,
  bit USE_AS_BUFF = 1,
  int MIN_PKT_SIZE= 1
) (
  input logic clear=1'b0,
  input logic error=1'b0,
  interface   i,  // AxiStreamIf or AxiStreamPacketIf
  interface   o   // AxiStreamIf or AxiStreamPacketIf
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
    assert (i.TLAST == 1) else
      $fatal(1, "i.TLAST not present");
    assert (o.TLAST == 1) else
      $fatal(1, "o.TLAST not present");
  end

  localparam WIDTH = i.DWIDTH + i.UWIDTH + i.KWIDTH;

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

  logic [WIDTH-1:0]  s0_data;
  logic [WIDTH-1:0] s1_data;
  always_comb s0_data = s0.pack(.INC_LAST(0));
  always_comb s1.unpack(s1_data,.INC_LAST(0));

  logic s0_ready, s1_valid, s1_last;
  always_comb s0.tready = s0_ready;
  always_comb s1.tvalid = s1_valid;
  always_comb s1.tlast  = s1_last;

  axi_packet_gate #(
    .WIDTH(WIDTH),.SIZE(SIZE), .USE_AS_BUFF(USE_AS_BUFF), .MIN_PKT_SIZE(MIN_PKT_SIZE)
  ) axi_packet_gate_i (
    .clk(s0.clk), .reset(s0.rst), .clear(clear),
    .i_terror(error),
    .i_tdata(s0_data),
    .i_tvalid(s0.tvalid),
    .i_tready(s0_ready),
    .i_tlast(s0.tlast),
    .o_tdata(s1_data),
    .o_tvalid(s1_valid),
    .o_tready(s1.tready),
    .o_tlast(s1_last)
  );

endmodule : axi4s_packet_gate
