//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_comb_filter_all_tb
//
// Description:
//
//   Testbench for instantiating cic_comb_filter testbenches.
//
// Parameters:
//
//

`default_nettype none

module cic_comb_filter_all_tb;

  // Include macros and time declarations for use with PkgTestExec
  `include "test_exec.svh"
  import PkgTestExec::*;

  //---------------------------------------------------------------------------
  // Testbench parameters and type definitions
  //---------------------------------------------------------------------------
  localparam int NUM_TESTS = 10;

  //---------------------------------------------------------------------------
  // Testbench instances
  //---------------------------------------------------------------------------
  cic_comb_filter_tb #(
    .SPC(2),
    .ACCUM_W(96),
    .SAMP_W(32)
  ) tb_0();

  cic_comb_filter_tb #(
    .SPC(4),
    .ACCUM_W(96),
    .SAMP_W(32)
  ) tb_1();

  cic_comb_filter_tb #(
    .SPC(8),
    .ACCUM_W(96),
    .SAMP_W(32)
  ) tb_2();

  cic_comb_filter_tb #(
    .SPC(1),
    .ACCUM_W(96),
    .SAMP_W(32)
  ) tb_3();

  cic_comb_filter_tb #(
    .SPC(8),
    .ACCUM_W(48),
    .SAMP_W(16)
  ) tb_4();

endmodule

`default_nettype wire
