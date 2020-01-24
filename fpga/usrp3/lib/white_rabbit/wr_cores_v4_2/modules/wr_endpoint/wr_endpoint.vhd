-------------------------------------------------------------------------------
-- Title      : 1000base-X MAC/Endpoint
-- Project    : White Rabbit
-------------------------------------------------------------------------------
-- File       : wr_endpoint.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2010-04-26
-- Last update: 2017-02-20
-- Platform   : FPGA-generics
-- Standard   : VHDL
-------------------------------------------------------------------------------
-- Description: Module implements the top level for the White Rabbit Endpoint
-- It's basically an extended Ethernet MAC providing extra timing/switch-specific
-- features such as:
-- - VLANs: inserting/removing tags (for ACCESS/TRUNK port support)
-- - RX/TX precise timestaping
-- - full PCS for optical Gigabit Ethernet 
-- - decodes MAC addresses, VIDs and priorities and passes them to the RTU.
-- Refer to the manual for more details.
-------------------------------------------------------------------------------
--
-- Copyright (c) 2011 - 2017 CERN / BE-CO-HT
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

use work.gencores_pkg.all;
use work.endpoint_private_pkg.all;
use work.endpoint_pkg.all;
use work.ep_wbgen2_pkg.all;
use work.wr_fabric_pkg.all;
use work.wishbone_pkg.all;

