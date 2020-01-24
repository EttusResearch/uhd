-------------------------------------------------------------------------------
--
-- Copyright 2018 Ettus Research, a National Instruments Company
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
--
-- Purpose:
--
-- Uses the RP and SP edges to cross a trigger from the RefClk domain to
-- the SampleClk domain. The RP FE captures the input trigger and sends it to
-- the SampleClk domain. There, it is double-synchronized but only allowed to pass
-- when the SP RE occurs. The trigger (now in the SampleClk domain) is then passed
-- through an elastic buffer before being sent on it's merry way.
--
-- Below is the latency through this module. If you assert rTriggerIn before or after
-- the rRP RE, then you need to add/subtract the distance to the rRP RE.
--
-- Deterministic latency through this module is (starting at the rRP RE):
--    Measured difference between rRP and sSP rising edges (using a TDC, positive value
--      if rRP rises before sSP).
--  + One period of sSP
--  + Two periods of SampleClk (Double Sync)
--  + (sElasticBufferPtr value + 1) * SampleClk Period
--  + One period of SampleClk
--
-- How much skew between RP and SP can we allow and still safely pass triggers?
-- Our "launch" edge is essentially the RP FE, and our "latch" edge is the SP RE.
-- Consider the no skew (RP and SP edges align) case first. Our setup and hold budget
-- is balanced at T/2. Based on this, it seems we can tolerate almost T/2 skew in either
-- direction (ignoring a few Reference and Sample Clock cycles here and there).
-- My recommendation is to keep the skew to a minimum, like less than T/4.
-- In the context of the FTDC project for N310, this should be a no-brainer since
-- the SP pulses are started only a few RefClk cycles after RP. The skew is
-- easily verified by taking a FTDC measurement. If the skew is less than T/4, you can
-- sleep easy. If not, then I recommend doing a comprehensive analysis of how much
-- settling time you have between the trigger being launched from the RefClk domain
-- and latched in the SampleClk domain.
--
-- vreview_group Tdc
-------------------------------------------------------------------------------

library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;

library unisim;
  use unisim.vcomponents.all;


entity CrossTrigger is
  port (
    aReset          : in  boolean;

    RefClk          : in  std_logic;
    -- For convenience while writing this, I have only considered the N3x0 case where
    -- rRP is slightly ahead of sSP in phase.
    rRP             : in  boolean;
    -- De-asserts the clock cycle after rTriggerIn asserts. Re-asserts after the
    -- second falling edge of rRP, indicating new triggers can be accepted.
    rReadyForInput  : out boolean;
    -- Only one pulse will be output for each rising edge of rTriggerIn. rTriggerIn is
    -- ignored when rReadyForInput is de-asserted. All levels are ignored when Enable
    -- is de-asserted.
    rEnableTrigger  : in  boolean;
    rTriggerIn      : in  boolean;

    SampleClk       : in  std_logic;
    sSP             : in  boolean;
    -- An elastic buffer just before the output is used to compensate for skew
    -- in sSP pulses across boards. Default should be in the middle of the 4 bit
    -- range at 7.
    sElasticBufferPtr : in unsigned(3 downto 0);
    -- Single-cycle pulse output.
    sTriggerOut     : out boolean
  );
end CrossTrigger;


