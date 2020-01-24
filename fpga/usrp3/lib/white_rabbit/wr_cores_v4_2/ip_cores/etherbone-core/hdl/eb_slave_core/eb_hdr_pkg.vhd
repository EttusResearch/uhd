--! @file eb_hdr_pkg.vhd
--! @brief EtherBone Header definitions - Eth, IPV4, UDP, EB
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

library IEEE;
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

library work;
use work.wishbone_pkg.all;

package eb_hdr_pkg is

  constant c_eth_typ_ip : std_logic_vector((2*8)-1 downto 0) := x"0800";
  type t_eth_hdr is record
    dst : std_logic_vector((6*8)-1 downto 0); 
    src : std_logic_vector((6*8)-1 downto 0);
    typ : std_logic_vector((2*8)-1 downto 0);
  end record;
  constant c_eth_len : natural := 14;
  constant c_eth_init : t_eth_hdr := (
    dst => (others => '0'), -- set!
    src => (others => '0'), -- set!
    typ => c_eth_typ_ip);
  
  constant c_ip_typ_udp : std_logic_vector(7 downto 0) := x"11";
  type t_ip_hdr is record
    ver : std_logic_vector(3 downto 0);
    ihl : std_logic_vector(3 downto 0);
    tos : std_logic_vector(7 downto 0);
    tol : std_logic_vector(15 downto 0);
    id  : std_logic_vector(15 downto 0);
    flg : std_logic_vector(2 downto 0);
    fro : std_logic_vector(12 downto 0);
    ttl : std_logic_vector(7 downto 0);
    pro : std_logic_vector(7 downto 0);
    sum : std_logic_vector(15 downto 0);
    src : std_logic_vector(31 downto 0);
    dst : std_logic_vector(31 downto 0);
  end record;
  constant c_ip_len : natural := 20;
  constant c_ip_init : t_ip_hdr := (
    ver => x"4",
    ihl => x"5",
    tos => x"00",
    tol => (others => '0'),  -- set!
    id  => (others => '0'),
    flg => "010", -- don't fragment
    fro => (others => '0'),
    ttl => x"3f",
    pro => c_ip_typ_udp,
    sum => (others => '0'),  -- set!
    src => (others => '0'),  -- set!
    dst => (others => '0')); -- set!
  
  type t_udp_hdr is record
    src : std_logic_vector(15 downto 0);
    dst : std_logic_vector(15 downto 0);
    len : std_logic_vector(15 downto 0);
    sum : std_logic_vector(15 downto 0);
  end record;
  constant c_udp_len : natural := 8;
  constant c_udp_init : t_udp_hdr := (
    src => (others => '0'),  -- set!
    dst => (others => '0'),  -- set!
    len => (others => '0'),  -- set!
    sum => (others => '0'));
  
  constant c_eb_magic : std_logic_vector(15 downto 0) := x"4e6f";
  constant c_eb_ver   : std_logic_vector(3 downto 0) := x"1";
  type t_eb_hdr is record
    magic       : std_logic_vector(15 downto 0);
    ver         : std_logic_vector(3 downto 0);
    res1        : std_logic;
    no_response : std_logic;
    probe_res   : std_logic;
    probe       : std_logic;
    addr_size   : std_logic_vector(3 downto 0);
    data_size   : std_logic_vector(3 downto 0);
  end record;
  constant c_eb_len : natural := 4;
  constant c_eb_init : t_eb_hdr := (
    magic       => c_eb_magic,
    ver         => c_eb_ver,
    res1        => '0',
    no_response => '1',
    probe_res   => '0',
    probe       => '0',
    addr_size   => x"4", -- 32-bit only
    data_size   => x"4");
  
  type t_rec_hdr is record
    bca_cfg  : std_logic;
    rca_cfg  : std_logic;
    rd_fifo  : std_logic;
    res1     : std_logic;
    drop_cyc : std_logic;
    wca_cfg  : std_logic;
    wr_fifo  : std_logic;
    res2     : std_logic;
    sel      : std_logic_vector(7 downto 0);
    wr_cnt   : unsigned(7 downto 0);
    rd_cnt   : unsigned(7 downto 0);
  end record;
  constant c_rec_len : natural := 4;
  constant c_rec_init : t_rec_hdr := (
    bca_cfg  => '0',
    rca_cfg  => '0',
    rd_fifo  => '0',
    res1     => '0',
    drop_cyc => '0',
    wca_cfg  => '0',
    wr_fifo  => '0',
    res2     => '0',
    sel      => (others => '0'),
    wr_cnt   => (others => '0'),
    rd_cnt   => (others => '0'));
  
  function f_parse_eth(x : std_logic_vector) return t_eth_hdr;
  function f_parse_ip (x : std_logic_vector) return t_ip_hdr;
  function f_parse_udp(x : std_logic_vector) return t_udp_hdr;
  function f_parse_eb (x : std_logic_vector) return t_eb_hdr;
  function f_parse_rec(x : std_logic_vector) return t_rec_hdr;
  
  function f_format_eth(x : t_eth_hdr) return std_logic_vector; --(c_eth_len*8-1 downto 0);
  function f_format_ip (x : t_ip_hdr)  return std_logic_vector; --(c_ip_len *8-1 downto 0);
  function f_format_udp(x : t_udp_hdr) return std_logic_vector; --(c_udp_len*8-1 downto 0);
  function f_format_eb (x : t_eb_hdr)  return std_logic_vector; --(c_eb_len *8-1 downto 0);
  function f_format_rec(x : t_rec_hdr) return std_logic_vector; --(c_rec_len*8-1 downto 0);
  
  -- To be used only on constants!
  function f_checksum(x : std_logic_vector) return std_logic_vector;
  
