-------------------------------------------------------------------------------
-- Title      : White Rabbit MAC/Endpoint 
-- Project    : White Rabbit Switch 
-------------------------------------------------------------------------------
-- File       : ep_rx_wb_master
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2009-06-22
-- Last update: 2017-02-02
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: RX Wishbone Master. Converts the internal fabric (DREQ-VALID
-- throttling) to Pipelined Wishbone (b4)
-------------------------------------------------------------------------------
--
-- Copyright (c) 2011-2017 CERN
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
-- Date        Version  Author  Description
-- 2011-08-22  0.1      twlostow  Created
------------------------------------------------------------------------------


library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.endpoint_private_pkg.all;
use work.endpoint_pkg.all;
use work.wr_fabric_pkg.all;

entity ep_rx_wb_master is
  generic(
    g_ignore_ack   : boolean := true;
    g_cyc_on_stall : boolean := false);
  port (
    clk_sys_i : in std_logic;
    rst_n_i   : in std_logic;

-- physical coding sublayer (PCS) interface
    snk_fab_i  : in  t_ep_internal_fabric;
    snk_dreq_o : out std_logic;

-- Wishbone I/O (master)
    src_wb_i : in  t_wrf_source_in;
    src_wb_o : out t_wrf_source_out
    );

end ep_rx_wb_master;

architecture behavioral of ep_rx_wb_master is

  type t_state is (IDLE, DATA, FLUSH_STALL, FINISH_CYCLE, THROW_ERROR);

  signal state       : t_state;
  signal ack_count   : unsigned(3 downto 0);
  signal src_out_int : t_wrf_source_out;

  signal tmp_sel : std_logic;
  signal tmp_dat : std_logic_vector(15 downto 0);
  signal tmp_adr : std_logic_vector(1 downto 0);
  signal enter_idle : std_logic;
  signal sof_reg  : std_logic;

begin  -- behavioral
  
  gen_cyc_on_stall: if g_cyc_on_stall = true generate
    snk_dreq_o <= '1' when ((src_wb_i.stall = '0' and state /= FINISH_CYCLE and
                  state /= THROW_ERROR and snk_fab_i.eof = '0' and snk_fab_i.error = '0' and snk_fab_i.sof = '0' and enter_idle = '0') or state = IDLE) else '0';
  end generate;

  gen_nocyc_on_stall: if g_cyc_on_stall = false generate
--     snk_dreq_o <= '1' when (src_wb_i.stall = '0' and state /= FINISH_CYCLE and snk_fab_i.eof = '0' and snk_fab_i.error = '0' and snk_fab_i.sof = '0' and enter_idle = '0') else '0';
--     snk_dreq_o <= '1' when (src_wb_i.stall = '0' and state /= FINISH_CYCLE and snk_fab_i.eof = '0' and snk_fab_i.error = '0' and enter_idle = '0') else '0';
    snk_dreq_o <= '1' when (src_wb_i.stall = '0' and state /= FINISH_CYCLE and
                  state /= THROW_ERROR and snk_fab_i.eof = '0' and snk_fab_i.error = '0') else '0';
  end generate;

  p_count_acks : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' or src_out_int.cyc = '0' then
        ack_count <= (others => '0');
      else
        if(src_out_int.stb = '1' and src_wb_i.stall = '0' and src_wb_i.ack = '0') then
          ack_count <= ack_count + 1;
        elsif(src_wb_i.ack = '1' and not(src_out_int.stb = '1' and src_wb_i.stall = '0')) then
          ack_count <= ack_count - 1;
        end if;
        
      end if;
    end if;
  end process;

  process(clk_sys_i)
    variable stat : t_wrf_status_reg := c_wrf_status_init_value;
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        state           <= IDLE;
        src_out_int.stb <= '0';
        src_out_int.we  <= '1';
        src_out_int.adr <= c_WRF_DATA;
        src_out_int.cyc <= '0';
        enter_idle <= '1';
        sof_reg    <= '0';
      else
        case state is
          when IDLE =>
            enter_idle <= '0';
            src_out_int.adr <= snk_fab_i.addr;
            src_out_int.dat <= snk_fab_i.data;

            if((snk_fab_i.sof='1' or sof_reg='1') and src_wb_i.err = '0') then
              src_out_int.cyc <= '1';
              state           <= DATA;
            end if;
            
          when DATA =>
            sof_reg <= '0';
            if(src_wb_i.stall = '0') then
              src_out_int.adr <= snk_fab_i.addr;
              src_out_int.dat    <= snk_fab_i.data;
              src_out_int.stb    <= snk_fab_i.dvalid;
              src_out_int.sel(1) <= '1';
              src_out_int.sel(0) <= not snk_fab_i.bytesel;
            end if;


            if(src_wb_i.err = '1') then
              state <= IDLE;
              enter_idle <= '1';
              src_out_int.cyc <= '0';
              src_out_int.stb <= '0';
            elsif(snk_fab_i.error = '1') then
              state <= THROW_ERROR;
            elsif(src_wb_i.stall = '1' and snk_fab_i.dvalid = '1') then
              state <= FLUSH_STALL;
            end if;

            if(snk_fab_i.eof = '1')then
              state <= FINISH_CYCLE;
            end if;

            
            
            tmp_adr <= snk_fab_i.addr;
            tmp_dat <= snk_fab_i.data;
            tmp_sel <= snk_fab_i.bytesel;

          when FLUSH_STALL =>
            if(src_wb_i.err = '1') then
              state <= IDLE;
              enter_idle <= '1';
              src_out_int.cyc <= '0';
              src_out_int.stb <= '0';
            elsif(src_wb_i.stall = '0') then
              src_out_int.dat    <= tmp_dat;
              src_out_int.adr    <= tmp_adr;
              src_out_int.stb    <= '1';
              src_out_int.sel(1) <= '1';
              src_out_int.sel(0) <= not tmp_sel;
              state              <= DATA;
            end if;

          when THROW_ERROR =>
            if(snk_fab_i.sof='1') then
              sof_reg <= '1';
            end if;
            if(src_wb_i.err = '1') then
              enter_idle <= '1';
              state <= IDLE;
              src_out_int.cyc <= '0';
              src_out_int.stb <= '0';
            elsif(src_wb_i.stall = '0') then
              stat.error      := '1';
              src_out_int.adr <= c_WRF_STATUS;
              src_out_int.dat <= f_marshall_wrf_status(stat);
              src_out_int.stb <= '1';
              state           <= FINISH_CYCLE;
            end if;
              
            
          when FINISH_CYCLE =>
            if(snk_fab_i.sof='1') then
              sof_reg <= '1';
            end if;
            if(src_wb_i.stall = '0') then
              src_out_int.stb <= '0';
            end if;

            if(((ack_count = 0) or g_ignore_ack) and src_out_int.stb = '0') then
              src_out_int.cyc <= '0';
              enter_idle <= '1';
              state           <= IDLE;
            end if;
          when others => null;
        end case;
      end if;
    end if;
  end process;


  src_wb_o <= src_out_int;
  
end behavioral;

