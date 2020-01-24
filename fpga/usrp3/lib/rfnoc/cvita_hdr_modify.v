//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Modifies CVITA packet header fields

module cvita_hdr_modify (
  input  [127:0] header_in,
  output [127:0] header_out,
  input use_pkt_type,       input [1:0] pkt_type,
  input use_has_time,       input has_time,
  input use_eob,            input eob,
  input use_seqnum,         input [11:0] seqnum,
  input use_length,         input [15:0] length,
  input use_payload_length, input [15:0] payload_length,
  input use_src_sid,        input [15:0] src_sid,
  input use_dst_sid,        input [15:0] dst_sid,
  input use_vita_time,      input [63:0] vita_time
);

  wire [15:0] length_adj = payload_length + (header_out[125] /* Has time */ ? 16'd16 : 16'd8);

  assign header_out = {
    (use_pkt_type       == 1'b1) ? pkt_type   : header_in[127:126],
    (use_has_time       == 1'b1) ? has_time   : header_in[125],
    (use_eob            == 1'b1) ? eob        : header_in[124],
    (use_seqnum         == 1'b1) ? seqnum     : header_in[123:112],
    (use_length         == 1'b1) ? length     :
    (use_payload_length == 1'b1) ? length_adj : header_in[111:96],
    (use_src_sid        == 1'b1) ? src_sid    : header_in[95:80],
    (use_dst_sid        == 1'b1) ? dst_sid    : header_in[79:64],
    (use_vita_time      == 1'b1) ? vita_time  : header_in[63:0]};

endmodule
