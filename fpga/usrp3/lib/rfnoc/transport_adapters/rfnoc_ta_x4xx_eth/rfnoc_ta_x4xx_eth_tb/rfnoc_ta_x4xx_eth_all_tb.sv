//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_ta_x4xx_eth_all_tb
//
// Description:
//
//   Testbench for the QSFP wrapper to allow testing all protocols.
//

`include "./x4xx_mgt_types.vh"

module rfnoc_ta_x4xx_eth_all_tb;

  rfnoc_ta_x4xx_eth_tb #(
    .TEST_NAME ("100GbE_F"),
    .PROTOCOL0 (`MGT_100GbE),
    .CHDR_W    (512),
    .USE_MAC   (0)
  ) ETH_100Gb_fast ();

  rfnoc_ta_x4xx_eth_tb #(
    .TEST_NAME ("10GbE_F"),
    .PROTOCOL0 (`MGT_10GbE),
    .CHDR_W    (64),
    .USE_MAC   (0)
  ) ETH_10Gb_fast ();

  rfnoc_ta_x4xx_eth_tb #(
    .TEST_NAME ("10GbE_F_512"),
    .PROTOCOL0 (`MGT_10GbE),
    .CHDR_W    (512),
    .USE_MAC   (0)
  ) ETH_10Gb_fast_512 ();

  rfnoc_ta_x4xx_eth_tb #(
    .TEST_NAME ("10GbE_x4_F"),
    .PROTOCOL0 (`MGT_10GbE),
    .PROTOCOL1 (`MGT_10GbE),
    .PROTOCOL2 (`MGT_10GbE),
    .PROTOCOL3 (`MGT_10GbE),
    .CHDR_W    (64),
    .USE_MAC   (0)
  ) ETH_10Gb_x4_fast ();

  rfnoc_ta_x4xx_eth_tb #(
    .TEST_NAME ("100GbE_512S"),
    .PROTOCOL0 (`MGT_100GbE),
    .CHDR_W    (512),
    .USE_MAC   (1)
  ) ETH_100Gb_512serial ();

  rfnoc_ta_x4xx_eth_tb #(
    .TEST_NAME ("100GbE_128S"),
    .PROTOCOL0 (`MGT_100GbE),
    .CHDR_W    (128),
    .USE_MAC   (1)
  ) ETH_100Gb_128serial ();

  rfnoc_ta_x4xx_eth_tb #(
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
        ETH_10Gb_fast_512.test.done   &&
        ETH_10Gb_x4_fast.test.done    &&
        ETH_100Gb_512serial.test.done &&
        ETH_100Gb_128serial.test.done &&
        ETH_10Gb_serial.test.done
    ) $finish(1);
  end

endmodule
