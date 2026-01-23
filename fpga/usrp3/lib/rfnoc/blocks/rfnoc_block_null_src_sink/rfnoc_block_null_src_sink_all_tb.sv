//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_stream_endpoint_all_tb
//
// Description:
//
//   Testbench for rfnoc_block_null_src_sink that runs multiple CHDR widths.
//

module rfnoc_block_null_src_sink_all_tb;

  rfnoc_block_null_src_sink_tb #(.CHDR_W( 64)) CHDR64  ();
  rfnoc_block_null_src_sink_tb #(.CHDR_W(512)) CHDR512 ();

endmodule
