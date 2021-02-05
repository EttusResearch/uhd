--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: PcieCmiWrapper
--
-- Description:
--
--   This is an automatically generated file.
--   Do not modify this file directly!
--


library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;

--synopsys translate_off
--For synthesis, netlist comes from an qxp and there is no external library
--For simulation in client, netlist comes from PcieCmiLib external library
--For simulation in dev branch, netlist comes from work library.
--Binding will work fine in both simulation cases as long as PcieCmi is compiled
-- to either library. In dev branch PcieCmiLib will be an empty library just so
-- ModelSim doesn't complain.
library PcieCmiLib;
--synopsys translate_on

entity PcieCmiWrapper is
  generic (
    kSimulation        : natural := 0 -- set to 1 to speedup simulation
  );
  port (
    Clk                : in  std_logic; -- 40 MHz clock
    acReset            : in  std_logic;

    cSerialNumber      : in  std_logic_vector(39 downto 0);
    cBoardIsReady      : in  std_logic;
    cCmiReset          : out std_logic;
    cOtherSideDetected : out std_logic;

    aCblPrsnt_n        : in  std_logic;

    aSdaIn             : in  std_logic;
    aSdaOut            : out std_logic;
    aSclIn             : in  std_logic;
    aSclOut            : out std_logic
  );
end PcieCmiWrapper;

architecture rtl of PcieCmiWrapper is

  component PcieCmi
    generic (kSimulation : natural := 0);
    port (
      Clk                : in  std_logic;
      acReset            : in  std_logic;
      cSerialNumber      : in  std_logic_vector(39 downto 0);
      cBoardIsReady      : in  std_logic;
      cCmiReset          : out std_logic;
      cOtherSideDetected : out std_logic;
      aCblPrsnt_n        : in  std_logic;
      aSdaIn             : in  std_logic;
      aSdaOut            : out std_logic;
      aSclIn             : in  std_logic;
      aSclOut            : out std_logic);
  end component;

begin

  -- Just forward all signals to lower level entity.
  -- Leave the simulation generic in place as it important to be able to
  -- simulate the netlist in reasonable time. For the synthesis in the client
  -- the generic is ignored as the netlist is translated with kSimulation set to
  -- default value and "overwrites" it.

  --vhook PcieCmi
  PcieCmix: PcieCmi
    generic map (kSimulation => kSimulation)  --natural:=0
    port map (
      Clk                => Clk,                 --in  std_logic
      acReset            => acReset,             --in  std_logic
      cSerialNumber      => cSerialNumber,       --in  std_logic_vector(39:0)
      cBoardIsReady      => cBoardIsReady,       --in  std_logic
      cCmiReset          => cCmiReset,           --out std_logic
      cOtherSideDetected => cOtherSideDetected,  --out std_logic
      aCblPrsnt_n        => aCblPrsnt_n,         --in  std_logic
      aSdaIn             => aSdaIn,              --in  std_logic
      aSdaOut            => aSdaOut,             --out std_logic
      aSclIn             => aSclIn,              --in  std_logic
      aSclOut            => aSclOut);            --out std_logic

end architecture rtl;