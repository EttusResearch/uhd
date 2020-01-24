-------------------------------------------------------------------------------
--
-- Copyright 2018 Ettus Research, a National Instruments Company
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
--
-- Purpose:
--
-- Wrapper for the TDC and register control modules.
--
-- vreview_group Tdc
-- vreview_reviewers dabaker sgupta jmarsar
-------------------------------------------------------------------------------

library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;

library work;
  use work.PkgRegs.all;

entity TdcWrapper is
  port (
    -- Clocks and Resets : --------------------------------------------------------------
    -- Bus Clock and synchronous bus reset.
    BusClk          : in  std_logic;
    bBusReset       : in  std_logic;
    -- Reference Clock
    RefClk          : in  std_logic;
    -- Sample Clock
    SampleClk       : in  std_logic;
    -- Measurement Clock must run at a very specific frequency, determined by the
    -- SampleClk, RefClk, and Sync Pulse rates... oh and a lot of math.
    MeasClk         : in  std_logic;


    -- Register Port: -------------------------------------------------------------------
    bSyncRegPortOut : out RegPortOut_t;
    bSyncRegPortIn  : in  RegPortIn_t;


    -- PPS In and Out : -----------------------------------------------------------------
    -- Only required to pulse 1 RefClk cycle.
    rPpsPulse       : in  std_logic;
    -- PPS pulse output on the SampleClk domain.
    sPpsPulse       : out std_logic;


    -- Sync Pulse Outputs : -------------------------------------------------------------
    -- The repeating pulses can be useful for many things, including passing triggers.
    rRpTransfer : out std_logic;
    sSpTransfer : out std_logic;

    -- Pin bouncers out and in. Must go to unused and unconnected pins on the FPGA!
    rGatedPulseToPin : inout std_logic;
    sGatedPulseToPin : inout std_logic
  );
end TdcWrapper;


