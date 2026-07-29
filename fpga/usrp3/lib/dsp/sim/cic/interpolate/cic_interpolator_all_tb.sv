//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: cic_interpolator_all_tb
//
// Description:
//
//   Testbench that instantiates multiple parametrized cic_interpolator_tb instances.
//

module cic_interpolator_all_tb;

  cic_interpolator_tb #(.SPC(1), .SAMP_W(18), .R_MAX(15)) tb_0();
  cic_interpolator_tb #(.SPC(2), .SAMP_W(16), .R_MAX(16)) tb_1();
  cic_interpolator_tb #(.SPC(4), .SAMP_W(32), .R_MAX(18)) tb_2();
  cic_interpolator_tb #(.SPC(8), .SAMP_W( 4), .R_MAX(17)) tb_3();

endmodule : cic_interpolator_all_tb
