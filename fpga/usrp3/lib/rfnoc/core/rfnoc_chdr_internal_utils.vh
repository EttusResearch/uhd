//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

// =============================================================
//  Stream Status Bitfields
// =============================================================

// -----------------------
//  Line 0
// -----------------------
// Bits     Name            Meaning
// ----     ----            -------
// 63:24    capacity_bytes  Downstream buffer capacity in bytes
// 23:20    <reserved>
// 19:16    status          Stream status code (enumeration)
// 15:0     src_epid        Endpoint ID of the source of this msg

// -----------------------
//  Line 1
// -----------------------
// Bits     Name            Meaning
// ----     ----            -------
// 63:24    xfercnt_pkts    Transfer count in packets
// 23:0     capacity_pkts   Downstream buffer capacity in packets

// -----------------------
//  Line 2
// -----------------------
// Bits     Name            Meaning
// ----     ----            -------
// 63:0     xfercnt_bytes   Transfer count in bytes

// -----------------------
//  Line 3
// -----------------------
// Bits     Name            Meaning
// ----     ----            -------
// 63:16    status_info     Extended information about status (diagnostic only)
// 15:0     buff_info       Extended information about buffer state (diagnostic only)

localparam [3:0] CHDR_STRS_STATUS_OKAY    = 4'd0; // No error
localparam [3:0] CHDR_STRS_STATUS_CMDERR  = 4'd1; // Cmd execution failed
localparam [3:0] CHDR_STRS_STATUS_SEQERR  = 4'd2; // Sequence number discontinuity
localparam [3:0] CHDR_STRS_STATUS_DATAERR = 4'd3; // Data integrity check failed
localparam [3:0] CHDR_STRS_STATUS_RTERR   = 4'd4; // Unexpected destination

// 64-bit fields
function [39:0] chdr64_strs_get_capacity_bytes(input [63:0] header);
  chdr64_strs_get_capacity_bytes = header[63:24];
endfunction

function [3:0] chdr64_strs_get_status(input [63:0] header);
  chdr64_strs_get_status = header[19:16];
endfunction

function [15:0] chdr64_strs_get_src_epid(input [63:0] header);
  chdr64_strs_get_src_epid = header[15:0];
endfunction

function [39:0] chdr64_strs_get_xfercnt_pkts(input [63:0] header);
  chdr64_strs_get_xfercnt_pkts = header[63:24];
endfunction

function [23:0] chdr64_strs_get_capacity_pkts(input [63:0] header);
  chdr64_strs_get_capacity_pkts = header[23:0];
endfunction

function [63:0] chdr64_strs_get_xfercnt_bytes(input [63:0] header);
  chdr64_strs_get_xfercnt_bytes = header[63:0];
endfunction

function [47:0] chdr64_strs_get_status_info(input [63:0] header);
  chdr64_strs_get_status_info = header[63:16];
endfunction

function [15:0] chdr64_strs_get_buff_info(input [63:0] header);
  chdr64_strs_get_buff_info = header[15:0];
endfunction


// 128-bit fields
function [39:0] chdr128_strs_get_capacity_bytes(input [127:0] header);
  chdr128_strs_get_capacity_bytes = chdr64_strs_get_capacity_bytes(header[63:0]);
endfunction

function [3:0] chdr128_strs_get_status(input [127:0] header);
  chdr128_strs_get_status = chdr64_strs_get_status(header[63:0]);
endfunction

function [15:0] chdr128_strs_get_src_epid(input [127:0] header);
  chdr128_strs_get_src_epid = chdr64_strs_get_src_epid(header[63:0]);
endfunction

function [23:0] chdr128_strs_get_capacity_pkts(input [127:0] header);
  chdr128_strs_get_capacity_pkts = chdr64_strs_get_capacity_pkts(header[127:64]);
endfunction

function [39:0] chdr128_strs_get_xfercnt_pkts(input [127:0] header);
  chdr128_strs_get_xfercnt_pkts = chdr64_strs_get_xfercnt_pkts(header[127:64]);
endfunction

function [63:0] chdr128_strs_get_xfercnt_bytes(input [127:0] header);
  chdr128_strs_get_xfercnt_bytes = chdr64_strs_get_xfercnt_bytes(header[63:0]);
endfunction

function [47:0] chdr128_strs_get_status_info(input [127:0] header);
  chdr128_strs_get_status_info = chdr64_strs_get_status_info(header[127:64]);
endfunction