architecture struct of TdcWrapper is

  component SyncRegsIfc
    port (
      aBusReset               : in  std_logic;
      bBusReset               : in  std_logic;
      BusClk                  : in  std_logic;
      aTdcReset               : out std_logic;
      bRegPortInFlat          : in  std_logic_vector(49 downto 0);
      bRegPortOutFlat         : out std_logic_vector(33 downto 0);
      RefClk                  : in  std_logic;
      rResetTdc               : out std_logic;
      rResetTdcDone           : in  std_logic;
      rEnableTdc              : out std_logic;
      rReRunEnable            : out std_logic;
      rEnablePpsCrossing      : out std_logic;
      rPpsPulseCaptured       : in  std_logic;
      rPulserEnableDelayVal   : out std_logic_vector(3 downto 0);
      SampleClk               : in  std_logic;
      sPpsClkCrossDelayVal    : out std_logic_vector(3 downto 0);
      MeasClk                 : in  std_logic;
      mRpOffset               : in  std_logic_vector(39 downto 0);
      mSpOffset               : in  std_logic_vector(39 downto 0);
      mOffsetsDone            : in  std_logic;
      mOffsetsValid           : in  std_logic;
      rLoadRePulseCounts      : out std_logic;
      rRePulsePeriodInRClks   : out std_logic_vector(23 downto 0);
      rRePulseHighTimeInRClks : out std_logic_vector(23 downto 0);
      rLoadRpCounts           : out std_logic;
      rRpPeriodInRClks        : out std_logic_vector(15 downto 0);
      rRpHighTimeInRClks      : out std_logic_vector(15 downto 0);
      rLoadRptCounts          : out std_logic;
      rRptPeriodInRClks       : out std_logic_vector(15 downto 0);
      rRptHighTimeInRClks     : out std_logic_vector(15 downto 0);
      sLoadSpCounts           : out std_logic;
      sSpPeriodInSClks        : out std_logic_vector(15 downto 0);
      sSpHighTimeInSClks      : out std_logic_vector(15 downto 0);
      sLoadSptCounts          : out std_logic;
      sSptPeriodInSClks       : out std_logic_vector(15 downto 0);
      sSptHighTimeInSClks     : out std_logic_vector(15 downto 0));
  end component;

  -- Generic values for the TdcTop instantiation below. These generics are the maximum
  -- of possible values for all combinations of Sample and Reference clocks for the N3xx
  -- family of devices.
  constant kRClksPerRePulsePeriodBitsMax : integer := 24;
  constant kRClksPerRpPeriodBitsMax      : integer := 16;
  constant kSClksPerSpPeriodBitsMax      : integer := 16;
  constant kPulsePeriodCntSize           : integer := 13;
  -- The following are ideal values for balancing measurement time and accuracy, based
  -- on calcs given in the spec doc.
  constant kFreqRefPeriodsToCheckSize : integer := 17;
  constant kSyncPeriodsToStampSize    : integer := 10;

  --vhook_sigstart
  signal aTdcReset: std_logic;
  signal bSyncRegPortInFlat: std_logic_vector(49 downto 0);
  signal bSyncRegPortOutFlat: std_logic_vector(33 downto 0);
  signal mOffsetsDone: boolean;
  signal mOffsetsValid: boolean;
  signal mRpOffset: unsigned(kPulsePeriodCntSize+kSyncPeriodsToStampSize+kFreqRefPeriodsToCheckSize-1 downto 0);
  signal mSpOffset: unsigned(kPulsePeriodCntSize+kSyncPeriodsToStampSize+kFreqRefPeriodsToCheckSize-1 downto 0);
  signal rEnablePpsCrossing: std_logic;
  signal rEnableTdc: std_logic;
  signal rLoadRePulseCounts: std_logic;
  signal rLoadRpCounts: std_logic;
  signal rLoadRptCounts: std_logic;
  signal rPpsPulseCaptured: boolean;
  signal rPulserEnableDelayVal: std_logic_vector(3 downto 0);
  signal rRePulseHighTimeInRClks: std_logic_vector(kRClksPerRePulsePeriodBitsMax-1 downto 0);
  signal rRePulsePeriodInRClks: std_logic_vector(kRClksPerRePulsePeriodBitsMax-1 downto 0);
  signal rReRunEnable: std_logic;
  signal rResetTdc: std_logic;
  signal rResetTdcDone: boolean;
  signal rRpHighTimeInRClks: std_logic_vector(kRClksPerRpPeriodBitsMax-1 downto 0);
  signal rRpPeriodInRClks: std_logic_vector(kRClksPerRpPeriodBitsMax-1 downto 0);
  signal rRptHighTimeInRClks: std_logic_vector(kRClksPerRpPeriodBitsMax-1 downto 0);
  signal rRptPeriodInRClks: std_logic_vector(kRClksPerRpPeriodBitsMax-1 downto 0);
  signal rRpTransferBool: boolean;
  signal sLoadSpCounts: std_logic;
  signal sLoadSptCounts: std_logic;
  signal sPpsClkCrossDelayVal: std_logic_vector(3 downto 0);
  signal sPpsPulseAsyncReset: boolean;
  signal sSpHighTimeInSClks: std_logic_vector(kSClksPerSpPeriodBitsMax-1 downto 0);
  signal sSpPeriodInSClks: std_logic_vector(kSClksPerSpPeriodBitsMax-1 downto 0);
  signal sSptHighTimeInSClks: std_logic_vector(kSClksPerSpPeriodBitsMax-1 downto 0);
  signal sSptPeriodInSClks: std_logic_vector(kSClksPerSpPeriodBitsMax-1 downto 0);
  signal sSpTransferBool: boolean;
  --vhook_sigend

  signal rPpsPulseAsyncReset_ms, rPpsPulseAsyncReset,
         sPpsPulseOut_ms,        sPpsPulseOut : std_logic := '0';

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

  attribute ASYNC_REG : string;
  attribute ASYNC_REG of rPpsPulseAsyncReset_ms : signal is "true";
  attribute ASYNC_REG of rPpsPulseAsyncReset    : signal is "true";
  attribute ASYNC_REG of sPpsPulseOut_ms : signal is "true";
  attribute ASYNC_REG of sPpsPulseOut    : signal is "true";

