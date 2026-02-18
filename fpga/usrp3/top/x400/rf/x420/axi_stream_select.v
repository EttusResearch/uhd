//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_stream_select
//
// Description:
//
//   This module selects between two AXI stream interfaces.
//   The mux_select signal determines which input stream is forwarded to the
//   output. Set mux_select to 0 to select s_axis_0, and set it to 1 to select
//   s_axis_1.
//   The port list naming is designed to be compatible with the Vivado block
//   diagram. The signals of one AXI bus are combined into a single port on the
//   block diagram. The clock and reset are getting associated with all AXI
//   ports.
//
// Parameters:
//
//   AXI_WIDTH : Width of the AXI stream data bus.
//

module axi_stream_select #(
  parameter AXI_WIDTH = 128
) (
  input wire aclk,
  input wire aresetn,

  input wire mux_select,

  input  wire [AXI_WIDTH-1:0] s_axis_tdata_0,
  input  wire s_axis_tvalid_0,
  output wire s_axis_tready_0,
  input  wire [AXI_WIDTH-1:0] s_axis_tdata_1,
  input  wire s_axis_tvalid_1,
  output wire s_axis_tready_1,

  output wire [AXI_WIDTH-1:0] m_axis_tdata,
  output wire m_axis_tvalid,
  input  wire m_axis_tready
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

  // the axi_mux_select module is used to select between the two input streams.
  axi_mux_select #(
    .WIDTH(AXI_WIDTH),
    .PRE_FIFO_SIZE(0), // no FIFO on the input
    .POST_FIFO_SIZE(1), // flop2 instance on the output
    .SWITCH_ON_LAST(1), // wait until tlast is asserted before switching
    .SIZE(2) // two input streams
  ) axi_mux_select_inst (
    .clk(aclk),
    .reset(reset),
    .clear(1'b0),
    .select(mux_select),
    .i_tdata({s_axis_tdata_1, s_axis_tdata_0}),
    .i_tlast({1'b1, 1'b1}), // there is no tlast signal in this design, so tie it high to allow switching at any time
    .i_tvalid({s_axis_tvalid_1, s_axis_tvalid_0}),
    .i_tready({s_axis_tready_1, s_axis_tready_0}),
    .o_tdata(m_axis_tdata),
    .o_tlast(), // tlast is not used in this design, so leave it unconnected
    .o_tvalid(m_axis_tvalid),
    .o_tready(m_axis_tready)
  );

endmodule
