-------------------------------------------------------------------------------
-- Title      : AXI4Lite-to-WB bridge
-- Project    : General Cores
-------------------------------------------------------------------------------
-- File       : wb_axi4lite_bridge.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN
-- Platform   : FPGA-generics
-- Standard   : VHDL '93
-------------------------------------------------------------------------------
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
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.axi4_pkg.all;
use work.wishbone_pkg.all;

entity wb_axi4lite_bridge is
  port (
    clk_sys_i : in std_logic;
    rst_n_i   : in std_logic;

    ARVALID : in  std_logic;
    AWVALID : in  std_logic;
    BREADY  : in  std_logic;
    RREADY  : in  std_logic;
    WLAST   : in  std_logic;
    WVALID  : in  std_logic;
    ARADDR  : in  std_logic_vector (31 downto 0);
    AWADDR  : in  std_logic_vector (31 downto 0);
    WDATA   : in  std_logic_vector (31 downto 0);
    WSTRB   : in  std_logic_vector (3 downto 0);
    ARREADY : out std_logic;
    AWREADY : out std_logic;
    BVALID  : out std_logic;
    RLAST   : out std_logic;
    RVALID  : out std_logic;
    WREADY  : out std_logic;
    BRESP   : out std_logic_vector (1 downto 0);
    RRESP   : out std_logic_vector (1 downto 0);
    RDATA   : out std_logic_vector (31 downto 0);

    wb_adr : out std_logic_vector(c_wishbone_address_width-1 downto 0);
    wb_dat_m2s : out std_logic_vector(c_wishbone_data_width-1 downto 0);
    wb_sel : out std_logic_vector(c_wishbone_data_width/8-1 downto 0);
    wb_cyc : out std_logic;
    wb_stb : out std_logic;
    wb_we  : out std_logic;

    wb_dat_s2m   : in std_logic_vector(c_wishbone_data_width-1 downto 0);
    wb_err   : in std_logic := '0';
    wb_rty   : in std_logic := '0';
    wb_ack   : in std_logic;
    wb_stall : in std_logic
    );

end wb_axi4lite_bridge;

architecture rtl of wb_axi4lite_bridge is

  signal axi_in  : t_axi4_lite_master_out_32;
  signal axi_out : t_axi4_lite_master_in_32;
  signal wb_in   : t_wishbone_master_in;
  signal wb_out  : t_wishbone_master_out;
  
begin

  axi_in.ARVALID  <= ARVALID;
  axi_in.AWVALID <= AWVALID;
  axi_in.BREADY  <= BREADY;
  axi_in.RREADY  <= RREADY;
  axi_in.WLAST   <= WLAST;
  axi_in.WVALID  <= WVALID;
  axi_in.ARADDR  <= ARADDR;
  axi_in.AWADDR  <= AWADDR;
  axi_in.WDATA   <= WDATA;
  axi_in.WSTRB   <= WSTRB;
  ARREADY         <= axi_out.ARREADY;
  AWREADY         <= axi_out.AWREADY;
  BVALID          <= axi_out.BVALID;
  RLAST           <= axi_out.RLAST;
  RVALID          <= axi_out.RVALID;
  WREADY          <= axi_out.WREADY;
  BRESP           <= axi_out.BRESP;
  RRESP           <= axi_out.RRESP;
  RDATA           <= axi_out.RDATA;

  wb_adr <= wb_out.adr;
  wb_dat_m2s <= wb_out.dat;
  wb_stb <= wb_out.stb;
  wb_sel <= wb_out.sel;
  wb_cyc <= wb_out.cyc;
  wb_we <= wb_out.we;

  wb_in.err <= wb_err;
  wb_in.rty <= wb_rty;
  wb_in.ack <= wb_ack;
  wb_in.int <= '0';
  wb_in.stall <= wb_stall;
  wb_in.dat <= wb_dat_s2m;

  U_Wrapped_Bridge : xwb_axi4lite_bridge
    port map (
      clk_sys_i    => clk_sys_i,
      rst_n_i      => rst_n_i,
      axi4_slave_i => axi_in,
      axi4_slave_o => axi_out,
      wb_master_o  => wb_out,
      wb_master_i  => wb_in);
  
end rtl;