function [15:0] chdr128_strs_get_buff_info(input [127:0] header);
  chdr128_strs_get_buff_info = chdr64_strs_get_buff_info(header[127:64]);
endfunction


// 256-bit fields
function [39:0] chdr256_strs_get_capacity_bytes(input [255:0] header);
  chdr256_strs_get_capacity_bytes = chdr64_strs_get_capacity_bytes(header[63:0]);
endfunction

function [3:0] chdr256_strs_get_status(input [255:0] header);
  chdr256_strs_get_status = chdr64_strs_get_status(header[63:0]);
endfunction

function [15:0] chdr256_strs_get_src_epid(input [255:0] header);
  chdr256_strs_get_src_epid = chdr64_strs_get_src_epid(header[63:0]);
endfunction

function [23:0] chdr256_strs_get_capacity_pkts(input [255:0] header);
  chdr256_strs_get_capacity_pkts = chdr64_strs_get_capacity_pkts(header[127:64]);
endfunction

function [39:0] chdr256_strs_get_xfercnt_pkts(input [255:0] header);
  chdr256_strs_get_xfercnt_pkts = chdr64_strs_get_xfercnt_pkts(header[127:64]);
endfunction

function [63:0] chdr256_strs_get_xfercnt_bytes(input [255:0] header);
  chdr256_strs_get_xfercnt_bytes = chdr64_strs_get_xfercnt_bytes(header[191:128]);
endfunction

function [47:0] chdr256_strs_get_status_info(input [255:0] header);
  chdr256_strs_get_status_info = chdr64_strs_get_status_info(header[255:192]);
endfunction

function [15:0] chdr256_strs_get_buff_info(input [255:0] header);
  chdr256_strs_get_buff_info = chdr64_strs_get_buff_info(header[255:192]);
endfunction

// Stream Status Setter Functions
//

