//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_downsizer
// Description: 
//   An AXI-Stream width conversion module that narrows the input
//   sample with by a factor of RATIO.
//   NOTE: This module has end-to-end combanitorial paths. For a
//         pipelined version, please use axis_width_conv
//
// Parameters:
//   - OUT_DATA_W: The bitwidth of the output data bus. The width of the
//                 input data bus is OUT_DATA_W*RATIO
//   - OUT_USER_W: The bitwidth of the output user bus. The width of the
//                 input user bus is OUT_USER_W*RATIO
//   - RATIO: The downsizing ratio
//
// Signals:
//   - s_axis_* : Input sample stream (AXI-Stream)
//   - m_axis_* : Output sample stream (AXI-Stream)

module axis_downsizer #(
  parameter OUT_DATA_W  = 32,
  parameter OUT_USER_W  = 1,
  parameter RATIO       = 4
)(
  // Clock, reset and settings
  input  wire                          clk,              // Clock
  input  wire                          reset,            // Reset
  // Data In (AXI-Stream)              
  input  wire [(OUT_DATA_W*RATIO)-1:0] s_axis_tdata,     // Input stream tdata
  input  wire [(OUT_USER_W*RATIO)-1:0] s_axis_tuser,     // Input stream tuser
  input  wire [RATIO-1:0]              s_axis_tkeep,     // Input stream tkeep
  input  wire                          s_axis_tlast,     // Input stream tlast
  input  wire                          s_axis_tvalid,    // Input stream tvalid
  output wire                          s_axis_tready,    // Input stream tready
  // Data Out (AXI-Stream)             
  output wire [OUT_DATA_W-1:0]         m_axis_tdata,     // Output stream tdata
  output wire [OUT_USER_W-1:0]         m_axis_tuser,     // Output stream tuser
  output wire                          m_axis_tlast,     // Output stream tlast
  output wire                          m_axis_tvalid,    // Output stream tvalid
  input  wire                          m_axis_tready     // Output stream tready
);

  genvar i;
  generate if (RATIO != 1) begin
    // Constants
    localparam [$clog2(RATIO)-1:0] SEL_FIRST  = 'd0;
    localparam [$clog2(RATIO)-1:0] SEL_LAST   = RATIO-1;
    localparam [RATIO-1:0]         KEEP_FIRST = {{(RATIO-1){1'b0}}, 1'b1};
    localparam [RATIO-1:0]         KEEP_ALL   = {(RATIO){1'b1}};

    // Keep a binary-coded and one-hot version of the current
    // section of the input that is being processed.
    reg [$clog2(RATIO)-1:0] select = SEL_FIRST;
    reg [RATIO-1:0]         keep   = KEEP_FIRST;

    // State machine to drive the select bits for the
    // input selection MUX.
    always @(posedge clk) begin
      if (reset) begin
        select <= SEL_FIRST;
        keep   <= KEEP_FIRST;
      end else if (m_axis_tvalid & m_axis_tready) begin
        select <= (select == SEL_LAST || m_axis_tlast) ? SEL_FIRST  : (select + 'd1);
        keep   <= (keep   == KEEP_ALL || m_axis_tlast) ? KEEP_FIRST : {keep[RATIO-2:0], 1'b1};
      end
    end

    // The input selection MUX
    wire [OUT_DATA_W-1:0] in_data[0:RATIO-1];
    wire [OUT_USER_W-1:0] in_user[0:RATIO-1];
    for (i = 0; i < RATIO; i=i+1) begin
      assign in_data[i] = s_axis_tdata[i*OUT_DATA_W+:OUT_DATA_W];
      assign in_user[i] = s_axis_tuser[i*OUT_USER_W+:OUT_USER_W];
    end
    assign m_axis_tdata   = in_data[select];
    assign m_axis_tuser   = in_user[select];
    assign m_axis_tlast   = s_axis_tlast && (keep == s_axis_tkeep);
    assign m_axis_tvalid  = s_axis_tvalid;
    assign s_axis_tready  = m_axis_tvalid && m_axis_tready && ((keep == KEEP_ALL) || m_axis_tlast);

  end else begin  // if (RATIO != 1)

    // Passthrough
    assign m_axis_tdata   = s_axis_tdata;
    assign m_axis_tuser   = s_axis_tuser;
    assign m_axis_tlast   = s_axis_tlast;
    assign m_axis_tvalid  = s_axis_tvalid;
    assign s_axis_tready  = m_axis_tready;

  end endgenerate

endmodule // axis_downsizer
