//
// Copyright 2022 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: x410_rfdc_regs
// Description:
//   Registers definition within the x4xx_ps_rfdc_bd IP.

//XmlParse xml_on
//<top name="X410_FPGA">
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
//    <register name="INVERT_DB0_IQ_REG" offset="0x10000" size="32">
//      <info>Control register for inverting I/Q data.</info>
//      <bitfield name="INVERT_DB0_ADC0_IQ" range="0"/>
//      <bitfield name="INVERT_DB0_ADC1_IQ" range="1"/>
//      <bitfield name="INVERT_DB0_ADC2_IQ" range="2"/>
//      <bitfield name="INVERT_DB0_ADC3_IQ" range="3"/>
//      <bitfield name="INVERT_DB0_DAC0_IQ" range="8"/>
//      <bitfield name="INVERT_DB0_DAC1_IQ" range="9"/>
//      <bitfield name="INVERT_DB0_DAC2_IQ" range="10"/>
//      <bitfield name="INVERT_DB0_DAC3_IQ" range="11"/>
//    </register>
//
//    <register name="INVERT_DB1_IQ_REG" offset="0x10800" size="32">
//      <info>Control register for inverting I/Q data.</info>
//      <bitfield name="INVERT_DB1_ADC0_IQ" range="0"/>
//      <bitfield name="INVERT_DB1_ADC1_IQ" range="1"/>
//      <bitfield name="INVERT_DB1_ADC2_IQ" range="2"/>
//      <bitfield name="INVERT_DB1_ADC3_IQ" range="3"/>
//      <bitfield name="INVERT_DB1_DAC0_IQ" range="8"/>
//      <bitfield name="INVERT_DB1_DAC1_IQ" range="9"/>
//      <bitfield name="INVERT_DB1_DAC2_IQ" range="10"/>
//      <bitfield name="INVERT_DB1_DAC3_IQ" range="11"/>
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
//    <register name="RF_RESET_CONTROL_REG" offset="0x12000" typename="RF_RESET_CONTROL_REGTYPE"/>
//    <register name="RF_RESET_STATUS_REG"  offset="0x12008" typename="RF_RESET_STATUS_REGTYPE"/>
//
//    <register name="RF_AXI_STATUS_REG" offset="0x13000" typename="RF_AXI_STATUS_REGTYPE"/>
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
//    <register name="FABRIC_DSP_REG" offset="0x13008" typename="FABRIC_DSP_REGTYPE"/>
//    <register name="ADC_TILEMAP_REG" offset="0x17000" typename="ADC_TILEMAP_REGTYPE"/>
//    <register name="DAC_TILEMAP_REG" offset="0x17008" typename="DAC_TILEMAP_REGTYPE"/>
//    <register name="RFDC_INFO_REG" offset="0x18000" typename="RFDC_INFO_REGTYPE"/>
//
//  </group>
//</regmap>
//XmlParse xml_off
