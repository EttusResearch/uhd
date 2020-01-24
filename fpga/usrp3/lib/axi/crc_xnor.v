//
// Copyright 2017 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Generates an LFSR based on a given seed value
// Note that not all length LFSRs are supported in the file
// For xnor LSFR equations please refer to following link:
// https://www.xilinx.com/support/documentation/application_notes/xapp210.pdf

// All indexing will be from 1 to match indexing used in table from app note above


module crc_xnor #(
  parameter INPUT_WIDTH=64,
  parameter OUTPUT_WIDTH=8
) (
   input clk,
   input [INPUT_WIDTH:1] input_data,
   input rst,
   input hold,
   output [OUTPUT_WIDTH:1] crc_out
);

    wire [INPUT_WIDTH:1] current_lfsr;
    reg  [INPUT_WIDTH:1] current_lfsr_r;

    // LFSR based on table given by Xilinx
    generate if (INPUT_WIDTH == 64) begin
        assign current_lfsr[1] = current_lfsr_r[64] ^ current_lfsr_r[63] ^ current_lfsr_r[61] ^ current_lfsr_r[60];
        assign current_lfsr[INPUT_WIDTH:2] = current_lfsr_r[INPUT_WIDTH-1:1]; 
    end else begin
        fake_error_thrower invalid_width_parameter();
    end endgenerate

   always @(posedge clk) begin
      if (rst) begin
         current_lfsr_r <= input_data;
      end else if(~hold) begin
         current_lfsr_r <= current_lfsr ^ input_data;
      end
   end

    // Sum reduce based on output width
    generate if(INPUT_WIDTH == 64 && OUTPUT_WIDTH == 16) begin
        assign crc_out =    current_lfsr_r[INPUT_WIDTH:INPUT_WIDTH/4*3+1]+current_lfsr_r[INPUT_WIDTH/4*3:INPUT_WIDTH/4*2+1]+
                            current_lfsr_r[INPUT_WIDTH/4*2:INPUT_WIDTH/4+1]+current_lfsr_r[INPUT_WIDTH/4:1];
    end else if(INPUT_WIDTH == 64 && OUTPUT_WIDTH == 32) begin
        assign crc_out =    current_lfsr_r[INPUT_WIDTH:INPUT_WIDTH/2+1]+current_lfsr_r[INPUT_WIDTH/2:1];
    end else begin
        fake_error_thrower invalid_width_parameter();
    end endgenerate
      
   

endmodule 
