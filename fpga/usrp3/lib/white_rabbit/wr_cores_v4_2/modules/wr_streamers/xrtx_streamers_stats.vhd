-------------------------------------------------------------------------------
-- Title      : WR Streamers statistics
-- Project    : WR Streamers
-- URL        : http://www.ohwr.org/projects/wr-cores/wiki/WR_Streamers
-------------------------------------------------------------------------------
-- File       : xrtx_streamers_stats.vhd
-- Author     : Maciej Lipinski
-- Company    : CERN
-- Created    : 2016-06-08
-- Platform   : FPGA-generics
-- Standard   : VHDL
-------------------------------------------------------------------------------
-- Description:
-- Module to collect, reset, snapshot statistics from the streamers. The
-- statistics are made available through wishbone I/F (outside this entity)
-- and diags_i/o (generic input/output arrays of 32-bit registers). Wishbone I/F
-- can be read via bus (PCI, VME,...). Diags can be read via wrpc commands and 
-- SNMP.
--
-- The module provides basic statistics such as:
-- * number of sent/received streamer frames
-- * number of lost frames/blocks
-- * accumulated latency of streamer frames
-- * count of the accumulated latencies
-- * max/min latency
-- * timestamp of the reset pulse
--
-- The module allows to snapshot the statistics values as to have a coherent
-- view.
-------------------------------------------------------------------------------
--
-- Copyright (c) 2016-2017CERN/BE-CO-HT
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
use work.streamers_priv_pkg.all;

entity xrtx_streamers_stats is
  generic (
    -- Indicates whether this module instantiates both streamers (rx and tx) or only one
    -- of them. An application that only receives or only transmits might want to use
    -- RX_ONLY or TX_ONLY mode to save resources.
    g_streamers_op_mode    : t_streamers_op_mode  := TX_AND_RX;
    -- Width of frame counters
    g_cnt_width            : integer := 50; -- min:15, max:64, 50 bits should be ok for 50 years
    g_acc_width            : integer := 64  -- max value 64
    );
  port (
    clk_i                  : in std_logic;
    rst_n_i                : in std_logic;

    -- input signals from streamers
    sent_frame_i           : in std_logic;
    rcvd_frame_i           : in std_logic;
    lost_block_i           : in std_logic;
    lost_frame_i           : in std_logic;
    lost_frames_cnt_i      : in std_logic_vector(14 downto 0);
    rcvd_latency_i         : in  std_logic_vector(27 downto 0);
    rcvd_latency_valid_i   : in  std_logic;

    clk_ref_i              : in std_logic;
    tm_time_valid_i        : in std_logic := '0';
    tm_tai_i               : in std_logic_vector(39 downto 0) := x"0000000000";
    tm_cycles_i            : in std_logic_vector(27 downto 0) := x"0000000";

    -- statistic control
    reset_stats_i          : in std_logic;
    snapshot_ena_i         : in std_logic := '0';
    ----------------------- statistics ----------------------------------------
    -- output statistics: time of last reset of statistics
    reset_time_tai_o       : out std_logic_vector(39 downto 0) := x"0000000000";
    reset_time_cycles_o    : out std_logic_vector(27 downto 0) := x"0000000";
    -- output statistics: tx/rx counters
    sent_frame_cnt_o       : out std_logic_vector(g_cnt_width-1 downto 0);
    rcvd_frame_cnt_o       : out std_logic_vector(g_cnt_width-1 downto 0);
    lost_frame_cnt_o       : out std_logic_vector(g_cnt_width-1 downto 0);
    lost_block_cnt_o       : out std_logic_vector(g_cnt_width-1 downto 0);
    -- output statistics: latency
    latency_cnt_o          : out std_logic_vector(g_cnt_width-1 downto 0);
    latency_acc_overflow_o : out std_logic;
    latency_acc_o          : out std_logic_vector(g_acc_width-1  downto 0);
    latency_max_o          : out std_logic_vector(27  downto 0);
    latency_min_o          : out std_logic_vector(27  downto 0);

    snmp_array_o           : out t_generic_word_array(c_WRS_STATS_ARR_SIZE_OUT-1 downto 0);
    snmp_array_i           : in  t_generic_word_array(c_WRS_STATS_ARR_SIZE_IN -1 downto 0)
    );

end xrtx_streamers_stats;

