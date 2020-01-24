-------------------------------------------------------------------------------
--
-- File: DaughterboardRegs.vhd
-- Author: Daniel Jepson
-- Original Project: N310
-- Date: 27 April 2016
--
-------------------------------------------------------------------------------
-- Copyright 2016-2018 Ettus Research, A National Instruments Company
-- SPDX-License-Identifier: LGPL-3.0
-------------------------------------------------------------------------------
--
-- Purpose:
--
-- Register interface to the semi-static control lines for the Mg
-- Daughterboard.
--
-- XML register definition is included below the module.
--
-------------------------------------------------------------------------------

library ieee;
  use ieee.std_logic_1164.all;

library work;
  use work.PkgDaughterboardRegMap.all;
  use work.PkgRegs.all;


entity DaughterboardRegs is
  port(
    -- Async reset. Can be tied low if desired.
    aReset                 : in  boolean;
    -- Sync reset... used in the same places as the async one.
    bReset                 : in  boolean;
    BusClk                 : in  std_logic;

    bRegPortOut            : out RegPortOut_t;
    bRegPortIn             : in  RegPortIn_t;

    -- Slot and DB ID values. These should be tied to constants!
    kDbId                  : in  std_logic_vector(15 downto 0);
    kSlotId                : in  std_logic

  );
end DaughterboardRegs;


architecture RTL of DaughterboardRegs is

  --vhook_sigstart
  --vhook_sigend

  signal bRegPortOutLcl : RegPortOut_t := kRegPortOutZero;

begin


  -- Read Registers : -------------------------------------------------------------------
  -- ------------------------------------------------------------------------------------
  ReadRegisters: process(aReset, BusClk)
  begin
    if aReset then
      bRegPortOutLcl <= kRegPortOutZero;
    elsif rising_edge(BusClk) then
      if bReset then
        bRegPortOutLcl <= kRegPortOutZero;
      else
        -- De-assert strobes
        bRegPortOutLcl.Data <= kRegPortDataZero;

        -- All of these transactions only take one clock cycle, so we do not have to
        -- de-assert the Ready signal (ever).
        bRegPortOutLcl.Ready <= true;

        if RegRead(kDaughterboardId, bRegPortIn) then
          bRegPortOutLcl.Data(kDbIdValMsb downto kDbIdVal) <= kDbId;
          bRegPortOutLcl.Data(kSlotIdVal)                  <= kSlotId;
        end if;

      end if;
    end if;
  end process ReadRegisters;

  -- Local to output
  bRegPortOut <= bRegPortOutLcl;


end RTL;


--XmlParse xml_on
--<regmap name="DaughterboardRegMap">
--  <group name="StaticControl" order="1">
--
--    <register name="DaughterboardId" size="32" offset="0x30" attributes="Readable">
--      <info>
--      </info>
--      <bitfield name="DbIdVal" range="15..0">
--        <info>
--          ID for the DB with which this file is designed to communicate. Matches the DB
--          EEPROM ID.
--        </info>
--      </bitfield>
--      <bitfield name="SlotIdVal" range="16">
--        <info>
--          ID for the Slot this module controls. Options are 0 and 1 for the N310 MB.
--        </info>
--      </bitfield>
--    </register>
--
--  </group>
--
--
--</regmap>
--XmlParse xml_off
