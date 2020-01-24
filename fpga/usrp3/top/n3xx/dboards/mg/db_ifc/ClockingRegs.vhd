-------------------------------------------------------------------------------
--
-- File: ClockingRegs.vhd
-- Author: Daniel Jepson
-- Original Project: N310
-- Date: 17 March 2016
--
-------------------------------------------------------------------------------
-- Copyright 2016-2018 Ettus Research, A National Instruments Company
-- SPDX-License-Identifier: LGPL-3.0
-------------------------------------------------------------------------------
--
-- Purpose:
--
-- Register access to the control/status bits and interfaces for the
-- RadioClocking module.
--
-- XML register definition is included below the module.
--
-------------------------------------------------------------------------------

library ieee;
  use ieee.std_logic_1164.all;

library work;
  use work.PkgClockingRegMap.all;
  use work.PkgRegs.all;


entity ClockingRegs is
  port(
    -- Async reset. Can be tied low if desired.
    aReset                 : in  boolean;
    -- Sync reset... used in the same places as the async one.
    bReset                 : in  boolean;
    -- Register Bus Clock -- this module connects the BusClk to PsClk, so it's limited
    -- to 200 MHz!
    BusClk                 : in  std_logic;

    bRegPortOut            : out RegPortOut_t;
    bRegPortIn             : in  RegPortIn_t;

    -- Phase shift interface to the RadioClkMmcm.
    -- There is a reset crossing here between the MMCM reset and aReset. The outgoing
    -- crossing is safe because (a) the enable signal driven to the MMCM is a strobe-only
    -- signal and (b) this interface should only be used when the MMCM is not in reset
    -- (SW waits for the MMCM to be out of reset and locked before using this interface).
    -- The only input signal, pPsDone, is double-synced in this file before being used.
    -- This is OK (even though it is a strobe signal) because there is only a reset
    -- crossing and not a clock domain crossing.
    pPsInc            : out std_logic;
    pPsEn             : out std_logic;
    pPsDone           : in  std_logic;

    -- PsClk is driven directly by BusClk, so p = b in the logic below!
    PsClk             : out std_logic;

    -- Sync reset strobes from the register bus to the RadioClkMmcm.
    bRadioClkMmcmReset  : out  std_logic;
    -- Status of RadioClk MMCM lock to register bus.
    aRadioClksValid     : in  std_logic;

    bRadioClk1xEnabled  : out std_logic;
    bRadioClk2xEnabled  : out std_logic;
    bRadioClk3xEnabled  : out std_logic;

    bJesdRefClkPresent  : in std_logic
  );
end ClockingRegs;


architecture RTL of ClockingRegs is

  --vhook_sigstart
  --vhook_sigend

  signal bRadioClkMmcmResetInt : std_logic := '1';

  signal bRegPortOutLcl : RegPortOut_t := kRegPortOutZero;

  signal bPsDone,
         bPsEn,
         bPsInc,
         pPsDoneDs_ms,
         pPsDoneDs : std_logic := '0';

  signal bRadioClk1xEnabledInt,
         bRadioClk2xEnabledInt,
         bRadioClk3xEnabledInt,
         bRadioClksValid_ms,
         bRadioClksValid : std_logic := '0';

  attribute ASYNC_REG : string;
  attribute ASYNC_REG of bRadioClksValid_ms : signal is "true";
  attribute ASYNC_REG of bRadioClksValid    : signal is "true";
  attribute ASYNC_REG of pPsDoneDs_ms       : signal is "true";
  attribute ASYNC_REG of pPsDoneDs          : signal is "true";

