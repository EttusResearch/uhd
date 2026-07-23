//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport_spi_adrv_all_tb
//
// Description:
//
//   Top-level testbench for ctrlport_spi_adrv, exercising multiple DUT
//   configurations. Each instance runs sequentially via the semaphore in
//   PkgTestExec::test.
//

`default_nettype none

module ctrlport_spi_adrv_all_tb;

  ctrlport_spi_adrv_tb #(.QUICK_TEST(1), .NUM_BYTES_W( 4), .HALF_PER(  0), .HALF_PER_EN(0), .CS_HOLD(0), .CS_GUARD(1)) tb_hold_guard_01 ();
  ctrlport_spi_adrv_tb #(.QUICK_TEST(1), .NUM_BYTES_W( 4), .HALF_PER(  0), .HALF_PER_EN(0), .CS_HOLD(1), .CS_GUARD(1)) tb_hold_guard_11 ();
  ctrlport_spi_adrv_tb #(.QUICK_TEST(1), .NUM_BYTES_W( 4), .HALF_PER(  0), .HALF_PER_EN(0), .CS_HOLD(0), .CS_GUARD(2)) tb_hold_guard_02 ();
  ctrlport_spi_adrv_tb #(.QUICK_TEST(1), .NUM_BYTES_W( 4), .HALF_PER(  0), .HALF_PER_EN(0), .CS_HOLD(1), .CS_GUARD(2)) tb_hold_guard_12 ();
  ctrlport_spi_adrv_tb #(.QUICK_TEST(1), .NUM_BYTES_W( 4), .HALF_PER(255), .HALF_PER_EN(0), .CS_HOLD(1), .CS_GUARD(2)) tb_slow_clk      ();
  ctrlport_spi_adrv_tb #(.QUICK_TEST(1), .NUM_BYTES_W(12), .HALF_PER(  0), .HALF_PER_EN(1), .CS_HOLD(1), .CS_GUARD(2)) tb_max_len       ();
  ctrlport_spi_adrv_tb #(.QUICK_TEST(0), .NUM_BYTES_W( 8), .HALF_PER(  4), .HALF_PER_EN(1), .CS_HOLD(1), .CS_GUARD(2)) tb_full_test     ();

endmodule : ctrlport_spi_adrv_all_tb

`default_nettype wire
