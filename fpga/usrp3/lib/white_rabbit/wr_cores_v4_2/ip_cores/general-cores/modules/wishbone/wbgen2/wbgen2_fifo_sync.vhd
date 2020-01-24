-------------------------------------------------------------------------------
-- Title      : WBGEN components
-- Project    : General Cores
-------------------------------------------------------------------------------
-- File       : wbgen2_fifo_sync.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN
-- Platform   : FPGA-generics
-- Standard   : VHDL '93
-------------------------------------------------------------------------------
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
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

use work.wbgen2_pkg.all;


entity wbgen2_fifo_sync is
  generic (
    g_width      : integer;
    g_size       : integer;
    g_usedw_size : integer);

  port
    (

      clk_i     : in std_logic;
      rst_n_i : in std_logic := '1';

      wr_data_i : in std_logic_vector(g_width-1 downto 0);
      wr_req_i  : in std_logic;

      rd_data_o : out std_logic_vector(g_width-1 downto 0);
      rd_req_i  : in  std_logic;

      wr_empty_o : out std_logic;
      wr_full_o  : out std_logic;
      wr_usedw_o : out std_logic_vector(g_usedw_size -1 downto 0);

      rd_empty_o : out std_logic;
      rd_full_o  : out std_logic;
      rd_usedw_o : out std_logic_vector(g_usedw_size -1 downto 0)

      );
end wbgen2_fifo_sync;


architecture rtl of wbgen2_fifo_sync is


  function f_log2_size (A : natural) return natural is
  begin
    for I in 1 to 64 loop               -- Works for up to 64 bits
      if (2**I > A) then
        return(I-1);
      end if;
    end loop;
    return(63);
  end function f_log2_size;

  component generic_sync_fifo
    generic (
      g_data_width             : natural;
      g_size                   : natural;
      g_show_ahead             : boolean;
      g_with_empty             : boolean;
      g_with_full              : boolean;
      g_with_almost_empty      : boolean;
      g_with_almost_full       : boolean;
      g_with_count             : boolean;
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
  
  signal empty_int : std_logic;
  signal full_int : std_logic;
  signal usedw_int : std_logic_vector(g_usedw_size-1 downto 0);
  
begin

  wrapped_fifo: generic_sync_fifo
    generic map (
      g_data_width             => g_width,
      g_size                   => g_size,
      g_show_ahead             => false,
      g_with_empty             => true,
      g_with_full              => true,
      g_with_almost_empty      => false,
      g_with_almost_full       => false,
      g_with_count             => true)
    port map (
      rst_n_i        => rst_n_i,
      clk_i          => clk_i,
      d_i            => wr_data_i,
      we_i           => wr_req_i,
      q_o            => rd_data_o,
      rd_i           => rd_req_i,
      empty_o        => empty_int,
      full_o         => full_int,
      almost_empty_o => open,
      almost_full_o  => open,
      count_o        => usedw_int);
  

  rd_empty_o <= empty_int;
  rd_full_o <= full_int;
  rd_usedw_o <= usedw_int;

  wr_empty_o <= empty_int;
  wr_full_o <= full_int;
  wr_usedw_o <= usedw_int;
  
  
end rtl;
