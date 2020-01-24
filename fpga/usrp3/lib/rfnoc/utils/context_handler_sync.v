//
// Copyright 2018-2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: context_handler_sync
// Description:
//
// Parameters:
//   - CHDR_W: Width of the input CHDR bus in bits
//
// Signals:
//

module context_handler_sync #(
  parameter CHDR_W = 256,
  parameter ITEM_W = 32,
  parameter NIPC   = 2
)(
  // Clock and reset
  input  wire              clk,
  input  wire              rst,
  // Context stream in (AXI-Stream)
  input  wire [CHDR_W-1:0] s_axis_context_tdata,
  input  wire [3:0]        s_axis_context_tuser,
  input  wire              s_axis_context_tlast,
  input  wire              s_axis_context_tvalid,
  output wire              s_axis_context_tready,
  // Context stream out (AXI-Stream)
  output wire [CHDR_W-1:0] m_axis_context_tdata,
  output wire [3:0]        m_axis_context_tuser,
  output wire              m_axis_context_tlast,
  output wire              m_axis_context_tvalid,
  input  wire              m_axis_context_tready,
  // Input payload stream monitor
  input  wire [NIPC-1:0]   in_payload_tkeep,
  input  wire              in_payload_tlast,
  input  wire              in_payload_tvalid,
  input  wire              in_payload_tready,
  // Output payload stream monitor
  input  wire [NIPC-1:0]   out_payload_tkeep,
  input  wire              out_payload_tlast,
  input  wire              out_payload_tvalid,
  input  wire              out_payload_tready,
  // Status
  output reg               length_err_stb,
  output reg               seq_err_stb
);

  `include "../core/rfnoc_chdr_utils.vh"

  // Thermometer to binary decoder
  // 4'b0000 => 3'd0
  // 4'b0001 => 3'd1
  // 4'b0011 => 3'd2
  // 4'b0111 => 3'd3
  // 4'b1111 => 3'd4
  function [$clog2(NIPC):0] thermo2bin(input [NIPC-1:0] thermo);
    reg [NIPC:0] onehot;
    integer i;
  begin
    onehot = thermo + 1;
    thermo2bin = 0;
    for (i = 0; i <= NIPC; i=i+1)
      if (onehot[i])
        thermo2bin = thermo2bin | i;
  end
  endfunction

  axi_fifo #(.WIDTH(CHDR_W+4+1), .SIZE(1)) ctxt_pipe_i (
    .clk(clk), .reset(rst), .clear(1'b0),
    .i_tdata({s_axis_context_tlast, s_axis_context_tuser, s_axis_context_tdata}),
    .i_tvalid(s_axis_context_tvalid), .i_tready(s_axis_context_tready),
    .o_tdata({m_axis_context_tlast, m_axis_context_tuser, m_axis_context_tdata}),
    .o_tvalid(m_axis_context_tvalid), .o_tready(m_axis_context_tready),
    .space(), .occupied()
  );

  wire is_ctxt_hdr = s_axis_context_tvalid && s_axis_context_tready &&
                     (s_axis_context_tuser == CONTEXT_FIELD_HDR || 
                      s_axis_context_tuser == CONTEXT_FIELD_HDR_TS);

  reg [15:0] exp_pkt_len = 16'd0;
  reg [15:0] exp_seq_num = 16'd0;
  reg        check_seq_num = 1'b0;
  always @(posedge clk) begin
    if (rst) begin
      exp_pkt_len <= 16'd0;
      check_seq_num <= 1'b0;
    end else if (is_ctxt_hdr) begin
      check_seq_num <= 1'b1;
      exp_pkt_len <= chdr_get_length(s_axis_context_tdata[63:0]);
      exp_seq_num <= chdr_get_seq_num(s_axis_context_tdata[63:0]) + 16'd1;
    end
    seq_err_stb <= is_ctxt_hdr && check_seq_num && 
                   (exp_seq_num != chdr_get_seq_num(s_axis_context_tdata[63:0]));
  end

  reg [15:0] pyld_pkt_len = 16'd0;
  always @(posedge clk) begin
    if (rst) begin
      pyld_pkt_len <= 16'd0;
    end else if (in_payload_tvalid && in_payload_tready) begin
      pyld_pkt_len <= in_payload_tlast ? 16'd0 : (pyld_pkt_len + ((ITEM_W*NIPC)/8));
    end
    length_err_stb <= in_payload_tvalid && in_payload_tready && in_payload_tlast &&
                      (pyld_pkt_len + (thermo2bin(in_payload_tkeep)*(ITEM_W/8)) != exp_pkt_len);
  end

endmodule // context_handler_sync
