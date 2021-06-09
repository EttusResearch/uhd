--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: ddc_saturate
--
-- Description:
--
--   Saturation logic for reducing 2x24 bit words to 2x16 bit words. See
--   comments below for full description.
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

entity ddc_saturate is
  port(
    Clk           : in  std_logic;
    cDataIn       : in  std_logic_vector(47 downto 0); -- [Q,I] (I in LSBs)
    cDataValidIn  : in  std_logic;
    cDataOut      : out std_logic_vector(31 downto 0); -- [Q,I] (I in LSBs)
    cDataValidOut : out std_logic
  );
end ddc_saturate;

architecture RTL of ddc_saturate is

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
  -- 1.17 * 1.15 = 2.32, and the Xilinx FIR core rounds to 2.15
  -- Data Out = 17 bits, 2 integer bits (2.15), with 17 LSBs already rounded
  -- off inside the FIR core.
  -- We need to manually saturate the 2.15 number back to a 1.15 number
  --
  -- If 2 MSBs = 00, output <= input without MSB, e.g. positive number < 1
  -- If 2 MSBs = 01, output <= 0.111111111111111, e.g. positive number >= 1
  -- If 2 MSBs = 10, output <= 1.000000000000000, e.g. negative number < -1
  -- If 2 MSBs = 11, output <= input without MSB, e.g. negative number >= -1
  -----------------------------------------------------------------------------
  Saturation:
  process(Clk)
  begin
    if rising_edge(Clk) then
      -- Pipeline data valid to match the data
      cDataValidOut <= cDataValidIn;

      -- I, from cDataIn(16 downto 0)
      if cDataIn(16 downto 15) = "01" then
        cDataOutI <= "0111111111111111";
      elsif cDataIn(16 downto 15) = "10" then
        cDataOutI <= "1000000000000000";
      else
        cDataOutI <= cDataIn(15 downto 0);
      end if;

      -- Q, from cDataIn(40 downto 24)
      if cDataIn(40 downto 39) = "01" then
        cDataOutQ <= "0111111111111111";
      elsif cDataIn(40 downto 39) = "10" then
        cDataOutQ <= "1000000000000000";
      else
        cDataOutQ <= cDataIn(39 downto 24);
      end if;

    end if;
  end process Saturation;

  cDataOut <= cDataOutQ & cDataOutI;

end RTL;
