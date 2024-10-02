//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_load_split_merge_all_tb
//
// Description:
//
//   Top-level testbench for axis_load_split and axis_load_merge modules. This
//   instantiates multiple instances of axis_load_split_merge_tb to test
//   different configurations.
//

module axis_load_split_merge_all_tb;

  localparam int MAX_PKT_LEN   = 16;
  localparam int INT_FIFO_SIZE = $clog2(MAX_PKT_LEN);
  localparam int DATA_W        = 8;
  localparam int USER_W        = 4;

  axis_load_split_merge_tb #(
    .MAX_PKT_LEN  (MAX_PKT_LEN),
    .DATA_W       (4*DATA_W),
    .USER_W       (USER_W),
    .USER_SEL     (3),
    .INT_DATA_W   (DATA_W),
    .NUM_PORTS    (4),
    .INT_FIFO_SIZE(INT_FIFO_SIZE),
    .EXT_FIFO_SIZE(1)
  ) tb_32b_to_4x8b();

  axis_load_split_merge_tb #(
    .MAX_PKT_LEN  (MAX_PKT_LEN),
    .DATA_W       (2*DATA_W),
    .USER_W       (USER_W),
    .USER_SEL     (0),
    .INT_DATA_W   (DATA_W),
    .NUM_PORTS    (2),
    .INT_FIFO_SIZE(INT_FIFO_SIZE),
    .EXT_FIFO_SIZE(1)
  ) tb_16b_to_2x8b();

  axis_load_split_merge_tb #(
    .MAX_PKT_LEN  (MAX_PKT_LEN),
    .DATA_W       (4*DATA_W),
    .USER_W       (USER_W),
    .USER_SEL     (1),
    .INT_DATA_W   (DATA_W),
    .NUM_PORTS    (3),
    .INT_FIFO_SIZE(INT_FIFO_SIZE),
    .EXT_FIFO_SIZE(1)
  ) tb_32b_to_3x8b();

  axis_load_split_merge_tb #(
    .MAX_PKT_LEN  (MAX_PKT_LEN),
    .DATA_W       (4*DATA_W),
    .USER_W       (USER_W),
    .USER_SEL     (2),
    .INT_DATA_W   (DATA_W),
    .NUM_PORTS    (2),
    .INT_FIFO_SIZE(INT_FIFO_SIZE),
    .EXT_FIFO_SIZE(1)
  ) tb_32b_to_2x8b();

  axis_load_split_merge_tb #(
    .MAX_PKT_LEN  (MAX_PKT_LEN),
    .DATA_W       (DATA_W),
    .USER_W       (USER_W),
    .USER_SEL     (0),
    .INT_DATA_W   (DATA_W),
    .NUM_PORTS    (4),
    .INT_FIFO_SIZE(INT_FIFO_SIZE),
    .EXT_FIFO_SIZE(4)
  ) tb_8b_to_4x8b();

  axis_load_split_merge_tb #(
    .MAX_PKT_LEN  (MAX_PKT_LEN),
    .DATA_W       (2*DATA_W),
    .USER_W       (USER_W),
    .USER_SEL     (0),
    .INT_DATA_W   (DATA_W),
    .NUM_PORTS    (4),
    .INT_FIFO_SIZE(INT_FIFO_SIZE),
    .EXT_FIFO_SIZE(4)
  ) tb_16b_to_4x8b();

endmodule
