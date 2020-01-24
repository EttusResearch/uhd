-------------------------------------------------------------------------------
-- Title      : Dual-port synchronous RAM with byte-write for Xilinx
-------------------------------------------------------------------------------
-- File       : generic_dpram_split.vhd
-- Author     : Grzegorz Daniluk
-- Company    : CERN BE-CO-HT
-- Created    : 2017-02-13
-- Last update: 2017-02-13
-- Platform   : 
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: 
-- This module is 32-bit RAM with byte-write enables. It was created for Xilinx
-- FPGAs, since Xilinx ISE is unable to infer dual-port block-RAM with
-- byte-writes (e.g. based on generic_dpram_sameclock.vhd module). When
-- synthesizing generic_dpram_sameclock with g_with_byte_enable, ISE uses a lot
-- of LUTs to get the byte-write behavior (instead of using the features of BRAM
-- blocks).
--
-- Note:
-- This module is hardcoded to 32-bits and 4 ram modules (ram0-3). It would be
-- much cleaner to have a generic code and using ram(0 to g_data_width/8-1).
-- However, it looks that ISE is not able to initialize 3-d array that would be
-- needed in this case.
-- I.e. this works:
-- shared variable ram0 : t_split_ram := f_file_to_ramtype(0);
-- shared variable ram1 : t_split_ram := f_file_to_ramtype(1);
-- shared variable ram2 : t_split_ram := f_file_to_ramtype(2);
-- shared variable ram3 : t_split_ram := f_file_to_ramtype(3);
--
-- but this doesn't:
-- type t_split_ram_array is array(0 to 3) of t_split_ram;
-- shared variable ram : t_split_ram_array := (f_file_to_ramtype(0),
-- f_file_to_ramtype(1),f_file_to_ramtype(2),
-- f_file_to_ramtype(3));
-- 
-- By "doesn't work" I mean that ISE does not fail during the synthesis, but RAM
-- does not get initialized.
-------------------------------------------------------------------------------
-- Copyright (c) 2017 CERN
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

library work;
use work.genram_pkg.all;
use work.memory_loader_pkg.all;

entity generic_dpram_split is
  generic (
    g_size                     : natural := 16384;
    g_addr_conflict_resolution : string  := "read_first";
    g_init_file                : string  := "";
    g_fail_if_file_not_found   : boolean := true);
  port (
    rst_n_i : in std_logic := '1';
    clk_i   : in std_logic;

    -- Port A
    bwea_i : in  std_logic_vector(3 downto 0);
    wea_i  : in  std_logic;
    aa_i   : in  std_logic_vector(f_log2_size(g_size)-1 downto 0);
    da_i   : in  std_logic_vector(31 downto 0);
    qa_o   : out std_logic_vector(31 downto 0);

    -- Port B
    bweb_i : in  std_logic_vector(3 downto 0);
    web_i  : in  std_logic;
    ab_i   : in  std_logic_vector(f_log2_size(g_size)-1 downto 0);
    db_i   : in  std_logic_vector(31 downto 0);
    qb_o   : out std_logic_vector(31 downto 0));
end generic_dpram_split;

architecture syn of generic_dpram_split is

  constant c_data_width : integer := 32;
  constant c_num_bytes  : integer := (c_data_width+7)/8;

  type t_split_ram is array(0 to g_size-1) of std_logic_vector(7 downto 0);

  impure function f_file_to_ramtype(idx : integer) return t_split_ram is
    variable tmp    : t_split_ram;
    variable mem8   : t_ram8_type(0 to g_size-1);
  begin
    -- If no file was given, there is nothing to convert, just return
    if (g_init_file = "" or g_init_file = "none") then
      tmp := (others=>(others=>'0'));
      return tmp;
    end if;

    mem8 := f_load_mem32_from_file_split(g_init_file, g_size, g_fail_if_file_not_found, idx);
    return t_split_ram(mem8);
  end f_file_to_ramtype;

  impure function f_file_contents return t_meminit_array is
  begin
    return f_load_mem_from_file(g_init_file, g_size, c_data_width, g_fail_if_file_not_found);
  end f_file_contents;

  shared variable ram0 : t_split_ram := f_file_to_ramtype(0);
  shared variable ram1 : t_split_ram := f_file_to_ramtype(1);
  shared variable ram2 : t_split_ram := f_file_to_ramtype(2);
  shared variable ram3 : t_split_ram := f_file_to_ramtype(3);

  signal s_we_a  : std_logic_vector(c_num_bytes-1 downto 0);
  signal s_we_b  : std_logic_vector(c_num_bytes-1 downto 0);
  signal wea_rep : std_logic_vector(c_num_bytes-1 downto 0);
  signal web_rep : std_logic_vector(c_num_bytes-1 downto 0);

