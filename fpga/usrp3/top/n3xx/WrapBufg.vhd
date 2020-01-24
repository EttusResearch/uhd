--------------------------------------------------------------------------------
--
-- File: WrapBufg.vhd
-- Author: Robert Atkinson
-- Original Project: RF-RIO
-- Date: 13 January 2012
--
--------------------------------------------------------------------------------
-- Copyright 2012 Ettus Research, A National Instruments Company
-- SPDX-License-Identifier: GPL-3.0
--------------------------------------------------------------------------------
--
-- Purpose: This is a simple wrapper around a BUFG to make instantiating BUFGCTRLs
--          easier without a headache to relearn the port usage each time. This
--          wrapper only supports a single input clock. When disabled, the BUFG
--          output is zero.
--
--------------------------------------------------------------------------------

library ieee;
  use ieee.std_logic_1164.all;

library UNISIM;
  use UNISIM.vcomponents.all;


entity WrapBufg is
  generic(
    -- ClkIn is selected by default if set to true, otherwise zero.
    kEnableByDefault  : boolean := false;
    -- If kIgnore is set to true, then the BUFG will switch inputs whenever
    -- the aCe signal changes as opposed to waiting for ClkIn to transition low.
    kIgnore           : boolean := false;
    -- If aCe is asynchronous to ClkIn, set this generic to true and the select
    -- lines of the BUFGCTRL will be used. If aCe is synchronous to ClkIn, then the CE pins
    -- will be used - note that this requires that setup and hold times be met! If aCe is
    -- synchronous to ClkIn but no timing relationship is really needed then set this to true.
    kEnableIsAsync    : boolean := false
  );
  port(
    -- Input clock
    ClkIn  : in  std_logic;
    -- Enable to the BUFG - this signal is treated either asynchronously
    -- or synchronously based on the value of the kEnableIsAsync generic.
    aCe    : in  std_logic;
    -- Clock output
    ClkOut : out std_logic
  );
end WrapBufg;


architecture rtl of WrapBufg is

  signal iCe0,
         iCe1,
         aSel0,
         aSel1,
         kIgnore0 : std_logic;

begin

  -- From Data Sheet:
  -- The BUFGCTRL is designed to switch between two clock inputs without the possibility of a
  -- glitch. When the presently selected clock transitions from High to Low after S0 and S1
  -- changes, the output is kept Low until the other (to-be-selected) clock has transitioned from
  -- High to Low. Then the new clock starts driving the output. The default configuration for
  -- BUFGCTRL is falling edge sensitive and held at Low prior to the input switching.
  -- BUFGCTRL can also be rising edge sensitive and held at High prior to the input switching
  -- by using the INIT_OUT attribute.
  -- In some applications the conditions previously described are not desirable. Asserting the
  -- IGNORE pins will bypass the BUFGCTRL from detecting the conditions for switching
  -- between two clock inputs. In other words, asserting IGNORE causes the MUX to switch
  -- the inputs at the instant the select pin changes. IGNORE0 causes the output to switch away
  -- from the I0 input immediately when the select pin changes, while IGNORE1 causes the
  -- output to switch away from the I1 input immediately when the select pin changes.
  -- Selection of an input clock requires a "select" pair (S0 and CE0, or S1 and CE1) to be
  -- asserted High. If either S or CE is not asserted High, the desired input will not be selected.
  -- In normal operation, both S and CE pairs (all four select lines) are not expected to be
  -- asserted High simultaneously. Typically only one pin of a "select" pair is used as a select
  -- line, while the other pin is tied High.

  -- If the aCe input is async to the I clock, then use the select pins
  -- of the BUFGCTRL since setup and hold times at the S* pins do not have
  -- to be met. Enable both Select pins if the aCe signal is synchronous to the I clock.
  aSel0 <= aCe     when kEnableIsAsync else
           '1';
  aSel1 <= not aCe when kEnableIsAsync else
           '1';

  -- Only use the CE pins when the input aCe signal is synchronous to the I clock.
  iCe0  <= '1'     when kEnableIsAsync else
           aCe;
  iCe1  <= '1'     when kEnableIsAsync else
           not aCe;

  -- No PkgNiUtilities for me.
  kIgnore0 <= '1' when kIgnore else
              '0';

  --vhook_i BUFGCTRL      GlobalBuffer
  --vhook_a {^IS_(.*)}    '0'
  --vhook_a INIT_OUT      0
  --vhook_a PRESELECT_I0  kEnableByDefault
  --vhook_a PRESELECT_I1  not kEnableByDefault
  --vhook_a O             ClkOut
  --vhook_a CE0           iCe0
  --vhook_a CE1           iCe1
  --vhook_a I0            ClkIn
  --vhook_a I1            '0'
  --vhook_a IGNORE0       kIgnore0
  --vhook_a IGNORE1       '1'
  --vhook_a S0            aSel0
  --vhook_a S1            aSel1
  GlobalBuffer: BUFGCTRL
    generic map (
      INIT_OUT            => 0,                     --integer:=0
      IS_CE0_INVERTED     => '0',                   --bit:='0'
      IS_CE1_INVERTED     => '0',                   --bit:='0'
      IS_I0_INVERTED      => '0',                   --bit:='0'
      IS_I1_INVERTED      => '0',                   --bit:='0'
      IS_IGNORE0_INVERTED => '0',                   --bit:='0'
      IS_IGNORE1_INVERTED => '0',                   --bit:='0'
      IS_S0_INVERTED      => '0',                   --bit:='0'
      IS_S1_INVERTED      => '0',                   --bit:='0'
      PRESELECT_I0        => kEnableByDefault,      --boolean:=false
      PRESELECT_I1        => not kEnableByDefault)  --boolean:=false
    port map (
      O       => ClkOut,    --out std_ulogic
      CE0     => iCe0,      --in  std_ulogic
      CE1     => iCe1,      --in  std_ulogic
      I0      => ClkIn,     --in  std_ulogic
      I1      => '0',       --in  std_ulogic
      IGNORE0 => kIgnore0,  --in  std_ulogic
      IGNORE1 => '1',       --in  std_ulogic
      S0      => aSel0,     --in  std_ulogic
      S1      => aSel1);    --in  std_ulogic


  --vscan Begin Add Explain Clock
  --vscan # These are the outputs of the BUFGCTRLs in the WrapBufg module. VScan
  --vscan # sees the output as glitchy but the clocks are guaranteed to switch
  --vscan # in a glitchless fashion provided that either setup and hold times
  --vscan # are met at the CE pins of the BUFGCTRL (these paths are analyzed by the
  --vscan # tools for timing) or the CE pins are hardwired and the S pins are used
  --vscan # which require no timing relationship to the input clocks.
  --vscan *[WrapBufg]GlobalBuffer/[BUFGCTRL]O
  --vscan End Add Explain Clock


  -- Check that enable lines match up with the generics of the BUFGCTRL
  --vscan vscan_off
  --synthesis translate_off
  SimProcess: process
  begin
    wait for 1 ns;
    if kEnableIsAsync then
      assert kEnableByDefault = (aSel0 = '1')
        report "Initial condition on enable lines do not match between" & LF
               & "the BUFGCTRL generic and the SEL line"
        severity error;
    else
      assert kEnableByDefault = (iCe0 = '1')
        report "Initial condition on enable lines do not match between" & LF
               & "the BUFGCTRL generic and the CE line"
        severity error;
    end if;
    wait;
  end process SimProcess;
  --synthesis translate_on
  --vscan vscan_on

end rtl;
