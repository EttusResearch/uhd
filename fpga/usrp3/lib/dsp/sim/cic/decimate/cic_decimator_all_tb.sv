//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_decimator_all_tb
//
// Description:
//   Testbench that instantiates multiple parametrized cic_decimator_tb instances.
//

`default_nettype none
module cic_decimator_all_tb;

  // Instantiate the DUT
  cic_decimator_tb #(
    .SPC   (8),
    .SAMP_W(48)
  ) tb_0 ();
  // Instantiate the DUT
  cic_decimator_tb #(
    .SPC   (4),
    .SAMP_W(48)
  ) tb_1 ();
  // Instantiate the DUT
  cic_decimator_tb #(
    .SPC   (2),
    .SAMP_W(48)
  ) tb_2 ();
  // Instantiate the DUT
  cic_decimator_tb #(
    .SPC   (1),
    .SAMP_W(48)
  ) tb_3 ();
  // Instantiate the DUT
  cic_decimator_tb #(
    .SPC   (8),
    .SAMP_W(32)
  ) tb_4 ();
  // Instantiate the DUT
  cic_decimator_tb #(
    .SPC   (8),
    .SAMP_W(16)
  ) tb_5 ();
endmodule

`default_nettype wire
