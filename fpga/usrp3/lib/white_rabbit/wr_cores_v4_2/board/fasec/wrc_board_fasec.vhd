-------------------------------------------------------------------------------
-- Title      : WRPC Wrapper for FASEC
-- Project    : WR PTP Core
-- URL        : http://www.ohwr.org/projects/wr-cores/wiki/Wrpc_core
-------------------------------------------------------------------------------
-- File       : wrc_board_fasec.vhd
-- Author(s)  : Grzegorz Daniluk <grzegorz.daniluk@cern.ch>
-- Company    : CERN (BE-CO-HT)
-- Created    : 2017-08-02
-- Last update: 2017-09-07
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: Top-level wrapper for WR PTP core including all the modules
-- needed to operate the core on the FASEC board.
-- Version with no VHDL records on the top-level (mainly for Verilog
-- instantiation).
-- http://www.ohwr.org/projects/fasec/
-------------------------------------------------------------------------------
-- Copyright (c) 2017 CERN
-------------------------------------------------------------------------------
-- GNU LESSER GENERAL PUBLIC LICENSE
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

library work;
use work.gencores_pkg.all;
use work.wrcore_pkg.all;
use work.wishbone_pkg.all;
use work.etherbone_pkg.all;
use work.wr_fabric_pkg.all;
use work.endpoint_pkg.all;
use work.streamers_pkg.all;
use work.wr_xilinx_pkg.all;
use work.wr_board_pkg.all;
use work.wr_fasec_pkg.all;