entity wr_endpoint is
  
  generic (
    g_interface_mode        : t_wishbone_interface_mode      := CLASSIC;
    g_address_granularity   : t_wishbone_address_granularity := WORD;
    g_tx_force_gap_length   : integer                        := 0;
    g_tx_runt_padding       : boolean                        := true;
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
    g_with_stop_traffic     : boolean                        := false
    );
  port (

-------------------------------------------------------------------------------
-- Clocks
-------------------------------------------------------------------------------

-- Endpoint transmit reference clock. Must be 125 MHz +- 100 ppm
    clk_ref_i : in std_logic;

-- reference clock / 2 (62.5 MHz, in-phase with refclk)
    clk_sys_i : in std_logic;

-- DMTD offset clock for phase tracking - used only if g_with_dmtd == true
    clk_dmtd_i : in std_logic;

-- resets for various clock domains
    rst_sys_n_i   : in std_logic;
    rst_ref_n_i   : in std_logic;
    rst_dmtd_n_i  : in std_logic;
    rst_txclk_n_i : in std_logic;
    rst_rxclk_n_i : in std_logic;

-- PPS input (1 clk_ref_i cycle HI) for synchronizing timestamp counter
    pps_csync_p1_i : in std_logic;

-- PPS valid input (clk_ref_i domain), when 1, the external PPS generator/servo
-- is not adjusting the time scale, so we can safely timestamp.
    pps_valid_i : in std_logic := '1';

-------------------------------------------------------------------------------
-- PHY Interace (8/16 bit PCS)
-------------------------------------------------------------------------------    

    phy_rst_o            : out std_logic;
    phy_loopen_o         : out std_logic;
    phy_loopen_vec_o     : out std_logic_vector(2 downto 0);
    phy_tx_prbs_sel_o    : out std_logic_vector(2 downto 0);
    phy_sfp_tx_fault_i   : in  std_logic;
    phy_sfp_los_i        : in  std_logic;
    phy_sfp_tx_disable_o : out std_logic;
    phy_rdy_i            : in  std_logic;

    phy_ref_clk_i      : in  std_logic;
    phy_tx_data_o      : out std_logic_vector(f_pcs_data_width(g_pcs_16bit)-1 downto 0);
    phy_tx_k_o         : out std_logic_vector(f_pcs_k_width(g_pcs_16bit)-1 downto 0);
    phy_tx_disparity_i : in  std_logic;
    phy_tx_enc_err_i   : in  std_logic;

    phy_rx_data_i     : in std_logic_vector(f_pcs_data_width(g_pcs_16bit)-1 downto 0);
    phy_rx_clk_i      : in std_logic;
    phy_rx_k_i        : in std_logic_vector(f_pcs_k_width(g_pcs_16bit)-1 downto 0);
    phy_rx_enc_err_i  : in std_logic;
    phy_rx_bitslide_i : in std_logic_vector(f_pcs_bts_width(g_pcs_16bit)-1 downto 0);

-------------------------------------------------------------------------------
-- GMII Interface (8-bit)
-------------------------------------------------------------------------------

    gmii_tx_clk_i : in  std_logic := '0';
    gmii_txd_o    : out std_logic_vector(7 downto 0);
    gmii_tx_en_o  : out std_logic;
    gmii_tx_er_o  : out std_logic;

    gmii_rx_clk_i : in std_logic                    := '0';
    gmii_rxd_i    : in std_logic_vector(7 downto 0) := x"00";
    gmii_rx_er_i  : in std_logic                    := '0';
    gmii_rx_dv_i  : in std_logic                    := '0';

    ---------------------------------------------------------------------------
    -- Wishbone I/O
    ---------------------------------------------------------------------------

    src_dat_o   : out std_logic_vector(15 downto 0);
    src_adr_o   : out std_logic_vector(1 downto 0);
    src_sel_o   : out std_logic_vector(1 downto 0);
    src_cyc_o   : out std_logic;
    src_stb_o   : out std_logic;
    src_we_o    : out std_logic;
    src_stall_i : in  std_logic;
    src_ack_i   : in  std_logic;
    src_err_i   : in  std_logic;

    snk_dat_i   : in  std_logic_vector(15 downto 0);
    snk_adr_i   : in  std_logic_vector(1 downto 0);
    snk_sel_i   : in  std_logic_vector(1 downto 0);
    snk_cyc_i   : in  std_logic;
    snk_stb_i   : in  std_logic;
    snk_we_i    : in  std_logic;
    snk_stall_o : out std_logic;
    snk_ack_o   : out std_logic;
    snk_err_o   : out std_logic;
    snk_rty_o   : out std_logic;

-------------------------------------------------------------------------------
-- TX timestamping unit interface
-------------------------------------------------------------------------------  

-- Port ID value
    txtsu_port_id_o  : out std_logic_vector(4 downto 0);
-- Frame ID value
    txtsu_frame_id_o : out std_logic_vector(16 -1 downto 0);

-- TX Timestamp and correctness info
    txtsu_ts_value_o     : out std_logic_vector(28 + 4 - 1 downto 0);
    txtsu_ts_incorrect_o : out std_logic;

-- TX timestamp strobe: HI tells the TX timestamping unit that a timestamp is
-- available on txtsu_ts_value_o, txtsu_fid_o andd txtsu_port_id_o. The correctness
-- of the timestamping is indiacted on txtsu_ts_incorrect_o. Line remains HI
-- until assertion of txtsu_ack_i.
    txtsu_stb_o : out std_logic;

-- TX timestamp acknowledge: HI indicates that TXTSU has successfully received
-- the timestamp
    txtsu_ack_i : in std_logic;

-------------------------------------------------------------------------------
-- RTU interface
-------------------------------------------------------------------------------

-- 1 indicates that coresponding RTU port is full.
    rtu_full_i : in std_logic;

-- 1 indicates that coresponding RTU port is almost full.
    rtu_almost_full_i : in std_logic;

-- request strobe, single HI pulse begins evaluation of the request. 
    rtu_rq_strobe_p1_o : out std_logic;
    
    rtu_rq_abort_o : out std_logic;

-- source and destination MAC addresses extracted from the packet header
    rtu_rq_smac_o : out std_logic_vector(48 - 1 downto 0);
    rtu_rq_dmac_o : out std_logic_vector(48 - 1 downto 0);

-- VLAN id (extracted from the header for TRUNK ports and assigned by the port
-- for ACCESS ports)
    rtu_rq_vid_o : out std_logic_vector(12 - 1 downto 0);

-- HI means that packet has valid assigned a valid VID (low - packet is untagged)
    rtu_rq_has_vid_o : out std_logic;

-- packet priority (either extracted from the header or assigned per port).
    rtu_rq_prio_o : out std_logic_vector(3 - 1 downto 0);

-- HI indicates that packet has assigned priority.
    rtu_rq_has_prio_o : out std_logic;

-------------------------------------------------------------------------------   
-- Wishbone bus
-------------------------------------------------------------------------------

    wb_cyc_i   : in  std_logic;
    wb_stb_i   : in  std_logic;
    wb_we_i    : in  std_logic;
    wb_sel_i   : in  std_logic_vector(3 downto 0);
    wb_adr_i   : in  std_logic_vector(7 downto 0);
    wb_dat_i   : in  std_logic_vector(31 downto 0);
    wb_dat_o   : out std_logic_vector(31 downto 0);
    wb_ack_o   : out std_logic;
    wb_stall_o : out std_logic;

-------------------------------------------------------------------------------
-- direct output of packet filter  (for TRU/HW-RSTP)
-------------------------------------------------------------------------------

   pfilter_pclass_o : out std_logic_vector(7 downto 0);
   pfilter_drop_o   : out std_logic;
   pfilter_done_o   : out std_logic;

-------------------------------------------------------------------------------
-- control of PAUSE sending (ML: not used and not tested... TRU uses packet injection) -- 
-------------------------------------------------------------------------------

   fc_tx_pause_req_i   : in  std_logic                     := '0';
   fc_tx_pause_delay_i : in  std_logic_vector(15 downto 0) := x"0000";
   fc_tx_pause_ready_o : out std_logic;

-------------------------------------------------------------------------------
-- information about received PAUSE (for SWcore)
-------------------------------------------------------------------------------

   fc_rx_pause_start_p_o   : out std_logic;
   fc_rx_pause_quanta_o    : out std_logic_vector(15 downto 0);
   fc_rx_pause_prio_mask_o : out std_logic_vector(7 downto 0);
   fc_rx_buffer_occupation_o : out std_logic_vector(7 downto 0);
-------------------------------------------------------------------------------
-- Packet Injection Interface (for TRU/HW-RSTP)
-------------------------------------------------------------------------------

-- injection request: triggers transmission of the packet to be injected,
-- allowed when inject_ready = 1
    inject_req_i : in std_logic := '0';

-- injection ready flag: when true, user application can request asynchronous
-- injection of a predefined packet
    inject_ready_o : out std_logic;

-- injection template selection (8 available)
    inject_packet_sel_i : in std_logic_vector(2 downto 0) := "000";

-- user-defined value to be embedded in the injected packet at a predefined
-- location
    inject_user_value_i : in std_logic_vector(15 downto 0) := x"0000";

-------------------------------------------------------------------------------
-- Misc stuff
-------------------------------------------------------------------------------
    rmon_events_o : out std_logic_vector(c_epevents_sz-1 downto 0);

    txts_o     : out std_logic; 		                -- 2013-Nov-28 peterj added for debugging/calibration
    rxts_o     : out std_logic; 		                -- 2013-Nov-28 peterj added for debugging/calibration

    led_link_o : out std_logic;
    led_act_o  : out std_logic;

-- HI physically kills the link (turn of laser)
    link_kill_i : in std_logic := '0';

-- HI indicates that link is up (so cable connected), LOW indicates that link is faulty 
-- (e.g.: cable disconnected)
    link_up_o : out std_logic;

    stop_traffic_i : in std_logic := '0';

    dbg_tx_pcs_wr_count_o     : out std_logic_vector(5+4 downto 0);
    dbg_tx_pcs_rd_count_o     : out std_logic_vector(5+4 downto 0);
    nice_dbg_o  : out t_dbg_ep
    );

