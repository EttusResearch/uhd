//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fft_reorder_all_tb
//
// Description:
//
//   Top-level testbench for fft_reorder_tb, testing different configurations of
//   the module.
//

module fft_reorder_all_tb;
  import fft_reorder_pkg::*;

  // Test different input orders. Do a larger size for bit reverse to test a
  // larger memory.
  fft_reorder_tb #(.INPUT_ORDER(    NATURAL), .MAX_FFT_LEN_LOG2( 6)) tb_01();
  fft_reorder_tb #(.INPUT_ORDER(BIT_REVERSE), .MAX_FFT_LEN_LOG2(12)) tb_02();

  // Test different input FIFO configs. Use smaller size for quicker test.
  fft_reorder_tb #(.IN_FIFO_LOG2(-1), .OUT_FIFO_LOG2(3), .INPUT_ORDER(NATURAL),     .MAX_FFT_LEN_LOG2(5)) tb_03();
  fft_reorder_tb #(.IN_FIFO_LOG2( 0), .OUT_FIFO_LOG2(5), .INPUT_ORDER(BIT_REVERSE), .MAX_FFT_LEN_LOG2(5)) tb_04();
  fft_reorder_tb #(.IN_FIFO_LOG2( 1), .OUT_FIFO_LOG2(6), .INPUT_ORDER(NATURAL),     .MAX_FFT_LEN_LOG2(5)) tb_05();

endmodule : fft_reorder_all_tb
