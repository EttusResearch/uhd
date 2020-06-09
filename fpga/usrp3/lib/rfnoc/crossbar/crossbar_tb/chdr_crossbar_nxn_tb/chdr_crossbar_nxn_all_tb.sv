//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_crossbar_nxn
//
// Description:  Testbench for chdr_crossbar_nxn that runs multiple widths
//

module chdr_crossbar_nxn_all_tb#(
  /* no PARAM */
)(
  /* no IO */
);

  chdr_crossbar_nxn_tb #(.TEST_NAME("64B"),.CHDR_W(64)) CHDR64 ();
  chdr_crossbar_nxn_tb #(.TEST_NAME("512B"),.CHDR_W(512)) CHDR512 ();

  // Wait for all done
  bit clk,rst;
  sim_clock_gen #(100.0) clk_gen (clk, rst);  
  always_ff@(posedge clk)
    if (CHDR64.impl.done && CHDR512.impl.done) $finish(1);
    
endmodule
