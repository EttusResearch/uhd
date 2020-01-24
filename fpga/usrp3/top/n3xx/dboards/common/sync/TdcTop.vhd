-------------------------------------------------------------------------------
--
-- Copyright 2018 Ettus Research, a National Instruments Company
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
--
-- Purpose:
--
-- This top level module orchestrates both of the TDC Cores for the RP and SP. It
-- handles PPS capture, resets, re-run logic, and PPS crossing logic. The guts of the TDC
-- are all located in the Cores.
--
-- This file (and the Cores) follows exactly the "TDC Detail" diagram from this document:
-- //MI/RF/HW/USRP/N310/HWCode/Common/Synchronization/design/Diagrams.vsdx
--
--
--
-- To control this module:
--  0) Default values expected to be driven on the control inputs:
--       aReset     <= true
--       rResetTdc  <= true
--       rEnableTdc <= false
--       rReRunEnable <= false
--       rEnablePpsCrossing   <= false
--       sPpsClkCrossDelayVal <= don't care
--     Prior to starting the core, the Sync Pulse counters must be loaded. Apply the
--     correct count values to rRpPeriodInRClks, etc, and then pulse the load bit for
--     each RP and SP. It is critical that this step is performed before de-asserting
--     reset.
--
--  1) De-assert the global reset, aReset, as well as the synchronous reset, rResetTdc,
--     after all clocks are active and stable. Wait until rResetTdcDone is de-asserted.
--     If it doesn't de-assert, then one of your clocks isn't running.
--
--  2) At any point after rResetTdcDone de-asserts it is safe to assert rEnableTdc.
--     The rPpsPulse input is now actively listening for PPS activity and the TDC
--     will begin on the first PPS pulse received. After a PPS is received, the
--     rPpsPulseCaptured bit will assert and will remain asserted until aReset or
--     rResetTdc is asserted.
--
--  3) When the TDC measurement completes, mRpOffsetDone and mSpOffsetDone will assert
--     (not necessarily at the same time). The results of the measurements will be valid
--     on mRpOffset and mSpOffset.
--
--  4) To cross the PPS trigger into the SampleClk domain, first write the correct delay
--     value to sPpsClkCrossDelayVal. Then (or at the same time), enable the crossing
--     logic by asserting rEnablePpsCrossing. All subsequent PPS pulses will be crossed
--     deterministically. Although not the typical use case, sPpsClkCrossDelayVal can
--     be adjusted on the fly without producing output glitches, although output pulses
--     may be skipped.
--
--  5) To run the measurement again, assert the rReRunEnable input and capture the new
--     offsets whenever mRpOffsetValid or mSpOffsetValid asserts.
--
--
--
-- Sync Pulse = RP and SP, which are the repeated pulses that are some integer
--  divisor of the Reference and Sample clocks. RP = Reference Pulse in the
--  RefClk domain. SP = Repeated TClk pulse in the SampleClk domain.
--
--
-- Clock period relationship requirements to meet system concerns:
--   1) MeasClkPeriod < 2*RefClkPeriod
--   2) MeasClkPeriod < 4*SampleClkPeriod
--
--
-- vreview_group Tdc
-------------------------------------------------------------------------------

library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;
  use ieee.math_real.all;

entity TdcTop is
  generic (
    -- Determines the maximum number of bits required to create the restart
    -- pulser. This value is based off of the RefClk and RePulse rates.
    kRClksPerRePulsePeriodBitsMax : integer range 3 to 32 := 24;
    -- Determines the maximum number of bits required to create the Gated and Freerunning
    -- sync pulsers. This value is based off of the RefClk and SyncPulse rates.
    kRClksPerRpPeriodBitsMax  : integer range 3 to 16 := 16;
    -- This value is based off of the SampleClk and SyncPulse rates.
    kSClksPerSpPeriodBitsMax  : integer range 3 to 16 := 16;
    -- Number of MeasClk periods required to count one period of RP or SP (in bits).
    kPulsePeriodCntSize       : integer := 13;
    -- Number of FreqRef periods to be measured (in bits).
    kFreqRefPeriodsToCheckSize: integer := 17;
    -- Number of Sync Pulse Periods to be timestamped (in bits).
    kSyncPeriodsToStampSize   : integer := 10
  );
  port (

    -- Clocks and Resets : --------------------------------------------------------------
    -- Asynchronous global reset.
    aReset          : in  boolean;
    -- Reference Clock
    RefClk          : in  std_logic;
    -- Sample Clock
    SampleClk       : in  std_logic;
    -- Measurement Clock must run at a very specific frequency, determined by the
    -- SampleClk, RefClk, and Sync Pulse rates... oh and a lot of math/luck.
    MeasClk         : in  std_logic;


    -- Controls and Status : ------------------------------------------------------------
    -- Soft reset for the module. Wait until rResetTdcDone asserts before de-asserting
    -- the reset.
    rResetTdc          : in  boolean;
    rResetTdcDone      : out boolean;
    -- Once enabled, the TDC waits for the next PPS pulse to begin measurements. Leave
    -- this signal asserted for the measurement duration (there is no need to de-assert
    -- it unless you want to capture a different PPS edge).
    rEnableTdc         : in  boolean;
    -- Assert this bit to allow the TDC to perform repeated measurements.
    rReRunEnable       : in  boolean;

    -- Only required to pulse 1 RefClk cycle.
    rPpsPulse          : in  boolean;
    -- Debug, held asserted when pulse is captured.
    rPpsPulseCaptured  : out boolean;

    -- Programmable value for delaying the RP and SP pulsers from when the Restart
    -- Pulser begins.
    rPulserEnableDelayVal : in  unsigned(3 downto 0);


    -- Crossing PPS into Sample Clock : -------------------------------------------------
    -- Enable crossing rPpsPulse into SampleClk domain. This should remain de-asserted
    -- until the TDC measurements are complete and sPpsClkCrossDelayVal is written.
    rEnablePpsCrossing   : in  boolean;
    -- Programmable delay value for crossing clock domains. This is used to compensate
    -- for differences in sSP pulses across modules. This value is typically set once
    -- after running initial synchronization.
    sPpsClkCrossDelayVal : in  unsigned(3 downto 0);
    -- PPS pulse output on the SampleClk domain.
    sPpsPulse            : out boolean;


    -- FTDC Measurement Results : -------------------------------------------------------
    -- Final FTDC measurements in MeasClk ticks. Done will assert when *Offset
    -- becomes valid and will remain asserted until aReset or rResetTdc asserts.
    -- FXP<+40,13> where kPulsePeriodCntSize is the number of integer bits.
    mRpOffset       : out unsigned(kPulsePeriodCntSize+
                                   kSyncPeriodsToStampSize+
                                   kFreqRefPeriodsToCheckSize-1 downto 0);
    mSpOffset       : out unsigned(kPulsePeriodCntSize+
                                   kSyncPeriodsToStampSize+
                                   kFreqRefPeriodsToCheckSize-1 downto 0);
    mOffsetsDone    : out boolean;
    mOffsetsValid   : out boolean;


    -- Setup for Pulsers : --------------------------------------------------------------
    -- Only load these counts when rResetTdc is asserted and rEnableTdc is de-asserted!!!
    -- If both of the above conditions are met, load the counts by pulsing Load
    -- when the counts are valid. It is not necessary to keep the count values valid
    -- after pulsing Load.
    rLoadRePulseCounts      : in boolean; -- RePulse
    rRePulsePeriodInRClks   : in unsigned(kRClksPerRePulsePeriodBitsMax - 1 downto 0);
    rRePulseHighTimeInRClks : in unsigned(kRClksPerRePulsePeriodBitsMax - 1 downto 0);
    rLoadRpCounts       : in boolean; -- RP
    rRpPeriodInRClks    : in unsigned(kRClksPerRpPeriodBitsMax - 1 downto 0);
    rRpHighTimeInRClks  : in unsigned(kRClksPerRpPeriodBitsMax - 1 downto 0);
    rLoadRptCounts      : in boolean; -- RP-transfer
    rRptPeriodInRClks   : in unsigned(kRClksPerRpPeriodBitsMax - 1 downto 0);
    rRptHighTimeInRClks : in unsigned(kRClksPerRpPeriodBitsMax - 1 downto 0);
    sLoadSpCounts       : in boolean; -- SP
    sSpPeriodInSClks    : in unsigned(kSClksPerSpPeriodBitsMax - 1 downto 0);
    sSpHighTimeInSClks  : in unsigned(kSClksPerSpPeriodBitsMax - 1 downto 0);
    sLoadSptCounts      : in boolean; -- SP-transfer
    sSptPeriodInSClks   : in unsigned(kSClksPerSpPeriodBitsMax - 1 downto 0);
    sSptHighTimeInSClks : in unsigned(kSClksPerSpPeriodBitsMax - 1 downto 0);


    -- Sync Pulse Outputs : -------------------------------------------------------------
    -- The repeating pulses can be useful for many things, including passing triggers.
    -- The rising edges will always have a fixed (but unknown) phase relationship to one
    -- another. This fixed phase relationship is valid across daughterboards and all
    -- modules using the same Reference Clock and Sample Clock rates and sources.
    rRpTransfer        : out boolean;
    sSpTransfer        : out boolean;

    -- Pin bouncers out and in. Must go to unused and unconnected pins on the FPGA!
    rGatedPulseToPin   : inout std_logic;
    sGatedPulseToPin   : inout std_logic
  );
