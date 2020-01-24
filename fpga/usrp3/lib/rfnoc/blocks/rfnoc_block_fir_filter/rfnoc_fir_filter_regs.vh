//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  fir_filter_regs (Header)
//
// Description:  Header file for rfnoc_block_fir_filter. All registers are 
//               32-bit words from software's perspective.
//

// Address space size, per FIR filter. That is, each filter is separated in the 
// CTRL Port address space by 2^FIR_FILTER_ADDR_W bytes.
localparam FIR_FILTER_ADDR_W = 4;



// REG_FIR_NUM_COEFFS (R)
//
// Contains the number of coefficients for the filter.
//
// [31:0] : Returns the number of coefficients (read-only)
//
localparam REG_FIR_NUM_COEFFS = 'h0;


// REG_FIR_LOAD_COEFF (R)
//
// Register for inputting the next coefficient to be loaded into the filter. To
// load a new set of filter coefficients, write NUM_COEFFS-1 coefficients to
// this register, then write the last coefficient to REG_FIR_LOAD_COEFF_LAST.
// The width of each coefficient is set by the COEFF_WIDTH parameter on the
// block.
//
// [31:(32-COEFF_WIDTH)] : Reserved
// [COEFF_WIDTH-1:0]     : The next coefficient to be loaded
//
localparam REG_FIR_LOAD_COEFF = 'h4;


// REG_FIR_LOAD_COEFF_LAST (R)
//
// Register for inputting the last coefficient to be loaded into the filter. To
// load a new set of filter coefficients, write NUM_COEFFS-1 coefficients to
// REG_FIR_LOAD_COEFF, then write the last coefficient to this register. The
// width of each coefficient is set by the COEFF_WIDTH parameter on the block.
//
// [31:(32-COEFF_WIDTH)] : Reserved
// [COEFF_WIDTH-1:0]     : The next coefficient to be loaded
//
localparam REG_FIR_LOAD_COEFF_LAST = 'h8;