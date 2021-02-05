--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: PcieCmi
--
-- Description:
--
--   This is an automatically generated file.
--   Do not modify this file directly!
--

library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;

entity PcieCmi is
  generic (
    kSimulation :  natural  := 0
  );
  port (
    Clk : in std_logic ;
    acReset : in std_logic ;
    cSerialNumber : in std_logic_vector (39 downto 0);
    cBoardIsReady : in std_logic ;
    cCmiReset : out std_logic ;
    cOtherSideDetected : out std_logic ;
    aCblPrsnt_n : in std_logic ;
    aSdaIn : in std_logic ;
    aSdaOut : out std_logic ;
    aSclIn : in std_logic ;
    aSclOut : out std_logic 
  );
end entity PcieCmi;
architecture rtl of PcieCmi is
begin
end architecture rtl;
