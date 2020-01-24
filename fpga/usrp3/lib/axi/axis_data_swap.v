//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_data_swap
// Description:
//  A generic data swapper module for AXI-Stream. The contents of
//  tdata are swapped based on the tswap signal. For each bit 'i'
//  in tswap, adjacent words of width 2^i are swapped if tswap[i]
//  is high. For example, if tswap[3] = 1, then each byte in tdata
//  will be swapped with its adjacent neighbor. It is permissible
//  for tswap to change for each transfer in an AXIS packet.
//  Swapping can also be configured to be static (zero logic) by
//  setting DYNAMIC = 0. To reduce area, certain swap stages can
//  even be disabled. For example, if STAGES_EN[2:0] is set to 0
//  then the lowest granularity for swaps will be a byte.
//
// Parameters:
//   - DATA_W: Width of the tdata bus in bits
//   - USER_W: Width of the tuser bus in bits
//   - STAGES_EN: Which swap stages are enabled.
//   - DYNAMIC: Dynamic swapping enabled (use tswap)
//
// Signals:
//   - s_axis_*: The input AXI stream
//   - m_axis_*: The output AXI stream
//

module axis_data_swap #(
  parameter integer              DATA_W    = 256,
  parameter integer              USER_W    = 1,
  parameter [$clog2(DATA_W)-1:0] STAGES_EN = 'hFFFFFFFF, //@HACK: Vivado does not allow $clog2 in value of this expr
  parameter [0:0]                DYNAMIC   = 1
)(
  // Clock and Reset
  input  wire                      clk,
  input  wire                      rst,
  // Input AXIS
  input  wire [DATA_W-1:0]         s_axis_tdata,
  input  wire [$clog2(DATA_W)-2:0] s_axis_tswap,
  input  wire [USER_W-1:0]         s_axis_tuser,
  input  wire                      s_axis_tlast,
  input  wire                      s_axis_tvalid,
  output wire                      s_axis_tready,
  // Output AXIS                   
  output wire [DATA_W-1:0]         m_axis_tdata,
  output wire [USER_W-1:0]         m_axis_tuser,
  output wire                      m_axis_tlast,
  output wire                      m_axis_tvalid,
  input  wire                      m_axis_tready
);

  parameter SWAP_STAGES = $clog2(DATA_W);
  parameter SWAP_W      = $clog2(DATA_W)-1;
  genvar s, w;

  wire [DATA_W-1:0] stg_tdata [0:SWAP_STAGES], stg_tdata_swp[0:SWAP_STAGES], stg_tdata_res[0:SWAP_STAGES];
  wire [SWAP_W-1:0] stg_tswap [0:SWAP_STAGES];
  wire [USER_W-1:0] stg_tuser [0:SWAP_STAGES];
  wire              stg_tlast [0:SWAP_STAGES];
  wire              stg_tvalid[0:SWAP_STAGES];
  wire              stg_tready[0:SWAP_STAGES];

  // Connect input and output to stage wires
  generate
    assign stg_tdata [0] = s_axis_tdata;
    assign stg_tswap [0] = s_axis_tswap;
    assign stg_tuser [0] = s_axis_tuser;
    assign stg_tlast [0] = s_axis_tlast;
    assign stg_tvalid[0] = s_axis_tvalid;
    assign s_axis_tready = stg_tready[0];

    assign m_axis_tdata            = stg_tdata [SWAP_STAGES];
    assign m_axis_tuser            = stg_tuser [SWAP_STAGES];
    assign m_axis_tlast            = stg_tlast [SWAP_STAGES];
    assign m_axis_tvalid           = stg_tvalid[SWAP_STAGES];
    assign stg_tready[SWAP_STAGES] = m_axis_tready;
  endgenerate

  // Instantiate AXIS flip-flops for each stage
  generate
    for (s = 0; s < SWAP_STAGES; s=s+1) begin
      if (STAGES_EN[SWAP_STAGES-s-1]) begin
        // Swap Logic
        for (w = 0; w < (1<<s); w=w+1) begin
          assign stg_tdata_swp[s][(w*(DATA_W/(1<<s)))+:(DATA_W/(1<<s))] =
            stg_tdata[s][(((1<<s)-w-1)*(DATA_W/(1<<s)))+:(DATA_W/(1<<s))];
        end
        if (DYNAMIC) begin
          // Honor tswap in DYNAMIC mode.
          // Also add a flip_flop to break the long start-to-end critical path
          assign stg_tdata_res[s] = (s > 0 && stg_tswap[s][SWAP_W-s]) ?
            stg_tdata_swp[s] : stg_tdata[s];
          // Flip-flop
          axi_fifo_flop #(.WIDTH(DATA_W+SWAP_W+USER_W+1)) reg_i (
            .clk(clk), .reset(rst), .clear(1'b0),
            .i_tdata({stg_tlast[s], stg_tuser[s], stg_tswap[s], stg_tdata_res[s]}),
            .i_tvalid(stg_tvalid[s]), .i_tready(stg_tready[s]),
            .o_tdata({stg_tlast[s+1], stg_tuser[s+1], stg_tswap[s+1], stg_tdata[s+1]}),
            .o_tvalid(stg_tvalid[s+1]), .o_tready(stg_tready[s+1]),
            .occupied(), .space()
          );
        end else begin
          // Static swapping logic
          assign stg_tdata [s+1] = stg_tdata_swp[s];
          assign stg_tswap [s+1] = stg_tswap    [s];
          assign stg_tuser [s+1] = stg_tuser    [s];
          assign stg_tlast [s+1] = stg_tlast    [s];
          assign stg_tvalid[s+1] = stg_tvalid   [s];
          assign stg_tready[s]   = stg_tready   [s+1];
        end
      end else begin
        // Skip this stage
        assign stg_tdata [s+1] = stg_tdata [s];
        assign stg_tswap [s+1] = stg_tswap [s];
        assign stg_tuser [s+1] = stg_tuser [s];
        assign stg_tlast [s+1] = stg_tlast [s];
        assign stg_tvalid[s+1] = stg_tvalid[s];
        assign stg_tready[s]   = stg_tready[s+1];
      end
    end
  endgenerate

endmodule // axis_data_swap
