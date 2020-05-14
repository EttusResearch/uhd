//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_addsub_all_tb
//
// Description:
//
// Top-level testbench for the Add/Sub RFNoC block. This instantiates
// rfnoc_block_addsub_tb with different parameters to test multiple
// configurations.
//

`default_nettype none


module rfnoc_block_addsub_all_tb;

  // Test all three implementations
  rfnoc_block_addsub_tb #(.USE_IMPL("Verilog")) test_verilog ();
  rfnoc_block_addsub_tb #(.USE_IMPL("VHDL"))    test_vhdl    ();
  rfnoc_block_addsub_tb #(.USE_IMPL("HLS"))     test_hls     ();

endmodule : rfnoc_block_addsub_all_tb


`default_nettype wire
