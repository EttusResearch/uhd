//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: db_gpio_reordering
//
// Description:
//   Reorders the GPIO wires towards the DB CPLDs in a common way for DB 0 and 1.
//
//   The digital daughterboard connector has 120 pins [A-F][1-20].
//   The numbering on the motherboard traces do not match for daughterboard 0 and 1.
//   This module orders the FPGA outputs MSB first and connects it to the DB
//   connection with increasing letter and increasing number.
//   For DB 0 this results in:
//   FPGA Bit 19 = A7  (trace: DB0/1_GPIO[19])
//   FPGA Bit 18 = A8  (trace: DB0/1_GPIO[17])
//   ...
//   FPGA Bit  0 = C19 (trace: DB0/1_GPIO[12])
//   This enables usages of the same daughterboard CPLD image on both connectors.
//

`default_nettype none

module db_gpio_reordering (
  // 20 bit internal interface
  output wire [19:0] db0_gpio_in_int,
  input  wire [19:0] db0_gpio_out_int,
  input  wire [19:0] db0_gpio_out_en_int,
  output wire [19:0] db1_gpio_in_int,
  input  wire [19:0] db1_gpio_out_int,
  input  wire [19:0] db1_gpio_out_en_int,

  // 20 bit external interface
  input  wire [19:0] db0_gpio_in_ext,
  output wire [19:0] db0_gpio_out_ext,
  output wire [19:0] db0_gpio_out_en_ext,
  input  wire [19:0] db1_gpio_in_ext,
  output wire [19:0] db1_gpio_out_ext,
  output wire [19:0] db1_gpio_out_en_ext
);

  //port indexes
  localparam ENTRY_BITWIDTH = 5;
  localparam NUM_ENTRIES = 20;
  localparam [ENTRY_BITWIDTH*NUM_ENTRIES-1:0] PORT0_MAPPING = {
    5'd 19,
    5'd 17,
    5'd  0,
    5'd 14,
    5'd 15,
    5'd 10,
    5'd  4,
    5'd  5,
    5'd 16,
    5'd 18,
    5'd  8,
    5'd  6,
    5'd  1,
    5'd  9,
    5'd  2,
    5'd  3,
    5'd 11,
    5'd  7,
    5'd 13,
    5'd 12 };
  localparam [ENTRY_BITWIDTH*NUM_ENTRIES-1:0] PORT1_MAPPING = {
    5'd 10,
    5'd  6,
    5'd  7,
    5'd  2,
    5'd  3,
    5'd  0,
    5'd  1,
    5'd  4,
    5'd  8,
    5'd  9,
    5'd 11,
    5'd  5,
    5'd 13,
    5'd 12,
    5'd 15,
    5'd 14,
    5'd 19,
    5'd 18,
    5'd 17,
    5'd 16 };

  // reordering assignments
  generate
    genvar i;
    for (i=0; i<NUM_ENTRIES; i=i+1) begin : reordering_gen
      // input data
      assign db0_gpio_in_int[i] = db0_gpio_in_ext[PORT0_MAPPING[i*ENTRY_BITWIDTH +: ENTRY_BITWIDTH]];
      assign db1_gpio_in_int[i] = db1_gpio_in_ext[PORT1_MAPPING[i*ENTRY_BITWIDTH +: ENTRY_BITWIDTH]];

      // output data
      assign db0_gpio_out_ext[PORT0_MAPPING[i*ENTRY_BITWIDTH +: ENTRY_BITWIDTH]] = db0_gpio_out_int[i];
      assign db1_gpio_out_ext[PORT1_MAPPING[i*ENTRY_BITWIDTH +: ENTRY_BITWIDTH]] = db1_gpio_out_int[i];

      // output enable
      assign db0_gpio_out_en_ext[PORT0_MAPPING[i*ENTRY_BITWIDTH +: ENTRY_BITWIDTH]] = db0_gpio_out_en_int[i];
      assign db1_gpio_out_en_ext[PORT1_MAPPING[i*ENTRY_BITWIDTH +: ENTRY_BITWIDTH]] = db1_gpio_out_en_int[i];
    end
  endgenerate
endmodule

`default_nettype wire
