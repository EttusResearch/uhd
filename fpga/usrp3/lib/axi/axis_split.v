//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  axis_split
//
// Description:
//
//   This module takes a single AXI-Stream input and duplicates it onto
//   multiple AXI-Stream outputs. This block correctly handles the somewhat
//   tricky flow-control logic so that the AXI-Stream handshake protocol is
//   honored at all top-level ports.
//
//   The internal buffering is finite, so if the data from any of the output
//   ports can't be consumed then the flow-control logic will cause the input
//   to stall (i.e., s_axis_tready will deassert).
//
// Parameters:
//
//   DATA_W    : The bit width of tdata for all ports.
//   NUM_PORTS : The number of output ports on which to duplicate the input.
//   INPUT_REG : Set to 1 to add an input register stage to break combinatorial
//               paths. Set to 0 to allow combinatorial input paths.
//


module axis_split #(
  parameter DATA_W     = 32,
  parameter NUM_PORTS  = 4,
  parameter INPUT_REG  = 0
) (
  input wire clk,
  input wire rst,

  // Input AXI-Stream
  input  wire [DATA_W-1:0] s_axis_tdata,
  input  wire              s_axis_tvalid,
  output wire              s_axis_tready,

  // Output AXI-Streams
  output wire [DATA_W*NUM_PORTS-1:0] m_axis_tdata,
  output wire [       NUM_PORTS-1:0] m_axis_tvalid,
  input  wire [       NUM_PORTS-1:0] m_axis_tready
);

  // Output of the input-register stage
  wire [DATA_W-1:0] reg_tdata;
  wire              reg_tvalid;
  wire              reg_tready;

  // Input to the Output FIFO stage
  wire [DATA_W*NUM_PORTS-1:0] fifo_tdata;
  wire [       NUM_PORTS-1:0] fifo_tvalid;
  wire [       NUM_PORTS-1:0] fifo_tready;

  // Indicates all output FIFOs are ready for a transfer
  wire all_fifo_tready;


  //---------------------------------------------------------------------------
  // Optional Input Register
  //---------------------------------------------------------------------------

  if (INPUT_REG) begin : gen_input_reg
    axi_fifo_flop2 #(
      .WIDTH (DATA_W)
    ) axi_fifo_flop2_i (
      .clk      (clk),
      .reset    (rst),
      .clear    (1'b0),
      .i_tdata  (s_axis_tdata),
      .i_tvalid (s_axis_tvalid),
      .i_tready (s_axis_tready),
      .o_tdata  (reg_tdata),
      .o_tvalid (reg_tvalid),
      .o_tready (reg_tready),
      .space    (),
      .occupied ()
    );
  end else begin : gen_no_input_reg
    assign reg_tdata     = s_axis_tdata;
    assign reg_tvalid    = s_axis_tvalid;
    assign s_axis_tready = reg_tready;
  end


  //---------------------------------------------------------------------------
  // Forking Logic
  //---------------------------------------------------------------------------

  assign all_fifo_tready = &fifo_tready;

  // Data transfer occurs when we have valid data on the input and all output
  // FIFOs are ready to accept data. Note that having tvalid depend on tready
  // is normally not allowed, but the FIFO has been chosen to tolerate this.
  assign reg_tready  = all_fifo_tready;
  assign fifo_tvalid = { NUM_PORTS {all_fifo_tready & reg_tvalid} };
  assign fifo_tdata  = { NUM_PORTS {reg_tdata} };


  //---------------------------------------------------------------------------
  // Output FIFOs
  //---------------------------------------------------------------------------

  genvar i;
  for (i = 0; i < NUM_PORTS; i = i + 1) begin : gen_ports

    // We use axi_fifo_short specifically because it can tolerate tvalid
    // de-asserting at any time. This is normally not allowed by AXI-Stream.
    axi_fifo_short #(
      .WIDTH (DATA_W)
    ) axi_fifo_short_i (
      .clk      (clk),
      .reset    (rst),
      .clear    (1'b0),
      .i_tdata  (fifo_tdata[i*DATA_W+:DATA_W]),
      .i_tvalid (fifo_tvalid[i]),
      .i_tready (fifo_tready[i]),
      .o_tdata  (m_axis_tdata[i*DATA_W+:DATA_W]),
      .o_tvalid (m_axis_tvalid[i]),
      .o_tready (m_axis_tready[i]),
      .space    (),
      .occupied ()
    );
  end

endmodule
