//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: eth_ipv4_interface_all_tb
//
// Description:
//
//   Top-level testbench for eth_ipv4_interface_tb. This instantiates different
//   variations of eth_ipv4_interface_tb to test them all.
//

`default_nettype none


module eth_ipv4_interface_all_tb;
  // 1 GbE and 10 GbE configurations
  eth_ipv4_interface_tb #(.ENET_W( 64), .CPU_W(64), .CHDR_W( 64), .EN_RAW_UDP(0)) eth_ipv4_interface_tb_00 ();
  eth_ipv4_interface_tb #(.ENET_W( 64), .CPU_W(64), .CHDR_W( 64), .EN_RAW_UDP(1)) eth_ipv4_interface_tb_01 ();
  // 100 GbE configurations
  eth_ipv4_interface_tb #(.ENET_W(512), .CPU_W(64), .CHDR_W(128), .EN_RAW_UDP(1)) eth_ipv4_interface_tb_02 ();
  eth_ipv4_interface_tb #(.ENET_W(512), .CPU_W(64), .CHDR_W(512), .EN_RAW_UDP(1)) eth_ipv4_interface_tb_03 ();
endmodule : eth_ipv4_interface_all_tb


`default_nettype wire
