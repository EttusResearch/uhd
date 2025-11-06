//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_gain_tb
//
// Description:
//
//   This is the top-level testbench for the gain RFNoC block. It instantiates
//   different variations of the gain block to test them all.
//

`default_nettype none


module rfnoc_block_gain_all_tb;

  rfnoc_block_gain_tb #(.IP_OPTION("HDL_IP")        ) rfnoc_block_gain_tb_hdl();
  rfnoc_block_gain_tb #(.IP_OPTION("IN_TREE_IP")    ) rfnoc_block_gain_tb_it ();
  rfnoc_block_gain_tb #(.IP_OPTION("OUT_OF_TREE_IP")) rfnoc_block_gain_tb_oot();

endmodule : rfnoc_block_gain_all_tb


`default_nettype wire
