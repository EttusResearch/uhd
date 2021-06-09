--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: rf_reset
--
-- Description:
--
--   Control RFDC, ADC, and DAC resets.
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

entity rf_reset is
  port(
    -- Clocks used in the data path.
    DataClk     : in std_logic;
    PllRefClk   : in std_logic;
    RfClk       : in std_logic;
    RfClk2x     : in std_logic;
    DataClk2x   : in std_logic;

    -- Master resets from the Radio.
    dTimedReset : in std_logic;
    dSwReset    : in std_logic;

    -- Resets outputs.
    dReset_n    : out std_logic := '0';
    d2Reset_n   : out std_logic := '0';
    r2Reset_n   : out std_logic := '0';
    rAxiReset_n : out std_logic := '0';
    rReset_n    : out std_logic := '0'
  );
end rf_reset;


architecture RTL of rf_reset is

  -- POR value for all resets are active high or low.
  signal dResetPulseDly     : std_logic_vector(2 downto 0) := "111";
  signal dResetPulseStretch : std_logic := '1';
  signal pResetPulseStretch : std_logic_vector(1 downto 0) := "11";
  signal pResetPulse_n      : std_logic := '0';
  signal pAxiReset_n        : std_logic := '0';


begin

  -----------------------------------------------------------------------------
  -- Clock Phase Diagram
  -----------------------------------------------------------------------------
  -- Before we look into the details of the clock alignment, here is the clock
  -- frequencies of all the synchronous clocks that is used in the design.
  -- PllRefClk is the reference clock for the FPGA PLL and all other clocks are
  -- derived from PllRefClk. PllRefClk for X410 is ~62.5 MHz
  -- PllRefClk = ~62.5 MHz (Sample clock/48. This is the X410 configuration and
  --                        could be different for other x4xx variants.)
  -- DataClk   = PllRefClk*2
  -- DataClkx2 = PllRefClk*4
  -- RfClk     = PllRefClk*3
  -- RfClkx2   = PllRefClk*6
  -- DataClk = PllRefClk*4 for legacy mode. In legacy mode, we will not use
  -- DataClkx2 as the clock frequency will be too high to close timing.
  -- Five clocks with five different frequencies, all related and occasionally
  -- aligned. Rising edge of all clocks are aligned to the rising edge of
  -- PllRefClk. We will use the rising edge of PllRefClk as the reference to
  -- assert synchronous reset for all clock domains. The synchronous reset
  -- pulse is in the DataClk domain. As we can see from the timing diagram, the
  -- DataClk rising edge is not always aligned to the rising edge of all the
  -- other clocks. But, it is guaranteed that the DataClk will be aligned to
  -- all the other clock on the rising edge of PLL reference clock. In case 1,
  -- the synchronous reset pulse is on the DataClk edge where the data clock is
  -- not aligned to RfClk. We stretch the pulse from DataClk domain and send
  -- the reset out on the rising edge of PllRefClk where all the clocks rising
  -- edge is aligned. In case 2, the synchronous reset is received on the
  -- DataClk cycle where all the clocks are aligned. This is because, in
  -- case 2, the synchronous reset is received on the rising edge of PllRefClk.
  -- For case 1 and case 2, all the output resets are asserted only on the
  -- PllRefClk rising edge to guarantee a known relationship between the resets
  -- in different clock domains.
  --
  --  Alignment    *                       *                       *
  --                ___________             ___________             ___________             ___________             ___________
  --  PllRefClk  __|           |___________|           |___________|           |___________|           |___________|           |
  --                _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _   _
  --    RfClk2x  __| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_| |_
  --                ___     ___     ___     ___     ___     ___     ___     ___     ___     ___     ___     ___     ___     ___
  --      RfClk  __|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|   |___|
  --                __    __    __    __    __    __    __    __    __    __    __    __    __    __    __    __    __    __
  --  DataClk2x  __|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|  |__|
  --                _____       _____       _____       _____       _____       _____       _____       _____       _____
  --    DataClk  __|     |_____|     |_____|     |_____|     |_____|     |_____|     |_____|     |_____|     |_____|     |_____|
  --                           .           :                       :                       :                       :
  -- --------- Case 1 ---------.--         :                       :                       :                       :
  --                           ^           :                       :                       ^                       :
  --          Reset Strobe --> |           :                      Aligned reset strobe  -->|                       :
  --                      ____________     :                       :                       :                       :
  --  dResetPulse________|            |______________________________________              :                       :
  --                                       :    _____________________________________________________________________________
  --  dResetPulseStretch ______________________|                   :
  --                                       :                          ________________________________________________
  --  pResetPulseStretch ____________________________________________|                     :                       :  |___
  --                _________________________________________________________________________                      :
  --  pResetPulse_n                        :                                                 |________________________________
  --                                       :                       :                       :                       :
  -- --------- Case 2 -----------          :                       :                       :                       :
  --                                       ^                       :                       ^                       :
  --                      Reset Strobe --> |                       :                       | <-- Aligned reset strobe
  --                                ____________                   :                       :                       :
  --  dResetPulse(0)       ________|            |______________________________________________________________________________
  --                                            _______________________________________________________________________________
  --  dResetPulseStretch ______________________|                   :
  --                                                                  ________________________________________________________
  --  pResetPulseStretch ____________________________________________|                     :
  --                _________________________________________________________________________
  --  pResetPulse_n                                                                          |________________________________
  -- --------------------------------------------------------------------------

  -----------------------------------------------------------------------------
  -- Implementation
  -----------------------------------------------------------------------------

  -- Since the dTimedReset is asserted only for one DataClk cycle, we need to
  -- stretch the strobe to four DataClk cycles, so the strobe is wide enough to
  -- be sampled by PllRefClk which is four times the DataClk period. Pulse
  -- stretch is done for 4 DataClk periods to support the legacy mode. We also
  -- do a logical OR on resets from software. Software resets are from the
  -- ConfigClock domain which is a slower clock than the PllRefClk. So, we
  -- don't have to stretch the software reset.
  PulseStretch: process(DataClk)
  begin
    if rising_edge(DataClk) then
      dResetPulseDly <= dResetPulseDly(1 downto 0) & (dTimedReset or dSwReset);
      dResetPulseStretch <= '0';
      if (dResetPulseDly /= "000") or dTimedReset = '1' or dSwReset = '1' then
        dResetPulseStretch <= '1';
      end if;
    end if;
  end process PulseStretch;

  -- Strobe reset pulse for 2 PllRefClk period to make sure we have the reset
  -- asserted for longer period. The FIR filter is the only design that
  -- requires reset to be asserted for 2 clock cycles. This requirement is
  -- satisfied with one PllRefClk period. RFDC does not have any AXI stream
  -- reset time requirement. We will reset all designs for two PllRefClk period
  -- just to be on the safer side. The same strategy is used for DAC resets as
  -- well.
  ResetOut: process(PllRefClk)
  begin
    if rising_edge(PllRefClk) then
      pResetPulseStretch <= pResetPulseStretch(0) & dResetPulseStretch;
      pResetPulse_n      <= not (pResetPulseStretch(1) or pResetPulseStretch(0));
    end if;
  end process ResetOut;

  -- We are using PllRefClk as the reference and issuing resets to all the
  -- other clock domains. We are not trying to align all the resets in
  -- different clock domains. We are making sure that all resets will be
  -- asserted with respect to each other at the same time from run to run.
  DataClkReset: process(DataClk)
  begin
      if rising_edge(DataClk) then
        dReset_n <= pResetPulse_n;
      end if;
  end process DataClkReset;

  DataClk2xReset: process(DataClk2x)
  begin
      if rising_edge(DataClk2x) then
        d2Reset_n <= pResetPulse_n;
      end if;
  end process DataClk2xReset;

  Rfclk2xReset: process(RfClk2x)
  begin
      if rising_edge(RfClk2x) then
        r2Reset_n <= pResetPulse_n;
      end if;
  end process Rfclk2xReset;

  RfclkReset: process(RfClk)
  begin
      if rising_edge(RfClk) then
        rReset_n <= pResetPulse_n;
      end if;
  end process RfclkReset;

  -------------------------------------
  -- RF Resets
  -------------------------------------
  -- RFDC resets are asserted only once and it should be done using the reset
  -- from software. This is because we want the RFDC AXI-S interface in reset
  -- until the RfClk is stable. The only way to know if the RfClk is stable is
  -- by reading the lock status of sample clock PLL and MMCM used to generate
  -- all clocks in the signal path. dSwReset is a software reset while is
  -- asserted for a longer period of time and it does not require any pulse
  -- stretch.

  RfdcReset: process(PllRefClk)
  begin
      if rising_edge(PllRefClk) then
        pAxiReset_n <= not dSwReset;
      end if;
  end process RfdcReset;

  RfclkAxiReset: process(RfClk)
  begin
      if rising_edge(RfClk) then
        rAxiReset_n <= pAxiReset_n;
      end if;
  end process RfclkAxiReset;

end RTL;
