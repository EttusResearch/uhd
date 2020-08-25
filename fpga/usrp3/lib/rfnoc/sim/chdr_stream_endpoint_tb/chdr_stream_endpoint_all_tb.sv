//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_stream_endpoint_all_tb
//
// Description:  Testbench for chdr_stream_endpoint that runs multiple widths
//

module chdr_stream_endpoint_all_tb#(
  /* no PARAM */
)(
  /* no IO */
);

  chdr_stream_endpoint_tb #(.TEST_NAME("64B"),  .CHDR_W(64))  CHDR64  ();
  chdr_stream_endpoint_tb #(.TEST_NAME("512B"), .CHDR_W(512)) CHDR512 ();

endmodule