end wr_endpoint;

architecture syn of wr_endpoint is

  constant c_zeros : std_logic_vector(63 downto 0) := (others => '0');
  constant c_ones  : std_logic_vector(63 downto 0) := (others => '0');

-------------------------------------------------------------------------------
  component dmtd_phase_meas
    generic (
      g_deglitcher_threshold : integer;
      g_counter_bits         : integer);
    port (
      rst_sys_n_i    : in  std_logic;
      rst_dmtd_n_i   : in  std_logic;
      clk_sys_i      : in  std_logic;
      clk_a_i        : in  std_logic;
      clk_b_i        : in  std_logic;
      clk_dmtd_i     : in  std_logic;
      en_i           : in  std_logic;
      navg_i         : in  std_logic_vector(11 downto 0);
      phase_meas_o   : out std_logic_vector(31 downto 0);
      phase_meas_p_o : out std_logic);
  end component;

-------------------------------------------------------------------------------
-- TX FRAMER -> TX PCS signals
-------------------------------------------------------------------------------

  signal txpcs_fab   : t_ep_internal_fabric;
  signal txpcs_dreq  : std_logic;
  signal txpcs_error : std_logic;
  signal txpcs_busy  : std_logic;

-------------------------------------------------------------------------------
-- Timestamping/OOB signals
-------------------------------------------------------------------------------

  signal txoob_fid_value : std_logic_vector(15 downto 0);
  signal txoob_fid_stb   : std_logic;

  signal txpcs_timestamp_trigger_p_a : std_logic;

  signal txts_timestamp_stb   : std_logic;
  signal txts_timestamp_valid : std_logic;
  signal txts_timestamp_value : std_logic_vector(31 downto 0);


  signal rxpcs_timestamp_stb         : std_logic;
  signal rxpcs_timestamp_trigger_p_a : std_logic;
  signal rxpcs_timestamp_valid       : std_logic;
  signal rxpcs_timestamp_value       : std_logic_vector(31 downto 0);


-------------------------------------------------------------------------------
-- RX PCS -> RX DEFRAMER signals
-------------------------------------------------------------------------------

  signal rxpcs_fab             : t_ep_internal_fabric;
  signal rxpath_fab            : t_ep_internal_fabric;
  signal rxpcs_busy            : std_logic;
  signal rxpcs_fifo_almostfull : std_logic;

-------------------------------------------------------------------------------
-- WB slave signals
-------------------------------------------------------------------------------

  signal regs_fromwb     : t_ep_out_registers;
  signal regs_towb       : t_ep_in_registers;
  signal regs_towb_ep    : t_ep_in_registers;
  signal regs_towb_tsu   : t_ep_in_registers;
  signal regs_towb_rpath : t_ep_in_registers;
  signal regs_towb_tpath : t_ep_in_registers;
  signal regs_towb_dmtd  : t_ep_in_registers;

-------------------------------------------------------------------------------
-- flow control signals
-------------------------------------------------------------------------------

  signal txfra_flow_enable : std_logic;
  signal rxfra_pause_p     : std_logic;
  signal rxfra_pause_delay : std_logic_vector(15 downto 0);

  signal txfra_pause_req   : std_logic;
  signal txfra_pause_ready : std_logic;
  signal txfra_pause_delay : std_logic_vector(15 downto 0);

  signal link_ok : std_logic;

  signal txfra_enable : std_logic;
  signal mdio_addr    : std_logic_vector(15 downto 0);

  signal sink_in  : t_wrf_sink_in;
  signal sink_out : t_wrf_sink_out;

  signal src_in  : t_wrf_source_in;
  signal src_out : t_wrf_source_out;

  signal rst_n_rx : std_logic;

  signal wb_in  : t_wishbone_slave_in;
  signal wb_out : t_wishbone_slave_out;

  signal extended_ADDR : std_logic_vector(c_wishbone_address_width-1 downto 0);

  signal phase_meas    : std_logic_vector(31 downto 0);
  signal phase_meas_p  : std_logic;
  signal validity_cntr : unsigned(1 downto 0);
  signal r_dmcr_en     : std_logic;
  signal r_dmcr_n_avg  : std_logic_vector(11 downto 0);


  signal rtu_rq               : t_ep_internal_rtu_request;
  signal dvalid_tx, dvalid_rx : std_logic;

