-------------------------------------------------------------------------------
-- Title      : 1000base-X MAC/Endpoint
-- Project    : White Rabbit 
-------------------------------------------------------------------------------
-- File       : endpoint_pkg.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2010-04-26
-- Last update: 2017-02-20
-- Platform   : FPGA-generic
-- Standard   : VHDL '93
-------------------------------------------------------------------------------
-- Description: Public package for the WR Endpoint. Contains public data
-- structures and component declarations.
-------------------------------------------------------------------------------
--
-- Copyright (c) 2011-2017 CERN / BE-CO-HT
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

use work.wishbone_pkg.all;
use work.wr_fabric_pkg.all;

package endpoint_pkg is

  function f_pcs_data_width(pcs_16 : boolean) return integer;
  function f_pcs_k_width(pcs_16 : boolean) return integer;
  function f_pcs_bts_width(pcs_16 : boolean) return integer;
  function f_pcs_clock_rate(pcs_16 : boolean) return integer;

  type t_txtsu_timestamp is record
    stb       : std_logic;
    tsval     : std_logic_vector(31 downto 0);
    port_id   : std_logic_vector(5 downto 0);
    frame_id  : std_logic_vector(15 downto 0);
    incorrect : std_logic;
  end record;

  type t_txtsu_timestamp_array is array(integer range <>) of t_txtsu_timestamp;

  -- Endpoint's internal fabric used to connect the submodules with each other.
  -- Easier to handle than pipelined Wishbone.
  type t_ep_internal_fabric is record
    sof                : std_logic;
    eof                : std_logic;
    error              : std_logic;
    dvalid             : std_logic;
    bytesel            : std_logic;
    has_rx_timestamp   : std_logic;
    rx_timestamp_valid : std_logic;
    data               : std_logic_vector(15 downto 0);
    addr               : std_logic_vector(1 downto 0);
  end record;
  type t_fab_pipe is array(integer range <>) of t_ep_internal_fabric;

  -----------------------------
  -- Phy i/f types
  -----------------------------
  -- 8-bit Serdes
  type t_phy_8bits_to_wrc is record
    ref_clk        : std_logic;
    tx_disparity   : std_logic;
    tx_enc_err     : std_logic;
    rx_data        : std_logic_vector(7 downto 0);
    rx_clk         : std_logic;
    rx_k           : std_logic_vector(0 downto 0);
    rx_enc_err     : std_logic;
    rx_bitslide    : std_logic_vector(3 downto 0);
    rdy            : std_logic;
    sfp_tx_fault   : std_logic;
    sfp_los        : std_logic;
  end record;
  type t_phy_8bits_from_wrc is record
    rst            : std_logic;
    loopen         : std_logic;
    tx_data        : std_logic_vector(7 downto 0);
    tx_k           : std_logic_vector(0 downto 0);
    loopen_vec     : std_logic_vector(2 downto 0);
    tx_prbs_sel    : std_logic_vector(2 downto 0);
    sfp_tx_disable : std_logic;
  end record;

  constant c_dummy_phy8_to_wrc : t_phy_8bits_to_wrc :=
    ('0', '0', '0', (others=>'0'), '0', (others=>'0'), '0',
    (others=>'0'), '0', '0', '0');
  constant c_dummy_phy8_from_wrc : t_phy_8bits_from_wrc :=
    ('0', '0', (others=>'0'), (others=>'0'), (others=>'0'),
    (others=>'0'), '0');

  -- 16-bit Serdes
  type t_phy_16bits_to_wrc is record
    ref_clk        : std_logic;
    tx_disparity   : std_logic;
    tx_enc_err     : std_logic;
    rx_data        : std_logic_vector(15 downto 0);
    rx_clk         : std_logic;
    rx_k           : std_logic_vector(1 downto 0);
    rx_enc_err     : std_logic;
    rx_bitslide    : std_logic_vector(4 downto 0);
    rdy            : std_logic;
    sfp_tx_fault   : std_logic;
    sfp_los        : std_logic;
  end record;
  type t_phy_16bits_from_wrc is record
    rst            : std_logic;
    loopen         : std_logic;
    tx_data        : std_logic_vector(15 downto 0);
    tx_k           : std_logic_vector(1 downto 0);
    loopen_vec     : std_logic_vector(2 downto 0);
    tx_prbs_sel    : std_logic_vector(2 downto 0);
    sfp_tx_disable : std_logic;
  end record;

  constant c_dummy_phy16_to_wrc : t_phy_16bits_to_wrc :=
    ('0', '0', '0', (others=>'0'), '0', (others=>'0'), '0',
    (others=>'0'), '0', '0', '0');
  constant c_dummy_phy16_from_wrc : t_phy_16bits_from_wrc :=
    ('0', '0', (others=>'0'), (others=>'0'), (others=>'0'),
    (others=>'0'), '0');


  -- debug CS types
  type t_dbg_ep_rxpcs is record
    fsm : std_logic_vector(2 downto 0);
  end record;

  type t_dbg_ep_pcs is record
    rx : t_dbg_ep_rxpcs;
  end record;

  type t_dbg_ep_rxpath is record
    fab_pipe  : t_fab_pipe(9 downto 0);
    dreq_pipe : std_logic_vector(9 downto 0);
    pcs_fifo_afull : std_logic;
    pcs_fifo_empty : std_logic;
    pcs_fifo_full  : std_logic;
    rxbuf_full     : std_logic;
  end record;

  type t_dbg_ep is record
    pcs    : t_dbg_ep_pcs;
    rxpath : t_dbg_ep_rxpath;
  end record;
  ------------------------------------

  constant c_epevents_sz  : integer := 29;  --how many events the endpoint generates

  component xwr_endpoint
    generic (
      g_interface_mode        : t_wishbone_interface_mode      := CLASSIC;
      g_address_granularity   : t_wishbone_address_granularity := WORD;
      g_simulation            : boolean                        := false;
      g_pcs_16bit             : boolean                        := false;
      g_records_for_phy       : boolean                        := false;
      g_tx_force_gap_length   : integer                        := 0;
      g_tx_runt_padding       : boolean                        := false;
      g_rx_buffer_size        : integer                        := 1024;
      g_with_rx_buffer        : boolean                        := true;
      g_with_flow_control     : boolean                        := true;
      g_with_timestamper      : boolean                        := true;
      g_with_dpi_classifier   : boolean                        := false;
      g_with_vlans            : boolean                        := false;
      g_with_rtu              : boolean                        := false;
      g_with_leds             : boolean                        := false;
      g_with_dmtd             : boolean                        := false;
      g_with_packet_injection : boolean                        := false;
      g_use_new_rxcrc         :	boolean                        := false;
      g_use_new_txcrc         :	boolean                        := false;
      g_with_stop_traffic     : boolean                        := false);
    port (
      clk_ref_i            : in  std_logic;
      clk_sys_i            : in  std_logic;
      clk_dmtd_i           : in  std_logic                     := '0';
      rst_sys_n_i          : in  std_logic;
      rst_ref_n_i          : in  std_logic;
      rst_dmtd_n_i         : in  std_logic;
      rst_txclk_n_i        : in  std_logic;
      rst_rxclk_n_i        : in  std_logic;
      pps_csync_p1_i       : in  std_logic                     := '0';
      pps_valid_i          : in  std_logic                     := '1';
      phy_rst_o            : out std_logic;
      phy_loopen_o         : out std_logic;
      phy_loopen_vec_o     : out std_logic_vector(2 downto 0);
      phy_tx_prbs_sel_o    : out std_logic_vector(2 downto 0);
      phy_sfp_tx_fault_i   : in  std_logic                     := '0';
      phy_sfp_los_i        : in  std_logic                     := '0';
      phy_sfp_tx_disable_o : out std_logic;
      phy_rdy_i            : in  std_logic;
      phy_ref_clk_i        : in  std_logic                     := '0';
      phy_tx_data_o        : out std_logic_vector(f_pcs_data_width(g_pcs_16bit)-1 downto 0);
      phy_tx_k_o           : out std_logic_vector(f_pcs_k_width(g_pcs_16bit)-1 downto 0);
      phy_tx_disparity_i   : in  std_logic                     := '0';
      phy_tx_enc_err_i     : in  std_logic                     := '0';
      phy_rx_data_i        : in  std_logic_vector(f_pcs_data_width(g_pcs_16bit)-1 downto 0) := (others=>'0');
      phy_rx_clk_i         : in  std_logic                     := '0';
      phy_rx_k_i           : in  std_logic_vector(f_pcs_k_width(g_pcs_16bit)-1 downto 0) := (others=>'0');
      phy_rx_enc_err_i     : in  std_logic                     := '0';
      phy_rx_bitslide_i    : in  std_logic_vector(f_pcs_bts_width(g_pcs_16bit)-1 downto 0) := (others=>'0');
      phy8_o               : out t_phy_8bits_from_wrc;
      phy8_i               : in  t_phy_8bits_to_wrc  := c_dummy_phy8_to_wrc;
      phy16_o              : out t_phy_16bits_from_wrc;
      phy16_i              : in  t_phy_16bits_to_wrc := c_dummy_phy16_to_wrc;
      gmii_tx_clk_i        : in  std_logic                     := '0';
      gmii_txd_o           : out std_logic_vector(7 downto 0);
      gmii_tx_en_o         : out std_logic;
      gmii_tx_er_o         : out std_logic;
      gmii_rx_clk_i        : in  std_logic                     := '0';
      gmii_rxd_i           : in  std_logic_vector(7 downto 0)  := x"00";
      gmii_rx_er_i         : in  std_logic                     := '0';
      gmii_rx_dv_i         : in  std_logic                     := '0';
      src_o                : out t_wrf_source_out;
      src_i                : in  t_wrf_source_in;
      snk_o                : out t_wrf_sink_out;
      snk_i                : in  t_wrf_sink_in;
      txtsu_port_id_o      : out std_logic_vector(4 downto 0);
      txtsu_frame_id_o     : out std_logic_vector(16 -1 downto 0);
      txtsu_ts_value_o     : out std_logic_vector(28 + 4 - 1 downto 0);
      txtsu_ts_incorrect_o : out std_logic;
      txtsu_stb_o          : out std_logic;
      txtsu_ack_i          : in  std_logic                     := '1';
      rtu_full_i           : in  std_logic                     := '0';
      rtu_almost_full_i    : in  std_logic                     := '0';
      rtu_rq_strobe_p1_o   : out std_logic;
      rtu_rq_abort_o       : out std_logic;
      rtu_rq_smac_o        : out std_logic_vector(48 - 1 downto 0);
      rtu_rq_dmac_o        : out std_logic_vector(48 - 1 downto 0);
      rtu_rq_vid_o         : out std_logic_vector(12 - 1 downto 0);
      rtu_rq_has_vid_o     : out std_logic;
      rtu_rq_prio_o        : out std_logic_vector(3 - 1 downto 0);
      rtu_rq_has_prio_o    : out std_logic;
      wb_i                 : in  t_wishbone_slave_in;
      wb_o                 : out t_wishbone_slave_out;
      pfilter_pclass_o     : out   std_logic_vector(7 downto 0);
      pfilter_drop_o       : out   std_logic;
      pfilter_done_o       : out   std_logic;
      fc_tx_pause_req_i    : in std_logic := '0';
      fc_tx_pause_delay_i  : in std_logic_vector(15 downto 0) := x"0000";
      fc_tx_pause_ready_o  : out std_logic;
      fc_rx_pause_start_p_o     : out std_logic;
      fc_rx_pause_quanta_o      : out std_logic_vector(15 downto 0);
      fc_rx_pause_prio_mask_o   : out std_logic_vector(7 downto 0);      
      fc_rx_buffer_occupation_o : out std_logic_vector(7 downto 0);
      inject_req_i         : in  std_logic                     := '0';
      inject_ready_o       : out std_logic;
      inject_packet_sel_i  : in  std_logic_vector(2 downto 0)  := "000";
      inject_user_value_i  : in  std_logic_vector(15 downto 0) := x"0000";
      rmon_events_o        : out std_logic_vector(c_epevents_sz-1 downto 0);
      txts_o               : out std_logic; 		-- 2013-Nov-28 peterj added for debugging/calibration
      rxts_o               : out std_logic; 		-- 2013-Nov-28 peterj added for debugging/calibration
      led_link_o           : out std_logic;
      led_act_o            : out std_logic;
      link_kill_i          : in  std_logic                     := '0';
      link_up_o            : out std_logic;
      stop_traffic_i       : in std_logic := '0';
      dbg_tx_pcs_wr_count_o     : out std_logic_vector(5+4 downto 0);
      dbg_tx_pcs_rd_count_o     : out std_logic_vector(5+4 downto 0);
      nice_dbg_o  : out t_dbg_ep);
  end component;

  component wr_endpoint
    generic (
      g_interface_mode        : t_wishbone_interface_mode      := CLASSIC;
      g_address_granularity   : t_wishbone_address_granularity := WORD;
      g_tx_force_gap_length   : integer                        := 0;
      g_tx_runt_padding       : boolean                        := false;
      g_simulation            : boolean                        := false;
      g_pcs_16bit             : boolean                        := true;
      g_rx_buffer_size        : integer                        := 1024;
      g_with_rx_buffer        : boolean                        := true;
      g_with_flow_control     : boolean                        := true;
      g_with_timestamper      : boolean                        := true;
      g_with_dpi_classifier   : boolean                        := false;
      g_with_vlans            : boolean                        := true;
      g_with_rtu              : boolean                        := true;
      g_with_leds             : boolean                        := true;
      g_with_dmtd             : boolean                        := false;
      g_with_packet_injection : boolean                        := false;
      g_use_new_rxcrc         : boolean                        := false;
      g_use_new_txcrc         : boolean                        := false;
      g_with_stop_traffic     : boolean                        := false);
    port (
      clk_ref_i            : in  std_logic;
      clk_sys_i            : in  std_logic;
      clk_dmtd_i           : in  std_logic;
      rst_sys_n_i          : in  std_logic;
      rst_ref_n_i          : in  std_logic;
      rst_dmtd_n_i         : in  std_logic;
      rst_txclk_n_i        : in  std_logic;
      rst_rxclk_n_i        : in  std_logic;
      pps_csync_p1_i       : in  std_logic;
      pps_valid_i          : in  std_logic                     := '1';
      phy_rst_o            : out std_logic;
      phy_loopen_o         : out std_logic;
      phy_loopen_vec_o     : out std_logic_vector(2 downto 0);
      phy_tx_prbs_sel_o    : out std_logic_vector(2 downto 0);
      phy_sfp_tx_fault_i   : in  std_logic                     := '0';
      phy_sfp_los_i        : in  std_logic                     := '0';
      phy_sfp_tx_disable_o : out std_logic;
      phy_rdy_i            : in  std_logic;
      phy_ref_clk_i        : in  std_logic;
      phy_tx_data_o        : out std_logic_vector(f_pcs_data_width(g_pcs_16bit)-1 downto 0);
      phy_tx_k_o           : out std_logic_vector(f_pcs_k_width(g_pcs_16bit)-1 downto 0);
      phy_tx_disparity_i   : in  std_logic;
      phy_tx_enc_err_i     : in  std_logic;
      phy_rx_data_i        : in  std_logic_vector(f_pcs_data_width(g_pcs_16bit)-1 downto 0) := (others=>'0');
      phy_rx_clk_i         : in  std_logic;
      phy_rx_k_i           : in  std_logic_vector(f_pcs_k_width(g_pcs_16bit)-1 downto 0) := (others=>'0');
      phy_rx_enc_err_i     : in  std_logic;
      phy_rx_bitslide_i    : in  std_logic_vector(f_pcs_bts_width(g_pcs_16bit)-1 downto 0) := (others=>'0');
      gmii_tx_clk_i        : in  std_logic	 									 := '0';
      gmii_txd_o           : out std_logic_vector(7 downto 0)  := x"00";
      gmii_tx_en_o         : out std_logic                     := '0';
      gmii_tx_er_o         : out std_logic                     := '0';
      gmii_rx_clk_i        : in  std_logic										 := '0';
      gmii_rxd_i           : in  std_logic_vector(7 downto 0)  := x"00";
      gmii_rx_er_i         : in  std_logic := '0';
      gmii_rx_dv_i         : in  std_logic := '0';
      src_dat_o            : out std_logic_vector(15 downto 0);
      src_adr_o            : out std_logic_vector(1 downto 0);
      src_sel_o            : out std_logic_vector(1 downto 0);
      src_cyc_o            : out std_logic;
      src_stb_o            : out std_logic;
      src_we_o             : out std_logic;
      src_stall_i          : in  std_logic;
      src_ack_i            : in  std_logic;
      src_err_i            : in  std_logic;
      snk_dat_i            : in  std_logic_vector(15 downto 0);
      snk_adr_i            : in  std_logic_vector(1 downto 0);
      snk_sel_i            : in  std_logic_vector(1 downto 0);
      snk_cyc_i            : in  std_logic;
      snk_stb_i            : in  std_logic;
      snk_we_i             : in  std_logic;
      snk_stall_o          : out std_logic;
      snk_ack_o            : out std_logic;
      snk_err_o            : out std_logic;
      snk_rty_o            : out std_logic;
      txtsu_port_id_o      : out std_logic_vector(4 downto 0);
      txtsu_frame_id_o     : out std_logic_vector(16 -1 downto 0);
      txtsu_ts_value_o     : out std_logic_vector(28 + 4 - 1 downto 0);
      txtsu_ts_incorrect_o : out std_logic;
      txtsu_stb_o          : out std_logic;
      txtsu_ack_i          : in  std_logic;
      rtu_full_i           : in  std_logic;
      rtu_almost_full_i    : in  std_logic;
      rtu_rq_strobe_p1_o   : out std_logic;
      rtu_rq_abort_o       : out std_logic;
      rtu_rq_smac_o        : out std_logic_vector(48 - 1 downto 0);
      rtu_rq_dmac_o        : out std_logic_vector(48 - 1 downto 0);
      rtu_rq_vid_o         : out std_logic_vector(12 - 1 downto 0);
      rtu_rq_has_vid_o     : out std_logic;
      rtu_rq_prio_o        : out std_logic_vector(3 - 1 downto 0);
      rtu_rq_has_prio_o    : out std_logic;
      wb_cyc_i             : in  std_logic;
      wb_stb_i             : in  std_logic;
      wb_we_i              : in  std_logic;
      wb_sel_i             : in  std_logic_vector(3 downto 0);
      wb_adr_i             : in  std_logic_vector(7 downto 0);
      wb_dat_i             : in  std_logic_vector(31 downto 0);
      wb_dat_o             : out std_logic_vector(31 downto 0);
      wb_ack_o             : out std_logic;
      wb_stall_o           : out std_logic;
      pfilter_pclass_o     : out   std_logic_vector(7 downto 0);
      pfilter_drop_o       : out   std_logic;
      pfilter_done_o       : out   std_logic;
      fc_tx_pause_req_i    : in std_logic := '0';
      fc_tx_pause_delay_i  : in std_logic_vector(15 downto 0) := x"0000";
      fc_tx_pause_ready_o  : out std_logic;
      fc_rx_pause_start_p_o     : out std_logic;
      fc_rx_pause_quanta_o      : out std_logic_vector(15 downto 0);
      fc_rx_pause_prio_mask_o   : out std_logic_vector(7 downto 0);     
      fc_rx_buffer_occupation_o : out std_logic_vector(7 downto 0);
      inject_req_i         : in  std_logic                     := '0';
      inject_ready_o       : out std_logic;
      inject_packet_sel_i  : in  std_logic_vector(2 downto 0)  := "000";
      inject_user_value_i  : in  std_logic_vector(15 downto 0) := x"0000";
      rmon_events_o        : out std_logic_vector(c_epevents_sz-1 downto 0);
      txts_o               : out std_logic; 		-- 2013-Nov-28 peterj added for debugging/calibration
      rxts_o               : out std_logic; 		-- 2013-Nov-28 peterj added for debugging/calibration
      led_link_o           : out std_logic;
      led_act_o            : out std_logic;
      link_kill_i          : in  std_logic                     := '0';
      link_up_o            : out std_logic;
      stop_traffic_i       : in std_logic := '0';
      dbg_tx_pcs_wr_count_o     : out std_logic_vector(5+4 downto 0);
      dbg_tx_pcs_rd_count_o     : out std_logic_vector(5+4 downto 0);
      nice_dbg_o  : out t_dbg_ep);
  end component;
  
  constant c_xwr_endpoint_sdb : t_sdb_device := (
    abi_class     => x"0000",              -- undocumented device
    abi_ver_major => x"01",
    abi_ver_minor => x"01",
    wbd_endian    => c_sdb_endian_big,
    wbd_width     => x"7",                 -- 8/16/32-bit port granularity
    sdb_component => (
      addr_first  => x"0000000000000000",
      addr_last   => x"00000000000000ff",
      product     => (
        vendor_id => x"000000000000CE42",  -- CERN
        device_id => x"650c2d4f",
        version   => x"00000002",
        date      => x"20121116",
        name      => "WR-Endpoint        ")));

end endpoint_pkg;

package body endpoint_pkg is

  function f_pcs_data_width(pcs_16 : boolean)
    return integer is
  begin
    if (pcs_16) then
      return 16;
    else
      return 8;
    end if;
  end function;

  function f_pcs_k_width(pcs_16 : boolean)
    return integer is
  begin
    if (pcs_16) then
      return 2;
    else
      return 1;
    end if;
  end function;

  function f_pcs_bts_width(pcs_16 : boolean)
    return integer is
  begin
    if (pcs_16) then
      return 5;
    else
      return 4;
    end if;
  end function;

  function f_pcs_clock_rate(pcs_16 : boolean)
    return integer is
  begin
    if (pcs_16) then
      return 62500000;
    else
      return 125000000;
    end if;
  end function;

end package body endpoint_pkg;
