//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_filter_interp_all_tb
//
// Description:
//
//   Top-level testbench that instantiates multiple parameterized variants of
//   cic_filter_interp_tb to cover different SPC/COMP_W/ORDER configurations
//   in a single simulation run.
//

`default_nettype none

module cic_filter_interp_all_tb;

  cic_filter_interp_tb #(
    .SPC       (2),
    .COMP_W    (16),
    .ORDER     (3),
    .MAX_INTERP(63)
  ) tb_0 ();

  cic_filter_interp_tb #(
    .SPC       (4),
    .COMP_W    (16),
    .ORDER     (4),
    .MAX_INTERP(33)
  ) tb_1 ();

  cic_filter_interp_tb #(
    .SPC       (2),
    .COMP_W    (16),
    .ORDER     (4),
    .MAX_INTERP(22)
  ) tb_2 ();

  cic_filter_interp_tb #(
    .SPC       (8),
    .COMP_W    (16),
    .ORDER     (4),
    .MAX_INTERP(63)
  ) tb_3 ();

  cic_filter_interp_tb #(
    .SPC       (4),
    .COMP_W    (12),
    .ORDER     (3),
    .MAX_INTERP(64)
  ) tb_4 ();

  cic_filter_interp_tb #(
    .SPC       (1),
    .COMP_W    (16),
    .ORDER     (4),
    .MAX_INTERP(127)
  ) tb_5 ();

endmodule : cic_filter_interp_all_tb

`default_nettype wire
