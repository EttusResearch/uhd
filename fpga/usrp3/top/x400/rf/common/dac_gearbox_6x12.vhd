--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: dac_gearbox_6x12
--
-- Description:
--
--   Gearbox to expand the data width from 6 SPC to 12 SPC.
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

entity dac_gearbox_6x12 is
  port(
    Clk1x          : in  std_logic;
    Clk2x          : in  std_logic;
    ac1Reset_n     : in  std_logic;
    ac2Reset_n     : in  std_logic;
    -- 16 bit data packing: [Q5,I5,Q4,I4,Q3,I3,Q2,I2,Q1,I1,Q0,I0] (I in LSBs)
    c2DataIn       : in  std_logic_vector(191 downto 0);
    c2DataValidIn  : in  std_logic;
    -- 16 bit data packing: [Q11,I11,Q10,I10,..,Q2,I2,Q1,I1,Q0,I0] (I in LSBs)
    c1DataOut      : out std_logic_vector(383 downto 0) := (others => '0');
    c1DataValidOut : out std_logic := '0'
  );
end dac_gearbox_6x12;

architecture RTL of dac_gearbox_6x12 is

  -- Even-phase word (first of each pair) held in Clk2x domain.
  signal c2EvenData    : std_logic_vector(191 downto 0) := (others => '0');
  -- Assembled 12-SPC pair; stable for two consecutive Clk2x cycles so
  -- the Clk1x register capture (see below) sees it regardless of phase.
  signal c2PairData    : std_logic_vector(383 downto 0) := (others => '0');
  -- Two-bit shift-register valid window: set to "11" when a pair is complete,
  -- then shifts to "01" then "00".  The two-cycle window spans exactly one
  -- Clk1x period so Clk1x captures the valid exactly once.
  signal c2PairValidSr : std_logic_vector(1 downto 0)  := (others => '0');
  -- Toggles only on valid input cycles so stalls never advance the phase.
  signal c2PhaseCount  : std_logic := '0';

begin

  -- Clk2x domain: accumulate pairs of 6-SPC words into a 12-SPC word.
  -- This mirrors the first-sample / sample-pair collection in adc_cdc:
  -- c2PhaseCount acts like first_sample_valid (toggling only on valid cycles),
  -- c2PairData / c2PairValidSr act like sample_pair_rclk / sample_pair_valid_rclk.
  Clk2xAccumulate: process(Clk2x, ac2Reset_n)
  begin
    if ac2Reset_n = '0' then
      c2PhaseCount  <= '0';
      c2PairValidSr <= (others => '0');
    elsif rising_edge(Clk2x) then
      -- Shift the valid window down each cycle so it expires after two cycles.
      c2PairValidSr <= '0' & c2PairValidSr(1);
      if c2DataValidIn = '1' then
        c2PhaseCount <= not c2PhaseCount;
        if c2PhaseCount = '0' then
          -- First word of the pair (even phase).
          c2EvenData <= c2DataIn;
        else
          -- Second word of the pair (odd phase); latch the complete 12-SPC
          -- word and open a two-cycle valid window for the Clk1x capture.
          c2PairData    <= c2DataIn & c2EvenData;
          c2PairValidSr <= "11";
        end if;
      end if;
    end if;
  end process;

  -- Clk1x domain: plain register capture of the assembled pair.
  -- Safe because the static phase relationship between Clk1x and Clk2x
  -- guarantees c2PairData is stable during the capture window, matching
  -- the sample_pair_rclk -> sample_pair_bclk register in adc_cdc.
  Clk2xToClk1xCrossing: process(Clk1x, ac1Reset_n)
  begin
    if ac1Reset_n = '0' then
      c1DataValidOut <= '0';
    elsif rising_edge(Clk1x) then
      c1DataOut      <= c2PairData;
      c1DataValidOut <= c2PairValidSr(1) or c2PairValidSr(0);
    end if;
  end process;

end RTL;
