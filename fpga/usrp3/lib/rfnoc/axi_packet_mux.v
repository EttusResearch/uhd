//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Muxes and packetizes input AXI-streams. Assumes header on tuser. 

module axi_packet_mux #(
  parameter NUM_INPUTS = 1,
  parameter MUX_PRE_FIFO_SIZE = 0,  // Use 0 (most efficient) unless there is need to compensate for unbalanced input path latencies
  parameter MUX_POST_FIFO_SIZE = 0, // Generally leave at 0, similar effect as FIFO_SIZE
  parameter FIFO_SIZE = 5           // Size of FIFO in CHDR framer
)(
  input clk, input reset, input clear,
  input [NUM_INPUTS*64-1:0] i_tdata, input [NUM_INPUTS-1:0] i_tlast, input [NUM_INPUTS-1:0] i_tvalid, output [NUM_INPUTS-1:0] i_tready, input [NUM_INPUTS*128-1:0] i_tuser,
  output [63:0] o_tdata, output o_tlast, output o_tvalid, input o_tready
);

  wire [NUM_INPUTS*(64+128)-1:0] i_tdata_flat;
  genvar i;
  generate
    for (i = 0; i < NUM_INPUTS; i = i + 1) begin
      assign i_tdata_flat[(128+64)*(i+1)-1:(128+64)*i] = {i_tuser[128*(i+1)-1:128*i],i_tdata[64*(i+1)-1:64*i]};
    end
  endgenerate

  wire [63:0] int_tdata;
  wire [127:0] int_tuser;
  wire int_tlast, int_tvalid, int_tready;
  axi_mux #(.PRIO(0), .WIDTH(128+64), .PRE_FIFO_SIZE(MUX_PRE_FIFO_SIZE), .POST_FIFO_SIZE(MUX_POST_FIFO_SIZE), .SIZE(NUM_INPUTS)) axi_mux (
    .clk(clk), .reset(reset), .clear(1'b0),
    .i_tdata(i_tdata_flat), .i_tlast(i_tlast), .i_tvalid(i_tvalid), .i_tready(i_tready),
    .o_tdata({int_tuser, int_tdata}), .o_tlast(int_tlast), .o_tvalid(int_tvalid), .o_tready(int_tready));

  chdr_framer #(.SIZE(FIFO_SIZE), .WIDTH(64)) chdr_framer (
    .clk(clk), .reset(reset), .clear(1'b0),
    .i_tdata(int_tdata), .i_tuser(int_tuser), .i_tlast(int_tlast), .i_tvalid(int_tvalid), .i_tready(int_tready),
    .o_tdata(o_tdata), .o_tlast(o_tlast), .o_tvalid(o_tvalid), .o_tready(o_tready));

endmodule