begin

  -- Cross the PPS from the no-reset domain into the aTdcReset domain since there is a
  -- reset crossing going into the TdcWrapper (reset by aTdcReset)! No clock domain
  -- crossing here, so crossing a single-cycle pulse is safe.
  DoubleSyncToAsyncReset : process (aTdcReset, RefClk)
  begin
    if to_boolean(aTdcReset) then
      rPpsPulseAsyncReset_ms <= '0';
      rPpsPulseAsyncReset    <= '0';
    elsif rising_edge(RefClk) then
      rPpsPulseAsyncReset_ms <= rPpsPulse;
      rPpsPulseAsyncReset    <= rPpsPulseAsyncReset_ms;
    end if;
  end process;

  -- In a similar fashion, cross the output PPS trigger from the async aTdcReset domain
  -- to the no-reset of the rest of the design. The odds of this signal triggering a
  -- failure are astronomically low (since it only pulses one clock cycle per second),
  -- but two flops is worth the assurance it won't mess something else up downstream.
  -- Note this double-sync mainly protects against the reset assertion case, since in the
  -- de-assertion case sPpsPulseAsyncReset should be zero and not transition for a long
  -- time afterwards. Again no clock crossing here, so crossing a single-cycle pulse
  -- is safe.
  DoubleSyncToNoReset : process (SampleClk)
  begin
    if rising_edge(SampleClk) then
      sPpsPulseOut_ms <= to_stdlogic(sPpsPulseAsyncReset);
      sPpsPulseOut    <= sPpsPulseOut_ms;
    end if;
  end process;

  sPpsPulse  <= sPpsPulseOut;


  rRpTransfer <= to_stdlogic(rRpTransferBool);
  sSpTransfer <= to_stdlogic(sSpTransferBool);

  --vhook_e TdcTop
  --vhook_a aReset               to_boolean(aTdcReset)
  --vhook_a rResetTdc            to_boolean(rResetTdc)
  --vhook_a rEnableTdc           to_boolean(rEnableTdc)
  --vhook_a rReRunEnable         to_boolean(rReRunEnable)
  --vhook_a rPpsPulse            to_boolean(rPpsPulseAsyncReset)
  --vhook_a rLoadRePulseCounts   to_boolean(rLoadRePulseCounts)
  --vhook_a rLoadRpCounts        to_boolean(rLoadRpCounts)
  --vhook_a rLoadRptCounts       to_boolean(rLoadRptCounts)
  --vhook_a sLoadSpCounts        to_boolean(sLoadSpCounts)
  --vhook_a sLoadSptCounts       to_boolean(sLoadSptCounts)
  --vhook_a rEnablePpsCrossing   to_boolean(rEnablePpsCrossing)
  --vhook_a rPulserEnableDelayVal unsigned(rPulserEnableDelayVal)
  --vhook_a sPpsClkCrossDelayVal  unsigned(sPpsClkCrossDelayVal)
  --vhook_a rRpTransfer          rRpTransferBool
  --vhook_a sSpTransfer          sSpTransferBool
  --vhook_a sPpsPulse            sPpsPulseAsyncReset
  --vhook_p {^rR(.*)In(.*)Clks}  unsigned(rR$1In$2Clks)
  --vhook_p {^sS(.*)In(.*)Clks}  unsigned(sS$1In$2Clks)
  TdcTopx: entity work.TdcTop (struct)
    generic map (
      kRClksPerRePulsePeriodBitsMax => kRClksPerRePulsePeriodBitsMax,  --integer range 3:32 :=24
      kRClksPerRpPeriodBitsMax      => kRClksPerRpPeriodBitsMax,       --integer range 3:16 :=16
      kSClksPerSpPeriodBitsMax      => kSClksPerSpPeriodBitsMax,       --integer range 3:16 :=16
      kPulsePeriodCntSize           => kPulsePeriodCntSize,            --integer:=13
      kFreqRefPeriodsToCheckSize    => kFreqRefPeriodsToCheckSize,     --integer:=17
      kSyncPeriodsToStampSize       => kSyncPeriodsToStampSize)        --integer:=10
    port map (
      aReset                  => to_boolean(aTdcReset),              --in  boolean
      RefClk                  => RefClk,                             --in  std_logic
      SampleClk               => SampleClk,                          --in  std_logic
      MeasClk                 => MeasClk,                            --in  std_logic
      rResetTdc               => to_boolean(rResetTdc),              --in  boolean
      rResetTdcDone           => rResetTdcDone,                      --out boolean
      rEnableTdc              => to_boolean(rEnableTdc),             --in  boolean
      rReRunEnable            => to_boolean(rReRunEnable),           --in  boolean
      rPpsPulse               => to_boolean(rPpsPulseAsyncReset),    --in  boolean
      rPpsPulseCaptured       => rPpsPulseCaptured,                  --out boolean
      rPulserEnableDelayVal   => unsigned(rPulserEnableDelayVal),    --in  unsigned(3:0)
      rEnablePpsCrossing      => to_boolean(rEnablePpsCrossing),     --in  boolean
      sPpsClkCrossDelayVal    => unsigned(sPpsClkCrossDelayVal),     --in  unsigned(3:0)
      sPpsPulse               => sPpsPulseAsyncReset,                --out boolean
      mRpOffset               => mRpOffset,                          --out unsigned(kPulsePeriodCntSize+ kSyncPeriodsToStampSize+ kFreqRefPeriodsToCheckSize-1:0)
      mSpOffset               => mSpOffset,                          --out unsigned(kPulsePeriodCntSize+ kSyncPeriodsToStampSize+ kFreqRefPeriodsToCheckSize-1:0)
      mOffsetsDone            => mOffsetsDone,                       --out boolean
      mOffsetsValid           => mOffsetsValid,                      --out boolean
      rLoadRePulseCounts      => to_boolean(rLoadRePulseCounts),     --in  boolean
      rRePulsePeriodInRClks   => unsigned(rRePulsePeriodInRClks),    --in  unsigned(kRClksPerRePulsePeriodBitsMax-1:0)
      rRePulseHighTimeInRClks => unsigned(rRePulseHighTimeInRClks),  --in  unsigned(kRClksPerRePulsePeriodBitsMax-1:0)
      rLoadRpCounts           => to_boolean(rLoadRpCounts),          --in  boolean
      rRpPeriodInRClks        => unsigned(rRpPeriodInRClks),         --in  unsigned(kRClksPerRpPeriodBitsMax-1:0)
      rRpHighTimeInRClks      => unsigned(rRpHighTimeInRClks),       --in  unsigned(kRClksPerRpPeriodBitsMax-1:0)
      rLoadRptCounts          => to_boolean(rLoadRptCounts),         --in  boolean
      rRptPeriodInRClks       => unsigned(rRptPeriodInRClks),        --in  unsigned(kRClksPerRpPeriodBitsMax-1:0)
      rRptHighTimeInRClks     => unsigned(rRptHighTimeInRClks),      --in  unsigned(kRClksPerRpPeriodBitsMax-1:0)
      sLoadSpCounts           => to_boolean(sLoadSpCounts),          --in  boolean
      sSpPeriodInSClks        => unsigned(sSpPeriodInSClks),         --in  unsigned(kSClksPerSpPeriodBitsMax-1:0)
      sSpHighTimeInSClks      => unsigned(sSpHighTimeInSClks),       --in  unsigned(kSClksPerSpPeriodBitsMax-1:0)
      sLoadSptCounts          => to_boolean(sLoadSptCounts),         --in  boolean
      sSptPeriodInSClks       => unsigned(sSptPeriodInSClks),        --in  unsigned(kSClksPerSpPeriodBitsMax-1:0)
      sSptHighTimeInSClks     => unsigned(sSptHighTimeInSClks),      --in  unsigned(kSClksPerSpPeriodBitsMax-1:0)
      rRpTransfer             => rRpTransferBool,                    --out boolean
      sSpTransfer             => sSpTransferBool,                    --out boolean
      rGatedPulseToPin        => rGatedPulseToPin,                   --inout std_logic
      sGatedPulseToPin        => sGatedPulseToPin);                  --inout std_logic

  -- Expand/compress the RegPort for moving through the netlist boundary.
  bSyncRegPortOut <= Unflatten(bSyncRegPortOutFlat);
  bSyncRegPortInFlat <= Flatten(bSyncRegPortIn);

  --vhook   SyncRegsIfc
  --vhook_# Tying this low is safe because the sync reset is used inside SyncRegsIfc.
  --vhook_a aBusReset '0'
  --vhook_a bRegPortInFlat  bSyncRegPortInFlat
  --vhook_a bRegPortOutFlat bSyncRegPortOutFlat
  --vhook_a rResetTdcDone     to_stdlogic(rResetTdcDone)
  --vhook_a rPpsPulseCaptured to_stdlogic(rPpsPulseCaptured)
  --vhook_a mOffsetsDone  to_stdlogic(mOffsetsDone)
  --vhook_a mOffsetsValid to_stdlogic(mOffsetsValid)
  --vhook_a mRpOffset std_logic_vector(mRpOffset)
  --vhook_a mSpOffset std_logic_vector(mSpOffset)
  SyncRegsIfcx: SyncRegsIfc
    port map (
      aBusReset               => '0',                             --in  std_logic
      bBusReset               => bBusReset,                       --in  std_logic
      BusClk                  => BusClk,                          --in  std_logic
      aTdcReset               => aTdcReset,                       --out std_logic
      bRegPortInFlat          => bSyncRegPortInFlat,              --in  std_logic_vector(49:0)
      bRegPortOutFlat         => bSyncRegPortOutFlat,             --out std_logic_vector(33:0)
      RefClk                  => RefClk,                          --in  std_logic
      rResetTdc               => rResetTdc,                       --out std_logic
      rResetTdcDone           => to_stdlogic(rResetTdcDone),      --in  std_logic
      rEnableTdc              => rEnableTdc,                      --out std_logic
      rReRunEnable            => rReRunEnable,                    --out std_logic
      rEnablePpsCrossing      => rEnablePpsCrossing,              --out std_logic
      rPpsPulseCaptured       => to_stdlogic(rPpsPulseCaptured),  --in  std_logic
      rPulserEnableDelayVal   => rPulserEnableDelayVal,           --out std_logic_vector(3:0)
      SampleClk               => SampleClk,                       --in  std_logic
      sPpsClkCrossDelayVal    => sPpsClkCrossDelayVal,            --out std_logic_vector(3:0)
      MeasClk                 => MeasClk,                         --in  std_logic
      mRpOffset               => std_logic_vector(mRpOffset),     --in  std_logic_vector(39:0)
      mSpOffset               => std_logic_vector(mSpOffset),     --in  std_logic_vector(39:0)
      mOffsetsDone            => to_stdlogic(mOffsetsDone),       --in  std_logic
      mOffsetsValid           => to_stdlogic(mOffsetsValid),      --in  std_logic
      rLoadRePulseCounts      => rLoadRePulseCounts,              --out std_logic
      rRePulsePeriodInRClks   => rRePulsePeriodInRClks,           --out std_logic_vector(23:0)
      rRePulseHighTimeInRClks => rRePulseHighTimeInRClks,         --out std_logic_vector(23:0)
      rLoadRpCounts           => rLoadRpCounts,                   --out std_logic
      rRpPeriodInRClks        => rRpPeriodInRClks,                --out std_logic_vector(15:0)
      rRpHighTimeInRClks      => rRpHighTimeInRClks,              --out std_logic_vector(15:0)
      rLoadRptCounts          => rLoadRptCounts,                  --out std_logic
      rRptPeriodInRClks       => rRptPeriodInRClks,               --out std_logic_vector(15:0)
      rRptHighTimeInRClks     => rRptHighTimeInRClks,             --out std_logic_vector(15:0)
      sLoadSpCounts           => sLoadSpCounts,                   --out std_logic
      sSpPeriodInSClks        => sSpPeriodInSClks,                --out std_logic_vector(15:0)
      sSpHighTimeInSClks      => sSpHighTimeInSClks,              --out std_logic_vector(15:0)
      sLoadSptCounts          => sLoadSptCounts,                  --out std_logic
      sSptPeriodInSClks       => sSptPeriodInSClks,               --out std_logic_vector(15:0)
      sSptHighTimeInSClks     => sSptHighTimeInSClks);            --out std_logic_vector(15:0)


