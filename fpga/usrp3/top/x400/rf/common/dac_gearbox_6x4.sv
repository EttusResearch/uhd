//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: dac_gearbox_6x4
//
// Description:
//
//   Gearbox to expand the data width from 6 SPC to 4 SPC.
//   Input clocks are all aligned to one another and come from the same MMCM
//   based on a common reference clock. These clock relationships below allow a
//   synchronous transfer of data with a deterministic latency.
//     Clk1x
//     Clk2x: 2x Clk1x
//     RfClk: 3x Clk1x
//

module dac_gearbox_6x4 (
  input  logic         clk_1x,
  input  logic         clk_2x,
  input  logic         rf_clk,
  input  logic         reset_n_c1,
  input  logic         reset_n_c2,
  // 16 bit data packing: [Q5,I5,Q4,I4,Q3,I3,Q2,I2,Q1,I1,Q0,I0] (I in LSBs)
  input  logic [191:0] data_c2,
  input  logic         data_valid_c2,
  // 16 bit data packing: [Q4,I4,Q3,I3,Q2,I2,Q1,I1,Q0,I0] (I in LSBs)
  output logic [127:0] data_rclk,
  output logic         data_valid_rclk
);

  logic [383:0] data_c1;
  logic         data_valid_c1;

  // We cannot move data from Clk2x to RfClk because the clock relation between
  // these two clocks will make it almost impossible to close timing. So, we
  // move data from Clk2x to Clk1x and then to the RfClk domain. Since we need
  // deterministic delay in the data path, we cannot use a FIFO to do data
  // crossing.

  dac_gearbox_6x12 dac_gearbox_6x12_i (
    .Clk1x          (clk_1x),
    .Clk2x          (clk_2x),
    .ac1Reset_n     (reset_n_c1),
    .ac2Reset_n     (reset_n_c2),
    .C2DataIn       (data_c2),
    .C2DataValidIn  (data_valid_c2),
    .c1DataOut      (data_c1),
    .c1DataValidOut (data_valid_c1)
  );

  dac_gearbox_12x4 dac_gearbox_12x4_i (
    .clk1x        (clk_1x),
    .reset_n_1x   (reset_n_c1),
    .data_in_1x   (data_c1),
    .valid_in_1x  (data_valid_c1),
    .clk3x        (rf_clk),
    .data_out_3x  (data_rclk),
    .valid_out_3x (data_valid_rclk)
  );

endmodule : dac_gearbox_6x4