end package;

package body eb_hdr_pkg is

  function f_parse_eth(x : std_logic_vector) return t_eth_hdr is
    variable o : t_eth_hdr;
  begin
    o.dst := x(111 downto 64);
    o.src := x( 63 downto 16);
    o.typ := x( 15 downto  0);
    return o;
  end function;
   
  function f_parse_ip(x : std_logic_vector) return t_ip_hdr is
    variable o : t_ip_hdr;
  begin
    o.ver := x(159 downto 156);
    o.ihl := x(155 downto 152);
    o.tos := x(151 downto 144);
    o.tol := x(143 downto 128);
    o.id  := x(127 downto 112);
    o.flg := x(111 downto 109);
    o.fro := x(108 downto 96);
    o.ttl := x(95 downto 88);
    o.pro := x(87 downto 80);
    o.sum := x(79 downto 64);
    o.src := x(63 downto 32);
    o.dst := x(31 downto 0);
    return o;
  end function;
   
  function f_parse_udp(x : std_logic_vector) return t_udp_hdr is
    variable o : t_udp_hdr;
  begin
    o.src := x(63 downto 48);
    o.dst := x(47 downto 32);
    o.len := x(31 downto 16);
    o.sum := x(15 downto  0);
    return o;
  end function;
  
  function f_parse_eb(x : std_logic_vector) return t_eb_hdr is
    variable o : t_eb_hdr;
  begin
    o.magic       := x(31 downto 16);
    o.ver         := x(15 downto 12);
    o.res1        := x(11);
    o.no_response := x(10);
    o.probe_res   := x(9);
    o.probe       := x(8);
    o.addr_size   := x( 7 downto  4);
    o.data_size   := x( 3 downto  0);
    return o;
  end function;
   
  function f_parse_rec(x : std_logic_vector) return t_rec_hdr is
    variable o : t_rec_hdr;
  begin
    o.bca_cfg  := x(31);
    o.rca_cfg  := x(30);
    o.rd_fifo  := x(29);
    o.res1     := x(28);
    o.drop_cyc := x(27);
    o.wca_cfg  := x(26);
    o.wr_fifo  := x(25);
    o.res2     := x(24);
    o.sel      := x(23 downto 16);
    o.wr_cnt   := unsigned(x(15 downto 8));
    o.rd_cnt   := unsigned(x( 7 downto 0));
    return o;
  end function;
   
  function f_format_eth(x : t_eth_hdr) return std_logic_vector is
  begin
    return x.dst & x.src & x.typ;
  end function;
  
  function f_format_ip (x : t_ip_hdr) return std_logic_vector is
  begin
    return x.ver & x.ihl & x.tos & x.tol & x.id  & x.flg &
           x.fro & x.ttl & x.pro & x.sum & x.src & x.dst;
  end function;
  
  function f_format_udp(x : t_udp_hdr) return std_logic_vector is
  begin
    return x.src & x.dst & x.len & x.sum;
  end function;
  
  function f_format_eb (x : t_eb_hdr)  return std_logic_vector is
  begin
    return x.magic & x.ver & x.res1 & x.no_response & x.probe_res &
           x.probe & x.addr_size & x.data_size;
  end function;
  
  function f_format_rec(x : t_rec_hdr) return std_logic_vector is
  begin
    return x.bca_cfg  & x.rca_cfg & x.rd_fifo & x.res1 & 
           x.drop_cyc & x.wca_cfg & x.wr_fifo & x.res2 & 
           x.sel & std_logic_vector(x.wr_cnt) & std_logic_vector(x.rd_cnt);
  end function;
  
  function f_checksum(x : std_logic_vector) return std_logic_vector is
    alias y : std_logic_vector((x'length/16)*16-1 downto 0) is x;
    variable o : unsigned(16 downto 0) := (others => '0');
  begin
    for i in y'length/16-1 downto 0 loop
      o := o + ('0' & unsigned(y(i*16+15 downto i*16)));
      if o(16) = '1' then
        o := o + 1;
        o(16) := '0';
      end if;
    end loop;
    return std_logic_vector(o(15 downto 0));
  end function;
  
end package body;
