-------------------------------------------------------------------------------
--
-- File: tb_sfp_eeprom.vhd
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
-- Testbench for sfp_eeprom.vhd
--
-------------------------------------------------------------------------------

library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;

entity tb_sfp_eeprom is
end tb_sfp_eeprom;

architecture test of tb_sfp_eeprom is

  constant kHalfPeriod    : time := 25 ns;

  signal clk_i     : std_logic := '1';
  signal sfp_scl   : std_logic := '1';
  signal sfp_sda_i : std_logic := '1';
  signal sfp_sda_o : std_logic := '1';


  signal StopSim : boolean := false;

  -- This procedure waits for X rising edges of Clk
  procedure ClkWait (X : integer := 1) is
  begin
    for i in 1 to X loop
      wait until rising_edge(clk_i);
    end loop;
  end procedure ClkWait;

  component sfp_eeprom
  port (
    clk_i     : in std_logic;
    sfp_scl   : in std_logic;
    sfp_sda_i : in std_logic;
    sfp_sda_o : out std_logic);
  end component;

begin

  -- Set up the clock(s)
  clk_i <= not clk_i after kHalfPeriod when not StopSim else '0';


  DUT: sfp_eeprom
    port map (
      clk_i => clk_i,
      sfp_scl => sfp_scl,
      sfp_sda_i => sfp_sda_i,
      sfp_sda_o => sfp_sda_o );


  Sim: process
  begin
    for K in 0 to 1 loop
      wait for 210 ns;
      -- send start condition
      sfp_sda_i <= '0';
      wait for 210 ns;
      sfp_scl <= '0';
      wait for 210 ns;
      for I in 0 to 150 loop
        for J in 0 to 8 loop
          sfp_scl <= '1';
          wait for 210 ns;
          sfp_scl <= '0';
          wait for 210 ns;
        end loop;
        wait for 210 ns;
      end loop;
      sfp_scl <= '1';
      wait for 210 ns;
      sfp_sda_i <= '1';
      wait for 210 ns;
    end loop;
    StopSim <= true;
    wait;
  end process Sim;


end test;
