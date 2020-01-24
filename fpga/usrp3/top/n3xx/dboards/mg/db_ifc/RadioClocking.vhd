-------------------------------------------------------------------------------
--
-- File: RadioClocking.vhd
-- Author: Daniel Jepson
-- Original Project: N310
-- Date: 22 February 2016
--
-------------------------------------------------------------------------------
-- Copyright 2016-2018 Ettus Research, A National Instruments Company
-- SPDX-License-Identifier: LGPL-3.0
-------------------------------------------------------------------------------
--
-- Purpose:
--
-- Instantiates a MMCM to produce 1x, 2x, and 3x versions of the Radio Clock
-- coming from the FPGA input pin. Handles all the buffering for the input clock.
-- Additionally allows the clocks to be turned on and off, and phase shifted.
--
-- NOTE: This module hard-codes the MMCM settings for a SPECIFIC clock rate!
--
-------------------------------------------------------------------------------

library ieee;
  use ieee.std_logic_1164.all;

library unisim;
  use unisim.vcomponents.all;


entity RadioClocking is
  port (
    -- Async reset. Can be tied low if desired.
    aReset                 : in  boolean;
    -- Sync reset... used in the same places as the async one.
    bReset                 : in  boolean;

    -- Should be a always-on clock
    BusClk                 : in  std_logic;

    -- Sync reset to the RadioClkMmcm.
    bRadioClkMmcmReset     : in  std_logic;

    -- Locked indication from the RadioClkMmcm in BusClk and aReset domains.
    bRadioClksValid        : out std_logic;

    bRadioClk1xEnabled     : in  std_logic;
    bRadioClk2xEnabled     : in  std_logic;
    bRadioClk3xEnabled     : in  std_logic;

    -- Phase shift interface for the RadioClkMmcm. PsClk must be <= 200 MHz.
    pPsInc                 : in  std_logic;
    pPsEn                  : in  std_logic;
    PsClk                  : in  std_logic;
    pPsDone                : out std_logic;

    -- Straight from pins. Buffer included in here.
    FpgaClk_n              : in  std_logic;
    FpgaClk_p              : in  std_logic;

    RadioClk1x             : out std_logic;
    RadioClk2x             : out std_logic;
    RadioClk3x             : out std_logic

  );
end RadioClocking;


architecture rtl of RadioClocking is

  --vhook_sigstart
  signal RadioClk1xLcl: std_logic;
  signal RadioClk1xPll: std_logic;
  signal RadioClk2xLcl: std_logic;
  signal RadioClk2xPll: std_logic;
  signal RadioClk3xLcl: std_logic;
  signal RadioClk3xPll: std_logic;
  --vhook_sigend

  signal RadioClkMmcmFeedbackIn,
         RadioClkMmcmFeedbackOut,
         FpgaClkSE,
         aRadioClkMmcmLocked : std_logic;

  signal bRadioClkMmcmLocked_ms,
         bRadioClkMmcmLocked,
         bEnableRadioClkBufgOutputs,
         bEnableRadioClk1xBufgOutput,
         bEnableRadioClk2xBufgOutput,
         bEnableRadioClk3xBufgOutput : std_logic := '0';

  signal aRadioClkMmcmResetInternal  : std_logic := '1';

  attribute ASYNC_REG : string;
  attribute ASYNC_REG of bRadioClkMmcmLocked_ms : signal is "true";
  attribute ASYNC_REG of bRadioClkMmcmLocked    : signal is "true";

