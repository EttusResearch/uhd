//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_switch
//
// Description:
//
//   Implementation of a M-input, N-output AXI-Stream switch. One of the M
//   input ports is allocated based on the s_axis_alloc signal and the packet
//   on that port is sent to one of the N output ports based on the tdest
//   signal.
//
// Parameters:
//
//   DATA_W    : tdata width
//   DEST_W    : Output tdest width
//   IN_PORTS  : Number of input ports
//   OUT_PORTS : Number of output ports
//   PIPELINE  : Instantiate output pipeline stage?
//   ALLOC_W   : PRIVATE. Do not modify.
//
// Ports:
//
//   clk           : Switch clock
//   reset         : Reset
//   s_axis_tdata  : Input data
//   s_axis_tdest  : Input destination
//   s_axis_tlast  : Input EOP (last)
//   s_axis_tvalid : Input valid
//   s_axis_tready : Input ready
//   s_axis_alloc  : Input port allocation for switch
//   m_axis_tdata  : Output data
//   m_axis_tdest  : Output destination
//   m_axis_tlast  : Output EOP (last)
//   m_axis_tvalid : Output valid
//   m_axis_tready : Output ready
//

`default_nettype none


module axis_switch #(
  parameter DATA_W    = 64,
  parameter DEST_W    = 1,
  parameter IN_PORTS  = 3,
  parameter OUT_PORTS = 3,
  parameter PIPELINE  = 1,
  parameter ALLOC_W   = (IN_PORTS == 1) ? 1 : $clog2(IN_PORTS)
) (
  // Clocks and resets
  input  wire                                             clk,
  input  wire                                             reset,
  // Input ports
  input  wire [(DATA_W*IN_PORTS)-1:0]                     s_axis_tdata,
  input  wire [((DEST_W+$clog2(OUT_PORTS))*IN_PORTS)-1:0] s_axis_tdest,
  input  wire [IN_PORTS-1:0]                              s_axis_tlast,
  input  wire [IN_PORTS-1:0]                              s_axis_tvalid,
  output wire [IN_PORTS-1:0]                              s_axis_tready,
  input  wire [ALLOC_W-1:0]                               s_axis_alloc,
  // Output ports
  output wire [(DATA_W*OUT_PORTS)-1:0]                    m_axis_tdata,
  output wire [(DEST_W*OUT_PORTS)-1:0]                    m_axis_tdest,
  output wire [OUT_PORTS-1:0]                             m_axis_tlast,
  output wire [OUT_PORTS-1:0]                             m_axis_tvalid,
  input  wire [OUT_PORTS-1:0]                             m_axis_tready
);
  localparam CLOG2_OUT_PORTS = $clog2(OUT_PORTS);

  //---------------------------------------------------------
  // Flatten/unflatten and pipeline
  //---------------------------------------------------------
  wire [DATA_W-1:0]                   i_tdata [0:IN_PORTS-1];
  wire [DEST_W+$clog2(OUT_PORTS)-1:0] i_tdest [0:IN_PORTS-1];
  wire                                i_tlast [0:IN_PORTS-1];
  wire [IN_PORTS-1:0]                 i_tvalid;
  wire [IN_PORTS-1:0]                 i_tready;
  wire [ALLOC_W-1:0]                  i_alloc;
  wire [DATA_W-1:0]                   o_tdata [0:OUT_PORTS-1];
  wire [DEST_W-1:0]                   o_tdest [0:OUT_PORTS-1];
  wire                                o_tlast [0:OUT_PORTS-1];
  wire [OUT_PORTS-1:0]                o_tvalid;
  wire [OUT_PORTS-1:0]                o_tready;

  genvar i, o;
  generate
    for (i = 0; i < IN_PORTS; i = i + 1) begin: in_ports
      assign i_tdata      [i] = s_axis_tdata [(i*DATA_W)+:DATA_W];
      assign i_tdest      [i] = s_axis_tdest [(i*(DEST_W+CLOG2_OUT_PORTS))+:(DEST_W+CLOG2_OUT_PORTS)];
      assign i_tlast      [i] = s_axis_tlast [i];
      assign i_tvalid     [i] = s_axis_tvalid[i];
      assign s_axis_tready[i] = i_tready     [i];
    end
    assign i_alloc = s_axis_alloc;  // i_alloc has to be delay-matched to valid

    for (o = 0; o < OUT_PORTS; o = o + 1) begin : gen_for_pipeline
      if (PIPELINE == 1) begin : gen_pipeline
        axi_fifo_flop2 #(.WIDTH(DEST_W+1+DATA_W)) axi_fifo_flop2_i (
          .clk(clk), .reset(reset), .clear(1'b0),
          .i_tdata({o_tdest[o], o_tlast[o], o_tdata[o]}),
          .i_tvalid(o_tvalid[o]), .i_tready(o_tready[o]),
          .o_tdata({m_axis_tdest[(o*DEST_W)+:DEST_W], m_axis_tlast[o], m_axis_tdata[(o*DATA_W)+:DATA_W]}),
          .o_tvalid(m_axis_tvalid[o]), .o_tready(m_axis_tready[o]),
          .space(), .occupied()
        );
      end else begin : gen_no_pipeline
        assign m_axis_tdata [(o*DATA_W)+:DATA_W] = o_tdata      [o];
        assign m_axis_tdest [(o*DEST_W)+:DEST_W] = o_tdest      [o];
        assign m_axis_tlast [o]                  = o_tlast      [o];
        assign m_axis_tvalid[o]                  = o_tvalid     [o];
        assign o_tready     [o]                  = m_axis_tready[o];
      end
    end
  endgenerate

  //---------------------------------------------------------
  // Allocator
  //---------------------------------------------------------
  // The "chosen" input port will drive this bus
  wire [DATA_W-1:0]                   master_tdata;
  wire [DEST_W+$clog2(OUT_PORTS)-1:0] master_tdest;
  wire                                master_tlast;
  wire                                master_tvalid;
  wire                                master_tready;

  generate if (IN_PORTS > 1) begin : gen_mult_in_ports
    reg [IN_PORTS-1:0] ialloc_oh;
    reg [$clog2(IN_PORTS)-1:0] alloc_reg;
    always @(posedge clk) begin
      if (reset) begin
        ialloc_oh <= {IN_PORTS{1'b0}};
      end else begin
        if (ialloc_oh == {IN_PORTS{1'b0}}) begin
          if (|i_tvalid) begin
            ialloc_oh[i_alloc] <= 1'b1;
            alloc_reg <= i_alloc;
          end
        end else begin
          if(master_tready & master_tvalid & master_tlast)
            ialloc_oh <= {IN_PORTS{1'b0}};
        end
      end
    end

    assign master_tdata   = i_tdata[alloc_reg];
    assign master_tdest   = i_tdest[alloc_reg];
    assign master_tlast   = i_tlast[alloc_reg];
    assign master_tvalid  = |(i_tvalid & ialloc_oh);
    assign i_tready       = i_tvalid & ialloc_oh & {IN_PORTS{master_tready}};
  end else begin : gen_single_in_port
    // Special case: One input port
    assign master_tdata   = i_tdata[0];
    assign master_tdest   = i_tdest[0];
    assign master_tlast   = i_tlast[0];
    assign master_tvalid  = i_tvalid[0];
    assign i_tready[0]    = master_tready;
  end endgenerate

  //---------------------------------------------------------
  // Router
  //---------------------------------------------------------
  generate if (OUT_PORTS > 1) begin : gen_mult_out_ports
    reg [OUT_PORTS-1:0] odst_oh;
    always @(posedge clk) begin
      if (reset) begin
        odst_oh <= {OUT_PORTS{1'b0}};
      end else begin
        if (odst_oh == {OUT_PORTS{1'b0}}) begin
          if (master_tvalid)
            odst_oh[master_tdest[CLOG2_OUT_PORTS-1:0]] <= 1'b1;
        end else begin
          if(master_tready & master_tvalid & master_tlast)
            odst_oh <= {OUT_PORTS{1'b0}};
        end
      end
    end
    assign master_tready = |(o_tready & odst_oh);
    assign o_tvalid = {OUT_PORTS{master_tvalid}} & odst_oh;
  end else begin : gen_single_out_port
    // Special case: One output port
    assign master_tready = o_tready[0];
    assign o_tvalid[0] = master_tvalid;
  end endgenerate

  generate for (o = 0; o < OUT_PORTS; o = o + 1) begin : gen_outputs
    assign o_tdata[o] = master_tdata;
    assign o_tdest[o] = master_tdest[DEST_W+CLOG2_OUT_PORTS-1:CLOG2_OUT_PORTS];
    assign o_tlast[o] = master_tlast;
  end endgenerate

endmodule


`default_nettype wire
