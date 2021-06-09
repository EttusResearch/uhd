--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: ddc_400m_saturate
--
-- Description:
--
--   Saturation logic for reducing 2x24 bit words to 2x16 bit words. See
--   comments below for full description.
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

library work;
  use work.PkgRf.all;

entity ddc_400m_saturate is
  port(
    Clk           : in  std_logic;
    -- This data is from the DDC with a sample width of 17 bits and 7 bits of
    -- padding. Data format is, [Q3,I3, ... , Q0,I0] (I in LSBs)
    cDataIn       : in  std_logic_vector(191 downto 0);
    cDataValidIn  : in  std_logic;
    -- 16 bits saturated data. Data format is [Q3,I3, ... , Q0,I0] (I in LSBs)
    cDataOut      : out std_logic_vector(127 downto 0);
    cDataValidOut : out std_logic );
end ddc_400m_saturate;

architecture RTL of ddc_400m_saturate is

  signal cDataOutSamples : Samples16_t(7 downto 0) := (others => (others => '0'));
  signal cDataInSamples  : Samples17_t(cDataOutSamples'range);

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

  -- Logic to saturate input data to 16-bit signed value. Information on DDC
  -- data packer is in PkgRf.vhd.
  cDataInSamples <= to_Samples17(cDataIn);
  GenSat: for i in cDataOutSamples'range generate
    Saturation:
    process(Clk)
    begin
      if rising_edge(Clk) then
        cDataOutSamples(i) <= Saturate(cDataInSamples(i));
      end if;
    end process;
  end generate GenSat;

  DValidPipeline: process(Clk)
  begin
    if rising_edge(Clk) then
      -- Pipeline data valid to match the data.
      cDataValidOut <= cDataValidIn;
    end if;
  end process;

  cDataOut  <= to_stdlogicvector(cDataOutSamples);

end RTL;
