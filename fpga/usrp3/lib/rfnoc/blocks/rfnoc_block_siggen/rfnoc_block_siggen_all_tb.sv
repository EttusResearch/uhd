//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_logpwr_all_tb
//
// Description: Top-level testbench for the Signal Generator RFNoC block. This
// instantiates rfnoc_block_siggen_tb with different parameters to test
// multiple configurations.
//

`default_nettype none


module rfnoc_block_siggen_all_tb;

  // Test multiple CHDR widths
  rfnoc_block_siggen_tb #(.CHDR_W(64),  .NUM_PORTS(1))  test_siggen_0();
  rfnoc_block_siggen_tb #(.CHDR_W(64),  .NUM_PORTS(2))  test_siggen_1();
  rfnoc_block_siggen_tb #(.CHDR_W(64),  .NUM_PORTS(3))  test_siggen_2();
  rfnoc_block_siggen_tb #(.CHDR_W(128), .NUM_PORTS(2))  test_siggen_3();
  rfnoc_block_siggen_tb #(.CHDR_W(256), .NUM_PORTS(1))  test_siggen_4();

endmodule : rfnoc_block_siggen_all_tb


`default_nettype wire
