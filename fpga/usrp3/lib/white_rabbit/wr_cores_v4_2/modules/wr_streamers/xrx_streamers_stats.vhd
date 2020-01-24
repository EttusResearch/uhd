-------------------------------------------------------------------------------
-- Title      : WR Recpetion Streamers statistics
-- Project    : White Rabbit Streamers
-- URL        : http://www.ohwr.org/projects/wr-cores/wiki/WR_Streamers
-------------------------------------------------------------------------------
-- File       : xrx_streamers_stats.vhd
-- Author     : Maciej Lipinski
-- Company    : CERN
-- Created    : 2017-04-19
-- Platform   : FPGA-generics
-- Standard   : VHDL
-------------------------------------------------------------------------------
-- Description:
-- This module provies the reception portion of statistics and
-- allows to snapshot their values. See xrtx_streamers_stats for
-- more detailed description.
-------------------------------------------------------------------------------
--
-- Copyright (c) 2016 CERN/BE-CO-HT
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
---------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

library work;
use work.wishbone_pkg.all;  -- needed for t_wishbone_slave_in, etc
use work.streamers_pkg.all; -- needed for streamers
use work.wr_fabric_pkg.all; -- neede for :t_wrf_source_in, etc
use work.wrcore_pkg.all;    -- needed for t_generic_word_array
-- use work.wr_transmission_wbgen2_pkg.all;

entity xrx_streamers_stats is
  
  generic (
    -- Width of frame counters
    g_cnt_width            : integer := 50; -- min:15, max:64, 50 bits should be ok for 50 years
    g_acc_width            : integer := 64  -- max value 64
    );
  port (
    clk_i                  : in std_logic;
    rst_n_i                : in std_logic;

    -- input signals from streamers
    rcvd_frame_i           : in std_logic;
    lost_block_i           : in std_logic;
    lost_frame_i           : in std_logic;
    lost_frames_cnt_i      : in std_logic_vector(14 downto 0);
    rcvd_latency_i         : in  std_logic_vector(27 downto 0);
    rcvd_latency_valid_i   : in  std_logic;
    tm_time_valid_i        : in  std_logic;

    snapshot_ena_i         : in std_logic := '0';
    reset_stats_i          : in std_logic;
    ----------------------- statistics ----------------------------------------
    -- output statistics: tx/rx counters
    rcvd_frame_cnt_o       : out std_logic_vector(g_cnt_width-1 downto 0);
    lost_frame_cnt_o       : out std_logic_vector(g_cnt_width-1 downto 0);
    lost_block_cnt_o       : out std_logic_vector(g_cnt_width-1 downto 0);
    -- output statistics: latency
    latency_cnt_o          : out std_logic_vector(g_cnt_width-1 downto 0);
    latency_acc_overflow_o : out std_logic;
    latency_acc_o          : out std_logic_vector(g_acc_width-1  downto 0);
    latency_max_o          : out std_logic_vector(27  downto 0);
    latency_min_o          : out std_logic_vector(27  downto 0)
    );

end xrx_streamers_stats;
  
architecture rtl of xrx_streamers_stats is


  signal rcvd_frame_cnt    : unsigned(g_cnt_width-1  downto 0);
  signal lost_frame_cnt    : unsigned(g_cnt_width-1  downto 0);
  signal lost_block_cnt    : unsigned(g_cnt_width-1  downto 0);
  signal latency_cnt       : unsigned(g_cnt_width-1  downto 0);

  signal latency_max       : std_logic_vector(27  downto 0);
  signal latency_min       : std_logic_vector(27  downto 0);
  signal latency_acc       : unsigned(g_acc_width-1+1  downto 0);
  signal latency_acc_overflow: std_logic;

  -- snaphsot
  signal rcvd_frame_cnt_d1    : unsigned(g_cnt_width-1  downto 0);
  signal lost_frame_cnt_d1    : unsigned(g_cnt_width-1  downto 0);
  signal lost_block_cnt_d1    : unsigned(g_cnt_width-1  downto 0);
  signal latency_cnt_d1       : unsigned(g_cnt_width-1  downto 0);

  signal latency_max_d1       : std_logic_vector(27  downto 0);
  signal latency_min_d1       : std_logic_vector(27  downto 0);
  signal latency_acc_d1       : unsigned(g_acc_width-1+1  downto 0);
  signal latency_acc_overflow_d1: std_logic;

  signal snapshot_ena_d1   : std_logic;

