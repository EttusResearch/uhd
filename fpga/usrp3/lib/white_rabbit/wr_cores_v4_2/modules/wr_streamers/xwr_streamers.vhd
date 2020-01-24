-------------------------------------------------------------------------------
-- Title      : WR Streamrs
-- Project    : WR Streamers
-- URL        : http://www.ohwr.org/projects/wr-cores/wiki/WR_Streamers
-------------------------------------------------------------------------------
-- File       : xwr_streamers.vhd (renamed from xwr_transmission.vhd)
-- Author     : Maciej Lipinski
-- Company    : CERN
-- Platform   : FPGA-generics
-- Standard   : VHDL
-- Created    : 2016-05-30
-------------------------------------------------------------------------------
-- Description:
--
-- This module is a top-level entity for WR streamers to be used conveniently
-- in application. It is inlcluded in the board top entity of wr-cores as one
-- of transportation means.
--
-- It allows to send and receive streames of data over Ethernet network. In
-- other words, it provides communication over Ethernet network that looks as
-- FIFO: one one node (e.g. SPEC, the user writes to tx streamer words of 
-- configureable size. These words are received by the rx streamer in another
-- node (e.g. SVEC) in the same order. 
--
-- This module wraps WR_Streamers-related stuff: i.e.
-- 1) IP core modules provided in wr-cores: xtx_streamer, xrx_streamer,
--    xrtx_streamers_stats
-- 2) wishbone registers that provide access to the statistics and streamer's
--    control/status registers.
--
-- This module interfaces:
-- 1) WR PTP Core for transmission/reception of raw ethernet frames
-- 2) Application-specific module for transmission/reception of data
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
--
---------------------------------------------------------------------------------

library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

library work;
use work.wishbone_pkg.all;  -- needed for t_wishbone_slave_in, etc
use work.streamers_pkg.all; -- needed for streamers and  c_WR_TRANS_ARR_SIZE_*
use work.wr_fabric_pkg.all; -- needed for :t_wrf_source_in, etc
use work.wrcore_pkg.all;    -- needed for t_generic_word_array
use work.wr_streamers_wbgen2_pkg.all;
use work.streamers_priv_pkg.all;

