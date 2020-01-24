//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_shift_register
// Description:
//   This module implements a chain of flip-flops in connected
//   using AXI-Stream. It can be used in the following ways:
//   * As a AXI-Stream shift register. The tready path is 
//     combinatorial from the output to the input so backpressure
//     is immediate. The same behavior makes this module non-ideal
//     to actually break timing critical paths.
//   * An AXI-Stream wrapper module for a multi-cycle operation
//     with clock-enables. This can most commonly be used with DSP
//     operations like filters. Enable the sideband datapath to 
//     let the module handle handshaking while processing samples
//     outside it.
//
// Parameters:
//   - WIDTH: The bitwidth of a sample on the data bus.
//   - NSPC: The number of parallel samples per cycle to process. The
//       total width of the data bus will be WIDTH*NSPC.
//   - LATENCY: Number of stages in the shift register
//   - SIDEBAND_DATAPATH: If SIDEBAND_DATAPATH==1 then tdata is managed
//       outside this module and imported from s_sideband_data. 
//       If SIDEBAND_DATAPATH=0, then tdata is managed internally and 
//       the sideband signals are unused.
//       Useful when using this module to manage a DSP pipeline where the
//       data could be changing in each stage.
//   - GAPLESS: After the shift register has filled up, should gaps be
//       allowed? If set to 1, then if s_axis_tvalid goes low then the 
//       pipeline will stall and all bits in stage_stb will immediately go low 
//       to ensure all stages in the shift register have valid data.
//       NOTE: This GAPLESS=1 will not allow the final "LATENCY" samples
//       to exit the shift register.
//   - PIPELINE: Which ports to pipeline? {NONE, IN, OUT, INOUT}
//
// Signals:
//   - s_axis_* : Input sample stream (AXI-Stream)
//   - m_axis_* : Output sample stream (AXI-Stream)
//   - stage_stb : Transfer strobe for each stage 
//   - stage_eop : Transfer end-of-packet out. bit[i] = stage[i]
//   - m_sideband_data : Sideband data out for external consumer
//   - m_sideband_keep : Sideband keep signal out for external consumer
//   - s_sideband_data : Sideband data in from external producer

