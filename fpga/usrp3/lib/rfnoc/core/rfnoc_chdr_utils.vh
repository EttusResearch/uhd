//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

// =============================================================
//  CHDR Bitfields
// =============================================================
//
// The Condensed Hierarchical Datagram for RFNoC (CHDR) is 
// a protocol that defines the fundamental unit of data transfer
// in an RFNoC network. 
// 
// -----------------------
//  Header
// -----------------------
// Bits     Name       Meaning
// ----     ----       -------
// 63:58    vc         Virtual Channel
// 57       eob        End of Burst Delimiter
// 56       eov        End of Vector Delimiter
// 55:53    pkt_type   Packet Type (enumeration)
// 52:48    num_mdata  Number of lines of metadata
// 47:32    seq_num    Sequence number for the packet
// 31:16    length     Length of the datagram in bytes
// 15:0     dst_epid   Destination Endpoint ID
// 
// Field: Packet Type
// -----------------------
// 3'd0     Management
// 3'd1     Stream Status
// 3'd2     Stream Command
// 3'd3     <Reserved>
// 3'd4     Control Transaction
// 3'd5     <Reserved>
// 3'd6     Data (without timestamp)
// 3'd7     Data (with timestamp)
//

// Special CHDR Values
//

// Packet Type
localparam [2:0] CHDR_PKT_TYPE_MGMT    = 3'd0;
localparam [2:0] CHDR_PKT_TYPE_STRS    = 3'd1;
localparam [2:0] CHDR_PKT_TYPE_STRC    = 3'd2;
//localparam [2:0] RESERVED            = 3'd3;
localparam [2:0] CHDR_PKT_TYPE_CTRL    = 3'd4;
//localparam [2:0] RESERVED            = 3'd5;
localparam [2:0] CHDR_PKT_TYPE_DATA    = 3'd6;
localparam [2:0] CHDR_PKT_TYPE_DATA_TS = 3'd7;

// Metadata
localparam [4:0] CHDR_NO_MDATA = 5'd0;

// EPID
localparam [15:0] NULL_EPID = 16'd0;

// CHDR Getter Functions
//
function [5:0] chdr_get_vc(input [63:0] header);
  chdr_get_vc = header[63:58];
endfunction

function [0:0] chdr_get_eob(input [63:0] header);
  chdr_get_eob = header[57];
endfunction

function [0:0] chdr_get_eov(input [63:0] header);
  chdr_get_eov = header[56];
endfunction

function [2:0] chdr_get_pkt_type(input [63:0] header);
  chdr_get_pkt_type = header[55:53];
endfunction

function [4:0] chdr_get_num_mdata(input [63:0] header);
  chdr_get_num_mdata = header[52:48];
endfunction

function [15:0] chdr_get_seq_num(input [63:0] header);
  chdr_get_seq_num = header[47:32];
endfunction

function [15:0] chdr_get_length(input [63:0] header);
  chdr_get_length = header[31:16];
endfunction

function [15:0] chdr_get_dst_epid(input [63:0] header);
  chdr_get_dst_epid = header[15:0];
endfunction

// CHDR Setter Functions
//
function [63:0] chdr_build_header(
  input [5:0]  vc,
  input [0:0]  eob,
  input [0:0]  eov,
  input [2:0]  pkt_type,
  input [4:0]  num_mdata,
  input [15:0] seq_num,
  input [15:0] length,
  input [15:0] dst_epid
);
  chdr_build_header = {vc, eob, eov, pkt_type, num_mdata, seq_num, length, dst_epid};
endfunction

function [63:0] chdr_set_vc(
  input [63:0] base_hdr,
  input [5:0]  vc
);
  chdr_set_vc = {vc, base_hdr[57:0]};
endfunction

function [63:0] chdr_set_eob(
  input [63:0] base_hdr,
  input [0:0]  eob
);
  chdr_set_eob = {base_hdr[63:58], eob, base_hdr[56:0]};
endfunction

function [63:0] chdr_set_eov(
  input [63:0] base_hdr,
  input [0:0]  eov
);
  chdr_set_eov = {base_hdr[63:57], eov, base_hdr[55:0]};
endfunction

function [63:0] chdr_set_delims(
  input [63:0] base_hdr,
  input [0:0]  eob,
  input [0:0]  eov
);
  chdr_set_delims = {base_hdr[63:58], eob, eov, base_hdr[55:0]};
endfunction

function [63:0] chdr_set_pkt_type(
  input [63:0] base_hdr,
  input [2:0]  pkt_type
);
  chdr_set_pkt_type = {base_hdr[63:56], pkt_type, base_hdr[52:0]};
endfunction

function [63:0] chdr_set_num_mdata(
  input [63:0] base_hdr,
  input [4:0]  num_mdata
);
  chdr_set_num_mdata = {base_hdr[63:53], num_mdata, base_hdr[47:0]};
endfunction

function [63:0] chdr_set_seq_num(
  input [63:0] base_hdr,
  input [15:0] seq_num
);
  chdr_set_seq_num = {base_hdr[63:48], seq_num, base_hdr[31:0]};
endfunction

function [63:0] chdr_set_length(
  input [63:0] base_hdr,
  input [15:0] length
);
  chdr_set_length = {base_hdr[63:32], length, base_hdr[15:0]};
endfunction

function [63:0] chdr_set_dst_epid(
  input [63:0] base_hdr,
  input [15:0] dst_epid
);
  chdr_set_dst_epid = {base_hdr[63:16], dst_epid};
endfunction

// =============================================================
//  Data Packet Specific
// =============================================================

localparam [3:0] CONTEXT_FIELD_HDR    = 4'd0;
localparam [3:0] CONTEXT_FIELD_HDR_TS = 4'd1;
localparam [3:0] CONTEXT_FIELD_TS     = 4'd2;
localparam [3:0] CONTEXT_FIELD_MDATA  = 4'd3;

function [0:0] chdr_get_has_time(input [63:0] header);
  chdr_get_has_time = (chdr_get_pkt_type(header) == CHDR_PKT_TYPE_DATA_TS);
endfunction

// Calculate the payload length in bytes based on the CHDR_W and header
function [15:0] chdr_calc_payload_length(input [31:0] chdr_w, input [63:0] header);
  reg [15:0] payload_length, mdata_length, header_length;
  begin
    if (chdr_w == 64) begin
      header_length = chdr_get_has_time(header) ? 2*(chdr_w/8) : (chdr_w/8);
    end else begin
      header_length = chdr_w/8;
    end
    mdata_length   = chdr_get_num_mdata(header) * (chdr_w/8);
    payload_length = chdr_get_length(header) - mdata_length - header_length;

    chdr_calc_payload_length = payload_length;
  end
endfunction
