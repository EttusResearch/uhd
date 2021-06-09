------------------------------------------------------------------------------------------
--
-- File: dac_400m_bd.vhd
-- Author: niBlockDesign::niBdExportStub
-- Original Project: HwBuildTools
-- Date: 22 April 2020
--
------------------------------------------------------------------------------------------
-- (c) Copyright National Instruments Corporation
-- All Rights Reserved
-- National Instruments Internal Information
------------------------------------------------------------------------------------------
--
-- Purpose: This is an automatically generated stub file to match the entity
-- declaration for 'dac_400m_bd'. This file was created using niBdExportStub
-- Do not modify this file directly!
--
------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
library unisim;
use unisim.vcomponents.all;

entity dac_400m_bd is
port (
    dac_data_in_resetn_dclk : in STD_LOGIC;
    dac_data_in_resetn_dclk2x : in STD_LOGIC;
    dac_data_in_resetn_rclk : in STD_LOGIC;
    dac_data_in_tdata : in STD_LOGIC_VECTOR ( 127 downto 0 );
    dac_data_in_tready : out STD_LOGIC;
    dac_data_in_tvalid : in STD_LOGIC;
    dac_data_out_tdata : out STD_LOGIC_VECTOR ( 255 downto 0 );
    dac_data_out_tready : in STD_LOGIC;
    dac_data_out_tvalid : out STD_LOGIC;
    data_clk : in STD_LOGIC;
    data_clk_2x : in STD_LOGIC;
    rfdc_clk : in STD_LOGIC
  );
  end entity dac_400m_bd;

architecture stub of dac_400m_bd is
begin
end architecture stub;
