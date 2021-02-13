//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: gearbox_2x1_all_tb
//
// Description: Top-level testbench for gearbox_2x1, testing different
// configurations of the module.
//

module gearbox_2x1_all_tb;
  gearbox_2x1_tb #(.IN_WORDS(1), .OUT_WORDS(2), .BIG_ENDIAN(0), .PHASE(0)) gearbox_2x1_tb_1();
  gearbox_2x1_tb #(.IN_WORDS(2), .OUT_WORDS(1), .BIG_ENDIAN(0), .PHASE(0)) gearbox_2x1_tb_2();
  gearbox_2x1_tb #(.IN_WORDS(2), .OUT_WORDS(4), .BIG_ENDIAN(1), .PHASE(0)) gearbox_2x1_tb_3();
  gearbox_2x1_tb #(.IN_WORDS(4), .OUT_WORDS(2), .BIG_ENDIAN(1), .PHASE(0)) gearbox_2x1_tb_4();
  gearbox_2x1_tb #(.IN_WORDS(1), .OUT_WORDS(2), .BIG_ENDIAN(0), .PHASE(1)) gearbox_2x1_tb_5();
  gearbox_2x1_tb #(.IN_WORDS(2), .OUT_WORDS(1), .BIG_ENDIAN(0), .PHASE(1)) gearbox_2x1_tb_6();
endmodule : gearbox_2x1_all_tb
