-------------------------------------------------------------------------------
-- Title      : Mini Embedded DMA Network Interface Controller
-- Project    : WhiteRabbit Core
-------------------------------------------------------------------------------
-- File       : xwrsw_mini_nic.vhd
-- Author     : Grzegorz Daniluk, Tomasz Wlostowski
-- Company    : CERN BE-Co-HT
-- Created    : 2010-07-26
-- Last update: 2017-02-03
-- Platform   : FPGA-generic
-- Standard   : VHDL
-------------------------------------------------------------------------------
--
-- Copyright (c) 2010-2016 CERN
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

use work.wr_fabric_pkg.all;
use work.wishbone_pkg.all;

entity xwr_mini_nic is

  generic (
    g_interface_mode       : t_wishbone_interface_mode      := CLASSIC;
    g_address_granularity  : t_wishbone_address_granularity := WORD;
    g_tx_fifo_size         : integer                        := 1024;
    g_rx_fifo_size         : integer                        := 2048;
    g_buffer_little_endian : boolean                        := false);

  port (
    clk_sys_i : in std_logic;
    rst_n_i   : in std_logic;

-------------------------------------------------------------------------------
-- Pipelined Wishbone interface
-------------------------------------------------------------------------------

    -- WBP Master (TX)
    src_o : out t_wrf_source_out;
    src_i : in  t_wrf_source_in;

    -- WBP Slave (RX)
    snk_o : out t_wrf_sink_out;
    snk_i : in  t_wrf_sink_in;

-------------------------------------------------------------------------------
-- TXTSU i/f
-------------------------------------------------------------------------------

    txtsu_port_id_i     : in  std_logic_vector(4 downto 0);
    txtsu_frame_id_i    : in  std_logic_vector(16 - 1 downto 0);
    txtsu_tsval_i       : in  std_logic_vector(28 + 4 - 1 downto 0);
    txtsu_tsincorrect_i : in  std_logic;
    txtsu_stb_i         : in  std_logic;
    txtsu_ack_o         : out std_logic;

-------------------------------------------------------------------------------
-- Wishbone slave
-------------------------------------------------------------------------------    

  wb_i : in  t_wishbone_slave_in;
  wb_o : out t_wishbone_slave_out
    );
end xwr_mini_nic;

architecture wrapper of xwr_mini_nic is

  component wr_mini_nic
    generic (
      g_interface_mode       : t_wishbone_interface_mode;
      g_address_granularity  : t_wishbone_address_granularity;
      g_tx_fifo_size         : integer;
      g_rx_fifo_size         : integer;
      g_buffer_little_endian : boolean);
    port (
      clk_sys_i           : in  std_logic;
      rst_n_i             : in  std_logic;
      src_dat_o           : out std_logic_vector(15 downto 0);
      src_adr_o           : out std_logic_vector(1 downto 0);
      src_sel_o           : out std_logic_vector(1 downto 0);
      src_cyc_o           : out std_logic;
      src_stb_o           : out std_logic;
      src_we_o            : out std_logic;
      src_stall_i         : in  std_logic;
      src_err_i           : in  std_logic;
      src_ack_i           : in  std_logic;
      snk_dat_i           : in  std_logic_vector(15 downto 0);
      snk_adr_i           : in  std_logic_vector(1 downto 0);
      snk_sel_i           : in  std_logic_vector(1 downto 0);
      snk_cyc_i           : in  std_logic;
      snk_stb_i           : in  std_logic;
      snk_we_i            : in  std_logic;
      snk_stall_o         : out std_logic;
      snk_err_o           : out std_logic;
      snk_ack_o           : out std_logic;
      txtsu_port_id_i     : in  std_logic_vector(4 downto 0);
      txtsu_frame_id_i    : in  std_logic_vector(16 - 1 downto 0);
      txtsu_tsval_i       : in  std_logic_vector(28 + 4 - 1 downto 0);
      txtsu_tsincorrect_i : in  std_logic;
      txtsu_stb_i         : in  std_logic;
      txtsu_ack_o         : out std_logic;
      wb_cyc_i            : in  std_logic;
      wb_stb_i            : in  std_logic;
      wb_we_i             : in  std_logic;
      wb_sel_i            : in  std_logic_vector(c_wishbone_data_width/8-1 downto 0);
      wb_adr_i            : in  std_logic_vector(c_wishbone_address_width-1 downto 0);
      wb_dat_i            : in  std_logic_vector(c_wishbone_data_width-1 downto 0);
      wb_dat_o            : out std_logic_vector(c_wishbone_data_width-1 downto 0);
      wb_ack_o            : out std_logic;
      wb_stall_o          : out std_logic;
      wb_int_o            : out std_logic);
  end component;
  
begin  -- wrapper

  U_Wrapped_Minic : wr_mini_nic
    generic map (
      g_interface_mode       => g_interface_mode,
      g_address_granularity  => g_address_granularity,
      g_tx_fifo_size         => g_tx_fifo_size,
      g_rx_fifo_size         => g_rx_fifo_size,
      g_buffer_little_endian => g_buffer_little_endian)
    port map (
      clk_sys_i           => clk_sys_i,
      rst_n_i             => rst_n_i,
      src_dat_o           => src_o.dat,
      src_adr_o           => src_o.adr,
      src_sel_o           => src_o.sel,
      src_cyc_o           => src_o.cyc,
      src_stb_o           => src_o.stb,
      src_we_o            => src_o.we,
      src_stall_i         => src_i.stall,
      src_err_i           => src_i.err,
      src_ack_i           => src_i.ack,
      snk_dat_i           => snk_i.dat,
      snk_adr_i           => snk_i.adr,
      snk_sel_i           => snk_i.sel,
      snk_cyc_i           => snk_i.cyc,
      snk_stb_i           => snk_i.stb,
      snk_we_i            => snk_i.we,
      snk_stall_o         => snk_o.stall,
      snk_err_o           => snk_o.err,
      snk_ack_o           => snk_o.ack,
      txtsu_port_id_i     => txtsu_port_id_i,
      txtsu_frame_id_i    => txtsu_frame_id_i,
      txtsu_tsval_i       => txtsu_tsval_i,
      txtsu_tsincorrect_i => txtsu_tsincorrect_i,
      txtsu_stb_i         => txtsu_stb_i,
      txtsu_ack_o         => txtsu_ack_o,
      wb_cyc_i            => wb_i.cyc,
      wb_stb_i            => wb_i.stb,
      wb_we_i             => wb_i.we,
      wb_sel_i            => wb_i.sel,
      wb_adr_i            => wb_i.adr,
      wb_dat_i            => wb_i.dat,
      wb_dat_o            => wb_o.dat,
      wb_ack_o            => wb_o.ack,
      wb_stall_o          => wb_o.stall,
      wb_int_o            => wb_o.int);


  wb_o.err <= '0';
  wb_o.rty <= '0';
  
  snk_o.rty <= '0';

end wrapper;
