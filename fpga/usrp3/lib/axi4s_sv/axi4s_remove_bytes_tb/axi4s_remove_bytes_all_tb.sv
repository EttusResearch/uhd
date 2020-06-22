//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi4s_remove_bytes_all_tb
//
// Description: Testbench for axi4s_remove_bytes_all_tb
//

module axi4s_remove_bytes_all_tb #(
  /* no PARAM */
)(
  /* no IO */
);

  // actual use cases
  // Strip Ethernet header from a packet includes a preamble
  axi4s_remove_bytes_tb #(.TEST_NAME("ENET_64_HREMOVE48"),.WIDTH(64),.REM_START(0),.REM_END(47))
    ENET_64_HREMOVE48 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("ENET_128_HREMOVE48"),.WIDTH(128),.REM_START(0),.REM_END(47))
    ENET_128_HREMOVE48 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("ENET_256_HREMOVE48"),.WIDTH(256),.REM_START(0),.REM_END(47))
    ENET_256_HREMOVE48 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("ENET_512_HREMOVE48"),.WIDTH(512),.REM_START(0),.REM_END(47))
    ENET_512_HREMOVE48 ();

  // Strip Ethernet header from a packet without a preamble
  axi4s_remove_bytes_tb #(.TEST_NAME("ENET_64_HREMOVE42"),.WIDTH(64),.REM_START(0),.REM_END(41))
    ENET_64_HREMOVE42 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("ENET_128_HREMOVE42"),.WIDTH(128),.REM_START(0),.REM_END(41))
    ENET_128_HREMOVE42 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("ENET_256_HREMOVE42"),.WIDTH(256),.REM_START(0),.REM_END(41))
    ENET_256_HREMOVE42 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("ENET_512_HREMOVE42"),.WIDTH(512),.REM_START(0),.REM_END(41))
    ENET_512_HREMOVE42 ();

  // Strip Preamble header from a packet
  axi4s_remove_bytes_tb #(.TEST_NAME("ENET_64_PREMOVE6"),.WIDTH(64),.REM_START(0),.REM_END(5))
    ENET_64_PREMOVE6 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("ENET_128_PREMOVE6"),.WIDTH(128),.REM_START(0),.REM_END(5))
    ENET_128_PREMOVE6 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("ENET_256_PREMOVE6"),.WIDTH(256),.REM_START(0),.REM_END(5))
    ENET_256_PREMOVE6 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("ENET_512_PREMOVE6"),.WIDTH(512),.REM_START(0),.REM_END(5))
    ENET_512_PREMOVE6 ();


  // TRUNCATE cases -
  axi4s_remove_bytes_tb #(.TEST_NAME("TRUNCATE_32_1:0"),.WIDTH(32),.REM_START(1),.REM_END(-1))
    TRUNCATE_32_0to1 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("TRUNCATE_32_2:0"),.WIDTH(32),.REM_START(2),.REM_END(-1))
    TRUNCATE_32_0to2 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("TRUNCATE_32_3:0"), .WIDTH(32),.REM_START(3),.REM_END(-1))
    TRUNCATE_32_0to3 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("TRUNCATE_32_4:0"),.WIDTH(32),.REM_START(4),.REM_END(-1))
    TRUNCATE_32_0to4 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("TRUNCATE_32_5:0"),.WIDTH(32),.REM_START(5),.REM_END(-1))
    TRUNCATE_32_0to5 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("TRUNCATE_32_6:0"),.WIDTH(32),.REM_START(6),.REM_END(-1))
    TRUNCATE_32_0to6 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("TRUNCATE_32_7:0"), .WIDTH(32),.REM_START(7),.REM_END(-1))
    TRUNCATE_32_0to7 ();


  // START cases - removal starts at LSB of word
  axi4s_remove_bytes_tb #(.TEST_NAME("SSTART_32_0:0"),.WIDTH(32),.REM_START(0),.REM_END(0))
    SSTART_32_0to0 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("SSTART_32_1:0"),.WIDTH(32),.REM_START(0),.REM_END(1))
    SSTART_32_0to1 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("SSTART_32_2:0"),.WIDTH(32),.REM_START(0),.REM_END(2))
    SSTART_32_0to2 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("EXACT_32_3:0"), .WIDTH(32),.REM_START(0),.REM_END(3))
    EXACT_32_0to3 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("MSTART_32_4:0"),.WIDTH(32),.REM_START(0),.REM_END(4))
    MSTART_32_0to4 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("MSTART_32_5:0"),.WIDTH(32),.REM_START(0),.REM_END(5))
    MSTART_32_0to5 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("MSTART_32_6:0"),.WIDTH(32),.REM_START(0),.REM_END(6))
    MSTART_32_0to6 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("EXACT_32_7:0"), .WIDTH(32),.REM_START(0),.REM_END(7))
    EXACT_32_0to7 ();


  // END cases - removal ends at MSB of word
  axi4s_remove_bytes_tb #(.TEST_NAME("MEND_32_7:1"),.WIDTH(32),.REM_START(1),.REM_END(7))
    MEND_32_1to7 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("MEND_32_7:2"),.WIDTH(32),.REM_START(2),.REM_END(7))
    MEND_32_2to7 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("MEND_32_7:3"),.WIDTH(32),.REM_START(3),.REM_END(7))
    MEND_32_3to7 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("EXACT_32_7:4"),.WIDTH(32),.REM_START(4),.REM_END(7))
    EXACT_32_4to7 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("SEND_32_7:5"),.WIDTH(32),.REM_START(5),.REM_END(7))
    SEND_32_5to7 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("SEND_32_7:6"),.WIDTH(32),.REM_START(6),.REM_END(7))
    SEND_32_6to7 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("SEND_32_7:7"),.WIDTH(32),.REM_START(7),.REM_END(7))
    SEND_32_7to7 ();

  axi4s_remove_bytes_tb #(.TEST_NAME("SEND_32_3:1"),.WIDTH(32),.REM_START(1),.REM_END(3))
    SEND_32_1to3 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("SEND_32_3:2"),.WIDTH(32),.REM_START(2),.REM_END(3))
    SEND_32_2to3 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("SEND_32_3:3"),.WIDTH(32),.REM_START(3),.REM_END(3))
    SEND_32_3to3 ();

  // MIDDLE cases - removal is in the middle of words
  axi4s_remove_bytes_tb #(.TEST_NAME("SMID_32_1:1"),.WIDTH(32),.REM_START(1),.REM_END(1))
    SMID_32_1to1 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("SMID_32_2:1"),.WIDTH(32),.REM_START(1),.REM_END(2))
    SMID_32_2to1 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("SMID_32_2:2"),.WIDTH(32),.REM_START(2),.REM_END(2))
    SMID_32_2to2 ();

  axi4s_remove_bytes_tb #(.TEST_NAME("MMID_32_5:1"),.WIDTH(32),.REM_START(1),.REM_END(5))
    MMID_32_1to5 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("MMID_32_6:1"),.WIDTH(32),.REM_START(1),.REM_END(6))
    MMID_32_1to6 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("MMID_32_6:2"),.WIDTH(32),.REM_START(2),.REM_END(6))
    MMID_32_2to6 ();

  // WRAP cases - removal is over word boundary
  axi4s_remove_bytes_tb #(.TEST_NAME("SWRAP_32_5:2"),.WIDTH(32),.REM_START(2),.REM_END(5))
    SWRAP_32_2to5 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("SWRAP_32_5:3"),.WIDTH(32),.REM_START(3),.REM_END(5))
    SWRAP_32_3to5 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("SWRAP_32_6:3"),.WIDTH(32),.REM_START(3),.REM_END(6))
    SWRAP_32_3to6 ();

  axi4s_remove_bytes_tb #(.TEST_NAME("MMID_32_9:2"),.WIDTH(32),.REM_START(2),.REM_END(9))
    MWRAP_32_2to9 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("SEND_32_9:3"),.WIDTH(32),.REM_START(3),.REM_END(9))
    MWRAP_32_3to9 ();
  axi4s_remove_bytes_tb #(.TEST_NAME("SEND_32_10:3"),.WIDTH(32),.REM_START(3),.REM_END(10))
    MWRAP_32_3to10();


endmodule
