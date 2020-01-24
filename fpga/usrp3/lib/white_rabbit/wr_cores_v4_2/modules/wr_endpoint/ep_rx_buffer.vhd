-------------------------------------------------------------------------------
-- Title      : RX Packet Buffer
-- Project    : White Rabbit MAC/Endpoint
-------------------------------------------------------------------------------
-- File       : ep_rx_buffer.vhd
-- Author     : Tomasz Włostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2010-11-18
-- Last update: 2017-02-02
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: A simple RX packet buffer, optimized for 18-bit Block RAM-based
-- FIFOs.
-------------------------------------------------------------------------------
--
-- Copyright (c) 2011 CERN / BE-CO-HT
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
-- from http://www.gnu.org/licenses/lgpl-2.1l.html
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;

use work.genram_pkg.all;
use work.endpoint_private_pkg.all;
use work.endpoint_pkg.all;
use work.wr_fabric_pkg.all;
use work.ep_wbgen2_pkg.all;

entity ep_rx_buffer is
  generic (
    g_size    : integer := 1024;
    g_with_fc : boolean := false
    );
  port(
    clk_sys_i : in std_logic;
    rst_n_i   : in std_logic;

    snk_fab_i  : in  t_ep_internal_fabric;
    snk_dreq_o : out std_logic;
    src_fab_o  : out t_ep_internal_fabric;
    src_dreq_i : in  std_logic;

    level_o    : out std_logic_vector(7 downto 0);
    full_o     : out std_logic;
    drop_req_i : in  std_logic;
    dropped_o  : out std_logic;
    regs_i     : in  t_ep_out_registers
    );

end ep_rx_buffer;

architecture behavioral of ep_rx_buffer is

  constant c_drop_threshold    : integer := g_size - 3;
  constant c_release_threshold : integer := g_size * 7 / 8;

  type t_write_state is(WAIT_FRAME, DATA);

  procedure f_pack_rbuf_contents
    (
      signal st         : in  t_write_state;
      signal fab        : in  t_ep_internal_fabric;
      signal prev_addr  : in  std_logic_vector;
      signal dout       : out std_logic_vector;
      signal dout_valid : out std_logic) is
    variable valid_tmp : std_logic;
  begin
    if(fab.sof = '1' or fab.error = '1' or fab.eof = '1') then
      -- tag = 11
      dout(17)          <= '1';
      dout(16)          <= '1';
      dout(15)          <= fab.sof;
      dout(14)          <= fab.eof;
      dout(13)          <= fab.error;
      dout(12 downto 0) <= (others => '0');
      valid_tmp         := '1';
--      dout_valid        <= '1';
    elsif(fab.dvalid = '1') then

      if(prev_addr /= fab.addr) then
        dout(17 downto 16) <= "10";     -- reg-change
      else
        dout(17 downto 16) <= '0' & fab.bytesel;
      end if;

      dout(15 downto 0) <= fab.data;
      valid_tmp         := '1';
    else
      dout(17 downto 0) <= (others => '0');
      valid_tmp         := '0';
    end if;

    if(fab.sof = '1') then
      dout_valid <= valid_tmp;
    elsif(st = DATA) then
      dout_valid <= valid_tmp;
    else
      dout_valid <= '0';
    end if;
  end f_pack_rbuf_contents;

  procedure f_unpack_rbuf_contents
    (
      signal din       : in  std_logic_vector;
      signal cur_addr  : in  std_logic_vector;
      signal din_valid : in  std_logic;
      signal fab       : out t_ep_internal_fabric;
      early_eof        :     boolean := false) is
  begin

    fab.data <= din(15 downto 0);
    if(din_valid = '1') then

      if(din(17 downto 16) = "10") then  -- some fancy encoding is necessary here
        case cur_addr(1 downto 0) is
          when c_WRF_DATA =>
            fab.addr <= c_WRF_OOB after 1 ns;
          when c_WRF_STATUS =>
            fab.addr <= c_WRF_DATA after 1 ns;
          when others => fab.addr <= c_WRF_DATA after 1 ns;
        end case;

      else
        fab.addr <= cur_addr after 1 ns;
      end if;

      fab.dvalid  <= not din(17) or (din(17) and not din(16));
      fab.sof     <= din(15) and din(17) and din(16);
      fab.eof     <= din(14) and din(17) and din(16);
      fab.error   <= din(13) and din(17) and din(16);
      fab.bytesel <= not din(17) and din(16);

    else
      fab.bytesel <= '0';
      fab.addr    <= cur_addr after 1 ns;
      fab.dvalid  <= '0';
      fab.sof     <= '0';
      fab.eof     <= '0';
      fab.error   <= '0';
      fab.data    <= (others => '0');
    end if;
    fab.has_rx_timestamp   <= '0';
    fab.rx_timestamp_valid <= '0';
  end f_unpack_rbuf_contents;


  signal q_in, q_out             : std_logic_vector(17 downto 0);
  signal q_usedw                 : std_logic_vector(f_log2_size(g_size)-1 downto 0);
  signal q_empty                 : std_logic;
  signal q_reset                 : std_logic;
  signal q_rd                    : std_logic;
  signal q_drop                  : std_logic;
  signal q_in_valid, q_out_valid : std_logic;
  signal q_aempty, q_afull       : std_logic;


  signal state         : t_write_state;
  signal fab_to_encode : t_ep_internal_fabric;
  signal src_fab_int   : t_ep_internal_fabric;

  signal in_prev_addr : std_logic_vector(1 downto 0);
  signal out_cur_addr : std_logic_vector(1 downto 0);
  
