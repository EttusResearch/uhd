--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: duc_saturate
--
-- Description:
--
--   Saturation logic for reducing 2x24 bit words to 2x16 bit words. See
--   comments below for full description.
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

entity duc_saturate is
  port(
    Clk            : in  std_logic;
    cDataIn        : in  std_logic_vector(47 downto 0);
    cDataValidIn   : in  std_logic;
    cReadyForInput : out std_logic;
    cDataOut       : out std_logic_vector(31 downto 0);
    cDataValidOut  : out std_logic := '0'
  );
end duc_saturate;

architecture RTL of duc_saturate is

  signal cDataOutI : std_logic_vector(15 downto 0) := (others => '0');
  signal cDataOutQ : std_logic_vector(15 downto 0) := (others => '0');

begin

  -----------------------------------------------------------------------------
  -- Saturation
  --
  -- The output of the Xilinx FIR Compiler has already been rounded on the LSB
  -- side, but hasn't been saturated on the MSB side.
  -- Coefficients = 18 bit, 1 integer bit (1.17)
  -- Data In = 16 bits, 1 integer bit (1.15)
  -- Xilinx FIR core rounds to output to 3.31. The filter coefficients has a
  -- gain of 3 to compensate for the amplitude loss in interpolation, the
  -- Xilinx FIR core rounds the output to 3.15.
  -- Data Out = 18 bits, 3 integer bits (3.15), with 16 LSBs already rounded
  -- off inside the FIR core.
  -- We need to manually saturate the 3.15 number back to a 1.15 number
  --
  -- If 3 MSBs = 000, output <= input without MSB, e.g. positive number < 1
  -- If 3 MSBs = 0x1/01x, output <= 0.111111111111111, e.g. positive number >= 1
  -- If 3 MSBs = 1x0/10x, output <= 1.000000000000000, e.g. negative number < -1
  -- If 3 MSBs = 111, output <= input without MSB, e.g. negative number >= -1
  -----------------------------------------------------------------------------
Saturation:
  process(Clk)
  begin
    if rising_edge(Clk) then
      -- Pipeline data valid to match the data
      cDataValidOut <= cDataValidIn;

      -- I, from cDataIn(17 downto 0)
      if cDataIn(17) = '0' and cDataIn(16 downto 15) /= "00" then
        cDataOutI <= "0111111111111111";
      elsif cDataIn(17) = '1' and cDataIn(16) /=  cDataIn(15) then
        cDataOutI <= "1000000000000000";
      else
        cDataOutI <= cDataIn(15 downto 0);
      end if;

      -- Q, from cDataIn(41 downto 24)
      if cDataIn(41) = '0' and cDataIn(40 downto 39) /= "00" then
        cDataOutQ <= "0111111111111111";
      elsif cDataIn(41) = '1' and
            (not (cDataIn(40 downto 39) = "11")) then
        cDataOutQ <= "1000000000000000";
      else
        cDataOutQ <= cDataIn(39 downto 24);
      end if;

    end if;
  end process Saturation;

  cDataOut <= cDataOutQ & cDataOutI;
  cReadyForInput <= '1';

end RTL;