begin

  -- Radio Clock Buffering : ------------------------------------------------------------
  --
  -- ------------------------------------------------------------------------------------
  --vhook_i IBUFDS FpgaClkIbufg hidegeneric=true
  --vhook_a I  FpgaClk_p
  --vhook_a IB FpgaClk_n
  --vhook_a O  FpgaClkSE
  FpgaClkIbufg: IBUFDS
    port map (
      O  => FpgaClkSE,  --out std_ulogic
      I  => FpgaClk_p,  --in  std_ulogic
      IB => FpgaClk_n); --in  std_ulogic

  ResetDelay : process(aReset, BusClk)
  begin
    if aReset then
      aRadioClkMmcmResetInternal   <= '1';
    elsif rising_edge(BusClk) then
      if bReset then
        aRadioClkMmcmResetInternal   <= '1';
      else
        -- Delay by 1 to allow the BUFGs to turn off before the MMCM is reset.
        aRadioClkMmcmResetInternal <= bRadioClkMmcmReset;
      end if;
    end if;
  end process ResetDelay;


  RadioClkMmcm: MMCME2_ADV
    generic map(
      COMPENSATION         => "ZHOLD",
      BANDWIDTH            => "OPTIMIZED",
      CLKFBOUT_MULT_F      => 6.000,    -- Feedback
      CLKOUT0_DIVIDE_F     => 6.000,    -- Data Clock 1x, RadioClk1xPll
      CLKOUT1_DIVIDE       => 3,        -- Data Clock 2x, RadioClk2xPll
      CLKOUT2_DIVIDE       => 2,        -- Data Clock 3x, RadioClk3xPll
      CLKOUT3_DIVIDE       => 1,        -- unused
      CLKOUT4_DIVIDE       => 1,        -- unused
      CLKOUT5_DIVIDE       => 1,        -- unused
      CLKOUT6_DIVIDE       => 1,        -- unused
      CLKFBOUT_PHASE       => 0.000,    -- Feedback
      CLKOUT0_PHASE        => 0.000,    -- Data Clock 1x
      CLKOUT1_PHASE        => 0.000,    -- Data Clock 2x
      CLKOUT2_PHASE        => 0.000,    -- Data Clock 3x
      CLKOUT3_PHASE        => 0.000,    -- unused
      CLKOUT4_PHASE        => 0.000,    -- unused
      CLKOUT5_PHASE        => 0.000,    -- unused
      CLKOUT6_PHASE        => 0.000,    -- unused
      CLKOUT0_DUTY_CYCLE   => 0.500,
      CLKOUT1_DUTY_CYCLE   => 0.500,
      CLKOUT2_DUTY_CYCLE   => 0.500,
      CLKOUT3_DUTY_CYCLE   => 0.500,
      CLKOUT4_DUTY_CYCLE   => 0.500,
      CLKOUT5_DUTY_CYCLE   => 0.500,
      CLKOUT6_DUTY_CYCLE   => 0.500,
      DIVCLK_DIVIDE        => 1,
      REF_JITTER1          => 0.010,
      CLKIN1_PERIOD        => 6.510,    -- 153.6 MHz max
      CLKFBOUT_USE_FINE_PS => true,
      CLKOUT0_USE_FINE_PS  => false,
      CLKOUT1_USE_FINE_PS  => false,
      CLKOUT2_USE_FINE_PS  => false,
      CLKOUT3_USE_FINE_PS  => false,
      CLKOUT4_USE_FINE_PS  => false,
      CLKOUT5_USE_FINE_PS  => false,
      CLKOUT6_USE_FINE_PS  => false,
      STARTUP_WAIT         => false,
      CLKOUT4_CASCADE      => false)
    port map (
      CLKINSEL     => '1',
      CLKIN1       => FpgaClkSE,
      CLKIN2       => '0',
      CLKFBIN      => RadioClkMmcmFeedbackIn,
      RST          => aRadioClkMmcmResetInternal,
      PWRDWN       => '0',
      DADDR        => (others => '0'),
      DI           => (others => '0'),
      DWE          => '0',
      DEN          => '0',
      DCLK         => '0',
      DO           => open,
      DRDY         => open,
      PSINCDEC     => pPsInc,
      PSEN         => pPsEn,
      PSCLK        => PsClk,
      PSDONE       => pPsDone,
      CLKOUT0      => RadioClk1xPll,
      CLKOUT0B     => open,
      CLKOUT1      => RadioClk2xPll,
      CLKOUT1B     => open,
      CLKOUT2      => RadioClk3xPll,
      CLKOUT2B     => open,
      CLKOUT3      => open,
      CLKOUT3B     => open,
      CLKOUT4      => open,
      CLKOUT5      => open,
      CLKOUT6      => open,
      CLKFBOUT     => RadioClkMmcmFeedbackOut,
      CLKFBOUTB    => open,
      LOCKED       => aRadioClkMmcmLocked,
      CLKINSTOPPED => open,
      CLKFBSTOPPED => open);

  RadioClkMmcmFeedbackBufg: BUFG
    port map (
      I => RadioClkMmcmFeedbackOut,
      O => RadioClkMmcmFeedbackIn
    );


  -- Only enable the WRAPBUFGs when the MMCM is locked. If the MMCM is ever placed in
  -- reset, we turn off the clocks one cycle before the asynchronous version
  -- (aRadioClkMmcmResetInternal) reaches the MMCM inputs in order to prevent
  -- output glitches.
  CombineEnablesForBuffers : process(aReset, BusClk)
  begin
    if aReset then
      bRadioClkMmcmLocked_ms <= '0';
      bRadioClkMmcmLocked    <= '0';
      bEnableRadioClk1xBufgOutput <= '0';
      bEnableRadioClk2xBufgOutput <= '0';
      bEnableRadioClk3xBufgOutput <= '0';
      bEnableRadioClkBufgOutputs  <= '0';
    elsif rising_edge(BusClk) then
      if bReset then
        bRadioClkMmcmLocked_ms <= '0';
        bRadioClkMmcmLocked    <= '0';
        bEnableRadioClk1xBufgOutput <= '0';
        bEnableRadioClk2xBufgOutput <= '0';
        bEnableRadioClk3xBufgOutput <= '0';
        bEnableRadioClkBufgOutputs  <= '0';
      else
        bRadioClkMmcmLocked_ms <= aRadioClkMmcmLocked;
        bRadioClkMmcmLocked    <= bRadioClkMmcmLocked_ms;

        bEnableRadioClkBufgOutputs <= bRadioClkMmcmLocked and
                                      not bRadioClkMmcmReset;
        bEnableRadioClk1xBufgOutput <= bRadioClk1xEnabled and bEnableRadioClkBufgOutputs;
        bEnableRadioClk2xBufgOutput <= bRadioClk2xEnabled and bEnableRadioClkBufgOutputs;
        bEnableRadioClk3xBufgOutput <= bRadioClk3xEnabled and bEnableRadioClkBufgOutputs;
      end if;
    end if;
  end process CombineEnablesForBuffers;

  bRadioClksValid <= bEnableRadioClkBufgOutputs;

  --vhook_e WrapBufg         RadioClk1xBuf
  --vhook_a kEnableByDefault false
  --vhook_a kIgnore          false
  --vhook_a kEnableIsAsync   true
  --vhook_a ClkIn            RadioClk1xPll
  --vhook_a aCe              bEnableRadioClk1xBufgOutput
  --vhook_a ClkOut           RadioClk1xLcl
  RadioClk1xBuf: entity work.WrapBufg (rtl)
    generic map (
      kEnableByDefault => false,  --boolean:=false
      kIgnore          => false,  --boolean:=false
      kEnableIsAsync   => true)   --boolean:=false
    port map (
      ClkIn  => RadioClk1xPll,                --in  std_logic
      aCe    => bEnableRadioClk1xBufgOutput,  --in  std_logic
      ClkOut => RadioClk1xLcl);               --out std_logic

  --vhook_e WrapBufg         RadioClk2xBuf
  --vhook_a kEnableByDefault false
  --vhook_a kIgnore          false
  --vhook_a kEnableIsAsync   true
  --vhook_a ClkIn            RadioClk2xPll
  --vhook_a aCe              bEnableRadioClk2xBufgOutput
  --vhook_a ClkOut           RadioClk2xLcl
  RadioClk2xBuf: entity work.WrapBufg (rtl)
    generic map (
      kEnableByDefault => false,  --boolean:=false
      kIgnore          => false,  --boolean:=false
      kEnableIsAsync   => true)   --boolean:=false
    port map (
      ClkIn  => RadioClk2xPll,                --in  std_logic
      aCe    => bEnableRadioClk2xBufgOutput,  --in  std_logic
      ClkOut => RadioClk2xLcl);               --out std_logic

  --vhook_e WrapBufg         RadioClk3xBuf
  --vhook_a kEnableByDefault false
  --vhook_a kIgnore          false
  --vhook_a kEnableIsAsync   true
  --vhook_a ClkIn            RadioClk3xPll
  --vhook_a aCe              bEnableRadioClk3xBufgOutput
  --vhook_a ClkOut           RadioClk3xLcl
  RadioClk3xBuf: entity work.WrapBufg (rtl)
    generic map (
      kEnableByDefault => false,  --boolean:=false
      kIgnore          => false,  --boolean:=false
      kEnableIsAsync   => true)   --boolean:=false
    port map (
      ClkIn  => RadioClk3xPll,                --in  std_logic
      aCe    => bEnableRadioClk3xBufgOutput,  --in  std_logic
      ClkOut => RadioClk3xLcl);               --out std_logic


  -- Assign outputs from locals.
  RadioClk1x   <= RadioClk1xLcl;
  RadioClk2x   <= RadioClk2xLcl;
  RadioClk3x   <= RadioClk3xLcl;



end rtl;
