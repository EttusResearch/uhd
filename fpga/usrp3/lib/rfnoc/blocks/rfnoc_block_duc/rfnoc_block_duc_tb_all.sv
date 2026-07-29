//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_duc_tb_all
//
// Description:
//   Runs rfnoc_block_duc_tb for both the single-sample (NIPC=1) and
//   multi-sample (NIPC=4) generate branches.
//

module rfnoc_block_duc_tb_all;

  // Single-sample path: NIPC=1 -> rfnoc_block_duc_ss
  rfnoc_block_duc_tb #(
    .CHDR_W (64),
    .NIPC   (1)
  ) rfnoc_block_duc_tb_ss ();

  // Multi-sample path: NIPC=4 -> rfnoc_block_duc_ms
  rfnoc_block_duc_tb #(
    .CHDR_W (64),
    .NIPC   (4)
  ) rfnoc_block_duc_tb_ms ();

endmodule
