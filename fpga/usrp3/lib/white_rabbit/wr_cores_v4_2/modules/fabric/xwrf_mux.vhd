-------------------------------------------------------------------------------
-- Title      : Simple Pipelined Wishbone MUX/DEMUX for WRPC
-- Project    : WhiteRabbit
-------------------------------------------------------------------------------
-- File       : xwrf_mux.vhd
-- Author     : Grzegorz Daniluk
-- Company    : CERN BE-CO-HT
-- Created    : 2011-08-11
-- Last update: 2017-02-03
-- Platform   : FPGA-generics
-- Standard   : VHDL
-------------------------------------------------------------------------------
-- Description:
-- This is the simple multiplexer/demultiplexer for WR Fabric interface 
-- (Pipelined Wishbone interface). It forwards ethernet frames between 
-- WR endpoint, Mini-NIC and external Fabric interface in both directions. 
-- In the direction 'from' WR endpoint it also decides whether the packet 
-- has to be forwarded to Mini-NIC (if it is the PTP message) or to the 
-- external interface (others).
-------------------------------------------------------------------------------
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
-- Revisions  :
-- Date        Version  Author          Description
-- 2011-08-11  1.0      greg.d          Created
-- 2012-10-16  2.0      greg.d          generic number of ports
-------------------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_misc.all;

use ieee.numeric_std.all;

library work;
use work.wr_fabric_pkg.all;
use work.genram_pkg.all;

entity xwrf_mux is
  generic(
    g_muxed_ports : integer := 2);
  port(
    clk_sys_i   : in  std_logic;
    rst_n_i     : in  std_logic;
    --ENDPOINT
    ep_src_o    : out t_wrf_source_out;
    ep_src_i    : in  t_wrf_source_in;
    ep_snk_o    : out t_wrf_sink_out;
    ep_snk_i    : in  t_wrf_sink_in;
    --Muxed ports
    mux_src_o   : out t_wrf_source_out_array(g_muxed_ports-1 downto 0);
    mux_src_i   : in  t_wrf_source_in_array(g_muxed_ports-1 downto 0);
    mux_snk_o   : out t_wrf_sink_out_array(g_muxed_ports-1 downto 0);
    mux_snk_i   : in  t_wrf_sink_in_array(g_muxed_ports-1 downto 0);
    --
    mux_class_i : in  t_wrf_mux_class(g_muxed_ports-1 downto 0)
    );
end xwrf_mux;

architecture behaviour of xwrf_mux is

  function f_hot_to_bin(x : std_logic_vector(g_muxed_ports-1 downto 0))
    return integer is
    variable rv : integer;
  begin
    rv := 0;
    -- if there are few ones set in _x_ then the least significant will be
    -- translated to bin
    for i in g_muxed_ports-1 downto 0 loop
      if x(i) = '1' then
        rv := i;
      end if;
    end loop;
    return rv;
  end function;

  function f_match_class(port_mask, pkt_mask : std_logic_vector(7 downto 0)) return std_logic is
    variable ret : std_logic;
  begin
    if((port_mask and pkt_mask) /= x"00") then
      return '1';
    else
      return '0';
    end if;
  end function;

  --==================================--
  --  Masters to Endpoint mux signals --
  --==================================--
  type   t_mux is (MUX_SEL, MUX_TRANSFER);
  signal mux        : t_mux;
  signal mux_cycs   : std_logic_vector(g_muxed_ports-1 downto 0);
  signal mux_rrobin : std_logic_vector(g_muxed_ports-1 downto 0);
  signal mux_select : std_logic_vector(g_muxed_ports-1 downto 0);

  --==================================--
  -- Endpoint to Slaves demux signals --
  --==================================--
  type   t_demux is (DMUX_WAIT, DMUX_STATUS, DMUX_PAYLOAD);
  signal demux            : t_demux;
  signal dmux_sel         : std_logic_vector(g_muxed_ports-1 downto 0);
  signal dmux_status_reg  : std_logic_vector(15 downto 0);
  signal dmux_select      : std_logic_vector(g_muxed_ports-1 downto 0);
  signal dmux_others      : std_logic_vector(g_muxed_ports-1 downto 0);
  signal dmux_sel_zero    : std_logic;
  signal dmux_snd_stat    : std_logic_vector(g_muxed_ports-1 downto 0);
  signal ep_stall_mask    : std_logic;
  signal ep_snk_out_stall : std_logic;

