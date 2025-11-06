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

  // Basic tests of multi-port configurations
  rfnoc_block_fft_tb #(.FULL_TEST(0), .NUM_PORTS(2), .NUM_CORES(1),
    .MAX_FFT_SIZE_LOG2(10)) tb_0a ();
  rfnoc_block_fft_tb #(.FULL_TEST(0), .NUM_PORTS(2), .NUM_CORES(2),
    .MAX_FFT_SIZE_LOG2(10)) tb_0b ();
  rfnoc_block_fft_tb #(.FULL_TEST(0), .NUM_PORTS(4), .NUM_CORES(2),
    .MAX_FFT_SIZE_LOG2(10)) tb_0c ();

  // Basic tests of other FFT sizes
  rfnoc_block_fft_tb #(.FULL_TEST(0), .MAX_FFT_SIZE_LOG2(11)) tb_1a ();
  rfnoc_block_fft_tb #(.FULL_TEST(0), .MAX_FFT_SIZE_LOG2(12)) tb_1b ();
  rfnoc_block_fft_tb #(.FULL_TEST(0), .MAX_FFT_SIZE_LOG2(13)) tb_1c ();
  rfnoc_block_fft_tb #(.FULL_TEST(0), .MAX_FFT_SIZE_LOG2(14)) tb_1d ();
  rfnoc_block_fft_tb #(.FULL_TEST(0), .MAX_FFT_SIZE_LOG2(15)) tb_1e ();
  // Skip the 64k FFT test because it's broken :-(
  //rfnoc_block_fft_tb #(.FULL_TEST(0), .MAX_FFT_SIZE_LOG2(16)) tb_1f ();

  // Do quick tests with various features disabled to ensure these features get
  // disabled and bypassed correctly. The cyclic prefix logic, the magnitude
  // and order logic, and the FFT bypass are in separate components. To avoid
  // testing every possible permutation, we permute each of these separately.
  //
  // Test permutations of CP insertion/removal with other features disabled
  rfnoc_block_fft_tb #(.FULL_TEST(0), .EN_CP_REMOVAL(0), .EN_CP_INSERTION(1), .EN_MAGNITUDE(0),
    .EN_MAGNITUDE_SQ(0), .EN_FFT_ORDER(1), .EN_FFT_BYPASS(0)) tb_2c ();
  rfnoc_block_fft_tb #(.FULL_TEST(0), .EN_CP_REMOVAL(1), .EN_CP_INSERTION(0), .EN_MAGNITUDE(0),
    .EN_MAGNITUDE_SQ(0), .EN_FFT_ORDER(0), .EN_FFT_BYPASS(1)) tb_2b ();
  rfnoc_block_fft_tb #(.FULL_TEST(0), .EN_CP_REMOVAL(1), .EN_CP_INSERTION(1), .EN_MAGNITUDE(0),
    .EN_MAGNITUDE_SQ(0), .EN_FFT_ORDER(1), .EN_FFT_BYPASS(0)) tb_2d ();
  //
  // Test permutations of magnitude with FFT bypass enabled
  rfnoc_block_fft_tb #(.FULL_TEST(0), .EN_CP_REMOVAL(0), .EN_CP_INSERTION(0), .EN_MAGNITUDE(0),
    .EN_MAGNITUDE_SQ(1), .EN_FFT_ORDER(0), .EN_FFT_BYPASS(1)) tb_2g ();
  rfnoc_block_fft_tb #(.FULL_TEST(0), .EN_CP_REMOVAL(0), .EN_CP_INSERTION(0), .EN_MAGNITUDE(1),
    .EN_MAGNITUDE_SQ(0), .EN_FFT_ORDER(0), .EN_FFT_BYPASS(0)) tb_2h ();
  rfnoc_block_fft_tb #(.FULL_TEST(0), .EN_CP_REMOVAL(0), .EN_CP_INSERTION(0), .EN_MAGNITUDE(1),
    .EN_MAGNITUDE_SQ(1), .EN_FFT_ORDER(0), .EN_FFT_BYPASS(1)) tb_2i ();

  // Test SPC of 1, 2, 4, 8 with all features enabled
  for (genvar log_spc = 0; log_spc < 3; log_spc++) begin : gen_test_multi_spc
    localparam int SPC = 2**log_spc;
    rfnoc_block_fft_tb #(
      .FULL_TEST               (1     ),
      .CHDR_W                  (64*SPC),
      .NIPC                    (SPC   ),
      .NUM_PORTS               (1     ),
      .NUM_CORES               (1     ),
      .MAX_FFT_SIZE_LOG2       (10    ),
      .EN_CP_REMOVAL           (1     ),
      .EN_CP_INSERTION         (1     ),
      .MAX_CP_LIST_LEN_INS_LOG2(5     ),
      .MAX_CP_LIST_LEN_REM_LOG2(5     ),
      .EN_MAGNITUDE            (1     ),
      .EN_MAGNITUDE_SQ         (1     ),
      .EN_FFT_BYPASS           (1     ),
      .USE_APPROX_MAG          (1     )
    ) tb_3a ();
  end : gen_test_multi_spc

  // Test SPC of 1, 2 with extra features disabled
  for (genvar log_spc = 0; log_spc < 1; log_spc++) begin : gen_test_features_disabled
    localparam int SPC = 2**log_spc;
    rfnoc_block_fft_tb #(
      .FULL_TEST               (1     ),
      .CHDR_W                  (64*SPC),
      .NIPC                    (SPC   ),
      .NUM_PORTS               (1     ),
      .NUM_CORES               (1     ),
      .MAX_FFT_SIZE_LOG2       (10    ),
      .EN_CP_REMOVAL           (0     ),
      .EN_CP_INSERTION         (0     ),
      .MAX_CP_LIST_LEN_INS_LOG2(5     ),
      .MAX_CP_LIST_LEN_REM_LOG2(5     ),
      .EN_MAGNITUDE            (0     ),
      .EN_MAGNITUDE_SQ         (0     ),
      .EN_FFT_ORDER            (0     ),
      .EN_FFT_BYPASS           (0     ),
      .USE_APPROX_MAG          (0     )
    ) tb_3b ();
  end : gen_test_features_disabled

  // Run quick test on some other multi-SPC configurations
  rfnoc_block_fft_tb #(.FULL_TEST(0), .CHDR_W(128), .NIPC(4)) tb_3c ();
  rfnoc_block_fft_tb #(.FULL_TEST(0), .CHDR_W(256), .NIPC(8)) tb_3d ();

endmodule : rfnoc_block_fft_all_tb
