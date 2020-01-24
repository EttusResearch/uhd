-------------------------------------------------------------------------------
--
-- File: DbCore.vhd
-- Author: Daniel Jepson
-- Original Project: N310
-- Date: 12 April 2017
--
-------------------------------------------------------------------------------
-- Copyright 2017-2018 Ettus Research, A National Instruments Company
-- SPDX-License-Identifier: LGPL-3.0
-------------------------------------------------------------------------------
--
-- Purpose:
--
-- Wrapper file for Daughterboard Control. This includes the semi-static control
-- and status registers, clocking, synchronization, and JESD204B cores.
--
-- There is no version register for the plain-text files here.
-- Version control for the Sync and JESD204B cores is internal to the netlists.
--
-- The resets for this core are almost entirely local and/or synchronous.
-- bBusReset is a Synchronous reset on the BusClk domain that resets all of the
-- registers connected to the RegPort, as well as any other stray registers
-- connected to the BusClk. All other resets are local to the modules they touch.
-- No other reset drives all modules universally.
--
-------------------------------------------------------------------------------

library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;

library work;
  use work.PkgMgPersonality.all;
  use work.PkgRegs.all;
  use work.PkgJesdConfig.all;


entity DbCore is
  generic(
    -- Set to '1' to include the White Rabbit TDC.
    kInclWhiteRabbitTdc : std_logic := '0'
  );
  port(

    -- Resets --
    -- Synchronous Reset for the BusClk domain (mainly for the RegPort)
    bBusReset              : in  std_logic;

    -- Clocks --
    -- Register Bus Clock (any frequency)
    BusClk                 : in  std_logic;
    -- Always-on at 40 MHz
    Clk40                  : in  std_logic;
    -- Super secret crazy awesome measurement clock at weird frequencies.
    MeasClk                : in  std_logic;
    -- FPGA Sample Clock from DB LMK
    FpgaClk_p              : in  std_logic;
    FpgaClk_n              : in  std_logic;

    -- Sample Clock Sharing. The clocks generated in this module are exported out to the
    -- top level so they can be shared amongst daughterboards. Therefore they must be
    -- driven back into the SampleClk*x inputs at a higher level in order for this module
    -- to work correctly. There are a few isolated cases where SampleClk*xOut is used
    -- directly in this module, and those are documented below.
    SampleClk1xOut         : out std_logic;
    SampleClk1x            : in  std_logic;
    SampleClk2xOut         : out std_logic;
    SampleClk2x            : in  std_logic;


    -- Register Ports --
    --
    -- Only synchronous resets can be used for these ports!
    bRegPortInFlat         : in  std_logic_vector(49 downto 0);
    bRegPortOutFlat        : out std_logic_vector(33 downto 0);

    -- Slot ID value. This should be tied to a constant!
    kSlotId                : in  std_logic;


    -- SYSREF --
    --
    -- SYSREF direct from the LMK
    sSysRefFpgaLvds_p,
    sSysRefFpgaLvds_n      : in  std_logic;
    -- SYNC directly to the LMK
    aLmkSync               : out std_logic;


    -- JESD Signals --
    --
    -- GTX Sample Clock Reference Input. Direct connect to FPGA pins.
    JesdRefClk_p,
    JesdRefClk_n           : in  std_logic;

    -- ADC JESD PHY Interface. Direct connect to FPGA pins.
    aAdcRx_p,
    aAdcRx_n               : in  std_logic_vector(3 downto 0);
    aSyncAdcOut_n          : out std_logic;

    -- DAC JESD PHY Interface. Direct connect to FPGA pins.
    aDacTx_p,
    aDacTx_n               : out std_logic_vector(3 downto 0);
    aSyncDacIn_n           : in  std_logic;


    -- Data Pipes to/from the DACs/ADCs --
    --
    --  - Data is presented as one sample per cycle.
    --  - sAdcDataValid asserts when ADC data is valid.
    --  - sDacReadyForInput asserts when DAC data is ready to be received.
    --
    -- Reset Crossings:
    -- The ADC data and valid outputs are synchronously cleared before the asynchronous
    -- reset is asserted--preventing any reset crossing issues here between the RX
    -- (internal to the core) reset and the no-reset domain of RFNoC.
    --
    -- The DAC samples should be zeros on reset de-assertion due to RFI being de-asserted
    -- in reset. If they are not zeros, then it is still OK because data is ignored until
    -- RFI is asserted. DAC RFI is double-synchronized to protect against the reset
    -- crossing. This is safe to do because it simply delays the output of RFI by two
    -- cycles on the assertion edge, and as long as reset is held for more than two
    -- cycles, the de-assertion edge of RFI should come long before the TX module is
    -- taken out of reset.
    sAdcDataValid          : out std_logic;
    sAdcDataSamples0I      : out std_logic_vector(15 downto 0);
    sAdcDataSamples0Q      : out std_logic_vector(15 downto 0);
    sAdcDataSamples1I      : out std_logic_vector(15 downto 0);
    sAdcDataSamples1Q      : out std_logic_vector(15 downto 0);
    sDacReadyForInput      : out std_logic;
    sDacDataSamples0I      : in  std_logic_vector(15 downto 0);
    sDacDataSamples0Q      : in  std_logic_vector(15 downto 0);
    sDacDataSamples1I      : in  std_logic_vector(15 downto 0);
    sDacDataSamples1Q      : in  std_logic_vector(15 downto 0);


    -- RefClk & Timing & Sync --
    RefClk                 : in  std_logic;
    rPpsPulse              : in  std_logic;
    rGatedPulseToPin       : inout std_logic; -- straight to pin
    sGatedPulseToPin       : inout std_logic; -- straight to pin
    sPps                   : out std_logic;
    sPpsToIob              : out std_logic;

    -- White Rabbit Timing & Sync --
    WrRefClk               : in  std_logic;
    rWrPpsPulse            : in  std_logic;
    rWrGatedPulseToPin     : inout std_logic; -- straight to pin
    sWrGatedPulseToPin     : inout std_logic; -- straight to pin
    aPpsSfpSel             : in  std_logic_vector(1 downto 0);


    -- Debug for JESD
    sAdcSync               : out std_logic;
    sDacSync               : out std_logic;
    sSysRef                : out std_logic;

    -- Debug for Timing & Sync
    rRpTransfer            : out std_logic;
    sSpTransfer            : out std_logic;
    rWrRpTransfer          : out std_logic;
    sWrSpTransfer          : out std_logic
  );

