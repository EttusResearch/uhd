//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi4s_add_bytes_all_tb
//
// Description:  Testbench for axi_add_bytes
//   Exercise corner cases parameters for adding bytes to the front of a packet
//

module axi4s_add_bytes_all_tb;

  // actual use cases
  // Add Ethernet header from a packet includes a preamble
  axi4s_add_bytes_tb #(.TEST_NAME("ENET_64_HADD"),.WIDTH(64),.ADD_START(0),.ADD_BYTES(48))
    ENET_64_HADD48 ();
  axi4s_add_bytes_tb #(.TEST_NAME("ENET_128_HADD"),.WIDTH(128),.ADD_START(0),.ADD_BYTES(48))
    ENET_128_HADD48 ();
  axi4s_add_bytes_tb #(.TEST_NAME("ENET_256_HADD"),.WIDTH(256),.ADD_START(0),.ADD_BYTES(48))
    ENET_256_HADD48 ();
  axi4s_add_bytes_tb #(.TEST_NAME("ENET_512_HADD"),.WIDTH(512),.ADD_START(0),.ADD_BYTES(48))
    ENET_512_HADD48 ();

  // Add Preamble header from a packet
  axi4s_add_bytes_tb #(.TEST_NAME("ENET_64_PADD"),.WIDTH(64),.ADD_START(0),.ADD_BYTES(6))
    ENET_64_PADD6 ();
  axi4s_add_bytes_tb #(.TEST_NAME("ENET_128_PADD"),.WIDTH(128),.ADD_START(0),.ADD_BYTES(6))
    ENET_128_PADD6 ();
  axi4s_add_bytes_tb #(.TEST_NAME("ENET_256_PADD"),.WIDTH(256),.ADD_START(0),.ADD_BYTES(6))
    ENET_256_PADD6 ();
  axi4s_add_bytes_tb #(.TEST_NAME("ENET_512_PADD"),.WIDTH(512),.ADD_START(0),.ADD_BYTES(6))
    ENET_512_PADD6 ();

  // START cases - removal starts at LSB of word
  axi4s_add_bytes_tb #(.TEST_NAME("SSTART_32_0:0"),.WIDTH(32),.ADD_START(0),.ADD_BYTES(1))
    SSTART_32_0to0 ();
  axi4s_add_bytes_tb #(.TEST_NAME("SSTART_32_1:0"),.WIDTH(32),.ADD_START(0),.ADD_BYTES(2))
    SSTART_32_0to1 ();
  axi4s_add_bytes_tb #(.TEST_NAME("SSTART_32_2:0"),.WIDTH(32),.ADD_START(0),.ADD_BYTES(3))
    SSTART_32_0to2 ();
  axi4s_add_bytes_tb #(.TEST_NAME("EXACT_32_3:0"), .WIDTH(32),.ADD_START(0),.ADD_BYTES(4))
    EXACT_32_0to3 ();
  axi4s_add_bytes_tb #(.TEST_NAME("MSTART_32_4:0"),.WIDTH(32),.ADD_START(0),.ADD_BYTES(5))
    MSTART_32_0to4 ();
  axi4s_add_bytes_tb #(.TEST_NAME("MSTART_32_5:0"),.WIDTH(32),.ADD_START(0),.ADD_BYTES(6))
    MSTART_32_0to5 ();
  axi4s_add_bytes_tb #(.TEST_NAME("MSTART_32_6:0"),.WIDTH(32),.ADD_START(0),.ADD_BYTES(7))
    MSTART_32_0to6 ();
  axi4s_add_bytes_tb #(.TEST_NAME("EXACT_32_7:0"), .WIDTH(32),.ADD_START(0),.ADD_BYTES(8))
    EXACT_32_0to7 ();

endmodule
