//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_rate_change_ms_all_tb
//
// Description:
//
//   Top-level testbench wrapper for axi_rate_change_ms_tb. Instantiates the
//   testbench with different WIDTH, MAX_N, and MAX_M parameter combinations.
//

`default_nettype none

module axi_rate_change_ms_all_tb;

  // Default configuration: 32-bit width, max rate 16
  axi_rate_change_ms_tb #(
    .WIDTH(32),
    .MAX_N(16),
    .MAX_M(16)
  ) tb_1 ();

  // Wider data path
  axi_rate_change_ms_tb #(
    .WIDTH(64),
    .MAX_N(16),
    .MAX_M(16)
  ) tb_2 ();

  // Asymmetric max rates
  axi_rate_change_ms_tb #(
    .WIDTH(32),
    .MAX_N(1),
    .MAX_M(255)
  ) tb_3 ();

  // Large max decimation rate
  axi_rate_change_ms_tb #(
    .WIDTH(32),
    .MAX_N(255),
    .MAX_M(1)
  ) tb_4 ();

endmodule : axi_rate_change_ms_all_tb

`default_nettype wire
