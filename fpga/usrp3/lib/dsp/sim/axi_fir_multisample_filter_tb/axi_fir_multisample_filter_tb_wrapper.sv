//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_fir_multisample_filter_tb_wrapper
//
// Description:
//
//   Top-level testbench wrapper for axi_fir_multisample_filter. This module
//   runs multiple test configurations to verify:
//   - Base functionality with various NUM_SPC, NUM_COEFFS, and feature combinations
//     (RELOADABLE_COEFFS, BLANK_OUTPUT, USE_EMBEDDED_REGS_COEFFS)
//   - Symmetric coefficient optimization with odd tap counts (non-reloadable only)
//   - Symmetric coefficient optimization with even tap counts (non-reloadable only) 
//   - Zero coefficient skipping optimization combined with symmetry (non-reloadable only)
//
//   Each test configuration uses nested generate loops to create comprehensive
//   parameter coverage across different filter configurations.
//

module axi_fir_multisample_filter_tb_wrapper();
  // Common Parameters
  `include "test_exec.svh"
  localparam int COEFF_WIDTH = 16;
  localparam integer NUM_SPC_TEST[2:0]    = {2 ,  4 ,  8};
  localparam integer NUM_COEFFS_TEST[2:0] = {5 , 17 , 41};
  localparam integer PARA_TEST[1:0]       = {0 , 1};
  
  // General tests:
  //   - General tests covering all combinations of NUM_SPC, NUM_COEFFS,
  //     RELOADABLE_COEFFS, BLANK_OUTPUT, and USE_EMBEDDED_REGS_COEFFS parameters.
  //
  //   Test parameters:
  //   - NUM_SPC: 2, 4, 8 samples per cycle
  //   - NUM_COEFFS: 5, 17, 41 taps
  //   - RELOADABLE_COEFFS: 0, 1
  //   - BLANK_OUTPUT: 0, 1
  //   - USE_EMBEDDED_REGS_COEFFS: 0, 1
  generate
    //---------------------------------------------------------------------------
    // Local parameters
    //---------------------------------------------------------------------------
    localparam bit [COEFF_WIDTH-1:0] MAX_COEFF_ARRAY [41] = '{
         16'sd158,    16'sd0,     16'sd33,   -16'sd0,    -16'sd256,
         16'sd553,    16'sd573,  -16'sd542,  -16'sd1012,  16'sd349,
         16'sd1536,   16'sd123,  -16'sd2097, -16'sd1012,  16'sd1633,
         16'sd1608,  -16'sd3077, -16'sd5946,  16'sd3370,  16'sd10513,
        -16'sd19295, // 16'sd19295, change to negative to avoid clipping
         16'sd10513, 16'sd3370,  -16'sd5946, -16'sd3077,  16'sd1608,
         16'sd1633, -16'sd1012,  -16'sd2097,  16'sd123,   16'sd1536,
         16'sd349,  -16'sd1012,  -16'sd542,   16'sd573,   16'sd553,
        -16'sd256,  -16'sd0,      16'sd33,    16'sd0,     16'sd158
      };
    //---------------------------------------------------------------------------
    // Nested test cases
    //---------------------------------------------------------------------------
    // Full coverage nested test loop
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
                .USE_EMBEDDED_REGS_COEFFS(PARA_TEST[n]),
                .COEFF_WIDTH(COEFF_WIDTH),
                .COEFFS_VEC(MAX_COEFF_ARRAY[0 +: NUM_COEFFS_TEST[j]])
              ) tb_i ();
            end : test_EMBEDDED
          end : test_BLANK
        end : test_RELOADABLE
      end : test_NUM_COEFFS
    end : test_NUM_SPC
  endgenerate

  //
  // Tests for symmetric coefficient optimization with odd tap counts.
  //   - Uses symmetric coefficient arrays to enable OPTIMIZE_FOR_SYMMETRIC_COEFFS.
  //   - Only non-reloadable configurations (RELOADABLE_COEFFS=0) are tested.
  //
  //   Test parameters:
  //   - NUM_SPC: 2, 4, 8 samples per cycle
  //   - NUM_COEFFS: 5, 17, 41 taps (all odd)
  //   - BLANK_OUTPUT: 0, 1
  //   - RELOADABLE_COEFFS: 0 (fixed)
  //   - USE_EMBEDDED_REGS_COEFFS: 0 (fixed)
  //
  generate
    //---------------------------------------------------------------------------
    // Local parameters
    //---------------------------------------------------------------------------
    localparam bit [COEFF_WIDTH-1:0] MAX_COEFF_ARRAY_SYMMETRIC_ODD [41] = '{
         16'sd158,   16'sd158,    16'sd33,  -16'sd256,  -16'sd256,
         16'sd553,   16'sd573,  -16'sd542,  -16'sd1012,  16'sd349,
         16'sd1536,  16'sd123,  -16'sd2097, -16'sd1012,  16'sd1633,
         16'sd1608, -16'sd3077, -16'sd5946,  16'sd3370,  16'sd10513,
        -16'sd19295, // 16'sd19295, change to negative to avoid clipping
         16'sd10513, 16'sd3370, -16'sd5946, -16'sd3077,  16'sd1608,
         16'sd1633, -16'sd1012, -16'sd2097,  16'sd123,   16'sd1536,
         16'sd349,  -16'sd1012, -16'sd542,   16'sd573,   16'sd553,
        -16'sd256,  -16'sd256,   16'sd33,    16'sd158,   16'sd158
    };
    //---------------------------------------------------------------------------
    // Nested test cases
    //---------------------------------------------------------------------------
    for (genvar i = 0; i <  $size(NUM_SPC_TEST); i = i + 1) begin : test_NUM_SPC_SYM
      for (genvar j = 0; j <  $size(NUM_COEFFS_TEST); j = j + 1) begin : test_NUM_COEFFS_SYM
        for (genvar m = 0; m <  $size(PARA_TEST); m = m + 1) begin : test_BLANK_SYM
          // ... extract symmetric coefficients for odd number of taps
          localparam bit [COEFF_WIDTH-1:0] COEFFS_SYMM_ODD [NUM_COEFFS_TEST[j]] =
            MAX_COEFF_ARRAY_SYMMETRIC_ODD[($size(MAX_COEFF_ARRAY_SYMMETRIC_ODD)-NUM_COEFFS_TEST[j])/2 +: NUM_COEFFS_TEST[j]];
          // ... instantiate testbench
          axi_fir_multisample_filter_tb #(
            .NUM_SPC(NUM_SPC_TEST[i]),
            .NUM_COEFFS(NUM_COEFFS_TEST[j]),
            .RELOADABLE_COEFFS(0),
            .BLANK_OUTPUT(PARA_TEST[m]),
            .USE_EMBEDDED_REGS_COEFFS(0),
            .COEFF_WIDTH(COEFF_WIDTH),
            .COEFFS_VEC(COEFFS_SYMM_ODD)
          ) tb_sym_i ();
        end : test_BLANK_SYM
      end : test_NUM_COEFFS_SYM
    end : test_NUM_SPC_SYM
  endgenerate

  //
  // Tests for symmetric coefficient optimization with even tap counts.
  //   - Uses symmetric coefficient arrays to enable OPTIMIZE_FOR_SYMMETRIC_COEFFS.
  //   - Only non-reloadable configurations (RELOADABLE_COEFFS=0) are tested.
  //
  //   Test parameters:
  //   - NUM_SPC: 2, 4, 8 samples per cycle
  //   - NUM_COEFFS: 6, 18, 40 taps (all even)
  //   - BLANK_OUTPUT: 0, 1
  //   - RELOADABLE_COEFFS: 0 (fixed)
  //   - USE_EMBEDDED_REGS_COEFFS: 0 (fixed)
  //
  generate
    //---------------------------------------------------------------------------
    // Local parameters
    //---------------------------------------------------------------------------
    localparam integer SYM_EVEN_NUM_COEFFS_TEST[2:0] = {6, 18, 40};
    // ... symmetric coefficients with even number of taps
    localparam bit [COEFF_WIDTH-1:0] MAX_COEFF_ARRAY_SYMMETRIC_EVEN [40] = '{
         16'sd158,   16'sd158,    16'sd33,  -16'sd256,  -16'sd256,
         16'sd553,   16'sd573,  -16'sd542,  -16'sd1012,  16'sd349,
         16'sd1536,  16'sd123,  -16'sd2097, -16'sd1012,  16'sd1633,
         16'sd1608, -16'sd3077, -16'sd5946,  16'sd3370,  16'sd10513,
         16'sd10513, 16'sd3370, -16'sd5946, -16'sd3077,  16'sd1608,
         16'sd1633, -16'sd1012, -16'sd2097,  16'sd123,   16'sd1536,
         16'sd349,  -16'sd1012, -16'sd542,   16'sd573,   16'sd553,
        -16'sd256,  -16'sd256,   16'sd33,    16'sd158,   16'sd158
    };
    //---------------------------------------------------------------------------
    // Nested test cases
    //---------------------------------------------------------------------------
    for (genvar i = 0; i <  $size(NUM_SPC_TEST); i = i + 1) begin : test_NUM_SPC_SYM_EVEN
      for (genvar j = 0; j <  $size(SYM_EVEN_NUM_COEFFS_TEST); j = j + 1) begin : test_NUM_COEFFS_SYM_EVEN
        for (genvar m = 0; m <  $size(PARA_TEST); m = m + 1) begin : test_BLANK_SYM_EVEN
          // ... extract symmetric coefficients for even number of taps
          localparam bit [COEFF_WIDTH-1:0] COEFFS_SYMM_EVEN [SYM_EVEN_NUM_COEFFS_TEST[j]] =
            MAX_COEFF_ARRAY_SYMMETRIC_EVEN[($size(MAX_COEFF_ARRAY_SYMMETRIC_EVEN)-SYM_EVEN_NUM_COEFFS_TEST[j])/2 +: SYM_EVEN_NUM_COEFFS_TEST[j]];
          // ... instantiate testbench
          axi_fir_multisample_filter_tb #(
            .NUM_SPC(NUM_SPC_TEST[i]),
            .NUM_COEFFS(SYM_EVEN_NUM_COEFFS_TEST[j]),
            .RELOADABLE_COEFFS(0),
            .BLANK_OUTPUT(PARA_TEST[m]),
            .USE_EMBEDDED_REGS_COEFFS(0),
            .COEFF_WIDTH(COEFF_WIDTH),
            .COEFFS_VEC(COEFFS_SYMM_EVEN)
          ) tb_sym_even_i ();
        end : test_BLANK_SYM_EVEN
      end : test_NUM_COEFFS_SYM_EVEN
    end : test_NUM_SPC_SYM_EVEN
  endgenerate

  //
  // Tests for combined symmetric and zero coefficient optimizations.
  //   - Uses symmetric coefficient arrays with every 2nd coefficient set to zero
  //     to enable both OPTIMIZE_FOR_SYMMETRIC_COEFFS and OPTIMIZE_FOR_ZERO_COEFFS.
  //   - Only non-reloadable configurations (RELOADABLE_COEFFS=0) are tested.
  //
  //   Test parameters:
  //   - NUM_SPC: 1, 2, 3, 4, 8, 16 samples per cycle
  //   - NUM_COEFFS: 5, 17, 41 taps (all odd)
  //   - BLANK_OUTPUT: 0, 1
  //   - RELOADABLE_COEFFS: 0 (fixed)
  //   - USE_EMBEDDED_REGS_COEFFS: 0 (fixed)
  //
  generate
    //---------------------------------------------------------------------------
    // Local parameters
    //---------------------------------------------------------------------------
    localparam integer SYM_ZEROS_NUM_SPC_TEST[3:0]   = {1, 2, 4, 8};
    // ... every 2nd coefficient is zero for testing zero-coefficient skipping optimization
    localparam bit [COEFF_WIDTH-1:0] MAX_COEFF_ARRAY_SYMMETRIC_ZEROS [41] = '{
         16'sd158,   16'sd0,     16'sd33,    16'sd0,    -16'sd256,
         16'sd0,     16'sd573,   16'sd0,    -16'sd1012,  16'sd0,
         16'sd1536,  16'sd0,    -16'sd2097,  16'sd0,     16'sd1633,
         16'sd0,    -16'sd3077,  16'sd0,     16'sd3370,  16'sd0,
        -16'sd19295, // center coefficient (odd number of taps)
         16'sd0,     16'sd3370,  16'sd0,    -16'sd3077,  16'sd0,
         16'sd1633,  16'sd0,    -16'sd2097,  16'sd0,     16'sd1536,
         16'sd0,    -16'sd1012,  16'sd0,     16'sd573,   16'sd0,
        -16'sd256,   16'sd0,     16'sd33,    16'sd0,     16'sd158
    };
    //---------------------------------------------------------------------------
    // Nested test cases
    //---------------------------------------------------------------------------
    for (genvar i = 0; i <  $size(SYM_ZEROS_NUM_SPC_TEST); i = i + 1) begin : test_NUM_SPC_SYM_ZEROS
      for (genvar j = 0; j <  $size(NUM_COEFFS_TEST); j = j + 1) begin : test_NUM_COEFFS_SYM_ZEROS
        for (genvar m = 0; m <  $size(PARA_TEST); m = m + 1) begin : test_BLANK_SYM_ZEROS
          // ... extract symmetric coefficients for odd number of taps
          localparam bit [COEFF_WIDTH-1:0] COEFFS_SYMM_ODD [NUM_COEFFS_TEST[j]] =
            MAX_COEFF_ARRAY_SYMMETRIC_ZEROS[($size(MAX_COEFF_ARRAY_SYMMETRIC_ZEROS)-NUM_COEFFS_TEST[j])/2 +: NUM_COEFFS_TEST[j]];
          // ... instantiate testbench
          axi_fir_multisample_filter_tb #(
            .NUM_SPC(SYM_ZEROS_NUM_SPC_TEST[i]),
            .NUM_COEFFS(NUM_COEFFS_TEST[j]),
            .RELOADABLE_COEFFS(0),
            .BLANK_OUTPUT(PARA_TEST[m]),
            .USE_EMBEDDED_REGS_COEFFS(0),
            .COEFF_WIDTH(COEFF_WIDTH),
            .COEFFS_VEC(COEFFS_SYMM_ODD)
          ) tb_sym_odd_zeros_i ();
        end : test_BLANK_SYM_ZEROS
      end : test_NUM_COEFFS_SYM_ZEROS
    end : test_NUM_SPC_SYM_ZEROS
  endgenerate
endmodule : axi_fir_multisample_filter_tb_wrapper