begin

  assert (g_addr_conflict_resolution = "read_first" or g_addr_conflict_resolution = "dont_care") 
  report "generic_dpram_split: only read_first and dont_care supported for now" severity failure;

  -- combine byte-write enable with write signals
  wea_rep <= (others => wea_i);
  web_rep <= (others => web_i);
  s_we_a <= bwea_i and wea_rep;
  s_we_b <= bweb_i and web_rep;

  --------------------------------------------------
  -- yes, I know this is 4 times exactly the same code for ram0,1,2,3,
  -- but ISE fails to initialize BRAM when ram is an array (check Note
  -- in the header of this file).
  GEN_RAM0_Port_A: process (clk_i)
  begin
    if rising_edge(clk_i) then
      qa_o(7 downto 0) <= ram0(f_check_bounds(to_integer(unsigned(aa_i)), 0, g_size-1));
      if s_we_a(0) = '1' then
        ram0(f_check_bounds(to_integer(unsigned(aa_i)), 0, g_size-1)) := da_i(7 downto 0);
      end if;
    end if;
  end process;

  GEN_RAM0_Port_B: process (clk_i)
  begin
    if rising_edge(clk_i) then
      qb_o(7 downto 0) <= ram0(f_check_bounds(to_integer(unsigned(ab_i)), 0, g_size-1));
      if s_we_b(0) = '1' then
        ram0(f_check_bounds(to_integer(unsigned(ab_i)), 0, g_size-1)) := db_i(7 downto 0);
      end if;
    end if;
  end process;

  --------------------------------------------------

  GEN_RAM1_Port_A: process (clk_i)
  begin
    if rising_edge(clk_i) then
      qa_o(15 downto 8) <= ram1(f_check_bounds(to_integer(unsigned(aa_i)), 0, g_size-1));
      if s_we_a(1) = '1' then
        ram1(f_check_bounds(to_integer(unsigned(aa_i)), 0, g_size-1)) := da_i(15 downto 8);
      end if;
    end if;
  end process;

  GEN_RAM1_Port_B: process (clk_i)
  begin
    if rising_edge(clk_i) then
      qb_o(15 downto 8) <= ram1(f_check_bounds(to_integer(unsigned(ab_i)), 0, g_size-1));
      if s_we_b(1) = '1' then
        ram1(f_check_bounds(to_integer(unsigned(ab_i)), 0, g_size-1)) := db_i(15 downto 8);
      end if;
    end if;
  end process;

  --------------------------------------------------

  GEN_RAM2_Port_A: process (clk_i)
  begin
    if rising_edge(clk_i) then
      qa_o(23 downto 16) <= ram2(f_check_bounds(to_integer(unsigned(aa_i)), 0, g_size-1));
      if s_we_a(2) = '1' then
        ram2(f_check_bounds(to_integer(unsigned(aa_i)), 0, g_size-1)) := da_i(23 downto 16);
      end if;
    end if;
  end process;

  GEN_RAM2_Port_B: process (clk_i)
  begin
    if rising_edge(clk_i) then
      qb_o(23 downto 16) <= ram2(f_check_bounds(to_integer(unsigned(ab_i)), 0, g_size-1));
      if s_we_b(2) = '1' then
        ram2(f_check_bounds(to_integer(unsigned(ab_i)), 0, g_size-1)) := db_i(23 downto 16);
      end if;
    end if;
  end process;

  --------------------------------------------------

  GEN_RAM3_Port_A: process (clk_i)
  begin
    if rising_edge(clk_i) then
      qa_o(31 downto 24) <= ram3(f_check_bounds(to_integer(unsigned(aa_i)), 0, g_size-1));
      if s_we_a(3) = '1' then
        ram3(f_check_bounds(to_integer(unsigned(aa_i)), 0, g_size-1)) := da_i(31 downto 24);
      end if;
    end if;
  end process;

  GEN_RAM3_Port_B: process (clk_i)
  begin
    if rising_edge(clk_i) then
      qb_o(31 downto 24) <= ram3(f_check_bounds(to_integer(unsigned(ab_i)), 0, g_size-1));
      if s_we_b(3) = '1' then
        ram3(f_check_bounds(to_integer(unsigned(ab_i)), 0, g_size-1)) := db_i(31 downto 24);
      end if;
    end if;
  end process;

  --------------------------------------------------

end syn;
