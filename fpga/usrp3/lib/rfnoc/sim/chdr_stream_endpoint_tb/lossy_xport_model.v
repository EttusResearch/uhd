//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: lossy_xport_model
//

module lossy_xport_model #(
  parameter CHDR_W = 256
)(
  input  wire              clk,
  input  wire              rst,
  input  wire [CHDR_W-1:0] s_axis_tdata,
  input  wire              s_axis_tlast,
  input  wire              s_axis_tvalid,
  output wire              s_axis_tready,
  output wire [CHDR_W-1:0] m_axis_tdata,
  output wire              m_axis_tlast,
  output wire              m_axis_tvalid,
  input  wire              m_axis_tready,
  input  wire [7:0]        seqerr_prob,
  input  wire [7:0]        rterr_prob,
  input  wire              lossy
);
  wire [CHDR_W-1:0] tmp_tdata;
  wire              tmp_tlast;
  wire              tmp_tvalid;
  wire              tmp_tready;

  reg pkt_header = 1'b1;
  always @(posedge clk) begin
    if (rst) begin
      pkt_header <= 1'b1;
    end else if (s_axis_tvalid && s_axis_tready) begin
      pkt_header <= s_axis_tlast;
    end
  end
  wire pkt_stb = (s_axis_tvalid && s_axis_tready && s_axis_tlast);

  reg force_seq_err, force_route_err;
  always @(pkt_stb or seqerr_prob) begin
    force_seq_err = ($urandom_range(99) < seqerr_prob);
  end
  always @(pkt_stb or rterr_prob) begin
    force_route_err = ($urandom_range(99) < rterr_prob);
  end

  wire [15:0] new_seq_num  = s_axis_tdata[47:32] + 16'd1;   //Increment SeqNum
  wire [15:0] new_dst_epid = ~s_axis_tdata[15:0];           //Invert DstEPID

  assign tmp_tdata = !pkt_header ? s_axis_tdata : (
    force_seq_err ? {s_axis_tdata[CHDR_W-1:48], new_seq_num, s_axis_tdata[31:0]} : (
      force_route_err ? {s_axis_tdata[CHDR_W-1:16], new_dst_epid} : s_axis_tdata));
  assign tmp_tlast  = s_axis_tlast;
  assign tmp_tvalid = s_axis_tvalid;
  assign s_axis_tready = lossy || tmp_tready;

  axi_fifo #(.WIDTH(CHDR_W+1), .SIZE(1)) out_fifo (
    .clk(clk), .reset(rst), .clear(1'b0),
    .i_tdata({tmp_tlast, tmp_tdata}), .i_tvalid(tmp_tvalid), .i_tready(tmp_tready),
    .o_tdata({m_axis_tlast, m_axis_tdata}), .o_tvalid(m_axis_tvalid), .o_tready(m_axis_tready),
    .space(), .occupied()
  );

endmodule