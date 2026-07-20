//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

// =============================================================
//  AXIS-Ctrl Bitfields
// =============================================================

// -----------------------
//  Line 0: HDR_0
// -----------------------
// Bits     Name          Meaning
// ----     ----          -------
// 31:16    rem_dst_epid  Endpoint ID of the remote dest of this msg
// 15       is_ack        Is this an acknowledgment to a transaction?
// 14       has_time      Does the transaction have a timestamp?
// 13:10    num_data      Number of data words
// 9:0      dst_port      Ctrl XB port that the destination block is on

// -----------------------
//  Line 1: HDR_1
// -----------------------
// Bits     Name          Meaning
// ----     ----          -------
// 31:28    req_size      Number of data words requested (Read/Block Read)
// 27:20    seq_num       Sequence number
// 19:10    rem_dst_port  Ctrl XB port that the remote dest block is on
// 9:0      src_port      Ctrl XB port that the source block is on

// -----------------------
//  Line 2: TS_LO (Optional)
// -----------------------
// Bits     Name       Meaning
// ----     ----       -------
// 31:0     timestamp  Lower 32 bits of the timestamp

// -----------------------
//  Line 3: TS_HI (Optional)
// -----------------------
// Bits     Name       Meaning
// ----     ----       -------
// 31:0     timestamp  Upper 32 bits of the timestamp

// -----------------------
//  Line 4: OP Word
// -----------------------
// Bits     Name       Meaning
// ----     ----       -------
// 31:30    status     The status of the ack
// 29:28    <Reserved>
// 27:24    opcode     Operation Code
// 23:20    byte_en    Byte enable strobe
// 19:0     address    Address for transaction

// AXIS-Ctrl Status
//
localparam [1:0] AXIS_CTRL_STS_OKAY    = 2'b00;
localparam [1:0] AXIS_CTRL_STS_CMDERR  = 2'b01;
localparam [1:0] AXIS_CTRL_STS_TSERR   = 2'b10;
localparam [1:0] AXIS_CTRL_STS_WARNING = 2'b11;

// AXIS-Ctrl Opcode Definitions
//
localparam [3:0] AXIS_CTRL_OPCODE_SLEEP       = 4'd0;
localparam [3:0] AXIS_CTRL_OPCODE_WRITE       = 4'd1;
localparam [3:0] AXIS_CTRL_OPCODE_READ        = 4'd2;
localparam [3:0] AXIS_CTRL_OPCODE_READ_WRITE  = 4'd3;
localparam [3:0] AXIS_CTRL_OPCODE_BLOCK_WRITE = 4'd4;
localparam [3:0] AXIS_CTRL_OPCODE_BLOCK_READ  = 4'd5;
localparam [3:0] AXIS_CTRL_OPCODE_POLL        = 4'd6;

// AXIS-Ctrl Getter Functions
//
function [0:0] axis_ctrl_get_is_ack(input [31:0] header);
  axis_ctrl_get_is_ack = header[15];
endfunction

function [0:0] axis_ctrl_get_has_time(input [31:0] header);
  axis_ctrl_get_has_time = header[14];
endfunction

function [7:0] axis_ctrl_get_seq_num(input [31:0] header);
  axis_ctrl_get_seq_num = header[27:20];
endfunction

function [3:0] axis_ctrl_get_num_data(input [31:0] header);
  axis_ctrl_get_num_data = header[13:10];
endfunction

function [9:0] axis_ctrl_get_src_port(input [31:0] header);
  axis_ctrl_get_src_port = header[9:0];
endfunction

function [9:0] axis_ctrl_get_dst_port(input [31:0] header);
  axis_ctrl_get_dst_port = header[9:0];
endfunction

function [15:0] axis_ctrl_get_rem_dst_epid(input [31:0] header);
  axis_ctrl_get_rem_dst_epid = header[31:16];
endfunction

function [3:0] axis_ctrl_get_req_size(input [31:0] header);
  axis_ctrl_get_req_size = header[31:28];
endfunction

function [9:0] axis_ctrl_get_rem_dst_port(input [31:0] header);
  axis_ctrl_get_rem_dst_port = header[19:10];
endfunction

function [1:0] axis_ctrl_get_status(input [31:0] header);
  axis_ctrl_get_status = header[31:30];
endfunction

function [3:0] axis_ctrl_get_opcode(input [31:0] header);
  axis_ctrl_get_opcode = header[27:24];
endfunction

function [3:0] axis_ctrl_get_byte_en(input [31:0] header);
  axis_ctrl_get_byte_en = header[23:20];
endfunction

function [19:0] axis_ctrl_get_address(input [31:0] header);
  axis_ctrl_get_address = header[19:0];
endfunction

// AXIS-Ctrl Setter Functions
//
function [31:0] axis_ctrl_build_hdr_lo(
  input [15:0] rem_dst_epid,
  input [0:0]  is_ack,
  input [0:0]  has_time,
  input [3:0]  num_data,
  input [9:0]  dst_port
);
  axis_ctrl_build_hdr_lo = {rem_dst_epid, is_ack, has_time, num_data, dst_port};
endfunction

function [31:0] axis_ctrl_build_hdr_hi(
  input [3:0]  req_size,
  input [7:0]  seq_num,
  input [9:0]  rem_dst_port,
  input [9:0]  src_port
);
  axis_ctrl_build_hdr_hi = {req_size, seq_num, rem_dst_port, src_port};
endfunction

function [31:0] chdr_ctrl_build_hdr_lo(
  input [15:0] src_epid,
  input [0:0]  is_ack,
  input [0:0]  has_time,
  input [3:0]  num_data,
  input [9:0]  dst_port
);
  chdr_ctrl_build_hdr_lo = {src_epid, is_ack, has_time, num_data, dst_port};
endfunction

function [31:0] chdr_ctrl_build_hdr_hi(
  input [3:0]  req_size,
  input [7:0]  seq_num,
  input [9:0]  src_port
);
  chdr_ctrl_build_hdr_hi = {req_size, seq_num, 10'h0, src_port};
endfunction

function [31:0] axis_ctrl_build_op_word(
  input [1:0]  status,
  input [3:0]  opcode,
  input [3:0]  byte_en,
  input [19:0] address
);
  axis_ctrl_build_op_word = {status, 2'b00, opcode, byte_en, address};
endfunction