entity wrc_board_fasec is
  generic(
    -- set to 1 to speed up some initialization processes during simulation
    g_simulation                : integer := 0;
    -- Select whether to include external ref clock input
    g_with_external_clock_input : integer := 0;
    -- Number of aux clocks syntonized by WRPC to WR timebase
    g_aux_clks                  : integer := 0;
    -- "plainfbrc" = expose WRC fabric interface
    -- "streamers" = attach WRC streamers to fabric interface
    -- "etherbone" = attach Etherbone slave to fabric interface
    g_fabric_iface              : string  := "plainfbrc";
    -- parameters configuration when g_fabric_iface = "streamers" (otherwise ignored)
    --g_streamers_op_mode        : t_streamers_op_mode  := TX_AND_RX;
    --g_tx_streamer_params       : t_tx_streamer_params := c_tx_streamer_params_defaut;
    --g_rx_streamer_params       : t_rx_streamer_params := c_rx_streamer_params_defaut;
    -- memory initialisation file for embedded CPU
    g_dpram_initf               : string  := "../../../../bin/wrpc/wrc_phy16.bram";
    -- identification (id and ver) of the layout of words in the generic diag interface
    g_diag_id                   : integer := 0;
    g_diag_ver                  : integer := 0;
    -- size the generic diag interface
    g_diag_ro_vector_width      : integer := 0;
    g_diag_rw_vector_width      : integer := 0
    );
  port (
    ---------------------------------------------------------------------------
    -- Clocks/resets
    ---------------------------------------------------------------------------
    -- Reset from system fpga
    areset_n_i          : in  std_logic;
    -- Optional reset input active low with rising edge detection. Does not
    -- reset PLLs.
    --areset_edge_n_i     : in  std_logic := '1';
    -- Clock inputs from the board
    clk_20m_vcxo_i      : in  std_logic;
    clk_125m_pllref_p_i : in  std_logic;
    clk_125m_pllref_n_i : in  std_logic;
    clk_125m_gtp_n_i    : in  std_logic;
    clk_125m_gtp_p_i    : in  std_logic;
    -- 10MHz ext ref clock input (g_with_external_clock_input = TRUE)
    clk_10m_ext_i       : in  std_logic := '0';
    -- External PPS input (g_with_external_clock_input = TRUE)
    pps_ext_i           : in  std_logic := '0';
    -- 62.5MHz sys clock output
    clk_sys_62m5_o      : out std_logic;
    -- 125MHz ref clock output
    clk_ref_125m_o      : out std_logic;
    -- active low reset outputs, synchronous to 62m5 and 125m clocks
    rst_sys_62m5_n_o    : out std_logic;
    rst_ref_125m_n_o    : out std_logic;

    ---------------------------------------------------------------------------
    -- Shared SPI interface to DACs
    ---------------------------------------------------------------------------
    plldac_sclk_o   : out std_logic;
    plldac_din_o    : out std_logic;
    pll25dac_cs_n_o : out std_logic;
    pll20dac_cs_n_o : out std_logic;

    ---------------------------------------------------------------------------
    -- SFP I/O for transceiver and SFP management info
    ---------------------------------------------------------------------------
    sfp_tx_p_o         : out std_logic;
    sfp_tx_n_o         : out std_logic;
    sfp_rx_p_i         : in  std_logic;
    sfp_rx_n_i         : in  std_logic;
    sfp_det_i         : in  std_logic := '1';
    sfp_sda_i         : in  std_logic;
    sfp_sda_o         : out std_logic;
    sfp_sda_t         : out std_logic;
    sfp_scl_i         : in  std_logic;
    sfp_scl_o         : out std_logic;
    sfp_scl_t         : out std_logic;
    sfp_rate_select_o : out std_logic;
    sfp_tx_fault_i    : in  std_logic := '0';
    sfp_tx_disable_o  : out std_logic;
    sfp_los_i         : in  std_logic := '0';

    ---------------------------------------------------------------------------
    -- I2C EEPROM
    ---------------------------------------------------------------------------
    eeprom_sda_i : in  std_logic;
    eeprom_sda_o : out std_logic;
    eeprom_sda_t : out std_logic;
    eeprom_scl_i : in  std_logic;
    eeprom_scl_o : out std_logic;
    eeprom_scl_t : out std_logic;

    ---------------------------------------------------------------------------
    -- Onewire interface
    ---------------------------------------------------------------------------
    thermo_id_i : in  std_logic;
    thermo_id_o : out std_logic;
    thermo_id_t : out std_logic;

    ---------------------------------------------------------------------------
    -- UART
    ---------------------------------------------------------------------------
    uart_rxd_i : in  std_logic;
    uart_txd_o : out std_logic;

    ---------------------------------------------------------------------------
    -- Flash memory SPI interface
    ---------------------------------------------------------------------------
    --flash_sclk_o : out std_logic;
    --flash_ncs_o  : out std_logic;
    --flash_mosi_o : out std_logic;
    --flash_miso_i : in  std_logic;

    ---------------------------------------------------------------------------
    -- External WB interface
    ---------------------------------------------------------------------------
    --aux_master_adr_o   : out std_logic_vector(c_wishbone_address_width-1 downto 0);
    --aux_master_dat_o   : out std_logic_vector(c_wishbone_data_width-1 downto 0);
    --aux_master_dat_i   : in  std_logic_vector(c_wishbone_data_width-1 downto 0) := (others => '0');
    --aux_master_sel_o   : out std_logic_vector(c_wishbone_address_width/8-1 downto 0);
    --aux_master_we_o    : out std_logic;
    --aux_master_cyc_o   : out std_logic;
    --aux_master_stb_o   : out std_logic;
    --aux_master_ack_i   : in  std_logic                                          := '0';
    --aux_master_int_i   : in  std_logic                                          := '0';
    --aux_master_err_i   : in  std_logic                                          := '0';
    --aux_master_rty_i   : in  std_logic                                          := '0';
    --aux_master_stall_i : in  std_logic                                          := '0';

    ------------------------------------------
    -- Axi Slave Bus Interface S00_AXI
    ------------------------------------------
    s00_axi_aclk_o  : out std_logic;
    s00_axi_aresetn : in  std_logic := '1';
    s00_axi_awaddr  : in  std_logic_vector(31 downto 0) := (others=>'0');
    --s00_axi_awprot  : in  std_logic_vector(2 downto 0);
    s00_axi_awvalid : in  std_logic := '0';
    s00_axi_awready : out std_logic;
    s00_axi_wdata   : in  std_logic_vector(31 downto 0) := (others=>'0');
    s00_axi_wstrb   : in  std_logic_vector(3 downto 0)  := (others=>'0');
    s00_axi_wvalid  : in  std_logic := '0';
    s00_axi_wready  : out std_logic;
    s00_axi_bresp   : out std_logic_vector(1 downto 0);
    s00_axi_bvalid  : out std_logic;
    s00_axi_bready  : in  std_logic := '0';
    s00_axi_araddr  : in  std_logic_vector(31 downto 0) := (others=>'0');
    --s00_axi_arprot  : in std_logic_vector(2 downto 0);
    s00_axi_arvalid : in  std_logic := '0';
    s00_axi_arready : out std_logic;
    s00_axi_rdata   : out std_logic_vector(31 downto 0);
    s00_axi_rresp   : out std_logic_vector(1 downto 0);
    s00_axi_rvalid  : out std_logic;
    s00_axi_rready  : in  std_logic := '0';
    s00_axi_rlast   : out std_logic;
    axi_int_o       : out std_logic;

    ---------------------------------------------------------------------------
    -- WR fabric interface (when g_fabric_iface = "plain")
    ---------------------------------------------------------------------------
    --wrf_src_adr_o   : out std_logic_vector(1 downto 0);
    --wrf_src_dat_o   : out std_logic_vector(15 downto 0);
    --wrf_src_cyc_o   : out std_logic;
    --wrf_src_stb_o   : out std_logic;
    --wrf_src_we_o    : out std_logic;
    --wrf_src_sel_o   : out std_logic_vector(1 downto 0);
    --wrf_src_ack_i   : in  std_logic;
    --wrf_src_stall_i : in  std_logic;
    --wrf_src_err_i   : in  std_logic;
    --wrf_src_rty_i   : in  std_logic;
    --wrf_snk_adr_i   : in  std_logic_vector(1 downto 0);
    --wrf_snk_dat_i   : in  std_logic_vector(15 downto 0);
    --wrf_snk_cyc_i   : in  std_logic;
    --wrf_snk_stb_i   : in  std_logic;
    --wrf_snk_we_i    : in  std_logic;
    --wrf_snk_sel_i   : in  std_logic_vector(1 downto 0);
    --wrf_snk_ack_o   : out std_logic;
    --wrf_snk_stall_o : out std_logic;
    --wrf_snk_err_o   : out std_logic;
    --wrf_snk_rty_o   : out std_logic;

    ---------------------------------------------------------------------------
    -- WR streamers (when g_fabric_iface = "streamers")
    ---------------------------------------------------------------------------
    ----wrs_tx_data_i  : in  std_logic_vector(c_tx_streamer_params_defaut.data_width-1 downto 0) := (others => '0');
    --wrs_tx_data_i  : in  std_logic_vector(31 downto 0) := (others => '0');
    --wrs_tx_valid_i : in  std_logic                                        := '0';
    --wrs_tx_dreq_o  : out std_logic;
    --wrs_tx_last_i  : in  std_logic                                        := '1';
    --wrs_tx_flush_i : in  std_logic                                        := '0';
    --wrs_tx_cfg_mac_l_i : in  std_logic_vector(47 downto 0)                := x"000000000000";
    --wrs_tx_cfg_mac_t_i : in  std_logic_vector(47 downto 0)                := x"ffffffffffff";
    --wrs_tx_cfg_etype_i : in  std_logic_vector(15 downto 0)                := x"dbff";
    --wrs_rx_first_o : out std_logic;
    --wrs_rx_last_o  : out std_logic;
    ----wrs_rx_data_o  : out std_logic_vector(c_rx_streamer_params_defaut.data_width-1 downto 0);
    --wrs_rx_data_o  : out std_logic_vector(31 downto 0);
    --wrs_rx_valid_o : out std_logic;
    --wrs_rx_dreq_i  : in  std_logic                                        := '0';
    --wrs_rx_cfg_mac_l_i : in std_logic_vector(47 downto 0)                 := x"000000000000";
    --wrs_rx_cfg_mac_r_i : in std_logic_vector(47 downto 0)                 := x"000000000000";
    --wrs_rx_cfg_etype_i : in std_logic_vector(15 downto 0)                 := x"dbff";
    --wrs_rx_cfg_acc_b_i : in std_logic                                     := '1';
    --wrs_rx_cfg_flt_r_i : in std_logic                                     := '0';
    --wrs_rx_cfg_fix_l_i : in std_logic_vector(27 downto 0)                 := x"0000000";
    ---------------------------------------------------------------------------
    -- Etherbone WB master interface (when g_fabric_iface = "etherbone")
    ---------------------------------------------------------------------------
    --wb_eth_adr_o   : out std_logic_vector(c_wishbone_address_width-1 downto 0);
    --wb_eth_dat_o   : out std_logic_vector(c_wishbone_data_width-1 downto 0);
    --wb_eth_dat_i   : in  std_logic_vector(c_wishbone_data_width-1 downto 0) := (others => '0');
    --wb_eth_sel_o   : out std_logic_vector(c_wishbone_address_width/8-1 downto 0);
    --wb_eth_we_o    : out std_logic;
    --wb_eth_cyc_o   : out std_logic;
    --wb_eth_stb_o   : out std_logic;
    --wb_eth_ack_i   : in  std_logic                                          := '0';
    --wb_eth_int_i   : in  std_logic                                          := '0';
    --wb_eth_err_i   : in  std_logic                                          := '0';
    --wb_eth_rty_i   : in  std_logic                                          := '0';
    --wb_eth_stall_i : in  std_logic                                          := '0';

    ---------------------------------------------------------------------------
    -- Generic diagnostics interface (access from WRPC via SNMP or uart console
    ---------------------------------------------------------------------------
    --aux_diag_i : in  std_logic_vector(g_diag_ro_vector_width - 1 downto 0) := (others => '0');
    --aux_diag_o : out std_logic_vector(g_diag_rw_vector_width - 1 downto 0) := (others => '0');

    ---------------------------------------------------------------------------
    -- Aux clocks control
    ---------------------------------------------------------------------------
    --tm_dac_value_o       : out std_logic_vector(23 downto 0);
    --tm_dac_wr_o          : out std_logic_vector(g_aux_clks-1 downto 0);
    --tm_clk_aux_lock_en_i : in  std_logic_vector(g_aux_clks-1 downto 0) := (others => '0');
    --tm_clk_aux_locked_o  : out std_logic_vector(g_aux_clks-1 downto 0);

    ---------------------------------------------------------------------------
    -- External Tx Timestamping I/F
    ---------------------------------------------------------------------------
    --tstamps_stb_o       : out std_logic;
    --tstamps_tsval_o     : out std_logic_vector(31 downto 0);
    --tstamps_port_id_o   : out std_logic_vector(5 downto 0);
    --tstamps_frame_id_o  : out std_logic_vector(15 downto 0);
    --tstamps_incorrect_o : out std_logic;
    --tstamps_ack_i       : in  std_logic := '1';

    -----------------------------------------
    -- Timestamp helper signals, used for Absolute Calibration
    -----------------------------------------
    --abscal_txts_o       : out std_logic;
    --abscal_rxts_o       : out std_logic;

    ---------------------------------------------------------------------------
    -- Pause Frame Control
    ---------------------------------------------------------------------------
    --fc_tx_pause_req_i   : in  std_logic                     := '0';
    --fc_tx_pause_delay_i : in  std_logic_vector(15 downto 0) := x"0000";
    --fc_tx_pause_ready_o : out std_logic;

    ---------------------------------------------------------------------------
    -- Timecode I/F
    ---------------------------------------------------------------------------
    --tm_link_up_o    : out std_logic;
    --tm_time_valid_o : out std_logic;
    --tm_tai_o        : out std_logic_vector(39 downto 0);
    --tm_cycles_o     : out std_logic_vector(27 downto 0);

    ---------------------------------------------------------------------------
    -- Buttons, LEDs and PPS output
    ---------------------------------------------------------------------------
    led_act_o  : out std_logic;
    led_link_o : out std_logic;
    --btn1_i     : in  std_logic := '1';
    --btn2_i     : in  std_logic := '1';
    -- 1PPS output
    pps_p_o    : out std_logic;
    pps_led_o  : out std_logic;
    -- Link ok indication
    link_ok_o  : out std_logic
    );

end entity wrc_board_fasec;


architecture std_wrapper of wrc_board_fasec is

  -----------------------------------------------------------------------------
  -- Signals
  -----------------------------------------------------------------------------

  -- WR fabric interface
  signal wrf_src_out : t_wrf_source_out;
  signal wrf_src_in  : t_wrf_source_in;
  signal wrf_snk_out : t_wrf_sink_out;
  signal wrf_snk_in  : t_wrf_sink_in;

  signal aux_master_out : t_wishbone_master_out;
  signal aux_master_in  : t_wishbone_master_in;

  -- Etherbone interface
  signal wb_eth_master_out : t_wishbone_master_out;
  signal wb_eth_master_in  : t_wishbone_master_in;

  -- Aux diagnostics
  constant c_diag_ro_size : integer := g_diag_ro_vector_width/32;
  constant c_diag_rw_size : integer := g_diag_rw_vector_width/32;

  signal aux_diag_in  : t_generic_word_array(c_diag_ro_size-1 downto 0);
  signal aux_diag_out : t_generic_word_array(c_diag_rw_size-1 downto 0);

  -- External Tx Timestamping I/F
  signal timestamps_out : t_txtsu_timestamp;

  -- streamers config
  signal wrs_tx_cfg_in  : t_tx_streamer_cfg;
  signal wrs_rx_cfg_in  : t_rx_streamer_cfg;

  -- axi signals
  signal s_axi_araddr : std_logic_vector(31 downto 0);
  signal s_axi_awaddr : std_logic_vector(31 downto 0);

begin  -- architecture struct

  -- Map top-level signals to internal records
  --aux_master_adr_o <= aux_master_out.adr;
  --aux_master_dat_o <= aux_master_out.dat;
  --aux_master_cyc_o <= aux_master_out.cyc;
  --aux_master_stb_o <= aux_master_out.stb;
  --aux_master_sel_o <= aux_master_out.sel;
  --aux_master_we_o  <= aux_master_out.we;

  --aux_master_in.dat   <= aux_master_dat_i;
  --aux_master_in.ack   <= aux_master_ack_i;
  --aux_master_in.int   <= aux_master_int_i;
  --aux_master_in.err   <= aux_master_err_i;
  --aux_master_in.rty   <= aux_master_rty_i;
  --aux_master_in.stall <= aux_master_stall_i;

  --wrf_src_adr_o    <= wrf_src_out.adr;
  --wrf_src_dat_o    <= wrf_src_out.dat;
  --wrf_src_cyc_o    <= wrf_src_out.cyc;
  --wrf_src_stb_o    <= wrf_src_out.stb;
  --wrf_src_we_o     <= wrf_src_out.we;
  --wrf_src_sel_o    <= wrf_src_out.sel;
  --wrf_src_in.ack   <= wrf_src_ack_i;
  --wrf_src_in.stall <= wrf_src_stall_i;
  --wrf_src_in.err   <= wrf_src_err_i;
  --wrf_src_in.rty   <= wrf_src_rty_i;

  --wrf_snk_in.adr  <= wrf_snk_adr_i;
  --wrf_snk_in.dat  <= wrf_snk_dat_i;
  --wrf_snk_in.cyc  <= wrf_snk_cyc_i;
  --wrf_snk_in.stb  <= wrf_snk_stb_i;
  --wrf_snk_in.we   <= wrf_snk_we_i;
  --wrf_snk_in.sel  <= wrf_snk_sel_i;
  --wrf_snk_ack_o   <= wrf_snk_out.ack;
  --wrf_snk_stall_o <= wrf_snk_out.stall;
  --wrf_snk_err_o   <= wrf_snk_out.err;
  --wrf_snk_rty_o   <= wrf_snk_out.rty;


  --wb_eth_adr_o <= wb_eth_master_out.adr;
  --wb_eth_dat_o <= wb_eth_master_out.dat;
  --wb_eth_cyc_o <= wb_eth_master_out.cyc;
  --wb_eth_stb_o <= wb_eth_master_out.stb;
  --wb_eth_sel_o <= wb_eth_master_out.sel;
  --wb_eth_we_o  <= wb_eth_master_out.we;

  --wb_eth_master_in.dat   <= wb_eth_dat_i;
  --wb_eth_master_in.ack   <= wb_eth_ack_i;
  --wb_eth_master_in.int   <= wb_eth_int_i;
  --wb_eth_master_in.err   <= wb_eth_err_i;
  --wb_eth_master_in.rty   <= wb_eth_rty_i;
  --wb_eth_master_in.stall <= wb_eth_stall_i;

  --aux_diag_in <= f_de_vectorize_diag(aux_diag_i, g_diag_ro_vector_width);
  --aux_diag_o  <= f_vectorize_diag(aux_diag_out, g_diag_rw_vector_width);

  --tstamps_stb_o      <= timestamps_out.stb;
  --tstamps_tsval_o    <= timestamps_out.tsval;
  --tstamps_port_id_o  <= timestamps_out.port_id;
  --tstamps_frame_id_o <= timestamps_out.frame_id;

  --wrs_tx_cfg_in.mac_local         <= wrs_tx_cfg_mac_l_i;
  --wrs_tx_cfg_in.mac_target        <= wrs_tx_cfg_mac_t_i;
  --wrs_tx_cfg_in.ethertype         <= wrs_tx_cfg_etype_i;

  --wrs_rx_cfg_in.mac_local         <= wrs_rx_cfg_mac_l_i;
  --wrs_rx_cfg_in.mac_remote        <= wrs_rx_cfg_mac_r_i;
  --wrs_rx_cfg_in.ethertype         <= wrs_rx_cfg_etype_i;
  --wrs_rx_cfg_in.accept_broadcasts <= wrs_rx_cfg_acc_b_i;
  --wrs_rx_cfg_in.filter_remote     <= wrs_rx_cfg_flt_r_i;
  --wrs_rx_cfg_in.fixed_latency     <= wrs_rx_cfg_fix_l_i;

  -- axi supports word-addressing only, i.e. per 4 bytes; shift for wb-bridge
  s_axi_araddr  <= "00" & s00_axi_araddr(31 downto 2);
  s_axi_awaddr  <= "00" & s00_axi_awaddr(31 downto 2);
  -- Instantiate the records-based module
  cmp_xwrc_board_fasec : xwrc_board_fasec
    generic map (
      g_simulation                => g_simulation,
      g_with_external_clock_input => f_int2bool(g_with_external_clock_input),
      g_aux_clks                  => g_aux_clks,
      g_fabric_iface              => f_str2iface_type(g_fabric_iface),
      g_streamers_op_mode         => TX_AND_RX,
      g_tx_streamer_params        => c_tx_streamer_params_defaut,
      g_rx_streamer_params        => c_rx_streamer_params_defaut,
      g_dpram_initf               => g_dpram_initf,
      g_diag_id                   => g_diag_id,
      g_diag_ver                  => g_diag_ver,
      g_diag_ro_size              => c_diag_ro_size,
      g_diag_rw_size              => c_diag_rw_size)
    port map (
      areset_n_i           => areset_n_i,
      --areset_edge_n_i      => areset_edge_n_i,
      clk_20m_vcxo_i       => clk_20m_vcxo_i,
      clk_125m_pllref_p_i  => clk_125m_pllref_p_i,
      clk_125m_pllref_n_i  => clk_125m_pllref_n_i,
      clk_125m_gtp_n_i     => clk_125m_gtp_n_i,
      clk_125m_gtp_p_i     => clk_125m_gtp_p_i,
      clk_10m_ext_i        => clk_10m_ext_i,
      pps_ext_i            => pps_ext_i,
      clk_sys_62m5_o       => clk_sys_62m5_o,
      clk_ref_125m_o       => clk_ref_125m_o,
      rst_sys_62m5_n_o     => rst_sys_62m5_n_o,
      rst_ref_125m_n_o     => rst_ref_125m_n_o,
      --
      plldac_sclk_o        => plldac_sclk_o,
      plldac_din_o         => plldac_din_o,
      pll25dac_cs_n_o      => pll25dac_cs_n_o,
      pll20dac_cs_n_o      => pll20dac_cs_n_o,
      --
      sfp_txp_o            => sfp_tx_p_o,
      sfp_txn_o            => sfp_tx_n_o,
      sfp_rxp_i            => sfp_rx_p_i,
      sfp_rxn_i            => sfp_rx_n_i,
      sfp_det_i            => sfp_det_i,
      sfp_sda_i            => sfp_sda_i,
      sfp_sda_o            => sfp_sda_o,
      sfp_sda_t            => sfp_sda_t,
      sfp_scl_i            => sfp_scl_i,
      sfp_scl_o            => sfp_scl_o,
      sfp_scl_t            => sfp_scl_t,
      sfp_rate_select_o    => sfp_rate_select_o,
      sfp_tx_fault_i       => sfp_tx_fault_i,
      sfp_tx_disable_o     => sfp_tx_disable_o,
      sfp_los_i            => sfp_los_i,
      --
      eeprom_sda_i => eeprom_sda_i,
      eeprom_sda_o => eeprom_sda_o,
      eeprom_sda_t => eeprom_sda_t,
      eeprom_scl_i => eeprom_scl_i,
      eeprom_scl_o => eeprom_scl_o,
      eeprom_scl_t => eeprom_scl_t,
      --
      thermo_id_i  => thermo_id_i,
      thermo_id_o  => thermo_id_o,
      thermo_id_t  => thermo_id_t,
      --
      uart_rxd_i           => uart_rxd_i,
      uart_txd_o           => uart_txd_o,
      --
      --flash_sclk_o         => flash_sclk_o,
      --flash_ncs_o          => flash_ncs_o,
      --flash_mosi_o         => flash_mosi_o,
      --flash_miso_i         => flash_miso_i,
      --
      --aux_master_o         => aux_master_out,
      --aux_master_i         => aux_master_in,
      --
      s00_axi_aclk_o       => s00_axi_aclk_o,
      s00_axi_aresetn      => s00_axi_aresetn,
      s00_axi_awaddr       => s_axi_awaddr,
      s00_axi_awprot       => (others=>'0'), --s00_axi_awprot,
      s00_axi_awvalid      => s00_axi_awvalid,
      s00_axi_awready      => s00_axi_awready,
      s00_axi_wdata        => s00_axi_wdata,
      s00_axi_wstrb        => s00_axi_wstrb,
      s00_axi_wvalid       => s00_axi_wvalid,
      s00_axi_wready       => s00_axi_wready,
      s00_axi_bresp        => s00_axi_bresp,
      s00_axi_bvalid       => s00_axi_bvalid,
      s00_axi_bready       => s00_axi_bready,
      s00_axi_araddr       => s_axi_araddr,
      s00_axi_arprot       => (others=>'0'), --s00_axi_arprot,
      s00_axi_arvalid      => s00_axi_arvalid,
      s00_axi_arready      => s00_axi_arready,
      s00_axi_rdata        => s00_axi_rdata,
      s00_axi_rresp        => s00_axi_rresp,
      s00_axi_rvalid       => s00_axi_rvalid,
      s00_axi_rready       => s00_axi_rready,
      s00_axi_rlast        => s00_axi_rlast,
      axi_int_o            => axi_int_o,
      --
      --wrf_src_o            => wrf_src_out,
      --wrf_src_i            => wrf_src_in,
      --wrf_snk_o            => wrf_snk_out,
      --wrf_snk_i            => wrf_snk_in,
      --
      --wrs_tx_data_i        => wrs_tx_data_i,
      --wrs_tx_valid_i       => wrs_tx_valid_i,
      --wrs_tx_dreq_o        => wrs_tx_dreq_o,
      --wrs_tx_last_i        => wrs_tx_last_i,
      --wrs_tx_flush_i       => wrs_tx_flush_i,
      --wrs_tx_cfg_i         => wrs_tx_cfg_in,
      --wrs_rx_first_o       => wrs_rx_first_o,
      --wrs_rx_last_o        => wrs_rx_last_o,
      --wrs_rx_data_o        => wrs_rx_data_o,
      --wrs_rx_valid_o       => wrs_rx_valid_o,
      --wrs_rx_dreq_i        => wrs_rx_dreq_i,
      --wrs_rx_cfg_i         => wrs_rx_cfg_in,
      --
      --wb_eth_master_o      => wb_eth_master_out,
      --wb_eth_master_i      => wb_eth_master_in,
      --
      --aux_diag_i           => aux_diag_in,
      --aux_diag_o           => aux_diag_out,
      --
      --tm_dac_value_o       => tm_dac_value_o,
      --tm_dac_wr_o          => tm_dac_wr_o,
      --tm_clk_aux_lock_en_i => tm_clk_aux_lock_en_i,
      --tm_clk_aux_locked_o  => tm_clk_aux_locked_o,
      --
      --timestamps_o         => timestamps_out,
      --timestamps_ack_i     => tstamps_ack_i,
      --
      --abscal_txts_o        => abscal_txts_o,
      --abscal_rxts_o        => abscal_rxts_o,
      --
      --fc_tx_pause_req_i    => fc_tx_pause_req_i,
      --fc_tx_pause_delay_i  => fc_tx_pause_delay_i,
      --fc_tx_pause_ready_o  => fc_tx_pause_ready_o,
      --
      --tm_link_up_o         => tm_link_up_o,
      --tm_time_valid_o      => tm_time_valid_o,
      --tm_tai_o             => tm_tai_o,
      --tm_cycles_o          => tm_cycles_o,
      --
      led_act_o            => led_act_o,
      led_link_o           => led_link_o,
      --btn1_i               => btn1_i,
      --btn2_i               => btn2_i,
      --
      pps_p_o              => pps_p_o,
      pps_led_o            => pps_led_o,
      --
      link_ok_o            => link_ok_o);

end architecture std_wrapper;
