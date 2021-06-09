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

  subtype Word_t is std_logic_vector(191 downto 0);
  type Words_t is array(natural range<>) of Word_t;

  signal c1DataInDly, c2DataInDly : Words_t(2 downto 0);

  signal c2DataValidInDly : std_logic_vector(1 downto 0) := (others => '0');
  signal c1PhaseCount, c2PhaseCount : std_logic := '0';
  signal c1DataValidIn, c1DataValidDly0 : std_logic := '0';

begin

  -- Input data pipeline.
  InputValidPipeline: process(Clk2x, ac2Reset_n)
  begin
    if ac2Reset_n = '0' then
      c2DataValidInDly <= (others => '0');
    elsif rising_edge(Clk2x) then
      c2DataValidInDly <= c2DataValidInDly(c2DataValidInDly'left-1 downto 0) &
                          c2DataValidIn;
    end if;
  end process;

  InputDataPipeline: process(Clk2x)
  begin
    if rising_edge(Clk2x) then
      c2DataInDly <= c2DataInDly(c2DataInDly'high-1 downto 0) & c2DataIn;
    end if;
  end process;

  -- Process to determine if data valid was asserted when both clocks were
  -- in-phase. Since we are crossing a 2x clock domain to a 1x clock domain,
  -- there are only two possible phase. One is data valid assertion when both
  -- clocks rising edges are aligned. The other case is data valid assertion
  -- when Clk2x is aligned to the falling edge.
  Clock2xPhaseCount: process(ac2Reset_n, Clk2x)
  begin
    if ac2Reset_n = '0' then
      c2PhaseCount <= '0';
    elsif rising_edge(Clk2x) then
      -- This is a single bit counter. This counter is enabled for an extra
      -- clock cycle to account for the output pipeline delay.
      c2PhaseCount <= (not c2PhaseCount) and
                      (c2DataValidInDly(1) or c2DataValidInDly(0));
    end if;
  end process;

  -- Crossing clock from Clk2x to Clk1x.
  Clk2xToClk1xCrossing: process(Clk1x)
  begin
    if rising_edge(Clk1x) then
      c1DataInDly   <= c2DataInDly;
      c1PhaseCount  <= c2PhaseCount;
      c1DataValidIn <= c2DataValidInDly(0);
    end if;
  end process;

  -- Output data packing is determined based on when input data valid was
  -- asserted. c1PhaseCount is '1' when input data valid was asserted when both
  -- clocks are rising edge aligned. In this case, we can send data from the
  -- with 1 and 2 pipeline delays.
  -- When data valid is asserted when the two clock are not rising edge
  -- aligned, we will use data from 2 and 3 pipeline delays.
  DataOut: process(Clk1x)
  begin
    if rising_edge(Clk1x) then
      c1DataOut <= c1DataInDly(1) & c1DataInDly(2);
      if c1PhaseCount = '1' then
        c1DataOut <= c1DataInDly(0) & c1DataInDly(1);
      end if;
    end if;
  end process;

  -- Similar to data output, when input data valid is asserted and both clocks
  -- are rising edge aligned, the output data valid is asserted with a single
  -- pipeline stage. If not, output data valid is asserted with two pipeline
  -- stages.
  DataValidOut: process(Clk1x, ac1Reset_n)
  begin
    if ac1Reset_n = '0' then
      c1DataValidDly0 <= '0';
      c1DataValidOut  <= '0';
    elsif rising_edge(Clk1x) then
      c1DataValidDly0 <= c1DataValidIn;
      c1DataValidOut  <= c1DataValidDly0;
      if c1PhaseCount = '1' then
        c1DataValidOut <= c1DataValidIn;
      end if;
    end if;
  end process;

end RTL;
