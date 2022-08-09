//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_resize_all_tb
//
// Description:
//
//   Top-level testbench for chdr_resize, testing different configurations of
//   the module.
//

module chdr_resize_all_tb;
  // Up-size CHDR bus
  chdr_resize_tb #(.I_CHDR_W( 64), .O_CHDR_W(128), .PIPELINE("NONE" )) chdr_resize_tb_00();
  chdr_resize_tb #(.I_CHDR_W( 64), .O_CHDR_W(256), .PIPELINE("IN"   )) chdr_resize_tb_01();
  chdr_resize_tb #(.I_CHDR_W(128), .O_CHDR_W(256), .PIPELINE("OUT"  )) chdr_resize_tb_02();
  chdr_resize_tb #(.I_CHDR_W(128), .O_CHDR_W(512), .PIPELINE("INOUT")) chdr_resize_tb_03();
  chdr_resize_tb #(.I_CHDR_W(256), .O_CHDR_W(512), .PIPELINE("NONE" )) chdr_resize_tb_04();
  // Down-size CHDR bus
  chdr_resize_tb #(.I_CHDR_W(128), .O_CHDR_W( 64), .PIPELINE("NONE" )) chdr_resize_tb_10();
  chdr_resize_tb #(.I_CHDR_W(256), .O_CHDR_W( 64), .PIPELINE("IN"   )) chdr_resize_tb_11();
  chdr_resize_tb #(.I_CHDR_W(256), .O_CHDR_W(128), .PIPELINE("OUT"  )) chdr_resize_tb_12();
  chdr_resize_tb #(.I_CHDR_W(512), .O_CHDR_W(128), .PIPELINE("INOUT")) chdr_resize_tb_13();
  chdr_resize_tb #(.I_CHDR_W(512), .O_CHDR_W(256), .PIPELINE("NONE" )) chdr_resize_tb_14();

  // Up-size CHDR encoding (keep bus width)
  chdr_resize_tb #(.I_CHDR_W( 64), .O_CHDR_W(128), .I_DATA_W( 64), .O_DATA_W( 64), .PIPELINE("NONE" )) chdr_resize_tb_20();
  chdr_resize_tb #(.I_CHDR_W( 64), .O_CHDR_W(512), .I_DATA_W( 64), .O_DATA_W( 64), .PIPELINE("IN"   )) chdr_resize_tb_21();
  chdr_resize_tb #(.I_CHDR_W(128), .O_CHDR_W(256), .I_DATA_W(128), .O_DATA_W(128), .PIPELINE("OUT"  )) chdr_resize_tb_22();
  chdr_resize_tb #(.I_CHDR_W(128), .O_CHDR_W(512), .I_DATA_W(128), .O_DATA_W(128), .PIPELINE("INOUT")) chdr_resize_tb_23();
  chdr_resize_tb #(.I_CHDR_W(256), .O_CHDR_W(512), .I_DATA_W(256), .O_DATA_W(256), .PIPELINE("NONE" )) chdr_resize_tb_24();
  // Down-size CHDR encoding (keep bus width)
  chdr_resize_tb #(.I_CHDR_W(128), .O_CHDR_W( 64), .I_DATA_W( 64), .O_DATA_W( 64), .PIPELINE("NONE" )) chdr_resize_tb_30();
  chdr_resize_tb #(.I_CHDR_W(512), .O_CHDR_W( 64), .I_DATA_W( 64), .O_DATA_W( 64), .PIPELINE("IN"   )) chdr_resize_tb_31();
  chdr_resize_tb #(.I_CHDR_W(256), .O_CHDR_W(128), .I_DATA_W(128), .O_DATA_W(128), .PIPELINE("OUT"  )) chdr_resize_tb_32();
  chdr_resize_tb #(.I_CHDR_W(512), .O_CHDR_W(128), .I_DATA_W(128), .O_DATA_W(128), .PIPELINE("INOUT")) chdr_resize_tb_33();
  chdr_resize_tb #(.I_CHDR_W(512), .O_CHDR_W(256), .I_DATA_W(256), .O_DATA_W(256), .PIPELINE("NONE" )) chdr_resize_tb_34();

  // No resize (pass through)
  chdr_resize_tb #(.I_CHDR_W(512), .O_CHDR_W(512), .PIPELINE("NONE" )) chdr_resize_tb_40();
  chdr_resize_tb #(.I_CHDR_W(256), .O_CHDR_W(256), .PIPELINE("IN"   )) chdr_resize_tb_41();
  chdr_resize_tb #(.I_CHDR_W(128), .O_CHDR_W(128), .PIPELINE("OUT"  )) chdr_resize_tb_42();
  chdr_resize_tb #(.I_CHDR_W( 64), .O_CHDR_W( 64), .PIPELINE("INOUT")) chdr_resize_tb_43();
endmodule : chdr_resize_all_tb
