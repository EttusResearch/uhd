//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_duc_ms_tb_all
//
// Description:
//   Top-level testbench wrapper for the multisample DUC.
//   Runs rfnoc_block_duc_ms_tb with several SPC configurations.
//

module rfnoc_block_duc_ms_tb_all;

    // Instantiate testbench with SPC 1 and 2 ports
    rfnoc_block_duc_ms_tb #(
        .CHDR_W(64),
        .SPC(1),
        .NUM_PORTS(2),
        .EXTENDED_TEST(0)
    ) rfnoc_block_duc_ms_tb_i1 ();

    // Instantiate testbench with SPC 2 and 1 port
    rfnoc_block_duc_ms_tb #(
        .CHDR_W(64),
        .SPC(2),
        .NUM_PORTS(1),
        .EXTENDED_TEST(1)
    ) rfnoc_block_duc_ms_tb_i2 ();

    // Instantiate testbench with SPC 8 and 8 ports
    rfnoc_block_duc_ms_tb #(
        .CHDR_W(256),
        .SPC(8),
        .NUM_PORTS(8),
        .EXTENDED_TEST(0)
    ) rfnoc_block_duc_ms_tb_i3 ();

endmodule
