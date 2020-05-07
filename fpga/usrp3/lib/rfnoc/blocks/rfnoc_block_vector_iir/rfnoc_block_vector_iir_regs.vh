//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_vector_iir_regs (Header)
//
// Description:
//
//   This is a header file that contains the register descriptions for the
//   RFNoC Vector IIR block.
//
//   Each RFNoC Vector IIR block consists of NUM_PORTS independent Vector IIR
//   filters. Each one has its own address space that is VECTOR_IIR_ADDR_W bits
//   wide. That is, Vector IIR block N can be addressed starting at byte offset
//   N*(2**VECTOR_IIR_ADDR_W).
//

//-----------------------------------------------------------------------------
// Register Space
//-----------------------------------------------------------------------------

// The amount of address space taken up by each Vector IIR filter. That is, the
// address space for port N starts at N*(2^VECTOR_IIR_ADDR_W).
localparam VECTOR_IIR_ADDR_W = 20'h00004;


//-----------------------------------------------------------------------------
// Vector IIR Register Descriptions
//-----------------------------------------------------------------------------

// REG_DELAY (R/W)
//
// This register controls and reports the state of the filter delay. 
//
// [31:16] REG_MAX_DELAY : This field reports the maximum supported vector
//                         length, in samples. That is, it returns the
//                         MAX_DELAY block parameter. 
// [15: 0] REG_DELAY     : This field controls/reports the current vector delay
//                         length in samples. Values of 5 or more are supported.
//
localparam REG_DELAY = 'h00;
//
localparam REG_MAX_DELAY_LEN = 16;
localparam REG_MAX_DELAY_POS = 16;
//
localparam REG_DELAY_LEN = 16;
localparam REG_DELAY_POS = 0;

// REG_ALPHA (R/W)
//
// This register controls the Alpha value for the filter. This is a signed
// 16-bit value.
//
// [31:0] : Unused
// [15:0] : Alpha value to use
//
localparam REG_ALPHA = 'h04;
//
localparam REG_ALPHA_LEN = 16;
localparam REG_ALPHA_POS = 0;

// REG_BETA (R/W)
//
// This register controls the Beta value for the filter. This is a signed
// 16-bit value.
//
// [31:0] : Unused
// [15:0] : Beta value to use
//
localparam REG_BETA = 'h08;
//
localparam REG_BETA_LEN = 16;
localparam REG_BETA_POS = 0;
