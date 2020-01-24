-------------------------------------------------------------------------------
-- Title      : RX Status Register Inserter
-- Project    : White Rabbit 
-------------------------------------------------------------------------------
-- File       : ep_rx_status_reg_insert.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Platform   : FPGA-generic
-- Standard   : VHDL '93
-------------------------------------------------------------------------------
--
-- Copyright (c) 2011 - 2017 CERN / BE-CO-HT
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
use ieee.numeric_std.all;

use work.endpoint_private_pkg.all;
use work.endpoint_pkg.all;
use work.wr_fabric_pkg.all;

entity ep_rx_status_reg_insert is
  
  port (
    clk_sys_i : in std_logic;
    rst_n_i   : in std_logic;

    snk_fab_i  : in  t_ep_internal_fabric;
    snk_dreq_o : out std_logic;

    src_fab_o  : out t_ep_internal_fabric;
    src_dreq_i : in  std_logic;

    mbuf_valid_i    : in  std_logic;
    mbuf_ack_o      : out std_logic;
    mbuf_drop_i     : in  std_logic;
    mbuf_pclass_i   : in  std_logic_vector(7 downto 0);
    mbuf_is_hp_i    : in  std_logic;
    mbuf_is_pause_i : in  std_logic;

    rmon_pfilter_drop_o : out std_logic
    );

end ep_rx_status_reg_insert;

architecture rtl of ep_rx_status_reg_insert is

  type t_state is (WAIT_FRAME, WAIT_MBUF, GEN_STATUS);

  signal dreq_mask    : std_logic;
  signal embed_status : std_logic;
  signal sreg         : t_wrf_status_reg;
  signal state        : t_state;
  signal src_fab_out  : t_ep_internal_fabric;

  signal sof_mask : std_logic;
  
begin  -- rtl
  
  embed_status     <= '1'                         when (state = GEN_STATUS) else '0';
  src_fab_out.data <= f_marshall_wrf_status(sreg) when (embed_status = '1') else snk_fab_i.data;
  src_fab_out.addr <= c_WRF_STATUS                when (embed_status = '1') else snk_fab_i.addr;

  src_fab_out.eof     <= snk_fab_i.eof;
  src_fab_out.error   <= snk_fab_i.error;
  src_fab_out.bytesel <= snk_fab_i.bytesel;
  src_fab_out.dvalid  <= snk_fab_i.dvalid or (embed_status and src_dreq_i);

  src_fab_out.has_rx_timestamp   <= snk_fab_i.has_rx_timestamp;
  src_fab_out.rx_timestamp_valid <= snk_fab_i.rx_timestamp_valid;

  src_fab_o <= src_fab_out;

  src_fab_out.sof <= '1' when (mbuf_valid_i = '1' and state = WAIT_MBUF and mbuf_drop_i = '0' and mbuf_is_pause_i = '0') else '0';
  mbuf_ack_o      <= '1' when (mbuf_valid_i = '1' and state = WAIT_MBUF)                                                 else '0';

  snk_dreq_o <= src_dreq_i and dreq_mask and not snk_fab_i.sof;
--   snk_dreq_o <= src_dreq_i and not snk_fab_i.sof;

  p_gen_status : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        rmon_pfilter_drop_o <= '0';
        state     <= WAIT_FRAME;
        dreq_mask <= '1';
        sreg.match_class <= (others =>'0');
        sreg.is_hp       <= '0';
        sreg.has_crc     <= '0';
        sreg.has_smac    <= '0';
        sreg.error       <= '0';        
      else
        case state is
          when WAIT_FRAME =>
            rmon_pfilter_drop_o <= '0';
            if(snk_fab_i.sof = '1') then
              state     <= WAIT_MBUF;
              dreq_mask <= '0';
            end if;
            
          when WAIT_MBUF =>
            if(mbuf_valid_i = '1') then
                rmon_pfilter_drop_o <= mbuf_drop_i;

                if(mbuf_drop_i = '0' and mbuf_is_pause_i = '0') then
                  state <= GEN_STATUS;
                  dreq_mask <= '1';
                else
                  state <= WAIT_FRAME;
                  dreq_mask <= '1';
                end if;

                sreg.match_class <= mbuf_pclass_i;
                sreg.is_hp       <= mbuf_is_hp_i;
                sreg.has_crc     <= '0';
                sreg.has_smac    <= '1';
                sreg.error       <= '0';
            else
              rmon_pfilter_drop_o <= '0';
              --rmon_o.rx_path_timing_failure <= '0';
            end if;
            
          when GEN_STATUS =>
            rmon_pfilter_drop_o <= '0';
            if(src_dreq_i = '1') then
              state     <= WAIT_FRAME;
              dreq_mask <= '1';
            end if;
        end case;
      end if;
    end if;
  end process;


end rtl;
