-------------------------------------------------------------------------------
--
-- Copyright 2018 Ettus Research, a National Instruments Company
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
--
-- Purpose:
--
-- The purpose of this module is to create a psuedo-clock "pulse" on the output
-- cPulse whenever cEnablePulse is asserted.
--
-- The output period and high time are determined by the inputs cPeriod and
-- cHighTime, where cPeriod must be greater than cHighTime+2. When these values
-- are valid at the inputs, pulse cLoadLimits to load them into the pulser routine.
-- It is not recommended to leave cEnablePulse asserted when loading new limits.
--
-- Dynamic period and duty cycle setup:
-- 1) Disable the pulser by de-asserting cEnablePulse.
-- 2) Load new period and duty cycle by modifying cPeriod and cHighTime. Pulse
--    cLoadLimits for at least one Clk cycle.
-- 3) Enable the pulser by asserting cEnablePulse.
-- 4) Repeat 1-3 as necessary.
--
-- Static period and duty cycle setup:
-- 1) Tie cLoadLimits to asserted.
-- 2) Tie cPeriod and cHighTime to static values.
-- 3) Enable and disable the pulser by asserting and de-asserting cEnablePulser at will.
--    This input can also be tied asserted in this case.
--
-- vreview_group Tdc
-------------------------------------------------------------------------------

library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;
  use ieee.math_real.all;

entity Pulser is
  generic (
    -- The pulse counter is kClksPerPulseMaxBits wide.
    -- Why 16? Then both cPeriod and cHighTime fit nicely into one 32 bit register!
    -- Minimum of 3 to make our default values for cHighTime work out.
    kClksPerPulseMaxBits : integer range 3 to 32 := 16
  );
  port (
    aReset          : in  boolean;
    Clk             : in  std_logic;

    -- Pulse cLoadLimits when cPeriod and cHighTime are valid. Is it not recommended to
    -- load new limits when the output is enabled.
    -- Alternatively, cLoadLimits can be tied high if cPeriod and cHighTime are also
    -- tied to static values.
    cLoadLimits     : in  boolean;
    cPeriod         : in  unsigned(kClksPerPulseMaxBits - 1 downto 0);
    cHighTime       : in  unsigned(kClksPerPulseMaxBits - 1 downto 0);

    -- When cEnablePulse is de-asserted, cPulse idles low on the following cycle.
    -- When asserted, cPulse will then assert within a few cycles.
    -- This input can be tied high, if desired, and the pulses will start several
    -- clock cycles after aReset de-assertion.
    cEnablePulse    : in  boolean;

    -- When cEnablePulse is asserted, cPulse will produce a rising edge every
    -- cPeriod of the Clk input and a falling edge cHighTime cycles after
    -- the rising edge.
    cPulse          : out boolean
  );
end Pulser;