architecture rtl of CrossTrigger is

  --vhook_sigstart
  --vhook_sigend

  signal rRpFE,
         rRpDly,
         rTriggerToSClk,
         rTriggerCaptured,
         sSpRE,
         sSpDly : boolean;

  signal sTriggerBuffer : unsigned(2**sElasticBufferPtr'length-1 downto 0);
  signal sTriggerInSClk, sTriggerInSClk_ms : boolean;

  function to_StdLogic(b : boolean) return std_ulogic is
  begin
    if b then
      return '1';
    else
      return '0';
    end if;
  end to_StdLogic;

  function to_Boolean (s : std_ulogic) return boolean is
  begin
    return (To_X01(s)='1');
  end to_Boolean;

  attribute async_reg : string;
  attribute async_reg of sTriggerInSClk    : signal is "TRUE";
  attribute async_reg of sTriggerInSClk_ms : signal is "TRUE";

begin

  -- Reference Clock Domain Trigger Capture : -------------------------------------------
  -- The trigger input is captured whenever it is high. The captured value is reset
  -- by the falling edge of rRP.
  -- ------------------------------------------------------------------------------------

  rRpFE <= rRpDly and not rRP;

  CaptureTrigger : process(aReset, RefClk)
  begin
    if aReset then
      rTriggerCaptured <= false;
      rRpDly <= false;
    elsif rising_edge(RefClk) then
      rRpDly <= rRP;
      if not rEnableTrigger then
        rTriggerCaptured <= false;
      elsif rTriggerIn then
        -- Capture trigger whenever the input is asserted (so this will work with single
        -- cycle and multi-cycle pulses).
        rTriggerCaptured <= true;
      elsif rRpFE then
        -- Reset the captured trigger one cycle after the rRP FE.
        rTriggerCaptured <= false;
      end if;
    end if;
  end process;


  -- Send Trigger To Sample Clock Domain : ----------------------------------------------
  -- Send the captured trigger on the falling edge of rRP.
  -- ------------------------------------------------------------------------------------
  SendTrigger : process(aReset, RefClk)
  begin
    if aReset then
      rTriggerToSClk <= false;
    elsif rising_edge(RefClk) then
      if not rEnableTrigger then
        rTriggerToSClk <= false;
      elsif rRpFE then
        rTriggerToSClk <= rTriggerCaptured;
      end if;
    end if;
  end process;

  rReadyForInput <= not (rTriggerToSClk or rTriggerCaptured);

  -- Capture Trigger in Sample Clock Domain : -------------------------------------------
  -- On the rising edge of sSP, capture the trigger. To keep things free of
  -- metastability, we double-sync the trigger into the SampleClk domain first.
  -- ------------------------------------------------------------------------------------

  ReceiveAndProcessTrigger : process(aReset, SampleClk)
  begin
    if aReset then
      sSpDly           <= false;
      sTriggerBuffer    <= (others => '0');
      sTriggerOut       <= false;
      sTriggerInSClk_ms <= false;
      sTriggerInSClk    <= false;
    elsif rising_edge(SampleClk) then
      -- Edge detector delays.
      sSpDly           <= sSP;

      -- Double-synchronizer for trigger.
      sTriggerInSClk_ms <= rTriggerToSClk;
      sTriggerInSClk    <= sTriggerInSClk_ms;

      -- Delay chain for the elastic buffer. Move to the left people! Note that this
      -- operation incurs at least one cycle of delay. Also note the trigger input is
      -- gated with the SP RE.
      sTriggerBuffer <= sTriggerBuffer(sTriggerBuffer'high-1 downto 0) &
                        to_stdlogic(sTriggerInSClk and sSpRE);

      -- Based on the buffer pointer value select and flop the output one more time.
      sTriggerOut <= to_boolean(sTriggerBuffer(to_integer(sElasticBufferPtr)));
    end if;
  end process;

  -- Rising edge detectors.
  sSpRE <= sSP and not sSpDly;


end rtl;





--------------------------------------------------------------------------------
-- Testbench for CrossTrigger
--
-- Meh coverage on the triggers so far... but this tests general operation
-- and latency.
--------------------------------------------------------------------------------

--synopsys translate_off
library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;

entity tb_CrossTrigger is end tb_CrossTrigger;

architecture test of tb_CrossTrigger is

  -- Sets up a 1.25 MHz period.
  constant kClksPerPulseMaxBits: integer := 10;
  constant kRpPeriodInRClks   : integer := 8;
  constant kRpHighTimeInRClks : integer := 4;
  constant kSpPeriodInRClks   : integer := 100;
  constant kSpHighTimeInRClks : integer := 50;

  --vhook_sigstart
  signal aReset: boolean;
  signal RefClk: std_logic := '0';
  signal rEnablePulser: boolean;
  signal rEnableTrigger: boolean;
  signal rReadyForInput: boolean;
  signal rRP: boolean;
  signal rTriggerIn: boolean;
  signal SampleClk: std_logic := '0';
  signal sElasticBufferPtr: unsigned(3 downto 0);
  signal sEnablePulser: boolean;
  signal sSP: boolean;
  signal sTriggerOut: boolean;
  --vhook_sigend

  signal StopSim : boolean;
  -- shared variable Rand : Random_t;
  constant kSPer : time :=   8.000 ns; -- 125.00 MHz
  constant kRPer : time := 100.000 ns; --  10.00 MHz

  signal rRfiExpected: boolean:= true;
  signal sTriggerOutExpected: boolean:= false;

  procedure ClkWait(
    signal   Clk   : in std_logic;
    X : positive := 1) is
  begin
    for i in 1 to X loop
      wait until rising_edge(Clk);
    end loop;
  end procedure ClkWait;

begin

  SampleClk   <= not SampleClk   after kSPer/2 when not StopSim else '0';
  RefClk      <= not RefClk      after kRPer/2 when not StopSim else '0';

  --vhook_e Pulser RpPulser
  --vhook_a Clk            RefClk
  --vhook_a cLoadLimits    true
  --vhook_a cPeriod        to_unsigned(kRpPeriodInRClks,kClksPerPulseMaxBits)
  --vhook_a cHighTime      to_unsigned(kRpHighTimeInRClks,kClksPerPulseMaxBits)
  --vhook_a cEnablePulse   rEnablePulser
  --vhook_a cPulse         rRP
  RpPulser: entity work.Pulser (rtl)
    generic map (kClksPerPulseMaxBits => kClksPerPulseMaxBits)  --integer range 3:32 :=16
    port map (
      aReset       => aReset,                                                --in  boolean
      Clk          => RefClk,                                                --in  std_logic
      cLoadLimits  => true,                                                  --in  boolean
      cPeriod      => to_unsigned(kRpPeriodInRClks,kClksPerPulseMaxBits),    --in  unsigned(kClksPerPulseMaxBits-1:0)
      cHighTime    => to_unsigned(kRpHighTimeInRClks,kClksPerPulseMaxBits),  --in  unsigned(kClksPerPulseMaxBits-1:0)
      cEnablePulse => rEnablePulser,                                         --in  boolean
      cPulse       => rRP);                                                  --out boolean

  --vhook_e Pulser SpPulser
  --vhook_a Clk            SampleClk
  --vhook_a cLoadLimits    true
  --vhook_a cPeriod        to_unsigned(kSpPeriodInRClks,kClksPerPulseMaxBits)
  --vhook_a cHighTime      to_unsigned(kSpHighTimeInRClks,kClksPerPulseMaxBits)
  --vhook_a cEnablePulse   sEnablePulser
  --vhook_a cPulse         sSP
  SpPulser: entity work.Pulser (rtl)
    generic map (kClksPerPulseMaxBits => kClksPerPulseMaxBits)  --integer range 3:32 :=16
    port map (
      aReset       => aReset,                                                --in  boolean
      Clk          => SampleClk,                                             --in  std_logic
      cLoadLimits  => true,                                                  --in  boolean
      cPeriod      => to_unsigned(kSpPeriodInRClks,kClksPerPulseMaxBits),    --in  unsigned(kClksPerPulseMaxBits-1:0)
      cHighTime    => to_unsigned(kSpHighTimeInRClks,kClksPerPulseMaxBits),  --in  unsigned(kClksPerPulseMaxBits-1:0)
      cEnablePulse => sEnablePulser,                                         --in  boolean
      cPulse       => sSP);                                                  --out boolean


  main: process
    procedure SendTrigger is
    begin
      assert rReadyForInput
        report "RFI isn't high, so we can't issue a trigger" severity error;

      -- Give it some action. We need to ideally test this for every phase offset of
      -- rTriggerIn with respect to the rising edge of rRP, but let's get to that later.
      -- For now, wait until a rising edge on rRP and then wait for most of the period
      -- to issue the trigger.
      wait until rRP and not rRP'delayed;
      wait for (kRpPeriodInRClks-3)*kRPer;
      rTriggerIn <= true;
      ClkWait(RefClk);
      rTriggerIn <= false;
      rRfiExpected <= false;

      -- At this point, we wait until a sSP RE, plus two SampleClks, plus sElasticBufferPtr
      -- plus 1 worth of SampleClks, plus one more SampleClk, and then the trigger
      -- should appear.
      wait until not rRP and rRP'delayed;
      wait until sSP and not sSP'delayed;
      ClkWait(SampleClk,1);
      ClkWait(SampleClk, to_integer(sElasticBufferPtr)+1);
      sTriggerOutExpected <= true;
      ClkWait(SampleClk,1);
      sTriggerOutExpected <= false;
      wait until not rRP and rRP'delayed;
      ClkWait(RefClk,1);
      rRfiExpected <= true;
    end procedure SendTrigger;

  begin
    rEnablePulser <= false;
    sEnablePulser <= false;
    rEnableTrigger <= true;
    sElasticBufferPtr <= to_unsigned(7, sElasticBufferPtr'length);

    aReset <= true, false after 10 ns;
    ClkWait(RefClk,5);

    -- Start up the pulsers and ensure nothing comes out of the trigger for a while.
    rEnablePulser <= true;
    ClkWait(RefClk, 3);
    ClkWait(SampleClk, 2);
    sEnablePulser <= true;

    ClkWait(RefClk, kRpPeriodInRClks*5);
    assert (not sTriggerOut) and sTriggerOut'stable(kRpPeriodInRClks*5*kRPer)
      report "Rogue activity on sTriggerOut before rTriggerIn asserted!" severity error;
    assert (rReadyForInput) and rReadyForInput'stable(kRpPeriodInRClks*5*kRPer)
      report "Ready for Input was not high before trigger!" severity error;


    SendTrigger;

    ClkWait(RefClk, kRpPeriodInRClks*5);

    SendTrigger;

    ClkWait(RefClk, kRpPeriodInRClks*5);

    -- Turn off the trigger enable and send a trigger.
    rEnableTrigger <= false;
    ClkWait(RefClk);
    rTriggerIn <= true;
    ClkWait(RefClk);
    rTriggerIn <= false;

    -- And nothing should happen.
    ClkWait(RefClk, kRpPeriodInRClks*5);
    assert (not sTriggerOut) and sTriggerOut'stable(kRpPeriodInRClks*5*kRPer)
      report "Rogue activity on sTriggerOut before rTriggerIn asserted!" severity error;


    ClkWait(RefClk, kRpPeriodInRClks*5);
    StopSim <= true;
    wait;
  end process;


  CheckRfi : process(RefClk)
  begin
    if falling_edge(RefClk) then
      assert rReadyForInput = rRfiExpected
        report "RFI didn't match expected" severity error;
    end if;
  end process;

  CheckTrigOut : process(SampleClk)
  begin
    if falling_edge(SampleClk) then
      assert sTriggerOut = sTriggerOutExpected
        report "Trigger Out didn't match expected" severity error;
    end if;
  end process;


  --vhook_e CrossTrigger dutx
  dutx: entity work.CrossTrigger (rtl)
    port map (
      aReset            => aReset,             --in  boolean
      RefClk            => RefClk,             --in  std_logic
      rRP               => rRP,                --in  boolean
      rReadyForInput    => rReadyForInput,     --out boolean
      rEnableTrigger    => rEnableTrigger,     --in  boolean
      rTriggerIn        => rTriggerIn,         --in  boolean
      SampleClk         => SampleClk,          --in  std_logic
      sSP               => sSP,                --in  boolean
      sElasticBufferPtr => sElasticBufferPtr,  --in  unsigned(3:0)
      sTriggerOut       => sTriggerOut);       --out boolean


end test;
--synopsys translate_on
