//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Compute IP header checksum.  2 cycles of latency.
module ip_hdr_checksum
  (input clk, input [159:0] in, output reg [15:0] out);

   wire [18:0] padded [0:9];
   reg [18:0]  sum_a, sum_b;
   
   genvar     i;
   generate
      for(i=0 ; i<10 ; i=i+1)
	assign padded[i] = {3'b000,in[i*16+15:i*16]};
   endgenerate

   always @(posedge clk)  sum_a = padded[0] + padded[1] + padded[2] + padded[3] + padded[4];
   always @(posedge clk)  sum_b = padded[5] + padded[6] + padded[7] + padded[8] + padded[9];

   wire [18:0] sum = sum_a + sum_b;

   always @(posedge clk)
     out <= ~(sum[15:0] + {13'd0,sum[18:16]});
   
   
endmodule // ip_hdr_checksum
