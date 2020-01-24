--! @file eb_stream_adapt.vhd
--! @brief Adapts the width of a wishbone data stream.
--!
--! Copyright (C) 2013 GSI Helmholtz Centre for Heavy Ion Research GmbH 
--!
--! Fairly simple state-machine
--!
--! @author Wesley W. Terpstra <w.terpstra@gsi.de>
--!
--------------------------------------------------------------------------------
--! This library is free software; you can redistribute it and/or
--! modify it under the terms of the GNU Lesser General Public
--! License as published by the Free Software Foundation; either
--! version 3 of the License, or (at your option) any later version.
--!
--! This library is distributed in the hope that it will be useful,
--! but WITHOUT ANY WARRANTY; without even the implied warranty of
--! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
--! Lesser General Public License for more details.
--!  
--! You should have received a copy of the GNU Lesser General Public
--! License along with this library. If not, see <http://www.gnu.org/licenses/>.
--------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.wishbone_pkg.all;
use work.eb_internals_pkg.all;
use work.genram_pkg.all;

-- Completely breaks/ignores: adr, sel, we, ack, err
entity eb_stream_widen is
  generic(
    g_slave_width  : natural;
    g_master_width : natural);
  port(
    clk_i    : in  std_logic;
    rst_n_i  : in  std_logic;
    slave_i  : in  t_wishbone_slave_in;
    slave_o  : out t_wishbone_slave_out;
    master_i : in  t_wishbone_master_in;
    master_o : out t_wishbone_master_out);
end eb_stream_widen;

architecture rtl of eb_stream_widen is
  
  subtype t_index is unsigned(f_ceil_log2(g_master_width/g_slave_width)-1 downto 0);
  constant c_max  : t_index := to_unsigned(g_master_width/g_slave_width-1, t_index'length);
  constant c_zero : t_index := (others => '0');

  signal r_ack       : std_logic;
  signal r_cyc       : std_logic;
  signal r_drop      : std_logic; -- need to report any cycle line drops
  signal r_stb       : std_logic;
  signal r_idx       : t_index;
  signal r_dat       : t_wishbone_data;
  signal s_stall     : std_logic;
  signal s_cyc_cases : std_logic_vector(2 downto 0);
  
begin

  assert (g_master_width <= c_wishbone_data_width)
  report "g_master_width can not exceed wishbone data width"
  severity error;

  assert (g_master_width mod g_slave_width = 0)
  report "g_slave_width must divide g_master_width"
  severity error;

  assert (g_master_width > g_slave_width)
  report "g_slave_width must be < g_master_width"
  severity error;

  -- Outputs
  slave_o.ack   <= r_ack;
  slave_o.err   <= '0';
  slave_o.stall <= s_stall;
  slave_o.dat   <= (others => '0');
  
  master_o.cyc <= r_cyc;
  master_o.stb <= r_stb;
  master_o.we  <= '1';
  master_o.adr <= (others => '0');
  master_o.dat <= r_dat;
  
  output_sel : for i in t_wishbone_byte_select'range generate
    master_o.sel(i) <= '1' when (i*8 < g_master_width) else '0';
  end generate;

  -- Actual logic
  s_stall <= (r_stb and master_i.stall) or r_drop;
  s_cyc_cases(2) <= r_drop;
  s_cyc_cases(1) <= r_stb;
  s_cyc_cases(0) <= slave_i.cyc;
  
  main : process(clk_i, rst_n_i) is
  begin
    if rst_n_i = '0' then
      r_ack  <= '0';
      r_cyc  <= '0';
      r_drop <= '0';
      r_stb  <= '0';
      r_idx  <= c_max;
      r_dat  <= (others => '0');
    elsif rising_edge(clk_i) then
      r_ack <= slave_i.cyc and slave_i.stb and not s_stall;
      
      -- Make sure we always report cycle line changes
      case s_cyc_cases is
        when "000"  => r_cyc <= '0'; r_drop <= '0'; -- can end cycle immediately
        when "001"  => r_cyc <= '1'; r_drop <= '0'; -- start new cycle
        when "010"  => r_cyc <= '1'; r_drop <= '1'; -- report this!
        when "011"  => r_cyc <= '1'; r_drop <= '0'; -- operation in progress
        when "100"  => r_cyc <= '0'; r_drop <= '0'; -- reported!
        when "101"  => r_cyc <= '0'; r_drop <= '0'; -- reported!
        when others => r_cyc <= '1'; r_drop <= '1'; -- wait for r_stb to fall
      end case;
      
      -- Transfer done?
      if (r_stb and not master_i.stall) = '1' then
        r_stb <= '0';
      end if;
      
      -- Did the slave push us data?
      if slave_i.cyc = '0' then
        r_idx <= c_max; -- reset counter to restore alignment
        -- leave r_stb/r_dat unchanged!
      elsif (slave_i.stb and not s_stall) = '1' then
        r_dat(((to_integer(r_idx)+1)*g_slave_width)-1 downto to_integer(r_idx)*g_slave_width) <= 
          slave_i.dat(g_slave_width-1 downto 0);
        if r_idx = c_zero then
          r_idx <= c_max;
          r_stb <= '1';
        else
          r_idx <= r_idx - 1;
        end if;
      end if;
      
    end if;
  end process;
  
end rtl;
