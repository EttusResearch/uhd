-------------------------------------------------------------------------------
-- Title      : WR PTP Core Diagnostics
-- Project    : WhiteRabbit
-------------------------------------------------------------------------------
-- File       : xwrc_diags_wb.vhd
-- Author     : Maciej Lipinski <maciej.lipinski@cern.ch>
-- Company    : CERN
-- Created    : 2017-04-24
-- Platform   : FPGA-generics
-- Standard   : VHDL
-------------------------------------------------------------------------------
-- Description:
-- Wrapper for wrc_diags_wb. Uses types instead of std_logic signals and
-- can use pipelined or classic wishbone.
--
-------------------------------------------------------------------------------
--
-- Copyright (c) 2017 CERN
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
use work.wishbone_pkg.all;
use work.wrc_diags_wbgen2_pkg.all;

entity xwr_diags_wb is
  generic(
    g_interface_mode      : t_wishbone_interface_mode      := CLASSIC;
    g_address_granularity : t_wishbone_address_granularity := WORD
  );
  port (
    rst_n_i   : in  std_logic;
    clk_sys_i : in  std_logic;

    slave_i   : in  t_wishbone_slave_in;
    slave_o   : out t_wishbone_slave_out;

    regs_i    : in  t_wrc_diags_in_registers;
    regs_o    : out t_wrc_diags_out_registers
  );
end xwr_diags_wb;

architecture syn of xwr_diags_wb is

  component wrc_diags_wb is
    port (
      rst_n_i                                  : in     std_logic;
      clk_sys_i                                : in     std_logic;
      wb_adr_i                                 : in     std_logic_vector(4 downto 0);
      wb_dat_i                                 : in     std_logic_vector(31 downto 0);
      wb_dat_o                                 : out    std_logic_vector(31 downto 0);
      wb_cyc_i                                 : in     std_logic;
      wb_sel_i                                 : in     std_logic_vector(3 downto 0);
      wb_stb_i                                 : in     std_logic;
      wb_we_i                                  : in     std_logic;
      wb_ack_o                                 : out    std_logic;
      wb_stall_o                               : out    std_logic;
      regs_i                                   : in     t_wrc_diags_in_registers;
      regs_o                                   : out    t_wrc_diags_out_registers
    );
  end component;


  signal wb_out : t_wishbone_slave_out;
  signal wb_in  : t_wishbone_slave_in;

begin

  U_Adapter : wb_slave_adapter
    generic map(
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
      sl_adr_i   => slave_i.adr,
      sl_dat_i   => slave_i.dat,
      sl_sel_i   => slave_i.sel,
      sl_cyc_i   => slave_i.cyc,
      sl_stb_i   => slave_i.stb,
      sl_we_i    => slave_i.we,
      sl_dat_o   => slave_o.dat, 
      sl_ack_o   => slave_o.ack,
      sl_stall_o => slave_o.stall);

  WRAPPED_DIAGS: wrc_diags_wb
    port map(
      rst_n_i    => rst_n_i,
      clk_sys_i  => clk_sys_i,
      wb_adr_i   => wb_in.adr(4 downto 0),
      wb_dat_i   => wb_in.dat,
      wb_dat_o   => wb_out.dat,
      wb_cyc_i   => wb_in.cyc,
      wb_sel_i   => wb_in.sel,
      wb_stb_i   => wb_in.stb,
      wb_we_i    => wb_in.we,
      wb_ack_o   => wb_out.ack,
      wb_stall_o => wb_out.stall,
      regs_i     => regs_i,
      regs_o     => regs_o);

  slave_o.err <= '0';
  slave_o.rty <= '0';
  slave_o.int <= '0';

end syn;