end DbCore;


architecture RTL of DbCore is

  component Jesd204bXcvrCore
    port (
      bBusReset          : in  STD_LOGIC;
      BusClk             : in  STD_LOGIC;
      ReliableClk40      : in  STD_LOGIC;
      FpgaClk1x          : in  STD_LOGIC;
      FpgaClk2x          : in  STD_LOGIC;
      bFpgaClksStable    : in  STD_LOGIC;
      bRegPortInFlat     : in  STD_LOGIC_VECTOR(49 downto 0);
      bRegPortOutFlat    : out STD_LOGIC_VECTOR(33 downto 0);
      aLmkSync           : out STD_LOGIC;
      cSysRefFpgaLvds_p  : in  STD_LOGIC;
      cSysRefFpgaLvds_n  : in  STD_LOGIC;
      fSysRef            : out STD_LOGIC;
      CaptureSysRefClk   : in  STD_LOGIC;
      JesdRefClk_p       : in  STD_LOGIC;
      JesdRefClk_n       : in  STD_LOGIC;
      bJesdRefClkPresent : out STD_LOGIC;
      aAdcRx_p           : in  STD_LOGIC_VECTOR(3 downto 0);
      aAdcRx_n           : in  STD_LOGIC_VECTOR(3 downto 0);
      aSyncAdcOut_n      : out STD_LOGIC;
      aDacTx_p           : out STD_LOGIC_VECTOR(3 downto 0);
      aDacTx_n           : out STD_LOGIC_VECTOR(3 downto 0);
      aSyncDacIn_n       : in  STD_LOGIC;
      fAdc0DataFlat      : out STD_LOGIC_VECTOR(31 downto 0);
      fAdc1DataFlat      : out STD_LOGIC_VECTOR(31 downto 0);
      fDac0DataFlat      : in  STD_LOGIC_VECTOR(31 downto 0);
      fDac1DataFlat      : in  STD_LOGIC_VECTOR(31 downto 0);
      fAdcDataValid      : out STD_LOGIC;
      fDacReadyForInput  : out STD_LOGIC;
      aDacSync           : out STD_LOGIC;
      aAdcSync           : out STD_LOGIC);
  end component;

  function to_Boolean (s : std_ulogic) return boolean is
  begin
    return (To_X01(s)='1');
  end to_Boolean;

  function to_StdLogic(b : boolean) return std_ulogic is
  begin
    if b then
      return '1';
    else
      return '0';
    end if;
  end to_StdLogic;

  --vhook_sigstart
  signal aAdcSync: STD_LOGIC;
  signal aDacSync: STD_LOGIC;
  signal bClockingRegPortOut: RegPortOut_t;
  signal bDbRegPortOut: RegPortOut_t;
  signal bFpgaClksStable: STD_LOGIC;
  signal bJesdCoreRegPortInFlat: STD_LOGIC_VECTOR(49 downto 0);
  signal bJesdCoreRegPortOutFlat: STD_LOGIC_VECTOR(33 downto 0);
  signal bJesdRefClkPresent: STD_LOGIC;
  signal bRadioClk1xEnabled: std_logic;
  signal bRadioClk2xEnabled: std_logic;
  signal bRadioClk3xEnabled: std_logic;
  signal bRadioClkMmcmReset: std_logic;
  signal bRadioClksValid: std_logic;
  signal pPsDone: std_logic;
  signal pPsEn: std_logic;
  signal pPsInc: std_logic;
  signal PsClk: std_logic;
  signal sAdc0DataFlat: STD_LOGIC_VECTOR(31 downto 0);
  signal sAdc1DataFlat: STD_LOGIC_VECTOR(31 downto 0);
  signal SampleClk1xOutLcl: std_logic;
  signal sDac0DataFlat: STD_LOGIC_VECTOR(31 downto 0);
  signal sDac1DataFlat: STD_LOGIC_VECTOR(31 downto 0);
  signal sDacReadyForInputAsyncReset: STD_LOGIC;
  signal sRegPps: std_logic;
  signal sSysRefAsyncReset: STD_LOGIC;
  signal sWrPps: std_logic;
  --vhook_sigend

  signal bJesdRegPortInGrp, bSyncRegPortIn, bWrSyncRegPortIn, bRegPortIn : RegPortIn_t;
  signal bJesdRegPortOut, bSyncRegPortOut, bWrSyncRegPortOut, bRegPortOut : RegPortOut_t;

  signal sDacReadyForInput_ms,   sDacReadyForInputLcl,
         sDacSync_ms,            sDacSyncLcl,
         sAdcSync_ms,            sAdcSyncLcl,
         sSysRef_ms,             sSysRefLcl    : std_logic := '0';

  signal sAdc0Data, sAdc1Data : AdcData_t;
  signal sDac0Data, sDac1Data : DacData_t;

  signal sPpsSfpSel_ms, sPpsSfpSel : std_logic_vector(1 downto 0) := (others => '0');
  signal sUseWrTdcPps : boolean := false;
  signal sPpsInt, sPpsMuxed : std_logic := '0';

  attribute ASYNC_REG : string;
  attribute ASYNC_REG of sDacReadyForInput_ms : signal is "true";
  attribute ASYNC_REG of sDacReadyForInputLcl : signal is "true";
  attribute ASYNC_REG of sDacSync_ms : signal is "true";
  attribute ASYNC_REG of sDacSyncLcl : signal is "true";
  attribute ASYNC_REG of sAdcSync_ms : signal is "true";
  attribute ASYNC_REG of sAdcSyncLcl : signal is "true";
  attribute ASYNC_REG of sSysRef_ms  : signal is "true";
  attribute ASYNC_REG of sSysRefLcl  : signal is "true";
  attribute ASYNC_REG of sPpsSfpSel_ms  : signal is "true";
  attribute ASYNC_REG of sPpsSfpSel     : signal is "true";

