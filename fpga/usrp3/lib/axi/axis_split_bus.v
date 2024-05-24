//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  axis_split
//
// Description:
//
//   This module takes a single AXI-Stream input with width N and splits it
//   into M AXI-Stream outputs each with N/M bits from the input bus.
//
//   This block correctly handles the somewhat tricky flow-control logic so
//   that the AXI-Stream handshake protocol is honored at all top-level ports.
//
//   The internal buffering is finite, so if the data from any of the output
//   ports cannot be consumed then the flow-control logic will cause the input
//   to stall (i.e., s_axis_tready will deassert).
//
// Parameters:
//
//   WIDTH      : The bit width of tdata for all ports.
//   USER_WIDTH : The bit width of tuser for all ports.
//   NUM_PORTS  : The number of output ports on which to duplicate the input.
//   INPUT_REG  : Set to 1 to add an input register stage to break combinatorial
//                paths. Set to 0 to allow combinatorial input paths.
//

module axis_split_bus #(
  parameter WIDTH      = 32,
  parameter USER_WIDTH = 1,
  parameter NUM_PORTS  = 4,
  parameter INPUT_REG  = 0
) (
  input wire clk,
  input wire reset,

  // Input AXI-Stream
  input  wire [     WIDTH*NUM_PORTS-1:0] s_axis_tdata,
  input  wire [USER_WIDTH*NUM_PORTS-1:0] s_axis_tuser,
  input  wire                            s_axis_tlast,
  input  wire                            s_axis_tvalid,
  output wire                            s_axis_tready,

  // Output AXI-Streams
  output wire [     WIDTH*NUM_PORTS-1:0] m_axis_tdata,
  output wire [USER_WIDTH*NUM_PORTS-1:0] m_axis_tuser,
  output wire [           NUM_PORTS-1:0] m_axis_tlast,
  output wire [           NUM_PORTS-1:0] m_axis_tvalid,
  input  wire [           NUM_PORTS-1:0] m_axis_tready
);

  // Output of the input-register stage
  wire [     WIDTH*NUM_PORTS-1:0] reg_tdata;
  wire [USER_WIDTH*NUM_PORTS-1:0] reg_tuser;
  wire                            reg_tlast;
  wire                            reg_tvalid;
  wire                            reg_tready;

  // Input to the Output FIFO stage
  wire [     WIDTH*NUM_PORTS-1:0] fifo_tdata;
  wire [USER_WIDTH*NUM_PORTS-1:0] fifo_tuser;
  wire [           NUM_PORTS-1:0] fifo_tlast;
  wire [           NUM_PORTS-1:0] fifo_tvalid;
  wire [           NUM_PORTS-1:0] fifo_tready;

  // Indicates all output FIFOs are ready for a transfer
  wire all_fifo_tready;


  //---------------------------------------------------------------------------
  // Optional Input Register
  //---------------------------------------------------------------------------

  if (INPUT_REG) begin : gen_input_reg
    axi_fifo_flop2 #(
      .WIDTH (WIDTH+USER_WIDTH+1)
    ) axi_fifo_flop2_i (
      .clk      (clk),
      .reset    (reset),
      .clear    (1'b0),
      .i_tdata  ({s_axis_tdata,s_axis_tuser,s_axis_tlast}),
      .i_tvalid (s_axis_tvalid),
      .i_tready (s_axis_tready),
      .o_tdata  ({reg_tdata,reg_tuser,reg_tlast}),
      .o_tvalid (reg_tvalid),
      .o_tready (reg_tready),
      .space    (),
      .occupied ()
    );
  end else begin : gen_no_input_reg
    assign reg_tdata     = s_axis_tdata;
    assign reg_tuser     = s_axis_tuser;
    assign reg_tlast     = s_axis_tlast;
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
  assign fifo_tdata  = reg_tdata;
  assign fifo_tuser  = reg_tuser;
  assign fifo_tlast  = { NUM_PORTS {reg_tlast} };


  //---------------------------------------------------------------------------
  // Output FIFOs
  //---------------------------------------------------------------------------

  genvar i;
  for (i = 0; i < NUM_PORTS; i = i + 1) begin : gen_ports
    // We use axi_fifo_short specifically because it can tolerate tvalid
    // de-asserting at any time. This is normally not allowed by AXI-Stream.
    axi_fifo_short #(
      .WIDTH (WIDTH+USER_WIDTH+1)
    ) axi_fifo_short_i (
      .clk      (clk),
      .reset    (reset),
      .clear    (1'b0),
      .i_tdata  ({fifo_tdata[i*WIDTH+:WIDTH],fifo_tuser[i*USER_WIDTH+:USER_WIDTH],fifo_tlast[i]}),
      .i_tvalid (fifo_tvalid[i]),
      .i_tready (fifo_tready[i]),
      .o_tdata  ({m_axis_tdata[i*WIDTH+:WIDTH],m_axis_tuser[i*USER_WIDTH+:USER_WIDTH],m_axis_tlast[i]}),
      .o_tvalid (m_axis_tvalid[i]),
      .o_tready (m_axis_tready[i]),
      .space    (),
      .occupied ()
    );
  end

endmodule
