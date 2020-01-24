-------------------------------------------------------------------------------
-- Title      : Simple Wishbone UART
-- Project    : General Cores Collection (gencores) library
-------------------------------------------------------------------------------
-- File       : wb_simple_uart.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-Co-HT
-- Created    : 2011-02-21
-- Last update: 2017-02-03
-- Platform   : FPGA-generics
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: A simple UART controller, providing two modes of operation
-- (both can be used simultenously):
-- - physical UART (encoding fixed to 8 data bits, no parity and one stop bit)
-- - virtual UART: TXed data is passed via a FIFO to the Wishbone host (and
--   vice versa).
-------------------------------------------------------------------------------
-- Copyright (c) 2011 CERN
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
-- 2011-02-21  1.0      twlostow        Created
-- 2011-10-04  1.1      twlostow        merged with VUART, added adapter
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

use work.genram_pkg.all;
use work.wishbone_pkg.all;
use work.UART_wbgen2_pkg.all;

entity wb_simple_uart is
  generic(
    g_with_virtual_uart   : boolean;
    g_with_physical_uart  : boolean;
    g_interface_mode      : t_wishbone_interface_mode      := CLASSIC;
    g_address_granularity : t_wishbone_address_granularity := WORD;
    g_vuart_fifo_size     : integer := 1024
    );
  port (

    clk_sys_i : in std_logic;
    rst_n_i   : in std_logic;

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
    uart_txd_o : out std_logic
    );
end wb_simple_uart;

architecture syn of wb_simple_uart is

  constant c_baud_acc_width  : integer := 16;

  component uart_baud_gen
    generic (
      g_baud_acc_width : integer);
    port (
      clk_sys_i    : in  std_logic;
      rst_n_i      : in  std_logic;
      baudrate_i   : in  std_logic_vector(g_baud_acc_width downto 0);
      baud_tick_o  : out std_logic;
      baud8_tick_o : out std_logic);
  end component;

  component uart_async_rx
    port (
      clk_sys_i    : in  std_logic;
      rst_n_i      : in  std_logic;
      baud8_tick_i : in  std_logic;
      rxd_i        : in  std_logic;
      rx_ready_o   : out std_logic;
      rx_error_o   : out std_logic;
      rx_data_o    : out std_logic_vector(7 downto 0));
  end component;

  component uart_async_tx
    port (
      clk_sys_i    : in  std_logic;
      rst_n_i      : in  std_logic;
      baud_tick_i  : in  std_logic;
      txd_o        : out std_logic;
      tx_start_p_i : in  std_logic;
      tx_data_i    : in  std_logic_vector(7 downto 0);
      tx_busy_o    : out std_logic);
  end component;

  component simple_uart_wb
    port (
      rst_n_i     : in  std_logic;
      clk_sys_i   : in  std_logic;
      wb_adr_i    : in  std_logic_vector(2 downto 0);
      wb_dat_i    : in  std_logic_vector(31 downto 0);
      wb_dat_o    : out std_logic_vector(31 downto 0);
      wb_cyc_i    : in  std_logic;
      wb_sel_i    : in  std_logic_vector(3 downto 0);
      wb_stb_i    : in  std_logic;
      wb_we_i     : in  std_logic;
      wb_ack_o    : out std_logic;
      wb_stall_o  : out std_logic;
      rdr_rack_o  : out std_logic;
      host_rack_o : out std_logic;
      regs_i      : in  t_uart_in_registers;
      regs_o      : out t_uart_out_registers
    );
  end component;

  signal rx_ready_reg    : std_logic;
  signal rx_ready        : std_logic;
  signal uart_bcr        : std_logic_vector(31 downto 0);

  signal rdr_rack         : std_logic;
  signal host_rack : std_logic;

  signal baud_tick  : std_logic;
  signal baud_tick8 : std_logic;

  signal resized_addr : std_logic_vector(c_wishbone_address_width-1 downto 0);

  signal wb_in  : t_wishbone_slave_in;
  signal wb_out : t_wishbone_slave_out;

  signal regs_in  : t_UART_in_registers;
  signal regs_out : t_UART_out_registers;

  signal fifo_empty, fifo_full, fifo_rd, fifo_wr : std_logic;
  signal fifo_count                              : std_logic_vector(f_log2_size(g_vuart_fifo_size)-1 downto 0);

  signal phys_rx_ready, phys_tx_busy : std_logic;
  
  signal phys_rx_data : std_logic_vector(7 downto 0);
  
  
