--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: dac_gearbox_6x8
--
-- Description:
--
--   Gearbox to expand the data width from 6 SPC to 8 SPC.
--   Input Clocks, all aligned to one another and coming from same MMCM
--     PLL reference clock = 61.44 or 62.5 MHz.
--     RfClk:       184.32 or 187.5 MHz (3x PLL reference clock)
--     Clk1x:       122.88 or 125 MHz (2x PLL reference clock)
--     Clk2x:       245.76 or 250 MHz (4x PLL reference clock)
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

entity dac_gearbox_6x8 is
  port(
    Clk1x           : in  std_logic;
    Clk2x           : in  std_logic;
    RfClk           : in  std_logic;
    ac1Reset_n      : in  std_logic;
    ac2Reset_n      : in  std_logic;
    arReset_n       : in  std_logic;
    -- 16 bit data packing: [Q5,I5,Q4,I4,Q3,I3,Q2,I2,Q1,I1,Q0,I0] (I in LSBs)
    c2DataIn        : in  std_logic_vector(191 downto 0);
    c2DataValidIn   : in  std_logic;
    -- 16 bit data packing: [Q7,I7,Q6,I6,..,Q2,I2,Q1,I1,Q0,I0] (I in LSBs)
    rDataOut        : out std_logic_vector(255 downto 0) := (others => '0');
    rReadyForOutput : in  std_logic;
    rDataValidOut   : out std_logic := '0'
  );
end dac_gearbox_6x8;

architecture struct of dac_gearbox_6x8 is

  signal c1DataOut : std_logic_vector(383 downto 0);
  signal c1DataValidOut : std_logic;

begin

  -- Clk1x, Clk2x, and RfClk are source from the same PLL and have a known
  -- phase relationship between power cycles. Since, they have known phase
  -- relationship, clock crossing as be done without a dual clock FIFO or any
  -- other handshaking mechanism. We cannot move data from Clk2x to RfClk
  -- because of the clock relation between these two clocks will make it almost
  -- impossible to close timing. So, we move data from Clk2x to Clk1x and then
  -- to RfClk domain. Since, we need deterministic delay in the data path, we
  -- cannot use a FIFO to do data crossing.
  --
  -- Clk1x = Sample clock/24
  -- Clk2x = Sample clock/12
  -- RfClk = Sample clock/16
  --
  -- Clk1x __/-----\_____/-----\_____/-----\_____/-----\_____/-----\___
  --                                             |   |
  -- Clk2x __/--\__/--\__/--\__/--\__/--\        |   |
  --               | |                           |   |
  --               | | <- Setup relationship     |   | <- Setup relationship
  --               | |                           |   |
  -- RfClk __/---\___/---\___/---\___/---\___/---\___/---\___/---\___/-
  --
  -- As you can see the setup relationship for passing data synchronously from
  -- Clk2x to RfClk is very small (Sample clock period * 4). It is not possible
  -- to close timing with this requirement. For passing data from Clk1x to
  -- RfClk the setup relationship is (Sample clock period * 8) which is
  -- relatively easy to close timing.

  dac_gearbox_6x12_i: entity work.dac_gearbox_6x12 (RTL)
    port map (
      Clk1x          => Clk1x,
      Clk2x          => Clk2x,
      ac1Reset_n     => ac1Reset_n,
      ac2Reset_n     => ac2Reset_n,
      c2DataIn       => c2DataIn,
      c2DataValidIn  => c2DataValidIn,
      c1DataOut      => c1DataOut,
      c1DataValidOut => c1DataValidOut
    );

  dac_gearbox_12x8_i: entity work.dac_gearbox_12x8 (RTL)
    port map (
      Clk1x           => Clk1x,
      RfClk           => RfClk,
      ac1Reset_n      => ac1Reset_n,
      arReset_n       => arReset_n,
      c1DataIn        => c1DataOut,
      c1DataValidIn   => c1DataValidOut,
      rDataOut        => rDataOut,
      rReadyForOutput => rReadyForOutput,
      rDataValidOut   => rDataValidOut
    );

end struct;