-------------------------------------------------------------------------------
-- TRU stuff
-------------------------------------------------------------------------------
  signal ep_ctrl        : std_logic;
  signal pfilter_pclass : std_logic_vector(7 downto 0);
  signal pfilter_drop   : std_logic;
  signal pfilter_done   : std_logic;
  signal tx_pclass      : std_logic_vector(7 downto 0);

-------------------------------------------------------------------------------
-- RMON signals
-------------------------------------------------------------------------------
  signal pcs_rmon     : t_rmon_triggers;
  signal rx_path_rmon : t_rmon_triggers;
  signal rmon         : t_rmon_triggers;
 
-------------------------------------------------------------------------------
-- chipscope (for desperates)
-------------------------------------------------------------------------------
  signal CONTROL0                   : std_logic_vector(35 downto 0);
  signal TRIG0, TRIG1, TRIG2, TRIG3 : std_logic_vector(31 downto 0);

  component chipscope_icon
    port (
      CONTROL0 : inout std_logic_vector(35 downto 0));
  end component;

  component chipscope_ila
    port (
      CONTROL : inout std_logic_vector(35 downto 0);
      CLK     : in    std_logic;
      TRIG0   : in    std_logic_vector(31 downto 0);
      TRIG1   : in    std_logic_vector(31 downto 0);
      TRIG2   : in    std_logic_vector(31 downto 0);
      TRIG3   : in    std_logic_vector(31 downto 0));
  end component;

begin

  rst_n_rx  <= rst_rxclk_n_i and phy_rdy_i;

-------------------------------------------------------------------------------
-- 1000Base-X PCS
-------------------------------------------------------------------------------

  mdio_addr <= regs_fromwb.mdio_asr_phyad_o & regs_fromwb.mdio_cr_addr_o;

  U_PCS_1000BASEX : ep_1000basex_pcs
    generic map (
      g_simulation => g_simulation,
      g_16bit      => g_pcs_16bit)
    port map (
      rst_sys_n_i   => rst_sys_n_i,
      rst_rxclk_n_i => rst_rxclk_n_i,
      rst_txclk_n_i => rst_txclk_n_i,
      clk_sys_i     => clk_sys_i,

      rxpcs_fab_o             => rxpcs_fab,
      rxpcs_busy_o            => rxpcs_busy,
      rxpcs_fifo_almostfull_i => rxpcs_fifo_almostfull,

      rxpcs_timestamp_trigger_p_a_o => rxpcs_timestamp_trigger_p_a,
      rxpcs_timestamp_i             => rxpcs_timestamp_value,
      rxpcs_timestamp_stb_i         => rxpcs_timestamp_stb,
      rxpcs_timestamp_valid_i       => rxpcs_timestamp_valid,

      txpcs_fab_i   => txpcs_fab,
      txpcs_busy_o  => txpcs_busy,
      txpcs_dreq_o  => txpcs_dreq,
      txpcs_error_o => txpcs_error,

      txpcs_timestamp_trigger_p_a_o => txpcs_timestamp_trigger_p_a,

      link_ok_o  => link_ok,
      link_ctr_i => ep_ctrl,

      serdes_rst_o             => phy_rst_o,
      serdes_loopen_o          => phy_loopen_o,
      serdes_loopen_vec_o      => phy_loopen_vec_o,
      serdes_tx_prbs_sel_o     => phy_tx_prbs_sel_o,
      serdes_sfp_tx_fault_i    => phy_sfp_tx_fault_i,
      serdes_sfp_los_i         => phy_sfp_los_i,
      serdes_sfp_tx_disable_o  => phy_sfp_tx_disable_o,
      serdes_rdy_i             => phy_rdy_i,

      serdes_tx_clk_i       => phy_ref_clk_i,
      serdes_tx_data_o      => phy_tx_data_o,
      serdes_tx_k_o         => phy_tx_k_o,
      serdes_tx_disparity_i => phy_tx_disparity_i,
      serdes_tx_enc_err_i   => phy_tx_enc_err_i,
      serdes_rx_data_i      => phy_rx_data_i,
      serdes_rx_clk_i       => phy_rx_clk_i,
      serdes_rx_k_i         => phy_rx_k_i,
      serdes_rx_enc_err_i   => phy_rx_enc_err_i,
      serdes_rx_bitslide_i  => phy_rx_bitslide_i,

      rmon_o => pcs_rmon,

      mdio_addr_i  => mdio_addr,
      mdio_data_i  => regs_fromwb.mdio_cr_data_o,
      mdio_data_o  => regs_towb_ep.mdio_asr_rdata_i,
      mdio_stb_i   => regs_fromwb.mdio_cr_data_wr_o,
      mdio_rw_i    => regs_fromwb.mdio_cr_rw_o,
      mdio_ready_o => regs_towb_ep.mdio_asr_ready_i,
      dbg_tx_pcs_wr_count_o => dbg_tx_pcs_wr_count_o,
      dbg_tx_pcs_rd_count_o => dbg_tx_pcs_rd_count_o,
      nice_dbg_o   => nice_dbg_o.pcs);


