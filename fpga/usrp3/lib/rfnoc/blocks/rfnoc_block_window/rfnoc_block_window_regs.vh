//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  rfnoc_block_window_regs (Header)
//
// Description: RFNoC Window block register descriptions
//

// Address space size, per window module. That is, each window module's
// address space is separated in the CtrlPort address space by 2^WINDOW_ADDR_W
// bytes.
localparam WINDOW_ADDR_W = 4;



// REG_WINDOW_SIZE (R/W)
//
// Controls the current window size, in number of samples.
//
localparam REG_WINDOW_SIZE = 'h0;


// REG_WINDOW_MAX_SIZE (R)
//
// Reports the maximum supported window size, in number of samples.
//
localparam REG_WINDOW_MAX_SIZE = 'h4;


// REG_LOAD_COEFF (W)
//
// Register for inputting the next coefficient to be loaded into the window
// module. To load a new set of coefficients, write REG_WINDOW_SIZE-1
// coefficients to this register, then write the last coefficient to
// REG_LOAD_COEFF_LAST.
//
// [31:16] : Reserved
// [15: 0] : The next coefficient to be loaded
//
localparam REG_LOAD_COEFF = 'h8;
//
localparam REG_LOAD_COEFF_LEN = 16;


// REG_LOAD_COEFF_LAST (W)
//
// Register for inputting the last coefficient to be loaded into the window
// module. To load a new set of filter coefficients, write REG_WINDOW_SIZE-1
// coefficients to REG_LOAD_COEFF, then write the last coefficient to this
// register.
//
// [31:16] : Reserved
// [15: 0] : The last coefficient to be loaded
//
localparam REG_LOAD_COEFF_LAST = 'hC;
//
// The length of the last coefficient is the same as REG_LOAD_COEFF_LEN.
