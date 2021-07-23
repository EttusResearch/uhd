//
// Copyright 2021 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: common_regs
// Description:
//   Registers definition within the x4xx_ps_rfdc_bd IP.

//XmlParse xml_on
//<top name="X4XX_FPGA">
//  <ports>
//    <info>
//      This section lists all common Processing System ports through
//      which the register maps in this project are accessed. Each input
//      port to the fabric will point to a regmap.
//    </info>
//    <port name="ARM_M_AXI_HPM0" targetregmap="AXI_HPM0_REGMAP">
//      <info>
//        This is the main AXI4-Lite master interface that the PS
//        exposes to the kernel to interact with the FPGA fabric.
//        There are multiple endpoints connected to this interface.
//      </info>
//    </port>
//    <port name="ARM_S_AXI_HPC0" sourcewindow="PL_DMA_MASTER_REGMAP|AXI_HPC0_WINDOW">
//      <info>
//        This is one of the two cache-coherent AXI slave ports available to
//        communicate from the fabric (master) to the PS (slave).
//      </info>
//    </port>
//    <port name="ARM_S_AXI_HPC1" sourcewindow="PL_DMA_MASTER_REGMAP|AXI_HPC1_WINDOW">
//      <info>
//        This is one of the two cache-coherent AXI slave ports available to
//        communicate from the fabric (master) to the PS (slave).
//      </info>
//    </port>
//    <port name="ARM_SPI1_CS3" targetregmap="MB_CPLD_PS_REGMAP">
//      <info>
//        This is the SPI1 interface
//        (see <a href="https://www.xilinx.com/html_docs/registers/ug1087/mod___spi.html" target="_blank">Zynq UltraScale+ Devices Register Reference</a>)
//        of the PS.
//        With chip select 3 enabled transactions are targeted for the PS MB CPLD register interface linked here.{br}
//        The request format on SPI is defined as.{br}
//        {b}Write request:{/b}
//        {ul}
//        {li}1'b1 = write
//        {li}15 bit address
//        {li}32 bit data (MOSI)
//        {li}8 bit processing gap
//        {li}5 bit padding
//        {li}1 bit ack
//        {li}2 bit status
//        {/ul}
//        {b}Read request:{/b}
//        {ul}
//        {li}1'b0 = read
//        {li}15 bit address
//        {li}8 bit processing gap
//        {li}32 bit data (MISO)
//        {li}5 bit padding
//        {li}1 bit ack
//        {li}2 bit status
//        {/ul}
//      </info>
//    </port>
//  </ports>
//  <regmapcfg readablestrobes="false">
//    <map name="AXI_HPM0_REGMAP"/>
//    <map name="MB_CPLD_PS_REGMAP"/>
//  </regmapcfg>
//</top>
//
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
//
//<regmap name="RFDC_REGS_REGMAP" readablestrobes="false" generatevhdl="true" generateverilog="true" ettusguidelines="true">
//  <group name="RFDC_REGS">
//    <info>
//      These are the registers located within the RFDC block design
//      that provide control and status support for the RF chain.
//    </info>
//
//    <window name="MMCM" offset="0x0" size="0x10000">
//      <info>
//        Register space for controlling the data clock MMCM instance
//        within the RFDC block design.
//        Refer to Xilinx' Clocking Wizard v6.0 Product Guide for the
//        regiter space description in chapter 2.
//        (https://www.xilinx.com/support/documentation/ip_documentation/clk_wiz/v6_0/pg065-clk-wiz.pdf)
//      </info>
//    </window>
//
//    <register name="INVERT_IQ_REG" offset="0x10000" size="32">
//      <info>Control register for inverting I/Q data.</info>
//      <!-- TODO: possibly redo these bitfields -->
//      <bitfield name="INVERT_DB0_ADC0_IQ" range="0"/>
//      <bitfield name="INVERT_DB0_ADC1_IQ" range="1"/>
//      <bitfield name="INVERT_DB0_ADC2_IQ" range="2"/>
//      <bitfield name="INVERT_DB0_ADC3_IQ" range="3"/>
//      <bitfield name="INVERT_DB1_ADC0_IQ" range="4"/>
//      <bitfield name="INVERT_DB1_ADC1_IQ" range="5"/>
//      <bitfield name="INVERT_DB1_ADC2_IQ" range="6"/>
//      <bitfield name="INVERT_DB1_ADC3_IQ" range="7"/>
//      <bitfield name="INVERT_DB0_DAC0_IQ" range="8"/>
//      <bitfield name="INVERT_DB0_DAC1_IQ" range="9"/>
//      <bitfield name="INVERT_DB0_DAC2_IQ" range="10"/>
//      <bitfield name="INVERT_DB0_DAC3_IQ" range="11"/>
//      <bitfield name="INVERT_DB1_DAC0_IQ" range="12"/>
//      <bitfield name="INVERT_DB1_DAC1_IQ" range="13"/>
//      <bitfield name="INVERT_DB1_DAC2_IQ" range="14"/>
//      <bitfield name="INVERT_DB1_DAC3_IQ" range="15"/>
//    </register>
//
//    <register name="MMCM_RESET_REG" offset="0x11000" size="32">
//      <info>Control register for resetting the data clock MMCM.</info>
//      <bitfield name="RESET_MMCM" range="0">
//        <info>
//          Write a '1' to this bit to reset the MMCM. Then write a
//          '0' to place the MMCM out of reset.
//        </info>
//      </bitfield>
//    </register>
//
//    <register name="RF_RESET_CONTROL_REG" offset="0x12000" size="32">
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
//    </register>
//
//    <register name="RF_RESET_STATUS_REG" offset="0x12008" size="32" writable="false">
//      <info>
//        Status register for the RF reset controller.
//        Verify the FSM ID before polling starting any reset sequence.
//        Refer to RF_RESET_CONTROL_REG for instructions on how to use
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
//    </register>
//
//    <register name="RF_AXI_STATUS_REG" offset="0x13000" size="32" writable="false">
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
//    </register>
//
//    <register name="CALIBRATION_DATA" offset="0x014000">
//      <info>
//        The fields of this register provide data to all the DAC channels when enabled
//        by the CALIBRATION_ENABLE register.
//      </info>
//      <bitfield name="Q_DATA" range="31..16">
//      </bitfield>
//      <bitfield name="I_DATA" range="15..00">
//      </bitfield>
//    </register>
//
//    <register name="CALIBRATION_ENABLE" offset="0x014008">
//      <info>
//        This register enables calibration data in the DAC data path for each of the
//        four channels. Each of these bits is normally '0'. When written '1', DAC data
//        for the corresponding channel will be constantly driven with the contents of
//        the CALIBRATION_DATA register.
//      </info>
//      <bitfield name="ENABLE_CALIBRATION_DATA_0" range="0">
//        <info>
//          Enables calibration data for channel 0.
//        </info>
//      </bitfield>
//      <bitfield name="ENABLE_CALIBRATION_DATA_1" range="1">
//        <info>
//          Enables calibration data for channel 1.
//        </info>
//      </bitfield>
//      <bitfield name="ENABLE_CALIBRATION_DATA_2" range="4">
//        <info>
//          Enables calibration data for channel 2.
//        </info>
//      </bitfield>
//      <bitfield name="ENABLE_CALIBRATION_DATA_3" range="5">
//        <info>
//          Enables calibration data for channel 3.
//        </info>
//      </bitfield>
//    </register>
//
//    <register name="RF_PLL_CONTROL_REG" offset="0x16000" size="32" writable="true">
//      <info>
//        Enable RF MMCM outputs.
//      </info>
//        <bitfield name="ENABLE_DATA_CLK"          range="0"/>
//        <bitfield name="ENABLE_DATA_CLK_2X"       range="4"/>
//        <bitfield name="ENABLE_RF_CLK"            range="8"/>
//        <bitfield name="ENABLE_RF_CLK_2X"         range="12"/>
//        <bitfield name="CLEAR_DATA_CLK_UNLOCKED"  range="16"/>
//    </register>
//
//    <register name="RF_PLL_STATUS_REG" offset="0x16008" size="32" writable="false">
//      <info>
//        Data Clk Pll Status Register
//      </info>
//        <bitfield name="DATA_CLK_PLL_UNLOCKED_STICKY" range="16"/>
//        <bitfield name="DATA_CLK_PLL_LOCKED" range="20"/>
//    </register>
//
//    <register name="THRESHOLD_STATUS" offset="0x015000">
//      <info>
//         This register shows threshold status for the ADCs. Each bit reflects the
//         RFDC's real-time ADC status signals, which will assert when the ADC input
//         signal exceeds the programmed threshold value. The status will remain
//         asserted until cleared by software.
//         The bitfield names follow the pattern ADCX_ZZ_over_threshold(1|2), where X is
//         the location of the tile in the converter column and ZZ is either 01 (the
//         lower RF-ADC in the tile) or 23 (the upper RF-ADC in the tile).
//         See also the Xilinx document PG269.
//      </info>
//      <bitfield name="ADC0_01_THRESHOLD1" range="0">
//      </bitfield>
//      <bitfield name="ADC0_01_THRESHOLD2" range="1">
//      </bitfield>
//      <bitfield name="ADC0_23_THRESHOLD1" range="2">
//      </bitfield>
//      <bitfield name="ADC0_23_THRESHOLD2" range="3">
//      </bitfield>
//      <bitfield name="ADC2_01_THRESHOLD1" range="8">
//      </bitfield>
//      <bitfield name="ADC2_01_THRESHOLD2" range="9">
//      </bitfield>
//      <bitfield name="ADC2_23_THRESHOLD1" range="10">
//      </bitfield>
//      <bitfield name="ADC2_23_THRESHOLD2" range="11">
//      </bitfield>
//    </register>
//
//    <enumeratedtype name="FABRIC_DSP_BW_ENUM" showhex="true">
//      <value name="FABRIC_DSP_BW_NONE" integer="0"/>
//      <value name="FABRIC_DSP_BW_100M" integer="100"/>
//      <value name="FABRIC_DSP_BW_200M" integer="200"/>
//      <value name="FABRIC_DSP_BW_400M" integer="400"/>
//    </enumeratedtype>
//
//    <register name="FABRIC_DSP_REG" offset="0x13008" size="32" writable="false">
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
//      <bitfield name="FABRIC_DSP_BW"         range="11..0" type="FABRIC_DSP_BW_ENUM" initialvalue="FABRIC_DSP_BW_NONE">
//        <info>Fabric DSP BW in MHz for daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="FABRIC_DSP_RX_CNT"     range="13..12" initialvalue="0">
//        <info>Fabric DSP RX channel count for daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="FABRIC_DSP_TX_CNT"     range="15..14" initialvalue="0">
//        <info>Fabric DSP TX channel count for daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="FABRIC_DSP_BW_DB1"     range="27..16" type="FABRIC_DSP_BW_ENUM" initialvalue="FABRIC_DSP_BW_NONE">
//        <info>Fabric DSP BW in MHz for daughterboard 1.</info>
//      </bitfield>
//      <bitfield name="FABRIC_DSP_RX_CNT_DB1" range="29..28" initialvalue="0">
//        <info>Fabric DSP RX channel count for daughterboard 0.</info>
//      </bitfield>
//      <bitfield name="FABRIC_DSP_TX_CNT_DB1" range="31..30" initialvalue="0">
//        <info>Fabric DSP TX channel count for daughterboard 0.</info>
//      </bitfield>
//    </register>
//
//  </group>
//</regmap>
//XmlParse xml_off