architecture rtl of xrtx_streamers_stats is

  signal reset_time_tai    : std_logic_vector(39 downto 0);
  signal reset_time_cycles : std_logic_vector(27 downto 0);

  signal sent_frame_cnt    : unsigned(g_cnt_width-1  downto 0);
  signal rcvd_frame_cnt    : unsigned(g_cnt_width-1  downto 0);
  signal lost_frame_cnt    : unsigned(g_cnt_width-1  downto 0);
  signal lost_block_cnt    : unsigned(g_cnt_width-1  downto 0);
  signal latency_cnt       : unsigned(g_cnt_width-1  downto 0);

  signal latency_max       : std_logic_vector(27  downto 0);
  signal latency_min       : std_logic_vector(27  downto 0);
  signal latency_acc       : unsigned(g_acc_width-1+1  downto 0);
  signal latency_acc_overflow: std_logic;

  signal sent_frame_cnt_out       : std_logic_vector(g_cnt_width-1 downto 0);
  signal rcvd_frame_cnt_out       : std_logic_vector(g_cnt_width-1 downto 0);
  signal lost_frame_cnt_out       : std_logic_vector(g_cnt_width-1 downto 0);
  signal lost_block_cnt_out       : std_logic_vector(g_cnt_width-1 downto 0);
  signal latency_cnt_out          : std_logic_vector(g_cnt_width-1 downto 0);
  signal latency_acc_overflow_out : std_logic;
  signal latency_acc_out          : std_logic_vector(g_acc_width-1  downto 0);
  signal latency_max_out          : std_logic_vector(27  downto 0);
  signal latency_min_out          : std_logic_vector(27  downto 0);

  --- statistics resets:
  signal reset_stats_remote: std_logic;
  signal reset_stats       : std_logic;
  signal reset_stats_d1    : std_logic;
  signal reset_stats_p     : std_logic;
  signal snapshot_remote_ena  : std_logic;
  signal snapshot_ena      : std_logic;
  signal snapshot_ena_d1   : std_logic;
  
  -- for code cleanness
  constant c_cw              : integer := g_cnt_width;
  constant c_aw              : integer := g_acc_width;
