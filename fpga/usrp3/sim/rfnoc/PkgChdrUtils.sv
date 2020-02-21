//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: PkgChdrUtils
//
// Description: Various types, constants, and functions for interacting with 
// the RFNoC CHDR bus infrastructure.
//



package PkgChdrUtils;

  import PkgChdrData::*;


  //---------------------------------------------------------------------------
  // Type Definitions
  //---------------------------------------------------------------------------

  //----------------------
  // AXIS-CHDR Definitions
  //----------------------

  // Expose the CHDR word and item/sample data types and methods in the
  // ChdrData class. The width of these types is a class parameter. 
  export PkgChdrData::ChdrData;

  // CHDR header fields
  typedef enum bit [2:0] { 
    CHDR_MANAGEMENT   = 3'd0,
    CHDR_STRM_STATUS  = 3'd1,
    CHDR_STRM_CMD     = 3'd2,
    CHDR_RESERVED_0   = 3'd3,
    CHDR_CONTROL      = 3'd4,
    CHDR_RESERVED_1   = 3'd5,
    CHDR_DATA_NO_TS   = 3'd6,
    CHDR_DATA_WITH_TS = 3'd7
  } chdr_pkt_type_t;                    // CHDR Packet Type

  typedef bit [ 5:0] chdr_vc_t;         // CHDR Virtual Channel field
  typedef bit [ 0:0] chdr_eob_t;        // CHDR End of Burst field
  typedef bit [ 0:0] chdr_eov_t;        // CHDR End of Vector field
  typedef bit [ 4:0] chdr_num_mdata_t;  // CHDR Num Metadata field
  typedef bit [15:0] chdr_seq_num_t;    // CHDR SeqNum field
  typedef bit [15:0] chdr_length_t;     // CHDR Length field
  typedef bit [15:0] chdr_epid_t;       // CHDR EPID field
  typedef bit [63:0] chdr_timestamp_t;  // CHDR Timestamp field

  // CHDR Context Field Identifiers
  typedef enum bit [3:0] { 
    CONTEXT_FIELD_HDR    = 4'd0,
    CONTEXT_FIELD_HDR_TS = 4'd1,
    CONTEXT_FIELD_TS     = 4'd2,
    CONTEXT_FIELD_MDATA  = 4'd3
  } chdr_context_type_t;

  //----------------------
  // AXIS-Ctrl Definitions
  //----------------------

  // The fundamental unit of the AXIS-Ctrl (control) bus, which is always 32 bits
  typedef logic [31:0] ctrl_word_t;

  typedef enum bit [3:0] { 
    CTRL_OP_SLEEP      = 4'd0,
    CTRL_OP_WRITE      = 4'd1,
    CTRL_OP_READ       = 4'd2,
    CTRL_OP_WRITE_READ = 4'd3,
    CTRL_OP_RESERVED_0 = 4'd4,
    CTRL_OP_RESERVED_1 = 4'd5,
    CTRL_OP_RESERVED_2 = 4'd6,
    CTRL_OP_RESERVED_3 = 4'd7,
    CTRL_OP_RESERVED_4 = 4'd8,
    CTRL_OP_RESERVED_5 = 4'd9
  } ctrl_opcode_t;                    // Control OpCode Type

  typedef enum bit [1:0] { 
    CTRL_STS_OKAY    = 2'd0,
    CTRL_STS_CMDERR  = 2'd1,
    CTRL_STS_TSERR   = 2'd2,
    CTRL_STS_WARNING = 2'd3
  } ctrl_status_t;                    // Control OpCode Type

  typedef bit  [5:0] ctrl_seq_num_t;  // AXIS-Ctrl SeqNum field
  typedef bit  [3:0] ctrl_num_data_t; // AXIS-Ctrl NumData field
  typedef bit  [9:0] ctrl_port_t;     // AXIS-Ctrl source/destination port field
  typedef bit  [3:0] ctrl_byte_en_t;  // AXIS-Ctrl ByteEnable field
  typedef bit [19:0] ctrl_address_t;  // AXIS-Ctrl Address field

  //-------------------------------
  // CHDR Type-Specific Definitions
  //-------------------------------

  // CHDR Status packet fields
  typedef enum bit [3:0] {
    STRS_OKAY    = 4'd0,
    STRS_CMDERR  = 4'd1,
    STRS_SEQERR  = 4'd2,
    STRS_DATAERR = 4'd3,
    STRS_RTERR   = 4'd4
  } chdr_strs_status_t;  // CHDR stream status packet status field

  // CHDR Control packet fields
  typedef enum bit [3:0] {
    STRC_INIT    = 4'd0,
    STRC_PING    = 4'd1,
    STRC_RESYNC  = 4'd2
  } chdr_strc_opcode_t;  // CHDR stream command packet opcode filed

  // CHDR Management packet field
  typedef enum bit [2:0] {
    CHDR_W_64      = 3'd0,
    CHDR_W_128     = 3'd1,
    CHDR_W_256     = 3'd2,
    CHDR_W_512     = 3'd3,
    CHDR_W_INVALID = 3'd7
  } chdr_mgmt_width_t;   // CHDR management packet CHDR Width field

  function automatic chdr_mgmt_width_t translate_chdr_w(int bitwidth);
    case (bitwidth)
      64:      return CHDR_W_64;
      128:     return CHDR_W_128;
      256:     return CHDR_W_256;
      512:     return CHDR_W_512;
      default: return CHDR_W_INVALID;
    endcase
  endfunction : translate_chdr_w

  typedef enum bit [7:0] {
    MGMT_OP_NOP         = 8'd0,
    MGMT_OP_ADVERTISE   = 8'd1,
    MGMT_OP_SEL_DEST    = 8'd2,
    MGMT_OP_RETURN      = 8'd3,
    MGMT_OP_INFO_REQ    = 8'd4,
    MGMT_OP_INFO_RESP   = 8'd5,
    MGMT_OP_CFG_WR_REQ  = 8'd6,
    MGMT_OP_CFG_RD_REQ  = 8'd7,
    MGMT_OP_CFG_RD_RESP = 8'd8
  } chdr_mgmt_opcode_t;  // CHDR management packet OpCode field


  //---------------------------------------------------------------------------
  // Packet Data Structures
  //---------------------------------------------------------------------------

  // CHDR packet header
  typedef struct packed {
    chdr_vc_t        vc;
    chdr_eob_t       eob;
    chdr_eov_t       eov;
    chdr_pkt_type_t  pkt_type;
    chdr_num_mdata_t num_mdata;
    chdr_seq_num_t   seq_num;
    chdr_length_t    length;
    chdr_epid_t      dst_epid;
  } chdr_header_t;

  // AXIS-Ctrl packet header
  typedef struct packed {
    // Word 1
    bit [ 5:0]      _rsvd_0;
    ctrl_port_t     rem_dst_port;
    chdr_epid_t     rem_dst_epid;
    // Word 0
    bit             is_ack;
    bit             has_time;
    ctrl_seq_num_t  seq_num;
    ctrl_num_data_t num_data;
    ctrl_port_t     src_port;
    ctrl_port_t     dst_port;
  } axis_ctrl_header_t;

  // AXIS-Ctrl packet header
  typedef struct packed {
    ctrl_status_t status;
    bit [ 1:0]    _rsvd_0;
    ctrl_opcode_t op_code;
    bit [ 3:0]    byte_enable;
    bit [19:0]    address;
  } ctrl_op_word_t;

  // Ctrl packet header when in the payload of a CHDR packet
  typedef struct packed {
    bit [15:0]      _rsvd_0;
    chdr_epid_t     src_epid;
    bit             is_ack;
    bit             has_time;
    ctrl_seq_num_t  seq_num;
    ctrl_num_data_t num_data;
    ctrl_port_t     src_port;
    ctrl_port_t     dst_port;
  } chdr_ctrl_header_t;

  // CHDR stream status packet payload
  typedef struct packed {
    // Word 3
    bit [47:0]          status_info;
    bit [15:0]          buff_info;
    // Word 2
    bit [63:0]          xfer_count_bytes;
    // Word 1
    bit [39:0]          xfer_count_pkts;
    bit [23:0]          capacity_pkts;
    // Word 0
    bit [39:0]          capacity_bytes;
    bit [ 3:0]          _rsvd_0;
    chdr_strs_status_t  status;
    chdr_epid_t         src_epid;
  } chdr_str_status_t;

  // CHDR stream command packet payload
  typedef struct packed {
    // Word 1
    bit [63:0]          num_bytes;
    // Word 0
    bit [39:0]          num_pkts;
    bit [ 3:0]          op_data;
    chdr_strc_opcode_t  op_code;
    chdr_epid_t         src_epid;
  } chdr_str_command_t;

  // CHDR management packet header
  typedef struct packed {
    bit [15:0]          prot_ver;
    chdr_mgmt_width_t   chdr_width;
    bit [18:0]          _rsvd_0;
    bit [ 9:0]          num_hops;
    chdr_epid_t         src_epid;
  } chdr_mgmt_header_t;

  // CHDR management packet operation
  typedef struct packed {
    bit [47:0]          op_payload;
    chdr_mgmt_opcode_t  op_code;
    bit [ 7:0]          ops_pending;
  } chdr_mgmt_op_t;

  // CHDR management packet
  typedef struct {
    chdr_mgmt_header_t  header;
    chdr_mgmt_op_t      ops[$];
  } chdr_mgmt_t;


endpackage : PkgChdrUtils