entity xwr_streamers is
  generic (
    -- Indicates whether this module instantiates both streamers (rx and tx) or only one
    -- of them. An application that only receives or only transmits might want to use
    -- RX_ONLY or TX_ONLY mode to save resources.
    g_streamers_op_mode        : t_streamers_op_mode  := TX_AND_RX;
    -----------------------------------------------------------------------------------------
    -- Transmission/reception parameters
    -----------------------------------------------------------------------------------------
    g_tx_streamer_params       : t_tx_streamer_params := c_tx_streamer_params_defaut;
    g_rx_streamer_params       : t_rx_streamer_params := c_rx_streamer_params_defaut;
    -----------------------------------------------------------------------------------------
    -- Statistics config
    -----------------------------------------------------------------------------------------
    -- width of counters: frame rx/tx/lost, block lost, counter of accumuted latency
    -- (min:15, max:64, 50 bits should be ok for 50 years)
    g_stats_cnt_width          : integer := 50;
    -- width of latency accumulator (max value 64)
    g_stats_acc_width          : integer := 64;
    -----------------------------------------------------------------------------------------
    -- WB I/F configuration
    -----------------------------------------------------------------------------------------
    g_slave_mode               : t_wishbone_interface_mode      := CLASSIC;
    g_slave_granularity        : t_wishbone_address_granularity := BYTE;
    g_simulation               : integer := 0
    );

  port (
    clk_sys_i                  : in std_logic;
    rst_n_i                    : in std_logic;

    ---------------------------------------------------------------------------
    -- WR tx/rx interface
    ---------------------------------------------------------------------------
    -- Tx
    src_i                      : in  t_wrf_source_in;
    src_o                      : out t_wrf_source_out;
    -- Rx
    snk_i                      : in  t_wrf_sink_in;
    snk_o                      : out t_wrf_sink_out;

    ---------------------------------------------------------------------------
    -- User tx interface
    ---------------------------------------------------------------------------
    -- Data word to be sent.
    tx_data_i                  : in std_logic_vector(g_tx_streamer_params.data_width-1 downto 0);
    -- 1 indicates that the tx_data_i contains a valid data word.
    tx_valid_i                 : in std_logic;
    -- Synchronous data request: if active, the user may send a data word in
    -- the following clock cycle.
    tx_dreq_o                  : out std_logic;
    -- Last signal. Can be used to indicate the last data word in a larger
    -- block of samples (see documentation for more details).
    tx_last_p1_i               : in std_logic := '1';
    -- Flush input. When asserted, the streamer will immediatly send out all
    -- the data that is stored in its TX buffer, ignoring g_tx_timeout.
    tx_flush_p1_i              : in std_logic := '0';

    ---------------------------------------------------------------------------
    -- User rx interface
    ---------------------------------------------------------------------------
    -- 1 indicates the 1st word of the data block on rx_data_o.
    rx_first_p1_o              : out std_logic;
    -- 1 indicates the last word of the data block on rx_data_o.
    rx_last_p1_o               : out std_logic;
    -- Received data.
    rx_data_o                  : out std_logic_vector(g_rx_streamer_params.data_width-1 downto 0);
    -- 1 indicted that rx_data_o is outputting a valid data word.
    rx_valid_o                 : out std_logic;
    -- Synchronous data request input: when 1, the streamer may output another
    -- data word in the subsequent clock cycle.
    rx_dreq_i                  : in  std_logic;

    ---------------------------------------------------------------------------
    -- WRC Timing interface, used for latency measurement
    ---------------------------------------------------------------------------

    -- White Rabbit reference clock
    clk_ref_i                  : in std_logic := '0';
    -- Time valid flag
    tm_time_valid_i            : in std_logic := '0';
    -- TAI seconds
    tm_tai_i                   : in std_logic_vector(39 downto 0) := x"0000000000";
    -- Fractional part of the second (in clk_ref_i cycles)
    tm_cycles_i                : in std_logic_vector(27 downto 0) := x"0000000";

    -- status of the link, in principle the tx can be done only if link is oK
    link_ok_i                  : in std_logic;
    -- wishbone interface 
    wb_slave_i                 : in  t_wishbone_slave_in := cc_dummy_slave_in;
    wb_slave_o                 : out t_wishbone_slave_out;

    snmp_array_o               : out t_generic_word_array(c_WR_STREAMERS_ARR_SIZE_OUT-1 downto 0);
    snmp_array_i               : in  t_generic_word_array(c_WR_STREAMERS_ARR_SIZE_IN -1 downto 0);

    -----------------------------------------------------------------------------------------
    -- Transmission and Reception configuration
    -----------------------------------------------------------------------------------------
    tx_streamer_cfg_i          : in t_tx_streamer_cfg := c_tx_streamer_cfg_default;
    rx_streamer_cfg_i          : in t_rx_streamer_cfg := c_rx_streamer_cfg_default
    );

end xwr_streamers;

