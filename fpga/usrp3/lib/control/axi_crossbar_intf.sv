//
// Copyright 2015 Ettus Research LLC
// Copyright 2017 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Wraps AXI crossbar and exposes cvita_stream_t and settings_bus_t interfaces 

`include "sim_cvita_lib.svh"
`include "sim_set_rb_lib.svh"

module axi_crossbar_intf
#(
  parameter BASE        = 0,  // settings bus base address
  parameter FIFO_WIDTH  = 64, // AXI4-STREAM data bus width
  parameter DST_WIDTH   = 16, // Width of DST field we are routing on.
  parameter NUM_PORTS   = 2   // number of cvita busses
)(
  input clk,
  input reset,
  input clear,
  input [7:0] local_addr,
  axis_t.slave s_cvita[0:NUM_PORTS-1],
  axis_t.master m_cvita[0:NUM_PORTS-1],
  settings_bus_t.slave  set_bus,
  readback_bus_t.master rb_bus
);

  wire [NUM_PORTS*64-1:0]  flat_i_tdata;
  wire [NUM_PORTS-1:0]     i_tlast, i_tvalid, i_tready;
  wire [NUM_PORTS*64-1:0]  flat_o_tdata;
  wire [NUM_PORTS-1:0]     o_tlast, o_tvalid, o_tready;

  // Flattern CE tdata arrays
  genvar i;
  generate
    for (i = 0; i < NUM_PORTS; i = i + 1) begin
      assign flat_i_tdata[i*FIFO_WIDTH+FIFO_WIDTH-1:i*FIFO_WIDTH] = s_cvita[i].tdata;
      assign i_tlast[i] = s_cvita[i].tlast;
      assign i_tvalid[i] = s_cvita[i].tvalid;
      assign s_cvita[i].tready = i_tready[i];
    end
    for (i = 0; i < NUM_PORTS; i = i + 1) begin
      assign m_cvita[i].tdata = flat_o_tdata[i*FIFO_WIDTH+FIFO_WIDTH-1:i*FIFO_WIDTH];
      assign m_cvita[i].tlast = o_tlast[i];
      assign m_cvita[i].tvalid = o_tvalid[i];
      assign o_tready[i] = m_cvita[i].tready;
    end
  endgenerate

  wire set_stb = set_bus.stb;
  wire [15:0] set_addr = set_bus.addr;
  wire [31:0] set_data = set_bus.data;
  wire rb_rd_stb = rb_bus.stb;
  wire [2*$clog2(NUM_PORTS):0] rb_addr = rb_bus.addr[2*$clog2(NUM_PORTS):0];
  wire [31:0] rb_data;
  assign rb_bus.data = rb_data;

  axi_crossbar #(
    .BASE(BASE),
    .FIFO_WIDTH(FIFO_WIDTH),
    .DST_WIDTH(DST_WIDTH),
    .NUM_INPUTS(NUM_PORTS),
    .NUM_OUTPUTS(NUM_PORTS))
  inst_axi_crossbar (
    .clk(clk),
    .reset(reset),
    .clear(clear),
    .local_addr(local_addr),
    .i_tdata(flat_i_tdata),
    .i_tvalid(i_tvalid),
    .i_tlast(i_tlast),
    .i_tready(i_tready),
    .pkt_present(i_tvalid),
    .set_stb(set_stb),
    .set_addr(set_addr),
    .set_data(set_data),
    .o_tdata(flat_o_tdata),
    .o_tvalid(o_tvalid),
    .o_tlast(o_tlast),
    .o_tready(o_tready),
    .rb_rd_stb(rb_rd_stb),
    .rb_addr(rb_addr),
    .rb_data(rb_data));

endmodule

