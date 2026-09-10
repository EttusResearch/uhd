//
// Copyright 2026 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_pyld_ctxt_converter_all_tb
//
// Description:
//
//   Instantiate the payload/context converter testbench with all supported
//   parameter combinations.
//

`default_nettype none

module axis_pyld_ctxt_converter_all_tb;

  axis_pyld_ctxt_converter_tb #(
    .CLK_PERIOD          (6.0),
    .ITEM_W              (64),
    .NIPC                (1),
    .CONTEXT_FIFO_SIZE   (5),
    .PAYLOAD_FIFO_SIZE   (7),
    .CONTEXT_PREFETCH_EN (1)
  ) tb_0();

  axis_pyld_ctxt_converter_tb #(
    .CLK_PERIOD          (20.0),
    .ITEM_W              (32),
    .NIPC                (6),
    .CONTEXT_FIFO_SIZE   (5),
    .PAYLOAD_FIFO_SIZE   (1),
    .CONTEXT_PREFETCH_EN (1)
  ) tb_1();

  axis_pyld_ctxt_converter_tb #(
    .CLK_PERIOD          (3.0),
    .ITEM_W              (32),
    .NIPC                (4),
    .CONTEXT_FIFO_SIZE   (1),
    .PAYLOAD_FIFO_SIZE   (2),
    .CONTEXT_PREFETCH_EN (0)
  ) tb_2();

  axis_pyld_ctxt_converter_tb #(
    .CLK_PERIOD          (10.0),
    .ITEM_W              (16),
    .NIPC                (4),
    .CONTEXT_FIFO_SIZE   (8),
    .PAYLOAD_FIFO_SIZE   (5),
    .CONTEXT_PREFETCH_EN (1)
  ) tb_3();

  axis_pyld_ctxt_converter_tb #(
    .CLK_PERIOD          (3.0),
    .ITEM_W              (32),
    .NIPC                (2),
    .CONTEXT_FIFO_SIZE   (1),
    .PAYLOAD_FIFO_SIZE   (7),
    .CONTEXT_PREFETCH_EN (0)
  ) tb_4();

  axis_pyld_ctxt_converter_tb #(
    .CLK_PERIOD          (3.0),
    .ITEM_W              (8),
    .NIPC                (13),
    .CONTEXT_FIFO_SIZE   (1),
    .PAYLOAD_FIFO_SIZE   (7),
    .CONTEXT_PREFETCH_EN (0)
  ) tb_5();

endmodule

`default_nettype wire
