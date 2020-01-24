-------------------------------------------------------------------------------
-- Title      : Optical 1000base-X endpoint - IEEE1588 timestamping counter
-- Project    : White Rabbit Switch
-------------------------------------------------------------------------------
-- File       : ep_ts_counter.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2009-06-22
-- Last update: 2017-02-03
-- Platform   : FPGA-generic
-- Standard   : VHDL'87
-------------------------------------------------------------------------------
-- Description: Implementation of dual-edge synchronous counter for
-- PTP timestamping purposes. Falling edge counter always follows the value of
-- rising edge counter. For space reasons only some LSBs of falling edge
-- counter are outputted.
-------------------------------------------------------------------------------
--
-- Copyright (c) 2009 CERN
--
-- This source file is free software; you can redistribute it   
-- and/or modify it under the terms of the GNU Lesser General   
-- Public License as published by the Free Software Foundation; 
-- either version 2.1 of the License, or (at your option) any   
-- later version.                                               
--
-- This source is distributed in the hope that it will be       
-- useful, but WITHOUT ANY WARRANTY; without even the implied   
-- warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      
-- PURPOSE.  See the GNU Lesser General Public License for more 
-- details.                                                     
--
-- You should have received a copy of the GNU Lesser General    
-- Public License along with this source; if not, download it   
-- from http://www.gnu.org/licenses/lgpl-2.1.html
--
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author    Description
-- 2009-06-22  1.0      twlostow  Created
-- 2010-09-01  1.0      twlostow  Added PPS sync
-------------------------------------------------------------------------------


library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.endpoint_private_pkg.all;

entity ep_ts_counter is

  generic (
-- number of bits for rising edge counter
    g_num_bits_r : natural := 27;
-- number of bits to copy for falling edge counter
    g_num_bits_f : natural := 5;
-- initial value of the counter (after reset)
    g_init_value : natural := 124999990;
-- max value of the counter after reaching which it goes back to 0
    g_max_value  : natural := 124999999
    );

  port (
    -- counter clock 
    clk_i      : in  std_logic;
    -- synchronous reset, active LOW
    rst_n_i    : in  std_logic;
-- overflow indicator, active HI when counter value = g_max_value
    overflow_o : out std_logic := '0';
-- counter values (rising and falling edge)
    value_r_o  : out std_logic_vector(g_num_bits_r-1 downto 0);
    value_f_o  : out std_logic_vector(g_num_bits_f-1 downto 0);

-- PPS pulse input, active HI, synchronous to clk_i. Resets the counter when
-- synchronization is enabled
    pps_p_i : in std_logic;

-- Synchronization enable: a HI pulse on this line will cause the counter to be
-- synced to next incoming PPS pulse. After then, sync_done_o will become HI.
    sync_start_p_i : in std_logic;

-- Synchronization done: HI indicates that we've got a PPS pulse we've been waiting for
-- and now the counter value equals 0 when PPS pulse is active.
    sync_done_o : out std_logic
    );

end ep_ts_counter;

architecture syn of ep_ts_counter is

  signal cntr_r           : unsigned(g_num_bits_r-1 downto 0) := (others => '0');
  signal cntr_f           : unsigned (g_num_bits_f-1 downto 0);
  
begin  -- syn


  -- the main TS counter counts on rising clock edge.
  rising_ctr : process (clk_i)
  begin
    if rising_edge(clk_i) then
      if(rst_n_i = '0') then
        cntr_r           <= to_unsigned(g_init_value, g_num_bits_r);
        sync_done_o      <= '1';
      else

-- we've got a synchronization request
        --if(sync_start_p_i = '1') then
        --  sync_in_progress <= '1';
        --  sync_done_o      <= '0';
-- so wait for the next PPS pulse, and then set the PPS counter to 1 (so it
-- equals to 0 when PPS input is active)
        --elsif(sync_in_progress = '1' and pps_p_i = '1') then
        --  sync_in_progress <= '0';
        --  sync_done_o      <= '1';
        --  cntr_r           <= to_unsigned(1, g_num_bits_r);
        --else


        -- increment the counter, reset on overflow
        if(cntr_r = to_unsigned(g_max_value, g_num_bits_r) or pps_p_i = '1') then
          cntr_r <= (others => '0');
        else
          cntr_r <= cntr_r + 1;
        end if;
      end if;

      -- generate overflow output
      if(cntr_r = to_unsigned(0, g_num_bits_r)) then
        overflow_o <= '1';
      else
        overflow_o <= '0';
      end if;
--      end if;
    end if;
  end process;


-- the falling edge counter just copies the rising edge counter half a clock
-- cycle after.
  falling_ctr : process(clk_i)
  begin
    if falling_edge(clk_i)then
      cntr_f <= cntr_r(g_num_bits_f-1 downto 0);
    end if;
  end process;

  -- stupid VHDL type conversion mumbo-jumbo
  value_r_o <= std_logic_vector(cntr_r);
  value_f_o <= std_logic_vector(cntr_f);
end syn;
