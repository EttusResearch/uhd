-------------------------------------------------------------------------------
-- Title      : Common WRPC Wrapper package
-- Project    : WR PTP Core
-- URL        : http://www.ohwr.org/projects/wr-cores/wiki/Wrpc_core
-------------------------------------------------------------------------------
-- File       : xwrc_board_common.vhd
-- Company    : CERN (BE-CO-HT)
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
--
-- Copyright (c) 2017 CERN
--
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
use work.wishbone_pkg.all;
use work.wr_fabric_pkg.all;
use work.endpoint_pkg.all;
use work.wrcore_pkg.all;
use work.streamers_pkg.all;

package wr_board_pkg is

  type t_board_fabric_iface is (PLAIN, STREAMERS, ETHERBONE, always_last_invalid);

  -- TODO: using these default paths requires absolute path. If relative path is used,
  --       they work only for reference designs by chance. Once we figure out how to
  --       automatically figure out absolute path of <wr-cores location>/bin/wrpc, we
  --       will be able to use this concept in f_find_default_lm32_firmware()
  constant dpram_initf_default_altera_phy8     : string := "";
  constant dpram_initf_default_xilinx_phy8     : string := "";
  constant dpram_initf_default_altera_phy8_sim : string := "";
  constant dpram_initf_default_xilinx_phy8_sim : string := "";

  procedure f_check_fabric_iface_type (
    constant iface_type : in t_board_fabric_iface);

  procedure f_check_diag_id (
    constant diag_id, diag_ver : in integer);

  function f_str2iface_type (
    constant iface_str : string(1 to 9))
    return t_board_fabric_iface;

  function f_pick_diag_val (
    iface           : t_board_fabric_iface;
    streamers_val   : integer;
    application_val : integer
    ) return integer;

  function f_pick_diag_size (
    iface            : t_board_fabric_iface;
    streamers_size   : integer;
    application_size : integer
    ) return integer;

  function f_vectorize_diag (
    diag_in          : t_generic_word_array;
    diag_vector_size : integer)
    return std_logic_vector;

  function f_de_vectorize_diag (
    diag_in          : std_logic_vector;
    diag_vector_size : integer)
    return t_generic_word_array;

  function f_find_default_lm32_firmware (
    dpram_initf : string;
    simulation  : integer;
    pcs_16_bit  : boolean)
    return string;

  component xwrc_board_common is
    generic (
      g_simulation                : integer                        := 0;
      g_with_external_clock_input : boolean                        := TRUE;
      g_board_name                : string                         := "NA  ";
      g_flash_secsz_kb            : integer                        := 256;        -- default for M25P128
      g_flash_sdbfs_baddr         : integer                        := 16#600000#; -- default for M25P128
      g_phys_uart                 : boolean                        := TRUE;
      g_virtual_uart              : boolean                        := TRUE;
      g_aux_clks                  : integer                        := 0;
      g_ep_rxbuf_size             : integer                        := 1024;
      g_tx_runt_padding           : boolean                        := TRUE;
      g_dpram_initf               : string                         := "";
      g_dpram_size                : integer                        := 131072/4;
      g_interface_mode            : t_wishbone_interface_mode      := PIPELINED;
      g_address_granularity       : t_wishbone_address_granularity := BYTE;
      g_aux_sdb                   : t_sdb_device                   := c_wrc_periph3_sdb;
      g_softpll_enable_debugger   : boolean                        := FALSE;
      g_vuart_fifo_size           : integer                        := 1024;
      g_pcs_16bit                 : boolean                        := FALSE;
      g_diag_id                   : integer                        := 0;
      g_diag_ver                  : integer                        := 0;
      g_diag_ro_size              : integer                        := 0;
      g_diag_rw_size              : integer                        := 0;
      g_streamers_op_mode         : t_streamers_op_mode            := TX_AND_RX;
      g_tx_streamer_params        : t_tx_streamer_params           := c_tx_streamer_params_defaut;
      g_rx_streamer_params        : t_rx_streamer_params           := c_rx_streamer_params_defaut;
      g_fabric_iface              : t_board_fabric_iface           := PLAIN);
    port (
      clk_sys_i            : in  std_logic;
      clk_dmtd_i           : in  std_logic;
      clk_ref_i            : in  std_logic;
      clk_aux_i            : in  std_logic_vector(g_aux_clks-1 downto 0)          := (others => '0');
      clk_10m_ext_i        : in  std_logic                                        := '0';
      clk_ext_mul_i        : in  std_logic                                        := '0';
      clk_ext_mul_locked_i : in  std_logic                                        := '1';
      clk_ext_stopped_i    : in  std_logic                                        := '0';
      clk_ext_rst_o        : out std_logic;
      pps_ext_i            : in  std_logic                                        := '0';
      rst_n_i              : in  std_logic;
      dac_hpll_load_p1_o   : out std_logic;
      dac_hpll_data_o      : out std_logic_vector(15 downto 0);
      dac_dpll_load_p1_o   : out std_logic;
      dac_dpll_data_o      : out std_logic_vector(15 downto 0);
      phy8_o               : out t_phy_8bits_from_wrc;
      phy8_i               : in  t_phy_8bits_to_wrc                               := c_dummy_phy8_to_wrc;
      phy16_o              : out t_phy_16bits_from_wrc;
      phy16_i              : in  t_phy_16bits_to_wrc                              := c_dummy_phy16_to_wrc;
      scl_o                : out std_logic;
      scl_i                : in  std_logic                                        := '1';
      sda_o                : out std_logic;
      sda_i                : in  std_logic                                        := '1';
      sfp_scl_o            : out std_logic;
      sfp_scl_i            : in  std_logic                                        := '1';
      sfp_sda_o            : out std_logic;
      sfp_sda_i            : in  std_logic                                        := '1';
      sfp_det_i            : in  std_logic;
      spi_sclk_o           : out std_logic;
      spi_ncs_o            : out std_logic;
      spi_mosi_o           : out std_logic;
      spi_miso_i           : in  std_logic                                        := '0';
      uart_rxd_i           : in  std_logic                                        := '0';
      uart_txd_o           : out std_logic;
      owr_pwren_o          : out std_logic_vector(1 downto 0);
      owr_en_o             : out std_logic_vector(1 downto 0);
      owr_i                : in  std_logic_vector(1 downto 0)                     := (others => '1');
      wb_slave_i           : in  t_wishbone_slave_in                              := cc_dummy_slave_in;
      wb_slave_o           : out t_wishbone_slave_out;
      aux_master_o         : out t_wishbone_master_out;
      aux_master_i         : in  t_wishbone_master_in                             := cc_dummy_master_in;
      wrf_src_o            : out t_wrf_source_out;
      wrf_src_i            : in  t_wrf_source_in                                  := c_dummy_src_in;
      wrf_snk_o            : out t_wrf_sink_out;
      wrf_snk_i            : in  t_wrf_sink_in                                    := c_dummy_snk_in;
      wrs_tx_data_i        : in  std_logic_vector(g_tx_streamer_params.data_width-1 downto 0) := (others => '0');
      wrs_tx_valid_i       : in  std_logic                                        := '0';
      wrs_tx_dreq_o        : out std_logic;
      wrs_tx_last_i        : in  std_logic                                        := '1';
      wrs_tx_flush_i       : in  std_logic                                        := '0';
      wrs_rx_first_o       : out std_logic;
      wrs_rx_last_o        : out std_logic;
      wrs_rx_data_o        : out std_logic_vector(g_rx_streamer_params.data_width-1 downto 0);
      wrs_rx_valid_o       : out std_logic;
      wrs_rx_dreq_i        : in  std_logic                                        := '0';
      wrs_tx_cfg_i         : in t_tx_streamer_cfg := c_tx_streamer_cfg_default;
      wrs_rx_cfg_i         : in t_rx_streamer_cfg := c_rx_streamer_cfg_default;
      wb_eth_master_o      : out t_wishbone_master_out;
      wb_eth_master_i      : in  t_wishbone_master_in                             := cc_dummy_master_in;
      aux_diag_i           : in  t_generic_word_array(g_diag_ro_size-1 downto 0)  := (others => (others => '0'));
      aux_diag_o           : out t_generic_word_array(g_diag_rw_size-1 downto 0);
      tm_dac_value_o       : out std_logic_vector(23 downto 0);
      tm_dac_wr_o          : out std_logic_vector(g_aux_clks-1 downto 0);
      tm_clk_aux_lock_en_i : in  std_logic_vector(g_aux_clks-1 downto 0)          := (others => '0');
      tm_clk_aux_locked_o  : out std_logic_vector(g_aux_clks-1 downto 0);
      timestamps_o         : out t_txtsu_timestamp;
      timestamps_ack_i     : in  std_logic                                        := '1';
      abscal_txts_o        : out std_logic;
      abscal_rxts_o        : out std_logic;
      fc_tx_pause_req_i    : in  std_logic                                        := '0';
      fc_tx_pause_delay_i  : in  std_logic_vector(15 downto 0)                    := x"0000";
      fc_tx_pause_ready_o  : out std_logic;
      tm_link_up_o         : out std_logic;
      tm_time_valid_o      : out std_logic;
      tm_tai_o             : out std_logic_vector(39 downto 0);
      tm_cycles_o          : out std_logic_vector(27 downto 0);
      led_act_o            : out std_logic;
      led_link_o           : out std_logic;
      btn1_i               : in  std_logic                                        := '1';
      btn2_i               : in  std_logic                                        := '1';
      pps_csync_o          : out std_logic;
      pps_p_o              : out std_logic;
      pps_led_o            : out std_logic;
      link_ok_o            : out std_logic);
  end component xwrc_board_common;

