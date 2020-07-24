//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_switchboard_all_tb
//
// Description:
//
// Top-level testbench for the Switchboard RFNoC block. This instantiates
// rfnoc_block_switchboard_tb with different parameters to test multiple 
// configurations.
//

`default_nettype none


module rfnoc_block_switchboard_all_tb;

  // Standard test:
  rfnoc_block_switchboard_tb #(.CHDR_W( 64), .NUM_INPUTS(2), .NUM_OUTPUTS(2)) dut_0 ();
  // Multiplexer test:
  rfnoc_block_switchboard_tb #(.CHDR_W( 64), .NUM_INPUTS(2), .NUM_OUTPUTS(1)) dut_1 ();
  // Demultiplexer test:
  rfnoc_block_switchboard_tb #(.CHDR_W( 64), .NUM_INPUTS(1), .NUM_OUTPUTS(2)) dut_2 ();
  // Test multiple ports:
  rfnoc_block_switchboard_tb #(.CHDR_W( 64), .NUM_INPUTS(3), .NUM_OUTPUTS(9)) dut_3 ();
  rfnoc_block_switchboard_tb #(.CHDR_W( 64), .NUM_INPUTS(4), .NUM_OUTPUTS(12)) dut_4 ();
  rfnoc_block_switchboard_tb #(.CHDR_W( 64), .NUM_INPUTS(8), .NUM_OUTPUTS(4)) dut_5 ();
  // Test CHDR_W > 64:
  rfnoc_block_switchboard_tb #(.CHDR_W(128), .NUM_INPUTS(2), .NUM_OUTPUTS(2)) dut_6 ();
  rfnoc_block_switchboard_tb #(.CHDR_W(128), .NUM_INPUTS(16), .NUM_OUTPUTS(16)) dut_7 ();

endmodule : rfnoc_block_switchboard_all_tb


`default_nettype wire