begin

  -- reset statistics when receiving signal from SNMP or Wishbone
  reset_stats <= reset_stats_remote or reset_stats_i;
  -------------------------------------------------------------------------------------------
  -- produce pulse of reset input signal, this pulse produces timesstamp to be timestamped
  -------------------------------------------------------------------------------------------
  -- pulse is on falling and rising edge of the reset signal (reset when signal HIGH)
  -- in this way, one can 
  -- 1. read the timestamp of the start of statistics acquisition
  -- 2. reset HIGH
  -- 3. read the timestamp of the end of statistics acquisition
  -- 4. start acqusition
  -------------------------------------------------------------------------------------------
  -- when exiting the reset, produce pulse for the timestamper
  p_stats_reset: process(clk_i)
  begin
    if rising_edge(clk_i) then
      if (rst_n_i = '0') then
         reset_stats_p  <= '0';
         reset_stats_d1 <= '0';
      else
        reset_stats_d1 <= reset_stats;
        reset_stats_p  <= reset_stats xor reset_stats_d1;
      end if;
    end if;
  end process;

  -------------------------------------------------------------------------------------------
  -- Timestamp of reset
  -------------------------------------------------------------------------------------------
  -- process that timestamps the reset so that we can make statistics over time
  U_Reset_Timestamper : pulse_stamper
    port map (
      clk_ref_i       => clk_ref_i,
      clk_sys_i       => clk_i,
      rst_n_i         => rst_n_i,
      pulse_a_i       => reset_stats_p,
      tm_time_valid_i => tm_time_valid_i,
      tm_tai_i        => tm_tai_i,
      tm_cycles_i     => tm_cycles_i,
      tag_tai_o       => reset_time_tai,
      tag_cycles_o    => reset_time_cycles);

  reset_time_tai_o    <= reset_time_tai;
  reset_time_cycles_o <= reset_time_cycles;

  -------------------------------------------------------------------------------------------
  -- snapshot 
  -------------------------------------------------------------------------------------------
  -- snapshot is used to expose to user coherent value, so that the count for accumulated
  -- latency is coherent with the accumulated latency and the average can be accurately 
  -- calculated
  -------------------------------------------------------------------------------------------
  snapshot_ena <= snapshot_ena_i or snapshot_remote_ena;
  -- snapshot

  gen_tx_stats: if(g_streamers_op_mode=TX_ONLY OR g_streamers_op_mode=TX_AND_RX) generate
    U_TX_STATS: xtx_streamers_stats
      generic map (
        g_cnt_width            => g_cnt_width
        )
      port map(
        clk_i                  => clk_i,
        rst_n_i                => rst_n_i,
        sent_frame_i           => sent_frame_i,
        reset_stats_i          => reset_stats,
        snapshot_ena_i         => snapshot_ena,
        sent_frame_cnt_o       => sent_frame_cnt_out);
  end generate gen_tx_stats;
  gen_not_tx_stats: if(g_streamers_op_mode=RX_ONLY) generate
    sent_frame_cnt_out <= (others => '0');
  end generate gen_not_tx_stats;

  gen_rx_stats: if(g_streamers_op_mode=RX_ONLY OR g_streamers_op_mode=TX_AND_RX) generate
    U_RX_STATS: xrx_streamers_stats
      generic map(
        g_cnt_width            => g_cnt_width,
        g_acc_width            => g_acc_width
        )
      port map(
        clk_i                  => clk_i,
        rst_n_i                => rst_n_i,
        rcvd_frame_i           => rcvd_frame_i,
        lost_block_i           => lost_block_i,
        lost_frame_i           => lost_frame_i,
        lost_frames_cnt_i      => lost_frames_cnt_i,
        rcvd_latency_i         => rcvd_latency_i,
        rcvd_latency_valid_i   => rcvd_latency_valid_i,
        tm_time_valid_i        => tm_time_valid_i,
        snapshot_ena_i         => snapshot_ena,
        reset_stats_i          => reset_stats,
        rcvd_frame_cnt_o       => rcvd_frame_cnt_out,
        lost_frame_cnt_o       => lost_frame_cnt_out,
        lost_block_cnt_o       => lost_block_cnt_out,
        latency_cnt_o          => latency_cnt_out,
        latency_acc_overflow_o => latency_acc_overflow_out,
        latency_acc_o          => latency_acc_out,
        latency_max_o          => latency_max_out,
        latency_min_o          => latency_min_out);
  end generate gen_rx_stats;
  gen_not_rx_stats: if(g_streamers_op_mode=TX_ONLY) generate
    rcvd_frame_cnt_out       <= (others => '0');
    lost_frame_cnt_out       <= (others => '0');
    lost_block_cnt_out       <= (others => '0');
    latency_cnt_out          <= (others => '0');
    latency_acc_overflow_out <= '0';
    latency_acc_out          <= (others => '0');
    latency_max_out          <= (others => '0');
    latency_min_out          <= (others => '0');
  end generate gen_not_rx_stats;
  -------------------------------------------------------------------------------------------
  -- wishbone local output
  -------------------------------------------------------------------------------------------
  sent_frame_cnt_o         <= sent_frame_cnt_out;
  rcvd_frame_cnt_o         <= rcvd_frame_cnt_out;
  lost_frame_cnt_o         <= lost_frame_cnt_out;
  lost_block_cnt_o         <= lost_block_cnt_out;
  latency_max_o            <= latency_max_out;
  latency_min_o            <= latency_min_out;
  latency_acc_o            <= latency_acc_out;
  latency_cnt_o            <= latency_cnt_out;
  latency_acc_overflow_o   <= latency_acc_overflow_out;

  -------------------------------------------------------------------------------------------
  -- SNMP remote output
  -- Generic communication with WRPC that allows SNMP access via generic array of 32-bits 
  -- std_logic_vectors. The mapping of the generic vectors to meaningful information needs
  -- to be made available to the user of SNMP
  -------------------------------------------------------------------------------------------
  -- check sanity of values
  assert (c_cw <= 64) 
    report "g_cnt_width value not suppported by f_pack_streamers_statistics" severity error;
  assert (c_aw <= 64) 
    report "g_acc_width value not suppported by f_pack_streamers_statistics" severity error;

  -- translate generic input vectors to meaningful signals
  reset_stats_remote                  <= snmp_array_i(0)(0);
  snapshot_remote_ena                 <= snmp_array_i(0)(1);

  snmp_array_o(0)(             0)     <= reset_stats;                   -- loop back for diagnostics
  snmp_array_o(0)(             1)     <= latency_acc_overflow_out;
  snmp_array_o(0) (31   downto 2)     <= (others => '0');

  snmp_array_o(1)(   31 downto 0)     <= x"0" & reset_time_cycles( 27 downto 0);
  snmp_array_o(2)(   31 downto 0)     <= reset_time_tai(    31 downto 0);
  snmp_array_o(3)(   31 downto 0)     <= x"000000" & reset_time_tai(    39 downto 32);

  -- translate meaningful signals (statistics values) to generic output vectors
  snmp_array_o(4 )(31   downto 0)     <= x"0" & latency_max_out(27 downto 0);
  snmp_array_o(5 )(31   downto 0)     <= x"0" & latency_min_out(27 downto 0); 

  CNT_SINGLE_WORD_gen: if(c_cw < 33) generate
    snmp_array_o(6 )(c_cw-1    downto       0) <= sent_frame_cnt_out;
    snmp_array_o(6 )(31        downto    c_cw) <= (others => '0');
    snmp_array_o(7 )(31        downto       0) <= (others => '0');

    snmp_array_o(8 )(c_cw-1    downto       0) <= rcvd_frame_cnt_out;
    snmp_array_o(8 )(31        downto    c_cw) <= (others => '0');
    snmp_array_o(9 )(31        downto       0) <= (others => '0');

    snmp_array_o(10)(c_cw-1    downto       0) <= lost_frame_cnt_out;
    snmp_array_o(10)(31        downto    c_cw) <= (others => '0');
    snmp_array_o(11)(31        downto       0) <= (others => '0');

    snmp_array_o(12)(c_cw-1    downto       0) <= lost_block_cnt_out;
    snmp_array_o(12)(31        downto    c_cw) <= (others => '0');
    snmp_array_o(13)(31        downto       0) <= (others => '0');

    snmp_array_o(14)(c_cw-1    downto       0) <= latency_cnt_out;
    snmp_array_o(14)(31        downto    c_cw) <= (others => '0');
    snmp_array_o(15)(31        downto       0) <= (others => '0');
  end generate;
  ACC_SINGLE_WORD_gen: if(c_aw < 33) generate
    snmp_array_o(16)(c_aw-1    downto       0) <= latency_acc_out;
    snmp_array_o(16)(31        downto    c_aw) <= (others => '0');
    snmp_array_o(17)(31        downto       0) <= (others => '0');
  end generate;

  ---
  CNT_TWO_WORDs_gen:  if(c_cw > 32)  generate
    snmp_array_o(6 )(31        downto       0) <= sent_frame_cnt_out(31     downto 0);
    snmp_array_o(7 )(c_cw-32-1 downto       0) <= sent_frame_cnt_out(c_cw-1 downto 32);
    snmp_array_o(7 )(31        downto c_cw-32) <= (others => '0');

    snmp_array_o(8 )(31        downto       0) <= rcvd_frame_cnt_out(31     downto 0);
    snmp_array_o(9 )(c_cw-32-1 downto       0) <= rcvd_frame_cnt_out(c_cw-1 downto 32);
    snmp_array_o(9 )(31        downto c_cw-32) <= (others => '0');

    snmp_array_o(10)(31        downto       0) <= lost_frame_cnt_out(31     downto 0);
    snmp_array_o(11)(c_cw-32-1 downto       0) <= lost_frame_cnt_out(c_cw-1 downto 32);
    snmp_array_o(11 )(31       downto c_cw-32) <= (others => '0');

    snmp_array_o(12)(31        downto       0) <= lost_block_cnt_out(31     downto 0);
    snmp_array_o(13)(c_cw-32-1 downto       0) <= lost_block_cnt_out(c_cw-1 downto 32);
    snmp_array_o(13 )(31       downto c_cw-32) <= (others => '0');

    snmp_array_o(14)(31        downto       0) <= latency_cnt_out(31     downto 0);
    snmp_array_o(15)(c_cw-32-1 downto       0) <= latency_cnt_out(c_cw-1 downto 32);
    snmp_array_o(15 )(31       downto c_cw-32) <= (others => '0');
  end generate;
  ACC_TWO_WORDs_gen:   if(c_aw > 32) generate
    snmp_array_o(16)(31        downto       0) <= latency_acc_out(31     downto 0);
    snmp_array_o(17)(c_aw-32-1 downto       0) <= latency_acc_out(c_aw-1 downto 32) ;
    snmp_array_o(17)(31        downto c_aw-32) <= (others => '0'); 
  end generate;

end rtl;

