//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_split_stream_all_tb
//
// Description:
//
// Top-level testbench for the split_stream RFNoC block. This instantiates
// rfnoc_block_split_stream_tb with different parameters to test multiple
// configurations.
//

`default_nettype none


module rfnoc_block_split_stream_all_tb;

  // Standard test:
  rfnoc_block_split_stream_tb #(.CHDR_W( 64), .NUM_PORTS(1), .NUM_BRANCHES(2)) dut_0 ();
  // Test multiple ports:
  rfnoc_block_split_stream_tb #(.CHDR_W( 64), .NUM_PORTS(2), .NUM_BRANCHES(2)) dut_1 ();
  // Test NUM_BRANCH > 2:
  rfnoc_block_split_stream_tb #(.CHDR_W( 64), .NUM_PORTS(2), .NUM_BRANCHES(3)) dut_2 ();
  // Test CHDR_W > 64:
  rfnoc_block_split_stream_tb #(.CHDR_W(128), .NUM_PORTS(1), .NUM_BRANCHES(2)) dut_3 ();
  rfnoc_block_split_stream_tb #(.CHDR_W(128), .NUM_PORTS(2), .NUM_BRANCHES(2)) dut_4 ();

endmodule : rfnoc_block_split_stream_all_tb


`default_nettype wire
