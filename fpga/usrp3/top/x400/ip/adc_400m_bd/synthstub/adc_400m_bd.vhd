------------------------------------------------------------------------------------------
--
-- File: adc_400m_bd.vhd
-- Author: niBlockDesign::niBdExportStub
-- Original Project: HwBuildTools
-- Date: 21 October 2020
--
------------------------------------------------------------------------------------------
-- (c) Copyright National Instruments Corporation
-- All Rights Reserved
-- National Instruments Internal Information
------------------------------------------------------------------------------------------
--
-- Purpose: This is an automatically generated stub file to match the entity
-- declaration for 'adc_400m_bd'. This file was created using niBdExportStub
-- Do not modify this file directly!
--
------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
library unisim;
use unisim.vcomponents.all;

entity adc_400m_bd is
port (
    adc_data_out_resetn_dclk : in STD_LOGIC;
    data_clk : in STD_LOGIC;
    enable_data_to_fir_rclk : in STD_LOGIC;
    fir_resetn_rclk2x : in STD_LOGIC;
    rfdc_adc_axi_resetn_rclk : in STD_LOGIC;
    rfdc_clk : in STD_LOGIC;
    rfdc_clk_2x : in STD_LOGIC;
    swap_iq_2x : in STD_LOGIC;
    adc_q_data_in_tvalid : in STD_LOGIC;
    adc_q_data_in_tready : out STD_LOGIC;
    adc_q_data_in_tdata : in STD_LOGIC_VECTOR ( 127 downto 0 );
    adc_i_data_in_tvalid : in STD_LOGIC;
    adc_i_data_in_tready : out STD_LOGIC;
    adc_i_data_in_tdata : in STD_LOGIC_VECTOR ( 127 downto 0 );
    adc_data_out_tvalid : out STD_LOGIC;
    adc_data_out_tdata : out STD_LOGIC_VECTOR ( 127 downto 0 )
  );
  end entity adc_400m_bd;

architecture stub of adc_400m_bd is
begin
end architecture stub;
