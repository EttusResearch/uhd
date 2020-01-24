-------------------------------------------------------------------------------
-- Title      : Controller of
-- Project    : White Rabbit MAC/Endpoint
-------------------------------------------------------------------------------
-- File       : ep_tx_inject_ctrl.vhd
-- Author     : Maciej Lipinski
-- Company    : CERN BE-CO-HT
-- Created    : 2014-01-16
-- Last update: 2014-01-16
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: this module enables to control HW packet injection in order
-- to turn the Endpoint into simple traffic generator for testing purposes
-------------------------------------------------------------------------------
--
-- Copyright (c) 2014 CERN / BE-CO-HT
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

library work;
use work.wr_fabric_pkg.all;
use work.endpoint_private_pkg.all;
use work.endpoint_pkg.all;
use work.ep_wbgen2_pkg.all;

entity ep_tx_inject_ctrl is

  generic(
    g_min_if_gap_length     : integer
    );

  port (
    clk_sys_i             : in  std_logic;
    rst_n_i               : in  std_logic;

    snk_fab_i             : in  t_ep_internal_fabric;
    snk_dreq_o            : out std_logic;

    src_fab_o             : out t_ep_internal_fabric;
    src_dreq_i            : in  std_logic;

    inject_req_o          : out std_logic;
    inject_ready_i        : in  std_logic;
    inject_packet_sel_o   : out std_logic_vector(2 downto 0);
    inject_user_value_o   : out std_logic_vector(15 downto 0);
    inject_ctr_ena_o      : out std_logic;
    inject_ctr_mode_o     : out std_logic_vector(1 downto 0);

    regs_i                : in  t_ep_out_registers;
    regs_o                : out t_ep_in_registers
    );
end ep_tx_inject_ctrl;

architecture rtl of ep_tx_inject_ctrl is

  -- there a lag between setting inject_req and SOF, this nees to be included
  constant if_gap_offset : unsigned(15 downto 0) := x"0000"; -- unused
  constant src_fab_null  : t_ep_internal_fabric := (
    sof                => '0',
    eof                => '0',
    error              => '0',
    dvalid             => '0',
    bytesel            => '0',
    has_rx_timestamp   => '0',
    rx_timestamp_valid => '0',
    data               => (others => '0'),
    addr               => (others => '0'));
  type t_state is (IDLE, INJECT_REQ, INJECT, IFG, END_GEN);
  
  -- Wishbone settings
  signal if_gap_value  : unsigned(15 downto 0);
  signal pck_sel       : std_logic_vector(2  downto 0);
  signal gen_ena       : std_logic;
  signal inj_mode      : std_logic_vector(1 downto 0);

  signal if_gap_cnt    : unsigned(15 downto 0);
  signal frame_id_cnt  : unsigned(15 downto 0);
  signal within_packet : std_logic;
  signal state         : t_state;

  -- translation betwen if_gap_value and real IFG:
  -- | ----------------------------------------- |
  -- | if_gap_value | gap in words | gap in time |
  -- |   0          |     7        |   112 ns    | disallowed 
  -- | ......................................... |
  -- |   5          |     12       |   192 ns    | minimal leagal
  -- |   6          |     13       |   208 ns    | 
  -- | ......................................... |
  -- |   65536      |     65546    |   1.048ms   | maximum allowed due to register size (16 bits)
  -- | ----------------------------------------- |
begin  -- rtl

  p_detect_within : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        within_packet <= '0';
      else
        if(snk_fab_i.sof = '1')then
          within_packet <= '1';
        elsif(snk_fab_i.eof = '1' or snk_fab_i.error = '1') then
          within_packet <= '0';
        end if;
      end if;
    end if;
  end process;

  p_config_reg: process(clk_sys_i)
  begin
   if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
          if_gap_value <= (others=>'0');
          pck_sel      <= (others=>'0');
          gen_ena      <= '0';
          inj_mode     <= (others=>'0');
      else
        if(regs_i.inj_ctrl_pic_ena_load_o = '1') then  -- writing the register
          if (regs_i.inj_ctrl_pic_conf_valid_o  = '1') then
            if_gap_value <= unsigned(regs_i.inj_ctrl_pic_conf_ifg_o);
            pck_sel      <= regs_i.inj_ctrl_pic_conf_sel_o;
          end if;
          if(regs_i.inj_ctrl_pic_mode_valid_o = '1') then
            inj_mode     <= regs_i.inj_ctrl_pic_mode_id_o(1 downto 0);
          end if;
          gen_ena        <= regs_i.inj_ctrl_pic_ena_o;
        end if;
      end if;
    end if;
  end process;  

  p_ctrl_fsm : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        state           <= IDLE;
        inject_req_o    <= '0';
        if_gap_cnt      <= (others => '0');
        frame_id_cnt    <= (others => '0');
      else
        case state is
          when IDLE =>
            
            --  start when 
            --  1) inject enabled, and 
            --  2) no packet being received from SWcore and 
            --  3) no packet being just started (otherwise, we could have two SOFs
            if(gen_ena = '1' and within_packet = '0' and snk_fab_i.sof = '0') then 
              inject_req_o   <= '1';                                
              if_gap_cnt     <= if_gap_offset;
              frame_id_cnt   <= (others => '0');
              state          <= INJECT_REQ;
            end if;
            
          when INJECT_REQ =>
            
            inject_req_o <= '0';
            state        <= INJECT;
            
          when INJECT =>
          
            if(inject_ready_i = '1') then
              frame_id_cnt <= frame_id_cnt + 1;
              state        <= IFG;
            end if;
          
          when IFG =>
            
            if(gen_ena = '0' ) then --gen disabled
              if_gap_cnt      <= (others => '0');
              frame_id_cnt    <= (others => '0');             
              if(within_packet = '0') then  -- if there is no frame being currently dumped
                state           <= IDLE;    -- go to idle state          
              else                          -- if there is a frame being dumped, 
                state           <= END_GEN; -- wait until it finishes
              end if;
            elsif(if_gap_cnt < if_gap_value) then
              if_gap_cnt <= if_gap_cnt + 1;
            else
              inject_req_o   <= '1';
              if_gap_cnt     <= if_gap_offset;
              state          <= INJECT_REQ;          
            end if;
            
          when END_GEN =>
          
            if(within_packet = '0') then -- now we can gracefully come back to normal functing
              state           <= IDLE; 
            end if;
          
          when others =>

            state           <= IDLE;
            if_gap_cnt      <= (others => '0');
            frame_id_cnt    <= (others => '0');              

        end case;
      end if;
    end if;
  end process;

  inject_user_value_o            <= std_logic_vector(frame_id_cnt);
  inject_packet_sel_o            <= pck_sel;
  inject_ctr_ena_o               <= gen_ena;
  inject_ctr_mode_o              <= inj_mode;
  snk_dreq_o                     <= src_dreq_i when (state = IDLE) else '1';         -- dev/null if gen
  src_fab_o                      <= snk_fab_i  when (state = IDLE) else src_fab_null;-- dev/null if gen
  regs_o.inj_ctrl_pic_conf_ifg_i <= std_logic_vector(if_gap_value);
  regs_o.inj_ctrl_pic_conf_sel_i <= pck_sel;
  regs_o.inj_ctrl_pic_conf_valid_i  <= '0';
  regs_o.inj_ctrl_pic_ena_i      <= gen_ena;
  regs_o.inj_ctrl_pic_mode_id_i  <= '0' & inj_mode;

end rtl;
