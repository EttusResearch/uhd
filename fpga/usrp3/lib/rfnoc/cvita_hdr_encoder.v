//
// Copyright 2016 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Encodes CVITA packet header fields into a header word

module cvita_hdr_encoder (
  input [1:0] pkt_type, input eob, input has_time,
  input [11:0] seqnum,
  input [15:0] payload_length,
  input [15:0] src_sid, input [15:0] dst_sid,
  input [63:0] vita_time,
  output [127:0] header
);

  assign header = {pkt_type, has_time, eob, seqnum,
                   payload_length + (has_time ? 16'd16 : 16'd8),
                   src_sid, dst_sid, vita_time};

endmodule
