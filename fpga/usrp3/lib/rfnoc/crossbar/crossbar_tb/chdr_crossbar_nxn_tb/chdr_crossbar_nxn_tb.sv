//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later


`timescale 1ns/1ps

module chdr_crossbar_nxn_tb();
  crossbar_tb #(
    .TEST_NAME          ("chdr_crossbar_nxn_tb"),
    .ROUTER_IMPL        ("chdr_crossbar_nxn"   ), // Router implementation
    .ROUTER_PORTS       (10                    ), // Number of ports
    .ROUTER_DWIDTH      (64                    ), // Router datapath width
    .MTU_LOG2           (7                     ), // log2 of max packet size for router
    .NUM_MASTERS        (10                    ), // Number of data generators in test
    .TEST_MAX_PACKETS   (100                   ), // How many packets to stream per test case?
    .TEST_LPP           (100                   ), // Lines per packet
    .TEST_MIN_INJ_RATE  (60                    ), // Minimum injection rate to test
    .TEST_MAX_INJ_RATE  (100                   ), // Maximum injection rate to test
    .TEST_INJ_RATE_INCR (10                    ), // Injection rate increment
    .TEST_GEN_LL_FILES  (0                     )  // Generate files to produce load-latency graphs?
  ) impl (
    /* no IO */
  );
endmodule
