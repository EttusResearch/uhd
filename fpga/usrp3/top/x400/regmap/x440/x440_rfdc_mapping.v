//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: x440_rfdc_mapping
// Description:
//   Auxiliary register map to provide a single source for RFDC mapping information

//XmlParse xml_on
//
//<regmap name="RFDC_MAPPING_REGMAP" readablestrobes="false" generatevhdl="true" generateverilog="true" ettusguidelines="true">
//  <group name="CHANNEL_SWAPPING">
//
//    <enumeratedtype name="RFDC_ADC_MAPPING" showhex="true">
//      <value name="CH0_RX_MAPPING" integer="3"> Located in Tile 225, Channel 1 </value>
//      <value name="CH1_RX_MAPPING" integer="1"> Located in Tile 224, Channel 1 </value>
//      <value name="CH2_RX_MAPPING" integer="0"> Located in Tile 224, Channel 0 </value>
//      <value name="CH3_RX_MAPPING" integer="2"> Located in Tile 225, Channel 0 </value>
//      <value name="CH4_RX_MAPPING" integer="7"> Located in Tile 227, Channel 1 </value>
//      <value name="CH5_RX_MAPPING" integer="5"> Located in Tile 226, Channel 1 </value>
//      <value name="CH6_RX_MAPPING" integer="4"> Located in Tile 226, Channel 0 </value>
//      <value name="CH7_RX_MAPPING" integer="6"> Located in Tile 227, Channel 0 </value>
//    </enumeratedtype>
//
//    <enumeratedtype name="RFDC_DAC_MAPPING" showhex="true">
//      <value name="CH0_TX_MAPPING" integer="0"> Located in Tile 228, Channel 0 </value>
//      <value name="CH1_TX_MAPPING" integer="2"> Located in Tile 228, Channel 2 </value>
//      <value name="CH2_TX_MAPPING" integer="3"> Located in Tile 228, Channel 3 </value>
//      <value name="CH3_TX_MAPPING" integer="1"> Located in Tile 228, Channel 1 </value>
//      <value name="CH4_TX_MAPPING" integer="4"> Located in Tile 229, Channel 0 </value>
//      <value name="CH5_TX_MAPPING" integer="6"> Located in Tile 229, Channel 2 </value>
//      <value name="CH6_TX_MAPPING" integer="7"> Located in Tile 229, Channel 3 </value>
//      <value name="CH7_TX_MAPPING" integer="5"> Located in Tile 229, Channel 1 </value>
//    </enumeratedtype>
//
//  </group>
//</regmap>
//XmlParse xml_off