architecture rtl of xwr_streamers is

  signal to_wb              : t_wr_streamers_in_registers;
  signal from_wb            : t_wr_streamers_out_registers;
  signal dbg_word                : std_logic_vector(31 downto 0);
  signal start_bit               : std_logic_vector(from_wb.dbg_ctrl_start_byte_o'length-1+3 downto 0);
  signal rx_data                 : std_logic_vector(g_rx_streamer_params.data_width-1 downto 0);
  signal wb_regs_slave_in        : t_wishbone_slave_in;
  signal wb_regs_slave_out       : t_wishbone_slave_out;  
  signal tx_frame                : std_logic;
  signal reset_time_tai          : std_logic_vector(39 downto 0);
  signal latency_acc             : std_logic_vector(g_stats_acc_width-1 downto 0);
  signal latency_cnt             : std_logic_vector(g_stats_cnt_width-1 downto 0);
  signal sent_frame_cnt_out      : std_logic_vector(g_stats_cnt_width-1 downto 0);
  signal rcvd_frame_cnt_out      : std_logic_vector(g_stats_cnt_width-1 downto 0);
  signal lost_frame_cnt_out      : std_logic_vector(g_stats_cnt_width-1 downto 0);
  signal lost_block_cnt_out      : std_logic_vector(g_stats_cnt_width-1 downto 0);
  signal rx_valid                : std_logic;

  signal rx_latency_valid        : std_logic;
  signal rx_latency              : std_logic_vector(27 downto 0);
  signal rx_lost_frames          : std_logic;
  signal rx_lost_frames_cnt      : std_logic_vector(14 downto 0);
  signal rx_lost_blocks          : std_logic;
  signal rx_frame                : std_logic;

  signal tx_streamer_cfg         : t_tx_streamer_cfg;
  signal rx_streamer_cfg         : t_rx_streamer_cfg;

  -- for code cleanness
  constant c_cw              : integer := g_stats_cnt_width;
  constant c_aw              : integer := g_stats_acc_width;
begin

  -------------------------------------------------------------------------------------------
  -- Instantiate transmission streamer, if configured to do so
  -------------------------------------------------------------------------------------------
  gen_tx: if(g_streamers_op_mode=TX_ONLY OR g_streamers_op_mode=TX_AND_RX) generate
    U_TX: xtx_streamer
      generic map(
        g_data_width             => g_tx_streamer_params.data_width,
        g_tx_buffer_size         => g_tx_streamer_params.buffer_size,
        g_tx_threshold           => g_tx_streamer_params.threshold,
        g_tx_max_words_per_frame => g_tx_streamer_params.max_words_per_frame,
        g_tx_timeout             => g_tx_streamer_params.timeout,
        g_escape_code_disable    => g_tx_streamer_params.escape_code_disable,
        g_simulation             => g_simulation)
      port map(
        clk_sys_i                => clk_sys_i,
        rst_n_i                  => rst_n_i,
        src_i                    => src_i,
        src_o                    => src_o,
        clk_ref_i                => clk_ref_i,
        tm_time_valid_i          => tm_time_valid_i,
        tm_tai_i                 => tm_tai_i,
        tm_cycles_i              => tm_cycles_i,
        link_ok_i                => link_ok_i,
        tx_data_i                => tx_data_i,
        tx_valid_i               => tx_valid_i,
        tx_dreq_o                => tx_dreq_o,
        tx_last_p1_i             => tx_last_p1_i,
        tx_flush_p1_i            => tx_flush_p1_i,
        tx_reset_seq_i           => from_wb.sscr1_rst_seq_id_o,
        tx_frame_p1_o            => tx_frame,
        tx_streamer_cfg_i        => tx_streamer_cfg);
  end generate gen_tx;

  gen_not_tx: if(g_streamers_op_mode=RX_ONLY) generate
    src_o      <= c_dummy_snk_in;
    tx_dreq_o  <= '0';
    tx_frame   <= '0';
  end generate gen_not_tx;

  -------------------------------------------------------------------------------------------
  -- -- Instantiate reception streamer, if configured to do so
  -------------------------------------------------------------------------------------------
  gen_rx: if(g_streamers_op_mode=RX_ONLY OR g_streamers_op_mode=TX_AND_RX) generate
    U_RX: xrx_streamer
      generic map(
        g_data_width             => g_rx_streamer_params.data_width,
        g_buffer_size            => g_rx_streamer_params.buffer_size,
        g_escape_code_disable    => g_rx_streamer_params.escape_code_disable,
        g_expected_words_number  => g_rx_streamer_params.expected_words_number
        )
      port map(
        clk_sys_i                => clk_sys_i,
        rst_n_i                  => rst_n_i,
        snk_i                    => snk_i,
        snk_o                    => snk_o,
        clk_ref_i                => clk_ref_i,
        tm_time_valid_i          => tm_time_valid_i,
        tm_tai_i                 => tm_tai_i,
        tm_cycles_i              => tm_cycles_i,
        rx_first_p1_o            => rx_first_p1_o,
        rx_last_p1_o             => rx_last_p1_o,
        rx_data_o                => rx_data,
        rx_valid_o               => rx_valid,
        rx_dreq_i                => rx_dreq_i,
        rx_lost_p1_o             => rx_lost_blocks,
        rx_lost_frames_p1_o      => rx_lost_frames,
        rx_lost_frames_cnt_o     => rx_lost_frames_cnt,
        rx_latency_o             => rx_latency,
        rx_latency_valid_o       => rx_latency_valid,
        rx_frame_p1_o            => rx_frame,
        rx_streamer_cfg_i        => rx_streamer_cfg);
  end generate gen_rx;
  gen_not_rx: if(g_streamers_op_mode=TX_ONLY) generate
    snk_o         <= c_dummy_src_in;
    rx_first_p1_o <= '0';
    rx_last_p1_o  <= '0';
    rx_data       <= (others => '0');
    rx_valid      <= '0';
  end generate gen_not_rx;

  -------------------------------------------------------------------------------------------
  -- Instantiate statistics module - it calculates statistics of rx/tx frames
  -------------------------------------------------------------------------------------------
  U_STATS: xrtx_streamers_stats
    generic map(
      g_streamers_op_mode      => g_streamers_op_mode,
      g_cnt_width              => g_stats_cnt_width,
      g_acc_width              => g_stats_acc_width
      )
    port map(
      clk_i                    => clk_sys_i,
      rst_n_i                  => rst_n_i,
      sent_frame_i             => tx_frame,
      rcvd_frame_i             => rx_frame,
      lost_frame_i             => rx_lost_frames,
      lost_block_i             => rx_lost_blocks,
      lost_frames_cnt_i        => rx_lost_frames_cnt,
      rcvd_latency_i           => rx_latency,
      rcvd_latency_valid_i     => rx_latency_valid,
      clk_ref_i                => clk_ref_i,
      tm_time_valid_i          => tm_time_valid_i,
      tm_tai_i                 => tm_tai_i,
      tm_cycles_i              => tm_cycles_i,
      reset_stats_i            => from_wb.sscr1_rst_stats_o,
      snapshot_ena_i           => from_wb.sscr1_snapshot_stats_o,
      reset_time_tai_o         => reset_time_tai,
      reset_time_cycles_o      => to_wb.sscr1_rst_ts_cyc_i,
      sent_frame_cnt_o         => sent_frame_cnt_out,
      rcvd_frame_cnt_o         => rcvd_frame_cnt_out,
      lost_frame_cnt_o         => lost_frame_cnt_out,
      lost_block_cnt_o         => lost_block_cnt_out,
      latency_cnt_o            => latency_cnt,
      latency_acc_o            => latency_acc,
      latency_max_o            => to_wb.rx_stat0_rx_latency_max_i,
      latency_min_o            => to_wb.rx_stat1_rx_latency_min_i,
      latency_acc_overflow_o   => to_wb.sscr1_rx_latency_acc_overflow_i,
      snmp_array_o             => snmp_array_o(c_WRS_STATS_ARR_SIZE_OUT-1 downto 0),
      snmp_array_i             => snmp_array_i
      );

  to_wb.sscr2_rst_ts_tai_lsb_i             <= reset_time_tai(31 downto 0);
  to_wb.sscr3_rst_ts_tai_msb_i             <= reset_time_tai(39 downto 32);

  assert (g_stats_acc_width <= 64 and g_stats_acc_width > 32)
    report "g_stats_acc_width (c_aw) must be between 33 and 64" severity error;
  assert (g_stats_cnt_width <= 64 and g_stats_cnt_width > 32)
    report "g_stats_cnt_width (c_cw) must be between 33 and 64" severity error;

  to_wb.tx_stat2_tx_sent_cnt_lsb_i                             <= sent_frame_cnt_out(31     downto 0);
  to_wb.tx_stat3_tx_sent_cnt_msb_i        (c_cw-32-1 downto 0) <= sent_frame_cnt_out(c_cw-1 downto 32);
  to_wb.rx_stat4_rx_rcvd_cnt_lsb_i                             <= rcvd_frame_cnt_out(31     downto 0);
  to_wb.rx_stat5_rx_rcvd_cnt_msb_i        (c_cw-32-1 downto 0) <= rcvd_frame_cnt_out(c_cw-1 downto 32);
  to_wb.rx_stat6_rx_loss_cnt_lsb_i                             <= lost_frame_cnt_out(31     downto 0);
  to_wb.rx_stat7_rx_loss_cnt_msb_i        (c_cw-32-1 downto 0) <= lost_frame_cnt_out(c_cw-1 downto 32);
  to_wb.rx_stat8_rx_lost_block_cnt_lsb_i                       <= lost_block_cnt_out(31     downto 0);
  to_wb.rx_stat9_rx_lost_block_cnt_msb_i  (c_cw-32-1 downto 0) <= lost_block_cnt_out(c_cw-1 downto 32);
  to_wb.rx_stat10_rx_latency_acc_lsb_i                         <= latency_acc       (31     downto 0);
  to_wb.rx_stat11_rx_latency_acc_msb_i    (c_aw-32-1 downto 0) <= latency_acc       (c_aw-1 downto 32);
  to_wb.rx_stat12_rx_latency_acc_cnt_lsb_i                     <= latency_cnt       (31     downto 0);
  to_wb.rx_stat13_rx_latency_acc_cnt_msb_i(c_cw-32-1 downto 0) <= latency_cnt       (c_cw-1 downto 32);

  rx_data_o  <= rx_data;
  rx_valid_o <= rx_valid;

  -------------------------------------------------------------------------------------------
  -- Wishbone access to statistics and configuration
  -------------------------------------------------------------------------------------------
  U_WB_ADAPTER : wb_slave_adapter
    generic map (
      g_master_use_struct  => true,
      g_master_mode        => CLASSIC,
      g_master_granularity => WORD,
      g_slave_use_struct   => true,
      g_slave_mode         => g_slave_mode,
      g_slave_granularity  => g_slave_granularity)
    port map (
      clk_sys_i  => clk_sys_i,
      rst_n_i    => rst_n_i,
      slave_i    => wb_slave_i,
      slave_o    => wb_slave_o,
      master_i   => wb_regs_slave_out,
      master_o   => wb_regs_slave_in);

  U_WB:  wr_streamers_wb
    port map (
      rst_n_i      => rst_n_i,
      clk_sys_i    => clk_sys_i,
      wb_adr_i     => wb_regs_slave_in.adr(5 downto 0),
      wb_dat_i     => wb_regs_slave_in.dat,
      wb_dat_o     => wb_regs_slave_out.dat,
      wb_cyc_i     => wb_regs_slave_in.cyc,
      wb_sel_i     => wb_regs_slave_in.sel(3 downto 0),
      wb_stb_i     => wb_regs_slave_in.stb,
      wb_we_i      => wb_regs_slave_in.we,
      wb_ack_o     => wb_regs_slave_out.ack,
      wb_stall_o   => wb_regs_slave_out.stall,
      regs_i       => to_wb,
      regs_o       => from_wb
    );

  -------------------------------------------------------------------------------------------
  -- Provide generic debugging through WB - user can read 32 bits from each send or received
  -- word. It is possible to configure through WB which 32-bits of each word should be 
  -- "snooped". In particular, user can sent:
  -- * whether he/she wishes to look at received or sent words
  -- * the byte number at which the snooping shall start, i.e. say users sets byte=2, it means
  --   that he will be able to read through wishbone bytes 3-6 of each word, provided the
  --   word is of sufficient width
  -------------------------------------------------------------------------------------------
  start_bit <= from_wb.dbg_ctrl_start_byte_o & "000";
  p_debug_mux: process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        dbg_word <= (others =>'0');
      else
        if(from_wb.dbg_ctrl_mux_o = '1') then --rx
          if(rx_valid = '1') then
            dbg_word <= f_dbg_word_starting_at_bit(rx_data,start_bit,g_rx_streamer_params.data_width);
          end if;
        else -- tx
          if(tx_valid_i = '1') then
            dbg_word <= f_dbg_word_starting_at_bit(tx_data_i,start_bit,g_tx_streamer_params.data_width);
          end if;
        end if;
      end if;
    end if;
  end process;

  snmp_array_o(c_WRS_STATS_ARR_SIZE_OUT)   <= dbg_word;
  snmp_array_o(c_WRS_STATS_ARR_SIZE_OUT+1) <= x"DEADBEEF";

  to_wb.dbg_data_i      <= dbg_word;
  to_wb.dummy_dummy_i   <= x"DEADBEEF";

  -------------------------------------------------------------------------------------------
  -- set configuration: the user can access configuration through two channels:
  -- 1) records (tx_streamer_cfg & rx_streamer_cfg) with input signals
  -- 2) wishbone write
  -- By default, the input signals are used. A user can overwrite this default
  -- configuration using WB access. To override the default configuration two 
  -- values needs to be written:
  -- 1) the value of a proper configuration, e.g.: tx_cfg0_ethertype, rx_cfg0_filter_remote
  -- 2) bit that enables overriding of the configuration, e.g. cfg_or_tx_ethtype and
  --    cfg_or_rx_ftr_remote_o respectively
  -- The latter (overriding bit) is the so that user can first configure all values
  -- that he wishes to and then enable all his desired configuration at the same instant.
  -- Otherwise, during the transition (e.g. between writing lsb and msb of MAC), the
  -- behaviour could be unpredictable.
  -------------------------------------------------------------------------------------------
  -- tx config
  tx_streamer_cfg.ethertype         <= from_wb.tx_cfg0_ethertype_o        when (from_wb.cfg_or_tx_ethtype_o='1') else
                                       tx_streamer_cfg_i.ethertype;
  tx_streamer_cfg.mac_local         <= from_wb.tx_cfg2_mac_local_msb_o &
                                       from_wb.tx_cfg1_mac_local_lsb_o    when (from_wb.cfg_or_tx_mac_loc_o='1') else
                                       tx_streamer_cfg_i.mac_local;
  tx_streamer_cfg.mac_target        <= from_wb.tx_cfg4_mac_target_msb_o &
                                       from_wb.tx_cfg3_mac_target_lsb_o   when (from_wb.cfg_or_tx_mac_tar_o='1') else
                                       tx_streamer_cfg_i.mac_target;
  tx_streamer_cfg.qtag_ena          <= from_wb.tx_cfg5_qtag_ena_o         when (from_wb.cfg_or_tx_qtag_o='1') else
                                       tx_streamer_cfg_i.qtag_ena;
  tx_streamer_cfg.qtag_vid          <= from_wb.tx_cfg5_qtag_vid_o         when (from_wb.cfg_or_tx_qtag_o='1') else
                                       tx_streamer_cfg_i.qtag_vid;
  tx_streamer_cfg.qtag_prio         <= from_wb.tx_cfg5_qtag_prio_o        when (from_wb.cfg_or_tx_qtag_o='1') else
                                       tx_streamer_cfg_i.qtag_prio;

  -- rx config
  rx_streamer_cfg.ethertype         <= from_wb.rx_cfg0_ethertype_o        when (from_wb.cfg_or_rx_ethertype_o='1') else
                                       rx_streamer_cfg_i.ethertype;
  rx_streamer_cfg.mac_local         <= from_wb.rx_cfg2_mac_local_msb_o &
                                       from_wb.rx_cfg1_mac_local_lsb_o    when (from_wb.cfg_or_rx_mac_loc_o='1') else
                                       rx_streamer_cfg_i.mac_local;
  rx_streamer_cfg.mac_remote        <= from_wb.rx_cfg4_mac_remote_msb_o &
                                       from_wb.rx_cfg3_mac_remote_lsb_o   when (from_wb.cfg_or_rx_mac_rem_o='1') else
                                       rx_streamer_cfg_i.mac_remote;
  rx_streamer_cfg.accept_broadcasts <= from_wb.rx_cfg0_accept_broadcast_o when (from_wb.cfg_or_rx_acc_broadcast_o='1') else
                                       rx_streamer_cfg_i.accept_broadcasts;
  rx_streamer_cfg.filter_remote     <= from_wb.rx_cfg0_filter_remote_o    when (from_wb.cfg_or_rx_ftr_remote_o='1') else
                                       rx_streamer_cfg_i.filter_remote;
  rx_streamer_cfg.fixed_latency     <= from_wb.rx_cfg5_fixed_latency_o    when (from_wb.cfg_or_rx_fix_lat_o='1') else
                                       rx_streamer_cfg_i.fixed_latency;
end rtl;