//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: lbus_all_tb
//
// Description:
//
//   Testbench for LBU<->AXI
//

module lbus_all_tb #(
  /* no PARAM */
)(
  /* no IO */
);

  lbus_axi_tb #(.TEST_NAME("L2A")) L2A ();
  axi_lbus_tb #(.TEST_NAME("A2L")) A2L ();

endmodule
