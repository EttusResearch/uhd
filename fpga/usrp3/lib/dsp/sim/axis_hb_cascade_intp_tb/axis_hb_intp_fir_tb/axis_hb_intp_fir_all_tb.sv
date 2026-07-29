//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_hb_intp_fir_all_tb.sv
//
// Description:
//   Parameter-sweep wrapper for axis_hb_intp_fir_tb.
//   Instantiates the testbench for every entry in the TEST_CASES table.
//   Add a new row to cover an additional configuration; no other changes needed.
//

`default_nettype none

module axis_hb_intp_fir_all_tb ();

  typedef struct {
    int samp_w;
    int spc_out;      // Output samples per clock (FIR parallelism; input = spc_out/2)
    int num_coeffs;
    bit preload_zeroes;
  } test_case_t;

  // ---------------------------------------------------------------------------
  // Test-case table
  //
  // Columns: SAMP_W  SPC_OUT  NUM_COEFFS  PRELOAD_ZEROES
  //
  // SPC_OUT = 1  → temporal mode (input alternates x[n] / 0 each clock)
  // SPC_OUT = 2  → SPC_IN = 1 spatial: one input sample → two output samples/clk
  // SPC_OUT = 4  → SPC_IN = 2
  // SPC_OUT = 8  → SPC_IN = 4
  // ---------------------------------------------------------------------------
  localparam test_case_t TEST_CASES [14] = '{
    // HB47, PRELOAD_ZEROES = 0
    '{ 48,  8, 47, 0 },
    '{ 48,  4, 47, 0 },
    '{ 48,  2, 47, 0 },
    '{ 48,  1, 47, 0 },
    // HB63, PRELOAD_ZEROES = 0
    '{ 48,  8, 63, 0 },
    '{ 48,  4, 63, 0 },
    '{ 48,  2, 63, 0 },
    '{ 48,  1, 63, 0 },
    // Narrower sample widths
    '{ 32,  8, 47, 0 },
    '{ 16,  8, 63, 0 },
    // PRELOAD_ZEROES = 1 — exercises TC3 (clear + preload)
    '{ 32,  8, 47, 1 },
    '{ 16,  8, 63, 1 },
    '{ 32,  1, 47, 1 },
    '{ 32,  1, 63, 1 }
  };

  generate
    for (genvar i = 0; i < $size(TEST_CASES); i++) begin : gen_tb
      axis_hb_intp_fir_tb #(
        .SAMP_W        (TEST_CASES[i].samp_w),
        .SPC_OUT       (TEST_CASES[i].spc_out),
        .NUM_COEFFS    (TEST_CASES[i].num_coeffs),
        .PRELOAD_ZEROES(TEST_CASES[i].preload_zeroes)
      ) tb_i ();
    end : gen_tb
  endgenerate

endmodule : axis_hb_intp_fir_all_tb

`default_nettype wire
