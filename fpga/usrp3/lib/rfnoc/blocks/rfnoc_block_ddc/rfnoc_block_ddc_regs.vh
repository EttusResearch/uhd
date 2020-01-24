//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_ddc_regs (Header)
//
// Description: Header file for RFNoC DDC functionality. This includes 
// register offsets, bitfields and constants for the radio components.
//

// For now, these offsets match the original DDC
localparam DDC_BASE_ADDR = 'h00;
localparam DDC_ADDR_W    = 8;

localparam RB_COMPAT_NUM    = 0;
localparam RB_NUM_HB        = 1;
localparam RB_CIC_MAX_DECIM = 2;
localparam SR_N_ADDR        = 128;
localparam SR_M_ADDR        = 129;
localparam SR_CONFIG_ADDR   = 130;
localparam SR_FREQ_ADDR     = 132;
localparam SR_SCALE_IQ_ADDR = 133;
localparam SR_DECIM_ADDR    = 134;
localparam SR_MUX_ADDR      = 135;
localparam SR_COEFFS_ADDR   = 136;

