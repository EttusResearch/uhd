//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: timed_complex_gain_regs (Header)
//
// Description: Header file for timed complex gain functionality.
//

//-----------------------------------------------------------------------------
// Register Addresses
//-----------------------------------------------------------------------------

localparam REG_CGAIN_COEFF = 'h00;  // The gain coefficient register:
                                    //  - Bits [31:16] : Real part
                                    //  - Bits [15:0]  : Imaginary part
