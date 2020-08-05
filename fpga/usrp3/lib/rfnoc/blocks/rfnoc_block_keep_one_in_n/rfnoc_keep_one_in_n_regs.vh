//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  rfnoc_keep_one_in_n_regs
//
// Description:  Header file for rfnoc_block_keep_one_in_n_regs.
//

// Offset in bytes between each ports's bank of registers. This is a hardcoded
// value in ctrlport_to_settings_bus.v put here for documentation purposes only.
localparam REG_BANK_OFFSET = 2**11; // 2048

// [WIDTH_N-1:0] : N, drop N-1 samples or packets
localparam REG_N     = 0;
localparam REG_N_LEN = WIDTH_N;

// [0:0] : 0 = Sample Mode, 1 = Packet Mode
localparam REG_MODE     = 1;
localparam REG_MODE_LEN = 1;

// [31:0] : Bit width of N, Read Only
localparam REG_WIDTH_N     = 2;
localparam REG_WIDTH_N_LEN = 32;