end wr_board_pkg;

package body wr_board_pkg is

  procedure f_check_fabric_iface_type (
    constant iface_type : in t_board_fabric_iface) is
  begin
    if iface_type >= always_last_invalid then
      assert FALSE
        report "WR PTP core fabric interface [" & t_board_fabric_iface'image(iface_type) & "] is not supported"
        severity FAILURE;
    end if;
  end procedure f_check_fabric_iface_type;

  procedure f_check_diag_id (
    constant diag_id, diag_ver : in integer) is
  begin
    assert (diag_id /= 1) report
      "g_diag_id=1 is reserved for wr_streamers and cannot be set by users"
      severity FAILURE;

    assert (not (diag_id /= 0 and diag_ver = 0)) report
      "If diag_id is set by the user (diag_id > 1), g_diag_ver must be at least 1"
      severity FAILURE;
  end procedure f_check_diag_id;

  function f_str2iface_type (
    constant iface_str : string(1 to 9))
    return t_board_fabric_iface is
    variable result : t_board_fabric_iface;
  begin
    case iface_str is
      when "PLAINFBRC" => result := PLAIN;
      when "STREAMERS" => result := STREAMERS;
      when "ETHERBONE" => result := ETHERBONE;
      when others      => result := always_last_invalid;
    end case;
    return result;
  end function f_str2iface_type;

  -- this function decides what is the diag_id/ver used in the WRPC and MIB for access
  -- via SNMP
  function f_pick_diag_val (
    iface           : t_board_fabric_iface;
    streamers_val   : integer;
    application_val : integer
    ) return integer is
  begin
    -- streamers are enabled and application/user does nto use diags (no vector specified),
    -- use default streamer's id/ver
    if(iface = STREAMERS and application_val = 0) then
      return streamers_val;
    else  -- otherwise, use id/ver specified by the user/application. This is the case also
      -- when streamers are used.
      return application_val;
    end if;
  end f_pick_diag_val;

  -- provide the size of the final diag array.
  function f_pick_diag_size (
    iface            : t_board_fabric_iface;
    streamers_size   : integer;
    application_size : integer
    ) return integer is
  begin
    -- when streamers are used, concatenate the array of streamers and application/user
    if(iface = STREAMERS) then
      return (streamers_size+application_size);
    else  -- otherwise, only the size provided by the application/user
      return application_size;
    end if;
  end f_pick_diag_size;

  function f_vectorize_diag (
    diag_in          : t_generic_word_array;
    diag_vector_size : integer)
    return std_logic_vector is
    variable result : std_logic_vector(diag_vector_size-1 downto 0);
  begin
    assert (diag_vector_size mod 32 = 0) report
      "g_diag_ro/w_vector_width must have value that is a mutiple of 32"
      severity FAILURE;
    for i in 0 to diag_vector_size/32-1 loop
      result(i*32-31 downto i*32) := diag_in(i);
    end loop;
    return result;
  end function f_vectorize_diag;

  function f_de_vectorize_diag (
    diag_in          : std_logic_vector;
    diag_vector_size : integer)
    return t_generic_word_array is
    variable result : t_generic_word_array(diag_vector_size/32-1 downto 0);
  begin
    assert (diag_vector_size mod 32 = 0) report
      "g_diag_ro/w_vector_width must have value that is a mutiple of 32"
      severity FAILURE;
    for i in 0 to diag_vector_size/32-1 loop
      result(i) := diag_in(i*32-31 downto i*32);
    end loop;
    return result;
  end function f_de_vectorize_diag;

  function f_find_default_lm32_firmware (
    dpram_initf : string;
    simulation  : integer;
    pcs_16_bit  : boolean)
    return string is
  begin
    if((dpram_initf = "default_altera" or dpram_initf = "default_xilinx") and
       pcs_16_bit = TRUE) then
      assert FALSE
        report "[Board:Software for LM32 in WR Core] No release binary for " &
        "pcs_16_bit." severity FAILURE;
      return "";
    elsif (dpram_initf /= "default_altera" and dpram_initf /= "default_xilinx") then
      report "[Board:Software for LM32 in WR Core]  Using user-provided LM32 " &
        "firmware ("&dpram_initf&")." severity NOTE;
      return dpram_initf;
    elsif (simulation = 0 and dpram_initf = "default_altera" and pcs_16_bit = FALSE and
           dpram_initf_default_altera_phy8 /= "") then
      report "[Board:Software for LM32 in WR Core] Using release LM32 firmware " &
        "(altera, phy8)." severity NOTE;
      return dpram_initf_default_altera_phy8;
    elsif (simulation = 0 and dpram_initf = "default_xilinx" and pcs_16_bit = FALSE and
           dpram_initf_default_xilinx_phy8 /= "") then
      report "[Board:Software for LM32 in WR Core] Using release LM32 firmware " &
        "(xilnix, phy8)" severity NOTE;
      return dpram_initf_default_xilinx_phy8;
    elsif (simulation = 1 and dpram_initf = "default_altera" and pcs_16_bit = FALSE and
           dpram_initf_default_altera_phy8_sim /= "") then
      report "Board:[Software for LM32 in WR Core] Using release LM32 firmware " &
        "(altera, phy8, sim)." severity NOTE;
      return dpram_initf_default_altera_phy8_sim;
    elsif (simulation = 1 and dpram_initf = "default_xilinx" and pcs_16_bit = FALSE and
           dpram_initf_default_xilinx_phy8_sim /= "") then
      report "[Board:Software for LM32 in WR Core] Using release LM32 firmware " &
        "(xilinx, phy8, sim)." severity NOTE;
      return dpram_initf_default_xilinx_phy8_sim;
    else
      assert FALSE
        report "[Board:Software for LM32 in WR Core] Default inclussion of LM32" &
        "binary with software is not supported yet. In your instantiation" &
        "of the board (xwrc_board_{spec,svec,vfchd} set g_dpram_initf to the" &
        "correct path, i.e." &
        "<your wr-cores location>/bin/wrpc/wrc_phy8_sim.{bram, mif} " severity FAILURE;
      return "";
    end if;
  end function;

end package body wr_board_pkg;