end TdcTop;


architecture struct of TdcTop is

  component TdcCore
    generic (
      kSourceClksPerPulseMaxBits : integer range 3 to 16 := 16;
      kPulsePeriodCntSize        : integer := 13;
      kFreqRefPeriodsToCheckSize : integer := 17;
      kSyncPeriodsToStampSize    : integer := 10);
    port (
      aReset             : in  boolean;
      MeasClk            : in  std_logic;
      mResetPeriodMeas   : in  boolean;
      mPeriodMeasDone    : out boolean;
      mResetTdcMeas      : in  boolean;
      mRunTdcMeas        : in  boolean;
      mGatedPulse        : out boolean;
      mAvgOffset         : out unsigned(kPulsePeriodCntSize+kSyncPeriodsToStampSize+kFreqRefPeriodsToCheckSize-1 downto 0);
      mAvgOffsetDone     : out boolean;
      mAvgOffsetValid    : out boolean;
      SourceClk          : in  std_logic;
      sResetTdc          : in  boolean;
      sSyncPulseLoadCnt  : in  boolean;
      sSyncPulsePeriod   : in  unsigned(kSourceClksPerPulseMaxBits-1 downto 0);
      sSyncPulseHighTime : in  unsigned(kSourceClksPerPulseMaxBits-1 downto 0);
      sSyncPulseEnable   : in  boolean;
      sGatedPulse        : out boolean;
      sGatedPulseToPin   : inout std_logic);
  end component;

  --vhook_sigstart
  signal mRP: boolean;
  signal mRpOffsetDoneLcl: boolean;
  signal mRpOffsetValidLcl: boolean;
  signal mRunTdc: boolean;
  signal mSP: boolean;
  signal mSpOffsetDoneLcl: boolean;
  signal mSpOffsetValidLcl: boolean;
  signal rCrossTrigRFI: boolean;
  signal rGatedCptrPulseIn: boolean;
  signal rRePulse: boolean;
  signal rRePulseEnable: boolean;
  signal rRpEnable: boolean;
  signal rRptPulse: boolean;
  signal sSpEnable: boolean;
  signal sSptPulse: boolean;
  --vhook_sigend

  signal sSpEnable_ms : boolean;

  -- Delay chain for enables.
  constant kDelaySizeForRpEnable  : integer := 15;
  constant kAddtlDelayForSpEnable : integer := 3;
  signal rSyncPulseEnableDly :
    std_logic_vector(kDelaySizeForRpEnable+
                     kAddtlDelayForSpEnable-1 downto 0) := (others => '0');
  -- Adding kAddtlDelayForSpEnable stages, so this vector needs to handle one extra
  -- bit of range (hence no -1 downto 0).
  signal rSyncPulseEnableDlyVal : unsigned(rPulserEnableDelayVal'length downto 0);

  signal rResetTdcFlop_ms, rResetTdcFlop,
         rResetTdcDone_ms,
         rSpEnable,
         mRunTdcEnable_ms, mRunTdcEnable,
         mRunTdcEnableDly, mRunTdcEnableRe,
         mResetTdc_ms,     mResetTdc,
         sResetTdc_ms,     sResetTdc,
         mRpValidStored,  mSpValidStored,
         mOffsetsValidLcl,
         rPpsPulseDly,  rPpsPulseRe,
         mReRunEnable_ms,  mReRunEnable  : boolean;

  signal rPpsCaptured : std_logic;

  type EnableFsmState_t is (Disabled, WaitForRunComplete, ReRuns);
  signal mEnableState : EnableFsmState_t;

  attribute ASYNC_REG : string;
  attribute ASYNC_REG of sSpEnable_ms : signal is "true";
  attribute ASYNC_REG of sSpEnable    : signal is "true";
  attribute ASYNC_REG of rResetTdcFlop_ms    : signal is "true";
  attribute ASYNC_REG of rResetTdcFlop       : signal is "true";
  attribute ASYNC_REG of rResetTdcDone_ms    : signal is "true";
  attribute ASYNC_REG of rResetTdcDone       : signal is "true";
  attribute ASYNC_REG of mRunTdcEnable_ms    : signal is "true";
  attribute ASYNC_REG of mRunTdcEnable       : signal is "true";
  attribute ASYNC_REG of mResetTdc_ms        : signal is "true";
  attribute ASYNC_REG of mResetTdc           : signal is "true";
  attribute ASYNC_REG of sResetTdc_ms        : signal is "true";
  attribute ASYNC_REG of sResetTdc           : signal is "true";
  attribute ASYNC_REG of mReRunEnable_ms     : signal is "true";
  attribute ASYNC_REG of mReRunEnable        : signal is "true";

begin


  -- Generate Resets : ------------------------------------------------------------------
  -- Double-sync the reset to the MeasClk domain and then back to the RefClk domain to
  -- prove it made it all the way into the TDC. Also move it into the SampleClk domain.
  -- ------------------------------------------------------------------------------------
  GenResets : process(aReset, RefClk)
  begin
    if aReset then
      rResetTdcFlop_ms <= true;
      rResetTdcFlop    <= true;
      rResetTdcDone_ms <= true;
      rResetTdcDone    <= true;
    elsif rising_edge(RefClk) then
      -- Run this through a double-sync in case the user defaults it to false, which
      -- could cause rResetTdcFlop_ms to go meta-stable.
      rResetTdcFlop_ms <= rResetTdc;
      rResetTdcFlop    <= rResetTdcFlop_ms;
      -- Second double-sync to move the reset from the MeasClk domain back to RefClk.
      rResetTdcDone_ms <= mResetTdc;
      rResetTdcDone    <= rResetTdcDone_ms;
    end if;
  end process;

  GenResetsMeasClk : process(aReset, MeasClk)
  begin
    if aReset then
      mResetTdc_ms <= true;
      mResetTdc    <= true;
    elsif rising_edge(MeasClk) then
      -- Move the reset from the RefClk to the MeasClk domain.
      mResetTdc_ms <= rResetTdcFlop;
      mResetTdc    <= mResetTdc_ms;
    end if;
  end process;

  GenResetsSampleClk : process(aReset, SampleClk)
  begin
    if aReset then
      sResetTdc_ms <= true;
      sResetTdc    <= true;
    elsif rising_edge(SampleClk) then
      -- Move the reset from the RefClk to the SampleClk domain.
      sResetTdc_ms <= rResetTdcFlop;
      sResetTdc    <= sResetTdc_ms;
    end if;
  end process;


  -- Generate Enables for TDCs : --------------------------------------------------------
  -- When the TDC is enabled by asserting rEnableTdc, we start "listening" for a PPS
  -- rising edge to occur. We capture the first edge we see and then keep the all the
  -- enables asserted until the TDC is disabled.
  -- ------------------------------------------------------------------------------------
  rPpsPulseRe <= rPpsPulse and not rPpsPulseDly;

  EnableTdc : process(aReset, RefClk)
  begin
    if aReset then
      rPpsPulseDly <= false;
      rPpsCaptured <= '0';
      rSyncPulseEnableDly <= (others => '0');
    elsif rising_edge(RefClk) then
      -- RE detector for PPS to ONLY trigger on the edge and not accidentally half
      -- way through the high time.
      rPpsPulseDly <= rPpsPulse;
      -- When the TDC is enabled we capture the first PPS. This starts the Sync Pulses
      -- (RP / SP) as well as enables the TDC measurement for capturing edges. Note
      -- that this is independent from any synchronous reset such that we can control
      -- the PPS capture and the edge capture independently.
      if rEnableTdc then
        if rPpsPulseRe then
          rPpsCaptured <= '1';
        end if;
      else
        rPpsCaptured <= '0';
        rSyncPulseEnableDly <= (others => '0');
      end if;

      -- Delay chain for the enable bits. Shift left low to high.
      rSyncPulseEnableDly <=
        rSyncPulseEnableDly(rSyncPulseEnableDly'high-1 downto 0) & rPpsCaptured;
    end if;
  end process;

  rSyncPulseEnableDlyVal <= resize(rPulserEnableDelayVal, rSyncPulseEnableDlyVal'length);

  -- Enables for the RePulse/RP/SP. The RePulse enable must be asserted two cycles
  -- before the other enables to allow the TDC to start running before the RP/SP begin.
  rRePulseEnable <= rPpsCaptured = '1'; -- no delay
  rRpEnable <= rSyncPulseEnableDly(to_integer(rSyncPulseEnableDlyVal)) = '1';
  rSpEnable <= rSyncPulseEnableDly(to_integer(rSyncPulseEnableDlyVal)+kAddtlDelayForSpEnable-1) = '1';

  -- Local to output.
  rPpsPulseCaptured <= rPpsCaptured = '1';

  -- Sync rSpEnable to the SampleClk now... based on the "TDC 2.0" diagram.
  SyncEnableToSampleClk : process(aReset, SampleClk)
  begin
    if aReset then
      sSpEnable_ms <= false;
      sSpEnable    <= false;
    elsif rising_edge(SampleClk) then
      sSpEnable_ms <= rSpEnable;
      sSpEnable    <= sSpEnable_ms;
    end if;
  end process;

  --vhook_e Pulser ReRunPulser
  --vhook_a kClksPerPulseMaxBits kRClksPerRePulsePeriodBitsMax
  --vhook_a Clk            RefClk
  --vhook_a cLoadLimits    rLoadRePulseCounts
  --vhook_a cPeriod        rRePulsePeriodInRClks
  --vhook_a cHighTime      rRePulseHighTimeInRClks
  --vhook_a cEnablePulse   rRePulseEnable
  --vhook_a cPulse         rRePulse
  ReRunPulser: entity work.Pulser (rtl)
    generic map (kClksPerPulseMaxBits => kRClksPerRePulsePeriodBitsMax)  --integer range 3:32 :=16
    port map (
      aReset       => aReset,                   --in  boolean
      Clk          => RefClk,                   --in  std_logic
      cLoadLimits  => rLoadRePulseCounts,       --in  boolean
      cPeriod      => rRePulsePeriodInRClks,    --in  unsigned(kClksPerPulseMaxBits-1:0)
      cHighTime    => rRePulseHighTimeInRClks,  --in  unsigned(kClksPerPulseMaxBits-1:0)
      cEnablePulse => rRePulseEnable,           --in  boolean
      cPulse       => rRePulse);                --out boolean

  mRunTdcEnableRe <= mRunTdcEnable and not mRunTdcEnableDly;

  -- FSM to generate the master Run signal, as well as the repeat run.
  SyncEnableToMeasClk : process(aReset, MeasClk)
  begin
    if aReset then
      mRunTdcEnable_ms <= false;
      mRunTdcEnable    <= false;
      mReRunEnable_ms  <= false;
      mReRunEnable     <= false;
      mRunTdcEnableDly <= false;
      mRunTdc          <= false;
      mEnableState     <= Disabled;
    elsif rising_edge(MeasClk) then
      -- rRePulse is many, many MeasClk cycles high/low, so this is safe to double-sync.
      mRunTdcEnable_ms <= rRePulse;
      mRunTdcEnable    <= mRunTdcEnable_ms;
      mReRunEnable_ms  <= rReRunEnable;
      mReRunEnable     <= mReRunEnable_ms;

      mRunTdcEnableDly <= mRunTdcEnable;

      -- STATE MACHINE STARTUP !!! ------------------------------------------------------
      -- This state machine starts safely because it cannot change state until
      -- mRunTdcEnable is asserted, which cannot happen until several cycles after
      -- aReset de-assertion due to the double-synchronizer from the RefClk domain.
      -- --------------------------------------------------------------------------------
      -- De-assert strobe.
      mRunTdc <= false;

      case mEnableState is
        -- Transition to WaitForRunComplete when the TDC is enabled. Pulse mRunTdc here,
        -- and then wait for it to complete in WaitForRunComplete.
        when Disabled =>
          if mRunTdcEnableRe then
            mRunTdc <= true;
            mEnableState <= WaitForRunComplete;
          end if;

        -- The TDC measurement is complete when both offsets are valid. Go to the re-run
        -- state regardless of whether re-runs are enabled. If they aren't we just sit
        -- there and wait for more instructions...
        when WaitForRunComplete =>
          if mOffsetsValidLcl then
            mEnableState <= ReRuns;
          end if;

        -- Only pulse mRunTdc again if re-runs are enabled and the rising edge of
        -- the enable signal occurs. This guarantees our RP/SP have the correct phase
        -- relationship every time the TDC is run.
        when ReRuns =>
          if mReRunEnable and mRunTdcEnableRe then
            mRunTdc <= true;
            mEnableState <= WaitForRunComplete;
          end if;

        when others =>
          mEnableState <= Disabled;
      end case;

      -- Synchronous reset for FSM.
      if mResetTdc then
        mEnableState <= Disabled;
        mRunTdc <= false;
      end if;

    end if;
  end process;



  -- Generate Output Valid Signals : ----------------------------------------------------
  -- Depending on how fast SW can read the measurements (and in what order they read)
  -- the readings could be out of sync with one another. This section conditions the
  -- output valid signals from each core and asserts a single output valid pulse after
  -- BOTH valids have asserted. It is agnostic to the order in which the valids assert.
  -- It creates a delay in the output valid assertion. Minimal delay is one MeasClk cycle
  -- if the core valids assert together. Worst-case delay is two MeasClk cycles after
  -- the latter of the two valids asserts. This is acceptable delay because the core
  -- cannot be re-run until both valids have asserted (mOffsetsValidLcl is fed back into
  -- the ReRun FSM above).
  -- ------------------------------------------------------------------------------------
  ConditionDataValidProc : process(aReset, MeasClk) is
  begin
    if aReset then
      mOffsetsValidLcl <= false;
      mRpValidStored   <= false;
      mSpValidStored   <= false;
    elsif rising_edge(MeasClk) then
      -- Reset the strobe signals.
      mOffsetsValidLcl <= false;

      -- First, we're sensitive to the TDC sync reset signal.
      if mResetTdc then
        mOffsetsValidLcl <= false;
        mRpValidStored   <= false;
        mSpValidStored   <= false;
      -- Case 1: Both Valid signals pulse at the same time.
      -- Case 4: Both Valid signals have been stored independently. Yes, this incurs
      --         a one-cycle delay in the output valid (from when the second one asserts)
      --         but it makes for cleaner code and is safe because by design because the
      --         valid signals cannot assert again for a longggg time.
      elsif (mRpOffsetValidLcl and mSpOffsetValidLcl) or
            (mRpValidStored    and mSpValidStored) then
        mOffsetsValidLcl <= true;
        mRpValidStored   <= false;
        mSpValidStored   <= false;
      -- Case 2: RP Valid pulses alone.
      elsif mRpOffsetValidLcl then
        mRpValidStored <= true;
      -- Case 3: SP Valid pulses alone.
      elsif mSpOffsetValidLcl then
        mSpValidStored <= true;
      end if;
    end if;
  end process;

  -- Local to output.
  mOffsetsValid <= mOffsetsValidLcl;
  -- Only assert done with both cores are done.
  mOffsetsDone  <= mRpOffsetDoneLcl and mSpOffsetDoneLcl;



  -- Reference Clock TDC (RP) : ---------------------------------------------------------
  -- mRP is only used for testbenching purposes, so ignore vhook warnings.
  --vhook_nowarn mRP
  -- ------------------------------------------------------------------------------------

  --vhook   TdcCore    RpTdc
  --vhook_g kSourceClksPerPulseMaxBits kRClksPerRpPeriodBitsMax
  --vhook_a mResetPeriodMeas mResetTdc
  --vhook_a mResetTdcMeas    mResetTdc
  --vhook_a mPeriodMeasDone  open
  --vhook_a mRunTdcMeas      mRunTdc
  --vhook_a mGatedPulse      mRP
  --vhook_a mAvgOffset       mRpOffset
  --vhook_a mAvgOffsetDone   mRpOffsetDoneLcl
  --vhook_a mAvgOffsetValid  mRpOffsetValidLcl
  --vhook_a SourceClk        RefClk
  --vhook_a sResetTdc        rResetTdcFlop
  --vhook_a sSyncPulseLoadCnt  rLoadRpCounts
  --vhook_a sSyncPulsePeriod   rRpPeriodInRClks
  --vhook_a sSyncPulseHighTime rRpHighTimeInRClks
  --vhook_a sSyncPulseEnable   rRpEnable
  --vhook_a sGatedPulse        open
  --vhook_a {^sGated(.*)}      rGated$1
  RpTdc: TdcCore
    generic map (
      kSourceClksPerPulseMaxBits => kRClksPerRpPeriodBitsMax,    --integer range 3:16 :=16
      kPulsePeriodCntSize        => kPulsePeriodCntSize,         --integer:=13
      kFreqRefPeriodsToCheckSize => kFreqRefPeriodsToCheckSize,  --integer:=17
      kSyncPeriodsToStampSize    => kSyncPeriodsToStampSize)     --integer:=10
    port map (
      aReset             => aReset,              --in  boolean
      MeasClk            => MeasClk,             --in  std_logic
      mResetPeriodMeas   => mResetTdc,           --in  boolean
      mPeriodMeasDone    => open,                --out boolean
      mResetTdcMeas      => mResetTdc,           --in  boolean
      mRunTdcMeas        => mRunTdc,             --in  boolean
      mGatedPulse        => mRP,                 --out boolean
      mAvgOffset         => mRpOffset,           --out unsigned(kPulsePeriodCntSize+ kSyncPeriodsToStampSize+ kFreqRefPeriodsToCheckSize-1:0)
      mAvgOffsetDone     => mRpOffsetDoneLcl,    --out boolean
      mAvgOffsetValid    => mRpOffsetValidLcl,   --out boolean
      SourceClk          => RefClk,              --in  std_logic
      sResetTdc          => rResetTdcFlop,       --in  boolean
      sSyncPulseLoadCnt  => rLoadRpCounts,       --in  boolean
      sSyncPulsePeriod   => rRpPeriodInRClks,    --in  unsigned(kSourceClksPerPulseMaxBits-1:0)
      sSyncPulseHighTime => rRpHighTimeInRClks,  --in  unsigned(kSourceClksPerPulseMaxBits-1:0)
      sSyncPulseEnable   => rRpEnable,           --in  boolean
      sGatedPulse        => open,                --out boolean
      sGatedPulseToPin   => rGatedPulseToPin);   --inout std_logic

  --vhook_e Pulser RpTransferPulse
  --vhook_a kClksPerPulseMaxBits kRClksPerRpPeriodBitsMax
  --vhook_a Clk            RefClk
  --vhook_a cLoadLimits    rLoadRptCounts
  --vhook_a cPeriod        rRptPeriodInRClks
  --vhook_a cHighTime      rRptHighTimeInRClks
  --vhook_a cEnablePulse   rRpEnable
  --vhook_a cPulse         rRptPulse
  RpTransferPulse: entity work.Pulser (rtl)
    generic map (kClksPerPulseMaxBits => kRClksPerRpPeriodBitsMax)  --integer range 3:32 :=16
    port map (
      aReset       => aReset,               --in  boolean
      Clk          => RefClk,               --in  std_logic
      cLoadLimits  => rLoadRptCounts,       --in  boolean
      cPeriod      => rRptPeriodInRClks,    --in  unsigned(kClksPerPulseMaxBits-1:0)
      cHighTime    => rRptHighTimeInRClks,  --in  unsigned(kClksPerPulseMaxBits-1:0)
      cEnablePulse => rRpEnable,            --in  boolean
      cPulse       => rRptPulse);           --out boolean

  -- Local to output
  rRpTransfer <= rRptPulse;


  -- Sample Clock TDC (SP) : ------------------------------------------------------------
  -- mSP is only used for testbenching purposes, so ignore vhook warnings.
  --vhook_nowarn mSP
  -- ------------------------------------------------------------------------------------

  --vhook   TdcCore    SpTdc
  --vhook_g kSourceClksPerPulseMaxBits kSClksPerSpPeriodBitsMax
  --vhook_a mResetPeriodMeas mResetTdc
  --vhook_a mResetTdcMeas    mResetTdc
  --vhook_a mPeriodMeasDone  open
  --vhook_a mRunTdcMeas      mRunTdc
  --vhook_a mGatedPulse      mSP
  --vhook_a mAvgOffset       mSpOffset
  --vhook_a mAvgOffsetDone   mSpOffsetDoneLcl
  --vhook_a mAvgOffsetValid  mSpOffsetValidLcl
  --vhook_a SourceClk        SampleClk
  --vhook_a sResetTdc        sResetTdc
  --vhook_a sSyncPulseLoadCnt  sLoadSpCounts
  --vhook_a sSyncPulsePeriod   sSpPeriodInSClks
  --vhook_a sSyncPulseHighTime sSpHighTimeInSClks
  --vhook_a sSyncPulseEnable sSpEnable
  --vhook_a sGatedPulse      open
  --vhook_a {^sGated(.*)}    sGated$1
  SpTdc: TdcCore
    generic map (
      kSourceClksPerPulseMaxBits => kSClksPerSpPeriodBitsMax,    --integer range 3:16 :=16
      kPulsePeriodCntSize        => kPulsePeriodCntSize,         --integer:=13
      kFreqRefPeriodsToCheckSize => kFreqRefPeriodsToCheckSize,  --integer:=17
      kSyncPeriodsToStampSize    => kSyncPeriodsToStampSize)     --integer:=10
    port map (
      aReset             => aReset,              --in  boolean
      MeasClk            => MeasClk,             --in  std_logic
      mResetPeriodMeas   => mResetTdc,           --in  boolean
      mPeriodMeasDone    => open,                --out boolean
      mResetTdcMeas      => mResetTdc,           --in  boolean
      mRunTdcMeas        => mRunTdc,             --in  boolean
      mGatedPulse        => mSP,                 --out boolean
      mAvgOffset         => mSpOffset,           --out unsigned(kPulsePeriodCntSize+ kSyncPeriodsToStampSize+ kFreqRefPeriodsToCheckSize-1:0)
      mAvgOffsetDone     => mSpOffsetDoneLcl,    --out boolean
      mAvgOffsetValid    => mSpOffsetValidLcl,   --out boolean
      SourceClk          => SampleClk,           --in  std_logic
      sResetTdc          => sResetTdc,           --in  boolean
      sSyncPulseLoadCnt  => sLoadSpCounts,       --in  boolean
      sSyncPulsePeriod   => sSpPeriodInSClks,    --in  unsigned(kSourceClksPerPulseMaxBits-1:0)
      sSyncPulseHighTime => sSpHighTimeInSClks,  --in  unsigned(kSourceClksPerPulseMaxBits-1:0)
      sSyncPulseEnable   => sSpEnable,           --in  boolean
      sGatedPulse        => open,                --out boolean
      sGatedPulseToPin   => sGatedPulseToPin);   --inout std_logic

  --vhook_e Pulser SpTransferPulse
  --vhook_a kClksPerPulseMaxBits kSClksPerSpPeriodBitsMax
  --vhook_a Clk            SampleClk
  --vhook_a cLoadLimits    sLoadSptCounts
  --vhook_a cPeriod        sSptPeriodInSClks
  --vhook_a cHighTime      sSptHighTimeInSClks
  --vhook_a cEnablePulse   sSpEnable
  --vhook_a cPulse         sSptPulse
  SpTransferPulse: entity work.Pulser (rtl)
    generic map (kClksPerPulseMaxBits => kSClksPerSpPeriodBitsMax)  --integer range 3:32 :=16
    port map (
      aReset       => aReset,               --in  boolean
      Clk          => SampleClk,            --in  std_logic
      cLoadLimits  => sLoadSptCounts,       --in  boolean
      cPeriod      => sSptPeriodInSClks,    --in  unsigned(kClksPerPulseMaxBits-1:0)
      cHighTime    => sSptHighTimeInSClks,  --in  unsigned(kClksPerPulseMaxBits-1:0)
      cEnablePulse => sSpEnable,            --in  boolean
      cPulse       => sSptPulse);           --out boolean

  -- Local to output
  sSpTransfer <= sSptPulse;


  -- Cross PPS to SampleClk : ----------------------------------------------------------
  -- Cross it safely and with deterministic delay.
  -- ------------------------------------------------------------------------------------

  -- Keep the module from over-pulsing itself by gating the input with the RFI signal,
  -- although at 1 Hz, this module should never run into the RFI de-asserted case
  -- by design.
  rGatedCptrPulseIn <= rCrossTrigRFI and rPpsPulseRe;

  --vhook_e CrossTrigger CrossCptrPulse
  --vhook_a rRP               rRptPulse
  --vhook_a rReadyForInput    rCrossTrigRFI
  --vhook_a rEnableTrigger    rEnablePpsCrossing
  --vhook_a rTriggerIn        rGatedCptrPulseIn
  --vhook_a sSP               sSptPulse
  --vhook_a sElasticBufferPtr sPpsClkCrossDelayVal
  --vhook_a sTriggerOut       sPpsPulse
  CrossCptrPulse: entity work.CrossTrigger (rtl)
    port map (
      aReset            => aReset,                --in  boolean
      RefClk            => RefClk,                --in  std_logic
      rRP               => rRptPulse,             --in  boolean
      rReadyForInput    => rCrossTrigRFI,         --out boolean
      rEnableTrigger    => rEnablePpsCrossing,    --in  boolean
      rTriggerIn        => rGatedCptrPulseIn,     --in  boolean
      SampleClk         => SampleClk,             --in  std_logic
      sSP               => sSptPulse,             --in  boolean
      sElasticBufferPtr => sPpsClkCrossDelayVal,  --in  unsigned(3:0)
      sTriggerOut       => sPpsPulse);            --out boolean


end struct;







--------------------------------------------------------------------------------
-- Testbench for TdcTop
--------------------------------------------------------------------------------

--synopsys translate_off
library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;
  use ieee.math_real.all;

entity tb_TdcTop is end tb_TdcTop;

architecture test of tb_TdcTop is

  -- Constants for the clock periods.
  constant kSPer : time :=   8.000 ns; -- 125.00 MHz
  constant kMPer : time :=   5.050 ns; -- 198.00 MHz
  constant kRPer : time := 100.000 ns; --  10.00 MHz

  constant kRClksPerRePulsePeriodBitsMax : integer := 24;
  constant kRClksPerRpPeriodBitsMax      : integer := 16;
  constant kSClksPerSpPeriodBitsMax      : integer := 16;

  -- Constants for the RP/SP pulses, based on the clock frequencies above. The periods
  -- should all divide into one another without remainders, so this is safe to do...
  -- High time is 50% duty cycle, or close to it if the period isn't a round number.
  constant kRpPeriod           : time    :=  1000 ns;
  constant kRpPeriodInRClks    : integer := kRpPeriod/kRPer;
  constant kRpHighTimeInRClks  : integer := integer(floor(real(kRpPeriodInRClks)/2.0));
  constant kRptPeriod          : time    := 25000 ns;
  constant kRptPeriodInRClks   : integer := kRptPeriod/kRPer;
  constant kRptHighTimeInRClks : integer := integer(floor(real(kRptPeriodInRClks)/2.0));
  constant kSpPeriod           : time    :=   800 ns;
  constant kSpPeriodInSClks    : integer := kSpPeriod/kSPer;
  constant kSpHighTimeInSClks  : integer := integer(floor(real(kSpPeriodInSClks)/2.0));
  constant kSptPeriod          : time    := 25000 ns;
  constant kSptPeriodInSClks   : integer := kSptPeriod/kSPer;
  constant kSptHighTimeInSClks : integer := integer(floor(real(kSptPeriodInSClks)/2.0));
  constant kRePulsePeriod      : time    := 2.500 ms;
  constant kRePulsePeriodInRClks   : integer := kRePulsePeriod/kRPer;
  constant kRePulseHighTimeInRClks : integer := integer(floor(real(kRePulsePeriodInRClks)/2.0));

  -- This doesn't come out to a nice number (or shouldn't), but that's ok. Round up.
  constant kMeasClksPerRp : integer := kRpPeriod/kMPer+1;

  -- Inputs to DUT
  constant kPulsePeriodCntSize       : integer := integer(ceil(log2(real(kMeasClksPerRp))));
  constant kFreqRefPeriodsToCheckSize: integer := 12; -- usually 17, but to save run time...
  constant kSyncPeriodsToStampSize   : integer := 10;

  constant kMeasurementTimeout : time :=
             kMPer*(kMeasClksPerRp*(2**kSyncPeriodsToStampSize) +
                    40*(2**kSyncPeriodsToStampSize) +
                    kMeasClksPerRp*(2**kFreqRefPeriodsToCheckSize)
                   );

  --vhook_sigstart
  signal aReset: boolean;
  signal MeasClk: std_logic := '0';
  signal mOffsetsDone: boolean;
  signal mOffsetsValid: boolean;
  signal mRpOffset: unsigned(kPulsePeriodCntSize+kSyncPeriodsToStampSize+kFreqRefPeriodsToCheckSize-1 downto 0);
  signal mSpOffset: unsigned(kPulsePeriodCntSize+kSyncPeriodsToStampSize+kFreqRefPeriodsToCheckSize-1 downto 0);
  signal RefClk: std_logic := '0';
  signal rEnablePpsCrossing: boolean;
  signal rEnableTdc: boolean;
  signal rGatedPulseToPin: std_logic;
  signal rLoadRePulseCounts: boolean;
  signal rLoadRpCounts: boolean;
  signal rLoadRptCounts: boolean;
  signal rPpsPulse: boolean;
  signal rPpsPulseCaptured: boolean;
  signal rPulserEnableDelayVal: unsigned(3 downto 0);
  signal rReRunEnable: boolean;
  signal rResetTdc: boolean;
  signal rResetTdcDone: boolean;
  signal rRpTransfer: boolean;
  signal SampleClk: std_logic := '0';
  signal sGatedPulseToPin: std_logic;
  signal sLoadSpCounts: boolean;
  signal sLoadSptCounts: boolean;
  signal sPpsClkCrossDelayVal: unsigned(3 downto 0);
  signal sPpsPulse: boolean;
  signal sSpTransfer: boolean;
  --vhook_sigend

  signal StopSim : boolean;
  signal EnableOutputChecks : boolean := true;

  signal ExpectedRpOutput,
         ExpectedFinalMeas,
         ExpectedSpOutput : real := 0.0;

  alias mRunTdc is <<signal .tb_TdcTop.dutx.mRunTdc : boolean>>;
  alias mSP is <<signal .tb_TdcTop.dutx.mSP : boolean>>;
  alias mRP is <<signal .tb_TdcTop.dutx.mRP : boolean>>;

  procedure ClkWait(
    signal   Clk   : in std_logic;
    X : positive := 1) is
  begin
    for i in 1 to X loop
      wait until rising_edge(Clk);
    end loop;
  end procedure ClkWait;

  function OffsetToReal (Offset : unsigned) return real is
    variable TempVar : real := 0.0;
  begin
    TempVar :=
      real(to_integer(
        Offset(Offset'high downto kFreqRefPeriodsToCheckSize+kSyncPeriodsToStampSize))) +
      real(to_integer(
        Offset(kFreqRefPeriodsToCheckSize+kSyncPeriodsToStampSize-1 downto 0)))*
      real(2.0**(-(kFreqRefPeriodsToCheckSize+kSyncPeriodsToStampSize)));
    return TempVar;
  end OffsetToReal;

begin

  SampleClk   <= not SampleClk   after kSPer/2 when not StopSim else '0';
  RefClk      <= not RefClk      after kRPer/2 when not StopSim else '0';
  MeasClk     <= not MeasClk     after kMPer/2 when not StopSim else '0';


  main: process
  begin
    -- Defaults, per instructions in Purpose
    sPpsClkCrossDelayVal  <= to_unsigned(0, sPpsClkCrossDelayVal'length);
    rPulserEnableDelayVal <= to_unsigned(1, rPulserEnableDelayVal'length);
    rResetTdc    <= true;
    rEnableTdc   <= false;
    rReRunEnable <= false;
    rEnablePpsCrossing <= false;
    rPpsPulse  <= false;
    rLoadRePulseCounts <= false;
    rLoadRpCounts  <= false;
    rLoadRptCounts <= false;
    sLoadSpCounts  <= false;
    sLoadSptCounts <= false;

    aReset <= true, false after kRPer*4;
    ClkWait(RefClk,10);

    -- Step 0 : -------------------------------------------------------------------------
    -- Prior to de-asserting reset, we need to load the counters, so pulse the loads.
    ClkWait(RefClk);
    rLoadRePulseCounts <= true;
    rLoadRpCounts  <= true;
    rLoadRptCounts <= true;
    ClkWait(RefClk);
    rLoadRePulseCounts <= false;
    rLoadRpCounts  <= false;
    rLoadRptCounts <= false;
    ClkWait(SampleClk);
    sLoadSpCounts  <= true;
    sLoadSptCounts <= true;
    ClkWait(SampleClk);
    sLoadSpCounts  <= false;
    sLoadSptCounts <= false;


    -- Step 1 : -------------------------------------------------------------------------
    report "De-asserting Synchronous Reset..." severity note;
    ClkWait(RefClk);
    rResetTdc <= false;
    wait until not rResetTdcDone for (kRPer*4)+(kMPer*2);
    assert not rResetTdcDone
      report "rRestTdcDone didn't de-assert in time"
      severity error;


    -- Step 2 : -------------------------------------------------------------------------
    report "Enabling TDC Measurement & Capturing PPS..." severity note;
    rEnableTdc <= true;
    ClkWait(RefClk,5);

    -- Trigger a PPS one-cycle pulse.
    rPpsPulse <= true;
    ClkWait(RefClk);
    rPpsPulse <= false;
    ClkWait(RefClk);
    assert rPpsPulseCaptured report "PPS not captured" severity error;


    -- Step 3 : -------------------------------------------------------------------------
    report "Waiting for Measurements to Complete..." severity note;
    wait until mOffsetsDone for kMeasurementTimeout;
    assert mOffsetsDone
      report "Offset measurements not completed within timeout"
      severity error;

    -- Offset values checked below in CheckOutput.

    report "Printing Results..." & LF &
       "RP:   " & real'image(OffsetToReal(mRpOffset)) &
       " Expected: " & real'image(ExpectedRpOutput) & LF &
       "SP:  " & real'image(OffsetToReal(mSpOffset)) &
       " Expected: " & real'image(ExpectedSpOutput) & LF &
       "Meas: " & real'image((OffsetToReal(mSpOffset-mRpOffset)*real(kMPer/1 ns)+
                              real(kRPer/1 ns)-real(kSPer/1 ns))/real(kSPer/1 ns)) &
       " Expected: " & real'image(ExpectedFinalMeas)
      severity note;


    -- Step 4 : -------------------------------------------------------------------------
    -- Trigger another PPS one-cycle pulse to watch it all cross over correctly.
    -- Issue the trigger around where a real PPS pulse will come (RE of RP).
    -- First, set the programmable delay sPpsClkCrossDelayVal.
    ClkWait(SampleClk);
    sPpsClkCrossDelayVal <= to_unsigned(4, sPpsClkCrossDelayVal'length);
    ClkWait(RefClk);
    rEnablePpsCrossing   <= true;
    wait until rRpTransfer and not rRpTransfer'delayed;
    rPpsPulse <= true;
    ClkWait(RefClk);
    rPpsPulse <= false;
    ClkWait(RefClk);

    -- We expect the PPS output pulse to arrive after FE and RE of sSP have passed,
    -- and then a few extra cycles of SampleClk delay on there as well.
    wait until (not sSpTransfer) and (    sSpTransfer'delayed); -- FE
    wait until (    sSpTransfer) and (not sSpTransfer'delayed); -- RE
    ClkWait(SampleClk, 2 + to_integer(sPpsClkCrossDelayVal));
    -- Check on falling edge of clock.
    wait until falling_edge(SampleClk);
    assert sPpsPulse and not sPpsPulse'delayed(kSPer) report "sPpsPulse did not assert";
    wait until falling_edge(SampleClk);
    assert not sPpsPulse report "sPpsPulse did not pulse correctly";


    -- Step 5 : -------------------------------------------------------------------------
    report "Repeating TDC Measurement..." severity note;
    ClkWait(RefClk);
    rReRunEnable <= true;

    -- Now wait for the measurement to complete.
    wait until mOffsetsValid for kMeasurementTimeout;
    assert mOffsetsValid
      report "Offset measurements not re-completed within timeout"
      severity error;

    -- Offset values checked below in CheckOutput.

    report "Printing Results..." & LF &
       "RP:   " & real'image(OffsetToReal(mRpOffset)) &
       " Expected: " & real'image(ExpectedRpOutput) & LF &
       "SP:   " & real'image(OffsetToReal(mSpOffset)) &
       " Expected: " & real'image(ExpectedSpOutput) & LF &
       "Meas: " & real'image((OffsetToReal(mSpOffset-mRpOffset)*real(kMPer/1 ns)+
                              real(kRPer/1 ns)-real(kSPer/1 ns))/real(kSPer/1 ns)) &
       " Expected: " & real'image(ExpectedFinalMeas)
      severity note;

    ClkWait(MeasClk,100);


    -- Let it run for a while : ---------------------------------------------------------
    for i in 0 to 9 loop
      wait until mOffsetsValid for kMeasurementTimeout;
      assert mOffsetsValid
        report "Offset measurements not re-completed within timeout"
        severity error;
      report "Printing Results..." & LF &
         "RP:   " & real'image(OffsetToReal(mRpOffset)) &
         " Expected: " & real'image(ExpectedRpOutput) & LF &
         "SP:   " & real'image(OffsetToReal(mSpOffset)) &
         " Expected: " & real'image(ExpectedSpOutput) & LF &
         "Meas: " & real'image((OffsetToReal(mSpOffset-mRpOffset)*real(kMPer/1 ns)+
                                real(kRPer/1 ns)-real(kSPer/1 ns))/real(kSPer/1 ns)) &
         " Expected: " & real'image(ExpectedFinalMeas)
        severity note;
    end loop;


    -- And stop it : --------------------------------------------------------------------
    report "Stopping Repeating TDC Measurements..." severity note;
    ClkWait(RefClk);
    rReRunEnable <= false;
    -- Wait to make sure it doesn't keep going.
    wait until mOffsetsValid
         for 2*(kMPer*(kMeasClksPerRp*(2**kSyncPeriodsToStampSize) + 40*(2**kSyncPeriodsToStampSize)));
    assert not mOffsetsValid;



    -- Let it run for a while : ---------------------------------------------------------
    report "Starting again Repeating TDC Measurements..." severity note;
    ClkWait(RefClk);
    rReRunEnable <= true;
    for i in 0 to 2 loop
      wait until mOffsetsValid for kMeasurementTimeout;
      assert mOffsetsValid
        report "Offset measurements not re-completed within timeout"
        severity error;
      report "Printing Results..." & LF &
         "RP:   " & real'image(OffsetToReal(mRpOffset)) &
         " Expected: " & real'image(ExpectedRpOutput) & LF &
         "SP:   " & real'image(OffsetToReal(mSpOffset)) &
         " Expected: " & real'image(ExpectedSpOutput) & LF &
         "Meas: " & real'image((OffsetToReal(mSpOffset-mRpOffset)*real(kMPer/1 ns)+
                                real(kRPer/1 ns)-real(kSPer/1 ns))/real(kSPer/1 ns)) &
         " Expected: " & real'image(ExpectedFinalMeas)
        severity note;
    end loop;


    StopSim <= true;
    wait;
  end process;


  ExpectedFinalMeasGen : process
    variable StartTime : time := 0 ns;
  begin
    wait until rPpsPulse;
    wait until rRpTransfer;
    StartTime := now;
    wait until sSpTransfer;
    ExpectedFinalMeas <= real((now - StartTime)/1 ps)/real((kSPer/1 ps));
    wait until rResetTdc;
  end process;


  ExpectedRpOutputGen : process
    variable StartTime : time := 0 ns;
  begin
    wait until mRunTdc;
    StartTime := now;
    wait until mRP;
    ExpectedRpOutput <= real((now - StartTime)/1 ps)/real((kMPer/1 ps));
    wait until mOffsetsValid;
  end process;

  ExpectedSpOutputGen : process
    variable StartTime : time := 0 ns;
  begin
    wait until mRunTdc;
    StartTime := now;
    wait until mSP;
    ExpectedSpOutput <= real((now - StartTime)/1 ps)/real((kMPer/1 ps));
    wait until mOffsetsValid;
  end process;

  CheckOutput : process(MeasClk)
  begin
    if falling_edge(MeasClk) then
      if EnableOutputChecks then

        if mOffsetsValid then
          assert (OffsetToReal(mRpOffset) < ExpectedRpOutput + 1.0) and
                 (OffsetToReal(mRpOffset) > ExpectedRpOutput - 1.0)
            report "Mismatch between mRpOffset and expected!" & LF &
               "Actual: " & real'image(OffsetToReal(mRpOffset)) & LF &
               "Expect: " & real'image(ExpectedRpOutput)
            severity error;
          assert (OffsetToReal(mSpOffset) < ExpectedSpOutput + 1.0) and
                 (OffsetToReal(mSpOffset) > ExpectedSpOutput - 1.0)
            report "Mismatch between mSpOffset and expected!" & LF &
               "Actual: " & real'image(OffsetToReal(mSpOffset)) & LF &
               "Expect: " & real'image(ExpectedSpOutput)
            severity error;
        end if;
      end if;
    end if;
  end process;


  --vhook_e TdcTop dutx
  --vhook_a rRpPeriodInRClks    to_unsigned(kRpPeriodInRClks,   kRClksPerRpPeriodBitsMax)
  --vhook_a rRpHighTimeInRClks  to_unsigned(kRpHighTimeInRClks, kRClksPerRpPeriodBitsMax)
  --vhook_a sSpPeriodInSClks    to_unsigned(kSpPeriodInSClks,   kSClksPerSpPeriodBitsMax)
  --vhook_a sSpHighTimeInSClks  to_unsigned(kSpHighTimeInSClks, kSClksPerSpPeriodBitsMax)
  --vhook_a rRptPeriodInRClks   to_unsigned(kRptPeriodInRClks,   kRClksPerRpPeriodBitsMax)
  --vhook_a rRptHighTimeInRClks to_unsigned(kRptHighTimeInRClks, kRClksPerRpPeriodBitsMax)
  --vhook_a sSptPeriodInSClks   to_unsigned(kSptPeriodInSClks,   kSClksPerSpPeriodBitsMax)
  --vhook_a sSptHighTimeInSClks to_unsigned(kSptHighTimeInSClks, kSClksPerSpPeriodBitsMax)
  --vhook_a rRePulsePeriodInRClks   to_unsigned(kRePulsePeriodInRClks,   kRClksPerRePulsePeriodBitsMax)
  --vhook_a rRePulseHighTimeInRClks to_unsigned(kRePulseHighTimeInRClks, kRClksPerRePulsePeriodBitsMax)
  dutx: entity work.TdcTop (struct)
    generic map (
      kRClksPerRePulsePeriodBitsMax => kRClksPerRePulsePeriodBitsMax,  --integer range 3:32 :=24
      kRClksPerRpPeriodBitsMax      => kRClksPerRpPeriodBitsMax,       --integer range 3:16 :=16
      kSClksPerSpPeriodBitsMax      => kSClksPerSpPeriodBitsMax,       --integer range 3:16 :=16
      kPulsePeriodCntSize           => kPulsePeriodCntSize,            --integer:=13
      kFreqRefPeriodsToCheckSize    => kFreqRefPeriodsToCheckSize,     --integer:=17
      kSyncPeriodsToStampSize       => kSyncPeriodsToStampSize)        --integer:=10
    port map (
      aReset                  => aReset,                                                               --in  boolean
      RefClk                  => RefClk,                                                               --in  std_logic
      SampleClk               => SampleClk,                                                            --in  std_logic
      MeasClk                 => MeasClk,                                                              --in  std_logic
      rResetTdc               => rResetTdc,                                                            --in  boolean
      rResetTdcDone           => rResetTdcDone,                                                        --out boolean
      rEnableTdc              => rEnableTdc,                                                           --in  boolean
      rReRunEnable            => rReRunEnable,                                                         --in  boolean
      rPpsPulse               => rPpsPulse,                                                            --in  boolean
      rPpsPulseCaptured       => rPpsPulseCaptured,                                                    --out boolean
      rPulserEnableDelayVal   => rPulserEnableDelayVal,                                                --in  unsigned(3:0)
      rEnablePpsCrossing      => rEnablePpsCrossing,                                                   --in  boolean
      sPpsClkCrossDelayVal    => sPpsClkCrossDelayVal,                                                 --in  unsigned(3:0)
      sPpsPulse               => sPpsPulse,                                                            --out boolean
      mRpOffset               => mRpOffset,                                                            --out unsigned(kPulsePeriodCntSize+ kSyncPeriodsToStampSize+ kFreqRefPeriodsToCheckSize-1:0)
      mSpOffset               => mSpOffset,                                                            --out unsigned(kPulsePeriodCntSize+ kSyncPeriodsToStampSize+ kFreqRefPeriodsToCheckSize-1:0)
      mOffsetsDone            => mOffsetsDone,                                                         --out boolean
      mOffsetsValid           => mOffsetsValid,                                                        --out boolean
      rLoadRePulseCounts      => rLoadRePulseCounts,                                                   --in  boolean
      rRePulsePeriodInRClks   => to_unsigned(kRePulsePeriodInRClks, kRClksPerRePulsePeriodBitsMax),    --in  unsigned(kRClksPerRePulsePeriodBitsMax-1:0)
      rRePulseHighTimeInRClks => to_unsigned(kRePulseHighTimeInRClks, kRClksPerRePulsePeriodBitsMax),  --in  unsigned(kRClksPerRePulsePeriodBitsMax-1:0)
      rLoadRpCounts           => rLoadRpCounts,                                                        --in  boolean
      rRpPeriodInRClks        => to_unsigned(kRpPeriodInRClks, kRClksPerRpPeriodBitsMax),              --in  unsigned(kRClksPerRpPeriodBitsMax-1:0)
      rRpHighTimeInRClks      => to_unsigned(kRpHighTimeInRClks, kRClksPerRpPeriodBitsMax),            --in  unsigned(kRClksPerRpPeriodBitsMax-1:0)
      rLoadRptCounts          => rLoadRptCounts,                                                       --in  boolean
      rRptPeriodInRClks       => to_unsigned(kRptPeriodInRClks, kRClksPerRpPeriodBitsMax),             --in  unsigned(kRClksPerRpPeriodBitsMax-1:0)
      rRptHighTimeInRClks     => to_unsigned(kRptHighTimeInRClks, kRClksPerRpPeriodBitsMax),           --in  unsigned(kRClksPerRpPeriodBitsMax-1:0)
      sLoadSpCounts           => sLoadSpCounts,                                                        --in  boolean
      sSpPeriodInSClks        => to_unsigned(kSpPeriodInSClks, kSClksPerSpPeriodBitsMax),              --in  unsigned(kSClksPerSpPeriodBitsMax-1:0)
      sSpHighTimeInSClks      => to_unsigned(kSpHighTimeInSClks, kSClksPerSpPeriodBitsMax),            --in  unsigned(kSClksPerSpPeriodBitsMax-1:0)
      sLoadSptCounts          => sLoadSptCounts,                                                       --in  boolean
      sSptPeriodInSClks       => to_unsigned(kSptPeriodInSClks, kSClksPerSpPeriodBitsMax),             --in  unsigned(kSClksPerSpPeriodBitsMax-1:0)
      sSptHighTimeInSClks     => to_unsigned(kSptHighTimeInSClks, kSClksPerSpPeriodBitsMax),           --in  unsigned(kSClksPerSpPeriodBitsMax-1:0)
      rRpTransfer             => rRpTransfer,                                                          --out boolean
      sSpTransfer             => sSpTransfer,                                                          --out boolean
      rGatedPulseToPin        => rGatedPulseToPin,                                                     --inout std_logic
      sGatedPulseToPin        => sGatedPulseToPin);                                                    --inout std_logic


end test;
--synopsys translate_on
