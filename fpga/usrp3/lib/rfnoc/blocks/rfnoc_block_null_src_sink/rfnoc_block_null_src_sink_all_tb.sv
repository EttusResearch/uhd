//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_stream_endpoint_all_tb
//
// Description:  Testbench for chdr_stream_endpoint that runs multiple widths
//

module rfnoc_block_null_src_sink_all_tb#(
  /* no PARAM */
)(
  /* no IO */
);

  rfnoc_block_null_src_sink_tb #(.TEST_NAME("64B"),.CHDR_W(64)) CHDR64 ();
  rfnoc_block_null_src_sink_tb #(.TEST_NAME("512B"),.CHDR_W(512)) CHDR512 ();

  // Wait for all done
  bit clk,rst;
  sim_clock_gen #(100.0) clk_gen (clk, rst);  
  always_ff@(posedge clk)
    if (CHDR64.test.done && CHDR512.test.done) $finish(1);
    
endmodule
