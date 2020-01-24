-------------------------------------------------------------------------------
-- Title      : AXI4Lite-to-WB bridge package
-- Project    : General Cores
-------------------------------------------------------------------------------
-- File       : axi4_pkg.vhd
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

use work.wishbone_pkg.all;

package axi4_pkg is

  -- AXI4-Full interface, master output ports, 32 bits
  type t_axi4_full_master_out_32 is record
    ARVALID : std_logic;
    AWVALID : std_logic;
    BREADY  : std_logic;
    RREADY  : std_logic;
    WLAST   : std_logic;
    WVALID  : std_logic;
    ARID    : std_logic_vector (11 downto 0);
    AWID    : std_logic_vector (11 downto 0);
    WID     : std_logic_vector (11 downto 0);
    ARBURST : std_logic_vector (1 downto 0);
    ARLOCK  : std_logic_vector (1 downto 0);
    ARSIZE  : std_logic_vector (2 downto 0);
    AWBURST : std_logic_vector (1 downto 0);
    AWLOCK  : std_logic_vector (1 downto 0);
    AWSIZE  : std_logic_vector (2 downto 0);
    ARPROT  : std_logic_vector (2 downto 0);
    AWPROT  : std_logic_vector (2 downto 0);
    ARADDR  : std_logic_vector (31 downto 0);
    AWADDR  : std_logic_vector (31 downto 0);
    WDATA   : std_logic_vector (31 downto 0);
    ARCACHE : std_logic_vector (3 downto 0);
    ARLEN   : std_logic_vector (3 downto 0);
    ARQOS   : std_logic_vector (3 downto 0);
    AWCACHE : std_logic_vector (3 downto 0);
    AWLEN   : std_logic_vector (3 downto 0);
    AWQOS   : std_logic_vector (3 downto 0);
    WSTRB   : std_logic_vector (3 downto 0);
  end record;

  -- AXI4-Full interface, master input ports, 32 bits
  type t_axi4_full_master_in_32 is record
    ARREADY : std_logic;
    AWREADY : std_logic;
    BVALID  : std_logic;
    RLAST   : std_logic;
    RVALID  : std_logic;
    WREADY  : std_logic;
    BID     : std_logic_vector (11 downto 0);
    RID     : std_logic_vector (11 downto 0);
    BRESP   : std_logic_vector (1 downto 0);
    RRESP   : std_logic_vector (1 downto 0);
    RDATA   : std_logic_vector (31 downto 0);
  end record;

  -- AXI4-Lite interface, master output ports, 32 bits
  type t_axi4_lite_master_out_32 is record
    ARVALID : std_logic;
    AWVALID : std_logic;
    BREADY  : std_logic;
    RREADY  : std_logic;
    WLAST   : std_logic;
    WVALID  : std_logic;
    ARADDR  : std_logic_vector (31 downto 0);
    AWADDR  : std_logic_vector (31 downto 0);
    WDATA   : std_logic_vector (31 downto 0);
    WSTRB   : std_logic_vector (3 downto 0);
  end record;

  -- AXI4-Lite interface, master input ports, 32 bits
  type t_axi4_lite_master_in_32 is record
    ARREADY : std_logic;
    AWREADY : std_logic;
    BVALID  : std_logic;
    RLAST   : std_logic;
    RVALID  : std_logic;
    WREADY  : std_logic;
    BRESP   : std_logic_vector (1 downto 0);
    RRESP   : std_logic_vector (1 downto 0);
    RDATA   : std_logic_vector (31 downto 0);
  end record;

  constant c_axi4_lite_default_master_in_32 : t_axi4_lite_master_in_32 :=
    (
      AWREADY => '0',
      ARREADY => '0',
      BVALID  => '0',
      RLAST   => '0',
      RVALID  => '0',
      WREADY  => '0',
      BRESP   => "00",
      RRESP   => "00",
      RDATA   => (others => '0')
      );


  constant c_axi4_lite_default_master_out_32 : t_axi4_lite_master_out_32 :=
    (
      ARVALID => '0',
      AWVALID => '0',
      BREADY  => '0',
      RREADY  => '0',
      WLAST   => '0',
      WVALID  => '0',
      ARADDR  => (others => '0'),
      AWADDR  => (others => '0'),
      WDATA   => (others => '0'),
      WSTRB   => (others => '0')
      );



  subtype t_axi4_lite_slave_in_32 is t_axi4_lite_master_out_32;
  subtype t_axi4_lite_slave_out_32 is t_axi4_lite_master_in_32;

  constant c_AXI4_RESP_OKAY   : std_logic_vector(1 downto 0) := "00";
  constant c_AXI4_RESP_EXOKAY : std_logic_vector(1 downto 0) := "01";
  constant c_AXI4_RESP_SLVERR : std_logic_vector(1 downto 0) := "10";
  constant c_AXI4_RESP_DECERR : std_logic_vector(1 downto 0) := "11";

    function f_axi4_full_to_lite (
    f : t_axi4_full_master_out_32
    )  return t_axi4_lite_master_out_32;
  
  function f_axi4_lite_to_full (
    l : t_axi4_lite_master_in_32
    )  return t_axi4_full_master_in_32;

  component xwb_axi4lite_bridge is
    port (
      clk_sys_i    : in  std_logic;
      rst_n_i      : in  std_logic;
      axi4_slave_i : in  t_axi4_lite_slave_in_32;
      axi4_slave_o : out t_axi4_lite_slave_out_32;
      wb_master_o  : out t_wishbone_master_out;
      wb_master_i  : in  t_wishbone_master_in);
  end component xwb_axi4lite_bridge;

  component wb_axi4lite_bridge is
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
  end component;
  
end package;

package body axi4_pkg is

    function f_axi4_full_to_lite (
    f : t_axi4_full_master_out_32
    )  return t_axi4_lite_master_out_32 is
    variable l : t_axi4_lite_master_out_32;
  begin

    l.ARVALID := f.ARVALID;
    l.AWVALID := f.AWVALID;
    l.BREADY  := f.BREADY;
    l.RREADY  := f.RREADY;
    l.WLAST   := f.WLAST;
    l.WVALID  := f.WVALID;
    l.ARADDR  := f.ARADDR;
    l.AWADDR  := f.AWADDR;
    l.WDATA   := f.WDATA;
    l.WSTRB   := f.WSTRB;

    return l;
    
  end f_axi4_full_to_lite;

  function f_axi4_lite_to_full (
    l : t_axi4_lite_master_in_32
    )  return t_axi4_full_master_in_32 is
    variable f : t_axi4_full_master_in_32;
  begin
    f.ARREADY := l.ARREADY;
    f.AWREADY := l.AWREADY;
    f.BVALID  := l.BVALID;
    f.RLAST   := l.RLAST;
    f.RVALID  := l.RVALID;
    f.WREADY  := l.WREADY;
    f.BID     := (others => '0');
    f.RID     := (others => '0');
    f.BRESP   := l.BRESP;
    f.RRESP   := l.RRESP;
    f.RDATA   := l.RDATA;

    return f;
    
  end f_axi4_lite_to_full;


end package body;
