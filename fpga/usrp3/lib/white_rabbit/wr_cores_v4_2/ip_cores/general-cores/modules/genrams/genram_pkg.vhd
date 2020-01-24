-------------------------------------------------------------------------------
-- Title      : Main package file
-- Project    : Generics RAMs and FIFOs collection
-------------------------------------------------------------------------------
-- File       : genram_pkg.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2011-01-25
-- Last update: 2013-10-30
-- Platform   : 
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
--
-- Copyright (c) 2011 CERN
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
-- 2011-01-25  1.0      twlostow        Created
-------------------------------------------------------------------------------


library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

package genram_pkg is

  function f_log2_size (A       : natural) return natural;
  function f_gen_dummy_vec (val : std_logic; size : natural) return std_logic_vector;
  function f_zeros (size : integer) return std_logic_vector;
  function f_check_bounds(x : integer; minx : integer; maxx : integer) return integer;

  type t_generic_ram_init is array (integer range <>, integer range <>) of std_logic;
  type t_ram8_type  is array (integer range <>) of std_logic_vector(7 downto 0);
  type t_ram16_type is array (integer range <>) of std_logic_vector(15 downto 0);
  type t_ram32_type is array (integer range <>) of std_logic_vector(31 downto 0);
  
  -- Single-port synchronous RAM
  component generic_spram
    generic (
      g_data_width               : natural;
      g_size                     : natural;
      g_with_byte_enable         : boolean := false;
      g_init_file                : string  := "none";
      g_addr_conflict_resolution : string  := "dont_care") ;
    port (
      rst_n_i : in  std_logic;
      clk_i   : in  std_logic;
      bwe_i   : in  std_logic_vector((g_data_width+7)/8-1 downto 0):= f_gen_dummy_vec('1', (g_data_width+7)/8);
      we_i    : in  std_logic;
      a_i     : in  std_logic_vector(f_log2_size(g_size)-1 downto 0);
      d_i     : in  std_logic_vector(g_data_width-1 downto 0) := f_gen_dummy_vec('0', g_data_width);
      q_o     : out std_logic_vector(g_data_width-1 downto 0));
  end component;

  component generic_simple_dpram
    generic (
      g_data_width               : natural;
      g_size                     : natural;
      g_with_byte_enable         : boolean := false;
      g_addr_conflict_resolution : string  := "dont_care";
      g_init_file                : string  := "none";
      g_dual_clock               : boolean := true);
    port (
      rst_n_i : in  std_logic := '1';
      clka_i  : in  std_logic;
      bwea_i  : in  std_logic_vector((g_data_width+7)/8 -1 downto 0) := f_gen_dummy_vec('1', (g_data_width+7)/8);
      wea_i   : in  std_logic;
      aa_i    : in  std_logic_vector(f_log2_size(g_size)-1 downto 0);
      da_i    : in  std_logic_vector(g_data_width       -1 downto 0);
      clkb_i  : in  std_logic;
      ab_i    : in  std_logic_vector(f_log2_size(g_size)-1 downto 0);
      qb_o    : out std_logic_vector(g_data_width       -1 downto 0));
  end component;

  component generic_dpram
    generic (
      g_data_width               : natural;
      g_size                     : natural;
      g_with_byte_enable         : boolean := false;
      g_addr_conflict_resolution : string  := "dont_care";
      g_init_file                : string  := "none";
      g_dual_clock               : boolean := true);
    port (
      rst_n_i : in  std_logic := '1';
      clka_i  : in  std_logic;
      bwea_i  : in  std_logic_vector((g_data_width+7)/8-1 downto 0) := f_gen_dummy_vec('1', (g_data_width+7)/8);
      wea_i   : in  std_logic := '0';
      aa_i    : in  std_logic_vector(f_log2_size(g_size)-1 downto 0);
      da_i    : in  std_logic_vector(g_data_width-1 downto 0) := f_gen_dummy_vec('0', g_data_width);
      qa_o    : out std_logic_vector(g_data_width-1 downto 0);
      clkb_i  : in  std_logic;
      bweb_i  : in  std_logic_vector((g_data_width+7)/8-1 downto 0) := f_gen_dummy_vec('1', (g_data_width+7)/8);
      web_i   : in  std_logic := '0';
      ab_i    : in  std_logic_vector(f_log2_size(g_size)-1 downto 0);
      db_i    : in  std_logic_vector(g_data_width-1 downto 0) := f_gen_dummy_vec('0', g_data_width);
      qb_o    : out std_logic_vector(g_data_width-1 downto 0));
  end component;

  component generic_dpram_mixed
    generic (
      g_data_a_width             : natural;
      g_data_b_width             : natural;
      g_size                     : natural;
      g_addr_conflict_resolution : string := "dont_care";
      g_init_file                : string := "none";
      g_dual_clock               : boolean := true);
    port (
      rst_n_i : in  std_logic := '1';
      clka_i  : in  std_logic;
      bwea_i  : in  std_logic_vector((g_data_a_width+7)/8-1 downto 0) := f_gen_dummy_vec('1', (g_data_a_width+7)/8);
      wea_i   : in  std_logic := '0';
      aa_i    : in  std_logic_vector(f_log2_size(g_size)-1 downto 0);
      da_i    : in  std_logic_vector(g_data_a_width-1 downto 0) := f_gen_dummy_vec('0', g_data_a_width);
      qa_o    : out std_logic_vector(g_data_a_width-1 downto 0);
      clkb_i  : in  std_logic;
      bweb_i  : in  std_logic_vector((g_data_b_width+7)/8-1 downto 0) := f_gen_dummy_vec('1', (g_data_b_width+7)/8);
      web_i   : in  std_logic := '0';
      ab_i    : in  std_logic_vector(f_log2_size(g_data_a_width*g_size/g_data_b_width)-1 downto 0);
      db_i    : in  std_logic_vector(g_data_b_width-1 downto 0) := f_gen_dummy_vec('0', g_data_b_width);
      qb_o    : out std_logic_vector(g_data_b_width-1 downto 0));
  end component;

  component generic_async_fifo
    generic (
      g_data_width             : natural;
      g_size                   : natural;
      g_show_ahead             : boolean := false;
      g_with_rd_empty          : boolean := true;
      g_with_rd_full           : boolean := false;
      g_with_rd_almost_empty   : boolean := false;
      g_with_rd_almost_full    : boolean := false;
      g_with_rd_count          : boolean := false;
      g_with_wr_empty          : boolean := false;
      g_with_wr_full           : boolean := true;
      g_with_wr_almost_empty   : boolean := false;
      g_with_wr_almost_full    : boolean := false;
      g_with_wr_count          : boolean := false;
      g_almost_empty_threshold : integer := 0;
      g_almost_full_threshold  : integer := 0);
    port (
      rst_n_i           : in  std_logic := '1';
      clk_wr_i          : in  std_logic;
      d_i               : in  std_logic_vector(g_data_width-1 downto 0);
      we_i              : in  std_logic;
      wr_empty_o        : out std_logic;
      wr_full_o         : out std_logic;
      wr_almost_empty_o : out std_logic;
      wr_almost_full_o  : out std_logic;
      wr_count_o        : out std_logic_vector(f_log2_size(g_size)-1 downto 0);
      clk_rd_i          : in  std_logic;
      q_o               : out std_logic_vector(g_data_width-1 downto 0);
      rd_i              : in  std_logic;
      rd_empty_o        : out std_logic;
      rd_full_o         : out std_logic;
      rd_almost_empty_o : out std_logic;
      rd_almost_full_o  : out std_logic;
      rd_count_o        : out std_logic_vector(f_log2_size(g_size)-1 downto 0));
  end component;


  component generic_sync_fifo
    generic (
      g_data_width             : natural;
      g_size                   : natural;
      g_show_ahead             : boolean := false;
      g_with_empty             : boolean := true;
      g_with_full              : boolean := true;
      g_with_almost_empty      : boolean := false;
      g_with_almost_full       : boolean := false;
      g_with_count             : boolean := false;
      g_almost_empty_threshold : integer := 0;
      g_almost_full_threshold  : integer := 0);
    port (
      rst_n_i        : in  std_logic := '1';
      clk_i          : in  std_logic;
      d_i            : in  std_logic_vector(g_data_width-1 downto 0);
      we_i           : in  std_logic;
      q_o            : out std_logic_vector(g_data_width-1 downto 0);
      rd_i           : in  std_logic;
      empty_o        : out std_logic;
      full_o         : out std_logic;
      almost_empty_o : out std_logic;
      almost_full_o  : out std_logic;
      count_o        : out std_logic_vector(f_log2_size(g_size)-1 downto 0));
  end component;

  component generic_shiftreg_fifo
    generic (
      g_data_width : integer;
      g_size       : integer);
    port (
      rst_n_i       : in  std_logic := '1';
      clk_i         : in  std_logic;
      d_i           : in  std_logic_vector(g_data_width-1 downto 0);
      we_i          : in  std_logic;
      q_o           : out std_logic_vector(g_data_width-1 downto 0);
      rd_i          : in  std_logic;
      full_o        : out std_logic;
      almost_full_o : out std_logic;
      q_valid_o     : out std_logic
      );
  end component;
  
end genram_pkg;

package body genram_pkg is

  function f_log2_size (A : natural) return natural is
  begin
    for I in 1 to 64 loop               -- Works for up to 64 bits
      if (2**I >= A) then
        return(I);
      end if;
    end loop;
    return(63);
  end function f_log2_size;

  function f_gen_dummy_vec (val : std_logic; size : natural) return std_logic_vector is
    variable tmp : std_logic_vector(size-1 downto 0);
  begin
    for i in 0 to size-1 loop
      tmp(i) := val;
    end loop;  -- i
    return tmp;
  end f_gen_dummy_vec;

  function f_zeros(size : integer)
    return std_logic_vector is
  begin
    return std_logic_vector(to_unsigned(0, size));
  end f_zeros;

  function f_check_bounds(x : integer; minx : integer; maxx : integer) return integer is
  begin
    if(x < minx) then
      return minx;
    elsif(x > maxx) then
      return maxx;
    else
      return x;
    end if;
  end f_check_bounds;

end genram_pkg;
