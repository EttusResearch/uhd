//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_channel_mux_all_tb.sv
//
// Description:
//
// Top level testbench for the chdr_channel_demux module. The testbench
// instantiates the chdr_channel_demux_tb for different combinations of
// channels/ports and CHDR_W.
//

module chdr_channel_demux_all_tb;

  chdr_channel_demux_tb #(.NUM_PORTS(1), .CHDR_W( 64), .CHANNEL_OFFSET(0)) tb_1_064_i ();
  chdr_channel_demux_tb #(.NUM_PORTS(2), .CHDR_W(128), .CHANNEL_OFFSET(1)) tb_2_128_i ();
  chdr_channel_demux_tb #(.NUM_PORTS(4), .CHDR_W( 64), .CHANNEL_OFFSET(2)) tb_4_064_i ();
  chdr_channel_demux_tb #(.NUM_PORTS(5), .CHDR_W( 64), .CHANNEL_OFFSET(3)) tb_5_064_i ();

endmodule