module axis_shift_register #(
  parameter WIDTH             = 32,
  parameter NSPC              = 1,
  parameter LATENCY           = 3,
  parameter SIDEBAND_DATAPATH = 0,
  parameter GAPLESS           = 0,
  parameter PIPELINE          = "NONE"
)(
  // Clock, reset and settings
  input  wire                     clk,              // Clock
  input  wire                     reset,            // Reset
  // Serial Data In (AXI-Stream)              
  input  wire [(WIDTH*NSPC)-1:0]  s_axis_tdata,     // Input stream tdata
  input  wire [NSPC-1:0]          s_axis_tkeep,     // Input stream tkeep (used as a sample qualifier)
  input  wire                     s_axis_tlast,     // Input stream tlast
  input  wire                     s_axis_tvalid,    // Input stream tvalid
  output wire                     s_axis_tready,    // Input stream tready
  // Serial Data Out (AXI-Stream)             
  output wire [(WIDTH*NSPC)-1:0]  m_axis_tdata,     // Output stream tdata
  output wire [NSPC-1:0]          m_axis_tkeep,     // Output stream tkeep (used as a sample qualifier)
  output wire                     m_axis_tlast,     // Output stream tlast
  output wire                     m_axis_tvalid,    // Output stream tvalid
  input  wire                     m_axis_tready,    // Output stream tready
  // Signals for the sideband data path                     
  output wire [LATENCY-1:0]       stage_stb,        // Transfer strobe out. bit[i] = stage[i]
  output wire [LATENCY-1:0]       stage_eop,        // Transfer end-of-packet out. bit[i] = stage[i]
  output wire [(WIDTH*NSPC)-1:0]  m_sideband_data,  // Sideband data out for external consumer
  output wire [NSPC-1:0]          m_sideband_keep,  // Sideband keep signal out for external consumer
  input  wire [(WIDTH*NSPC)-1:0]  s_sideband_data   // Sideband data in from external producer
);
  // Shift register width depends on whether the datapath is internal
  localparam SHREG_WIDTH     = SIDEBAND_DATAPATH[0] ? (NSPC + 1) : ((WIDTH*NSPC) + NSPC + 1);
  localparam SHREG_TLAST_LOC = SHREG_WIDTH-1;
  localparam SHREG_TKEEP_HI  = SHREG_WIDTH-2;
  localparam SHREG_TKEEP_LO  = SHREG_WIDTH-NSPC-1;

  //----------------------------------------------
  // Pipeline Logic
  // (fifo_flop2 is used because it breaks timing
  //  path going both ways: valid and ready)
  //----------------------------------------------
  wire [(WIDTH*NSPC)-1:0] i_tdata,  o_tdata;
  wire [NSPC-1:0]         i_tkeep,  o_tkeep;
  wire                    i_tlast,  o_tlast;
  wire                    i_tvalid, o_tvalid;
  wire                    i_tready, o_tready;

  generate
    // Input pipeline register if requested
    if (PIPELINE == "IN" || PIPELINE == "INOUT") begin
      axi_fifo_flop2 #(.WIDTH((WIDTH*NSPC) + NSPC + 1)) in_pipe_i (
        .clk(clk), .reset(reset), .clear(1'b0),
        .i_tdata({s_axis_tlast, s_axis_tkeep, s_axis_tdata}),
        .i_tvalid(s_axis_tvalid), .i_tready(s_axis_tready),
        .o_tdata({i_tlast, i_tkeep, i_tdata}), .o_tvalid(i_tvalid), .o_tready(i_tready),
        .space(), .occupied()
      );
    end else begin
      assign {i_tlast, i_tkeep, i_tdata} = {s_axis_tlast, s_axis_tkeep, s_axis_tdata};
      assign i_tvalid = s_axis_tvalid;
      assign s_axis_tready = i_tready;
    end

    // Output pipeline register if requested
    if (PIPELINE == "OUT" || PIPELINE == "INOUT") begin
      axi_fifo_flop2 #(.WIDTH((WIDTH*NSPC) + NSPC + 1)) out_pipe_i (
        .clk(clk), .reset(reset), .clear(1'b0),
        .i_tdata({o_tlast, o_tkeep, o_tdata}), .i_tvalid(o_tvalid), .i_tready(o_tready),
        .o_tdata({m_axis_tlast, m_axis_tkeep, m_axis_tdata}),
        .o_tvalid(m_axis_tvalid), .o_tready(m_axis_tready),
        .space(), .occupied()
      );
    end else begin
      assign {m_axis_tlast, m_axis_tkeep, m_axis_tdata} = {o_tlast, o_tkeep, o_tdata};
      assign m_axis_tvalid = o_tvalid;
      assign o_tready = m_axis_tready;
    end
  endgenerate

  assign m_sideband_data = i_tdata;
  assign m_sideband_keep = i_tkeep;

  //----------------------------------------------
  // Shift register stages
  //----------------------------------------------
  genvar i;
  generate
    if (GAPLESS == 0) begin
      // Individual stage wires
      wire [SHREG_WIDTH-1:0]  stg_tdata [0:LATENCY];
      wire                    stg_tvalid[0:LATENCY];
      wire                    stg_tready[0:LATENCY];
      // Shift register input
      assign stg_tdata[0] = SIDEBAND_DATAPATH[0] ? {i_tlast, i_tkeep} : {i_tlast, i_tkeep, i_tdata};
      assign stg_tvalid[0] = i_tvalid;
      assign i_tready = stg_tready[0];
      // Shift register output
      assign o_tlast = stg_tdata[LATENCY][SHREG_TLAST_LOC];
      assign o_tkeep = stg_tdata[LATENCY][SHREG_TKEEP_HI:SHREG_TKEEP_LO];
      assign o_tdata = SIDEBAND_DATAPATH[0] ? s_sideband_data : stg_tdata[LATENCY][(WIDTH*NSPC)-1:0];
      assign o_tvalid = stg_tvalid[LATENCY];
      assign stg_tready[LATENCY] = o_tready;
  
      for (i = 0; i < LATENCY; i=i+1) begin: stages
        axi_fifo_flop #(.WIDTH(SHREG_WIDTH)) reg_i (
          .clk(clk), .reset(reset), .clear(1'b0),
          .i_tdata(stg_tdata[i  ]), .i_tvalid(stg_tvalid[i  ]), .i_tready(stg_tready[i  ]),
          .o_tdata(stg_tdata[i+1]), .o_tvalid(stg_tvalid[i+1]), .o_tready(stg_tready[i+1]),
          .occupied(), .space()
        );
        assign stage_stb[i] = stg_tvalid[i] & stg_tready[i];
        assign stage_eop[i] = stage_stb[i] & stg_tdata[i][SHREG_TLAST_LOC];
      end
    end else begin // if (GAPLESS == 0)
      wire [(WIDTH*NSPC)-1:0] o_tdata_fifo;
      wire [NSPC-1:0]         o_tkeep_fifo;
      wire                    o_tlast_fifo, o_tvalid_fifo, o_tready_fifo;

      // Shift register to hold valids
      reg  [LATENCY-1:0]     stage_valid = {LATENCY{1'b0}};
      // Shift register to hold data/last
      reg  [SHREG_WIDTH-1:0] stage_shreg[0:LATENCY-1];
      wire [SHREG_WIDTH-1:0] shreg_input = SIDEBAND_DATAPATH[0] ? {i_tlast, i_tkeep} : {i_tlast, i_tkeep, i_tdata};
      wire                   shreg_ce = i_tready & i_tvalid;

      assign i_tready      = o_tready_fifo;
      assign o_tvalid_fifo = stage_valid[LATENCY-1] & shreg_ce;
      assign o_tlast_fifo  = stage_shreg[LATENCY-1][SHREG_TLAST_LOC];
      assign o_tkeep_fifo  = stage_shreg[LATENCY-1][SHREG_TKEEP_HI:SHREG_TKEEP_LO];
      assign o_tdata_fifo  = SIDEBAND_DATAPATH[0] ? s_sideband_data : stage_shreg[LATENCY-1][(WIDTH*NSPC)-1:0];

      for (i = 0; i < LATENCY; i=i+1) begin
        // Initialize shift register
        initial begin
          stage_shreg[i] <= {SHREG_WIDTH{1'b0}};
        end
        // Shift register logic
        always @(posedge clk) begin
          if (reset) begin
            stage_shreg[i] <= {SHREG_WIDTH{1'b0}};
            stage_valid[i] <= 1'b0;
          end else if (shreg_ce) begin
            stage_shreg[i] <= (i == 0) ? shreg_input : stage_shreg[i-1];
            stage_valid[i] <= (i == 0) ? 1'b1        : stage_valid[i-1];
          end
        end
        // Outputs
        assign stage_stb[i] = ((i == 0) ? 1'b1 : stage_valid[i-1]) & shreg_ce;
        assign stage_eop[i] = stage_stb[i] & ((i == 0) ? i_tlast : stage_shreg[i-1][SHREG_TLAST_LOC]);
      end

      // The "gapless" logic violates AXI-Stream by having an o_tready -> o_tvalid dependency, 
      // so we add a FIFO downstream to prevent deadlocks.
      axi_fifo #(.WIDTH((WIDTH*NSPC) + NSPC + 1), .SIZE($clog2(LATENCY))) out_fifo_i (
        .clk(clk), .reset(reset), .clear(1'b0),
        .i_tdata({o_tlast_fifo, o_tkeep_fifo, o_tdata_fifo}), .i_tvalid(o_tvalid_fifo), .i_tready(o_tready_fifo),
        .o_tdata({o_tlast, o_tkeep, o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready),
        .space(), .occupied()
      );
    end
  endgenerate
endmodule // axis_shift_register