begin

  bRegPortOutFlat <= Flatten(bRegPortOut);
  bRegPortIn      <= Unflatten(bRegPortInFlat);


  -- Combine return RegPorts.
  bRegPortOut <=   bJesdRegPortOut
                 + bClockingRegPortOut
                 + bSyncRegPortOut + bWrSyncRegPortOut
                 + bDbRegPortOut;


  -- Clocking : -------------------------------------------------------------------------
  -- Automatically export the Sample Clocks and only use the incoming clocks in the
  -- remainder of the logic. For a single module, the clocks must be looped back
  -- in at a higher level!
  -- ------------------------------------------------------------------------------------

  --vhook_e RadioClocking
  --vhook_a aReset false
  --vhook_a bReset to_boolean(bBusReset)
  --vhook_a RadioClk1x    SampleClk1xOutLcl
  --vhook_a RadioClk2x    SampleClk2xOut
  --vhook_a RadioClk3x    open
  RadioClockingx: entity work.RadioClocking (rtl)
    port map (
      aReset             => false,                  --in  boolean
      bReset             => to_boolean(bBusReset),  --in  boolean
      BusClk             => BusClk,                 --in  std_logic
      bRadioClkMmcmReset => bRadioClkMmcmReset,     --in  std_logic
      bRadioClksValid    => bRadioClksValid,        --out std_logic
      bRadioClk1xEnabled => bRadioClk1xEnabled,     --in  std_logic
      bRadioClk2xEnabled => bRadioClk2xEnabled,     --in  std_logic
      bRadioClk3xEnabled => bRadioClk3xEnabled,     --in  std_logic
      pPsInc             => pPsInc,                 --in  std_logic
      pPsEn              => pPsEn,                  --in  std_logic
      PsClk              => PsClk,                  --in  std_logic
      pPsDone            => pPsDone,                --out std_logic
      FpgaClk_n          => FpgaClk_n,              --in  std_logic
      FpgaClk_p          => FpgaClk_p,              --in  std_logic
      RadioClk1x         => SampleClk1xOutLcl,      --out std_logic
      RadioClk2x         => SampleClk2xOut,         --out std_logic
      RadioClk3x         => open);                  --out std_logic

  -- We need an internal copy of SampleClk1x for the TDC, since we don't want to try
  -- and align the other DB's clock accidentally.
  SampleClk1xOut <= SampleClk1xOutLcl;

  --vhook_e ClockingRegs
  --vhook_a aReset false
  --vhook_a bReset to_boolean(bBusReset)
  --vhook_a bRegPortOut       bClockingRegPortOut
  --vhook_a aRadioClksValid   bRadioClksValid
  ClockingRegsx: entity work.ClockingRegs (RTL)
    port map (
      aReset             => false,                  --in  boolean
      bReset             => to_boolean(bBusReset),  --in  boolean
      BusClk             => BusClk,                 --in  std_logic
      bRegPortOut        => bClockingRegPortOut,    --out RegPortOut_t
      bRegPortIn         => bRegPortIn,             --in  RegPortIn_t
      pPsInc             => pPsInc,                 --out std_logic
      pPsEn              => pPsEn,                  --out std_logic
      pPsDone            => pPsDone,                --in  std_logic
      PsClk              => PsClk,                  --out std_logic
      bRadioClkMmcmReset => bRadioClkMmcmReset,     --out std_logic
      aRadioClksValid    => bRadioClksValid,        --in  std_logic
      bRadioClk1xEnabled => bRadioClk1xEnabled,     --out std_logic
      bRadioClk2xEnabled => bRadioClk2xEnabled,     --out std_logic
      bRadioClk3xEnabled => bRadioClk3xEnabled,     --out std_logic
      bJesdRefClkPresent => bJesdRefClkPresent);    --in  std_logic



  -- JESD204B : -------------------------------------------------------------------------
  -- ------------------------------------------------------------------------------------

  bJesdRegPortInGrp <= Mask(RegPortIn       => bRegPortIn,
                            kRegisterOffset => kJesdRegGroupInDbRegs); -- 0x2000 to 0x3FFC

  -- Expand/compress the RegPort for moving through the netlist boundary.
  bJesdRegPortOut <= Unflatten(bJesdCoreRegPortOutFlat);
  bJesdCoreRegPortInFlat <= Flatten(bJesdRegPortInGrp);

  --vhook   Jesd204bXcvrCore
  --vhook_a bRegPortInFlat   bJesdCoreRegPortInFlat
  --vhook_a bRegPortOutFlat  bJesdCoreRegPortOutFlat
  --vhook_a FpgaClk1x        SampleClk1x
  --vhook_a FpgaClk2x        SampleClk2x
  --vhook_a ReliableClk40    Clk40
  --vhook_a CaptureSysRefClk   SampleClk1xOutLcl
  --vhook_a cSysRefFpgaLvds_p  sSysRefFpgaLvds_p
  --vhook_a cSysRefFpgaLvds_n  sSysRefFpgaLvds_n
  --vhook_a fSysRef            sSysRefAsyncReset
  --vhook_a fDacReadyForInput  sDacReadyForInputAsyncReset
  --vhook_a {^f(.*)}         s$1
  Jesd204bXcvrCorex: Jesd204bXcvrCore
    port map (
      bBusReset          => bBusReset,                    --in  STD_LOGIC
      BusClk             => BusClk,                       --in  STD_LOGIC
      ReliableClk40      => Clk40,                        --in  STD_LOGIC
      FpgaClk1x          => SampleClk1x,                  --in  STD_LOGIC
      FpgaClk2x          => SampleClk2x,                  --in  STD_LOGIC
      bFpgaClksStable    => bFpgaClksStable,              --in  STD_LOGIC
      bRegPortInFlat     => bJesdCoreRegPortInFlat,       --in  STD_LOGIC_VECTOR(49:0)
      bRegPortOutFlat    => bJesdCoreRegPortOutFlat,      --out STD_LOGIC_VECTOR(33:0)
      aLmkSync           => aLmkSync,                     --out STD_LOGIC
      cSysRefFpgaLvds_p  => sSysRefFpgaLvds_p,            --in  STD_LOGIC
      cSysRefFpgaLvds_n  => sSysRefFpgaLvds_n,            --in  STD_LOGIC
      fSysRef            => sSysRefAsyncReset,            --out STD_LOGIC
      CaptureSysRefClk   => SampleClk1xOutLcl,            --in  STD_LOGIC
      JesdRefClk_p       => JesdRefClk_p,                 --in  STD_LOGIC
      JesdRefClk_n       => JesdRefClk_n,                 --in  STD_LOGIC
      bJesdRefClkPresent => bJesdRefClkPresent,           --out STD_LOGIC
      aAdcRx_p           => aAdcRx_p,                     --in  STD_LOGIC_VECTOR(3:0)
      aAdcRx_n           => aAdcRx_n,                     --in  STD_LOGIC_VECTOR(3:0)
      aSyncAdcOut_n      => aSyncAdcOut_n,                --out STD_LOGIC
      aDacTx_p           => aDacTx_p,                     --out STD_LOGIC_VECTOR(3:0)
      aDacTx_n           => aDacTx_n,                     --out STD_LOGIC_VECTOR(3:0)
      aSyncDacIn_n       => aSyncDacIn_n,                 --in  STD_LOGIC
      fAdc0DataFlat      => sAdc0DataFlat,                --out STD_LOGIC_VECTOR(31:0)
      fAdc1DataFlat      => sAdc1DataFlat,                --out STD_LOGIC_VECTOR(31:0)
      fDac0DataFlat      => sDac0DataFlat,                --in  STD_LOGIC_VECTOR(31:0)
      fDac1DataFlat      => sDac1DataFlat,                --in  STD_LOGIC_VECTOR(31:0)
      fAdcDataValid      => sAdcDataValid,                --out STD_LOGIC
      fDacReadyForInput  => sDacReadyForInputAsyncReset,  --out STD_LOGIC
      aDacSync           => aDacSync,                     --out STD_LOGIC
      aAdcSync           => aAdcSync);                    --out STD_LOGIC

  JesdDoubleSyncToNoResetSampleClk : process (SampleClk1x)
  begin
    if rising_edge(SampleClk1x) then
      sDacReadyForInput_ms <= sDacReadyForInputAsyncReset;
      sDacReadyForInputLcl <= sDacReadyForInput_ms;
      -- No clock crossing here -- just reset, although the prefix declares otherwise...
      sDacSync_ms <= aDacSync;
      sDacSyncLcl <= sDacSync_ms;
      sAdcSync_ms <= aAdcSync;
      sAdcSyncLcl <= sAdcSync_ms;
      sSysRef_ms  <= sSysRefAsyncReset;
      sSysRefLcl  <= sSysRef_ms;
    end if;
  end process;

  -- Locals to outputs.
  sDacReadyForInput <= sDacReadyForInputLcl;
  sDacSync <= sDacSyncLcl;
  sAdcSync <= sAdcSyncLcl;
  sSysRef  <= sSysRefLcl;

  -- Just combine the first two enables, since they're the ones that are used for JESD.
  -- No reset crossing here, since bFpgaClksStable is only received by a no-reset domain
  -- and the MGTs directly.
  bFpgaClksStable <= bRadioClksValid and bRadioClk1xEnabled and bRadioClk2xEnabled;

  -- Compress/expand the flat data types from the netlist and route to top level.
  sAdc0Data     <= Unflatten(sAdc0DataFlat);
  sAdc1Data     <= Unflatten(sAdc1DataFlat);
  sDac0DataFlat <= Flatten(sDac0Data);
  sDac1DataFlat <= Flatten(sDac1Data);

  sAdcDataSamples0I <= sAdc0Data.I;
  sAdcDataSamples0Q <= sAdc0Data.Q;
  sAdcDataSamples1I <= sAdc1Data.I;
  sAdcDataSamples1Q <= sAdc1Data.Q;

  sDac0Data.I <= sDacDataSamples0I;
  sDac0Data.Q <= sDacDataSamples0Q;
  sDac1Data.I <= sDacDataSamples1I;
  sDac1Data.Q <= sDacDataSamples1Q;


  -- Timing and Sync : ------------------------------------------------------------------
  -- ------------------------------------------------------------------------------------

  bSyncRegPortIn <= Mask(RegPortIn       => bRegPortIn,
                         kRegisterOffset => kTdc0OffsetsInEndpoint); -- 0x0200

  --vhook_e TdcWrapper
  --vhook_# Use the local copy of the SampleClock, since we want the TDC to measure the
  --vhook_# clock offset for this daughterboard, not the global SampleClock.
  --vhook_a SampleClk SampleClk1xOutLcl
  --vhook_a sPpsPulse sRegPps
  TdcWrapperx: entity work.TdcWrapper (struct)
    port map (
      BusClk           => BusClk,             --in  std_logic
      bBusReset        => bBusReset,          --in  std_logic
      RefClk           => RefClk,             --in  std_logic
      SampleClk        => SampleClk1xOutLcl,  --in  std_logic
      MeasClk          => MeasClk,            --in  std_logic
      bSyncRegPortOut  => bSyncRegPortOut,    --out RegPortOut_t
      bSyncRegPortIn   => bSyncRegPortIn,     --in  RegPortIn_t
      rPpsPulse        => rPpsPulse,          --in  std_logic
      sPpsPulse        => sRegPps,            --out std_logic
      rRpTransfer      => rRpTransfer,        --out std_logic
      sSpTransfer      => sSpTransfer,        --out std_logic
      rGatedPulseToPin => rGatedPulseToPin,   --inout std_logic
      sGatedPulseToPin => sGatedPulseToPin);  --inout std_logic

  WrTdcGen: if kInclWhiteRabbitTdc = '1' generate
    bWrSyncRegPortIn <= Mask(RegPortIn       => bRegPortIn,
                             kRegisterOffset => kTdc1OffsetsInEndpoint); -- 0x0400

    --vhook_e TdcWrapper WrTdcWrapperx
    --vhook_# Use the local copy of the SampleClock, since we want the TDC to measure the
    --vhook_# clock offset for this daughterboard, not the global SampleClock.
    --vhook_a bSyncRegPortIn  bWrSyncRegPortIn
    --vhook_a bSyncRegPortOut bWrSyncRegPortOut
    --vhook_a SampleClk SampleClk1xOutLcl
    --vhook_a RefClk WrRefClk
    --vhook_a rPpsPulse rWrPpsPulse
    --vhook_a sPpsPulse sWrPps
    --vhook_a rRpTransfer rWrRpTransfer
    --vhook_a sSpTransfer sWrSpTransfer
    --vhook_a rGatedPulseToPin rWrGatedPulseToPin
    --vhook_a sGatedPulseToPin sWrGatedPulseToPin
    WrTdcWrapperx: entity work.TdcWrapper (struct)
      port map (
        BusClk           => BusClk,              --in  std_logic
        bBusReset        => bBusReset,           --in  std_logic
        RefClk           => WrRefClk,            --in  std_logic
        SampleClk        => SampleClk1xOutLcl,   --in  std_logic
        MeasClk          => MeasClk,             --in  std_logic
        bSyncRegPortOut  => bWrSyncRegPortOut,   --out RegPortOut_t
        bSyncRegPortIn   => bWrSyncRegPortIn,    --in  RegPortIn_t
        rPpsPulse        => rWrPpsPulse,         --in  std_logic
        sPpsPulse        => sWrPps,              --out std_logic
        rRpTransfer      => rWrRpTransfer,       --out std_logic
        sSpTransfer      => sWrSpTransfer,       --out std_logic
        rGatedPulseToPin => rWrGatedPulseToPin,  --inout std_logic
        sGatedPulseToPin => sWrGatedPulseToPin); --inout std_logic
  end generate WrTdcGen;

  WrTdcNotGen: if kInclWhiteRabbitTdc = '0' generate
    bWrSyncRegPortOut <= kRegPortOutZero;
    sWrPps <= '0';
    rWrRpTransfer <= '0';
    sWrSpTransfer <= '0';
    rWrGatedPulseToPin <= '0';
    sWrGatedPulseToPin <= '0';
  end generate WrTdcNotGen;

  -- Mux the output PPS based on the SFP selection bits. Encoding is one-hot, with zero
  -- also a valid state. Regardless of whether the user selects SFP0 or SFP1 as the time
  -- source, there is only one White Rabbit TDC, so '01' and '10' are equivalent.
  -- '00': Use the PPS output from the "regular" TDC.
  -- '01': Use the PPS output from the "white rabbit" TDC.
  -- '10': Use the PPS output from the "white rabbit" TDC.
  PpsOutputMux : process (SampleClk1xOutLcl)
  begin
    if rising_edge(SampleClk1xOutLcl) then
      -- Double-sync the control bits to the Sample Clock domain.
      sPpsSfpSel_ms <= aPpsSfpSel;
      sPpsSfpSel    <= sPpsSfpSel_ms;

      -- OR the control bits together to produce a single override enable for the WR TDC.
      sUseWrTdcPps <= to_boolean(sPpsSfpSel(0) or sPpsSfpSel(1));

      -- Flop the outputs. One flop for the PPS output IOB, the other for use internally.
      sPpsInt <= sPpsMuxed;
    end if;
  end process PpsOutputMux;

  sPpsMuxed <= sWrPps when sUseWrTdcPps else sRegPps;
  sPps      <= sPpsInt;
  sPpsToIob <= sPpsMuxed; -- No added flop here since there's an IOB outside this module.

  -- Daughterboard Control : ------------------------------------------------------------
  -- ------------------------------------------------------------------------------------

  --vhook_e DaughterboardRegs
  --vhook_# Tying this low is safe because the sync reset is used inside DaughterboardRegs.
  --vhook_a aReset false
  --vhook_a bReset to_boolean(bBusReset)
  --vhook_a bRegPortOut bDbRegPortOut
  --vhook_a kDbId       std_logic_vector(to_unsigned(16#150#,16))
  DaughterboardRegsx: entity work.DaughterboardRegs (RTL)
    port map (
      aReset      => false,                                      --in  boolean
      bReset      => to_boolean(bBusReset),                      --in  boolean
      BusClk      => BusClk,                                     --in  std_logic
      bRegPortOut => bDbRegPortOut,                              --out RegPortOut_t
      bRegPortIn  => bRegPortIn,                                 --in  RegPortIn_t
      kDbId       => std_logic_vector(to_unsigned(16#150#,16)),  --in  std_logic_vector(15:0)
      kSlotId     => kSlotId);                                   --in  std_logic


end RTL;
