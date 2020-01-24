-------------------------------------------------------------------------------
--
-- File: PkgRhPersonality.vhd
-- Author: National Instruments
-- Original Project: N32x
-- Date: 15 Dec 2017
--
-------------------------------------------------------------------------------
-- Copyright 2017 Ettus Research, A National Instruments Company
-- SPDX-License-Identifier: GPL-3.0
-------------------------------------------------------------------------------
--
-- Purpose: This package contains constants and helpful functions that enable
--          the FPGA to be compiled with different features.
--
-------------------------------------------------------------------------------

library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;

library work;
  use work.PkgRegs.all;


package PkgRhPersonality is

  -- Rhodium daughterboard ID definition.
  constant kDbId     : integer := 16#152#;
  constant kDbIdSize : integer := 16;


  -- RegPort Address Definitions : ------------------------------------------------------
  --
  -- DB Regs ...
  --
  -- Clocking             Offset: 0x 000    Width: 0x 200
  -- Tdco0                Offset: 0x 200    Width: 0x 200
  -- Tdco1                Offset: 0x 400    Width: 0x 200
  -- Daughterboard Ctrl   Offset: 0x 600    Width: 0x 200
  --                                        Total: 0x2000
  -- JESD 2x - A          Offset: 0x2000    Width: 0x1000
  -- JESD 2x - B          Offset: 0x3000    Width: 0x1000
  --                                        Total: 0x4000
  --                      Total: 0x8000 for two DBs
  -- ------------------------------------------------------------------------------------

  -- A single RegPort runs to the JESD204B Core.
  constant kJesdRegGroupInDbRegs          : RegOffset_t := (kOffset => 16#2000#,   -- 0x2000 to
                                                            kWidth  => 16#1000#);  -- 0x2FFF

  -- DB Regs : --------------------------------------------------------------------------
  constant kClockingOffsetInEndpoint      : RegOffset_t := (kOffset => 16#0000#,   -- 0x0000 to
                                                            kWidth  => 16#0200#);  -- 0x01FF
  constant kTdc0OffsetsInEndpoint         : RegOffset_t := (kOffset => 16#0200#,   -- 0x0200 to
                                                            kWidth  => 16#0200#);  -- 0x03FF
  constant kTdc1OffsetsInEndpoint         : RegOffset_t := (kOffset => 16#0400#,   -- 0x0400 to
                                                            kWidth  => 16#0200#);  -- 0x05FF
  constant kDaughterboardOffsetInEndpoint : RegOffset_t := (kOffset => 16#0600#,   -- 0x0600 to
                                                            kWidth  => 16#0200#);  -- 0x07FF

end package PkgRhPersonality;
