//
// Copyright 2026 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_hb_decim_fir_all_tb.sv
//
// Description: Testbench wrapper to run all AXI halfband generic FIR filter tests
//

`default_nettype none

module axis_hb_decim_fir_all_tb ();

  // Test-case table: each column is one parameter, each row is one test case.
  // Add a new row to add a new configuration; no other changes needed.
  typedef struct {
    int samp_w;
    int spc;
    int num_coeffs;
    bit preload_zeroes;
  } test_case_t;

  localparam test_case_t TEST_CASES [14] = '{
    // SAMP_W  SPC  NUM_COEFFS  PRELOAD_ZEROES
    '{ 48, 8, 47, 0 },
    '{ 48, 8, 63, 0 },
    '{ 48, 4, 47, 0 },
    '{ 48, 4, 63, 0 },
    '{ 48, 2, 47, 0 },
    '{ 48, 2, 63, 0 },
    '{ 48, 1, 47, 0 },
    '{ 48, 1, 63, 0 },
    '{ 32, 8, 63, 0 },
    '{ 16, 8, 63, 0 },
    '{ 32, 8, 47, 1 },
    '{ 16, 8, 63, 1 },
    '{ 32, 1, 47, 1 },
    '{ 32, 1, 63, 1 }
  };

  generate
    for (genvar i = 0; i < $size(TEST_CASES); i++) begin : gen_tb
      axis_hb_decim_fir_tb #(
        .SAMP_W        (TEST_CASES[i].samp_w),
        .SPC           (TEST_CASES[i].spc),
        .NUM_COEFFS    (TEST_CASES[i].num_coeffs),
        .PRELOAD_ZEROES(TEST_CASES[i].preload_zeroes)
      ) tb_i ();
    end : gen_tb
  endgenerate

endmodule : axis_hb_decim_fir_all_tb

`default_nettype wire
