//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_duc_regs (Header)
//
// Description: Header file for RFNoC DUC functionality. This includes 
// register offsets, bitfields and constants for the radio components.
//

// For now, these offsets match the original DUC
localparam DUC_BASE_ADDR = 'h00;
localparam DUC_ADDR_W    = 8;

localparam RB_COMPAT_NUM     = 0;
localparam RB_NUM_HB         = 1;
localparam RB_CIC_MAX_INTERP = 2;
localparam SR_N_ADDR         = 128;
localparam SR_M_ADDR         = 129;
localparam SR_CONFIG_ADDR    = 130;
localparam SR_INTERP_ADDR    = 131;
localparam SR_FREQ_ADDR      = 132;
localparam SR_SCALE_IQ_ADDR  = 133;

