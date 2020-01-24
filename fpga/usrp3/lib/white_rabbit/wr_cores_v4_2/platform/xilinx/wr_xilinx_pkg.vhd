-------------------------------------------------------------------------------
-- Title      : Platform-dependent components needed for WR PTP Core on Xilinx
-- Project    : WR PTP Core
-- URL        : http://www.ohwr.org/projects/wr-cores/wiki/Wrpc_core
-------------------------------------------------------------------------------
-- File       : wr_xilinx_pkg.vhd
-- Author     : Maciej Lipinski, Grzegorz Daniluk, Dimitrios Lampridis
-- Company    : CERN
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
--
-- Copyright (c) 2016-2017 CERN / BE-CO-HT
--
-- This source file is free software; you can redistribute it
-- and/or modify it under the terms of the GNU Lesser General
-- Public License as published by the Free Software Foundation;
-- either version 2.1 of the License, or (at your option) any
-- later version
--
-- This source is distributed in the hope that it will be
-- useful, but WITHOUT ANY WARRANTY; without even the implied
-- warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
-- PURPOSE.  See the GNU Lesser General Public License for more
-- details
--
-- You should have received a copy of the GNU Lesser General
-- Public License along with this source; if not, download it
-- from http://www.gnu.org/licenses/lgpl-2.1.html
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

library work;
use work.endpoint_pkg.all;

