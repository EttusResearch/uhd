//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: usrp_utils
//
// Description:
//
//   A header file containing commonly used Verilog macros.
//

`ifndef USRP_UTILS_SVH
`define USRP_UTILS_SVH

`define ABS(X)    ((X) < (0) ? -(X) : (X))
`define MAX(X, Y) ((X) > (Y) ?  (X) : (Y))
`define MIN(X, Y) ((X) < (Y) ?  (X) : (Y))

// Ceiling of X/Y for integer operands. Use $ceil(X/Y) for real numbers.
`define DIV_CEIL(X, Y) (((X) + (Y) - 1) / (Y))
// Floor of X/Y for integer operands. Use $floor(X/Y) for real numbers.
`define DIV_FLOOR(X ,Y) ((X) / (Y))

// Convert an unsigned number to a signed number. Note that this requires
// lengthening the value by one bit to allow for the sign.
`define TO_SIGNED(X) (signed'({1'b0, (X)}))

// Shorthand to get the I'th port of a concatenated bus. For example, if a bus
// contains four 16-bit ports, you can get the 3rd port (port 2) using the
// macro call `BUS_I(my_bus, 16, 2), which is equivalent to my_bus[32+:16], or
// my_bus[47:32]. The macro is useful when the width or index is an expression.
`define BUS_I(BUS_NAME, W, I) BUS_NAME[((W)*(I)) +: (W)]

// Return a string that contains the file and line number
`define LINE_INFO $sformatf("%s: %0d", `__FILE__, `__LINE__)

`endif // USRP_UTILS_SVH
