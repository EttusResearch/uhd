//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: eth_ifc_all_tb
//
// Description:  Testbench for eth_ifc
//

module eth_ifc_all_tb;

  eth_ifc_tb #(.TEST_NAME("ORIGINAL"), .ENET_W(64),  .CPU_W(64), .CHDR_W(64),  .SV_ETH_IFC(0), .PREAMBLE_BYTES(6), .CPU_PREAMBLE(0)) ORIGINAL       ();
  eth_ifc_tb #(.TEST_NAME("64B_X4XX"), .ENET_W(64),  .CPU_W(64), .CHDR_W(64),  .SV_ETH_IFC(1), .PREAMBLE_BYTES(0), .CPU_PREAMBLE(0)) ETH64_X4XX     ();
  eth_ifc_tb #(.TEST_NAME("64B_N3XX"), .ENET_W(64),  .CPU_W(64), .CHDR_W(64),  .SV_ETH_IFC(1), .PREAMBLE_BYTES(6), .CPU_PREAMBLE(0)) ETH64_N3XX     ();
  eth_ifc_tb #(.TEST_NAME("64B_X3XX"), .ENET_W(64),  .CPU_W(64), .CHDR_W(64),  .SV_ETH_IFC(1), .PREAMBLE_BYTES(6), .CPU_PREAMBLE(1)) ETH64_X3XX     ();
  eth_ifc_tb #(.TEST_NAME("512B"),     .ENET_W(512), .CPU_W(64), .CHDR_W(512), .SV_ETH_IFC(1), .PREAMBLE_BYTES(0), .CPU_PREAMBLE(0)) ETH512_CHDR512 ();
  eth_ifc_tb #(.TEST_NAME("512_64B"),  .ENET_W(512), .CPU_W(64), .CHDR_W(64),  .SV_ETH_IFC(1), .PREAMBLE_BYTES(0), .CPU_PREAMBLE(0)) ETH512_CHDR64  ();
  eth_ifc_tb #(.TEST_NAME("512_128B"), .ENET_W(512), .CPU_W(64), .CHDR_W(128), .SV_ETH_IFC(1), .PREAMBLE_BYTES(0), .CPU_PREAMBLE(0)) ETH512_CHDR128 ();

endmodule
