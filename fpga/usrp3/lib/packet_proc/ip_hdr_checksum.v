//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Description: 
//   Compute IP header checksum.  0-2 cycles of latency.
//
// Parameter:
//   LATENCY : sets how many pipeline stages are inserted
//     0 - No pipelining
//     1 - output Flop
//     2 - output + gen_sum Flop
//

module ip_hdr_checksum #(
  parameter LATENCY = 2
) (
  input clk, input [159:0] in, output reg [15:0] out, input clken
);

  wire [18:0] padded [0:9];
   
  genvar i;
  generate
    for(i=0 ; i<10 ; i=i+1)
	assign padded[i] = {3'b000,in[i*16+15:i*16]};
    endgenerate

  wire [18:0] sum_a_d, sum_b_d;
  reg  [18:0] sum_a_q, sum_b_q;

  assign sum_a_d = padded[0] + padded[1] + padded[2] + padded[3] + padded[4];
  assign sum_b_d = padded[5] + padded[6] + padded[7] + padded[8] + padded[9];

  if (LATENCY >= 1) begin : gen_sum_ff
    always @(posedge clk) if (clken) sum_a_q <= sum_a_d;
    always @(posedge clk) if (clken) sum_b_q <= sum_b_d;
  end else begin : gen_sum_comb
    always @(sum_a_d) sum_a_q = sum_a_d;
    always @(sum_b_d) sum_b_q = sum_b_d;
  end

  wire [18:0] sum;
  wire [15:0] out_d;

  assign sum = sum_a_q + sum_b_q;
  assign out_d = ~(sum[15:0] + {13'd0,sum[18:16]});

  if (LATENCY >= 2) begin : gen_out_ff
    always @(posedge clk)  if (clken) out <= out_d;
  end else begin : gen_out_comb
    always @(out_d) out = out_d;
  end

endmodule // ip_hdr_checksum
