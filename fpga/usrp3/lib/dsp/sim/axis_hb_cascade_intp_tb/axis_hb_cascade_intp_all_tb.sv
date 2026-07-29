//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_hb_cascade_intp_all_tb
//
// Description: Wrapper to run all AXI halfband filter cascade interpolator TB configs.
//

`default_nettype none

module axis_hb_cascade_intp_all_tb ();

  typedef struct {
    int samp_w;
    int spc;
    int num_hb;
    bit preload_zeroes;
  } test_case_t;

  localparam test_case_t TEST_CASES [9] = '{
    // SAMP_W SPC NUM_HB PRELOAD_ZEROES
    // Few TC for preload zeroes = 0 (main use case is preload zeroes=1)
    '{32, 1, 3, 0},
    '{48, 4, 2, 0},
    // Vary num_stages
    '{32, 2, 0, 1}, // Check zero-stage case (since user could set NUM_HB=0)
    '{32, 2, 1, 1},
    '{32, 2, 2, 1},
    '{32, 2, 3, 1},
    // Vary spc
    '{32, 1, 3, 1},
    '{32, 2, 2, 1},
    '{32, 8, 1, 1}
  };

  generate
    for (genvar i = 0; i < $size(TEST_CASES); i++) begin : gen_tb
      axis_hb_cascade_intp_tb #(
        .SAMP_W        (TEST_CASES[i].samp_w),
        .SPC           (TEST_CASES[i].spc),
        .NUM_HB        (TEST_CASES[i].num_hb),
        .PRELOAD_ZEROES(TEST_CASES[i].preload_zeroes)
      ) tb_i ();
    end : gen_tb
  endgenerate

endmodule : axis_hb_cascade_intp_all_tb

`default_nettype wire
