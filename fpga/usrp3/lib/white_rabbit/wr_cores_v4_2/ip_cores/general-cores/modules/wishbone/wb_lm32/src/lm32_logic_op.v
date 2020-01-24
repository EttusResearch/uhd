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
// File             : lm32_logic_op.v
// Title            : Logic operations (and / or / not etc)
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

module lm32_logic_op (
    // ----- Inputs -------
    logic_op_x,
    operand_0_x,
    operand_1_x,
    // ----- Outputs -------
    logic_result_x
    );

/////////////////////////////////////////////////////
// Inputs
/////////////////////////////////////////////////////

input [`LM32_LOGIC_OP_RNG] logic_op_x;
input [`LM32_WORD_RNG] operand_0_x;
input [`LM32_WORD_RNG] operand_1_x;

/////////////////////////////////////////////////////
// Outputs
/////////////////////////////////////////////////////

output [`LM32_WORD_RNG] logic_result_x;
reg    [`LM32_WORD_RNG] logic_result_x;
    
/////////////////////////////////////////////////////
// Internal nets and registers 
/////////////////////////////////////////////////////

integer logic_idx;

/////////////////////////////////////////////////////
// Combinational Logic
/////////////////////////////////////////////////////

always @(*)
begin
    for(logic_idx = 0; logic_idx < `LM32_WORD_WIDTH; logic_idx = logic_idx + 1)
        logic_result_x[logic_idx] = logic_op_x[{operand_1_x[logic_idx], operand_0_x[logic_idx]}];
end
    
endmodule

