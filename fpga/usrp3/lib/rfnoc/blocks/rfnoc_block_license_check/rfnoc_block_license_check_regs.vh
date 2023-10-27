//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_license_check_regs (Header)
//
// Description:
//
//   This is a header file that contains the register descriptions for the
//   RFNoC License checker block.
//

//-----------------------------------------------------------------------------
// Register Space
//-----------------------------------------------------------------------------

localparam BASE_ADDR = 'h0;


//-----------------------------------------------------------------------------
// Replay Register Descriptions
//-----------------------------------------------------------------------------

// REG_COMPAT (R)
//
// Compatibility version. This read-only register is used by software to
// determine if this block's version is compatible with the running software. A
// major version change indicates the software for the previous major version
// is no longer compatible. A minor version change means the previous version
// is compatible, but some new features may be unavailable.
//
// [31:16] Major version
// [15: 0] Minor version
//
localparam REG_COMPAT = 'h00;
//
localparam REG_MAJOR_POS = 16;
localparam REG_MAJOR_LEN = 16;
//
localparam REG_MINOR_POS =  0;
localparam REG_MINOR_LEN = 16;

// REG_FEATURE_ID (W)
//
// Returns information about the size of the attached memory. The address size
// allows software to determine what buffer size and base address values are
// valid.
//
// [31:16] : Memory Data Word Size. Returns the bit width of the RAM word size.
// [15: 0] : Memory Address Size. Returns the bit width of the RAM byte
//           address. That is, the memory is 2**VALUE bytes in size.
//
localparam REG_FEATURE_ID = 'h04;

// REG_USER_KEY (W)
//
// User key register. This is where the hash part of a license key is written to,
// one 32-bit word at a time.
//
localparam REG_USER_KEY = 'h08;


localparam REG_FEAT_ENB_RB = 'h0C;
