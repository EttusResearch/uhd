//
// Copyright 2025 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfdc_info_pkg
//
// Description:
//
//  Typedefs for RFDC information memory which are used by the device specific
//  RFDC memory content packages.
//

package rfdc_info_pkg;

  typedef enum logic [1:0] {
    ENABLED  = 2'b00,
    DISABLED = 2'b11
  } block_mode_t;

  typedef struct packed {
    logic        is_adc;
    logic        db;
    logic [1:0]  channel;
    logic [1:0]  reserved2;
    logic [1:0]  tile;
    logic [1:0]  block;
    block_mode_t block_mode;
  } rfdc_memory_entry_t;

  // Empty entry constant
  localparam rfdc_memory_entry_t EMPTY_ENTRY = '{
    is_adc: 0,
    db: 0,
    channel: 0,
    reserved2: 0,
    tile: 0,
    block: 0,
    block_mode: DISABLED
  };

  // current RFSoC has 8 DACs and 8 ADCs
  localparam int NUM_ENTRIES = 16;

endpackage

//XmlParse xml_on
//<regmap name="RFDC_REGS_REGMAP" generatesv="false">
//  <group name="RFDC_REGS">
//    <enumeratedtype name="RFDC_BLOCK_INFO_ENUM" showhex="true">
//      <value name="ENABLED" integer="0"/>
//      <value name="DISABLED" integer="3"/>
//    </enumeratedtype>
//    <regtype name="RFDC_INFO_MEMTYPE" size="32" writable="false">
//      <info>
//        This register provides information for one ADC/DAC within the RFSoC and its RFDC
//        configuration. There is further information on the RFNoC index of this channel.
//        The register is defined as a SystemVerilog typedef and consumed by MPM using the RFDC
//        Python register interface.
//      </info>
//      <bitfield name="BLOCK_MODE" range="1..0" type="RFDC_BLOCK_INFO_ENUM">
//        <info>The state of this ADC/DAC.</info>
//      </bitfield>
//      <bitfield name="BLOCK" range="3..2">
//        <info>Index of the ADC/DAC within the FPGA tile.</info>
//      </bitfield>
//      <bitfield name="TILE" range="5..4">
//        <info>
//          Zero based tile offset of the FPGA.
//          For DAC index i equals to FPGA tile 228+i.
//          For ADC index i equals to FPGA tile 224+i.
//        </info>
//      </bitfield>
//      <bitfield name="RESERVED2" range="7..6">
//        <info>
//          Reserved for later use.
//        </info>
//      </bitfield>
//      <bitfield name="CHANNEL" range="9..8">
//        <info>Index of the RFNoC channel per DB.</info>
//      </bitfield>
//      <bitfield name="DB" range="10">
//        <info>DB index.</info>
//      </bitfield>
//      <bitfield name="IS_ADC" range="11">
//        <info>If the converter is an ADC this bit is set. Otherwise it is an DAC.</info>
//      </bitfield>
//    </regtype>
//  </group>
//</regmap>
//XmlParse xml_off
