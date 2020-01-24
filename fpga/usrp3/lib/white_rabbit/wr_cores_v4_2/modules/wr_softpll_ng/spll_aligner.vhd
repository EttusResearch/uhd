-------------------------------------------------------------------------------
-- Title      : White Rabbit Softcore PLL (new generation) - SoftPLL-ng
-- Project    : White Rabbit
-------------------------------------------------------------------------------
-- File       : spll_aligner.vhd
-- Author     : Tomasz Włostowski
-- Company    : CERN BE-CO-HT
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
--
-- Copyright (c) 2012-2017 CERN
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

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.gencores_pkg.all;

entity spll_aligner is
  generic (
    g_counter_width  : integer := 28;
    g_ref_clock_rate : integer := 125000000;
    g_in_clock_rate  : integer := 10000000;
    g_sample_rate    : integer := 100
    );
  port (
    clk_sys_i : in std_logic;
    clk_in_i  : in std_logic;
    clk_ref_i : in std_logic;

    rst_n_sys_i    : in std_logic;
    rst_n_ref_i    : in  std_logic;
    rst_n_ext_i    : in  std_logic;

    pps_ext_a_i    : in std_logic;
    pps_csync_p1_i : in std_logic;

    sample_cref_o  : out std_logic_vector(g_counter_width-1 downto 0);
    sample_cin_o   : out std_logic_vector(g_counter_width-1 downto 0);
    sample_valid_o : out std_logic;
    sample_ack_i   : in  std_logic
    );

end spll_aligner;

architecture rtl of spll_aligner is

  constant c_div_ticks : integer := g_ref_clock_rate / g_sample_rate;

  signal cnt_ref_bin, cnt_in_bin, cnt_in_bin_x      : unsigned(g_counter_width-1 downto 0);
  signal cnt_in_gray, cnt_in_gray_x, cnt_in_gray_xd : std_logic_vector(g_counter_width-1 downto 0);

  signal cnt_ref_div           : unsigned(g_counter_width-1 downto 0);
  signal pps_ext_p, pps_ext_d0 : std_logic;
  signal ref_div_p             : std_logic;
  signal sample_ready_p        : std_logic;
begin

  p_ref_counter : process(clk_ref_i)
  begin
    if rising_edge(clk_ref_i) then
      if pps_csync_p1_i = '1' or rst_n_ref_i = '0' then
        cnt_ref_bin <= to_unsigned(0, g_counter_width);
      elsif(cnt_ref_bin = g_ref_clock_rate - 1) then
        cnt_ref_bin <= (others => '0');
      else
        cnt_ref_bin <= cnt_ref_bin + 1;
      end if;
    end if;
  end process;

  p_samplerate_divider : process(clk_ref_i)
  begin
    if rising_edge(clk_ref_i) then
      if pps_csync_p1_i = '1' or rst_n_ref_i = '0' then
        ref_div_p   <= '0';
        cnt_ref_div <= to_unsigned(0, g_counter_width);
      elsif (cnt_ref_div = c_div_ticks - 2) then
        ref_div_p   <= '1';
        cnt_ref_div <= cnt_ref_div + 1;
      elsif (cnt_ref_div = c_div_ticks - 1) then
        ref_div_p   <= '0';
        cnt_ref_div <= (others => '0');
      else
        ref_div_p   <= '0';
        cnt_ref_div <= cnt_ref_div + 1;
      end if;
    end if;
  end process;

  p_delay_ext_pps : process(clk_in_i)
  begin
    if rising_edge(clk_in_i) then
      pps_ext_d0 <= pps_ext_a_i;
    end if;
  end process;

  pps_ext_p <= not pps_ext_d0 and pps_ext_a_i;

  p_in_counter : process(clk_in_i)
  begin
    if rising_edge(clk_in_i) then
      if pps_ext_p = '1' or rst_n_ext_i = '0' then
        cnt_in_bin <= to_unsigned(2, g_counter_width);
      elsif(cnt_in_bin = g_in_clock_rate - 1) then
        cnt_in_bin <= (others => '0');
      else
        cnt_in_bin <= cnt_in_bin + 1;
      end if;
    end if;
  end process;

  p_in_bin2gray : process (clk_in_i)
  begin
    if rising_edge(clk_in_i) then
      cnt_in_gray <= f_gray_encode (std_logic_vector(cnt_in_bin));
    end if;
  end process;

  p_sample_difference : process(clk_ref_i)
  begin
    if rising_edge(clk_ref_i) then
      cnt_in_gray_x  <= cnt_in_gray;
      cnt_in_gray_xd <= cnt_in_gray_x;

      if(ref_div_p = '1') then
        sample_cin_o  <= f_gray_decode(cnt_in_gray_xd, 1);
        sample_cref_o <= std_logic_vector (cnt_ref_bin);
      end if;
    end if;
  end process;

  U_sync_sampling : gc_pulse_synchronizer2
    port map (
      clk_in_i    => clk_ref_i,
      rst_in_n_i  => rst_n_ref_i,
      clk_out_i   => clk_sys_i,
      rst_out_n_i => rst_n_sys_i,
      d_p_i       => ref_div_p,
      q_p_o       => sample_ready_p);


  p_gen_sample_valid : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_sys_i = '0' then
        sample_valid_o <= '0';
      else
        if sample_ready_p = '1' then
          sample_valid_o <= '1';
        elsif sample_ack_i = '1' then
          sample_valid_o <= '0';
        end if;
      end if;
    end if;
  end process;

end rtl;