begin  -- syn

  gen_check_generics : if(not g_with_physical_uart and not g_with_virtual_uart) generate
    assert false report "wb_simple_uart: dummy configuration (use virtual, physical or both uarts)" severity failure;
  end generate gen_check_generics;

  resized_addr(4 downto 0)                          <= wb_adr_i;
  resized_addr(c_wishbone_address_width-1 downto 5) <= (others => '0');

  U_Adapter : wb_slave_adapter
    generic map (
      g_master_use_struct  => true,
      g_master_mode        => CLASSIC,
      g_master_granularity => WORD,
      g_slave_use_struct   => false,
      g_slave_mode         => g_interface_mode,
      g_slave_granularity  => g_address_granularity)
    port map (
      clk_sys_i  => clk_sys_i,
      rst_n_i    => rst_n_i,
      master_i   => wb_out,
      master_o   => wb_in,
      sl_adr_i   => resized_addr,
      sl_dat_i   => wb_dat_i,
      sl_sel_i   => wb_sel_i,
      sl_cyc_i   => wb_cyc_i,
      sl_stb_i   => wb_stb_i,
      sl_we_i    => wb_we_i,
      sl_dat_o   => wb_dat_o,
      sl_ack_o   => wb_ack_o,
      sl_stall_o => wb_stall_o);

  U_WB_SLAVE : simple_uart_wb
    port map (
      rst_n_i    => rst_n_i,
      clk_sys_i  => clk_sys_i,
      wb_adr_i   => wb_in.adr(2 downto 0),
      wb_dat_i   => wb_in.dat,
      wb_dat_o   => wb_out.dat,
      wb_cyc_i   => wb_in.cyc,
      wb_sel_i   => wb_in.sel,
      wb_stb_i   => wb_in.stb,
      wb_we_i    => wb_in.we,
      wb_ack_o   => wb_out.ack,
      wb_stall_o => wb_out.stall,

      rdr_rack_o  => rdr_rack,
      host_rack_o => host_rack,
      regs_o      => regs_out,
      regs_i      => regs_in);

  wb_out.err <= '0';
  wb_out.rty <= '0';
  wb_out.int <= '0';

  gen_phys_uart : if(g_with_physical_uart) generate

    p_bcr_reg : process(clk_sys_i)
    begin
      if rising_edge(clk_sys_i) then
        if rst_n_i = '0' then
          uart_bcr <= (others => '0');
        elsif(regs_out.bcr_wr_o = '1')then
          uart_bcr <= regs_out.bcr_o;
        end if;
      end if;
    end process;

    U_BAUD_GEN : uart_baud_gen
      generic map (
        g_baud_acc_width => c_baud_acc_width)
      port map (
        clk_sys_i    => clk_sys_i,
        rst_n_i      => rst_n_i,
        baudrate_i   => uart_bcr(c_baud_acc_width downto 0),
        baud_tick_o  => baud_tick,
        baud8_tick_o => baud_tick8);

    U_TX : uart_async_tx
      port map (
        clk_sys_i    => clk_sys_i,
        rst_n_i      => rst_n_i,
        baud_tick_i  => baud_tick,
        txd_o        => uart_txd_o,
        tx_start_p_i => regs_out.tdr_tx_data_wr_o,
        tx_data_i    => regs_out.tdr_tx_data_o,
        tx_busy_o    => phys_tx_busy);

    U_RX : uart_async_rx
      port map (
        clk_sys_i    => clk_sys_i,
        rst_n_i      => rst_n_i,
        baud8_tick_i => baud_tick8,
        rxd_i        => uart_rxd_i,
        rx_ready_o   => phys_rx_ready,
        rx_error_o   => open,
        rx_data_o    => phys_rx_data);

  end generate gen_phys_uart;
  
  gen_vuart : if(g_with_virtual_uart) generate

    fifo_wr <= not fifo_full and regs_out.tdr_tx_data_wr_o;
    fifo_rd <= not fifo_empty and not regs_in.host_rdr_rdy_i;

    U_VUART_FIFO : generic_sync_fifo
      generic map (
        g_data_width => 8,
        g_size       => g_vuart_fifo_size,
        g_with_count => true)
      port map (
        rst_n_i => rst_n_i,
        clk_i   => clk_sys_i,
        d_i     => regs_out.tdr_tx_data_o,
        we_i    => fifo_wr,
        q_o     => regs_in.host_rdr_data_i,
        rd_i    => fifo_rd,
        empty_o => fifo_empty,
        full_o  => fifo_full,
        count_o => fifo_count);

    regs_in.host_rdr_count_i(fifo_count'left downto 0)    <= fifo_count;
    regs_in.host_rdr_count_i(15 downto fifo_count'length) <= (others => '0');

    p_vuart_rx_ready : process(clk_sys_i)
    begin
      if rising_edge(clk_sys_i) then
        if rst_n_i = '0' then
          regs_in.host_rdr_rdy_i <= '0';
        elsif(fifo_rd = '1') then
          regs_in.host_rdr_rdy_i <= '1';
        elsif(host_rack = '1') then
          regs_in.host_rdr_rdy_i <= '0';
        end if;
      end if;
    end process;

  end generate gen_vuart;

  p_drive_rx_ready : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        regs_in.sr_rx_rdy_i   <= '0';
        regs_in.rdr_rx_data_i <= (others => '0');
      else
        if(rdr_rack = '1' and phys_rx_ready = '0' and regs_out.host_tdr_data_wr_o = '0') then
          regs_in.sr_rx_rdy_i <= '0';
        elsif(phys_rx_ready = '1' and g_with_physical_uart) then
          regs_in.sr_rx_rdy_i   <= '1';
          regs_in.rdr_rx_data_i <= phys_rx_data;
        elsif(regs_out.host_tdr_data_wr_o = '1' and g_with_virtual_uart) then
          regs_in.sr_rx_rdy_i   <= '1';
          regs_in.rdr_rx_data_i <= regs_out.host_tdr_data_o;
        end if;
      end if;
    end if;
  end process;

  regs_in.sr_tx_busy_i <= phys_tx_busy when (g_with_physical_uart) else '0';
  regs_in.host_tdr_rdy_i <= not regs_in.sr_rx_rdy_i;
  
end syn;
