-------------------------------------------------------------------------------
-- Title      : Dropping buffer
-- Project    : WR Stramers
-- URL        : http://www.ohwr.org/projects/wr-cores/wiki/WR_Streamers
-------------------------------------------------------------------------------
-- File       : dropping_buffer.vhd
-- Author     : Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
-- Company    : CERN
-- Platform   : FPGA-generics
-- Standard   : VHDL
-- Created    : 2012-11-02
-------------------------------------------------------------------------------
-- Description:
--
-- A FIFO that can be cleared fast
-------------------------------------------------------------------------------
--
-- Copyright (c) 2012-2017 CERN/BE-CO-HT
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
use work.genram_pkg.all; -- needed for f_log2_size()

entity dropping_buffer is
  generic (
    g_size       : integer;
    g_data_width : integer);

  port
    (
      clk_i   : in std_logic;
      rst_n_i : in std_logic;

      d_i        : in  std_logic_vector(g_data_width-1 downto 0);
      d_req_o    : out std_logic;
      d_drop_i   : in  std_logic;
      d_accept_i : in  std_logic;
      d_valid_i  : in  std_logic;

      d_o       : out std_logic_vector(g_data_width-1 downto 0);
      d_valid_o : out std_logic;
      d_req_i   : in  std_logic);


end dropping_buffer;

architecture behavioral of dropping_buffer is

  type    t_mem_array is array(0 to g_size-1) of std_logic_vector(g_data_width-1 downto 0);
  subtype t_counter is unsigned(f_log2_size(g_size)-1 downto 0);

  signal wr_ptr, rd_ptr, boundary : t_counter := (others => '0');
  signal full, empty_comb         : std_logic;
  signal empty_reg                : std_logic := '0';
  signal mem                      : t_mem_array;
  
begin  -- behavioral

  p_counters : process(clk_i)
  begin
    if rising_edge(clk_i) then
      if rst_n_i = '0' then
        wr_ptr   <= (others => '0');
        rd_ptr   <= (others => '0');
        boundary <= (others => '0');
      else
        if(d_accept_i = '1') then
          boundary <= wr_ptr;
        end if;

        if(d_drop_i = '1') then
          wr_ptr <= boundary;
        elsif(d_valid_i = '1' and full = '0') then
          wr_ptr <= wr_ptr + 1;
        end if;

        if (d_req_i = '1' and empty_reg = '0' and empty_comb = '0') then
          rd_ptr <= rd_ptr + 1;
        end if;
      end if;
    end if;
  end process;

  empty_comb <= '1' when (boundary = rd_ptr)   else '0';
  full       <= '1' when (wr_ptr + 1 = rd_ptr) else '0';

  d_req_o <= not full;

  p_empty_reg : process(clk_i)
  begin
    if rising_edge(clk_i) then
      if rst_n_i = '0' then
        empty_reg <= '1';
      else
        empty_reg <= empty_comb;
      end if;
    end if;
  end process;


  p_mem_read : process(clk_i)
  begin
    if rising_edge(clk_i) then
      if(d_req_i = '1' and empty_reg = '0' and empty_comb = '0') then
        d_o       <= mem(to_integer(rd_ptr));
        d_valid_o <= '1';
      else
        d_o       <= (others => '0');
        d_valid_o <= '0';
      end if;
    end if;
  end process;

  p_mem_write : process(clk_i)
  begin
    if rising_edge(clk_i) then
      if(d_valid_i = '1') then
        mem(to_integer(wr_ptr)) <= d_i;
      end if;
    end if;
  end process;
  
end behavioral;
