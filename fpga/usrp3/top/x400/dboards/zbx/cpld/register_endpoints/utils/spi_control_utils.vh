//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: spi_control_utils
//
// Description:
//  Contains constants and functions common to blocks that interact
//  with the wishbone spi engine.
//

// SPI Master Control Register Offsets
localparam TX_DATA_REG        = 5'h00;
localparam RX_DATA_REG        = 5'h00;
localparam CLOCK_DIVIDER_REG  = 5'h14;
localparam CONTROL_REG        = 5'h10;
localparam SS_REG             = 5'h18;

// Simple function to return a vector of 0's with only position
// ss_input set to 1.
function automatic [31:0] set_ss_bit;
input  [3:0]  ss_input;
reg   [31:0]  ss_data;
begin
    ss_data           = 32'h0;
    ss_data[ss_input] = 1'b1;
    set_ss_bit        = ss_data;
end
endfunction
