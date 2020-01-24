------------------------------------------------------------------------------
-- Title      : Simple Wishbone UART
-- Project    : General Cores Collection (gencores) library
------------------------------------------------------------------------------
-- File       : xwb_simple_uart.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-Co-HT
-- Created    : 2010-05-18
-- Last update: 2017-02-03
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: A simple UART controller, providing two modes of operation
-- (both can be used simultenously):
-- - physical UART (encoding fixed to 8 data bits, no parity and one stop bit)
-- - virtual UART: TXed data is passed via a FIFO to the Wishbone host (and
--   vice versa).
-------------------------------------------------------------------------------
-- Copyright (c) 2010 CERN
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
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author          Description
-- 2010-05-18  1.0      twlostow        Created
-- 2011-10-04  1.1      twlostow        xwb module
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

library work;
use work.wishbone_pkg.all;

entity xwb_simple_uart is
  generic(
    g_with_virtual_uart   : boolean := true;
    g_with_physical_uart  : boolean := true;
    g_interface_mode      : t_wishbone_interface_mode      := CLASSIC;
    g_address_granularity : t_wishbone_address_granularity := WORD;
    g_vuart_fifo_size     : integer := 1024
    );

  port(
    clk_sys_i : in std_logic;
    rst_n_i   : in std_logic;

    -- Wishbone
    slave_i : in  t_wishbone_slave_in;
    slave_o : out t_wishbone_slave_out;
    desc_o  : out t_wishbone_device_descriptor;

    uart_rxd_i: in std_logic;
    uart_txd_o: out std_logic

    );

end xwb_simple_uart;

architecture rtl of xwb_simple_uart is

  component wb_simple_uart
    generic (
      g_with_virtual_uart   : boolean;
      g_with_physical_uart  : boolean;
      g_interface_mode      : t_wishbone_interface_mode;
      g_address_granularity : t_wishbone_address_granularity;
      g_vuart_fifo_size     : integer);
    port (
      clk_sys_i  : in  std_logic;
      rst_n_i    : in  std_logic;
      wb_adr_i   : in  std_logic_vector(4 downto 0);
      wb_dat_i   : in  std_logic_vector(31 downto 0);
      wb_dat_o   : out std_logic_vector(31 downto 0);
      wb_cyc_i   : in  std_logic;
      wb_sel_i   : in  std_logic_vector(3 downto 0);
      wb_stb_i   : in  std_logic;
      wb_we_i    : in  std_logic;
      wb_ack_o   : out std_logic;
      wb_stall_o : out std_logic;
      uart_rxd_i : in  std_logic;
      uart_txd_o : out std_logic);
  end component;
  
begin  -- rtl

  U_Wrapped_UART: wb_simple_uart
    generic map (
      g_with_virtual_uart   => g_with_virtual_uart,
      g_with_physical_uart  => g_with_physical_uart,
      g_interface_mode      => g_interface_mode,
      g_address_granularity => g_address_granularity,
      g_vuart_fifo_size     => g_vuart_fifo_size)
    port map (
      clk_sys_i  => clk_sys_i,
      rst_n_i    => rst_n_i,
      wb_adr_i   => slave_i.adr(4 downto 0),
      wb_dat_i   => slave_i.dat,
      wb_dat_o   => slave_o.dat,
      wb_cyc_i   => slave_i.cyc,
      wb_sel_i   => slave_i.sel,
      wb_stb_i   => slave_i.stb,
      wb_we_i    => slave_i.we,
      wb_ack_o   => slave_o.ack,
      wb_stall_o => slave_o.stall,
      uart_rxd_i => uart_rxd_i,
      uart_txd_o => uart_txd_o);

  slave_o.err <= '0';
  slave_o.rty <= '0';
  slave_o.int <='0';

  desc_o <= (others => '0');
  
end rtl;
