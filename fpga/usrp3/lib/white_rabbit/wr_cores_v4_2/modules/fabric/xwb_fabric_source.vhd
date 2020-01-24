-------------------------------------------------------------------------------
-- Title      : Wishbone Packet Fabric buffered packet source
-- Project    : WR Cores Collection
-------------------------------------------------------------------------------
-- File       : xwb_fabric_source.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2012-01-16
-- Last update: 2012-01-22
-- Platform   : 
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
--
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
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

use work.genram_pkg.all;
use work.wr_fabric_pkg.all;

entity xwb_fabric_source is
  
  port (
    clk_i   : in std_logic;
    rst_n_i : in std_logic;

    -- Wishbone Fabric Interface I/O
    src_i : in  t_wrf_source_in;
    src_o : out t_wrf_source_out;

    -- Decoded & buffered fabric
    addr_i    : in  std_logic_vector(1 downto 0);
    data_i    : in  std_logic_vector(15 downto 0);
    dvalid_i  : in  std_logic;
    sof_i     : in  std_logic;
    eof_i     : in  std_logic;
    error_i   : in  std_logic;
    bytesel_i : in  std_logic;
    dreq_o    : out std_logic
    );

end xwb_fabric_source;

architecture rtl of xwb_fabric_source is

  constant c_fifo_width : integer := 16 + 2 + 4;

  signal q_valid, full, we, rd, rd_d0 : std_logic;
  signal fin, fout                    : std_logic_vector(c_fifo_width-1 downto 0);

  signal pre_dvalid : std_logic;
  signal pre_eof    : std_logic;
  signal pre_data   : std_logic_vector(15 downto 0);
  signal pre_addr   : std_logic_vector(1 downto 0);

  signal post_dvalid, post_eof, post_bytesel, post_sof : std_logic;

  signal err_status : t_wrf_status_reg;
  signal cyc_int    : std_logic;
  
begin  -- rtl

  err_status.error <= '1';

  dreq_o <= not full;

  rd <= not src_i.stall;
  we <= sof_i or eof_i or error_i or dvalid_i;

  pre_dvalid <= dvalid_i or error_i;
  pre_data   <= data_i when (error_i = '0') else f_marshall_wrf_status(err_status);
  pre_addr   <= addr_i when (error_i = '0') else c_WRF_STATUS;
  pre_eof    <= error_i or eof_i;

  fin <= sof_i & pre_eof & bytesel_i & pre_dvalid & pre_addr & pre_data;

  U_FIFO : generic_shiftreg_fifo
    generic map (
      g_data_width => c_fifo_width,
      g_size       => 16)
    port map (
      rst_n_i       => rst_n_i,
      clk_i         => clk_i,
      d_i           => fin,
      we_i          => we,
      q_o           => fout,
      rd_i          => rd,
      almost_full_o => full,
      q_valid_o     => q_valid);

  post_sof    <= fout(21);
  post_eof    <= fout(20);
  post_dvalid <= fout(18);

  p_gen_cyc : process(clk_i)
  begin
    if rising_edge(clk_i) then
      if rst_n_i = '0' then
        cyc_int <= '0';
      else
        if( q_valid = '1') then
          if(post_sof = '1')then
            cyc_int <= '1';
          elsif(post_eof = '1') then
            cyc_int <= '0';
          end if;
        end if;
      end if;
    end if;
  end process;

  src_o.cyc <= cyc_int;
  src_o.we  <= '1';
  src_o.stb <= post_dvalid and q_valid;
  src_o.sel <= '1' & not fout(19);
  src_o.dat <= fout(15 downto 0);
  src_o.adr <= fout(17 downto 16);
  
end rtl;



library ieee;
use ieee.std_logic_1164.all;

use work.wr_fabric_pkg.all;

entity wb_fabric_source is
  
  port (
    clk_i   : in std_logic;
    rst_n_i : in std_logic;

    -- Wishbone Fabric Interface I/O

    src_dat_o   : out std_logic_vector(15 downto 0);
    src_adr_o   : out std_logic_vector(1 downto 0);
    src_sel_o   : out std_logic_vector(1 downto 0);
    src_cyc_o   : out std_logic;
    src_stb_o   : out std_logic;
    src_we_o    : out std_logic;
    src_stall_i : in  std_logic;
    src_ack_i   : in  std_logic;
    src_err_i   : in  std_logic;

    -- Decoded & buffered fabric
    addr_i    : in  std_logic_vector(1 downto 0);
    data_i    : in  std_logic_vector(15 downto 0);
    dvalid_i  : in  std_logic;
    sof_i     : in  std_logic;
    eof_i     : in  std_logic;
    error_i   : in  std_logic;
    bytesel_i : in  std_logic;
    dreq_o    : out std_logic
    );

end wb_fabric_source;

architecture wrapper of wb_fabric_source is
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

  signal src_in  : t_wrf_source_in;
  signal src_out : t_wrf_source_out;
  
begin  -- wrapper

  
  U_Wrapped_Source : xwb_fabric_source
    port map (
      clk_i     => clk_i,
      rst_n_i   => rst_n_i,
      src_i     => src_in,
      src_o     => src_out,
      addr_i    => addr_i,
      data_i    => data_i,
      dvalid_i  => dvalid_i,
      sof_i     => sof_i,
      eof_i     => eof_i,
      error_i   => error_i,
      bytesel_i => bytesel_i,
      dreq_o    => dreq_o);

  src_cyc_o <= src_out.cyc;
  src_stb_o <= src_out.stb;
  src_we_o  <= src_out.we;
  src_sel_o <= src_out.sel;
  src_adr_o <= src_out.adr;
  src_dat_o <= src_out.dat;

  src_in.rty   <= '0';
  src_in.err   <= src_err_i;
  src_in.ack   <= src_ack_i;
  src_in.stall <= src_stall_i;

  
end wrapper;
