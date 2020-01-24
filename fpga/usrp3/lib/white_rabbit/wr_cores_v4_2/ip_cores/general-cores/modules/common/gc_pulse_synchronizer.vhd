-------------------------------------------------------------------------------
-- Title      : Pulse synchronizer
-- Project    : General Cores Library 
-------------------------------------------------------------------------------
-- File       : gc_pulse_synchronizer.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2012-01-10
-- Last update: 2012-08-29
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: Full feedback pulse synchronizer (works independently of the
-- input/output clock domain frequency ratio)
-------------------------------------------------------------------------------
--
-- Copyright (c) 2012 CERN / BE-CO-HT
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
-- Date        Version  Author          Description
-- 2012-01-12  1.0      twlostow        Created
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

use work.gencores_pkg.all;

entity gc_pulse_synchronizer is
  
  port (
    -- pulse input clock
    clk_in_i  : in  std_logic;
    -- pulse output clock
    clk_out_i : in  std_logic;
    -- system reset (clk_in_i domain)
    rst_n_i   : in  std_logic;
    -- pulse input ready (clk_in_i domain). When HI, a pulse coming to d_p_i will be
    -- correctly transferred to q_p_o.
    d_ready_o : out std_logic;
    -- pulse input (clk_in_i domain)
    d_p_i     : in  std_logic;
    -- pulse output (clk_out_i domain)
    q_p_o     : out std_logic);

end gc_pulse_synchronizer;

architecture rtl of gc_pulse_synchronizer is

  constant c_sync_stages : integer := 3;

  signal ready, d_p_d0 : std_logic;

  signal in_ext, out_ext : std_logic;
  signal out_feedback    : std_logic;

  signal d_in2out : std_logic_vector(c_sync_stages-1 downto 0);
  signal d_out2in : std_logic_vector(c_sync_stages-1 downto 0);

begin  -- rtl

  process(clk_out_i, rst_n_i)
  begin
    if rst_n_i = '0' then
      d_in2out <= (others => '0');
      out_ext <= '0';
    elsif rising_edge(clk_out_i) then
      d_in2out <= d_in2out(c_sync_stages-2 downto 0) & in_ext;
      out_ext <= d_in2out(c_sync_stages-1);
    end if;
  end process;


  process(clk_in_i, rst_n_i)
  begin
    if rst_n_i = '0' then
      d_out2in <= (others => '0');
    elsif rising_edge(clk_in_i) then
      d_out2in <= d_out2in(c_sync_stages-2 downto 0) & out_ext;
    end if;
  end process;

  out_feedback <= d_out2in(c_sync_stages-1);

  p_input_ack : process(clk_in_i, rst_n_i)
  begin
    if rst_n_i = '0' then
      ready  <= '1';
      in_ext <= '0';
      d_p_d0 <= '0';
    elsif rising_edge(clk_in_i) then

      d_p_d0 <= d_p_i;
      
      if(ready = '1' and d_p_i = '1' and d_p_d0 = '0') then
        in_ext <= '1';
        ready  <= '0';
      elsif(in_ext = '1' and out_feedback = '1') then
        in_ext <= '0';
      elsif(in_ext = '0' and out_feedback = '0') then
        ready <= '1';
      end if;
    end if;
  end process;

  p_drive_output : process(clk_out_i, rst_n_i)
  begin
    if rst_n_i = '0' then
      q_p_o <= '0';
    elsif rising_edge(clk_out_i) then
      q_p_o <= not out_ext and d_in2out(c_sync_stages-1);
    end if;
  end process;

  d_ready_o <= ready;

end rtl;
