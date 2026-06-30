--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: adc_gearbox_2x4
--
-- Description:
--
--   Gearbox to expand the data width from 2 SPC to 4 SPC.
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

entity adc_gearbox_2x4 is
  port(
    Clk1x          : in  std_logic;
    Clk3x          : in  std_logic;
    -- Resets with synchronous de-assertion.
    ac1Reset_n     : in  std_logic;
    ac3Reset_n     : in  std_logic;
    -- Data packing: [Q1,I1,Q0,I0] (I in LSBs).
    c3DataIn       : in  std_logic_vector(95 downto 0);
    c3DataValidIn  : in  std_logic;
    -- Data packing: [Q3,I3,Q2,I2,Q1,I1,Q0,I0] (I in LSBs).
    c1DataOut      : out std_logic_vector(191 downto 0);
    c1DataValidOut : out std_logic
  );
end adc_gearbox_2x4;

architecture RTL of adc_gearbox_2x4 is

  signal c1DataValidInDly, c3DataValidInDly
         : std_logic_vector(3 downto 0) := (others => '0');

  subtype Word_t is std_logic_vector(95 downto 0);
  type Words_t is array(natural range<>) of Word_t;

  signal c3DataInDly, c1DataInDly : Words_t(3 downto 0);

begin

  -- Pipeline input data. We will need four pipeline stages to account for the
  -- three possible Clk1x and Clk3x phases and the nature of data packing done
  -- in the DDC filter. The DDC asserts data valid for two clock cycles and
  -- de-asserted for one clock cycle. This requires us to have shift register
  -- that is 4 sample words (each sample word is 2 SPC) deep.
  InputValidPipeline: process(Clk3x, ac3Reset_n)
  begin
    if ac3Reset_n = '0' then
      c3DataValidInDly <= (others => '0');
    -- These registers are on the falling edge to prevent a hold violation at
    -- the input to the following Clk1x FF (which may arrive late when more
    -- heavily loaded than Clk3x)
    elsif falling_edge(Clk3x) then
      c3DataValidInDly <= c3DataValidInDly(c3DataValidInDly'left-1 downto 0) &
                          c3DataValidIn;
    end if;
  end process;

  InputDataPipeline: process(Clk3x)
  begin
    -- These registers are on the falling edge to prevent a hold violation at
    -- the input to the following Clk1x FF (which may arrive late when more
    -- heavily loaded than Clk3x).
    if falling_edge(Clk3x) then
      c3DataInDly <= c3DataInDly(c3DataInDly'high-1 downto 0) & c3DataIn;
    end if;
  end process InputDataPipeline;

  -- Data valid clock crossing from Clk3x to Clk1x
  Clk3xToClk1xValidCrossing: process(Clk1x, ac1Reset_n)
  begin
    if ac1Reset_n = '0' then
      c1DataValidInDly <= (others => '0');
    elsif rising_edge(Clk1x) then
      c1DataValidInDly <= c3DataValidInDly;
    end if;
  end process;

  -- Data clock crossing from Clk3x to Clk1x
  Clk3xToClk1xDataCrossing: process(Clk1x)
  begin
    if rising_edge(Clk1x) then
      c1DataInDly <= c3DataInDly;
    end if;
  end process;

  -----------------------------------------------------------------------------
  --
  --                       p0              p1              p2              p0
  -- Clk3x          _______/¯¯¯¯¯¯¯\_______/¯¯¯¯¯¯¯\_______/¯¯¯¯¯¯¯\_______/¯¯¯
  --
  -- Clk1x          _______/¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯\_______________________/¯¯¯
  --
  -- c3DataValidIn  _/¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯\_______________/¯¯¯¯¯¯¯¯¯¯¯¯¯¯
  --
  -- This gearbox connect the DDC filter output to the remaining RX data path.
  -- For efficient use of DSP slices we run the DDC at 3x clock rate.  Both
  -- Clk3x and Clk1x are sourced from the same PLL and is phase locked as shown
  -- in the above timing diagram. The output of DDC filter is asserted for two
  -- clock cycles and is de-asserted for one clock cycle. The remaining part of
  -- the design cannot run at 3x clock rate. So, we increase the number of
  -- samples per clock cycle and decrease the clock frequency to 1x. Depending
  -- upon the pipeline delay through the filter and RF section, the phase of
  -- data valid assertion could be on either p0, p1, or p2 edge. And depending
  -- upon the phase, data packing to Clk1x domain will vary. Since there are
  -- three possible phase, we will need three different data packing options.
  --
  -- Data packing is done by looking for two consecutive ones in the data valid
  -- shift register (c1DataValidInDly).This pattern can be used only because of
  -- the way output data is packed in the filter. If we see two consecutive
  -- ones, then we know that we have enough data to be packed for the output of
  -- this gearbox. This is because, we need two Clk3x cycles of 2 SPC data to
  -- pack a 4 SPC data output on Clk1x. The location of two consecutive ones in
  -- the data valid shift register will provide the location of valid data in
  -- data shift register (c1DataInDly).
  DataPacker: process(Clk1x)
  begin
    if rising_edge(Clk1x) then
      -- Data valid is asserted when both Clk1x and Clk3x are phase aligned
      -- (p0). In this case, c1DataValidInDly will have consecutive ones in
      -- index 1 and 2.
      c1DataValidOut <= c1DataValidInDly(1) and c1DataValidInDly(2);
      c1DataOut <= c1DataInDly(1) & c1DataInDly(2);

      -- Data valid asserted on phase p1.
      if c1DataValidInDly(1 downto 0) = "11" then
        c1DataOut <= c1DataInDly(0) & c1DataInDly(1);
        c1DataValidOut <= '1';

      -- Data valid asserted on phase p2.
      elsif c1DataValidInDly(3 downto 2) = "11" then
        c1DataOut <= c1DataInDly(2) & c1DataInDly(3);
        c1DataValidOut <= '1';
      end if;
    end if;
  end process;

end RTL;
