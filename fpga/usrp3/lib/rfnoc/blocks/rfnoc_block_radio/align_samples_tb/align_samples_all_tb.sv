//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: align_samples_all_tb
//
// Description:
//
//   Top-level testbench for align_samples, testing different configurations of
//   the module.
//

module align_samples_all_tb;

  //---------------------------------------------------------------------------
  // Test Definitions
  //---------------------------------------------------------------------------

  typedef struct {
    int SAMP_W;
    int SPC;
    int USER_W;
    bit PIPE_IN;
    bit PIPE_OUT;
  } test_config_t;

  localparam NUM_TESTS = 6;

  localparam test_config_t tests[NUM_TESTS] = '{
    '{ SAMP_W:  8, SPC: 8, USER_W:  8, PIPE_IN: 0, PIPE_OUT: 0},
    '{ SAMP_W:  8, SPC: 4, USER_W:  8, PIPE_IN: 1, PIPE_OUT: 1},
    '{ SAMP_W:  8, SPC: 4, USER_W:  8, PIPE_IN: 1, PIPE_OUT: 0},
    '{ SAMP_W:  8, SPC: 4, USER_W:  8, PIPE_IN: 0, PIPE_OUT: 1},
    '{ SAMP_W:  8, SPC: 2, USER_W:  8, PIPE_IN: 0, PIPE_OUT: 0},
    '{ SAMP_W: 16, SPC: 2, USER_W: 16, PIPE_IN: 0, PIPE_OUT: 0}
  };


  //---------------------------------------------------------------------------
  // DUT Instances
  //---------------------------------------------------------------------------

  genvar i;
  for (i = 0; i < NUM_TESTS; i++) begin : gen_test_config
    align_samples_tb #(
      .SAMP_W  (tests[i].SAMP_W  ),
      .SPC     (tests[i].SPC     ),
      .USER_W  (tests[i].USER_W  ),
      .PIPE_IN (tests[i].PIPE_IN ),
      .PIPE_OUT(tests[i].PIPE_OUT)
    ) align_samples_tb_i ();
  end : gen_test_config

endmodule : align_samples_all_tb