-------------------------------------------------------------------------------
-- TX FRAMER
-------------------------------------------------------------------------------

--  txfra_enable <= link_ok and regs_fromwb.ecr_tx_en_o;

--   txfra_pause_req <= '0';

  U_Tx_Path : ep_tx_path
    generic map (
      g_with_packet_injection => g_with_packet_injection,
      g_with_vlans            => g_with_vlans,
      g_with_timestamper      => g_with_timestamper,
      g_force_gap_length      => g_tx_force_gap_length,
      g_runt_padding          => g_tx_runt_padding,
      g_use_new_crc           => g_use_new_txcrc)
    port map (
      clk_sys_i        => clk_sys_i,
      rst_n_i          => rst_sys_n_i,
      pcs_error_i      => txpcs_error,
      pcs_busy_i       => txpcs_busy,
      pcs_fab_o        => txpcs_fab,
      pcs_dreq_i       => txpcs_dreq,
      snk_i            => sink_in,
      snk_o            => sink_out,
      fc_pause_req_i   => txfra_pause_req,
      fc_pause_ready_o => txfra_pause_ready,
      fc_pause_delay_i => txfra_pause_delay,
      fc_flow_enable_i => txfra_flow_enable,
      ep_ctrl_i        => ep_ctrl,
      regs_i           => regs_fromwb,
      regs_o           => regs_towb_tpath,

      txts_timestamp_i       => txts_timestamp_value,
      txts_timestamp_valid_i => txts_timestamp_valid,

      txtsu_port_id_o      => txtsu_port_id_o,
      txtsu_fid_o          => txtsu_frame_id_o,
      txtsu_ts_value_o     => txtsu_ts_value_o,
      txtsu_ts_incorrect_o => txtsu_ts_incorrect_o,
      txtsu_stb_o          => txtsu_stb_o,
      txtsu_ack_i          => txtsu_ack_i,

      inject_req_i        => inject_req_i,
      inject_user_value_i => inject_user_value_i,
      inject_packet_sel_i => inject_packet_sel_i,
      inject_ready_o      => inject_ready_o);


  txfra_flow_enable <= '1';

  sink_in.dat <= snk_dat_i;
  sink_in.adr <= snk_adr_i;
  sink_in.sel <= snk_sel_i;
  sink_in.cyc <= snk_cyc_i;
  sink_in.stb <= snk_stb_i;
  sink_in.we  <= snk_we_i;
  snk_stall_o <= sink_out.stall;
  snk_ack_o   <= sink_out.ack;
  snk_err_o   <= sink_out.err;
  snk_rty_o   <= sink_out.rty;


-------------------------------------------------------------------------------
-- RX deframer
-------------------------------------------------------------------------------

  U_Rx_Path : ep_rx_path
    generic map (
      g_with_vlans          => g_with_vlans,
      g_with_dpi_classifier => g_with_dpi_classifier,
      g_with_rtu            => g_with_rtu,
      g_with_rx_buffer      => g_with_rx_buffer,
      g_rx_buffer_size      => g_rx_buffer_size,
      g_use_new_crc         => g_use_new_rxcrc)
    port map (
      clk_sys_i => clk_sys_i,
      clk_rx_i  => phy_rx_clk_i,

      rst_n_sys_i => rst_sys_n_i,
      rst_n_rx_i  => rst_n_rx,

      pcs_fab_i             => rxpath_fab,
      pcs_fifo_almostfull_o => rxpcs_fifo_almostfull,
      pcs_busy_i            => rxpcs_busy,

      fc_pause_p_o         => fc_rx_pause_start_p_o,  --rxfra_pause_p,
      fc_pause_quanta_o    => fc_rx_pause_quanta_o,   --rxfra_pause_delay,
      fc_pause_prio_mask_o => fc_rx_pause_prio_mask_o,
      fc_buffer_occupation_o => fc_rx_buffer_occupation_o,

      rmon_o => rx_path_rmon,
      regs_i => regs_fromwb,
      regs_o => regs_towb_rpath,

      pfilter_pclass_o => pfilter_pclass,
      pfilter_drop_o   => pfilter_drop,
      pfilter_done_o   => pfilter_done,

      rtu_full_i     => rtu_full_i,
      rtu_rq_o       => rtu_rq,
      rtu_rq_valid_o => rtu_rq_strobe_p1_o,
      rtu_rq_abort_o => rtu_rq_abort_o,
      src_wb_o       => src_out,
      src_wb_i       => src_in,
      nice_dbg_o     => nice_dbg_o.rxpath);


  rtu_rq_smac_o     <= rtu_rq.smac;
  rtu_rq_dmac_o     <= rtu_rq.dmac;
  rtu_rq_vid_o      <= rtu_rq.vid;
  rtu_rq_prio_o     <= rtu_rq.prio;
  rtu_rq_has_vid_o  <= rtu_rq.has_vid;
  rtu_rq_has_prio_o <= rtu_rq.has_prio;

  src_dat_o    <= src_out.dat;
  src_adr_o    <= src_out.adr;
  src_sel_o    <= src_out.sel;
  src_cyc_o    <= src_out.cyc;
  src_stb_o    <= src_out.stb;
  src_we_o     <= src_out.we;
  src_in.stall <= src_stall_i;
  src_in.ack   <= src_ack_i;
  src_in.err   <= src_err_i;
  src_in.rty   <= '0';

