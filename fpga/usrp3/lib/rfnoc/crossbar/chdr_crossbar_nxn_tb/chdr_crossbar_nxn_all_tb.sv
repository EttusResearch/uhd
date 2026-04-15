//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_crossbar_nxn
//
// Description:
//
//  Top-level testbench for chdr_crossbar_nxn that instantiates multiple
//  permutations of the DUT.
//


module chdr_crossbar_nxn_all_tb;

  localparam NUM_PKTS = 64;

  // 2x2, 64-bit, fully connected
  chdr_crossbar_nxn_tb #(
    .NUM_PORTS      (2        ),
    .CHDR_WIDTHS    ('{64, 64}),
    .ROUTES         ('{'b11,
                       'b11}  ),
    .TEST_BAD_ROUTES(1        ),
    .NUM_PKTS       (NUM_PKTS ),
    .USE_MGMT_PORTS (0        )
  ) chdr_crossbar_nxn_tb_2x2 ();

  // 3x3, 128-bit, loopback disabled
  chdr_crossbar_nxn_tb #(
    .NUM_PORTS      (3               ),
    .CHDR_WIDTHS    ('{128, 128, 128}),
    .ROUTES         ('{'b011,
                       'b101,
                       'b110}        ),
    .TEST_BAD_ROUTES(1               ),
    .NUM_PKTS       (NUM_PKTS        ),
    .USE_MGMT_PORTS (0               )
  ) chdr_crossbar_nxn_tb_3x3 ();

  // 4x4, variable port widths, paths disabled
  chdr_crossbar_nxn_tb #(
    .NUM_PORTS      (4                   ),
    .CHDR_WIDTHS    ('{64, 128, 256, 512}),
    .ROUTES         ('{'b0111,
                       'b1011,
                       'b1101,
                       'b1010}           ),
    .TEST_BAD_ROUTES(1                   ),
    .NUM_PKTS       (NUM_PKTS            ),
    .USE_MGMT_PORTS (0                   )
  ) chdr_crossbar_nxn_tb_4x4_var ();

  // 3x3, variable port widths, paths disabled. This tests 2 64-bit ports
  // saturating a single 128-bit port.
  chdr_crossbar_nxn_tb #(
    .NUM_PORTS      (3             ),
    .CHDR_WIDTHS    ('{64, 64, 128}),
    .ROUTES         ('{'b001,
                       'b001,
                       'b000}      ),
    .TEST_BAD_ROUTES(0             ),
    .NUM_PKTS       (NUM_PKTS      ),
    .USE_MGMT_PORTS (0             )
  ) chdr_crossbar_nxn_tb_3x3_var ();

  // complex use case based on X440_X4_400 configuration
  chdr_crossbar_nxn_tb #(
    .NUM_PORTS      (21),
    .CHDR_WIDTHS    ('{default:128}),
    .ROUTES         ('{
                      21'b000000001111000011111,
                      21'b000000001111000011111,
                      21'b000000001111000011111,
                      21'b000000001111000011111,
                      21'b000000000000111111111,
                      21'b000000000000111111111,
                      21'b000000000000111111111,
                      21'b000000000000111111111,
                      21'b111100000000000011111,
                      21'b111100000000000011111,
                      21'b111100000000000011111,
                      21'b111100000000000011111,
                      21'b000011110000000011111,
                      21'b000011110000000011111,
                      21'b000011110000000011111,
                      21'b000011110000000011111,
                      21'b111111111111111110000,
                      21'b111111111111111101000,
                      21'b111111111111111100100,
                      21'b111111111111111100010,
                      21'b111111111111111100001
    }),
    .TEST_BAD_ROUTES(0             ),
    .NUM_PKTS       (NUM_PKTS      ),
    .USE_MGMT_PORTS (0             )
  ) chdr_crossbar_nxn_tb_x440_x4_400_var ();

endmodule
