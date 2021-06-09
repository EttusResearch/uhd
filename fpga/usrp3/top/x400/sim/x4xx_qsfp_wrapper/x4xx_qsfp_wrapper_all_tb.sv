//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: x4xx_qsfp_wrapper_all_tb
//
// Description:
//
//   Testbench for the QSFP wrapper to allow testing all protocols.
//

`include "./x4xx_mgt_types.vh"

module x4xx_qsfp_wrapper_all_tb;

  x4xx_qsfp_wrapper_tb #(
    .TEST_NAME ("100GbE_F"),
    .PROTOCOL0 (`MGT_100GbE),
    .CHDR_W    (512),
    .USE_MAC   (0)
  ) ETH_100Gb_fast ();

  x4xx_qsfp_wrapper_tb #(
    .TEST_NAME ("10GbE_F"),
    .PROTOCOL0 (`MGT_10GbE),
    .CHDR_W    (64),
    .USE_MAC   (0)
  ) ETH_10Gb_fast ();

  x4xx_qsfp_wrapper_tb #(
    .TEST_NAME ("10GbE_x4_F"),
    .PROTOCOL0 (`MGT_10GbE),
    .PROTOCOL1 (`MGT_10GbE),
    .PROTOCOL2 (`MGT_10GbE),
    .PROTOCOL3 (`MGT_10GbE),
    .CHDR_W    (64),
    .USE_MAC   (0)
  ) ETH_10Gb_x4_fast ();

  x4xx_qsfp_wrapper_tb #(
    .TEST_NAME ("100GbE_512S"),
    .PROTOCOL0 (`MGT_100GbE),
    .CHDR_W    (512),
    .USE_MAC   (1)
  ) ETH_100Gb_512serial ();

  x4xx_qsfp_wrapper_tb #(
    .TEST_NAME ("100GbE_128S"),
    .PROTOCOL0 (`MGT_100GbE),
    .CHDR_W    (128),
    .USE_MAC   (1)
  ) ETH_100Gb_128serial ();

  x4xx_qsfp_wrapper_tb #(
    .TEST_NAME ("10GbE_S"),
    .PROTOCOL0 (`MGT_10GbE),
    .CHDR_W    (64),
    .USE_MAC   (1)
  ) ETH_10Gb_serial ();

  bit clk,rst;

  sim_clock_gen #(100.0) clk_gen (clk, rst);

  // Wait for all done
  always_ff@(posedge clk) begin
    if (ETH_100Gb_fast.test.done      &&
        ETH_10Gb_fast.test.done       &&
        ETH_10Gb_x4_fast.test.done    &&
        ETH_100Gb_512serial.test.done &&
        ETH_100Gb_128serial.test.done &&
        ETH_10Gb_serial.test.done
    ) $finish(1);
  end

endmodule
