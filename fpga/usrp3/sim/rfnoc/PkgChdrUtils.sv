//
// Copyright 2025 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: PkgChdrData
//
// Description:
//
//   This package exists for backwards compatibility with older testbenches
//   that reference it. It imports then exports following:
//
//     rfnoc_chdr_utils_pkg::*
//     PkgChdrData::ChdrData
//     PkgChdrBfm::chdr_mgmt_t
//
//   New testbenches should reference these directly. These were reorganized
//   because most of them synthesizable and not for simulation only.
//


package PkgChdrUtils;

  import rfnoc_chdr_utils_pkg::*;
  export rfnoc_chdr_utils_pkg::chdr_pkt_type_t;
  export rfnoc_chdr_utils_pkg::CHDR_MANAGEMENT;
  export rfnoc_chdr_utils_pkg::CHDR_STRM_STATUS;
  export rfnoc_chdr_utils_pkg::CHDR_STRM_CMD;
  export rfnoc_chdr_utils_pkg::CHDR_RESERVED_0;
  export rfnoc_chdr_utils_pkg::CHDR_CONTROL;
  export rfnoc_chdr_utils_pkg::CHDR_RESERVED_1;
  export rfnoc_chdr_utils_pkg::CHDR_DATA_NO_TS;
  export rfnoc_chdr_utils_pkg::CHDR_DATA_WITH_TS;
  export rfnoc_chdr_utils_pkg::chdr_vc_t;
  export rfnoc_chdr_utils_pkg::chdr_eob_t;
  export rfnoc_chdr_utils_pkg::chdr_eov_t;
  export rfnoc_chdr_utils_pkg::chdr_num_mdata_t;
  export rfnoc_chdr_utils_pkg::chdr_seq_num_t;
  export rfnoc_chdr_utils_pkg::chdr_length_t;
  export rfnoc_chdr_utils_pkg::chdr_epid_t;
  export rfnoc_chdr_utils_pkg::chdr_timestamp_t;
  export rfnoc_chdr_utils_pkg::chdr_context_type_t;
  export rfnoc_chdr_utils_pkg::ctrl_word_t;
  export rfnoc_chdr_utils_pkg::ctrl_opcode_t;
  export rfnoc_chdr_utils_pkg::CTRL_OP_SLEEP;
  export rfnoc_chdr_utils_pkg::CTRL_OP_WRITE;
  export rfnoc_chdr_utils_pkg::CTRL_OP_READ;
  export rfnoc_chdr_utils_pkg::CTRL_OP_WRITE_READ;
  export rfnoc_chdr_utils_pkg::CTRL_OP_RESERVED_0;
  export rfnoc_chdr_utils_pkg::CTRL_OP_RESERVED_1;
  export rfnoc_chdr_utils_pkg::CTRL_OP_RESERVED_2;
  export rfnoc_chdr_utils_pkg::CTRL_OP_RESERVED_3;
  export rfnoc_chdr_utils_pkg::CTRL_OP_RESERVED_4;
  export rfnoc_chdr_utils_pkg::CTRL_OP_RESERVED_5;
  export rfnoc_chdr_utils_pkg::ctrl_status_t;
  export rfnoc_chdr_utils_pkg::CTRL_STS_OKAY;
  export rfnoc_chdr_utils_pkg::CTRL_STS_CMDERR;
  export rfnoc_chdr_utils_pkg::CTRL_STS_TSERR;
  export rfnoc_chdr_utils_pkg::CTRL_STS_WARNING;
  export rfnoc_chdr_utils_pkg::ctrl_seq_num_t;
  export rfnoc_chdr_utils_pkg::ctrl_num_data_t;
  export rfnoc_chdr_utils_pkg::ctrl_port_t;
  export rfnoc_chdr_utils_pkg::ctrl_byte_en_t;
  export rfnoc_chdr_utils_pkg::ctrl_address_t;
  export rfnoc_chdr_utils_pkg::chdr_strs_status_t;
  export rfnoc_chdr_utils_pkg::STRS_OKAY;
  export rfnoc_chdr_utils_pkg::STRS_CMDERR;
  export rfnoc_chdr_utils_pkg::STRS_SEQERR;
  export rfnoc_chdr_utils_pkg::STRS_DATAERR;
  export rfnoc_chdr_utils_pkg::STRS_RTERR;
  export rfnoc_chdr_utils_pkg::chdr_strc_opcode_t;
  export rfnoc_chdr_utils_pkg::STRC_INIT;
  export rfnoc_chdr_utils_pkg::STRC_PING;
  export rfnoc_chdr_utils_pkg::STRC_RESYNC;
  export rfnoc_chdr_utils_pkg::chdr_mgmt_width_t;
  export rfnoc_chdr_utils_pkg::CHDR_W_64;
  export rfnoc_chdr_utils_pkg::CHDR_W_128;
  export rfnoc_chdr_utils_pkg::CHDR_W_256;
  export rfnoc_chdr_utils_pkg::CHDR_W_512;
  export rfnoc_chdr_utils_pkg::CHDR_W_INVALID;
  export rfnoc_chdr_utils_pkg::translate_chdr_w;
  export rfnoc_chdr_utils_pkg::translate_bit_w;
  export rfnoc_chdr_utils_pkg::chdr_mgmt_opcode_t;
  export rfnoc_chdr_utils_pkg::MGMT_OP_NOP;
  export rfnoc_chdr_utils_pkg::MGMT_OP_ADVERTISE;
  export rfnoc_chdr_utils_pkg::MGMT_OP_SEL_DEST;
  export rfnoc_chdr_utils_pkg::MGMT_OP_RETURN;
  export rfnoc_chdr_utils_pkg::MGMT_OP_INFO_REQ;
  export rfnoc_chdr_utils_pkg::MGMT_OP_INFO_RESP;
  export rfnoc_chdr_utils_pkg::MGMT_OP_CFG_WR_REQ;
  export rfnoc_chdr_utils_pkg::MGMT_OP_CFG_RD_REQ;
  export rfnoc_chdr_utils_pkg::MGMT_OP_CFG_RD_RESP;
  export rfnoc_chdr_utils_pkg::chdr_header_t;
  export rfnoc_chdr_utils_pkg::axis_ctrl_header_t;
  export rfnoc_chdr_utils_pkg::ctrl_op_word_t;
  export rfnoc_chdr_utils_pkg::chdr_ctrl_header_t;
  export rfnoc_chdr_utils_pkg::chdr_str_status_t;
  export rfnoc_chdr_utils_pkg::chdr_str_command_t;
  export rfnoc_chdr_utils_pkg::chdr_mgmt_header_t;
  export rfnoc_chdr_utils_pkg::chdr_mgmt_op_t;

  import PkgChdrData::*;
  export PkgChdrData::ChdrData;

  import PkgChdrBfm::*;
  export PkgChdrBfm::chdr_mgmt_t;

endpackage : PkgChdrUtils