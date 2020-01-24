//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_upsizer
// Description: 
//   An AXI-Stream width conversion module that widens the input
//   sample with by a factor of RATIO.
//   NOTE: This module has end-to-end combanitorial paths. For a
//         pipelined version, please use axis_width_conv
//
// Parameters:
//   - IN_DATA_W: The bitwidth of the input data bus. The width of the
//                output data bus is IN_DATA_W*RATIO
//   - IN_USER_W: The bitwidth of the input user bus. The width of the
//                output user bus is IN_USER_W*RATIO
//   - RATIO: The upsizing ratio
//
// Signals:
//   - s_axis_* : Input sample stream (AXI-Stream)
//   - m_axis_* : Output sample stream (AXI-Stream)

module axis_upsizer #(
  parameter IN_DATA_W = 32,
  parameter IN_USER_W = 1,
  parameter RATIO     = 4
)(
  // Clock, reset and settings
  input  wire                         clk,              // Clock
  input  wire                         reset,            // Reset
  // Data In (AXI-Stream)              
  input  wire [IN_DATA_W-1:0]         s_axis_tdata,     // Input stream tdata
  input  wire [IN_USER_W-1:0]         s_axis_tuser,     // Input stream tuser
  input  wire                         s_axis_tlast,     // Input stream tlast
  input  wire                         s_axis_tvalid,    // Input stream tvalid
  output wire                         s_axis_tready,    // Input stream tready
  // Data Out (AXI-Stream)             
  output wire [(IN_DATA_W*RATIO)-1:0] m_axis_tdata,     // Output stream tdata
  output wire [(IN_USER_W*RATIO)-1:0] m_axis_tuser,     // Output stream tuser
  output wire [RATIO-1:0]             m_axis_tkeep,     // Output stream tkeep
  output wire                         m_axis_tlast,     // Output stream tlast
  output wire                         m_axis_tvalid,    // Output stream tvalid
  input  wire                         m_axis_tready     // Output stream tready
);

  genvar i;
  generate if (RATIO != 1) begin
    // Constants
    localparam [$clog2(RATIO)-1:0] SEL_FIRST  = 'd0;
    localparam [$clog2(RATIO)-1:0] SEL_LAST   = RATIO-1;
    localparam [RATIO-1:0]         KEEP_FIRST = {{(RATIO-1){1'b0}}, 1'b1};
    localparam [RATIO-1:0]         KEEP_ALL   = {(RATIO){1'b1}};
  
    // Keep a binary-coded and one-hot version of the current
    // section of the output that is being processed.
    reg [$clog2(RATIO)-1:0] select = SEL_FIRST;
    reg [RATIO-1:0]         keep   = KEEP_FIRST;
    // Cached data. Incomplete output word.
    reg [IN_DATA_W-1:0] cached_data[0:RATIO-2];
    reg [IN_USER_W-1:0] cached_user[0:RATIO-2];

    // State machine to drive the select bits for the
    // output DEMUX.
    always @(posedge clk) begin
      if (reset) begin
        select <= SEL_FIRST;
        keep   <= KEEP_FIRST;
      end else if (s_axis_tvalid & s_axis_tready) begin
        select <= (select == SEL_LAST || s_axis_tlast) ? SEL_FIRST  : (select + 'd1);
        keep   <= (keep   == KEEP_ALL || s_axis_tlast) ? KEEP_FIRST : {keep[RATIO-2:0], 1'b1};
        cached_data[select] <= s_axis_tdata;
        cached_user[select] <= s_axis_tuser;
      end
    end

    // The output DEMUX
    for (i = 0; i < RATIO; i=i+1) begin
      if (i == SEL_LAST) begin
        assign m_axis_tdata[(i*IN_DATA_W)+:IN_DATA_W] = s_axis_tdata;
        assign m_axis_tuser[(i*IN_USER_W)+:IN_USER_W] = s_axis_tuser;
      end else begin
        assign m_axis_tdata[(i*IN_DATA_W)+:IN_DATA_W] = keep[i+1] ? cached_data[i] : s_axis_tdata;
        assign m_axis_tuser[(i*IN_USER_W)+:IN_USER_W] = keep[i+1] ? cached_user[i] : s_axis_tuser;
      end
    end
    assign m_axis_tkeep   = keep;
    assign m_axis_tlast   = s_axis_tlast;
    assign m_axis_tvalid  = s_axis_tvalid & ((keep == KEEP_ALL) | s_axis_tlast);
    assign s_axis_tready  = m_axis_tvalid ? m_axis_tready : s_axis_tvalid;

  end else begin  // if (RATIO != 1)

    // Passthrough
    assign m_axis_tdata   = s_axis_tdata;
    assign m_axis_tuser   = s_axis_tuser;
    assign m_axis_tkeep   = 1'b1;
    assign m_axis_tlast   = s_axis_tlast;
    assign m_axis_tvalid  = s_axis_tvalid;
    assign s_axis_tready  = m_axis_tready;

  end endgenerate

endmodule // axis_upsizer
