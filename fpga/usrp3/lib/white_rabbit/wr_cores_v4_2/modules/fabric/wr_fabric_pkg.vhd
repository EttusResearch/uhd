-------------------------------------------------------------------------------
-- Title      : Wishbone Packet Fabric package
-- Project    : WR Cores Collection
-------------------------------------------------------------------------------
-- File       : wr_fabric_pkg.vhd
-- Author     : Grzegorz Daniluk
-- Company    : CERN BE-CO-HT
-- Platform   : 
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
--
-- Copyright (c) 2012-2017 CERN
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


package wr_fabric_pkg is

  constant c_WRF_DATA   : std_logic_vector(1 downto 0) := "00";
  constant c_WRF_OOB    : std_logic_vector(1 downto 0) := "01";
  constant c_WRF_STATUS : std_logic_vector(1 downto 0) := "10";
  constant c_WRF_USER   : std_logic_vector(1 downto 0) := "11";

  constant c_WRF_OOB_TYPE_RX : std_logic_vector(3 downto 0) := "0000";
  constant c_WRF_OOB_TYPE_TX : std_logic_vector(3 downto 0) := "0001";

  type t_wrf_mux_class is array (natural range <>) of std_logic_vector(7 downto 0);

  type t_wrf_status_reg is record
    is_hp       : std_logic;
    has_smac    : std_logic;
    has_crc     : std_logic;
    error       : std_logic;
    tag_me      : std_logic;
    match_class : std_logic_vector(7 downto 0);
  end record;

  type t_wrf_source_out is record
    adr : std_logic_vector(1 downto 0);
    dat : std_logic_vector(15 downto 0);
    cyc : std_logic;
    stb : std_logic;
    we  : std_logic;
    sel : std_logic_vector(1 downto 0);
  end record;

  type t_wrf_source_in is record
    ack   : std_logic;
    stall : std_logic;
    err   : std_logic;
    rty   : std_logic;
  end record;


  type t_wrf_oob is record
    valid: std_logic;
    oob_type : std_logic_vector(3 downto 0);
    ts_r     : std_logic_vector(27 downto 0);
    ts_f     : std_logic_vector(3 downto 0);
    frame_id : std_logic_vector(15 downto 0);
    port_id  : std_logic_vector(5 downto 0);
  end record;

  subtype t_wrf_sink_in is t_wrf_source_out;
  subtype t_wrf_sink_out is t_wrf_source_in;

  type t_wrf_source_in_array is array (natural range <>) of t_wrf_source_in;
  type t_wrf_source_out_array is array (natural range <>) of t_wrf_source_out;

  subtype t_wrf_sink_in_array is t_wrf_source_out_array;
  subtype t_wrf_sink_out_array is t_wrf_source_in_array;

  function f_marshall_wrf_status (stat  : t_wrf_status_reg) return std_logic_vector;
  function f_unmarshall_wrf_status(stat : std_logic_vector) return t_wrf_status_reg;

  constant c_wrf_status_init_value : t_wrf_status_reg :=
    ('0', '0', '0', '0', '0', (others => '0'));

  constant c_dummy_src_in : t_wrf_source_in :=
    ('0', '0', '0', '0');
  constant c_dummy_snk_in : t_wrf_sink_in :=
    ("XX", "XXXXXXXXXXXXXXXX", '0', '0', '0', "XX");


  -----------------------------------------------------------------------------
  -- WRF MUX
  -----------------------------------------------------------------------------
  component xwrf_mux is
    generic(
      g_muxed_ports	:	integer := 2);
    port(
      clk_sys_i	: in std_logic;
      rst_n_i		: in std_logic;
      --ENDPOINT
      ep_src_o	:	out t_wrf_source_out;
      ep_src_i	: in  t_wrf_source_in;
      ep_snk_o	: out t_wrf_sink_out;
      ep_snk_i  : in  t_wrf_sink_in;
      --Muxed ports
      mux_src_o : out t_wrf_source_out_array(g_muxed_ports-1 downto 0);
      mux_src_i : in  t_wrf_source_in_array(g_muxed_ports-1 downto 0);
      mux_snk_o : out t_wrf_sink_out_array(g_muxed_ports-1 downto 0);
      mux_snk_i : in  t_wrf_sink_in_array(g_muxed_ports-1 downto 0);
      --
      mux_class_i : in t_wrf_mux_class(g_muxed_ports-1 downto 0)
    );
  end component;

  component xwrf_reg is
    generic(
      g_adr_width : integer := 2;
      g_dat_width : integer :=16);
    port(
      rst_n_i	:	in std_logic;
      clk_i   : in std_logic;
      snk_i		: in  t_wrf_sink_in;
      snk_o		:	out t_wrf_sink_out;
      src_i		:	in 	t_wrf_source_in;
      src_o		:	out t_wrf_source_out);
  end component;

  component xwrf_loopback
    generic(
      g_interface_mode        : t_wishbone_interface_mode      := CLASSIC;
      g_address_granularity   : t_wishbone_address_granularity := WORD);
    port(
      clk_sys_i : in  std_logic;
      rst_n_i   : in  std_logic;
  
      wrf_snk_i : in  t_wrf_sink_in;
      wrf_snk_o : out t_wrf_sink_out;
      wrf_src_o : out t_wrf_source_out;
      wrf_src_i : in  t_wrf_source_in;
  
      wb_i : in  t_wishbone_slave_in;
      wb_o : out t_wishbone_slave_out);
  end component;

  component xwb_fabric_sink
    port (
      clk_i     : in  std_logic;
      rst_n_i   : in  std_logic;
      snk_i     : in  t_wrf_sink_in;
      snk_o     : out t_wrf_sink_out;
      addr_o    : out std_logic_vector(1 downto 0);
      data_o    : out std_logic_vector(15 downto 0);
      dvalid_o  : out std_logic;
      sof_o     : out std_logic;
      eof_o     : out std_logic;
      error_o   : out std_logic;
      bytesel_o : out std_logic;
      dreq_i    : in  std_logic);
  end component;

  component xwb_fabric_source
    port (
      clk_i     : in  std_logic;
      rst_n_i   : in  std_logic;
      src_i     : in  t_wrf_source_in;
      src_o     : out t_wrf_source_out;
      addr_i    : in  std_logic_vector(1 downto 0);
      data_i    : in  std_logic_vector(15 downto 0);
      dvalid_i  : in  std_logic;
      sof_i     : in  std_logic;
      eof_i     : in  std_logic;
      error_i   : in  std_logic;
      bytesel_i : in  std_logic;
      dreq_o    : out std_logic);
  end component;

end wr_fabric_pkg;

package body wr_fabric_pkg is

  function f_marshall_wrf_status(stat : t_wrf_status_reg)
    return std_logic_vector is
    variable tmp : std_logic_vector(15 downto 0);
  begin
    tmp(0)           := stat.is_hp;
    tmp(1)           := stat.error;
    tmp(2)           := stat.has_smac;
    tmp(3)           := stat.has_crc;
    tmp(15 downto 8) := stat.match_class;
    return tmp;
  end function;

  function f_unmarshall_wrf_status(stat : std_logic_vector) return t_wrf_status_reg is
    variable tmp : t_wrf_status_reg;
  begin
    tmp.is_hp       := stat(0);
    tmp.error       := stat(1);
    tmp.has_smac    := stat(2);
    tmp.has_crc     := stat(3);
    tmp.match_class := stat(15 downto 8);
    return tmp;
    
  end function;


end wr_fabric_pkg;
