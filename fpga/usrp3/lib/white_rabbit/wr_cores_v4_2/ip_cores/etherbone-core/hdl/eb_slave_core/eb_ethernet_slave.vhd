--! @file eb_ethernet_slave.vhd
--! @brief Top file for EtherBone core
--!
--! Copyright (C) 2011-2012 GSI Helmholtz Centre for Heavy Ion Research GmbH 
--!
--! Important details about its implementation
--! should go in these comments.
--!
--! @author Mathias Kreider <m.kreider@gsi.de>
--! @author Wesley W. Terpstra <w.terpstra@gsi.de>
--!
--------------------------------------------------------------------------------
--! This library is free software; you can redistribute it and/or
--! modify it under the terms of the GNU Lesser General Public
--! License as published by the Free Software Foundation; either
--! version 3 of the License, or (at your option) any later version.
--!
--! This library is distributed in the hope that it will be useful,
--! but WITHOUT ANY WARRANTY; without even the implied warranty of
--! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
--! Lesser General Public License for more details.
--!  
--! You should have received a copy of the GNU Lesser General Public
--! License along with this library. If not, see <http://www.gnu.org/licenses/>.
---------------------------------------------------------------------------------

--! Standard library
library IEEE;
--! Standard packages    
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

library work;
use work.etherbone_pkg.all;
use work.eb_hdr_pkg.all;
use work.wishbone_pkg.all;
use work.wr_fabric_pkg.all;
use work.eb_internals_pkg.all;

entity eb_ethernet_slave is
  generic(
    g_sdb_address    : std_logic_vector(63 downto 0);
    g_timeout_cycles : natural;
    g_mtu            : natural);
  port(
    clk_i       : in  std_logic;
    nRst_i      : in  std_logic;
    snk_i       : in  t_wrf_sink_in;
    snk_o       : out t_wrf_sink_out;
    src_o       : out t_wrf_source_out;
    src_i       : in  t_wrf_source_in;
    cfg_slave_o : out t_wishbone_slave_out;
    cfg_slave_i : in  t_wishbone_slave_in;
    master_o    : out t_wishbone_master_out;
    master_i    : in  t_wishbone_master_in);
end eb_ethernet_slave;


architecture rtl of eb_ethernet_slave is
  signal s_his_mac,  s_my_mac  : std_logic_vector(47 downto 0);
  signal s_his_ip,   s_my_ip   : std_logic_vector(31 downto 0);
  signal s_his_port, s_my_port : std_logic_vector(15 downto 0);
  
  signal s_tx_stb     : std_logic;
  signal s_tx_stall   : std_logic;
  signal s_skip_stb   : std_logic;
  signal s_skip_stall : std_logic;
  signal s_length     : unsigned(15 downto 0); -- of UDP in words
  
  signal s_rx2widen   : t_wishbone_master_out;
  signal s_widen2rx   : t_wishbone_master_in;
  signal s_widen2fsm  : t_wishbone_master_out;
  signal s_fsm2widen  : t_wishbone_master_in;
  signal s_fsm2narrow : t_wishbone_master_out;
  signal s_narrow2fsm : t_wishbone_master_in;
  signal s_narrow2tx  : t_wishbone_master_out;
  signal s_tx2narrow  : t_wishbone_master_in;
  
begin
  rx : eb_eth_rx
    generic map(
      g_mtu => g_mtu)
    port map(
      clk_i     => clk_i,
      rst_n_i   => nRst_i,
      snk_i     => snk_i,
      snk_o     => snk_o,
      master_o  => s_rx2widen,
      master_i  => s_widen2rx,
      stb_o     => s_tx_stb,
      stall_i   => s_tx_stall,
      mac_o     => s_his_mac,
      ip_o      => s_his_ip,
      port_o    => s_his_port,
      length_o  => s_length);
  
  widen : eb_stream_widen
    generic map(
      g_slave_width  => 16,
      g_master_width => 32)
    port map(
      clk_i    => clk_i,
      rst_n_i  => nRst_i,
      slave_i  => s_rx2widen,
      slave_o  => s_widen2rx,
      master_i => s_fsm2widen,
      master_o => s_widen2fsm);
      
  eb : eb_slave_top
    generic map(
      g_sdb_address    => g_sdb_address(31 downto 0),
      g_timeout_cycles => g_timeout_cycles)
    port map(
      clk_i        => clk_i,
      nRst_i       => nRst_i,
      EB_RX_i      => s_widen2fsm,
      EB_RX_o      => s_fsm2widen,
      EB_TX_i      => s_narrow2fsm,
      EB_TX_o      => s_fsm2narrow,
      skip_stb_o   => s_skip_stb,
      skip_stall_i => s_skip_stall,
      WB_config_i  => cfg_slave_i,
      WB_config_o  => cfg_slave_o,
      WB_master_i  => master_i,
      WB_master_o  => master_o,
      my_mac_o     => s_my_mac,
      my_ip_o      => s_my_ip,
      my_port_o    => s_my_port);

  narrow : eb_stream_narrow
    generic map(
      g_slave_width  => 32,
      g_master_width => 16)
    port map(
      clk_i    => clk_i,
      rst_n_i  => nRst_i,
      slave_i  => s_fsm2narrow,
      slave_o  => s_narrow2fsm,
      master_i => s_tx2narrow,
      master_o => s_narrow2tx);

  tx : eb_eth_tx
    generic map(
      g_mtu => g_mtu)
    port map(
      clk_i        => clk_i,
      rst_n_i      => nRst_i,
      src_i        => src_i,
      src_o        => src_o,
      slave_o      => s_tx2narrow,
      slave_i      => s_narrow2tx,
      stb_i        => s_tx_stb,
      stall_o      => s_tx_stall,
      mac_i        => s_his_mac,
      ip_i         => s_his_ip,
      port_i       => s_his_port,
      length_i     => s_length,
      skip_stb_i   => s_skip_stb,
      skip_stall_o => s_skip_stall,
      my_mac_i     => s_my_mac,
      my_ip_i      => s_my_ip,
      my_port_i    => s_my_port);
  
end rtl;
