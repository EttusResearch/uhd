//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_radio_all_tb
//
// Description: This is the testbench for rfnoc_block_radio that instantiates
// several variations of rfnoc_block_radio_tb to test different configurations.
//


module rfnoc_block_radio_all_tb;

  //---------------------------------------------------------------------------
  // Test Configurations
  //---------------------------------------------------------------------------

  rfnoc_block_radio_tb #(.CHDR_W( 64), .ITEM_W(16), .NIPC(1), .NUM_PORTS(3), .STALL_PROB(10), .STB_PROB(100), .TEST_REGS(1)) tb_0 ();
  rfnoc_block_radio_tb #(.CHDR_W( 64), .ITEM_W(16), .NIPC(1), .NUM_PORTS(2), .STALL_PROB(25), .STB_PROB( 80), .TEST_REGS(1)) tb_1 ();
  rfnoc_block_radio_tb #(.CHDR_W( 64), .ITEM_W(16), .NIPC(2), .NUM_PORTS(1), .STALL_PROB(25), .STB_PROB( 80), .TEST_REGS(0)) tb_2 ();
  rfnoc_block_radio_tb #(.CHDR_W( 64), .ITEM_W(32), .NIPC(1), .NUM_PORTS(1), .STALL_PROB(25), .STB_PROB( 80), .TEST_REGS(0)) tb_3 ();
  rfnoc_block_radio_tb #(.CHDR_W( 64), .ITEM_W(32), .NIPC(2), .NUM_PORTS(1), .STALL_PROB(10), .STB_PROB( 80), .TEST_REGS(0)) tb_4 ();
  rfnoc_block_radio_tb #(.CHDR_W(128), .ITEM_W(32), .NIPC(1), .NUM_PORTS(3), .STALL_PROB(10), .STB_PROB(100), .TEST_REGS(1)) tb_5 ();
  rfnoc_block_radio_tb #(.CHDR_W(128), .ITEM_W(32), .NIPC(1), .NUM_PORTS(2), .STALL_PROB(25), .STB_PROB( 80), .TEST_REGS(0)) tb_6 ();
  rfnoc_block_radio_tb #(.CHDR_W(128), .ITEM_W(32), .NIPC(2), .NUM_PORTS(1), .STALL_PROB(25), .STB_PROB( 80), .TEST_REGS(0)) tb_7 ();
  rfnoc_block_radio_tb #(.CHDR_W(128), .ITEM_W(32), .NIPC(4), .NUM_PORTS(1), .STALL_PROB(10), .STB_PROB( 80), .TEST_REGS(0)) tb_8 ();
  rfnoc_block_radio_tb #(.CHDR_W(512), .ITEM_W(32), .NIPC(8), .NUM_PORTS(1), .STALL_PROB(10), .STB_PROB( 80), .TEST_REGS(0)) tb_9 ();

endmodule : rfnoc_block_radio_all_tb
