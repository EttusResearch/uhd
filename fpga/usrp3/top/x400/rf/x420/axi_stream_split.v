//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_stream_split
//
// Description:
//
//   This module splits a single AXI stream interface into two AXI stream
//   interfaces. The purpose of this module is to be used before the Xilinx RFDC
//   IP whose DAC AXI interfaces require a constant data stream.
//   The mux_select signal determines which output stream carries the input
//   data. The other stream will transfer zeros. AXI transfers are properly
//   handled between the input and the selected output port. The disabled output
//   port might violate the AXI protocol.
//   The port list naming is designed to be compatible with the Vivado block
//   diagram. The signals of one AXI bus are combined into a single port on the
//   block diagram. The clock and reset are getting associated with all AXI
//   ports.
//
// Parameters:
//
//   AXI_WIDTH : Width of the AXI stream data bus.
//

module axi_stream_split #(
  parameter AXI_WIDTH = 128
) (
  input wire aclk,
  input wire aresetn,

  input wire mux_select,

  input  wire [AXI_WIDTH-1:0] s_axis_tdata,
  input  wire s_axis_tvalid,
  output wire s_axis_tready,

  output wire [AXI_WIDTH-1:0] m_axis_tdata_0,
  output wire m_axis_tvalid_0,
  input  wire m_axis_tready_0,
  output wire [AXI_WIDTH-1:0] m_axis_tdata_1,
  output wire m_axis_tvalid_1,
  input  wire m_axis_tready_1
);
  // synchronize the reset
  wire resetn;
  wire reset;
  reset_sync reset_sync_inst (
    .clk(aclk),
    .reset_in(aresetn),
    .reset_out(resetn)
  );
  assign reset = ~resetn;

  // internal signals for register inputs
  wire [AXI_WIDTH-1:0] axis_tdata_0_int, axis_tdata_1_int;
  wire axis_tready_0_int, axis_tready_1_int;

  assign axis_tdata_0_int = ~mux_select ? s_axis_tdata : {AXI_WIDTH{1'b0}};
  assign axis_tdata_1_int =  mux_select ? s_axis_tdata : {AXI_WIDTH{1'b0}};
  assign s_axis_tready    = ~mux_select ? axis_tready_0_int : axis_tready_1_int;

  // break any combinatorial paths by adding a register in output paths
  axi_fifo_flop2 #(
    .WIDTH(AXI_WIDTH)
  ) axi_flop_inst0 (
    .clk(aclk),
    .reset(reset),
    .clear(1'b0),
    .i_tdata(axis_tdata_0_int),
    .i_tvalid(s_axis_tvalid),
    .i_tready(axis_tready_0_int),
    .o_tdata(m_axis_tdata_0),
    .o_tvalid(m_axis_tvalid_0),
    .o_tready(m_axis_tready_0),
    .space(),
    .occupied()
  );
  axi_fifo_flop2 #(
    .WIDTH(AXI_WIDTH)
  ) axi_flop_inst1 (
    .clk(aclk),
    .reset(reset),
    .clear(1'b0),
    .i_tdata(axis_tdata_1_int),
    .i_tvalid(s_axis_tvalid),
    .i_tready(axis_tready_1_int),
    .o_tdata(m_axis_tdata_1),
    .o_tvalid(m_axis_tvalid_1),
    .o_tready(m_axis_tready_1),
    .space(),
    .occupied()
  );

endmodule
