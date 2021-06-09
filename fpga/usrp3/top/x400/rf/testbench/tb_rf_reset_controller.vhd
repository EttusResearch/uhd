--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: tb_rf_reset_controller
--
-- Description:
--
--   Testbench for rf_reset_controller.
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

library WORK;
  use WORK.PkgRFDC_REGS_REGMAP.all;

entity tb_rf_reset_controller is
end tb_rf_reset_controller;


architecture RTL of tb_rf_reset_controller is

  component rf_reset_controller
    port (
      ConfigClk          : in  std_logic;
      DataClk            : in  std_logic;
      PllRefClk          : in  std_logic;
      RfClk              : in  std_logic;
      RfClk2x            : in  std_logic;
      DataClk2x          : in  std_logic;
      dAdcResetPulse     : in  std_logic;
      dDacResetPulse     : in  std_logic;
      dAdcDataOutReset_n : out std_logic;
      r2AdcFirReset_n    : out std_logic;
      rAdcRfdcAxiReset_n : out std_logic;
      rAdcEnableData     : out std_logic;
      rAdcGearboxReset_n : out std_logic;
      dDacDataInReset_n  : out std_logic;
      r2DacFirReset_n    : out std_logic;
      d2DacFirReset_n    : out std_logic;
      rDacRfdcAxiReset_n : out std_logic;
      rDacGearboxReset_n : out std_logic;
      cSoftwareControl   : in  std_logic_vector(31 downto 0);
      cSoftwareStatus    : out std_logic_vector(31 downto 0));
  end component;

  signal cSoftwareStatus    : std_logic_vector(31 downto 0);
  signal r2AdcFirReset_n    : std_logic;
  signal r2DacFirReset_n    : std_logic;
  signal rAdcGearboxReset_n : std_logic;
  signal rDacGearboxReset_n : std_logic;

  signal cSoftwareControl : std_logic_vector(31 downto 0) := (others => '0');
  signal dAdcResetPulse   : std_logic := '0';
  signal dDacResetPulse   : std_logic := '0';

  constant kSwReset    : std_logic := '0';
  constant kTimedReset : std_logic := '1';

  -- All constants mentioned below are number of the particular clock cycles
  -- PllRefClk period. For example, kDataClkCycles is the total number of
  -- DataClk cycles in the PllRefClk period.
  constant kDataClkCycles   : integer := 2;
  constant kDataClk2xCycles : integer := 4;
  constant kRfClkCycles     : integer := 3;
  constant kRfClk2xCycles   : integer := 6;
  constant kConfigPer       : time := 25 ns;
  -- Make sure the PllRefClk period is a least common multiple of all the other
  -- derived clock.
  constant kPllRefClkPer : time := 12 ns;
  constant kDataClkPer   : time := kPllRefClkPer/2;
  constant kDataClk2xPer : time := kPllRefClkPer/4;
  constant kRfClkPer     : time := kPllRefClkPer/3;
  constant kRfClk2xPer   : time := kPllRefClkPer/6;

  signal pReset  : boolean := false;
  signal dCount  : integer := 0;
  signal d2Count : integer := 0;
  signal rCount  : integer := 0;
  signal r2Count : integer := 0;

  signal StopSim   : boolean;
  signal ConfigClk : std_logic := '1';
  signal RfClk     : std_logic := '1';
  signal RfClk2x   : std_logic := '1';
  signal DataClk   : std_logic := '1';
  signal DataClk2x : std_logic := '1';
  signal PllRefClk : std_logic := '1';

  signal dAdcDataOutReset_n     : std_logic := '0';
  signal dAdcDataOutResetDlyd_n : std_logic := '0';
  signal dDacDataInReset_n      : std_logic := '0';
  signal dDacDataInResetDlyd_n  : std_logic := '0';
  signal d2DacFirReset_n        : std_logic := '0';
  signal d2DacFirResetDlyd_n    : std_logic := '0';
  signal rAdcRfdcAxiReset_n     : std_logic := '0';
  signal rAdcRfdcAxiResetDlyd_n : std_logic := '0';
  signal rDacRfdcAxiReset_n     : std_logic := '0';
  signal rDacRfdcAxiResetDlyd_n : std_logic := '0';
  signal r2AdcFirResetDlyd_n    : std_logic := '0';
  signal r2DacFirResetDlyd_n    : std_logic := '0';

  signal ExpectedSwAdcResetDone : std_logic := '0';
  signal ExpectedAdcReset       : std_logic := '0';
  signal ExpectedSwDacResetDone : std_logic := '0';
  signal ExpectedDacReset       : std_logic := '0';
  signal ExpectedAxiAdcResetOut : std_logic := '0';
  signal ExpectedAxiDacResetOut : std_logic := '0';

  -- Make sure the wait time for reset done check is at least 10 ConfigClk
  -- cycles to account for all clock domain crossings. We also have some status
  -- check in the testbench which requires the wait to be additional ConfigClk
  -- cycles. This wait is in ConfigClk period.
  constant kResetDoneWait : positive := 10;

  procedure ClkWait(signal clk : in std_logic; X : positive := 1) is
  begin
    for i in 1 to X loop
      wait until rising_edge(clk);
    end loop;
  end procedure ClkWait;

  -- Check phase alignment of reset. We want to make sure the reset is asserted
  -- on the 1st rising clock edge after the rising edge of PllRefClk.
  procedure CheckAlignment(
    signal Clk         : in    std_logic; -- Synchronous reset clock
    signal Reset_n     : in    std_logic; -- Synchronous reset
    signal ResetDlyd_n : inout std_logic; -- Delayed synchronous reset
    signal PhaseCount  : in    integer;   -- Phase count used to check alignment
    Message            : string) is       -- Assertion message
  begin

    -- Check if reset is asserted on the 1st Clk after the rising edge of
    -- PllRefClk.
    if falling_edge(Clk) then
      ResetDlyd_n <= Reset_n;
      if Reset_n = '0' and ResetDlyd_n = '1' then
        assert PhaseCount = 1
          report Message & " reset is not asserted in the expected time" severity error;
      end if;
    end if;
  end procedure CheckAlignment;

  -- Procedure to generate phase counter that is used to check the alignment of
  -- phase of all clocks related to PllRefClk.
  procedure PhaseCounter(
    signal Clk        : in    std_logic; -- Clock related to PllRefClk
    signal Reset      : in    boolean;   -- Reset synchronous to PllRefClk
    signal PhaseCount : inout integer;   -- Phase count of Clk with respect to PllRefClk
    ClockCycles       : integer) is      -- Number of Clk clock cycles in PllRefClk period
  begin
    if rising_edge(Clk) then
        if Reset or PhaseCount = ClockCycles-1 then
          PhaseCount <= 0;
        else
          PhaseCount <= PhaseCount+1;
        end if;
    end if;
  end procedure PhaseCounter;

  procedure CheckExpectedValue(
    signal Clk        : in std_logic;
    signal Actual     : in std_logic;
    signal Expected   : in std_logic;
    Message           : string) is
  begin
    if falling_edge(Clk) then
      -- Check if the actual value is as expected.
      assert std_match(Actual, Expected)
        report Message & " not as expected" & LF
             & "Expected = " & std_logic'image(Expected) & LF
             & "Actual = " & std_logic'image(Actual) severity error;
    end if;
  end procedure CheckExpectedValue;
