-------------------------------------------------------------------------------
-- Title      : 1000Base-X Autonegotiation 
-- Project    : White Rabbit MAC/Endpoint
-------------------------------------------------------------------------------
-- File       : ep_autonegotiation.vhd
-- Author     : Tomasz Włostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2010-11-18
-- Last update: 2017-02-21
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: Module implements the 1000Base-X autonegotiation engine as
-- defined in IEEE802.3.
-------------------------------------------------------------------------------
--
-- Copyright (c) 2009 CERN / BE-CO-HT
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
-- Revisions  :
-- Date        Version  Author    Description
-- 2010-11-18  0.4      twlostow  Initial release
-- 2011-02-07  0.5      twlostow  Tested on Spartan6 GTP
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.endpoint_private_pkg.all;

entity ep_autonegotiation is
  generic (
    g_simulation : boolean);
  port (
    clk_sys_i : in std_logic;
    rst_n_i   : in std_logic;

    -- RX PCS signals
    pcs_synced_i  : in  std_logic;
    pcs_los_i     : in  std_logic;
    pcs_link_ok_o : out std_logic;

    an_idle_match_i : in  std_logic;
    an_rx_en_o      : out std_logic;
    an_rx_val_i     : in  std_logic_vector(15 downto 0);
    an_rx_valid_i   : in  std_logic;

    -- TX PCS signals
    an_tx_en_o  : out std_logic;
    an_tx_val_o : out std_logic_vector(15 downto 0);

    mdio_mcr_anrestart_i    : in  std_logic;
    mdio_mcr_anenable_i     : in  std_logic;
    mdio_msr_anegcomplete_o : out std_logic;

    mdio_advertise_pause_i  : in std_logic_vector(1 downto 0);
    mdio_advertise_rfault_i : in std_logic_vector(1 downto 0);

    mdio_lpa_full_o   : out std_logic;
    mdio_lpa_half_o   : out std_logic;
    mdio_lpa_pause_o  : out std_logic_vector(1 downto 0);
    mdio_lpa_rfault_o : out std_logic_vector(1 downto 0);
    mdio_lpa_lpack_o  : out std_logic;
    mdio_lpa_npage_o  : out std_logic
    );

end ep_autonegotiation;

architecture syn of ep_autonegotiation is

  type t_autoneg_state is (AN_ENABLE, AN_RESTART, AN_ABILITY_DETECT, AN_DISABLE_LINK_OK, AN_ACKNOWLEDGE_DETECT, AN_COMPLETE_ACKNOWLEDGE, AN_IDLE_DETECT, AN_LINK_OK);

  signal state : t_autoneg_state;


  function f_eval_link_timer_size
    return integer is
  begin  -- f_eval_link_timer_size
    if(g_simulation = false) then
      return 20;
      else
        return 11;
      end if;
  end f_eval_link_timer_size;

  constant c_link_timer_bits : integer := f_eval_link_timer_size;
  
  signal link_timer : unsigned(c_link_timer_bits downto 0);  -- 23 for normal operation


  signal link_timer_restart : std_logic;
  signal link_timer_expired : std_logic;

  signal an_enable_changed, an_enable_d0 : std_logic;
  signal toggle_tx, toggle_rx            : std_logic;

  signal rx_config_reg : std_logic_vector(15 downto 0);

  signal acknowledge_match : std_logic;
  signal ability_match     : std_logic;
  signal consistency_match : std_logic;
  
  
  
