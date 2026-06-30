--
-- Copyright 2026 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: x420_resampler_reset_controller
--
-- Description:
--
--   Generates reset pulses for the fixed resamplers (RX decimator-by-3 and TX interpolator-by-3).
--   Combines SW-triggered reset bits (ConfigClk domain) with the radio-provided ADC/DAC reset
--   pulses (DataClk domain) and outputs DataClk-domain reset pulses for the resampler modules.
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

library WORK;
  use WORK.PkgRFDC_REGS_REGMAP.all;

entity x420_resampler_reset_controller is
  port(
    -- Clocks
    -- Config clock is async to all the others.
    ConfigClk          : in  std_logic;
    DataClk            : in  std_logic;

    -- Master resets from the Radio (DataClk domain)
    dAdcResetPulse     : in  std_logic;
    dDacResetPulse     : in  std_logic;

    -- ADC Resets
    dRxResamplerResetPulseDclk : out std_logic;

    -- DAC Resets
    dTxResamplerResetPulseDclk : out std_logic;

    -- SW Control
    -- Control to initiate resets to RFDC.
    cSoftwareControl   : in  std_logic_vector(31 downto 0)
  );
end x420_resampler_reset_controller;


architecture RTL of x420_resampler_reset_controller is

  component pulse_synchronizer
    generic (
      MODE   : string := "PULSE";
      STAGES : integer := 2);
    port (
      clk_a   : in  std_logic;
      rst_a   : in  std_logic;
      pulse_a : in  std_logic;
      busy_a  : out std_logic;
      clk_b   : in  std_logic;
      pulse_b : out std_logic);
  end component;

  signal cTriggerAdcReset        : std_logic;
  signal cTriggerDacReset        : std_logic;
  signal cTriggerAdcGearboxReset : std_logic;
  signal cTriggerDacGearboxReset : std_logic;

  signal dTriggerAdcReset : std_logic;
  signal dTriggerDacReset : std_logic;

begin

  cTriggerAdcReset        <= cSoftwareControl(kADC_RESET);
  cTriggerDacReset        <= cSoftwareControl(kDAC_RESET);
  cTriggerAdcGearboxReset <= cSoftwareControl(kADC_GEARBOX_RESET);
  cTriggerDacGearboxReset <= cSoftwareControl(kDAC_GEARBOX_RESET);

  -----------------------------------------------------------------------------
  -- Transfer combined SW triggers from ConfigClk to DataClk domain
  -----------------------------------------------------------------------------

  adc_reset_pulse_sync : pulse_synchronizer
    generic map (
      MODE   => "POSEDGE",
      STAGES => 2
    )
    port map (
      clk_a   => ConfigClk,
      rst_a   => '0',
      pulse_a => cTriggerAdcReset or cTriggerAdcGearboxReset,
      busy_a  => open,
      clk_b   => DataClk,
      pulse_b => dTriggerAdcReset
    );

  dac_reset_pulse_sync : pulse_synchronizer
    generic map (
      MODE   => "POSEDGE",
      STAGES => 2
    )
    port map (
      clk_a   => ConfigClk,
      rst_a   => '0',
      pulse_a => cTriggerDacReset or cTriggerDacGearboxReset,
      busy_a  => open,
      clk_b   => DataClk,
      pulse_b => dTriggerDacReset
    );

  -----------------------------------------------------------------------------
  -- Combined DataClk-domain reset pulses
  -----------------------------------------------------------------------------
  DataClkResets: process(DataClk)
  begin
    if rising_edge(DataClk) then
      dRxResamplerResetPulseDclk <= dAdcResetPulse or dTriggerAdcReset;
      dTxResamplerResetPulseDclk <= dDacResetPulse or dTriggerDacReset;
    end if;
  end process DataClkResets;

end RTL;
