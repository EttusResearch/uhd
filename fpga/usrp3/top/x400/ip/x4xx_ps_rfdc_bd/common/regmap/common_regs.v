//
// Copyright 2021 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: common_regs
// Description:
//   Common registers definition within the x4xx_ps_rfdc_bd between
//   different x4xx variants.

//XmlParse xml_on
//<regmap name="AXI_HPM0_REGMAP" readablestrobes="false" generatevhdl="true" ettusguidelines="true">
//  <info>
//    This is the map for the register space that the Processing System's
//    M_AXI_HPM0_FPD port (AXI4 master interface) has access to.
//    This port has a 40-bit address bus.
//  </info>
//  <group name="COMMON">
//    <window name="RPU"          offset="0x0080000000" size="0x00010000">
//      <info>Space reserved for RPU access</info>
//    </window>
//    <window name="JTAG_ENGINE"  offset="0x1000000000" size="0x1000">
//      <info>Register space for the JTAG engine for MB CPLD programming.</info>
//    </window>
//    <window name="RESERVED"     offset="0x100003F000" size="0x1000">
//      <info>Register space reserved for future use.</info>
//    </window>
//    <window name="MPM_ENDPOINT" offset="0x1000080000" size="0x20000"
//                                targetregmap="PL_CPLD_REGMAP">
//      <info>MPM endpoint fro MB/DB communication.</info>
//    </window>
//    <window name="CORE_REGS"    offset="0x10000A0000" size="0x4000"
//                                targetregmap="CORE_REGS_REGMAP">
//      <info>Register space reserved for mboard-regs (Core).</info>
//    </window>
//    <window name="INT_ETH_DMA"  offset="0x10000A4000" size="0x6000"
//                                targetregmap="ETH_DMA_CTRL_REGMAP">
//      <info>AXI DMA engine for internal Ethernet interface.</info>
//    </window>
//    <window name="INT_ETH_REGS" offset="0x10000AA000" size="0x2000">
//      <info>Misc. registers for internal Ethernet.</info>
//    </window>
//    <window name="RFDC"         offset="0x1000100000" size="0x40000">
//      <info>Register space occupied by the Xilinx RFDC IP block.</info>
//    </window>
//    <window name="RFDC_REGS"    offset="0x1000140000" size="0x20000"
//                                targetregmap="RFDC_REGS_REGMAP">
//      <info>Register space for RFDC control/status registers.</info>
//    </window>
//  </group>
//</regmap>
//
//<regmap name="ETH_DMA_CTRL_REGMAP" readablestrobes="false" generatevhdl="true" generateverilog="false" ettusguidelines="true">
//  <info>
//    This is the map that the nixge driver uses in Ethernet DMA to
//    move data between the Processing System's architecture and the fabric.
//    This map is a combination of two main components: a Xilix AXI DMA engine
//    and some registers for MAC/PHY control.
//  </info>
//  <group name="ETH_DMA_CTRL">
//    <window name="AXI_DMA_CTRL" offset="0x0"    size="0x4000">
//      <info>
//        Refer to Xilinx' AXI DMA v7.1 IP product guide for further
//        information on this register map:
//        https://www.xilinx.com/support/documentation/ip_documentation/axi_dma/v7_1/pg021_axi_dma.pdf
//      </info>
//    </window>
//    <window name="ETH_IO_CTRL"  offset="0x4000" size="0x2000">
//      <info>MAC/PHY control for the Ethernet interface.</info>
//    </window>
//  </group>
//</regmap>
//<regmap name="PL_DMA_MASTER_REGMAP" readablestrobes="false" generatevhdl="true" generateverilog="false" ettusguidelines="true">
//  <info>
//    This is a regmap to document the different ports that have access to the PS system memory.
//    Each port may have different restrictions on system memory. See the corresponding window
//     for details
//  </info>
//  <group name="HPC0_DMA">
//    <window name="AXI_HPC0_WINDOW" offset="0x0" size="0x10000000000">
//      <info>
//        The HPC0 port of the PS is used for general purpose cache-coherent accesses
//         to the PS system memory. Different applications may use it for different
//         purposes. Its access is configured as follows: {br}
//           {table border="1"}
//             {tr}{th}Offset{/th}        {th}Size{/th}          {th}Description{/th}{tr}
//             {tr}{td}0x000800000000{/td}{td}0x000800000000{/td}{td}DDR_HIGH{/td}{tr}
//             {tr}{td}0x00000000{/td}    {td}0x80000000{/td}    {td}DDR_LOW{/td}{tr}
//             {tr}{td}0xFF000000{/td}    {td}0x01000000{/td}    {td}LPS_OCM{/td}{tr}
//             {tr}{td}0xC0000000{/td}    {td}0x20000000{/td}    {td}QSPI{/td}{tr}
//           {/table}
//      </info>
//    </window>
//  </group>
//  <group name="HPC1_DMA">
//    <window name="AXI_HPC1_WINDOW" offset="0x0" size="0x1000000000">
//      <info>
//        The HPC1 port of the PS is connected to the Ethernet DMA module. Three slave
//        interfaces are lumped together in this window: scatter-gather, dma-rx, and dma-tx.
//         Its access is configured as follows: {br}
//           {table border="1"}
//             {tr}{th}Offset{/th}        {th}Size{/th}          {th}Description{/th}{tr}
//             {tr}{td}0x000800000000{/td}{td}0x000800000000{/td}{td}DDR_HIGH{/td}{tr}
//             {tr}{td}0x00000000{/td}    {td}0x80000000{/td}    {td}DDR_LOW{/td}{tr}
//             {tr}{td}0xC0000000{/td}    {td}0x20000000{/td}    {td}QSPI{/td}{tr}
//           {/table}
//      </info>
//    </window>
//  </group>
//</regmap>
//<regmap name="RFDC_REGS_REGMAP" readablestrobes="false" generatevhdl="true" generateverilog="true" ettusguidelines="true">
//  <group name="RFDC_REGS">
//    <regtype name="RF_RESET_CONTROL_REGTYPE" size="32">
//      <info>
//        Control register for the RF reset controller.
//        Verify the FSM ID before polling starting any reset sequence.
//        To use the SW reset triggers: Wait until DB*_DONE is de-asserted.
//        Assert either the *_RESET or *_ENABLE bitfields.
//        Wait until DB*_DONE is asserted to release the trigger.
//        The DB*_DONE signal should then de-assert.{BR/}
//        {b}Note: The *_DB1 constants are not used in the HDL, their purpose is
//        merely for documentation.{/b}
//      </info>
//      <bitfield name="FSM_RESET" range="0">
//        <info>
//          Write a '1' to this bit to reset the RF reset controller.
//          Write a '0' once db0_fsm_reset_done asserts.
//        </info>
//      </bitfield>
//      <bitfield name="ADC_RESET" range="4">
//        <info>
//          Write a '1' to this bit to trigger a reset for the
//          daughterboard 0 ADC chain. Write a '0' once db0_adc_seq_done
//          is asserted.
//        </info>
//      </bitfield>
//      <bitfield name="ADC_ENABLE" range="5">
//        <info>
//          Write a '1' to this bit to trigger the enable sequence for
//          the daughterboard 0 ADC chain. Write a '0' once
//          db0_adc_seq_done is asserted.
//        </info>
//      </bitfield>
//      <bitfield name="DAC_RESET" range="8">
//        <info>
//          Write a '1' to this bit to trigger a reset for the
//          daughterboard 0 DAC chain. Write a '0' once db0_dac_seq_done
//          is asserted.
//        </info>
//      </bitfield>
//      <bitfield name="DAC_ENABLE" range="9">
//        <info>
//          Write a '1' to this bit to trigger the enable sequence for
//          the daughterboard 0 DAC chain. Write a '0' once
//          db0_dac_seq_done is asserted.
//        </info>
//      </bitfield>
//    </regtype>
//
//    <regtype name="RF_RESET_STATUS_REGTYPE" size="32" writable="false">
//      <info>
//        Status register for the RF reset controller.
//        Verify the FSM ID before polling starting any reset sequence.
//        Refer to RF*_RESET_CONTROL_REG for instructions on how to use
//        the status bits in this register.{BR/}
//        {b}Note: The *_DB1 constants are not used in the HDL, their purpose is
//        merely for documentation.{/b}
//      </info>
//      <bitfield name="FSM_RESET_DONE" range="3">
//        <info>
//          This bit asserts ('1') when the DB0 RF reset controller FSM
//          reset sequence is completed. The bitfield deasserts ('0')
//          after deasserting db0_fsm_reset.
//        </info>
//      </bitfield>
//      <bitfield name="ADC_SEQ_DONE" range="7">
//        <info>
//          This bit asserts ('1') when the DB0 ADC chain reset sequence
//          is completed. The bitfield deasserts ('0') after
//          deasserting the issued triggered (enable or reset).
//        </info>
//      </bitfield>
//      <bitfield name="DAC_SEQ_DONE" range="11">
//        <info>
//          This bit asserts ('1') when the DB0 DAC chain reset sequence
//          is completed. The bitfield deasserts ('0') after
//          deasserting the issued triggered (enable or reset).
//        </info>
//      </bitfield>
//    </regtype>
//
//    <regtype name="RF_AXI_STATUS_REGTYPE" size="32" writable="false">
//      <info>
//        Status register for the RF AXI-Stream interfaces.{BR/}
//        {b}Note: The *_DB1 constants are not used in the HDL, their purpose is
//        merely for documentation.{/b}
//      </info>
//      <bitfield name="RFDC_DAC_TREADY" range="1..0">
//        <info>
//          This bitfield is wired to the RFDC's DAC (DB0) AXI-Stream
//          TReady handshake signals. The LSB is channel 0 and the MSB
//          is channel 1.
//        </info>
//      </bitfield>
//      <bitfield name="RFDC_DAC_TVALID" range="3..2">
//        <info>
//          This bitfield is wired to the RFDC's DAC (DB0) AXI-Stream
//          TValid handshake signals. The LSB is channel 0 and the MSB
//          is channel 1.
//        </info>
//      </bitfield>
//      <bitfield name="RFDC_ADC_Q_TREADY" range="5..4">
//        <info>
//          This bitfield is wired to the RFDC's ADC (DB0) AXI-Stream
//          TReady handshake signals (Q portion). The LSB is channel 0
//          and the MSB is channel 1.
//        </info>
//      </bitfield>
//      <bitfield name="RFDC_ADC_I_TREADY" range="7..6">
//        <info>
//          This bitfield is wired to the RFDC's ADC (DB0) AXI-Stream
//          TReady handshake signals (I portion). The LSB is channel 0
//          and the MSB is channel 1.
//        </info>
//      </bitfield>
//      <bitfield name="RFDC_ADC_Q_TVALID" range="9..8">
//        <info>
//          This bitfield is wired to the RFDC's ADC (DB0) AXI-Stream
//          TValid handshake signals (Q portion). The LSB is channel 0
//          and the MSB is channel 1.
//        </info>
//      </bitfield>
//      <bitfield name="RFDC_ADC_I_TVALID" range="11..10">
//        <info>
//          This bitfield is wired to the RFDC's ADC (DB0) AXI-Stream
//          TValid handshake signals (I portion). The LSB is channel 0
//          and the MSB is channel 1.
//        </info>
//      </bitfield>
//      <bitfield name="USER_ADC_TVALID" range="13..12">
//        <info>
//          This bitfield is wired to the user's ADC (DB0) AXI-Stream
//          TValid handshake signals. The LSB is channel 0 and the MSB
//          is channel 1.
//        </info>
//      </bitfield>
//      <bitfield name="USER_ADC_TREADY" range="15..14">
//        <info>
//          This bitfield is wired to the user's ADC (DB0) AXI-Stream
//          TReady handshake signals. The LSB is channel 0 and the MSB
//          is channel 1.
//        </info>
//      </bitfield>
//      <bitfield name="RFDC_DAC_TREADY_DB1" range="17..16">
//        <info>
//          This bitfield is wired to the RFDC's DAC (DB1) AXI-Stream
//          TReady handshake signals. The LSB is channel 0 and the MSB
//          is channel 1.
//        </info>
//      </bitfield>
//      <bitfield name="RFDC_DAC_TVALID_DB1" range="19..18">
//        <info>
//          This bitfield is wired to the RFDC's DAC (DB1) AXI-Stream
//          TValid handshake signals. The LSB is channel 0 and the MSB
//          is channel 1.
//        </info>
//      </bitfield>
//      <bitfield name="RFDC_ADC_Q_TREADY_DB1" range="21..20">
//        <info>
//          This bitfield is wired to the RFDC's ADC (DB1) AXI-Stream
//          TReady handshake signals (Q portion). The LSB is channel 0
//          and the MSB is channel 1.
//        </info>
//      </bitfield>
//      <bitfield name="RFDC_ADC_I_TREADY_DB1" range="23..22">
//        <info>
//          This bitfield is wired to the RFDC's ADC (DB1) AXI-Stream
//          TReady handshake signals (I portion). The LSB is channel 0
//          and the MSB is channel 1.
//        </info>
//      </bitfield>
//      <bitfield name="RFDC_ADC_Q_TVALID_DB1" range="25..24">
//        <info>
//          This bitfield is wired to the RFDC's ADC (DB1) AXI-Stream
//          TValid handshake signals (Q portion). The LSB is channel 0
//          and the MSB is channel 1.
//        </info>
//      </bitfield>
//      <bitfield name="RFDC_ADC_I_TVALID_DB1" range="27..26">
//        <info>
//          This bitfield is wired to the RFDC's ADC (DB1) AXI-Stream
//          TValid handshake signals (I portion). The LSB is channel 0
//          and the MSB is channel 1.
//        </info>
//      </bitfield>
//      <bitfield name="USER_ADC_TVALID_DB1" range="29..28">
//        <info>
//          This bitfield is wired to the user's ADC (DB1) AXI-Stream
//          TValid handshake signals. The LSB is channel 0 and the MSB
//          is channel 1.
//        </info>
//      </bitfield>
//      <bitfield name="USER_ADC_TREADY_DB1" range="31..30">
//        <info>
//          This bitfield is wired to the user's ADC (DB1) AXI-Stream
//          TReady handshake signals. The LSB is channel 0 and the MSB
//          is channel 1.
//        </info>
//      </bitfield>
//    </regtype>
//
//    <enumeratedtype name="FABRIC_DSP_BW_ENUM" showhex="true">
//      <value name="FABRIC_DSP_BW_NONE" integer="0"/>
//      <value name="FABRIC_DSP_BW_100M" integer="100"/>
//      <value name="FABRIC_DSP_BW_200M" integer="200"/>
//      <value name="FABRIC_DSP_BW_400M" integer="400"/>
//      <value name="FABRIC_DSP_BW_FULL" integer="1000"/>
//    </enumeratedtype>
//
//    <regtype name="FABRIC_DSP_REGTYPE" size="32" writable="false">
//      <info>
//        This register provides information to the driver on the type
//        of DSP that is instantiated in the fabric.{BR/}
//        The X410 platform supports multiple RF daughterboards, each requiring
//        a different fabric RF DSP chain that works with specific RFDC settings.
//        Each bandwidth DSP chain has a unique identifier (BW in MHz), this
//        information is conveyed in this register to let the driver
//        configure the RFDC with the proper settings.
//        Also, channel count for the DSP module is included.{BR/}
//        {b}Note: The *_DB1 constants are not used in the HDL, their purpose is
//        merely for documentation.{/b}
//      </info>
//      <bitfield name="FABRIC_DSP_RX_CNT"     range="3..0" initialvalue="0">
//        <info>Fabric DSP RX channel count for daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="FABRIC_DSP_TX_CNT"     range="7..4" initialvalue="0">
//        <info>Fabric DSP TX channel count for daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="FABRIC_DSP_RESERVED" range="9..8" initialvalue="0">
//        <info>Reserved for future use.</info>
//      </bitfield>
//      <bitfield name="FABRIC_DSP_RX_CNT_DB1" range="13..10" initialvalue="0">
//        <info>Fabric DSP RX channel count for daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="FABRIC_DSP_TX_CNT_DB1" range="17..14" initialvalue="0">
//        <info>Fabric DSP TX channel count for daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="FABRIC_DSP_RESERVED_DB1" range="19..18" initialvalue="0">
//        <info>Reserved for future use.</info>
//      </bitfield>
//      <bitfield name="FABRIC_DSP_BW" range="31..20" type="FABRIC_DSP_BW_ENUM" initialvalue="FABRIC_DSP_BW_NONE">
//        <info>Fabric DSP BW in MHz for both daughterboards.</info>
//      </bitfield>
//    </regtype>
//
//    <regtype name="RFDC_INFO_REGTYPE" size="32" writable="false">
//      <info>
//        This register provides information about how the RFDC is connected to
//        the rest of the fabric.{BR/}
//        Specifically, between the actual RFDC and the RFNoC infrastructure,
//        there may be additional resampling (if the RFDC resampler cannot handle
//        all the resampling itself) and it is important to know how wide the
//        connection from the RFDC gearbox FIFO to the rest of the design is.
//        {b}Note: The *_DB1 constants are not used in the HDL, their purpose is
//        merely for documentation.{/b}
//      </info>
//      <bitfield name="RFDC_INFO_XTRA_RESAMP"     range="3..0" initialvalue="1">
//        <info>Additional resampling happening outside the RFDC for daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="RFDC_INFO_SPC_RX"          range="6..4" initialvalue="1">
//        <info>Log2 of SPC value for RX connection (RFDC into fabric) for daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="RFDC_INFO_SPC_TX"          range="9..7" initialvalue="1">
//        <info>Log2 of SPC value for TX connection (fabric into RFDC) for daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="RFDC_INFO_XTRA_RESAMP_DB1" range="19..16" initialvalue="1">
//        <info>Additional resampling happening outside the RFDC for daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="RFDC_INFO_SPC_RX_DB1"      range="22..20" initialvalue="1">
//        <info>Log2 of SPC value for RX connection (RFDC into fabric) for daughterboard 1.</info>
//      </bitfield>
//      <bitfield name="RFDC_INFO_SPC_TX_DB1"      range="25..23" initialvalue="1">
//        <info>Log2 of SPC value for TX connection (fabric into RFDC) for daughterboard 1.</info>
//      </bitfield>
//    </regtype>
//
//    <regtype name="ADC_TILEMAP_REGTYPE" size="32" writable="false">
//      <info>
//        This register describes how the ADCs map to the respective tiles. It
//        lets us designate an ADC as channel 0, channel 1, etc. depending on
//        how those channels are externally connected to the RFSoC.{BR/}
//
//        For every channel, this register stores the tile number and the block
//        number of the converter. This can be used to then address the correct
//        converter in the various Xilinx interfaces/APIs.
//      </info>
//      <bitfield name="ADC_TILEMAP_DB0_CHAN0_TILE"     range="1..0" initialvalue="0">
//        <info>Tile number of the ADC for channel 0, daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="ADC_TILEMAP_DB0_CHAN0_BLOCK"     range="3..2" initialvalue="0">
//        <info>Block number (within the tile) of the ADC for channel 0, daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="ADC_TILEMAP_DB0_CHAN1_TILE"     range="5..4" initialvalue="0">
//        <info>Tile number of the ADC for channel 1, daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="ADC_TILEMAP_DB0_CHAN1_BLOCK"     range="7..6" initialvalue="0">
//        <info>Block number (within the tile) of the ADC for channel 1, daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="ADC_TILEMAP_DB0_CHAN2_TILE"     range="9..8" initialvalue="0">
//        <info>Tile number of the ADC for channel 2, daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="ADC_TILEMAP_DB0_CHAN2_BLOCK"     range="11..10" initialvalue="0">
//        <info>Block number (within the tile) of the ADC for channel 2, daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="ADC_TILEMAP_DB0_CHAN3_TILE"     range="13..12" initialvalue="0">
//        <info>Tile number of the ADC for channel 3, daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="ADC_TILEMAP_DB0_CHAN3_BLOCK"     range="15..14" initialvalue="0">
//        <info>Block number (within the tile) of the ADC for channel 3, daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="ADC_TILEMAP_DB1_CHAN0_TILE"     range="17..16" initialvalue="0">
//        <info>Tile number of the ADC for channel 0, daughterboard 1.</info>
//      </bitfield>
//      <bitfield name="ADC_TILEMAP_DB1_CHAN0_BLOCK"     range="19..18" initialvalue="0">
//        <info>Block number (within the tile) of the ADC for channel 0, daughterboard 1.</info>
//      </bitfield>
//      <bitfield name="ADC_TILEMAP_DB1_CHAN1_TILE"     range="21..20" initialvalue="0">
//        <info>Tile number of the ADC for channel 1, daughterboard 1.</info>
//      </bitfield>
//      <bitfield name="ADC_TILEMAP_DB1_CHAN1_BLOCK"     range="23..22" initialvalue="0">
//        <info>Block number (within the tile) of the ADC for channel 1, daughterboard 1.</info>
//      </bitfield>
//      <bitfield name="ADC_TILEMAP_DB1_CHAN2_TILE"     range="25..24" initialvalue="0">
//        <info>Tile number of the ADC for channel 2, daughterboard 1.</info>
//      </bitfield>
//      <bitfield name="ADC_TILEMAP_DB1_CHAN2_BLOCK"     range="27..26" initialvalue="0">
//        <info>Block number (within the tile) of the ADC for channel 2, daughterboard 1.</info>
//      </bitfield>
//      <bitfield name="ADC_TILEMAP_DB1_CHAN3_TILE"     range="29..28" initialvalue="0">
//        <info>Tile number of the ADC for channel 3, daughterboard 1.</info>
//      </bitfield>
//      <bitfield name="ADC_TILEMAP_DB1_CHAN3_BLOCK"     range="31..30" initialvalue="0">
//        <info>Block number (within the tile) of the ADC for channel 3, daughterboard 1.</info>
//      </bitfield>
//    </regtype>
//    <regtype name="DAC_TILEMAP_REGTYPE" size="32" writable="false">
//      <info>
//        This register describes how the DACs map to the respective tiles. It
//        lets us designate an DAC as channel 0, channel 1, etc. depending on
//        how those channels are externally connected to the RFSoC.{BR/}
//
//        For every channel, this register stores the tile number and the block
//        number of the converter. This can be used to then address the correct
//        converter in the various Xilinx interfaces/APIs.
//      </info>
//      <bitfield name="DAC_TILEMAP_DB0_CHAN0_TILE"     range="1..0" initialvalue="0">
//        <info>Tile number of the DAC for channel 0, daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="DAC_TILEMAP_DB0_CHAN0_BLOCK"     range="3..2" initialvalue="0">
//        <info>Block number (within the tile) of the DAC for channel 0, daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="DAC_TILEMAP_DB0_CHAN1_TILE"     range="5..4" initialvalue="0">
//        <info>Tile number of the DAC for channel 1, daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="DAC_TILEMAP_DB0_CHAN1_BLOCK"     range="7..6" initialvalue="0">
//        <info>Block number (within the tile) of the DAC for channel 1, daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="DAC_TILEMAP_DB0_CHAN2_TILE"     range="9..8" initialvalue="0">
//        <info>Tile number of the DAC for channel 2, daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="DAC_TILEMAP_DB0_CHAN2_BLOCK"     range="11..10" initialvalue="0">
//        <info>Block number (within the tile) of the DAC for channel 2, daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="DAC_TILEMAP_DB0_CHAN3_TILE"     range="13..12" initialvalue="0">
//        <info>Tile number of the DAC for channel 3, daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="DAC_TILEMAP_DB0_CHAN3_BLOCK"     range="15..14" initialvalue="0">
//        <info>Block number (within the tile) of the DAC for channel 3, daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="DAC_TILEMAP_DB1_CHAN0_TILE"     range="17..16" initialvalue="0">
//        <info>Tile number of the DAC for channel 0, daughterboard 1.</info>
//      </bitfield>
//      <bitfield name="DAC_TILEMAP_DB1_CHAN0_BLOCK"     range="19..18" initialvalue="0">
//        <info>Block number (within the tile) of the DAC for channel 0, daughterboard 1.</info>
//      </bitfield>
//      <bitfield name="DAC_TILEMAP_DB1_CHAN1_TILE"     range="21..20" initialvalue="0">
//        <info>Tile number of the DAC for channel 1, daughterboard 1.</info>
//      </bitfield>
//      <bitfield name="DAC_TILEMAP_DB1_CHAN1_BLOCK"     range="23..22" initialvalue="0">
//        <info>Block number (within the tile) of the DAC for channel 1, daughterboard 1.</info>
//      </bitfield>
//      <bitfield name="DAC_TILEMAP_DB1_CHAN2_TILE"     range="25..24" initialvalue="0">
//        <info>Tile number of the DAC for channel 2, daughterboard 1.</info>
//      </bitfield>
//      <bitfield name="DAC_TILEMAP_DB1_CHAN2_BLOCK"     range="27..26" initialvalue="0">
//        <info>Block number (within the tile) of the DAC for channel 2, daughterboard 1.</info>
//      </bitfield>
//      <bitfield name="DAC_TILEMAP_DB1_CHAN3_TILE"     range="29..28" initialvalue="0">
//        <info>Tile number of the DAC for channel 3, daughterboard 1.</info>
//      </bitfield>
//      <bitfield name="DAC_TILEMAP_DB1_CHAN3_BLOCK"     range="31..30" initialvalue="0">
//        <info>Block number (within the tile) of the DAC for channel 3, daughterboard 1.</info>
//      </bitfield>
//    </regtype>
//
//
//  </group>
//</regmap>
//XmlParse xml_off