begin

  -- Locals to outputs.
  PsClk  <= BusClk;
  pPsInc <= bPsInc;
  pPsEn  <= bPsEn;

  bRadioClkMmcmReset <= bRadioClkMmcmResetInt;

  bRadioClk1xEnabled <= bRadioClk1xEnabledInt;
  bRadioClk2xEnabled <= bRadioClk2xEnabledInt;
  bRadioClk3xEnabled <= bRadioClk3xEnabledInt;


  -- Write Registers : ------------------------------------------------------------------
  -- ------------------------------------------------------------------------------------
  WriteRegisters: process(aReset, BusClk)
  begin
    if aReset then
      bRadioClkMmcmResetInt <= '1';
      bPsInc <= '0';
      bPsEn  <= '0';
      bRadioClk1xEnabledInt <= '0';
      bRadioClk2xEnabledInt <= '0';
      bRadioClk3xEnabledInt <= '0';
    elsif rising_edge(BusClk) then

      if bReset then
        bRadioClkMmcmResetInt <= '1';
        bPsInc <= '0';
        bPsEn  <= '0';
        bRadioClk1xEnabledInt <= '0';
        bRadioClk2xEnabledInt <= '0';
        bRadioClk3xEnabledInt <= '0';
      else
        -- Clear strobe
        bPsEn  <= '0';

        if RegWrite(kPhaseShiftControl, bRegPortIn)  then
          if bRegPortIn.Data(kPsInc) = '1' then
            bPsInc <= '1';
            bPsEn  <= '1';
          elsif bRegPortIn.Data(kPsDec) = '1' then
            bPsInc <= '0';
            bPsEn  <= '1';
          end if;
        end if;

        if RegWrite(kRadioClkMmcm, bRegPortIn) then
          -- Set/Clear pair
          if bRegPortIn.Data(kRadioClkMmcmResetSet) = '1' then
            bRadioClkMmcmResetInt <= '1';
          elsif bRegPortIn.Data(kRadioClkMmcmResetClear) = '1' then
            bRadioClkMmcmResetInt <= '0';
          end if;
        end if;

        if RegWrite(kRadioClkEnables, bRegPortIn) then
          bRadioClk1xEnabledInt <= bRegPortIn.Data(kRadioClk1xEnabled);
          bRadioClk2xEnabledInt <= bRegPortIn.Data(kRadioClk2xEnabled);
          bRadioClk3xEnabledInt <= bRegPortIn.Data(kRadioClk3xEnabled);
        end if;

      end if;
    end if;
  end process WriteRegisters;


  DoubleSyncs : process (aReset, BusClk)
  begin
    if aReset then
      bRadioClksValid_ms <= '0';
      bRadioClksValid    <= '0';
      pPsDoneDs_ms <= '0';
      pPsDoneDs    <= '0';
    elsif rising_edge(BusClk) then
      -- No sync reset on double-syncs (however there are default assignments above)!
      bRadioClksValid_ms <= aRadioClksValid;
      bRadioClksValid    <= bRadioClksValid_ms;
      pPsDoneDs_ms <= pPsDone;
      pPsDoneDs    <= pPsDoneDs_ms;
    end if;
  end process;


  -- Read Registers : -------------------------------------------------------------------
  -- ------------------------------------------------------------------------------------
  ReadRegisters: process(aReset, BusClk)
  begin
    if aReset then
      bRegPortOutLcl   <= kRegPortOutZero;
      bPsDone          <= '0';
    elsif rising_edge(BusClk) then

      if bReset then
        bRegPortOutLcl   <= kRegPortOutZero;
        bPsDone          <= '0';
      else
        -- Deassert strobes
        bRegPortOutLcl.Data <= kRegPortDataZero;

        -- All of these transactions only take one clock cycle, so we do not have to
        -- de-assert the Ready signal (ever).
        bRegPortOutLcl.Ready <= true;

        -- Process the returned data from the phase shifter in the MMCM. Note that even
        -- though the prefixes are different (p and b), we drive the PsClk from the BusClk
        -- so this "crossing" is actually safe. Whenever the Done signal asserts (pPsDone -
        -- pay attention to the prefix!) from the MMCM, we set a sticky bit to tell SW
        -- that the shift operation is complete.
        --
        -- However, if pPsDone asserts at the same time that SW tries to read the register,
        -- we should accurately report that the operation is indeed complete and then NOT
        -- store the sticky (since it has already been read by SW). If a read does not come
        -- through at the same time pPsDone is asserted, then we store the done state as a
        -- sticky, bPsDone, which is only cleared by a read to this register.
        if RegRead(kPhaseShiftControl, bRegPortIn) then
          -- The phase shift is always enabled for the feedback clock in RadioClocking.vhd
          bRegPortOutLcl.Data(kPsEnabledForFdbClk) <= '1';
          bRegPortOutLcl.Data(kPsDone) <= bPsDone or pPsDoneDs;
          bPsDone            <= '0';
        elsif pPsDoneDs = '1' then
          bPsDone <= '1';
        end if;

        if RegRead(kRadioClkMmcm, bRegPortIn) then
          bRegPortOutLcl.Data(kRadioClkMmcmLocked) <= bRadioClksValid;
        end if;

        if RegRead(kRadioClkEnables, bRegPortIn) then
          bRegPortOutLcl.Data(kRadioClk1xEnabled) <= bRadioClk1xEnabledInt;
          bRegPortOutLcl.Data(kRadioClk2xEnabled) <= bRadioClk2xEnabledInt;
          bRegPortOutLcl.Data(kRadioClk3xEnabled) <= bRadioClk3xEnabledInt;
        end if;

        if RegRead(kMgtRefClkStatus, bRegPortIn) then
          bRegPortOutLcl.Data(kJesdRefClkPresent) <= bJesdRefClkPresent;
        end if;

      end if;
    end if;
  end process ReadRegisters;

  -- Local to output
  bRegPortOut <= bRegPortOutLcl;