begin

  full_o <= q_drop;
 
  p_fifo_write : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        q_drop       <= '0';
        state        <= WAIT_FRAME;
        in_prev_addr <= (others => '0');
        dropped_o    <= '0';
      else

        if(snk_fab_i.dvalid = '1') then
          in_prev_addr <= snk_fab_i.addr;
        end if;

        if(q_afull = '1') then
          q_drop <= '1';
        elsif(q_aempty = '1') then
          q_drop <= '0';
        end if;

        case state is
          when WAIT_FRAME =>
            in_prev_addr <= c_WRF_STATUS;

            if(snk_fab_i.sof = '1' and q_drop = '0' and drop_req_i = '0') then
              state <= DATA;
              dropped_o <= '0';
            elsif(snk_fab_i.sof = '1' and (q_drop = '1' or drop_req_i = '1')) then
              dropped_o <= '1';
            end if;

          when DATA =>
            if(q_drop = '1' or snk_fab_i.eof = '1' or snk_fab_i.error = '1') then
              state <= WAIT_FRAME;
            end if;
            
          when others => null;
        end case;
      end if;
    end if;
  end process;

  p_pack_rbuf : process(state, fab_to_encode, in_prev_addr, q_in, q_in_valid)
  begin
    f_pack_rbuf_contents(state, fab_to_encode, in_prev_addr, q_in, q_in_valid);
  end process;


  p_encode_fifo_in : process(drop_req_i, q_drop, snk_fab_i, state)
    variable fab_pre_encode : t_ep_internal_fabric;
    
  begin
    fab_pre_encode := snk_fab_i;

    if(fab_pre_encode.sof = '1' and (q_drop = '1' or drop_req_i = '1')) then
      fab_pre_encode.sof := '0';
    end if;

    if(state = DATA and q_drop = '1') then
      fab_pre_encode.dvalid := '0';
      fab_pre_encode.error  := '1';
    end if;

    fab_to_encode <= fab_pre_encode;
  end process;

  q_reset <= rst_n_i and regs_i.ecr_rx_en_o;

  BUF_FIFO : generic_sync_fifo
    generic map (
      g_data_width => 18,
      g_size       => g_size,
      g_with_almost_empty => true,
      g_with_almost_full  => true,
      g_almost_empty_threshold  => c_release_threshold,
      g_almost_full_threshold   => c_drop_threshold,
      g_with_count              => g_with_fc)
    port map (
      rst_n_i        => q_reset,
      clk_i          => clk_sys_i,
      d_i            => q_in,
      we_i           => q_in_valid,
      q_o            => q_out,
      rd_i           => q_rd,
      empty_o        => q_empty,
      full_o         => open,
      almost_empty_o => q_aempty,
      almost_full_o  => q_afull,
      count_o        => q_usedw);

  
  
  q_rd <= (not q_empty) and src_dreq_i;

  rd_valid_gen : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if(rst_n_i = '0') then
        q_out_valid  <= '0';
        out_cur_addr <= c_WRF_STATUS;
      else
        q_out_valid <= q_rd;

        if(src_fab_int.sof = '1' or src_fab_int.eof = '1' or src_fab_int.error = '1')then
          out_cur_addr <= c_WRF_STATUS;
        else
          out_cur_addr <= src_fab_int.addr;
        end if;
      end if;
    end if;
  end process;

  p_unpack : process(q_out, out_cur_addr, q_out_valid,src_fab_int)
  begin
    f_unpack_rbuf_contents(q_out, out_cur_addr, q_out_valid, src_fab_int);
  end process;

  src_fab_o  <= src_fab_int;
  snk_dreq_o <= '1';

  GEN_FC: if g_with_fc = true generate
    GEN_LEV_BIG : if f_log2_size(g_size)-1 > level_o'left generate
      level_o <= q_usedw(q_usedw'left downto q_usedw'left - 7);
    end generate;
    GEN_LEV_SML : if f_log2_size(g_size)-1 < level_o'left+1 generate
      level_o(q_usedw'left downto 0) <= q_usedw;
    end generate;
  end generate;

  GEN_NOFC: if g_with_fc = false generate
    level_o <= (others=>'X');
  end generate;

end behavioral;
