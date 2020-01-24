-------------------------------------------------------------------------------
-- Title      : Private constants/types/functions package
-- Project    : White Rabbit MAC/Endpoint
-------------------------------------------------------------------------------
-- File       : endpoint_private_pkg.vhd
-- Author     : Tomasz Włostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2010-11-18
-- Last update: 2017-02-20
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: Endpoint private definitions:
-- - 8B10B codes
-- - VLAN control registers
-- - Data types: internal fabric, RMON, RTU
-- - 18-bit FIFO fabric packing/unpacking functions
-- - Endpoint subcomponents declarations
-------------------------------------------------------------------------------
--
-- Copyright (c) 2009-2011 CERN / BE-CO-HT
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

use work.ep_wbgen2_pkg.all;
use work.wr_fabric_pkg.all;
use work.endpoint_pkg.all;
use work.genram_pkg.all;

package endpoint_private_pkg is

  -- special/control characters
  constant c_k28_5 : std_logic_vector(7 downto 0) := "10111100";  -- bc
  constant c_k23_7 : std_logic_vector(7 downto 0) := "11110111";  -- f7
  constant c_k27_7 : std_logic_vector(7 downto 0) := "11111011";  -- fb
  constant c_k29_7 : std_logic_vector(7 downto 0) := "11111101";  -- fd
  constant c_k30_7 : std_logic_vector(7 downto 0) := "11111110";  -- fe
  constant c_k28_7 : std_logic_vector(7 downto 0) := "11111100";  -- fc
  constant c_d21_5 : std_logic_vector(7 downto 0) := "10110101";  -- b5

  constant c_d2_2          : std_logic_vector(7 downto 0) := "01000010";  -- 42
  constant c_d5_6          : std_logic_vector(7 downto 0) := "11000101";  -- c5
  constant c_d16_2         : std_logic_vector(7 downto 0) := "01010000";  -- 50
  constant c_preamble_char : std_logic_vector(7 downto 0) := "01010101";
  constant c_preamble_sfd  : std_logic_vector(7 downto 0) := "11010101";

  constant c_QMODE_PORT_ACCESS        : std_logic_vector(1 downto 0) := "00";
  constant c_QMODE_PORT_TRUNK         : std_logic_vector(1 downto 0) := "01";
  constant c_QMODE_PORT_UNQUALIFIED   : std_logic_vector(1 downto 0) := "11";
  constant c_QMODE_PORT_VLAN_DISABLED : std_logic_vector(1 downto 0) := "10";

  -- fixme: remove these along with the non-WB version of the endpoint
  constant c_wrsw_ctrl_none      : std_logic_vector(4 - 1 downto 0) := x"0";
  constant c_wrsw_ctrl_dst_mac   : std_logic_vector(4 - 1 downto 0) := x"1";
  constant c_wrsw_ctrl_src_mac   : std_logic_vector(4 - 1 downto 0) := x"2";
  constant c_wrsw_ctrl_ethertype : std_logic_vector(4 - 1 downto 0) := x"3";
  constant c_wrsw_ctrl_vid_prio  : std_logic_vector(4 - 1 downto 0) := x"4";
  constant c_wrsw_ctrl_tx_oob    : std_logic_vector(4 - 1 downto 0) := x"5";
  constant c_wrsw_ctrl_rx_oob    : std_logic_vector(4 - 1 downto 0) := x"6";
  constant c_wrsw_ctrl_payload   : std_logic_vector(4 - 1 downto 0) := x"7";
  constant c_wrsw_ctrl_fcs       : std_logic_vector(4 - 1 downto 0) := x"8";

  type t_ep_internal_rtu_request is record
    smac     : std_logic_vector(47 downto 0);
    dmac     : std_logic_vector(47 downto 0);
    vid      : std_logic_vector(11 downto 0);
    prio     : std_logic_vector(2 downto 0);
    has_vid  : std_logic;
    has_prio : std_logic;
    hash     : std_logic_vector(15 downto 0);
  end record;

  type t_rmon_triggers is record
    rx_sync_lost           : std_logic;
    rx_invalid_code        : std_logic;
    rx_overrun             : std_logic;
    rx_crc_err             : std_logic;
    rx_ok                  : std_logic;
    rx_pfilter_drop        : std_logic;
    rx_runt                : std_logic;
    rx_giant               : std_logic;
    rx_pause               : std_logic;
    rx_pcs_err             : std_logic;
    rx_buffer_overrun      : std_logic;
    rx_rtu_overrun         : std_logic;
    rx_path_timing_failure : std_logic;
    tx_pause               : std_logic;
    tx_underrun            : std_logic;
    rx_pclass              : std_logic_vector(7 downto 0); -- packet class (from filter)
    rx_tclass              : std_logic_vector(7 downto 0); -- traffic class (priority)
    tx_frame               : std_logic;
    rx_frame               : std_logic;
    rx_drop_at_rtu_full    : std_logic;
  end record;

  component ep_1000basex_pcs
    generic (
      g_simulation : boolean;
      g_16bit      : boolean);
    port (
      rst_sys_n_i                   : in  std_logic;
      rst_txclk_n_i                 : in  std_logic;
      rst_rxclk_n_i                 : in  std_logic;
      clk_sys_i                     : in  std_logic;
      rxpcs_fab_o                   : out t_ep_internal_fabric;
      rxpcs_fifo_almostfull_i       : in  std_logic;
      rxpcs_busy_o                  : out std_logic;
      rxpcs_timestamp_trigger_p_a_o : out std_logic;
      rxpcs_timestamp_i             : in  std_logic_vector(31 downto 0);
      rxpcs_timestamp_stb_i         : in  std_logic;
      rxpcs_timestamp_valid_i       : in  std_logic;
      txpcs_fab_i                   : in  t_ep_internal_fabric;
      txpcs_error_o                 : out std_logic;
      txpcs_busy_o                  : out std_logic;
      txpcs_dreq_o                  : out std_logic;
      txpcs_timestamp_trigger_p_a_o : out std_logic;
      link_ok_o                     : out std_logic;
      link_ctr_i                    : in  std_logic := '1';
      serdes_rst_o                  : out std_logic;
      serdes_loopen_o               : out std_logic;
      serdes_loopen_vec_o           : out std_logic_vector(2 downto 0);
      serdes_tx_prbs_sel_o          : out std_logic_vector(2 downto 0);
      serdes_sfp_tx_fault_i         : in  std_logic;
      serdes_sfp_los_i              : in  std_logic;
      serdes_sfp_tx_disable_o       : out std_logic;
      serdes_rdy_i                  : in  std_logic;
      serdes_tx_clk_i               : in  std_logic;
      serdes_tx_data_o              : out std_logic_vector(f_pcs_data_width(g_16bit)-1 downto 0);
      serdes_tx_k_o                 : out std_logic_vector(f_pcs_k_width(g_16bit)-1 downto 0);
      serdes_tx_disparity_i         : in  std_logic;
      serdes_tx_enc_err_i           : in  std_logic;
      serdes_rx_clk_i               : in  std_logic;
      serdes_rx_data_i              : in  std_logic_vector(f_pcs_data_width(g_16bit)-1 downto 0);
      serdes_rx_k_i                 : in  std_logic_vector(f_pcs_k_width(g_16bit)-1 downto 0);
      serdes_rx_enc_err_i           : in  std_logic;
      serdes_rx_bitslide_i          : in  std_logic_vector(f_pcs_bts_width(g_16bit)-1 downto 0);
      rmon_o                        : out t_rmon_triggers;
      mdio_addr_i                   : in  std_logic_vector(15 downto 0);
      mdio_data_i                   : in  std_logic_vector(15 downto 0);
      mdio_data_o                   : out std_logic_vector(15 downto 0);
      mdio_stb_i                    : in  std_logic;
      mdio_rw_i                     : in  std_logic;
      mdio_ready_o                  : out std_logic;
      dbg_tx_pcs_wr_count_o     : out std_logic_vector(5+4 downto 0);
      dbg_tx_pcs_rd_count_o     : out std_logic_vector(5+4 downto 0);
      nice_dbg_o  : out t_dbg_ep_pcs);
  end component;

  component ep_tx_pcs_8bit
    port (
      rst_n_i                 : in  std_logic;
      rst_txclk_n_i           : in  std_logic;
      clk_sys_i               : in  std_logic;
      pcs_fab_i               : in  t_ep_internal_fabric;
      pcs_error_o             : out std_logic;
      pcs_busy_o              : out std_logic;
      pcs_dreq_o              : out std_logic;
      mdio_mcr_reset_i        : in  std_logic;
      mdio_mcr_pdown_i        : in  std_logic;
      mdio_wr_spec_tx_cal_i   : in  std_logic;
      an_tx_en_i              : in  std_logic;
      an_tx_val_i             : in  std_logic_vector(15 downto 0);
      timestamp_trigger_p_a_o : out std_logic;
      rmon_tx_underrun        : out std_logic;
      phy_tx_clk_i            : in  std_logic;
      phy_tx_data_o           : out std_logic_vector(7 downto 0);
      phy_tx_k_o              : out std_logic;
      phy_tx_disparity_i      : in  std_logic;
      phy_tx_enc_err_i        : in  std_logic);
  end component;

  component ep_tx_pcs_16bit
    port (
      rst_n_i                 : in  std_logic;
      rst_txclk_n_i           : in  std_logic;
      clk_sys_i               : in  std_logic;
      pcs_fab_i               : in  t_ep_internal_fabric;
      pcs_error_o             : out std_logic;
      pcs_busy_o              : out std_logic;
      pcs_dreq_o              : out std_logic;
      mdio_mcr_reset_i        : in  std_logic;
      mdio_mcr_pdown_i        : in  std_logic;
      mdio_wr_spec_tx_cal_i   : in  std_logic;
      an_tx_en_i              : in  std_logic;
      an_tx_val_i             : in  std_logic_vector(15 downto 0);
      timestamp_trigger_p_a_o : out std_logic;
      rmon_tx_underrun        : out std_logic;
      phy_tx_clk_i            : in  std_logic;
      phy_tx_data_o           : out std_logic_vector(15 downto 0);
      phy_tx_k_o              : out std_logic_vector(1 downto 0);
      phy_tx_disparity_i      : in  std_logic;
      phy_tx_enc_err_i        : in  std_logic;
      dbg_wr_count_o     : out std_logic_vector(5+4 downto 0);
      dbg_rd_count_o     : out std_logic_vector(5+4 downto 0));
  end component;

  component ep_rx_pcs_8bit
    generic (
      g_simulation : boolean);
    port (
      clk_sys_i                  : in  std_logic;
      rst_n_i                    : in  std_logic;
      rst_rxclk_n_i              : in  std_logic;
      pcs_fifo_almostfull_i      : in  std_logic;
      pcs_busy_o                 : out std_logic;
      pcs_fab_o                  : out t_ep_internal_fabric;
      timestamp_trigger_p_a_o    : out std_logic;  -- strobe for RX timestamping
      timestamp_i                : in  std_logic_vector(31 downto 0);
      timestamp_stb_i            : in  std_logic;
      timestamp_valid_i          : in  std_logic;
      phy_rdy_i                  : in  std_logic;
      phy_rx_clk_i               : in  std_logic;
      phy_rx_data_i              : in  std_logic_vector(7 downto 0);
      phy_rx_k_i                 : in  std_logic;
      phy_rx_enc_err_i           : in  std_logic;
      mdio_mcr_reset_i           : in  std_logic;
      mdio_mcr_pdown_i           : in  std_logic;
      mdio_wr_spec_cal_crst_i    : in  std_logic;
      mdio_wr_spec_rx_cal_stat_o : out std_logic;
      synced_o                   : out std_logic;
      sync_lost_o                : out std_logic;
      an_rx_en_i                 : in  std_logic;
      an_rx_val_o                : out std_logic_vector(15 downto 0);
      an_rx_valid_o              : out std_logic;
      an_idle_match_o            : out std_logic;
      rmon_rx_overrun            : out std_logic;
      rmon_rx_inv_code           : out std_logic;
      rmon_rx_sync_lost          : out std_logic);
  end component;

  component ep_rx_pcs_16bit
    generic (
      g_simulation : boolean);
    port (
      clk_sys_i                  : in  std_logic;
      rst_n_i                    : in  std_logic;
      rst_rxclk_n_i              : in  std_logic;
      pcs_fifo_almostfull_i      : in  std_logic;
      pcs_busy_o                 : out std_logic;
      pcs_fab_o                  : out t_ep_internal_fabric;
      timestamp_trigger_p_a_o    : out std_logic;  -- strobe for RX timestamping
      timestamp_i                : in  std_logic_vector(31 downto 0);
      timestamp_stb_i            : in  std_logic;
      timestamp_valid_i          : in  std_logic;
      phy_rdy_i                  : in  std_logic;
      phy_rx_clk_i               : in  std_logic;
      phy_rx_data_i              : in  std_logic_vector(15 downto 0);
      phy_rx_k_i                 : in  std_logic_vector(1 downto 0);
      phy_rx_enc_err_i           : in  std_logic;
      mdio_mcr_reset_i           : in  std_logic;
      mdio_mcr_pdown_i           : in  std_logic;
      mdio_wr_spec_cal_crst_i    : in  std_logic;
      mdio_wr_spec_rx_cal_stat_o : out std_logic;
      synced_o                   : out std_logic;
      sync_lost_o                : out std_logic;
      an_rx_en_i                 : in  std_logic;
      an_rx_val_o                : out std_logic_vector(15 downto 0);
      an_rx_valid_o              : out std_logic;
      an_idle_match_o            : out std_logic;
      rmon_rx_overrun            : out std_logic;
      rmon_rx_inv_code           : out std_logic;
      rmon_rx_sync_lost          : out std_logic;
      nice_dbg_o                 : out t_dbg_ep_rxpcs);
  end component;

  component ep_autonegotiation
    generic (
      g_simulation : boolean);
    port (
      clk_sys_i               : in  std_logic;
      rst_n_i                 : in  std_logic;
      pcs_synced_i            : in  std_logic;
      pcs_los_i               : in  std_logic;
      pcs_link_ok_o           : out std_logic;
      an_idle_match_i         : in  std_logic;
      an_rx_en_o              : out std_logic;
      an_rx_val_i             : in  std_logic_vector(15 downto 0);
      an_rx_valid_i           : in  std_logic;
      an_tx_en_o              : out std_logic;
      an_tx_val_o             : out std_logic_vector(15 downto 0);
      mdio_mcr_anrestart_i    : in  std_logic;
      mdio_mcr_anenable_i     : in  std_logic;
      mdio_msr_anegcomplete_o : out std_logic;
      mdio_advertise_pause_i  : in  std_logic_vector(1 downto 0);
      mdio_advertise_rfault_i : in  std_logic_vector(1 downto 0);
      mdio_lpa_full_o         : out std_logic;
      mdio_lpa_half_o         : out std_logic;
      mdio_lpa_pause_o        : out std_logic_vector(1 downto 0);
      mdio_lpa_rfault_o       : out std_logic_vector(1 downto 0);
      mdio_lpa_lpack_o        : out std_logic;
      mdio_lpa_npage_o        : out std_logic);
  end component;

  component ep_pcs_tbi_mdio_wb
    port (
      rst_n_i                    : in  std_logic;
      clk_sys_i                  : in  std_logic;
      wb_adr_i                   : in  std_logic_vector(4 downto 0);
      wb_dat_i                   : in  std_logic_vector(31 downto 0);
      wb_dat_o                   : out std_logic_vector(31 downto 0);
      wb_cyc_i                   : in  std_logic;
      wb_sel_i                   : in  std_logic_vector(3 downto 0);
      wb_stb_i                   : in  std_logic;
      wb_we_i                    : in  std_logic;
      wb_ack_o                   : out std_logic;
      wb_stall_o                 : out std_logic;
      tx_clk_i                   : in  std_logic;
      rx_clk_i                   : in  std_logic;
      mdio_mcr_uni_en_o          : out std_logic;
      mdio_mcr_anrestart_o       : out std_logic;
      mdio_mcr_pdown_o           : out std_logic;
      mdio_mcr_anenable_o        : out std_logic;
      mdio_mcr_reset_o           : out std_logic;
      mdio_mcr_loopback_o        : out std_logic;
      mdio_msr_lstatus_i         : in  std_logic;
      lstat_read_notify_o        : out std_logic;
      mdio_msr_rfault_i          : in  std_logic;
      mdio_msr_anegcomplete_i    : in  std_logic;
      mdio_advertise_pause_o     : out std_logic_vector(1 downto 0);
      mdio_advertise_rfault_o    : out std_logic_vector(1 downto 0);
      mdio_lpa_full_i            : in  std_logic;
      mdio_lpa_half_i            : in  std_logic;
      mdio_lpa_pause_i           : in  std_logic_vector(1 downto 0);
      mdio_lpa_rfault_i          : in  std_logic_vector(1 downto 0);
      mdio_lpa_lpack_i           : in  std_logic;
      mdio_lpa_npage_i           : in  std_logic;
      mdio_wr_spec_tx_cal_o      : out std_logic;
      mdio_wr_spec_rx_cal_stat_i : in  std_logic;
      mdio_wr_spec_cal_crst_o    : out std_logic;
      mdio_wr_spec_bslide_i      : in  std_logic_vector(4 downto 0);
      mdio_ectrl_lpbck_vec_o       : out std_logic_vector(2 downto 0);
      mdio_ectrl_sfp_tx_fault_i    : in  std_logic;
      mdio_ectrl_sfp_loss_i        : in  std_logic;
      mdio_ectrl_sfp_tx_disable_o  : out std_logic;
      mdio_ectrl_tx_prbs_sel_o     : out std_logic_vector(2 downto 0));
  end component;

  component ep_tx_header_processor
    generic (
      g_with_packet_injection : boolean;
      g_with_timestamper      : boolean;
      g_force_gap_length      : integer;
      g_runt_padding          : boolean);
    port (
      clk_sys_i              : in  std_logic;
      rst_n_i                : in  std_logic;
      src_fab_o              : out t_ep_internal_fabric;
      src_dreq_i             : in  std_logic;
      pcs_busy_i             : in  std_logic;
      pcs_error_i            : in  std_logic;
      wb_snk_i               : in  t_wrf_sink_in;
      wb_snk_o               : out t_wrf_sink_out;
      fc_pause_req_i         : in  std_logic;
      fc_pause_delay_i       : in  std_logic_vector(15 downto 0);
      fc_pause_ready_o       : out std_logic;
      fc_flow_enable_i       : in  std_logic;
      txtsu_port_id_o        : out std_logic_vector(4 downto 0);
      txtsu_fid_o            : out std_logic_vector(16 -1 downto 0);
      txtsu_ts_value_o       : out std_logic_vector(28 + 4 - 1 downto 0);
      txtsu_ts_incorrect_o   : out std_logic;
      txtsu_stb_o            : out std_logic;
      txtsu_ack_i            : in  std_logic;
      txts_timestamp_i       : in  std_logic_vector(31 downto 0);
      txts_timestamp_valid_i : in  std_logic;
      ep_ctrl_i              : in std_logic;
      regs_i                 : in  t_ep_out_registers);
  end component;

  component ep_tx_vlan_unit
    port (
      clk_sys_i         : in  std_logic;
      rst_n_i           : in  std_logic;
      snk_fab_i         : in  t_ep_internal_fabric;
      snk_dreq_o        : out std_logic;
      src_fab_o         : out t_ep_internal_fabric;
      src_dreq_i        : in  std_logic;
      inject_mem_addr_i : in  std_logic_vector(9 downto 0);
      inject_mem_data_o : out std_logic_vector(17 downto 0);
      uram_offset_wr_i  : in  std_logic;
      uram_offset_i     : in  std_logic_vector(9 downto 0);
      uram_data_i       : in  std_logic_vector(17 downto 0));
  end component;
  
  component ep_timestamping_unit
    generic (
      g_timestamp_bits_r : natural;
      g_timestamp_bits_f : natural;
      g_ref_clock_rate   : integer);
    port (
      clk_ref_i                  : in  std_logic;
      clk_sys_i                  : in  std_logic;
      clk_rx_i                   : in  std_logic;
      rst_n_rx_i                 : in  std_logic;
      rst_n_ref_i                : in  std_logic;
      rst_n_sys_i                : in  std_logic;
      pps_csync_p1_i             : in  std_logic;
      pps_valid_i                : in  std_logic;
      tx_timestamp_trigger_p_a_i : in  std_logic;
      rx_timestamp_trigger_p_a_i : in  std_logic;
      rxts_timestamp_o           : out std_logic_vector(31 downto 0);
      rxts_timestamp_stb_o       : out std_logic;
      rxts_timestamp_valid_o     : out std_logic;
      txts_timestamp_o           : out std_logic_vector(31 downto 0);
      txts_timestamp_stb_o       : out std_logic;
      txts_timestamp_valid_o     : out std_logic;
      txts_o                     : out std_logic; 		-- 2013-Nov-28 peterj added for debugging/calibration
      rxts_o                     : out std_logic; 		-- 2013-Nov-28 peterj added for debugging/calibration
      regs_i                     : in  t_ep_out_registers;
      regs_o                     : out t_ep_in_registers);
  end component;

  component ep_flow_control
    port (
      clk_sys_i          : in  std_logic;
      rst_n_i            : in  std_logic;
      rx_pause_p1_i      : in  std_logic;
      rx_pause_delay_i   : in  std_logic_vector(15 downto 0);
      tx_pause_o         : out std_logic;
      tx_pause_delay_o   : out std_logic_vector(15 downto 0);
      tx_pause_ack_i     : in  std_logic;
      tx_flow_enable_o   : out std_logic;
      rx_buffer_used_i   : in  std_logic_vector(7 downto 0);
      ep_fcr_txpause_i   : in  std_logic;
      ep_fcr_rxpause_i   : in  std_logic;
      ep_fcr_tx_thr_i    : in  std_logic_vector(7 downto 0);
      ep_fcr_tx_quanta_i : in  std_logic_vector(15 downto 0);
      rmon_rcvd_pause_o  : out std_logic;
      rmon_sent_pause_o  : out std_logic);
  end component;

  component ep_wishbone_controller
    port (
      rst_n_i            : in  std_logic;
      clk_sys_i          : in  std_logic;
      wb_adr_i          : in  std_logic_vector(4 downto 0);
      wb_dat_i          : in  std_logic_vector(31 downto 0);
      wb_dat_o          : out std_logic_vector(31 downto 0);
      wb_cyc_i           : in  std_logic;
      wb_sel_i           : in  std_logic_vector(3 downto 0);
      wb_stb_i           : in  std_logic;
      wb_we_i            : in  std_logic;
      wb_ack_o           : out std_logic;
      wb_stall_o         : out std_logic;
      tx_clk_i           : in  std_logic;
      rx_clk_i           : in  std_logic;
      regs_o             : out t_ep_out_registers;
      regs_i             : in  t_ep_in_registers);
  end component;

  component ep_leds_controller
    generic (
      g_blink_period_log2 : integer);
    port (
      clk_sys_i   : in  std_logic;
      rst_n_i     : in  std_logic;
      dvalid_tx_i : in  std_logic;
      dvalid_rx_i : in  std_logic;
      link_ok_i   : in  std_logic;
      led_link_o  : out std_logic;
      led_act_o   : out std_logic);
  end component;

  component ep_tx_packet_injection
    port (
      clk_sys_i           : in  std_logic;
      rst_n_i             : in  std_logic;
      snk_fab_i           : in  t_ep_internal_fabric;
      snk_dreq_o          : out std_logic;
      src_fab_o           : out t_ep_internal_fabric;
      src_dreq_i          : in  std_logic;
      inject_req_i        : in  std_logic;
      inject_ready_o      : out std_logic;
      inject_packet_sel_i : in  std_logic_vector(2 downto 0);
      inject_user_value_i : in  std_logic_vector(15 downto 0);
      inject_mode_i       : in  std_logic_vector(1 downto 0);
      mem_addr_o          : out std_logic_vector(9 downto 0);
      mem_data_i          : in  std_logic_vector(17 downto 0));
  end component;

  component ep_rtu_header_extract
    generic (
      g_with_rtu : boolean);
    port (
      clk_sys_i        : in  std_logic;
      rst_n_i          : in  std_logic;
      snk_fab_i        : in  t_ep_internal_fabric;
      snk_dreq_o       : out std_logic;
      src_fab_o        : out t_ep_internal_fabric;
      src_dreq_i       : in  std_logic;
      mbuf_is_pause_i  : in  std_logic;
      vlan_class_i     : in  std_logic_vector(2 downto 0);
      vlan_vid_i       : in  std_logic_vector(11 downto 0);
      vlan_tag_done_i  : in  std_logic;
      vlan_is_tagged_i : in  std_logic;
      rmon_drp_at_rtu_full_o: out std_logic;
      rtu_rq_o         : out t_ep_internal_rtu_request;
      rtu_full_i       : in  std_logic;
      rtu_rq_abort_o   : out std_logic;
      rtu_rq_valid_o   : out std_logic;
      rxbuf_full_i     : in  std_logic);
  end component;

  component ep_rx_early_address_match
    port (
      clk_sys_i               : in  std_logic;
      clk_rx_i                : in  std_logic;
      rst_n_sys_i             : in  std_logic;
      rst_n_rx_i              : in  std_logic;
      snk_fab_i               : in  t_ep_internal_fabric;
      src_fab_o               : out t_ep_internal_fabric;
      match_done_o            : out std_logic;
      match_is_hp_o           : out std_logic;
      match_is_pause_o        : out std_logic;
      match_pause_quanta_o    : out std_logic_vector(15 downto 0);
      match_pause_prio_mask_o : out std_logic_vector(7 downto 0);
      match_pause_p_o         : out std_logic;
      regs_i                  : in  t_ep_out_registers);
  end component;

  component ep_clock_alignment_fifo
    generic (
      g_size                 : integer;
      g_almostfull_threshold : integer);
    port (
      rst_n_rd_i       : in  std_logic;
      rst_n_wr_i       : in std_logic;
      clk_wr_i         : in  std_logic;
      clk_rd_i         : in  std_logic;
      dreq_i           : in  std_logic;
      fab_i            : in  t_ep_internal_fabric;
      fab_o            : out t_ep_internal_fabric;
      full_o           : out std_logic;
      empty_o          : out std_logic;
      almostfull_o     : out std_logic;
      pass_threshold_i : in  std_logic_vector(f_log2_size(g_size)-1 downto 0));
  end component;

  component ep_packet_filter
    port (
      clk_rx_i    : in  std_logic;
      clk_sys_i   : in  std_logic;
      rst_n_rx_i  : in  std_logic;
      rst_n_sys_i : in  std_logic;
      snk_fab_i   : in  t_ep_internal_fabric;
      src_fab_o   : out t_ep_internal_fabric;
      done_o      : out std_logic;
      pclass_o    : out std_logic_vector(7 downto 0);
      drop_o      : out std_logic;
      regs_i      : in  t_ep_out_registers);
  end component;

  component ep_rx_vlan_unit
    port (
      clk_sys_i   : in    std_logic;
      rst_n_i     : in    std_logic;
      snk_fab_i   : in    t_ep_internal_fabric;
      snk_dreq_o  : out   std_logic;
      src_fab_o   : out   t_ep_internal_fabric;
      src_dreq_i  : in    std_logic;
      tclass_o    : out   std_logic_vector(2 downto 0);
      vid_o       : out   std_logic_vector(11 downto 0);
      tag_done_o  : out   std_logic;
      is_tagged_o : out   std_logic;
      regs_i      : in    t_ep_out_registers;
      regs_o      : out   t_ep_in_registers);
  end component;

  component ep_rx_oob_insert
    port (
      clk_sys_i  : in  std_logic;
      rst_n_i    : in  std_logic;
      snk_fab_i  : in  t_ep_internal_fabric;
      snk_dreq_o : out std_logic;
      src_fab_o  : out t_ep_internal_fabric;
      src_dreq_i : in  std_logic;
      regs_i     : in  t_ep_out_registers);
  end component;

  component ep_rx_crc_size_check
  	generic (
      g_use_new_crc : boolean := false);
    port (
      clk_sys_i      : in  std_logic;
      rst_n_i        : in  std_logic;
      snk_fab_i      : in  t_ep_internal_fabric;
      snk_dreq_o     : out std_logic;
      src_fab_o      : out t_ep_internal_fabric;
      src_dreq_i     : in  std_logic;
      regs_i         : in  t_ep_out_registers;
      rmon_pcs_err_o : out std_logic;
      rmon_giant_o   : out std_logic;
      rmon_runt_o    : out std_logic;
      rmon_crc_err_o : out std_logic);
  end component;

  component ep_rx_wb_master
    generic (
      g_ignore_ack   : boolean;
      g_cyc_on_stall : boolean := false);
    port (
      clk_sys_i  : in  std_logic;
      rst_n_i    : in  std_logic;
      snk_fab_i  : in  t_ep_internal_fabric;
      snk_dreq_o : out std_logic;
      src_wb_i   : in  t_wrf_source_in;
      src_wb_o   : out t_wrf_source_out);
  end component;

  component ep_rx_status_reg_insert
    port (
      clk_sys_i           : in  std_logic;
      rst_n_i             : in  std_logic;
      snk_fab_i           : in  t_ep_internal_fabric;
      snk_dreq_o          : out std_logic;
      src_fab_o           : out t_ep_internal_fabric;
      src_dreq_i          : in  std_logic;
      mbuf_valid_i        : in  std_logic;
      mbuf_ack_o          : out std_logic;
      mbuf_drop_i         : in  std_logic;
      mbuf_pclass_i       : in  std_logic_vector(7 downto 0);
      mbuf_is_hp_i        : in  std_logic;
      mbuf_is_pause_i     : in  std_logic;
      rmon_pfilter_drop_o : out std_logic);
  end component;

  component ep_rx_buffer
    generic (
      g_size : integer;
      g_with_fc : boolean := false);
    port (
      clk_sys_i  : in  std_logic;
      rst_n_i    : in  std_logic;
      snk_fab_i  : in  t_ep_internal_fabric;
      snk_dreq_o : out std_logic;
      src_fab_o  : out t_ep_internal_fabric;
      src_dreq_i : in  std_logic;
      level_o    : out std_logic_vector(7 downto 0);
      full_o     : out std_logic;
      drop_req_i : in std_logic;
      dropped_o  : out std_logic;
      regs_i     : in  t_ep_out_registers);
  end component;


  component ep_rx_path
    generic (
      g_with_vlans          : boolean;
      g_with_dpi_classifier : boolean;
      g_with_rtu            : boolean;
      g_with_rx_buffer      : boolean;
      g_rx_buffer_size      : integer;
      g_use_new_crc         :	boolean);
    port (
      clk_sys_i              : in  std_logic;
      clk_rx_i               : in  std_logic;
      rst_n_sys_i            : in  std_logic;
      rst_n_rx_i             : in  std_logic;
      pcs_fab_i              : in  t_ep_internal_fabric;
      pcs_fifo_almostfull_o  : out std_logic;
      pcs_busy_i             : in  std_logic;
      src_wb_o               : out t_wrf_source_out;
      src_wb_i               : in  t_wrf_source_in;
      fc_pause_p_o           : out std_logic;
      fc_pause_quanta_o      : out std_logic_vector(15 downto 0);
      fc_pause_prio_mask_o   : out std_logic_vector(7 downto 0);
      fc_buffer_occupation_o : out std_logic_vector(7 downto 0);
      rmon_o                 : out t_rmon_triggers;
      regs_i                 : in  t_ep_out_registers;
      regs_o                 : out t_ep_in_registers;
      pfilter_pclass_o       : out std_logic_vector(7 downto 0);
      pfilter_drop_o         : out std_logic;
      pfilter_done_o         : out std_logic;
      rtu_rq_o               : out t_ep_internal_rtu_request;
      rtu_full_i             : in  std_logic;
      rtu_rq_valid_o         : out std_logic;
      rtu_rq_abort_o         : out std_logic;
      nice_dbg_o             : out t_dbg_ep_rxpath);
  end component;

  component ep_tx_path
    generic (
      g_with_vlans            : boolean;
      g_with_timestamper      : boolean;
      g_with_packet_injection : boolean;
      g_force_gap_length      : integer;
      g_runt_padding          : boolean;
      g_use_new_crc           :	boolean := false);
    port (
      clk_sys_i              : in  std_logic;
      rst_n_i                : in  std_logic;
      pcs_fab_o              : out t_ep_internal_fabric;
      pcs_error_i            : in  std_logic;
      pcs_busy_i             : in  std_logic;
      pcs_dreq_i             : in  std_logic;
      snk_i                  : in  t_wrf_sink_in;
      snk_o                  : out t_wrf_sink_out;
      fc_pause_req_i         : in  std_logic;
      fc_pause_delay_i       : in  std_logic_vector(15 downto 0);
      fc_pause_ready_o       : out std_logic;
      fc_flow_enable_i       : in  std_logic;
      txtsu_port_id_o        : out std_logic_vector(4 downto 0);
      txtsu_fid_o            : out std_logic_vector(16 -1 downto 0);
      txtsu_ts_value_o       : out std_logic_vector(28 + 4 - 1 downto 0);
      txtsu_ts_incorrect_o   : out std_logic;
      txtsu_stb_o            : out std_logic;
      txtsu_ack_i            : in  std_logic;
      txts_timestamp_i       : in  std_logic_vector(31 downto 0);
      txts_timestamp_valid_i : in  std_logic;
      inject_req_i           : in  std_logic                     := '0';
      inject_ready_o         : out std_logic;
      inject_packet_sel_i    : in  std_logic_vector(2 downto 0)  := "000";
      inject_user_value_i    : in  std_logic_vector(15 downto 0) := x"0000";
      ep_ctrl_i              : in  std_logic                     := '1';
      regs_i                 : in  t_ep_out_registers;
      regs_o                 : out t_ep_in_registers;
      dbg_o                  : out std_logic_vector(33 downto 0));
  end component;

  component ep_tx_crc_inserter
    generic(
      g_use_new_crc	: boolean := false);
    port (
      clk_sys_i  : in  std_logic;
      rst_n_i    : in  std_logic;
      snk_fab_i  : in  t_ep_internal_fabric;
      snk_dreq_o : out std_logic;
      src_fab_o  : out t_ep_internal_fabric;
      src_dreq_i : in  std_logic;
      dbg_o      : out std_logic_vector(2 downto 0));
  end component;
  
  component ep_tx_inject_ctrl
    generic(
      g_min_if_gap_length     : integer
      );
    port (
      clk_sys_i             : in  std_logic;
      rst_n_i               : in  std_logic;
      snk_fab_i             : in  t_ep_internal_fabric;
      snk_dreq_o            : out std_logic;
      src_fab_o             : out t_ep_internal_fabric;
      src_dreq_i            : in  std_logic;
      inject_req_o          : out std_logic;
      inject_ready_i        : in  std_logic;
      inject_packet_sel_o   : out std_logic_vector(2 downto 0);
      inject_user_value_o   : out std_logic_vector(15 downto 0);
      inject_ctr_ena_o      : out std_logic;
      inject_ctr_mode_o     : out std_logic_vector(1 downto 0);
      regs_i                : in  t_ep_out_registers;
      regs_o                : out t_ep_in_registers);
  end component;


  procedure f_pack_fifo_contents (
    signal fab        : in  t_ep_internal_fabric;
    signal dout       : out std_logic_vector;
    signal dout_valid : out std_logic;
    early_eof         :     boolean := false);


  procedure f_unpack_fifo_contents (
    signal din         : in  std_logic_vector;
    constant din_valid : in  std_logic;
    signal fab         : out t_ep_internal_fabric;
    early_eof          : boolean := false);


  procedure f_pack_rmon_triggers (
      signal trig_in  : in t_rmon_triggers;
      signal trig_out : out std_logic_vector(c_epevents_sz-1 downto 0));

