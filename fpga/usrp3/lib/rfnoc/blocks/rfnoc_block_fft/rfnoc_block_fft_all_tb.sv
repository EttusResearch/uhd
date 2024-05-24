//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_fft_all_tb
//
// Description:
//
//   This is the testbench for rfnoc_block_fft that instantiates several
//   variations of the testbench to test different configurations.
//


module rfnoc_block_fft_all_tb;

  //---------------------------------------------------------------------------
  // Test Configurations
  //---------------------------------------------------------------------------

  // Basic tests of multi-ports configurations
  rfnoc_block_fft_tb #(.FULL_TEST(0), .NUM_PORTS(2), .NUM_CORES(1), .MAX_FFT_SIZE_LOG2(10)) tb_0a ();
  rfnoc_block_fft_tb #(.FULL_TEST(0), .NUM_PORTS(2), .NUM_CORES(2), .MAX_FFT_SIZE_LOG2(10)) tb_0b ();
  rfnoc_block_fft_tb #(.FULL_TEST(0), .NUM_PORTS(4), .NUM_CORES(2), .MAX_FFT_SIZE_LOG2(10)) tb_0c ();

  // Basic tests of other FFT sizes
  rfnoc_block_fft_tb #(.FULL_TEST(0), .NUM_PORTS(1), .NUM_CORES(1), .MAX_FFT_SIZE_LOG2(11)) tb_1a ();
  rfnoc_block_fft_tb #(.FULL_TEST(0), .NUM_PORTS(1), .NUM_CORES(1), .MAX_FFT_SIZE_LOG2(12)) tb_1b ();
  rfnoc_block_fft_tb #(.FULL_TEST(0), .NUM_PORTS(1), .NUM_CORES(1), .MAX_FFT_SIZE_LOG2(13)) tb_1c ();
  rfnoc_block_fft_tb #(.FULL_TEST(0), .NUM_PORTS(1), .NUM_CORES(1), .MAX_FFT_SIZE_LOG2(14)) tb_1d ();
  rfnoc_block_fft_tb #(.FULL_TEST(0), .NUM_PORTS(1), .NUM_CORES(1), .MAX_FFT_SIZE_LOG2(15)) tb_1e ();
  rfnoc_block_fft_tb #(.FULL_TEST(0), .NUM_PORTS(1), .NUM_CORES(1), .MAX_FFT_SIZE_LOG2(16)) tb_1f ();

  // Test case where USE_APPROX_MAG = 1
  rfnoc_block_fft_tb #(.FULL_TEST(0), .NUM_PORTS(1), .NUM_CORES(1), .MAX_FFT_SIZE_LOG2(10),
    .EN_FFT_BYPASS(1), .EN_MAGNITUDE(1), .USE_APPROX_MAG(1)) tb_2a ();

  // Run full suite of tests on 1k FFT configuration
  rfnoc_block_fft_tb #(
    .FULL_TEST               (1 ),
    .NUM_PORTS               (1 ),
    .NUM_CORES               (1 ),
    .MAX_FFT_SIZE_LOG2       (10),
    .MAX_CP_LIST_LEN_INS_LOG2(5 ),
    .MAX_CP_LIST_LEN_REM_LOG2(5 ),
    .EN_MAGNITUDE_SQ         (1 ),
    .EN_MAGNITUDE            (1 ),
    .EN_FFT_BYPASS           (1 ),
    .USE_APPROX_MAG          (0 )
  ) tb_3a ();

endmodule : rfnoc_block_fft_all_tb