end RTL;


--XmlParse xml_on
--<regmap name="ClockingRegMap">
--  <group name="ClockingRegs">
--
--    <register name="RadioClkMmcm" size="32" offset="0x20" attributes="Readable|Writable">
--      <info>
--      </info>
--      <bitfield name="RadioClkMmcmLocked" range="4">
--        <info>
--          Reflects the locked status of the MMCM. '1' = locked. This bit is only valid
--          when the MMCM reset is de-asserted. Read-only.
--        </info>
--      </bitfield>
--      <bitfield name="RadioClkMmcmResetClear" range="1" attributes="Strobe">
--        <info>
--          Controls the reset to the Radio Clock MMCM. Strobe this bit to de-assert the
--          reset to the MMCM. Default is reset asserted. Write-only.
--        </info>
--      </bitfield>
--      <bitfield name="RadioClkMmcmResetSet" range="0" attributes="Strobe">
--        <info>
--          Controls the reset to the Radio Clock MMCM. Strobe this bit to assert the
--          reset to the MMCM. Default is reset asserted. Write-only.
--        </info>
--      </bitfield>
--    </register>
--
--    <register name="PhaseShiftControl" size="32" offset="0x24" attributes="Readable|Writable">
--      <info>
--        Phase Shift for RadioClkMmcm.
--      </info>
--      <bitfield name="PsDone" range="28">
--        <info>
--          This bit should set after a shift operation successfully completes.
--          Reading this register will clear this bit. Read-only.
--        </info>
--      </bitfield>
--      <bitfield name="PsInc" range="0" attributes="Strobe">
--        <info>
--          Strobe this bit to increment the phase. This bit is self-clearing and will
--          always return '0' when read. If PsInc and PsDec are asserted together,
--          the phase will increment.
--        </info>
--      </bitfield>
--      <bitfield name="PsDec" range="4" attributes="Strobe">
--        <info>
--          Strobe this bit to decrement the phase. This bit is self-clearing and will
--          always return '0' when read. If PsInc and PsDec are asserted together,
--          the phase will increment.
--        </info>
--      </bitfield>
--      <bitfield name="PsEnabledForFdbClk" range="16">
--        <info>
--          Read-only.
--        </info>
--      </bitfield>
--    </register>
--
--    <register name="RadioClkEnables" size="32" offset="0x28" attributes="Readable|Writable">
--      <info>
--      </info>
--      <bitfield name="RadioClk3xEnabled" range="8">
--        <info>
--          Set to '1' to enable the clock. Default disabled = '0'.
--          Do so ONLY after the MMCM is out of reset and locked!
--        </info>
--      </bitfield>
--      <bitfield name="RadioClk2xEnabled" range="4">
--        <info>
--          Set to '1' to enable the clock. Default disabled = '0'.
--          Do so ONLY after the MMCM is out of reset and locked!
--        </info>
--      </bitfield>
--      <bitfield name="RadioClk1xEnabled" range="0">
--        <info>
--          Set to '1' to enable the clock. Default disabled = '0'.
--          Do so ONLY after the MMCM is out of reset and locked!
--        </info>
--      </bitfield>
--    </register>
--
--    <register name="MgtRefClkStatus" size="32" offset="0x30" attributes="Readable">
--      <info>
--      </info>
--      <bitfield name="JesdRefClkPresent" range="0">
--        <info>
--          Live indicator of the MGT Reference Clock toggling and within expected
--          frequency limits. If this bit is de-asserted, then the JESD204b core will
--          not function correctly!
--        </info>
--      </bitfield>
--    </register>
--
--  </group>
--
--</regmap>
--XmlParse xml_off