begin  -- syn


  -- process: link timer (counts until MSB of link_timer == 1).
  -- inputs: link_timer_restart
  -- outputs: link_timer_expired
  p_link_timer : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' or link_timer_restart ='1'  then
        link_timer <= (others => '0');
      else
        if(link_timer_expired = '0') then
          link_timer <= link_timer + 1;
        end if;
      end if;
    end if;
  end process;

  link_timer_expired <= std_logic(link_timer(link_timer'high)) and (not link_timer_restart);


  -- process: detects the changes of Autonegotiation Enable bit.
  -- inputs: mdio_mcr_anenable_i
  -- outputs: an_enable_changed
  p_detect_enable_changed : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      an_enable_d0      <= mdio_mcr_anenable_i;
      an_enable_changed <= an_enable_d0 xor mdio_mcr_anenable_i;
    end if;
  end process;


  -- generate some FSM conditions combinatorially outside the FSM process to
  -- make the code more readable
  ability_match     <= an_rx_valid_i;
  acknowledge_match <= an_rx_valid_i and an_rx_val_i(14);
  consistency_match <= '1' when (an_rx_valid_i = '1' and rx_config_reg (15) = an_rx_val_i(15) and rx_config_reg(13 downto 0) = an_rx_val_i(13 downto 0));


-- process: main auto-negotiation state machine. More or less compatible with
-- IEEE 802.3.
  p_autonegotation_fsm : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        link_timer_restart <= '0';
        state              <= AN_ENABLE;
        pcs_link_ok_o      <= '0';

        an_tx_en_o  <= '0';
        an_rx_en_o  <= '1';
        an_tx_val_o <= (others => '0');

        mdio_lpa_lpack_o        <= '0';
        mdio_msr_anegcomplete_o <= '0';
        rx_config_reg           <= (others => '0');
      else

        if(mdio_mcr_anrestart_i = '1' or an_enable_changed = '1' or pcs_synced_i = '0') then
          state <= AN_ENABLE;
        else
          
          case state is
            when AN_ENABLE =>
              mdio_msr_anegcomplete_o <= '0';
              pcs_link_ok_o <= '0';

              if mdio_mcr_anenable_i = '1' then
                an_tx_val_o        <= (others => '0');  -- send breaklink
                an_tx_en_o         <= '1';
                link_timer_restart <= '1';
                state              <= AN_RESTART;
              else
                state <= AN_DISABLE_LINK_OK;
              end if;

            when AN_DISABLE_LINK_OK =>
              an_tx_en_o    <= '0';
              pcs_link_ok_o <= '1';
              
            when AN_RESTART =>
              link_timer_restart <= '0';
              if(link_timer_expired = '1') then
                state <= AN_ABILITY_DETECT;
              end if;


            when AN_ABILITY_DETECT =>
--              toggle_tx <= mr_adv_ability<12> -- ???
              an_rx_en_o                 <= '1';
              an_tx_en_o                 <= '1';
              an_tx_val_o (15)           <= '0';
              an_tx_val_o (14)           <= '0';
              an_tx_val_o (13 downto 12) <= mdio_advertise_rfault_i;
              an_tx_val_o (11 downto 9)  <= (others => '0');
              an_tx_val_o (8 downto 7)   <= mdio_advertise_pause_i;
              an_tx_val_o (6)            <= '0';  -- hdx not supported
              an_tx_val_o (5)            <= '1';  -- fdx supported
              an_tx_val_o (4 downto 0)   <= (others => '0');

              if(an_rx_valid_i = '1' and an_rx_val_i /= x"0000") then  -- ability_match=true
                state         <= AN_ACKNOWLEDGE_DETECT;
                rx_config_reg <= an_rx_val_i;
              end if;

            when AN_ACKNOWLEDGE_DETECT =>
              an_tx_val_o(14) <= '1';
              if(acknowledge_match = '1' and consistency_match = '0')
                or (ability_match = '1' and an_rx_val_i = x"0000") then
                state <= AN_ENABLE;
              elsif(acknowledge_match = '1' and consistency_match = '1') then
                state              <= AN_COMPLETE_ACKNOWLEDGE;
                link_timer_restart <= '1';
              end if;

            when AN_COMPLETE_ACKNOWLEDGE =>
              link_timer_restart <= '0';

              mdio_lpa_npage_o  <= an_rx_val_i(15);
              mdio_lpa_lpack_o  <= an_rx_val_i(14);
              mdio_lpa_rfault_o <= an_rx_val_i(13 downto 12);
              mdio_lpa_pause_o  <= an_rx_val_i (8 downto 7);
              mdio_lpa_half_o   <= an_rx_val_i(6);
              mdio_lpa_full_o   <= an_rx_val_i(5);


              if(ability_match = '1' and an_rx_val_i = x"0000")then
                state <= AN_ENABLE;
              elsif(link_timer_expired = '1' and (ability_match = '0' or an_rx_val_i /= x"0000")) then
                link_timer_restart <= '1';
                state              <= AN_IDLE_DETECT;
              end if;

            when AN_IDLE_DETECT =>
              link_timer_restart <= '0';
              an_tx_en_o         <= '0';
              if(ability_match = '1' and an_rx_val_i = x"0000") then
                state <= AN_ENABLE;
              elsif (an_idle_match_i = '1' and link_timer_expired = '1') then
                state <= AN_LINK_OK;
              end if;


            when AN_LINK_OK =>
              mdio_msr_anegcomplete_o <= '1';
              pcs_link_ok_o           <= '1';
              if(ability_match = '1') then
                state <= AN_ENABLE;
              end if;
              
            when others => null;
          end case;
        end if;
      end if;
    end if;
  end process;
end syn;
