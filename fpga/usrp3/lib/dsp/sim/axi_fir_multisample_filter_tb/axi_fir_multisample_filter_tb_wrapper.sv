//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_fir_multisample_filter_tb_wrapper
//
// Description:
//
//   testing various configurations of the filter
//

module axi_fir_multisample_filter_tb_wrapper();

//---------------------------------------------------------------------------
// Local Parameters
//---------------------------------------------------------------------------
timeunit 1ns / 1ps;

localparam integer NUM_SPC_TEST[2:0]         = {2 ,  4 ,  8};
localparam integer NUM_COEFFS_TEST[2:0]      = {5 , 17 , 41};
localparam integer PARA_TEST[1:0]            = {0 , 1};

//---------------------------------------------------------------------------
// Nested test cases
//---------------------------------------------------------------------------

// Full coverage nested test loop
generate
  for (genvar i = 0; i <  $size(NUM_SPC_TEST); i = i + 1) begin : test_NUM_SPC
    for (genvar j = 0; j <  $size(NUM_COEFFS_TEST); j = j + 1) begin : test_NUM_COEFFS
      for (genvar k = 0; k <  $size(PARA_TEST); k = k + 1) begin : test_RELOADABLE
        for (genvar m = 0; m <  $size(PARA_TEST); m = m + 1) begin : test_BLANK
          for (genvar n = 0; n <  $size(PARA_TEST); n = n + 1) begin : test_EMBEDDED
            axi_fir_multisample_filter_tb #(
              .NUM_SPC(NUM_SPC_TEST[i]),
              .NUM_COEFFS(NUM_COEFFS_TEST[j]),
              .RELOADABLE_COEFFS(PARA_TEST[k]),
              .BLANK_OUTPUT(PARA_TEST[m]),
              .USE_EMBEDDED_REGS_COEFFS(PARA_TEST[n])
            ) tb_i ();
          end : test_EMBEDDED
        end : test_BLANK
      end : test_RELOADABLE
    end : test_NUM_COEFFS
  end : test_NUM_SPC
endgenerate

endmodule
