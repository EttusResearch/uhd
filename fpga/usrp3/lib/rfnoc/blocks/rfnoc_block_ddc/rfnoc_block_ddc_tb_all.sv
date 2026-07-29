//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_ddc_tb_all
//
// Description:
//   Runs rfnoc_block_ddc_tb for both the single-sample (NIPC=1, gen_single_spc)
//   and multi-sample (NIPC=4, gen_multi_spc) generate branches.
//

module rfnoc_block_ddc_tb_all;

  // Single-sample path: NIPC=1 → gen_single_spc → rfnoc_block_ddc_ss
  rfnoc_block_ddc_tb #(
    .CHDR_W (64),
    .NIPC   (1)
  ) rfnoc_block_ddc_tb_ss ();

  // Multi-sample path: NIPC=4 → gen_multi_spc → rfnoc_block_ddc_ms
  rfnoc_block_ddc_tb #(
    .CHDR_W (64),
    .NIPC   (4)
  ) rfnoc_block_ddc_tb_ms ();

endmodule
