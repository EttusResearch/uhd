--
-- Copyright 2022 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: x440_rf_reset_controller
--
-- Description:
--
--   Control RFDC, ADC, and DAC resets.
--   This file contains a similar structure to '../x410/x410_rf_reset_controller',
--   with the main difference of not having to support a complex reset chain.
--   This means that the instantiations of 'rf_reset' can be taken out in favor
--   having a simple combination of the incoming pulses and the software triggers
--   to generate the different outputs.
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

library WORK;
  use WORK.PkgRFDC_REGS_REGMAP.all;

entity x440_rf_reset_controller is
  port(
    -- Clocks
    -- Config clock is async to all the others.
    ConfigClk          : in  std_logic;
    PllRefClk          : in  std_logic;
    RfClk              : in  std_logic;
    RfClk2x            : in  std_logic;

    -- Master resets from the Radio
    rAdcResetPulse     : in  std_logic;
    rDacResetPulse     : in  std_logic;

    -- ADC Resets
    r2AdcReset_n       : out std_logic;
    rAdcEnableData     : out std_logic;
    rAdcReset_n        : out std_logic;

    -- DAC Resets
    r2DacReset_n       : out std_logic;
    rDacReset_n        : out std_logic;

    -- SW Control and Status
    -- Control to initiate resets to RFDC.
    -- The reset status is a sticky status of both ADC and DAC.
    cSoftwareControl   : in  std_logic_vector(31 downto 0);
    cSoftwareStatus    : out std_logic_vector(31 downto 0)
  );
end x440_rf_reset_controller;


architecture RTL of x440_rf_reset_controller is

  -- POR value for all resets are high.
  signal cTriggerAdcReset     : std_logic := '1';
  signal cTriggerAdcResetDlyd : std_logic := '1';
  signal cTriggerDacReset     : std_logic := '1';
  signal cTriggerDacResetDlyd : std_logic := '1';

  signal rTriggerAdcReset_ms : std_logic := '1';
  signal rTriggerAdcReset    : std_logic := '1';
  signal rTriggerDacReset_ms : std_logic := '1';
  signal rTriggerDacReset    : std_logic := '1';

  -- POR value of all reset done signals are set to low.
  signal cTriggerAdcResetDone_ms : std_logic := '0';
  signal cTriggerAdcResetDone    : std_logic := '0';
  signal cAdcResetDoneSticky     : std_logic := '0';
  signal cTriggerDacResetDone_ms : std_logic := '0';
  signal cTriggerDacResetDone    : std_logic := '0';
  signal cDacResetDoneSticky     : std_logic := '0';

  attribute ASYNC_REG : string;
  attribute ASYNC_REG of rTriggerAdcReset        : signal is "TRUE";
  attribute ASYNC_REG of rTriggerDacReset        : signal is "TRUE";
  attribute ASYNC_REG of cTriggerAdcResetDone    : signal is "TRUE";
  attribute ASYNC_REG of cTriggerDacResetDone    : signal is "TRUE";
  attribute ASYNC_REG of rTriggerAdcReset_ms     : signal is "TRUE";
  attribute ASYNC_REG of rTriggerDacReset_ms     : signal is "TRUE";
  attribute ASYNC_REG of cTriggerAdcResetDone_ms : signal is "TRUE";
  attribute ASYNC_REG of cTriggerDacResetDone_ms : signal is "TRUE";

begin

  -- rAdcEnableData is set to '1' as we don't control the flow of RX data.
  rAdcEnableData <= '1';

  cTriggerAdcReset <= cSoftwareControl(kADC_RESET);
  cTriggerDacReset <= cSoftwareControl(kDAC_RESET);

  cSoftwareStatus <= (
                       kADC_SEQ_DONE => cAdcResetDoneSticky,
                       kDAC_SEQ_DONE => cDacResetDoneSticky,
                       others => '0'
                     );

  -----------------------------------------------------------------------------
  -- High-Level Resets Using ConfigClk
  -----------------------------------------------------------------------------
  -- Pass the master FSM reset around to the other clock domains and then
  -- return them back to the ConfigClk domain. This is also a handy way to
  -- prove all your clocks are toggling to some extent.
  -----------------------------------------------------------------------------

  SeqResetRfClk : process(RfClk)
  begin
    if rising_edge(RfClk) then
      -- double-syncs have no sync reset!
      rTriggerAdcReset_ms  <= cTriggerAdcReset;
      rTriggerAdcReset     <= rTriggerAdcReset_ms;
      rTriggerDacReset_ms  <= cTriggerDacReset;
      rTriggerDacReset     <= rTriggerDacReset_ms;
    end if;
  end process;

  -----------------------------------------------------------------------------
  -- Reset Sequence Done Status
  -----------------------------------------------------------------------------
  -- Now back to ConfigClk! We provide the status for all software controlled
  -- resets. We move the signal from ConfigClk to RfClk domain and move it
  -- back to ConfigClk domain. This just proves that RfClk is toggling and
  -- the reset requested by software is sampled in the RfClk.
  -----------------------------------------------------------------------------

  SeqResetDone : process(ConfigClk)
  begin
    if rising_edge(ConfigClk) then
      -- double-syncs have no sync reset!
      cTriggerAdcResetDone_ms   <= rTriggerAdcReset;
      cTriggerAdcResetDone      <= cTriggerAdcResetDone_ms;
      cTriggerDacResetDone_ms   <= rTriggerDacReset;
      cTriggerDacResetDone      <= cTriggerDacResetDone_ms;
    end if;
  end process;

  -- ADC reset done
  SwAdcResetDone: process(ConfigClk)
  begin
    if rising_edge(ConfigClk) then
      cTriggerAdcResetDlyd <= cTriggerAdcReset;
      -- De-assert reset status on the rising edge of SW ADC reset.
      if cTriggerAdcReset = '1' and cTriggerAdcResetDlyd = '0' then
        cAdcResetDoneSticky <= '0';
      -- Assert and hold the ADC reset status on ADC reset strobe.
      elsif cTriggerAdcResetDone = '1' then
        cAdcResetDoneSticky <= '1';
      end if;
    end if;
  end process SwAdcResetDone;

  -- DAC reset done
  SwDacResetDone: process(ConfigClk)
  begin
    if rising_edge(ConfigClk) then
      cTriggerDacResetDlyd <= cTriggerDacReset;
      -- De-assert reset status on the rising edge of SW DAC reset.
      if cTriggerDacReset = '1' and cTriggerDacResetDlyd = '0' then
        cDacResetDoneSticky <= '0';
      -- Assert and hold the DAC reset status on DAC reset strobe.
      elsif cTriggerDacResetDone = '1' then
        cDacResetDoneSticky <= '1';
      end if;
    end if;
  end process SwDacResetDone;

  -----------------------------------------------------------------------------
  -- rf_reset Instances
  -----------------------------------------------------------------------------
  RfClkResets: process(RfClk)
  begin
    if rising_edge(RfClk) then
      rAdcReset_n <= not (rAdcResetPulse or rTriggerAdcReset);
      rDacReset_n <= not (rDacResetPulse or rTriggerDacReset);
    end if;
  end process RfClkResets;

  RfClk2xResets: process(RfClk2x)
  begin
    if rising_edge(RfClk2x) then
      r2AdcReset_n <= not (rAdcResetPulse or rTriggerAdcReset);
      r2DacReset_n <= not (rDacResetPulse or rTriggerDacReset);
    end if;
  end process RfClk2xResets;

end RTL;