begin

  -------------------------------------------------------------------------------------------
  -- frame/block statistics, i.e. lost, sent, received
  -------------------------------------------------------------------------------------------
  -- process that counts: receved/lost frames
  p_cnts: process(clk_i)
  begin
    if rising_edge(clk_i) then
      if (rst_n_i = '0' or reset_stats_i = '1') then
        rcvd_frame_cnt        <= (others => '0');
        lost_frame_cnt        <= (others => '0');
        lost_block_cnt        <= (others => '0');
      else
        -- count received frames
        if(rcvd_frame_i = '1') then
          rcvd_frame_cnt <= rcvd_frame_cnt + 1;
        end if;
        -- count lost frames
        if(lost_frame_i = '1') then
          lost_frame_cnt <= lost_frame_cnt + resize(unsigned(lost_frames_cnt_i),lost_frame_cnt'length);
        end if;
        -- count lost blocks
        if(lost_block_i = '1') then
          lost_block_cnt <= lost_block_cnt + 1;
        end if;
      end if;
    end if;
  end process;

  -------------------------------------------------------------------------------------------
  -- latency statistics
  -------------------------------------------------------------------------------------------
  p_latency_stats: process(clk_i)
  begin
    if rising_edge(clk_i) then
      if (rst_n_i = '0' or reset_stats_i = '1') then
        latency_max            <= (others => '0');
        latency_min            <= (others => '1');
        latency_acc            <= (others => '0');
        latency_cnt            <= (others => '0');
        latency_acc_overflow   <= '0';
      else
        if(rcvd_latency_valid_i = '1' and tm_time_valid_i = '1') then
          if(latency_max < rcvd_latency_i) then
            latency_max <= rcvd_latency_i;
          end if;
          if(latency_min > rcvd_latency_i) then
            latency_min <= rcvd_latency_i;
          end if;
          if(latency_acc(g_acc_width) ='1') then
            latency_acc_overflow   <= '1';
          end if;
          latency_cnt <= latency_cnt + 1;
          latency_acc <= latency_acc + resize(unsigned(rcvd_latency_i),latency_acc'length);
        end if;
      end if;
    end if;
  end process;

  -------------------------------------------------------------------------------------------
  -- snapshot 
  -------------------------------------------------------------------------------------------
  -- snapshot is used to expose to user coherent value, so that the count for accumulated
  -- latency is coherent with the accumulated latency and the average can be accurately 
  -- calculated
  -------------------------------------------------------------------------------------------

  -- snapshot
  p_stats_snapshot: process(clk_i)
  begin
    if rising_edge(clk_i) then
      if (rst_n_i = '0') then
         snapshot_ena_d1         <= '0';
         rcvd_frame_cnt_d1       <= (others=>'0');
         lost_frame_cnt_d1       <= (others=>'0');
         lost_block_cnt_d1       <= (others=>'0');
         latency_cnt_d1          <= (others=>'0');

         latency_max_d1          <= (others=>'0');
         latency_min_d1          <= (others=>'0');
         latency_acc_d1          <= (others=>'0');
         latency_acc_overflow_d1 <= '0';
      else
        if(snapshot_ena_i = '1' and snapshot_ena_d1 = '0') then
         rcvd_frame_cnt_d1       <= rcvd_frame_cnt;
         lost_frame_cnt_d1       <= lost_frame_cnt;
         lost_block_cnt_d1       <= lost_block_cnt;
         latency_cnt_d1          <= latency_cnt;

         latency_max_d1          <= latency_max;
         latency_min_d1          <= latency_min;
         latency_acc_d1          <= latency_acc;
         latency_acc_overflow_d1 <= latency_acc_overflow;
        end if;
        snapshot_ena_d1 <= snapshot_ena_i;
      end if;
    end if;
  end process;

  -------------------------------------------------------------------------------------------
  -- snapshot or current value
  -------------------------------------------------------------------------------------------
  rcvd_frame_cnt_o       <= std_logic_vector(rcvd_frame_cnt_d1) when (snapshot_ena_d1 = '1') else
                            std_logic_vector(rcvd_frame_cnt);
  lost_frame_cnt_o       <= std_logic_vector(lost_frame_cnt_d1) when (snapshot_ena_d1 = '1') else
                            std_logic_vector(lost_frame_cnt);
  lost_block_cnt_o       <= std_logic_vector(lost_block_cnt_d1) when (snapshot_ena_d1 = '1') else
                            std_logic_vector(lost_block_cnt);
  latency_max_o          <= latency_max_d1 when (snapshot_ena_d1 = '1') else
                            latency_max;
  latency_min_o          <= latency_min_d1 when (snapshot_ena_d1 = '1') else
                            latency_min;
  latency_acc_o          <= std_logic_vector(latency_acc_d1(g_acc_width-1 downto 0)) when (snapshot_ena_d1 = '1') else
                            std_logic_vector(latency_acc(g_acc_width-1 downto 0));
  latency_cnt_o          <= std_logic_vector(latency_cnt_d1) when (snapshot_ena_d1 = '1') else
                            std_logic_vector(latency_cnt);
  latency_acc_overflow_o <= latency_acc_overflow_d1  when (snapshot_ena_d1 = '1') else
                            latency_acc_overflow;

end rtl;