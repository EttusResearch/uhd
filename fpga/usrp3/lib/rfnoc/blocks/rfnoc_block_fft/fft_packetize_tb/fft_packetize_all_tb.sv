//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fft_packetize_all_tb
//
// Description:
//
//   Top-level testbench for fft_packetize_tb, testing different configurations
//   of the module.
//

module fft_packetize_all_tb;

  // Parameters:
  //                 ┌ NIPC
  //                 |  ┌ NUM_CHAN
  //                 |  |  ┌ EN_CP_REMOVAL
  //                 |  |  |  ┌ EN_CP_INSERTION
  //                 |  |  |  |  ┌ EN_DATA_FIFOS
  //                 |  |  |  |  |  ┌ EN_TIME_ALL_PKTS
  //                 |  |  |  |  |  |
  //                 |  |  |  |  |  |
  //                 |  |  |  |  |  |
  fft_packetize_tb #(1, 1, 0, 0, 0, 0) tb_01();
  fft_packetize_tb #(1, 2, 0, 1, 0, 0) tb_02();
  fft_packetize_tb #(1, 3, 1, 0, 0, 0) tb_03();
  fft_packetize_tb #(1, 1, 1, 1, 0, 0) tb_04();
  fft_packetize_tb #(1, 2, 1, 1, 1, 1) tb_05();

  fft_packetize_tb #(2, 2, 0, 0, 0, 0) tb_11();
  fft_packetize_tb #(2, 1, 0, 1, 0, 0) tb_12();
  fft_packetize_tb #(2, 2, 1, 0, 0, 0) tb_13();
  fft_packetize_tb #(2, 3, 1, 1, 0, 0) tb_14();
  fft_packetize_tb #(2, 1, 1, 1, 1, 1) tb_15();

  fft_packetize_tb #(4, 1, 0, 0, 0, 0) tb_21();
  fft_packetize_tb #(4, 2, 0, 1, 0, 0) tb_22();
  fft_packetize_tb #(4, 1, 1, 0, 0, 0) tb_23();
  fft_packetize_tb #(4, 2, 1, 1, 0, 0) tb_24();
  fft_packetize_tb #(4, 3, 1, 1, 1, 1) tb_25();

endmodule : fft_packetize_all_tb