end endpoint_private_pkg;

-------------------------------------------------------------------------------

package body endpoint_private_pkg is

  procedure f_pack_fifo_contents
    (
      signal fab        : in  t_ep_internal_fabric;
      signal dout       : out std_logic_vector;
      signal dout_valid : out std_logic;
      early_eof         :     boolean := false) is
  begin
    -- the encodings are slightly different:
    -- - if early_eof == 1, the target needs the EOF information along with the last data word.
    --   This is the case for ep_tx_pcs.
    -- - if early_eof == 0, EOF is an independent transfer
    if(early_eof) then
      if(fab.sof = '1' or fab.error = '1') then
        -- tag = 01
        dout(17 downto 16) <= "01";
        dout(15)           <= fab.sof;
        dout(14)           <= 'X';
        dout(13)           <= fab.error;
        dout(12 downto 0)  <= (others => 'X');
        dout_valid         <= '1';
      elsif(fab.eof = '1' and 
         fab.dvalid = '1') then -- ML: it happened that the last word was "stalled" by PCS (dreq_i LOW)
                                -- at the EOF... CRC indiated that this word is not valid (dvalid LOW)
                                -- but this information was ignored... and tx was hanging the tree
        -- tag = 1x
        dout(17)          <= '1';
        dout(16)          <= fab.bytesel;
        dout(15 downto 0) <= fab.data;
        dout_valid        <= '1';
      elsif(fab.dvalid = '1') then
        -- tag = 00
        dout(17)          <= '0';
        dout(16)          <= '0';
        dout(15 downto 0) <= fab.data;
        dout_valid        <= '1';
      else
        dout(17 downto 0) <= (others => 'X');
        dout_valid        <= '0';
      end if;
    else
      if(fab.sof = '1' or fab.error = '1' or fab.eof = '1' or fab.has_rx_timestamp = '1') then
        -- tag = 01
        dout(17)          <= 'X';
        dout(16)          <= '1';
        dout(15)          <= fab.sof;
        dout(14)          <= fab.eof;
        dout(13)          <= fab.error;
        dout(12)          <= fab.has_rx_timestamp;
        dout(11)          <= fab.rx_timestamp_valid;
        dout(10 downto 0) <= (others => 'X');
        dout_valid        <= '1';
      elsif(fab.dvalid = '1') then
        dout(17)          <= fab.bytesel;
        dout(16)          <= '0';
        dout(15 downto 0) <= fab.data;
        dout_valid        <= '1';
      else
        dout(17 downto 0) <= (others => 'X');
        dout_valid        <= '0';
      end if;

    end if;
  end f_pack_fifo_contents;

  procedure f_unpack_fifo_contents
    (
      signal din         : in  std_logic_vector;
      constant din_valid : in  std_logic;
      signal fab         : out t_ep_internal_fabric;
      early_eof          : boolean := false) is
  begin

    fab.data <= din(15 downto 0);
    fab.addr <= (others => '0');
    if(din_valid = '1') then
      if(early_eof) then
        fab.dvalid             <= not (not din(17) and din(16));
        fab.sof                <= not din(17) and din(16) and din(15);
        fab.eof                <= din(17);
        fab.error              <= not din(17) and din(16) and din(13);
        fab.has_rx_timestamp   <= '0';
        fab.rx_timestamp_valid <= '0';
        fab.bytesel            <= din(17) and din(16);

      else
        fab.dvalid             <= not din(16);
        fab.sof                <= din(16) and din(15);
        fab.eof                <= din(16) and din(14);
        fab.error              <= din(16) and din(13);
        fab.has_rx_timestamp   <= din(16) and din(12);
        fab.rx_timestamp_valid <= din(16) and din(11);
        fab.bytesel            <= (not din(16)) and din(17);
      end if;
    else
      fab.bytesel            <= 'X';
      fab.dvalid             <= '0';
      fab.sof                <= '0';
      fab.eof                <= '0';
      fab.error              <= '0';
      fab.has_rx_timestamp   <= '0';
      fab.rx_timestamp_valid <= '0';
      fab.data               <= (others => 'X');
    end if;
  end f_unpack_fifo_contents;


  procedure f_pack_rmon_triggers
    (
      signal trig_in  : in t_rmon_triggers;
      signal trig_out : out std_logic_vector(c_epevents_sz-1 downto 0)) is
  begin
    --from 1000base pcs
    trig_out(0) <= trig_in.tx_underrun;
    trig_out(1) <= trig_in.rx_overrun;
    trig_out(2) <= trig_in.rx_invalid_code;
    trig_out(3) <= trig_in.rx_sync_lost;
    trig_out(4) <= trig_in.rx_pause;
    trig_out(5) <= trig_in.rx_pfilter_drop;
    trig_out(6) <= trig_in.rx_pcs_err;
    trig_out(7) <= trig_in.rx_giant;
    trig_out(8) <= trig_in.rx_runt;
    trig_out(9) <= trig_in.rx_crc_err;
    trig_out(17 downto 10) <= trig_in.rx_pclass(7 downto 0);
    trig_out(18)<= trig_in.tx_frame;
    trig_out(19)<= trig_in.rx_frame;
    trig_out(20)<= trig_in.rx_drop_at_rtu_full;
    trig_out(28 downto 21) <= trig_in.rx_tclass(7 downto 0);
  end f_pack_rmon_triggers;


end endpoint_private_pkg;

-------------------------------------------------------------------------------
