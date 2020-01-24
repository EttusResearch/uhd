// =============================================================================
//                           COPYRIGHT NOTICE
// Copyright 2006 (c) Lattice Semiconductor Corporation
// ALL RIGHTS RESERVED
// This confidential and proprietary software may be used only as authorised by
// a licensing agreement from Lattice Semiconductor Corporation.
// The entire notice above must be reproduced on all authorized copies and
// copies may only be made to the extent permitted by a licensing agreement from
// Lattice Semiconductor Corporation.
//
// Lattice Semiconductor Corporation        TEL : 1-800-Lattice (USA and Canada)
// 5555 NE Moore Court                            408-826-6000 (other locations)
// Hillsboro, OR 97124                     web  : http://www.latticesemi.com/
// U.S.A                                   email: techsupport@latticesemi.com
// =============================================================================/
//                         FILE DETAILS
// Project          : LatticeMico32
// File             : lm32_shifter.v
// Title            : Barrel shifter
// Dependencies     : lm32_include.v
// Version          : 6.1.17
//                  : Initial Release
// Version          : 7.0SP2, 3.0
//                  : No Change
// Version          : 3.1
//                  : No Change
// =============================================================================

`include "lm32_include.v"

/////////////////////////////////////////////////////
// Module interface
/////////////////////////////////////////////////////

module lm32_shifter (
    // ----- Inputs -------
    clk_i,
    rst_i,
    stall_x,
    direction_x,
    sign_extend_x,
    operand_0_x,
    operand_1_x,
    // ----- Outputs -------
    shifter_result_m
    );

/////////////////////////////////////////////////////
// Inputs
/////////////////////////////////////////////////////

input clk_i;                                // Clock
input rst_i;                                // Reset
input stall_x;                              // Stall instruction in X stage
input direction_x;                          // Direction to shift
input sign_extend_x;                        // Whether shift is arithmetic (1'b1) or logical (1'b0)
input [`LM32_WORD_RNG] operand_0_x;         // Operand to shift
input [`LM32_WORD_RNG] operand_1_x;         // Operand that specifies how many bits to shift by

/////////////////////////////////////////////////////
// Outputs
/////////////////////////////////////////////////////

output [`LM32_WORD_RNG] shifter_result_m;   // Result of shift
wire   [`LM32_WORD_RNG] shifter_result_m;

/////////////////////////////////////////////////////
// Internal nets and registers 
/////////////////////////////////////////////////////

reg direction_m;
wire [`LM32_WORD_WIDTH*2-1:0] right_shift_result_x;
reg [`LM32_WORD_RNG] left_shift_result;
reg [`LM32_WORD_RNG] right_shift_result_m;
reg [`LM32_WORD_RNG] left_shift_operand;
wire [`LM32_WORD_RNG] right_shift_operand;
wire fill_value;
wire [`LM32_WORD_RNG] right_shift_in;

integer shift_idx_0;
integer shift_idx_1;

/////////////////////////////////////////////////////
// Combinational Logic
/////////////////////////////////////////////////////
    
// Select operands - To perform a left shift, we reverse the bits and perform a right shift
always @(*)
begin
    for (shift_idx_0 = 0; shift_idx_0 < `LM32_WORD_WIDTH; shift_idx_0 = shift_idx_0 + 1)
        left_shift_operand[`LM32_WORD_WIDTH-1-shift_idx_0] = operand_0_x[shift_idx_0];
end
assign right_shift_operand = direction_x == `LM32_SHIFT_OP_LEFT ? left_shift_operand : operand_0_x;

// Determine fill value for right shift - Sign bit for arithmetic shift, or zero for logical shift
assign fill_value = (sign_extend_x == `TRUE) && (direction_x == `LM32_SHIFT_OP_RIGHT) 
                      ? operand_0_x[`LM32_WORD_WIDTH-1] 
                      : 1'b0;

// Determine bits to shift in for right shift or rotate
assign right_shift_in = {`LM32_WORD_WIDTH{fill_value}};

// Determine the result of the shifter
assign right_shift_result_x = {right_shift_in, right_shift_operand} >> operand_1_x[`LM32_SHIFT_RNG];

// Reverse bits to get left shift result
always @(*)
begin
    for (shift_idx_1 = 0; shift_idx_1 < `LM32_WORD_WIDTH; shift_idx_1 = shift_idx_1 + 1)
        left_shift_result[`LM32_WORD_WIDTH-1-shift_idx_1] = right_shift_result_m[shift_idx_1];
end

// Select result 
assign shifter_result_m = direction_m == `LM32_SHIFT_OP_LEFT ? left_shift_result : right_shift_result_m;
    
/////////////////////////////////////////////////////
// Sequential Logic
/////////////////////////////////////////////////////

// Perform right shift
always @(posedge clk_i `CFG_RESET_SENSITIVITY)
begin
    if (rst_i == `TRUE)
    begin
        right_shift_result_m <= {`LM32_WORD_WIDTH{1'b0}};
        direction_m <= `FALSE;
    end
    else
    begin
        if (stall_x == `FALSE)
        begin
            right_shift_result_m <= right_shift_result_x[`LM32_WORD_RNG];
            direction_m <= direction_x;
        end
    end
end 
    
endmodule