-------------------------------------------------------------------------------
-- Flow control unit
-------------------------------------------------------------------------------

  --U_FLOW_CTL : ep_flow_control
  --  port map (
  --    clk_sys_i => clk_sys_i,
  --    rst_n_i   => rst_n_i,

  --    rx_pause_p1_i    => rxfra_pause_p,
  --    rx_pause_delay_i => rxfra_pause_delay,

  --    tx_pause_o       => txfra_pause,
  --    tx_pause_delay_o => txfra_pause_delay,
  --    tx_pause_ack_i   => txfra_pause_ack,

  --    tx_flow_enable_o => txfra_flow_enable,

  --    rx_buffer_used_i => rx_buffer_used,

  --    ep_fcr_txpause_i   => regs.fcr_txpause_o,
  --    ep_fcr_rxpause_i   => regs.fcr_rxpause_o,
  --    ep_fcr_tx_thr_i    => regs.fcr_tx_thr_o,
  --    ep_fcr_tx_quanta_i => regs.fcr_tx_quanta_o,
  --    rmon_rcvd_pause_o  => rmon.rx_pause,
  --    rmon_sent_pause_o  => rmon.tx_pause
  --    );

-------------------------------------------------------------------------------
-- Timestamping unit
-------------------------------------------------------------------------------

  U_EP_TSU : ep_timestamping_unit
    generic map (
      g_timestamp_bits_r => 28,
      g_timestamp_bits_f => 4,
      g_ref_clock_rate   => f_pcs_clock_rate(g_pcs_16bit))
    port map (
      clk_ref_i      => clk_ref_i,
      clk_rx_i       => phy_rx_clk_i,
      clk_sys_i      => clk_sys_i,
      rst_n_rx_i     => rst_rxclk_n_i,
      rst_n_sys_i    => rst_sys_n_i,
      rst_n_ref_i    => rst_ref_n_i,
      pps_csync_p1_i => pps_csync_p1_i,
      pps_valid_i    => pps_valid_i,

      tx_timestamp_trigger_p_a_i => txpcs_timestamp_trigger_p_a,
      rx_timestamp_trigger_p_a_i => rxpcs_timestamp_trigger_p_a,

      rxts_timestamp_o       => rxpcs_timestamp_value,
      rxts_timestamp_valid_o => rxpcs_timestamp_valid,
      rxts_timestamp_stb_o   => rxpcs_timestamp_stb,

      txts_timestamp_o       => txts_timestamp_value,
      txts_timestamp_valid_o => txts_timestamp_valid,

      txts_o                 => txts_o,                   -- 2013-Nov-28 peterj added for debugging/calibration
      rxts_o                 => rxts_o, 		              -- 2013-Nov-28 peterj added for debugging/calibration

      regs_i => regs_fromwb,
      regs_o => regs_towb_tsu);