architecture rtl of Pulser is

  signal cCounter,
         cPeriodStored,
         cHighTimeStored : unsigned(cPeriod'range);

  signal cSafeToStart_ms, cSafeToStart, cSafeToStartDly : boolean;

  attribute ASYNC_REG : string;
  attribute ASYNC_REG of cSafeToStart_ms : signal is "true";
  attribute ASYNC_REG of cSafeToStart    : signal is "true";

begin

  --synthesis translate_off
  CheckInputRanges : process(Clk)
  begin
    if falling_edge(Clk) then
        -- +2 since we have the output high offset from the zero of the counter
        assert (cPeriodStored > cHighTimeStored + 2)
          report "cPeriod is not greater than cHighTime + 2" severity error;
        -- Ensure the high time is greater than 1...
        assert (cHighTimeStored > 1)
          report "cHighTime is not greater than 1" severity error;
    end if;
  end process;
  --synthesis translate_on


  -- ------------------------------------------------------------------------------------
  -- !!! SAFE COUNTER STARTUP !!!
  -- This counter starts safely, meaning it cannot start counting immediately after
  -- aReset de-assertion, because the counter cannot start until cSafeToStart asserts,
  -- which cannot happen until 1-2 clock cycles after aReset de-assertion.
  -- ------------------------------------------------------------------------------------
  CountFreqRefPeriod: process(aReset, Clk)
  begin
    if aReset then
      cCounter        <= (others => '0');
      cSafeToStart_ms <= false;
      cSafeToStart    <= false;
      cSafeToStartDly <= false;
      cPulse          <= false;
      cPeriodStored   <= (others => '1');
      -- This is a rather arbitrary start value, but we are guaranteed that it is
      -- less than the reset value of cPeriodStored as well as greater than 2,
      -- so it works well enough in case the module isn't set up correctly.
      cHighTimeStored <= to_unsigned(kClksPerPulseMaxBits+2,cHighTimeStored'length);
    elsif rising_edge(Clk) then
      -- Create a safe counter startup signal that asserts shortly after
      -- aReset de-assertion.
      cSafeToStart_ms <= true;
      cSafeToStart    <= cSafeToStart_ms;
      -- In the case where cLoadLimits and cEnablePulse are tied high, we need to give
      -- them one cycle to load before starting the counter, so we delay cSafeToStart
      -- by one for the counter.
      cSafeToStartDly <= cSafeToStart;

      if cEnablePulse and cSafeToStartDly then
        -- Simple counter increment until ceiling reached, then roll over.
        if cCounter >= cPeriodStored - 1 then
          cCounter <= (others => '0');
        else
          cCounter <= cCounter + 1;
        end if;

        -- Pulse the output when counter is between 1 and cHighTimeStored.
        if cCounter = 1 then
          cPulse <= true;
        elsif cCounter >= cHighTimeStored+1 then
          cPulse <= false;
        end if;

      else
        cPulse   <= false;
        cCounter <= (others => '0');
      end if;

      if cLoadLimits and cSafeToStart then
        cPeriodStored   <= cPeriod;
        cHighTimeStored <= cHighTime;
      end if;
    end if;
  end process;

end rtl;


--------------------------------------------------------------------------------
-- Testbench for Pulser
--------------------------------------------------------------------------------

--synopsys translate_off
library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;
  use ieee.math_real.all;

entity tb_Pulser is end tb_Pulser;

architecture test of tb_Pulser is

  constant kClksPerPulseMaxBits : integer := 16;

  --vhook_sigstart
  signal aReset: boolean;
  signal cEnablePulse: boolean;
  signal cHighTime: unsigned(kClksPerPulseMaxBits-1 downto 0);
  signal Clk: std_logic := '0';
  signal cLoadLimits: boolean;
  signal cPeriod: unsigned(kClksPerPulseMaxBits-1 downto 0);
  signal cPulse: boolean;
  signal cPulseDut2: boolean;
  --vhook_sigend

  signal StopSim : boolean;
  constant kPer : time := 10 ns;

  signal CheckPulse : boolean := false;
  signal cPulseSl : std_logic := '0';
  signal cPulseDut2Sl : std_logic := '0';

  procedure ClkWait(X : positive := 1) is
  begin
    for i in 1 to X loop
      wait until rising_edge(Clk);
    end loop;
  end procedure ClkWait;

begin

  Clk <= not Clk after kPer/2 when not StopSim else '0';

  main: process
  begin
    cEnablePulse <= false;
    aReset <= true, false after 10 ns;
    ClkWait(5);

    -- Ensure the pulse is quiet for a while.
    ClkWait(100);
    assert cPulse'stable(kPer*100) and not cPulse
      report "pulse not stable at false at startup"
      severity error;


    -- Set up, then enable the pulse; expect it to go high after a few cycles.
    cPeriod   <= to_unsigned(250,cPeriod'length);
    cHighTime <= to_unsigned(100,cPeriod'length);
    cLoadLimits <= true;
    ClkWait;
    cLoadLimits <= false;
    cEnablePulse <= true;
    ClkWait(2); -- pulse rises here
    wait until falling_edge(Clk);
    assert cPulse report "cPulse not high two cycles after enabling" severity error;
    -- After another clock cycle the checker below should be primed, so we can enable it.
    ClkWait;
    CheckPulse <= true;
    ClkWait(to_integer(cHighTime)-1);
    wait until falling_edge(Clk);
    assert not cPulse report "Pulse not low after high requirement" severity error;

    -- Check the pulse high and low for a few cycles (duplicated below, but this also
    -- checks that it actually is toggling).
    for i in 0 to 100 loop
      ClkWait(to_integer(cPeriod) - to_integer(cHighTime));
      wait until falling_edge(Clk);
      assert cPulse report "Pulse not high when expected" severity error;
      ClkWait(to_integer(cHighTime));
      wait until falling_edge(Clk);
      assert not cPulse report "Pulse not low after high requirement" severity error;
    end loop;

    -- Disable pulse, and check that it goes away for a long time
    cEnablePulse <= false;
    CheckPulse   <= false;
    -- 2 is about the max time for it to go away.
    ClkWait(2);
    ClkWait(2**kClksPerPulseMaxBits);
    assert (not cPulse) and cPulse'stable(2**kClksPerPulseMaxBits*kPer)
      report "disable didn't work" severity error;


    -- Re-do all the initial tests with different periods and such.

    -- Enable the pulse, expect it to go high after a few cycles
    cPeriod   <= to_unsigned(10,cPeriod'length);
    cHighTime <= to_unsigned(5,cPeriod'length);
    cLoadLimits <= true;
    ClkWait;
    cLoadLimits <= false;
    cEnablePulse <= true;
    ClkWait(2); -- pulse rises here
    wait until falling_edge(Clk);
    assert cPulse report "cPulse not high two cycles after enabling" severity error;
    -- After another clock cycle the checker below should be primed, so we can enable it.
    ClkWait;
    CheckPulse <= true;
    ClkWait(to_integer(cHighTime)-1);
    wait until falling_edge(Clk);
    assert not cPulse report "Pulse not low after high requirement" severity error;

    -- Check the pulse high and low for a few cycles (duplicated below, but this also
    -- checks that it actually is toggling).
    for i in 0 to 100 loop
      ClkWait(to_integer(cPeriod) - to_integer(cHighTime));
      wait until falling_edge(Clk);
      assert cPulse report "Pulse not high when expected" severity error;
      ClkWait(to_integer(cHighTime));
      wait until falling_edge(Clk);
      assert not cPulse report "Pulse not low after high requirement" severity error;
    end loop;

    ClkWait(100);


    StopSim <= true;
    wait;
  end process;

  cPulseSl <= '1' when cPulse else '0';

  -- Test the period and duty cycle of the pulse.
  CheckPulseSpecs : process(cPulseSl)
    variable LastRise : time := 0 ns;
  begin
    if falling_edge(cPulseSl) then
      assert (not CheckPulse) or (now - LastRise = kPer*to_integer(cHighTime))
        report "High cycles requirement not met" severity error;
    elsif rising_edge(cPulseSl) then
      assert (not CheckPulse) or (now - LastRise = kPer*to_integer(cPeriod))
        report "Period requirement not met" & LF &
               "Act: " & time'image(now-LastRise) & LF &
               "Req: " & time'image(kPer*to_integer(cPeriod))
        severity error;
      LastRise := now;
    end if;
  end process;

  --vhook_e Pulser dutx
  dutx: entity work.Pulser (rtl)
    generic map (kClksPerPulseMaxBits => kClksPerPulseMaxBits)  --integer range 3:32 :=16
    port map (
      aReset       => aReset,        --in  boolean
      Clk          => Clk,           --in  std_logic
      cLoadLimits  => cLoadLimits,   --in  boolean
      cPeriod      => cPeriod,       --in  unsigned(kClksPerPulseMaxBits-1:0)
      cHighTime    => cHighTime,     --in  unsigned(kClksPerPulseMaxBits-1:0)
      cEnablePulse => cEnablePulse,  --in  boolean
      cPulse       => cPulse);       --out boolean


  --vhook_e Pulser dut2
  --vhook_a cLoadLimits true
  --vhook_a cPeriod    to_unsigned(5,kClksPerPulseMaxBits)
  --vhook_a cHighTime  to_unsigned(2,kClksPerPulseMaxBits)
  --vhook_a cEnablePulse true
  --vhook_a cPulse cPulseDut2
  dut2: entity work.Pulser (rtl)
    generic map (kClksPerPulseMaxBits => kClksPerPulseMaxBits)  --integer range 3:32 :=16
    port map (
      aReset       => aReset,                               --in  boolean
      Clk          => Clk,                                  --in  std_logic
      cLoadLimits  => true,                                 --in  boolean
      cPeriod      => to_unsigned(5,kClksPerPulseMaxBits),  --in  unsigned(kClksPerPulseMaxBits-1:0)
      cHighTime    => to_unsigned(2,kClksPerPulseMaxBits),  --in  unsigned(kClksPerPulseMaxBits-1:0)
      cEnablePulse => true,                                 --in  boolean
      cPulse       => cPulseDut2);                          --out boolean

  cPulseDut2Sl <= '1' when cPulseDut2 else '0';

  CheckDut2 : process (cPulseDut2Sl)
    variable LastRise : time := 0 ns;
  begin
    if falling_edge(cPulseDut2Sl) then
      assert (not CheckPulse) or (now - LastRise = kPer*2)
        report "DUT 2 High cycles requirement not met" severity error;
    elsif rising_edge(cPulseDut2Sl) then
      assert (not CheckPulse) or (now - LastRise = kPer*5)
        report "DUT 2 Period requirement not met" & LF &
               "Act: " & time'image(now-LastRise) & LF &
               "Req: " & time'image(kPer*5)
        severity error;
      LastRise := now;
    end if;
  end process;


end test;
--synopsys translate_on