// 64-bit fields
function [63:0] chdr64_strs_build_w0(
  input [39:0] capacity_bytes,
  input [3:0]  status,
  input [15:0] src_epid
);
  chdr64_strs_build_w0 = {capacity_bytes, 4'h0, status, src_epid};
endfunction

function [63:0] chdr64_strs_build_w1(
  input [39:0] xfercnt_pkts,
  input [23:0] capacity_pkts
);
  chdr64_strs_build_w1 = {xfercnt_pkts, capacity_pkts};
endfunction

function [63:0] chdr64_strs_build_w2(
  input [63:0] xfercnt_bytes
);
  chdr64_strs_build_w2 = xfercnt_bytes;
endfunction

function [63:0] chdr64_strs_build_w3(
  input [47:0] status_info,
  input [15:0] buff_info
);
  chdr64_strs_build_w3 = {status_info, buff_info};
endfunction

// 128-bit fields
function [127:0] chdr128_strs_build_w0(
  input [39:0] xfercnt_pkts,
  input [23:0] capacity_pkts,
  input [39:0] capacity_bytes,
  input [3:0]  status,
  input [15:0] src_epid
);
  chdr128_strs_build_w0 = {
    chdr64_strs_build_w1(xfercnt_pkts, capacity_pkts),
    chdr64_strs_build_w0(capacity_bytes, status, src_epid)};
endfunction

function [127:0] chdr128_strs_build_w1(
  input [47:0] status_info,
  input [15:0] buff_info,
  input [63:0] xfercnt_bytes
);
  chdr128_strs_build_w1 = {
    chdr64_strs_build_w3(status_info, buff_info),
    chdr64_strs_build_w2(xfercnt_bytes)};
endfunction

// 256-bit fields
function [255:0] chdr256_strs_build(
  input [47:0] status_info,
  input [15:0] buff_info,
  input [63:0] xfercnt_bytes,
  input [39:0] xfercnt_pkts,
  input [23:0] capacity_pkts,
  input [39:0] capacity_bytes,
  input [3:0]  status,
  input [15:0] src_epid
);
  chdr256_strs_build = {
    chdr64_strs_build_w3(status_info, buff_info),
    chdr64_strs_build_w2(xfercnt_bytes),
    chdr64_strs_build_w1(xfercnt_pkts, capacity_pkts),
    chdr64_strs_build_w0(capacity_bytes, status, src_epid)};
endfunction

// =============================================================
//  Stream Command Bitfields
// =============================================================

// -----------------------
//  Line 0
// -----------------------
// Bits     Name            Meaning
// ----     ----            -------
// 63:24    num_pkts        Downstream buffer capacity in bytes
// 23:20    op_data         Payload for command
// 19:16    op_code         Command operation code (enumeration)
// 15:0     src_epid        Endpoint ID of the source of this msg

// -----------------------
//  Line 1
// -----------------------
// Bits     Name            Meaning
// ----     ----            -------
// 63:0     num_bytes       Transfer count in packets

localparam [3:0] CHDR_STRC_OPCODE_INIT    = 4'd0;
localparam [3:0] CHDR_STRC_OPCODE_PING    = 4'd1;
localparam [3:0] CHDR_STRC_OPCODE_RESYNC  = 4'd2;

// 64-bit fields
function [39:0] chdr64_strc_get_num_pkts(input [63:0] header);
  chdr64_strc_get_num_pkts = header[63:24];
endfunction

function [3:0] chdr64_strc_get_op_data(input [63:0] header);
  chdr64_strc_get_op_data = header[23:20];
endfunction

function [3:0] chdr64_strc_get_op_code(input [63:0] header);
  chdr64_strc_get_op_code = header[19:16];
endfunction

function [15:0] chdr64_strc_get_src_epid(input [63:0] header);
  chdr64_strc_get_src_epid = header[15:0];
endfunction

function [63:0] chdr64_strc_get_num_bytes(input [63:0] header);
  chdr64_strc_get_num_bytes = header[63:0];
endfunction

// 128-bit fields
function [39:0] chdr128_strc_get_num_pkts(input [127:0] header);
  chdr128_strc_get_num_pkts = chdr64_strc_get_num_pkts(header[63:0]);
endfunction

function [3:0] chdr128_strc_get_op_data(input [127:0] header);
  chdr128_strc_get_op_data = chdr64_strc_get_op_data(header[63:0]);
endfunction

function [3:0] chdr128_strc_get_op_code(input [127:0] header);
  chdr128_strc_get_op_code = chdr64_strc_get_op_code(header[63:0]);
endfunction

function [15:0] chdr128_strc_get_src_epid(input [127:0] header);
  chdr128_strc_get_src_epid = chdr64_strc_get_src_epid(header[63:0]);
endfunction

function [63:0] chdr128_strc_get_num_bytes(input [127:0] header);
  chdr128_strc_get_num_bytes = chdr64_strc_get_num_bytes(header[127:64]);
endfunction

// Stream Command Setter Functions
//

// 64-bit fields

function [63:0] chdr64_strc_build_w0(
  input [39:0] num_pkts,
  input [3:0]  op_data,
  input [3:0]  op_code,
  input [15:0] src_epid
);
  chdr64_strc_build_w0 = {num_pkts, op_data, op_code, src_epid};
endfunction

function [63:0] chdr64_strc_build_w1(
  input [63:0] num_bytes
);
  chdr64_strc_build_w1 = num_bytes;
endfunction

// 128-bit fields
function [127:0] chdr128_strc_build(
  input [63:0] num_bytes,
  input [39:0] num_pkts,
  input [3:0]  op_data,
  input [3:0]  op_code,
  input [15:0] src_epid
);
  chdr128_strc_build = {
    chdr64_strc_build_w1(num_bytes),
    chdr64_strc_build_w0(num_pkts, op_data, op_code, src_epid)};
endfunction

// =============================================================
//  Management Packet Bitfields
// =============================================================

// -----------------------
//  HDR
// -----------------------
// Bits     Name          Meaning
// ----     ----          -------
// 63:48    proto_ver     Protocol Version
// 47:45    chdr_w        Bitwidth of the CHDR interface
// 44:26    <Reserved>
// 25:16    num_hops      Number of hops that this message will take (TTL)
// 15:0     src_epid      Endpoint ID of the source of this msg

// -----------------------
//  OP
// -----------------------
// Bits     Name          Meaning
// ----     ----          -------
// 63:16    op_payload    Operation Payload
// 15:8     op_code       Operation code
// 7:0      ops_pending   Number of operations pending in this hop

localparam [2:0] CHDR_MGMT_WIDTH_64  = 3'd0;
localparam [2:0] CHDR_MGMT_WIDTH_128 = 3'd1;
localparam [2:0] CHDR_MGMT_WIDTH_256 = 3'd2;
localparam [2:0] CHDR_MGMT_WIDTH_512 = 3'd3;

function [2:0] chdr_w_to_enum(input integer bits);
  if (bits == 512)
    chdr_w_to_enum = CHDR_MGMT_WIDTH_512;
  else if (bits == 256)
    chdr_w_to_enum = CHDR_MGMT_WIDTH_256;
  else if (bits == 128)
    chdr_w_to_enum = CHDR_MGMT_WIDTH_128;
  else
    chdr_w_to_enum = CHDR_MGMT_WIDTH_64;
endfunction

localparam [7:0] CHDR_MGMT_OP_NOP         = 8'd0;
localparam [7:0] CHDR_MGMT_OP_ADVERTISE   = 8'd1;
localparam [7:0] CHDR_MGMT_OP_SEL_DEST    = 8'd2;
localparam [7:0] CHDR_MGMT_OP_RETURN      = 8'd3;
localparam [7:0] CHDR_MGMT_OP_INFO_REQ    = 8'd4;
localparam [7:0] CHDR_MGMT_OP_INFO_RESP   = 8'd5;
localparam [7:0] CHDR_MGMT_OP_CFG_WR_REQ  = 8'd6;
localparam [7:0] CHDR_MGMT_OP_CFG_RD_REQ  = 8'd7;
localparam [7:0] CHDR_MGMT_OP_CFG_RD_RESP = 8'd8;

function [15:0] chdr_mgmt_get_proto_ver(input [63:0] header);
  chdr_mgmt_get_proto_ver = header[63:48];
endfunction

function [2:0] chdr_mgmt_get_chdr_w(input [63:0] header);
  chdr_mgmt_get_chdr_w = header[47:45];
endfunction

function [9:0] chdr_mgmt_get_num_hops(input [63:0] header);
  chdr_mgmt_get_num_hops = header[25:16];
endfunction

function [15:0] chdr_mgmt_get_src_epid(input [63:0] header);
  chdr_mgmt_get_src_epid = header[15:0];
endfunction

function [47:0] chdr_mgmt_get_op_payload(input [63:0] header);
  chdr_mgmt_get_op_payload = header[63:16];
endfunction

function [7:0] chdr_mgmt_get_op_code(input [63:0] header);
  chdr_mgmt_get_op_code = header[15:8];
endfunction

function [7:0] chdr_mgmt_get_ops_pending(input [63:0] header);
  chdr_mgmt_get_ops_pending = header[7:0];
endfunction

function [63:0] chdr_mgmt_build_hdr(
  input [15:0] proto_ver,
  input [2:0]  chdr_w,
  input [9:0]  num_hops,
  input [15:0] src_epid
);
  chdr_mgmt_build_hdr = {proto_ver, chdr_w, 19'h0, num_hops, src_epid};
endfunction

function [63:0] chdr_mgmt_build_op(
  input [47:0] op_payload,
  input [7:0]  op_code,
  input [7:0]  ops_pending
);
  chdr_mgmt_build_op = {op_payload, op_code, ops_pending};
endfunction

// Definition for the TID field for the output of chdr_mgmt_pkt_handler
localparam [1:0] CHDR_MGMT_ROUTE_EPID    = 2'd0;  // Route based on EPID
localparam [1:0] CHDR_MGMT_ROUTE_TDEST   = 2'd1;  // Route based on tdest field
localparam [1:0] CHDR_MGMT_RETURN_TO_SRC = 2'd2;  // Return packet to sender

// -----------------------
//  OP specific fields
// -----------------------

localparam [3:0] NODE_TYPE_INVALID   = 4'd0;
localparam [3:0] NODE_TYPE_XBAR      = 4'd1;
localparam [3:0] NODE_TYPE_STREAM_EP = 4'd2;
localparam [3:0] NODE_TYPE_TRANSPORT = 4'd3;

function [47:0] chdr_mgmt_build_node_info(
  input [17:0] ext_info,
  input [9:0]  node_inst,
  input [3:0]  node_type,
  input [15:0] device_id
);
  chdr_mgmt_build_node_info = {ext_info, node_inst, node_type, device_id};
endfunction

function [9:0] chdr_mgmt_sel_dest_get_tdest(input [47:0] payload);
  chdr_mgmt_sel_dest_get_tdest = payload[9:0];
endfunction

function [15:0] chdr_mgmt_cfg_reg_get_addr(input [47:0] payload);
  chdr_mgmt_cfg_reg_get_addr = payload[15:0];
endfunction

function [31:0] chdr_mgmt_cfg_reg_get_data(input [47:0] payload);
  chdr_mgmt_cfg_reg_get_data = payload[47:16];
endfunction
