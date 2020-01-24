-------------------------------------------------------------------------------
-- Title      : WR tx streamers statistics
-- Project    : WR streamers
-- URL        : http://www.ohwr.org/projects/wr-cores/wiki/WR_Streamers
-------------------------------------------------------------------------------
-- File       : xrtx_streamers_stats.vhd
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
-- Copyright (c) 2017 CERN/BE-CO-HT
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
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

library work;
use work.wishbone_pkg.all;  -- needed for t_wishbone_slave_in, etc
use work.streamers_pkg.all; -- needed for streamers
use work.wr_fabric_pkg.all; -- neede for :t_wrf_source_in, etc
use work.wrcore_pkg.all;    -- needed for t_generic_word_array
-- use work.wr_transmission_wbgen2_pkg.all;

entity xtx_streamers_stats is
  
  generic (
    -- Width of frame counters
    g_cnt_width            : integer := 32 -- minimum 15 bits, max 32
    );
  port (
    clk_i                  : in std_logic;
    rst_n_i                : in std_logic;

    -- input signals from streamers
    sent_frame_i           : in std_logic;

    -- statistic control
    reset_stats_i          : in std_logic;
    snapshot_ena_i         : in std_logic := '0';
    ----------------------- statistics ----------------------------------------
    -- output statistics: tx/rx counters
    sent_frame_cnt_o       : out std_logic_vector(g_cnt_width-1 downto 0)
  );

end xtx_streamers_stats;
  
architecture rtl of xtx_streamers_stats is

  signal sent_frame_cnt    : unsigned(g_cnt_width-1  downto 0);
  signal sent_frame_cnt_d1 : unsigned(g_cnt_width-1  downto 0);
  signal snapshot_ena_d1   : std_logic;
begin

  -------------------------------------------------------------------------------------------
  -- frame/block statistics, i.e. lost, sent, received
  -------------------------------------------------------------------------------------------
  -- process that counts
  p_cnts: process(clk_i)
  begin
    if rising_edge(clk_i) then
      if (rst_n_i = '0' or reset_stats_i = '1') then
        sent_frame_cnt        <= (others => '0');
      else
        -- count sent frames
        if(sent_frame_i = '1') then
          sent_frame_cnt <= sent_frame_cnt + 1;
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
  p_stats_snapshot: process(clk_i)
  begin
    if rising_edge(clk_i) then
      if (rst_n_i = '0') then
         snapshot_ena_d1         <= '0';
         sent_frame_cnt_d1       <= (others=>'0');
      else
        if(snapshot_ena_i = '1' and snapshot_ena_d1 = '0') then
         sent_frame_cnt_d1       <= sent_frame_cnt;
        end if;
        snapshot_ena_d1 <= snapshot_ena_i;
      end if;
    end if;
  end process;

  -------------------------------------------------------------------------------------------
  -- snapshot or current value
  -------------------------------------------------------------------------------------------
  sent_frame_cnt_o       <= std_logic_vector(sent_frame_cnt_d1) when (snapshot_ena_d1 = '1') else
                            std_logic_vector(sent_frame_cnt);

end rtl;