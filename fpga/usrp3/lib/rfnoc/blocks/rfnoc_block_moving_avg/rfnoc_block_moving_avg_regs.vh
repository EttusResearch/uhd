//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  rfnoc_block_moving_avg_regs (Header)
//
// Description: RFNoC Moving Average block register descriptions
//

// Address space size, per moving average core. That is, each moving average
// core's address space is separated in the CtrlPort address space by
// 2^MOVING_AVG_ADDR_W bytes.
localparam MOVING_AVG_ADDR_W = 3;



// REG_SUM_LENGTH (R/W)
//
// Number of consecutive input samples for which to accumulate the I and Q
// values. Writing to this register clears the history and resets the
// accumulated sum to 0.
//
localparam REG_SUM_LENGTH = 'h0;
//
localparam REG_SUM_LENGTH_LEN = 8;


// REG_DIVISOR (R/W)
//
// Number by which to divide the accumulated sum. This is a signed integer
// value.
//
localparam REG_DIVISOR = 'h4;
//
localparam REG_DIVISOR_LEN = 24;
