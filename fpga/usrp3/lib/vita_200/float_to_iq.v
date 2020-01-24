//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module float_to_iq

  #(parameter BITS_IN = 32,
    parameter BITS_OUT = 16
    )

   (

    input [31:0]   in,
    output [15:0] out
    );

   //flags

   wire 		   neg_inf;
   wire 		   pos_inf;
   wire 		   denorm;
   wire tiny_exp;
   


   assign pos_inf = (in[31] == 0 && in[30:23] == 1 && in[22:0] == 0);
   assign neg_inf = (in[31] == 1 && in[30:23] == 1 && in[22:0] == 0);
   assign denorm = (in[30:23] == 0);
   assign tiny_exp = (in[30:23] < 'd111);
   
   
   



   wire [23:0] 		   implied_bit_fraction;
   wire [24:0] 		   operation_round;
   wire [15:0] 		   round_fraction;
   wire [15:0] 		   shifted_fraction;
   wire [7:0] 		   shift_val;

   wire [22:0] 		   true_frac;
   

   
   
   assign shift_val = (in[30:23] > 127)? (in[30:23] - 127): (127 - in[30:23]);
   assign implied_bit_fraction = {1'b1,in[22:0]};
 
   
   
   assign operation_round = (implied_bit_fraction + 'h000080);
   

   //testing for overflow
   assign round_fraction = (operation_round[24] == 0)?(operation_round[23:8]):(16'h7FFF);
   //shift the rounded value
   wire [15:0] shift = round_fraction >> (15 - shift_val);
   //2's complement the shifted output if the signed bit is 1
   wire [15:0] final_val = (in[31] == 1)?(~shift + 1'b1):shift;
   
   

   assign out = (pos_inf)?{1'b0,15'h7FFF}:(neg_inf)?{1'b1,15'h8000}:(denorm || tiny_exp)? 16'b0: final_val;
   


endmodule