begin

  --=============================================--
  --                                             --
  --   Many Fabric Masters talking to ENDPOINT   --
  --                                             --
  --=============================================--
  GEN_MUX_CYCS_REG : for I in 0 to g_muxed_ports-1 generate
    mux_cycs(I) <= mux_snk_i(I).cyc;
  end generate;

  p_mux : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if (rst_n_i = '0') then
        mux_rrobin(0)                        <= '1';
        mux_rrobin(g_muxed_ports-1 downto 1) <= (others => '0');
        mux                                  <= MUX_SEL;
      else
        case mux is
          when MUX_SEL =>
            if (unsigned(mux_cycs and mux_rrobin) /= 0)then
              mux_select <= mux_cycs and mux_rrobin;
              mux        <= MUX_TRANSFER;
            else
              mux_select <= (others => '0');
              mux_rrobin <= mux_rrobin(0) & mux_rrobin(g_muxed_ports-1 downto 1);
            end if;

          when MUX_TRANSFER =>
            if(unsigned(mux_cycs and mux_select) = 0) then  --cycle end
              mux_rrobin <= mux_rrobin(0) & mux_rrobin(g_muxed_ports-1 downto 1);
              mux        <= MUX_SEL;
            end if;
        end case;

      end if;
    end if;
  end process;


  GEN_MUX_CONNS : for J in 0 to g_muxed_ports-1 generate
    mux_snk_o(J).ack <= ep_src_i.ack when(mux /= MUX_SEL and mux_select(J) = '1') else
                        '0';
    mux_snk_o(J).stall <= ep_src_i.stall when(mux /= MUX_SEL and mux_select(J) = '1') else
                          '1';
    mux_snk_o(J).err <= ep_src_i.err when(mux /= MUX_SEL and mux_select(J) = '1') else
                        '0';
    mux_snk_o(J).rty <= '0';
  end generate;

  ep_src_o.cyc <= mux_snk_i(f_hot_to_bin(mux_select)).cyc when(mux /= MUX_SEL) else
                  '0';
  ep_src_o.stb <= mux_snk_i(f_hot_to_bin(mux_select)).stb when(mux /= MUX_SEL) else
                  '0';
  ep_src_o.adr <= mux_snk_i(f_hot_to_bin(mux_select)).adr;
  ep_src_o.dat <= mux_snk_i(f_hot_to_bin(mux_select)).dat;
  ep_src_o.sel <= mux_snk_i(f_hot_to_bin(mux_select)).sel;
  ep_src_o.we  <= '1';


  --=============================================--
  --                                             --
  --    ENDPOINT talking to many Fabric Slaves   --
  --                                             --
  --=============================================--

  CLASS_MATCH : for I in 0 to g_muxed_ports-1 generate
    dmux_sel(I) <= f_match_class(mux_class_i(I), f_unmarshall_wrf_status(dmux_status_reg).match_class);
  end generate;

  DMUX_FSM : process(clk_sys_i)
    variable sel : integer range 0 to g_muxed_ports-1;
  begin
    if rising_edge(clk_sys_i) then
      if(rst_n_i = '0') then
        dmux_select     <= (others => '0');
        dmux_snd_stat   <= (others => '0');
        dmux_status_reg <= (others => '0');
        ep_stall_mask   <= '0';
        demux           <= DMUX_WAIT;
      else
        case demux is
                                        ---------------------------------------------------------------
                                        --State DMUX_WAIT: Wait for the WRF cycle to start and then
                                        --                 wait for the STATUS word
                                        ---------------------------------------------------------------
          when DMUX_WAIT =>
            dmux_select     <= (others => '0');
            dmux_snd_stat   <= (others => '0');
            dmux_status_reg <= (others => '0');
            ep_stall_mask   <= '0';
            if(ep_snk_i.cyc = '1' and ep_snk_i.stb = '1' and ep_snk_i.adr = c_WRF_STATUS) then
              ep_stall_mask   <= '1';
              dmux_status_reg <= ep_snk_i.dat;
              demux           <= DMUX_STATUS;
            end if;

                                        ---------------------------------------------------------------
                                        --State DMUX_STATUS: Send Status word to appropriate interface
                                        ---------------------------------------------------------------
          when DMUX_STATUS =>
            ep_stall_mask <= '1';

            if(to_integer(unsigned(dmux_sel)) = 0) then  --class not matched to anything, pass pkt to last port
              dmux_select(g_muxed_ports-1)   <= '1';
              dmux_snd_stat(g_muxed_ports-1) <= '1';
              sel                            := g_muxed_ports-1;
            else
              dmux_select   <= dmux_sel;
              dmux_snd_stat <= dmux_sel;
              sel           := f_hot_to_bin(dmux_sel);
            end if;
            if(mux_src_i(sel).stall = '0') then
              demux <= DMUX_PAYLOAD;
            end if;

                                        ---------------------------------------------------------------
                                        --State DMUX_PAYLOAD: Just wait here till the end of the
                                        --                    current transfer
                                        ---------------------------------------------------------------
          when DMUX_PAYLOAD =>
            dmux_snd_stat <= (others => '0');
            ep_stall_mask <= '0';

            if(ep_snk_i.cyc = '0') then
              demux <= DMUX_WAIT;
            end if;

          when others =>
            demux <= DMUX_WAIT;
        end case;
      end if;
    end if;
  end process;

  dmux_sel_zero <= '1' when(to_integer(unsigned(dmux_select)) = 0) else
                   '0';

  -- dmux_others signal says for given interface I if any other interface was
  -- also matched to packet class
  dmux_others(0) <= '0';
  GEN_DMUX_OTHERS : for I in 1 to g_muxed_ports-1 generate
    dmux_others(I) <= or_reduce(dmux_select(I-1 downto 0));
  end generate;


  GEN_DMUX_CONN : for I in 0 to g_muxed_ports-1 generate
    mux_src_o(I).cyc <= ep_snk_i.cyc when(dmux_select(I) = '1' and dmux_others(I) = '0') else
                        '0';
    mux_src_o(I).stb <= '1' when(dmux_snd_stat(I) = '1' and dmux_others(I) = '0') else
                        ep_snk_i.stb when(dmux_select(I) = '1' and dmux_others(I) = '0') else
                        '0';
    mux_src_o(I).adr <= c_WRF_STATUS when(dmux_snd_stat(I) = '1' and dmux_others(I) = '0') else
                        ep_snk_i.adr when(dmux_select(I) = '1' and dmux_others(I) = '0') else
                        (others => '0');
    mux_src_o(I).dat <= dmux_status_reg when(dmux_snd_stat(I) = '1' and dmux_others(I) = '0') else
                        ep_snk_i.dat when(dmux_select(I) = '1' and dmux_others(I) = '0') else
                        (others => '0');
    mux_src_o(I).sel <= (others => '1') when(dmux_snd_stat(I) = '1' and dmux_others(I) = '0') else
                        ep_snk_i.sel when(dmux_select(I) = '1' and dmux_others(I) = '0') else
                        (others => '1');
    mux_src_o(I).we <= '1';
  end generate;


  ep_snk_o.ack <= ep_snk_i.cyc and ep_snk_i.stb and not ep_snk_out_stall when(dmux_sel_zero = '1') else
                  mux_src_i(f_hot_to_bin(dmux_select)).ack;

  ep_snk_o.err <= '0' when(dmux_sel_zero = '1') else
                  mux_src_i(f_hot_to_bin(dmux_select)).err;

  ep_snk_out_stall <= '1' when(ep_stall_mask = '1') else
                      '0' when(dmux_sel_zero = '1') else
                      mux_src_i(f_hot_to_bin(dmux_select)).stall;

  ep_snk_o.stall <= ep_snk_out_stall;

  ep_snk_o.rty <= '0';

end behaviour;

