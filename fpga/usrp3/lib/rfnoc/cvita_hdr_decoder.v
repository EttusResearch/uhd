//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Decoder header word into CVITA header fields

module cvita_hdr_decoder (
  input [127:0] header,
  output [1:0] pkt_type, output eob, output has_time,
  output [11:0] seqnum, output [15:0] length, output [15:0] payload_length,
  output [15:0] src_sid, output [15:0] dst_sid,
  output [63:0] vita_time
);

  wire [63:0] hdr[0:1];
  assign hdr[0] = header[127:64];
  assign hdr[1] = header[63:0];

  assign pkt_type  = hdr[0][63:62];
  assign has_time  = hdr[0][61];
  assign eob       = hdr[0][60];
  assign seqnum    = hdr[0][59:48];
  assign length    = hdr[0][47:32];
  assign src_sid   = hdr[0][31:16];
  assign dst_sid   = hdr[0][15:0];
  assign vita_time = hdr[1];

  assign payload_length = has_time ? length - 16'd16 : length - 16'd8;

endmodule