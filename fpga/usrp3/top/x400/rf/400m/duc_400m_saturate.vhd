--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: duc_400m_saturate
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

entity duc_400m_saturate is
  port(
    Clk            : in  std_logic;
    -- This data is from the DDC with a sample width of 18 bits and 6 bits of
    -- padding. Data format is, [Q5,I5, ... , Q0,I0] (I in LSBs)
    cDataIn        : in  std_logic_vector(287 downto 0);
    cDataValidIn   : in  std_logic;
    cReadyForInput : out std_logic;
    -- 16 bits saturated data. Data format is [Q5,I5, ... , Q0,I0] (I in LSBs)
    cDataOut       : out std_logic_vector(191 downto 0);
    cDataValidOut  : out std_logic := '0');
end duc_400m_saturate;

architecture RTL of duc_400m_saturate is

  signal cDataOutSamples : Samples16_t(11 downto 0) := (others => (others => '0'));
  signal cDataInputSamples : Samples18_t(cDataOutSamples'range);

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

  -- Logic to saturate input data to 16-bit signed value. Information on DUC data packer is in
  -- PkgRf.vhd.
  cDataInputSamples <= to_Samples18(cDataIn);
  GenSat: for i in cDataOutSamples'range generate
    Saturation:
    process(Clk)
    begin
      if rising_edge(Clk) then
        cDataOutSamples(i) <= Saturate(cDataInputSamples(i));
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

  cReadyForInput <= '1';

end RTL;
