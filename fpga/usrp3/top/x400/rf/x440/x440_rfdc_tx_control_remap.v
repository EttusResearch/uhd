//
// Copyright 2022 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: x440_rfdc_tx_control_remap.v
// Description:
//   Allows for remapping control signals to the correct tile on X440.
//   This file targets the DAC tiles specifically
//

`default_nettype none

module x440_rfdc_tx_control_remap (
  // Input control (Front-Panel ordering)
  input   wire [7:0]  input_controls,
  // output_control (DAC Tile ordering)
  output  wire [7:0]  output_controls
);

  `include "../../regmap/x440/rfdc_mapping_regmap_utils.vh"

  // Decode TX Channel mapping
  assign output_controls[CH0_TX_MAPPING] = input_controls[0];
  assign output_controls[CH1_TX_MAPPING] = input_controls[1];
  assign output_controls[CH2_TX_MAPPING] = input_controls[2];
  assign output_controls[CH3_TX_MAPPING] = input_controls[3];
  assign output_controls[CH4_TX_MAPPING] = input_controls[4];
  assign output_controls[CH5_TX_MAPPING] = input_controls[5];
  assign output_controls[CH6_TX_MAPPING] = input_controls[6];
  assign output_controls[CH7_TX_MAPPING] = input_controls[7];

endmodule

`default_nettype wire