-------------------------------------------------------------------------------
-- Wishbone controller & IO registers
-------------------------------------------------------------------------------

  extended_ADDR <= std_logic_vector(resize(unsigned(wb_adr_i), c_wishbone_address_width));

  U_Slave_adapter : wb_slave_adapter
    generic map (
      g_master_use_struct  => true,
      g_master_mode        => CLASSIC,
      g_master_granularity => WORD,
      g_slave_use_struct   => false,
      g_slave_mode         => g_interface_mode,
      g_slave_granularity  => g_address_granularity)
    port map (
      clk_sys_i  => clk_sys_i,
      rst_n_i    => rst_sys_n_i,
      sl_adr_i   => extended_ADDR,
      sl_dat_i   => wb_dat_i,
      sl_sel_i   => wb_sel_i,
      sl_cyc_i   => wb_cyc_i,
      sl_stb_i   => wb_stb_i,
      sl_we_i    => wb_we_i,
      sl_dat_o   => wb_dat_o,
      sl_ack_o   => wb_ack_o,
      sl_stall_o => wb_stall_o,
      master_i   => wb_out,
      master_o   => wb_in);

  U_WB_SLAVE : ep_wishbone_controller
    port map (
      rst_n_i    => rst_sys_n_i,
      clk_sys_i  => clk_sys_i,
      wb_adr_i   => wb_in.adr(4 downto 0),
      wb_dat_i   => wb_in.dat,
      wb_dat_o   => wb_out.dat,
      wb_cyc_i   => wb_in.cyc,
      wb_sel_i   => wb_in.sel,
      wb_stb_i   => wb_in.stb,
      wb_we_i    => wb_in.we,
      wb_ack_o   => wb_out.ack,
      wb_stall_o => open,

      tx_clk_i => clk_ref_i,
      rx_clk_i => phy_rx_clk_i,

      regs_o => regs_fromwb,
      regs_i => regs_towb
      );     

  wb_out.stall <= '0';
  wb_out.rty   <= '0';
  wb_out.err   <= '0';
  wb_out.int   <= '0';

  regs_towb <= regs_towb_ep or regs_towb_tsu or regs_towb_rpath or regs_towb_tpath or regs_towb_dmtd;


  p_link_activity : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then

      if(rst_sys_n_i = '0') or
        (regs_fromwb.dsr_lact_o = '1' and regs_fromwb.dsr_lact_load_o = '1') then
        regs_towb_ep.dsr_lact_i <= '0';
      else
        regs_towb_ep.dsr_lact_i <= dvalid_rx or dvalid_tx;
      end if;
    end if;
  end process;

  -- drive unused regs_towb_ep signals
  regs_towb_ep.ecr_feat_vlan_i           <= '0';
  regs_towb_ep.ecr_feat_dmtd_i           <= '0';
  regs_towb_ep.ecr_feat_ptp_i            <= '0';
  regs_towb_ep.ecr_feat_dpi_i            <= '0';
  regs_towb_ep.tscr_cs_done_i            <= '0';
  regs_towb_ep.tscr_rx_cal_result_i      <= '0';
  regs_towb_ep.tcar_pcp_map_i            <= (others => '0');
  regs_towb_ep.dsr_lstatus_i             <= '0';
  regs_towb_ep.dmcr_en_i                 <= '0';
  regs_towb_ep.dmcr_n_avg_i              <= (others => '0');
  regs_towb_ep.inj_ctrl_pic_conf_ifg_i   <= (others => '0');
  regs_towb_ep.inj_ctrl_pic_conf_sel_i   <= (others => '0');
  regs_towb_ep.inj_ctrl_pic_conf_valid_i <= '0';
  regs_towb_ep.inj_ctrl_pic_mode_id_i    <= (others => '0');
  regs_towb_ep.inj_ctrl_pic_mode_valid_i <= '0';
  regs_towb_ep.inj_ctrl_pic_ena_i        <= '0';

