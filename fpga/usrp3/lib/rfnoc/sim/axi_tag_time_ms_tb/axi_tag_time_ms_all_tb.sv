//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_tag_time_ms_all_tb
//
// Description: This is a wrapper for the axi_tag_time_ms module testbench
// instantiated with different SPC values.
//

`default_nettype none

module axi_tag_time_ms_all_tb;

  // Instantiate the testbench with desired parameters
  axi_tag_time_ms_tb #(
    .SPC(1)
  ) tb_1 ();

  axi_tag_time_ms_tb #(
    .SPC(2)
  ) tb_2 ();

  axi_tag_time_ms_tb #(
    .SPC(4)
  ) tb_4 ();

  axi_tag_time_ms_tb #(
    .SPC(8)
  ) tb_8 ();

endmodule
`default_nettype wire