begin

  ConfigClk <= not ConfigClk after kConfigPer/2    when not StopSim else '0';
  RfClk     <= not RfClk     after kRfClkPer/2     when not StopSim else '0';
  RfClk2x   <= not RfClk2x   after kRfClk2xPer/2   when not StopSim else '0';
  DataClk   <= not DataClk   after kDataClkPer/2   when not StopSim else '0';
  DataClk2x <= not DataClk2x after kDataClk2xPer/2 when not StopSim else '0';
  PllRefClk <= not PllRefClk after kPllRefClkPer/2 when not StopSim else '0';

  -- rAdcEnableData is a constant and is not tested.
  dut: rf_reset_controller
    port map (
      ConfigClk          => ConfigClk,
      DataClk            => DataClk,
      PllRefClk          => PllRefClk,
      RfClk              => RfClk,
      RfClk2x            => RfClk2x,
      DataClk2x          => DataClk2x,
      dAdcResetPulse     => dAdcResetPulse,
      dDacResetPulse     => dDacResetPulse,
      dAdcDataOutReset_n => dAdcDataOutReset_n,
      r2AdcFirReset_n    => r2AdcFirReset_n,
      rAdcRfdcAxiReset_n => rAdcRfdcAxiReset_n,
      rAdcEnableData     => open,
      rAdcGearboxReset_n => rAdcGearboxReset_n,
      dDacDataInReset_n  => dDacDataInReset_n,
      r2DacFirReset_n    => r2DacFirReset_n,
      d2DacFirReset_n    => d2DacFirReset_n,
      rDacRfdcAxiReset_n => rDacRfdcAxiReset_n,
      rDacGearboxReset_n => rDacGearboxReset_n,
      cSoftwareControl   => cSoftwareControl,
      cSoftwareStatus    => cSoftwareStatus
    );

  main: process

    -- Procedure to generate software reset and expected DUR reset output.
    procedure StrobeReset(
      signal TimedReset           : out std_logic;    -- SW Reset control
      signal ExpectedResetOut     : out std_logic;    -- Expected reset values
      signal ExpectedAxiResetOut  : out std_logic;    -- Expected reset values
      signal SwResetStatus        : out std_logic;    -- Expected SW reset status
      SwReset                     : integer;          -- SW Reset control
      ResetType                   : std_logic;        -- 0 = SW reset, 1 = UHD timed reset
      ResetWait                   : positive := 1) is -- Wait time for test iteration
    begin
      if ResetType = kSwReset then
        -- Assert software reset control on the rising edge of ConfigClk. Also
        -- change the expected status to don't care as the status will change
        -- only after few ConfigClk period.
        ClkWait(ConfigClk);
        TimedReset <= '0';
        cSoftwareControl(SwReset) <= '1';
        SwResetStatus <= '-';
        ExpectedResetOut <= '-';
        ExpectedAxiResetOut <= '-';
        ClkWait(ConfigClk, 1);
        SwResetStatus <= '0';
        -- Wait for additional ConfigClk before changing the expected reset
        -- value to '0'. This wait is needed to account for pipeline and clock
        -- crossing delays.
        ClkWait(ConfigClk, 1);
        -- Changed expected reset output to '0' (active low).
        ExpectedResetOut <= '0';
        ExpectedAxiResetOut <= '0';
        ClkWait(ConfigClk,1);
        -- SW reset status should be asserted after 3 ConfigClk periods. This
        -- wait is needed to account for pipeline and clock crossings.
        SwResetStatus <= '1';
        -- De-assert software reset
        ClkWait(ConfigClk,2);
        cSoftwareControl(SwReset) <= '0';
        -- Change the expected reset outputs to don't care as it will take few
        -- PllRefClk cycles and ConfigClk to DataClock crossing.
        ExpectedAxiResetOut <= '-';
        ClkWait(ConfigClk,1);
        ExpectedAxiResetOut <= '1';
        -- After few ConfigClk cycles, all reset outputs should be de-asserted.
        ClkWait(ConfigClk,1);
        ExpectedResetOut <= '-';
        ClkWait(ConfigClk,2);
        ExpectedResetOut <= '1';
        -- Wait for ResetWait time before exiting the test iteration.
        ClkWait(ConfigClk,ResetWait);
      else -- Timed command.
        ClkWait(DataClk,ResetWait);
        TimedReset <= '1';
        -- RFDC should not be asserted with timed reset.
        ExpectedAxiResetOut <= '1';
        -- Strobe the reset pulse only for one DataClk period.
        ClkWait(DataClk,1);
        TimedReset <= '0';
        ClkWait(PllRefClk,2);
        ExpectedResetOut <= '-';
        -- Wait for 3 PllRefClk to account for pipeline delays.
        ClkWait(PllRefClk,1);
        ExpectedResetOut <= '0';
        ClkWait(PllRefClk,2);
        ExpectedResetOut <= '-';
        -- Reset should be asserted only for two PllRefClk cycles.
        ClkWait(PllRefClk,2);
        ExpectedResetOut <= '1';
        ClkWait(DataClk,ResetWait); -- Wait between test.
      end if;
    end procedure StrobeReset;

  begin
    -- Expected power on reset values.
    ExpectedAdcReset       <= '0';
    ExpectedAxiAdcResetOut <= '0';
    ExpectedDacReset       <= '0';
    ExpectedAxiDacResetOut <= '0';

    ClkWait(ConfigClk,1);
    ClkWait(RfClk,1);
    ExpectedAxiAdcResetOut <= '1';
    ExpectedAxiDacResetOut <= '1';
    ClkWait(ConfigClk,1);
    ExpectedAdcReset <= '-';
    ExpectedDacReset <= '-';
    ClkWait(ConfigClk,1);
    ExpectedAdcReset <= '1';
    ExpectedDacReset <= '1';
    ClkWait(ConfigClk,5);
    -- This reset is for simulation to have a common reference to check for
    -- clock alignment.
    ClkWait(PllRefClk,1);
    pReset <= true;
    ClkWait(PllRefClk,1);
    pReset <= false;
    ClkWait(PllRefClk,1);

    ---------------------------------------------------------------------------
    -- Test resets from software
    ---------------------------------------------------------------------------

    -----------------------------------
    -- ADC
    -----------------------------------

    StrobeReset(dAdcResetPulse, ExpectedAdcReset, ExpectedAxiAdcResetOut,
                ExpectedSwAdcResetDone, kADC_RESET, kSwReset, kResetDoneWait);

    -- Align reset to the rising edge of PllRefClk
    ClkWait(PllRefClk,1);
    StrobeReset(dAdcResetPulse, ExpectedAdcReset, ExpectedAxiAdcResetOut,
                ExpectedSwAdcResetDone, kADC_RESET, kTimedReset, kResetDoneWait);
    StrobeReset(dAdcResetPulse, ExpectedAdcReset, ExpectedAxiAdcResetOut,
                ExpectedSwAdcResetDone, kADC_RESET, kSwReset, kResetDoneWait);

    -- Align reset to the falling edge of PllRefClk.
    ClkWait(PllRefClk,1);
    ClkWait(DataClk,1);
    StrobeReset(dAdcResetPulse, ExpectedAdcReset, ExpectedAxiAdcResetOut,
                ExpectedSwAdcResetDone, kADC_RESET, kTimedReset, kResetDoneWait);

    -----------------------------------
    -- DAC
    -----------------------------------

    StrobeReset(dDacResetPulse, ExpectedDacReset, ExpectedAxiDacResetOut,
                ExpectedSwDacResetDone, kDAC_RESET, kSwReset, kResetDoneWait);

    -- Align reset to the rising edge of PllRefClk.
    ClkWait(PllRefClk,1);
    StrobeReset(dDacResetPulse, ExpectedDacReset, ExpectedAxiDacResetOut,
                ExpectedSwDacResetDone, kDAC_RESET, kTimedReset, kResetDoneWait);
    StrobeReset(dDacResetPulse, ExpectedDacReset, ExpectedAxiDacResetOut,
                ExpectedSwDacResetDone, kDAC_RESET, kSwReset, kResetDoneWait);

    -- Align reset to the falling edge of PllRefClk.
    ClkWait(PllRefClk,1);
    ClkWait(DataClk,1);
    StrobeReset(dDacResetPulse, ExpectedDacReset, ExpectedAxiDacResetOut,
                ExpectedSwDacResetDone, kDAC_RESET, kTimedReset, kResetDoneWait);

    StopSim <= true;
    wait;
  end process main;

  -----------------------------------------------------------------------------
  -- Reset from software and UHD timed command
  -----------------------------------------------------------------------------
  -- Check if the correct resets are getting asserted when UHD timed reset or
  -- software reset is asserted. Except for RFDC AXI-S reset all other resets
  -- should be strobed for UHD timed reset.
  -----------------------------------------------------------------------------

  -- Check if the reset done status is getting asserted as expected.
  CheckExpectedValue(ConfigClk, cSoftwareStatus(kADC_SEQ_DONE),
                     ExpectedSwAdcResetDone, "ADC reset done status");
  CheckExpectedValue(ConfigClk, cSoftwareStatus(kDAC_SEQ_DONE),
                     ExpectedSwDacResetDone, "DAC reset done status");

  -- Check if resets state in DataClk is as expected.
  CheckExpectedValue(DataClk, dAdcDataOutReset_n, ExpectedAdcReset,
                     "ADC data out reset");
  CheckExpectedValue(DataClk, dDacDataInReset_n, ExpectedDacReset,
                     "DAC data out reset");

  -- Check if resets state in DataClk2x is as expected.
  CheckExpectedValue(DataClk2x, d2DacFirReset_n, ExpectedDacReset,
                     "400M interpolator reset");

  ---- Check if resets state in RfClk2x is as expected.
  CheckExpectedValue(RfClk2x, r2AdcFirReset_n, ExpectedAdcReset,
                     "ADC re-sampler reset");
  CheckExpectedValue(RfClk2x, r2DacFirReset_n, ExpectedDacReset,
                     "DAC re-sampler reset");

  ---- Check if resets state in RfClk is as expected.
  CheckExpectedValue(RfClk, rAdcRfdcAxiReset_n, ExpectedAxiAdcResetOut,
                     "ADC RFDC AXI-S interface reset");
  CheckExpectedValue(RfClk, rDacRfdcAxiReset_n, ExpectedAxiDacResetOut,
                     "DAC RFDC AXI-S interface reset");
  CheckExpectedValue(RfClk, rAdcGearboxReset_n, ExpectedAdcReset,
                     "ADC gearbox reset");
  CheckExpectedValue(RfClk, rDacGearboxReset_n, ExpectedDacReset,
                     "DAC gearbox reset");


  -----------------------------------------------------------------------------
  -- Reset alignment checks for resets
  -----------------------------------------------------------------------------

  -----------------------------------
  -- Clock counter
  -----------------------------------
  -- We use counters to check the phase of all the derived clocks with respect
  -- to PllRefClk. Each counter will rollover at the rising edge of PllRefClk.
  -----------------------------------
  PhaseCounter(DataClk,   pReset, dCount,  kDataClkCycles);
  PhaseCounter(DataClk2x, pReset, d2Count, kDataClk2xCycles);
  PhaseCounter(RfClk,     pReset, rCount,  kRfClkCycles);
  PhaseCounter(RfClk2x,   pReset, r2Count, kRfClk2xCycles);

  -- Check for DataClk based synchronous reset alignment to PllRefClk.
  CheckAlignment(DataClk, dAdcDataOutReset_n, dAdcDataOutResetDlyd_n, dCount,
                 "ADC data out");
  CheckAlignment(DataClk, dDacDataInReset_n, dDacDataInResetDlyd_n, dCount,
                 "DAC data in");

  -- Check for DataClk2x based synchronous reset alignment to PllRefClk.
  CheckAlignment(DataClk2x, d2DacFirReset_n, d2DacFirResetDlyd_n, d2Count,
                 "400M DAC FIR Filter");

  -- Check for RfClk based synchronous reset alignment to PllRefClk.
  CheckAlignment(RfClk, rAdcRfdcAxiReset_n, rAdcRfdcAxiResetDlyd_n, rCount,
                 "ADC RFDC reset ");
  CheckAlignment(RfClk, rDacRfdcAxiReset_n, rDacRfdcAxiResetDlyd_n, rCount,
                 "DAC RFDC reset ");

  -- Check for RfClk2x based synchronous reset alignment to PllRefClk.
  CheckAlignment(RfClk2x, r2AdcFirReset_n, r2AdcFirResetDlyd_n, r2Count,
                 "ADC decimation filter reset ");
  CheckAlignment(RfClk2x, r2DacFirReset_n, r2DacFirResetDlyd_n, r2Count,
                 "DAC interpolation filter reset ");

end RTL;
