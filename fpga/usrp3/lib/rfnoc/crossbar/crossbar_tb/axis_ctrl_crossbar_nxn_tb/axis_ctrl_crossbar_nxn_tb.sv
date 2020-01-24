//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later


`timescale 1ns/1ps

module axis_ctrl_crossbar_nxn_tb();
  crossbar_tb #(
    .TEST_NAME          ("axis_ctrl_crossbar_nxn_tb"),
    .ROUTER_IMPL        ("axis_ctrl_2d_torus"       ), // Router implementation
    .ROUTER_PORTS       (20                         ), // Number of ports
    .ROUTER_DWIDTH      (64                         ), // Router datapath width
    .MTU_LOG2           (5                          ), // log2 of max packet size for router
    .NUM_MASTERS        (4                          ), // Number of data generators in test
    .TEST_MAX_PACKETS   (100                        ), // How many packets to stream per test case?
    .TEST_LPP           (20                         ), // Lines per packet
    .TEST_MIN_INJ_RATE  (10                         ), // Minimum injection rate to test
    .TEST_MAX_INJ_RATE  (40                         ), // Maximum injection rate to test
    .TEST_INJ_RATE_INCR (10                         ), // Injection rate increment
    .TEST_GEN_LL_FILES  (0                          )  // Generate files to produce load-latency graphs?
  ) impl (
    /* no IO */
  );
endmodule
