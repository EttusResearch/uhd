-------------------------------------------------------------------------------
--
-- File: sfp_eeprom.vhd
-- Author: National Instruments
-- Original Project: N310
-- Date: 22 February 2018
--
-------------------------------------------------------------------------------
-- Copyright 2018 Ettus Research, A National Instruments Company
-- SPDX-License-Identifier: LGPL-3.0
-------------------------------------------------------------------------------
--
-- Purpose:
--
-- Responds to I2C reads from the WRC to pretend to be an AXGE SFP module.
--
-------------------------------------------------------------------------------

library IEEE;

use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;


entity sfp_eeprom is

  port (
    -- clock & reset
    clk_i     : in std_logic;
    sfp_scl   : in std_logic;
    sfp_sda_i : in std_logic;
    sfp_sda_o : out std_logic
  );
end sfp_eeprom;


architecture syn of sfp_eeprom is

  signal SfpSdaIn_ms    : std_logic := '0';
  signal SfpSdaIn       : std_logic := '0';
  signal SfpSdaInDel    : std_logic := '0';
  signal SfpScl_ms      : std_logic := '0';
  signal SfpScl         : std_logic := '0';
  signal SfpSclDel      : std_logic := '0';
  signal SfpSdaOut      : std_logic := '1';

  signal ClockRe        : boolean := false;
  signal ClockFe        : boolean := false;
  signal DataFe         : boolean := false;
  signal DataRe         : boolean := false;
  signal Running        : boolean := false;
  signal ReadSel        : boolean := false;

  signal BitCount       : integer range -1 to 7 := 7;
  signal AddrCount      : unsigned(4 downto 0) := (others=>'0');
  signal ByteCount      : integer range 0 to 143;

  type   Eeprom_t       is array(0 to 143) of std_logic_vector(7 downto 0);
  signal Eeprom         : Eeprom_t := (
  X"03", X"04", X"07", X"00", X"00", X"00", X"40", X"00", X"00", X"00", X"00", X"01", X"0d", X"00", X"0a", X"64",
  X"00", X"00", X"00", X"00", X"41", X"78", X"63", X"65", X"6e", X"20", X"50", X"68", X"6f", X"74", X"6f", X"6e",
  X"69", X"63", X"73", X"20", X"00", X"00", X"17", X"2d", X"41", X"58", X"47", X"45", X"2d", X"31", X"32", X"35",
  X"34", X"2d", X"30", X"35", X"33", X"31", X"20", X"20", X"56", X"31", X"2e", X"31", X"05", X"1e", X"00", X"51",
  X"00", X"1a", X"00", X"00", X"41", X"58", X"31", X"36", X"35", X"30", X"30", X"30", X"30", X"36", X"37", X"36",
  X"39", X"20", X"20", X"20", X"31", X"36", X"31", X"32", X"30", X"38", X"20", X"20", X"00", X"00", X"00", X"bd",
  X"45", X"58", X"54", X"52", X"45", X"4d", X"45", X"4c", X"59", X"20", X"43", X"4f", X"4d", X"50", X"41", X"54",
  X"49", X"42", X"4c", X"45", X"20", X"20", X"20", X"20", X"20", X"20", X"20", X"20", X"20", X"20", X"20", X"20",
  X"FF", X"FF", X"FF", X"FF", X"FF", X"FF", X"FF", X"FF", X"FF", X"FF", X"FF", X"FF", X"FF", X"FF", X"FF", X"FF");

  -- AXGE Optical
  --      0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f    0123456789abcdef
  -- 00: 03 04 07 00 00 00 40 00 00 00 00 01 0d 00 0a 64    ???...@....??.?d
  -- 10: 00 00 00 00 41 78 63 65 6e 20 50 68 6f 74 6f 6e    ....Axcen Photon
  -- 20: 69 63 73 20 00 00 17 2d 41 58 47 45 2d 31 32 35    ics ..?-AXGE-125
  -- 30: 34 2d 30 35 33 31 20 20 56 31 2e 31 05 1e 00 51    4-0531  V1.1??.Q
  -- 40: 00 1a 00 00 41 58 31 36 35 30 30 30 30 36 37 36    .?..AX1650000676
  -- 50: 39 20 20 20 31 36 31 32 30 38 20 20 00 00 00 bd    9   161208  ...?
  -- 60: 45 58 54 52 45 4d 45 4c 59 20 43 4f 4d 50 41 54    EXTREMELY COMPAT
  -- 70: 49 42 4c 45 20 20 20 20 20 20 20 20 20 20 20 20    IBLE
  -- 80: ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff ff    ................

begin

  sfp_sda_o <= SfpSdaOut;

  -- Double syncs on the I2C lines
  process(clk_i)
  begin
    if rising_edge(clk_i) then
      SfpSdaIn_ms  <= sfp_sda_i;
      SfpSdaIn     <= SfpSdaIn_ms;
      SfpSdaInDel  <= SfpSdaIn;

      SfpScl_ms    <= sfp_scl;
      SfpScl       <= SfpScl_ms;
      SfpSclDel    <= SfpScl;
    end if;
  end process;

  ClockRe <= SfpScl='1' and SfpSclDel='0';
  ClockFe <= SfpScl='0' and SfpSclDel='1';
  DataRe  <= SfpSdaIn='1' and SfpSdaInDel='0';
  DataFe  <= SfpSdaIn='0' and SfpSdaInDel='1';

  process(clk_i)
  begin
    if rising_edge(clk_i) then
      SfpSdaOut <= '1';
      if (DataFe and SfpScl='1') or not Running then   -- detect start condition
        BitCount <= 7;
        AddrCount <= (others=>'0');
        --ByteCount <= 0;
        Running <= true;
        ReadSel <= false;
      else                            -- if running, then start generating data
        if DataRe and SfpScl='1' then -- detect stop condition
          Running <= false;
        elsif AddrCount<9 then
          if ClockFe then
            AddrCount <= AddrCOunt + 1;
            if AddrCount=8 then
              if SfpSdaIn='0' then      -- reset byte count for write operation
                ByteCount <= 0;
              end if;
              ReadSel <= SfpSdaIn='1';
            end if;
          end if;
        elsif AddrCount=9 then
          SfpSdaOut <= '0';           -- output ACK and hold it for a clock tick
          if ClockFe then
            AddrCount <= AddrCOunt + 1;
          end if;
        else
          if BitCount>=0 then         -- shift out 8 bits if it's a read
            if ReadSel then
              SfpSdaOut <= Eeprom(ByteCount)(BitCount);
            else
              SfpSdaOut <= '1';
            end if;
            if ClockFe then
              BitCount <= BitCOunt - 1;
            end if;
          else                        -- ignore ACK, update byte counter
            SfpSdaOut <= '0';
            if ClockFe then
              if ReadSel then
                if ByteCount<143 then
                  ByteCount <= ByteCount + 1;
                else
                  ByteCount <= 143;    -- play X"FF" for all remaining bytes
                end if;
              end if;
              BitCount <= 7;
            end if;
          end if;
        end if;
      end if;                         -- Running
    end if;
  end process;

end syn;