-------------------------------------------------------------------------------
-- DMTD phase meter
------------------------------------------------------------------------------  

  gen_with_dmtd : if(g_with_dmtd) generate
    U_DMTD : dmtd_phase_meas
      generic map (
        g_counter_bits         => 14,
        g_deglitcher_threshold => 1000)
      port map (
        clk_sys_i => clk_sys_i,

        clk_a_i    => phy_ref_clk_i,
        clk_b_i    => phy_rx_clk_i,
        clk_dmtd_i => clk_dmtd_i,

        rst_sys_n_i  => rst_sys_n_i,
        rst_dmtd_n_i => rst_dmtd_n_i,

        en_i           => r_dmcr_en,
        navg_i         => r_dmcr_n_avg,
        phase_meas_o   => phase_meas,
        phase_meas_p_o => phase_meas_p);



    regs_towb_dmtd.dmcr_en_i    <= r_dmcr_en;
    regs_towb_dmtd.dmcr_n_avg_i <= r_dmcr_n_avg;

    p_dmtd_update : process(clk_sys_i)
    begin
      if rising_edge(clk_sys_i) then
        if rst_sys_n_i = '0' then
          validity_cntr              <= (others => '0');
          regs_towb_ep.dmsr_ps_rdy_i <= '0';
        else

          if(regs_fromwb.dmcr_en_load_o = '1') then
            r_dmcr_en    <= regs_fromwb.dmcr_en_o;
            r_dmcr_n_avg <= regs_fromwb.dmcr_n_avg_o;
          end if;

          if(r_dmcr_en = '0') then
            validity_cntr              <= (others => '0');
            regs_towb_ep.dmsr_ps_rdy_i <= '0';
          elsif(regs_fromwb.dmsr_ps_rdy_o = '1' and regs_fromwb.dmsr_ps_rdy_load_o = '1') then
            regs_towb_ep.dmsr_ps_rdy_i <= '0';
          elsif(phase_meas_p = '1') then

            if(validity_cntr = "11") then
              regs_towb_ep.dmsr_ps_rdy_i <= '1';
              regs_towb_ep.dmsr_ps_val_i <= phase_meas(23 downto 0);  -- discard few
            else
              regs_towb_ep.dmsr_ps_rdy_i <= '0';
              validity_cntr              <= validity_cntr + 1;
            end if;
          end if;
        end if;
      end if;
    end process;

  end generate gen_with_dmtd;

  gen_without_dmtd : if(not g_with_dmtd) generate
    regs_towb_ep.dmsr_ps_rdy_i <= '0';
    regs_towb_ep.dmsr_ps_val_i <= (others => 'X');
    regs_towb_dmtd             <= c_ep_in_registers_init_value;
  end generate gen_without_dmtd;

  dvalid_tx <= snk_cyc_i and snk_stb_i and link_ok;
  dvalid_rx <= src_out.cyc and src_out.stb and link_ok;

  gen_leds : if g_with_leds generate
    U_Led_Ctrl : ep_leds_controller
      generic map (
        g_blink_period_log2 => 22)
      port map (
        clk_sys_i   => clk_sys_i,
        rst_n_i     => rst_sys_n_i,
        dvalid_tx_i => dvalid_tx,
        dvalid_rx_i => dvalid_rx,
        link_ok_i   => link_ok,
        led_link_o  => led_link_o,
        led_act_o   => led_act_o);
  end generate gen_leds;

  -------------------------- TRU stuff -----------------------------------
  link_up_o <= link_ok;                 -- indicates that link is UP

  pfilter_pclass_o <= pfilter_pclass;
  pfilter_done_o   <= pfilter_done;
  pfilter_drop_o   <= pfilter_drop;

  txfra_pause_req     <= fc_tx_pause_req_i;
  fc_tx_pause_ready_o <= txfra_pause_ready;
  txfra_pause_delay   <= fc_tx_pause_delay_i;

  -- TRU needs to be able to share the control of ouput path, i.e. turn off the laser
  p_ep_ctrl : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_sys_n_i = '0' then
        ep_ctrl <= '1';
      else
        ep_ctrl <= not link_kill_i;
      end if;
    end if;
  end process;

  GEN_STOP: if(g_with_stop_traffic) generate
    rxpath_fab.sof    <= rxpcs_fab.sof    when(stop_traffic_i='0') else '0';
    rxpath_fab.dvalid <= rxpcs_fab.dvalid when(stop_traffic_i='0') else '0';
    rxpath_fab.eof   <= rxpcs_fab.eof;
    rxpath_fab.error <= rxpcs_fab.error;
    rxpath_fab.bytesel <= rxpcs_fab.bytesel;
    rxpath_fab.has_rx_timestamp <= rxpcs_fab.has_rx_timestamp;
    rxpath_fab.rx_timestamp_valid <= rxpcs_fab.rx_timestamp_valid;
    rxpath_fab.data <= rxpcs_fab.data;
    rxpath_fab.addr <= rxpcs_fab.addr;
  end generate;

  GEN_NO_STOP: if(not g_with_stop_traffic) generate
    rxpath_fab <= rxpcs_fab;
  end generate;

  -------------------------- RMON events -----------------------------------
  rmon.rx_pcs_err      <= rx_path_rmon.rx_pcs_err;  --from ep_rx_path
  rmon.rx_giant        <= rx_path_rmon.rx_giant;
  rmon.rx_runt         <= rx_path_rmon.rx_runt;
  rmon.rx_crc_err      <= rx_path_rmon.rx_crc_err;
  rmon.rx_pause        <= rx_path_rmon.rx_pause;
  rmon.rx_pfilter_drop <= rx_path_rmon.rx_pfilter_drop;
  rmon.rx_pclass       <= rx_path_rmon.rx_pclass;
  rmon.rx_tclass       <= rx_path_rmon.rx_tclass;
  rmon.rx_drop_at_rtu_full <= rx_path_rmon.rx_drop_at_rtu_full;
  rmon.tx_underrun     <= pcs_rmon.tx_underrun;
  rmon.rx_overrun      <= pcs_rmon.rx_overrun;
  rmon.rx_invalid_code <= pcs_rmon.rx_invalid_code;
  rmon.rx_sync_lost    <= pcs_rmon.rx_sync_lost;


  rmon_event_tx : gc_sync_ffs
    generic map(
      g_sync_edge => "negative")
    port map (
      clk_i    => clk_sys_i,
      rst_n_i  => rst_sys_n_i,
      data_i   => txpcs_timestamp_trigger_p_a,
      synced_o => open,
      npulse_o => open,
      ppulse_o => rmon.tx_frame);

  rmon_event_rx : gc_sync_ffs
    generic map(
      g_sync_edge => "negative")
    port map (
      clk_i    => clk_sys_i,
      rst_n_i  => rst_sys_n_i,
      data_i   => rxpcs_timestamp_trigger_p_a,
      synced_o => open,
      npulse_o => open,
      ppulse_o => rmon.rx_frame);

  f_pack_rmon_triggers(rmon, rmon_events_o(c_epevents_sz-1 downto 0));

--   CS_ICON : chipscope_icon
--    port map (
--     CONTROL0 => CONTROL0);
--   CS_ILA : chipscope_ila
--    port map (
--      CONTROL => CONTROL0,
--      CLK     => phy_ref_clk_i,
--      TRIG0   => TRIG0,
--      TRIG1   => TRIG1,
--      TRIG2   => TRIG2,
--      TRIG3   => TRIG3);
-- 
--   TRIG0(15    downto   0) <= phy_rx_data_i;
--   TRIG0(17    downto  16) <= phy_rx_k_i;
--   TRIG0(              18) <= phy_rx_enc_err_i;
--   TRIG0(23    downto  19) <= phy_rx_bitslide_i;

  -- Drive unsued GMII outputs
  gmii_txd_o   <= (others => '0');
  gmii_tx_en_o <= '0';
  gmii_tx_er_o <= '0';

end syn;


