------------------------------------------------------------------------------
-- Title      : Etherbone Commit FIFO
-- Project    : Etherbone Core
------------------------------------------------------------------------------
-- File       : eb_fifo.vhd
-- Author     : Wesley W. Terpstra
-- Company    : GSI
-- Created    : 2013-04-29
-- Last update: 2013-04-29
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: A FIFO which can commit/abort a sequence of writes
-------------------------------------------------------------------------------
-- Copyright (c) 2013 GSI
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author          Description
-- 2013-04-08  1.0      terpstra        Created
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.wishbone_pkg.all;
use work.eb_internals_pkg.all;
use work.genram_pkg.all;

-- r_dat_o is valid when r_empty_o=0 (show ahead)
-- w_dat_i is valid when w_push_i =1
-- r_pop_i  affects r_empty_o on the next cycle
-- w_push_i affects w_full_o  on the next cycle
entity eb_commit_fifo is
  generic(
    g_width : natural;
    g_size  : natural);
  port(
    clk_i      : in  std_logic;
    rstn_i     : in  std_logic;
    w_full_o   : out std_logic;
    w_push_i   : in  std_logic;
    w_dat_i    : in  std_logic_vector(g_width-1 downto 0);
    w_commit_i : in  std_logic;
    w_abort_i  : in  std_logic;
    r_empty_o  : out std_logic;
    r_pop_i    : in  std_logic;
    r_dat_o    : out std_logic_vector(g_width-1 downto 0));
end eb_commit_fifo;

architecture rtl of eb_commit_fifo is
  constant c_depth : natural := f_ceil_log2(g_size);
  
  signal r_idx  : unsigned(c_depth downto 0);
  signal w_idx  : unsigned(c_depth downto 0);
  signal e_idx  : unsigned(c_depth downto 0);
  signal r_idx1 : unsigned(c_depth downto 0);
  signal w_idx1 : unsigned(c_depth downto 0);
  
  constant c_low  : unsigned(c_depth-1 downto 0) := (others => '0');
  constant c_high : unsigned(c_depth   downto 0) := '1' & c_low;
  
begin

  ram : generic_simple_dpram
    generic map(
      g_data_width => g_width,
      g_size       => 2**c_depth,
      g_dual_clock => false)
    port map(
      rst_n_i => rstn_i,
      clka_i  => clk_i,
      bwea_i  => (others => '1'),
      wea_i   => w_push_i,
      aa_i    => std_logic_vector(w_idx(c_depth-1 downto 0)),
      da_i    => w_dat_i,
      clkb_i  => clk_i,
      ab_i    => std_logic_vector(r_idx1(c_depth-1 downto 0)),
      qb_o    => r_dat_o);
  
  r_idx1 <= (r_idx+1) when r_pop_i ='1' else r_idx;
  w_idx1 <= (w_idx+1) when w_push_i='1' else w_idx;
  
  main : process(rstn_i, clk_i) is
  begin
    if rstn_i = '0' then
      r_idx     <= (others => '0');
      e_idx     <= (others => '0');
      w_idx     <= (others => '0');
      w_full_o  <= '0';
      r_empty_o <= '1';
    elsif rising_edge(clk_i) then
      r_idx <= r_idx1;
      
      if w_commit_i = '1' then
        e_idx <= w_idx1;
      end if;
      
      if w_abort_i = '1' then
        w_idx <= e_idx;
      else
        w_idx <= w_idx1;
      end if;
      
      -- Compare the newest pointers
      if (w_idx1 xor c_high) = r_idx1 then
        w_full_o <= '1';
      else
        w_full_o <= '0';
      end if;
      
      -- Use the OLD write pointer to prevent read-during-write
      if e_idx = r_idx1 then
        r_empty_o <= '1';
      else
        r_empty_o <= '0';
      end if;
      
    end if;
  end process;

end rtl;
