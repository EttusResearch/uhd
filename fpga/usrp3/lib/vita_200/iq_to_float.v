//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module iq_to_float

  #(parameter BITS_IN =16,
    parameter BITS_OUT = 32
    )
   (
    input [15:0] in,
    output [31:0] out
    );

   //imaginary

   //2s complement
   wire [15:0] unsigned_mag;
   wire [15:0] complement;

   //leading bit registers
   wire [15:0] lead;
   wire [15:0] reversed_mag;

   //16-4 encoder
   wire [3:0] binary_out;

   wire [22:0] fraction;
   wire [7:0] exponent;

   wire [15:0] binary_in;

   binary_encoder #(.SIZE(16))
   encoding (.in(binary_in),.out(binary_out));

   // Detect sign, if negative detected perform 2's complement
   assign unsigned_mag = (in[15] == 1)?((~in[15:0])+1'b1):in[15:0];

   //detect leading one
   assign complement = ((~reversed_mag[BITS_IN-1:0])+1'b1);
   assign lead = complement & reversed_mag;

   //calculate fraction and exponent using shift value generated
   wire [15:0] pre_frac = unsigned_mag << ((15 - binary_out));
   assign fraction = {pre_frac[14:0],8'h0};
   assign exponent = (in == 16'b0)?(8'b0):(binary_out +'d127);

   //construct the output
   assign out = {in[15], exponent, fraction};

   //reverse the signed input
   genvar r;

   generate
      for (r = 0; r < 16; r = r+1) begin:bit_reverse
	assign reversed_mag[r] = unsigned_mag[BITS_IN-r-1];
      end
   endgenerate

   //reversed the output of the detect the leading bit procedure
   genvar i;
   generate
     for (i= 0; i < 16; i = i+1) begin: i_rev
       assign binary_in[i] = lead[BITS_IN-i-1];
     end
   endgenerate
endmodule