package wr_xilinx_pkg is

  component xwrc_platform_xilinx is
    generic (
      g_fpga_family               : string  := "spartan6";
      g_with_external_clock_input : boolean := FALSE;
      g_use_default_plls          : boolean := TRUE;
      g_gtp_enable_ch0            : integer := 0;
      g_gtp_enable_ch1            : integer := 1;
      g_simulation                : integer := 0;
      g_use_ibufgds               : boolean := false);
    port (
      areset_n_i            : in  std_logic             := '1';
      clk_10m_ext_i         : in  std_logic             := '0';
      clk_125m_gtp_p_i      : in  std_logic;
      clk_125m_gtp_n_i      : in  std_logic;
      clk_20m_vcxo_i        : in  std_logic             := '0';
      clk_125m_pllref_i     : in  std_logic             := '0';
      clk_125m_dmtd_i       : in  std_logic             := '0';
      clk_62m5_dmtd_i       : in  std_logic             := '0';
      clk_dmtd_locked_i     : in  std_logic             := '1';
      clk_62m5_sys_i        : in  std_logic             := '0';
      clk_sys_locked_i      : in  std_logic             := '1';
      clk_125m_ref_i        : in  std_logic             := '0';
      clk_ref_locked_i      : in  std_logic             := '1';
      clk_125m_ext_i        : in  std_logic             := '0';
      clk_ext_locked_i      : in  std_logic             := '1';
      clk_ext_stopped_i     : in  std_logic             := '0';
      clk_ext_rst_o         : out std_logic;
      sfp_txn_o             : out std_logic;
      sfp_txp_o             : out std_logic;
      sfp_rxn_i             : in  std_logic;
      sfp_rxp_i             : in  std_logic;
      sfp_tx_fault_i        : in  std_logic             := '0';
      sfp_los_i             : in  std_logic             := '0';
      sfp_tx_disable_o      : out std_logic;
      clk_62m5_sys_o        : out std_logic;
      clk_125m_ref_o        : out std_logic;
      clk_ref_locked_o      : out std_logic;
      clk_62m5_dmtd_o       : out std_logic;
      pll_locked_o          : out std_logic;
      clk_10m_ext_o         : out std_logic;
      phy8_o                : out t_phy_8bits_to_wrc;
      phy8_i                : in  t_phy_8bits_from_wrc  := c_dummy_phy8_from_wrc;
      phy16_o               : out t_phy_16bits_to_wrc;
      phy16_i               : in  t_phy_16bits_from_wrc := c_dummy_phy16_from_wrc;
      ext_ref_mul_o         : out std_logic;
      ext_ref_mul_locked_o  : out std_logic;
      ext_ref_mul_stopped_o : out std_logic;
      ext_ref_rst_i         : in  std_logic             := '0');
  end component xwrc_platform_xilinx;

  component wr_gtp_phy_spartan6
    generic (
      g_enable_ch0 : integer := 1;
      g_enable_ch1 : integer := 1;
      g_simulation : integer := 0);
    port (
      gtp_clk_i          : in  std_logic;
      ch0_ref_clk_i      : in  std_logic                    := '0';
      ch0_tx_data_i      : in  std_logic_vector(7 downto 0) := "00000000";
      ch0_tx_k_i         : in  std_logic                    := '0';
      ch0_tx_disparity_o : out std_logic;
      ch0_tx_enc_err_o   : out std_logic;
      ch0_rx_rbclk_o     : out std_logic;
      ch0_rx_data_o      : out std_logic_vector(7 downto 0);
      ch0_rx_k_o         : out std_logic;
      ch0_rx_enc_err_o   : out std_logic;
      ch0_rx_bitslide_o  : out std_logic_vector(3 downto 0);
      ch0_rst_i          : in  std_logic                    := '0';
      ch0_loopen_i       : in  std_logic                    := '0';
      ch0_loopen_vec_i   : in  std_logic_vector(2 downto 0) := (others => '0');
      ch0_tx_prbs_sel_i  : in  std_logic_vector(2 downto 0) := (others => '0');
      ch0_rdy_o          : out std_logic;
      ch1_ref_clk_i      : in  std_logic;
      ch1_tx_data_i      : in  std_logic_vector(7 downto 0) := "00000000";
      ch1_tx_k_i         : in  std_logic                    := '0';
      ch1_tx_disparity_o : out std_logic;
      ch1_tx_enc_err_o   : out std_logic;
      ch1_rx_data_o      : out std_logic_vector(7 downto 0);
      ch1_rx_rbclk_o     : out std_logic;
      ch1_rx_k_o         : out std_logic;
      ch1_rx_enc_err_o   : out std_logic;
      ch1_rx_bitslide_o  : out std_logic_vector(3 downto 0);
      ch1_rst_i          : in  std_logic                    := '0';
      ch1_loopen_i       : in  std_logic                    := '0';
      ch1_loopen_vec_i   : in  std_logic_vector(2 downto 0) := (others => '0');
      ch1_tx_prbs_sel_i  : in  std_logic_vector(2 downto 0) := (others => '0');
      ch1_rdy_o          : out std_logic;
      pad_txn0_o         : out std_logic;
      pad_txp0_o         : out std_logic;
      pad_rxn0_i         : in  std_logic                    := '0';
      pad_rxp0_i         : in  std_logic                    := '0';
      pad_txn1_o         : out std_logic;
      pad_txp1_o         : out std_logic;
      pad_rxn1_i         : in  std_logic                    := '0';
      pad_rxp1_i         : in  std_logic                    := '0');
  end component;

  component wr_gtx_phy_family7 is
    generic (
      -- set to non-zero value to speed up the simulation by reducing some delays
      g_simulation : integer := 0);
    port (
      clk_gtx_i      : in  std_logic;
      tx_out_clk_o   : out std_logic;
      tx_locked_o    : out std_logic;
      tx_data_i      : in  std_logic_vector(15 downto 0);
      tx_k_i         : in  std_logic_vector(1 downto 0);
      tx_disparity_o : out std_logic;
      tx_enc_err_o   : out std_logic;
      rx_rbclk_o     : out std_logic;
      rx_data_o      : out std_logic_vector(15 downto 0);
      rx_k_o         : out std_logic_vector(1 downto 0);
      rx_enc_err_o   : out std_logic;
      rx_bitslide_o  : out std_logic_vector(4 downto 0);
      rst_i          : in  std_logic;
      loopen_i       : in  std_logic_vector(2 downto 0);
      tx_prbs_sel_i  : in  std_logic_vector(2 downto 0);
      pad_txn_o      : out std_logic;
      pad_txp_o      : out std_logic;
      pad_rxn_i      : in  std_logic := '0';
      pad_rxp_i      : in  std_logic := '0';
      rdy_o          : out std_logic);
  end component;

  component wr_gtp_phy_family7 is
    generic (
      -- set to non-zero value to speed up the simulation by reducing some delays
      g_simulation : integer := 0);
    port (
      clk_gtp_i      : in  std_logic;
      tx_out_clk_o   : out std_logic;
      tx_locked_o    : out std_logic;
      tx_data_i      : in  std_logic_vector(15 downto 0);
      tx_k_i         : in  std_logic_vector(1 downto 0);
      tx_disparity_o : out std_logic;
      tx_enc_err_o   : out std_logic;
      rx_rbclk_o     : out std_logic;
      rx_data_o      : out std_logic_vector(15 downto 0);
      rx_k_o         : out std_logic_vector(1 downto 0);
      rx_enc_err_o   : out std_logic;
      rx_bitslide_o  : out std_logic_vector(4 downto 0);
      rst_i          : in  std_logic;
      loopen_i       : in  std_logic_vector(2 downto 0);
      tx_prbs_sel_i  : in  std_logic_vector(2 downto 0);
      pad_txn_o      : out std_logic;
      pad_txp_o      : out std_logic;
      pad_rxn_i      : in  std_logic := '0';
      pad_rxp_i      : in  std_logic := '0';
      rdy_o          : out std_logic);
  end component;

end wr_xilinx_pkg;
