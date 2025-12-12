//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: timed_complex_gain_tb_all
//
// Description: This is a wrapper for different instances of the
//              timed_complex_gain module testbench.
//

module timed_complex_gain_tb_all;

  // Instantiate the testbench with desired parameters
  timed_complex_gain_tb #(  // Default parameters
      .ITEM_W(32),
      .NIPC(1),
      .COEFF_W(16),
      .COEFF_FRAC_BITS(14),
      .STB_STALL_PROB(20),
      .EN_FIFO_OUT_REG(0)
  ) tb_0 ();

  timed_complex_gain_tb #(
      .ITEM_W(32),
      .NIPC(1),
      .COEFF_W(16),
      .COEFF_FRAC_BITS(8),
      .STB_STALL_PROB(60),
      .EN_FIFO_OUT_REG(1)
  ) tb_1 ();

  timed_complex_gain_tb #(  // COEFF_W 16, COEFF_FRAC_BITS = 12
      .ITEM_W(32),
      .NIPC(1),
      .COEFF_W(16),
      .COEFF_FRAC_BITS(12),
      .STB_STALL_PROB(20),
      .EN_FIFO_OUT_REG(1)
  ) tb_2 ();

  timed_complex_gain_tb #(  // COEFF_W 8, COEFF_FRAC_BITS = 4
      .ITEM_W(32),
      .NIPC(1),
      .COEFF_W(8),
      .COEFF_FRAC_BITS(4),
      .STB_STALL_PROB(20),
      .EN_FIFO_OUT_REG(0)
  ) tb_3 ();

  timed_complex_gain_tb #(  // ITEM_W = 16 (sc8 data)
      .ITEM_W(16),
      .NIPC(1),
      .COEFF_W(16),
      .COEFF_FRAC_BITS(14),
      .STB_STALL_PROB(20),
      .EN_FIFO_OUT_REG(1)
  ) tb_4 ();
endmodule