end struct;


--------------------------------------------------------------------------------
-- Testbench for TdcWrapper
--------------------------------------------------------------------------------

--synopsys translate_off
library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;

library work;
  use work.PkgRegs.all;

entity tb_TdcWrapper is end tb_TdcWrapper;

architecture test of tb_TdcWrapper is

  --vhook_sigstart
  signal bBusReset: std_logic;
  signal bSyncRegPortIn: RegPortIn_t;
  signal bSyncRegPortOut: RegPortOut_t;
  signal BusClk: std_logic := '0';
  signal MeasClk: std_logic := '0';
  signal RefClk: std_logic := '0';
  signal rGatedPulseToPin: std_logic;
  signal rPpsPulse: std_logic;
  signal rRpTransfer: std_logic;
  signal SampleClk: std_logic := '0';
  signal sGatedPulseToPin: std_logic;
  signal sPpsPulse: std_logic;
  signal sSpTransfer: std_logic;
  --vhook_sigend

begin

  --vhook_e TdcWrapper dutx
  dutx: entity work.TdcWrapper (struct)
    port map (
      BusClk           => BusClk,            --in  std_logic
      bBusReset        => bBusReset,         --in  std_logic
      RefClk           => RefClk,            --in  std_logic
      SampleClk        => SampleClk,         --in  std_logic
      MeasClk          => MeasClk,           --in  std_logic
      bSyncRegPortOut  => bSyncRegPortOut,   --out RegPortOut_t
      bSyncRegPortIn   => bSyncRegPortIn,    --in  RegPortIn_t
      rPpsPulse        => rPpsPulse,         --in  std_logic
      sPpsPulse        => sPpsPulse,         --out std_logic
      rRpTransfer      => rRpTransfer,       --out std_logic
      sSpTransfer      => sSpTransfer,       --out std_logic
      rGatedPulseToPin => rGatedPulseToPin,  --inout std_logic
      sGatedPulseToPin => sGatedPulseToPin); --inout std_logic

  main: process

  begin
    report "TdcWrapper Test is EMPTY! (but that's ok in this case)" severity note;
    --vhook_nowarn tb_TdcWrapper.test.*
    wait;
  end process;

end test;
--synopsys translate_on
