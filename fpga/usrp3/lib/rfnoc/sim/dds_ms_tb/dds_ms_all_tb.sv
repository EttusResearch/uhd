//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: dds_ms_all_tb
//
// Description: This is a wrapper for the dds_ms module testbench
// instantiated with different SPC values.
//

module dds_ms_all_tb;

  // Instantiate the testbench with desired parameters
  dds_ms_tb #(
    .SPC(1)
  ) tb_1 ();

  dds_ms_tb #(
    .SPC(2)
  ) tb_2 ();

  dds_ms_tb #(
    .SPC(4)
  ) tb_4 ();

  dds_ms_tb #(
    .SPC(8)
  ) tb_8 ();

endmodule
