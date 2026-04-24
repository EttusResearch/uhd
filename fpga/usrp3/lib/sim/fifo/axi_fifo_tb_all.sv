//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_fifo_tb_all
//
// Description:
//
//   Wrapper testbench for axi_fifo testing.
//

module axi_fifo_tb_all;

  // --------------------------------------------------------------
  // Standard FIFO testing
  // --------------------------------------------------------------
  axi_fifo_tb #() axi_fifo_tb_i();

  // --------------------------------------------------------------
  // URAM testing
  // --------------------------------------------------------------
  localparam int URAM_TEST_DEPTH[3] = '{4096, 4096*3, 4096*5};
  for (genvar i = 0; i < $size(URAM_TEST_DEPTH); i++) begin : gen_uram_tb
    axi_fifo_uram_tb #(
      .WIDTH(72),
      .DEPTH(URAM_TEST_DEPTH[i])
    ) axi_fifo_uram_tb_i ();
  end : gen_uram_tb

  // --------------------------------------------------------------
  // Large FIFO testing
  // --------------------------------------------------------------
  localparam int NUM_URAM_BLOCKS[5] = '{-1, 0, 2, 8, 20};
  localparam int WIDTHS[3] = '{64, 144, 256};
  // Depth subset chosen to exercise every structural code path in axi_fifo_large:
  //   - DEPTH <= 4096 (ULTRASCALE SIZE_THRESHOLD): single axi_fifo path
  //   - DEPTH > 4096: URAM + BRAM cascade
  //     * BRAM stages are driven by bits [13:9] of REMAINING_DEPTH_CEIL where
  //       REMAINING = DEPTH mod 4096.  The 8 unique bit patterns occur at
  //       REMAINING in {0, 512, 1024, 1536, 2048, 2560, 3072, 3584}.
  //     * URAM count = DEPTH / 4096; tested at 1, 2, 4, 8, 16 blocks.
  localparam int DEPTHS[24] = '{
    // Single-FIFO path (DEPTH <= ULTRASCALE threshold of 4096)
    512, 1024, 2048, 4096,
    // 1 URAM block + all 8 BRAM remainder patterns (REMAINING = 0..3584 step 512)
    // REMAINING=0 is the pure-URAM case; the rest add 1-3 small BRAM stages
    4096, 4608, 5120, 5632, 6144, 6656, 7168, 7680,
    // Higher URAM counts, pure-URAM (no BRAM remainder)
    8192, 16384, 32768,
    // Non-512-aligned depths: verify EXPECTED_SPACE rounds up to next 512 boundary
    100, 511, 513, 1000, 4097, 6000, 8456, 9103,
    // Large number
    2**16+512
  };

  for (genvar i = 0; i < $size(NUM_URAM_BLOCKS); i++) begin : gen_uram
    for (genvar j = 0; j < $size(WIDTHS); j++) begin : gen_width
      for (genvar k = 0; k < $size(DEPTHS); k++) begin : gen_depth
        axi_fifo_large_tb #(
          .WIDTH(WIDTHS[j]),
          .DEPTH(DEPTHS[k]),
          .DEVICE("ULTRASCALE"),
          .MAX_NUM_URAM_BLOCKS(NUM_URAM_BLOCKS[i])
        ) axi_fifo_large_tb_i();
      end : gen_depth
    end : gen_width
  end : gen_uram

endmodule
