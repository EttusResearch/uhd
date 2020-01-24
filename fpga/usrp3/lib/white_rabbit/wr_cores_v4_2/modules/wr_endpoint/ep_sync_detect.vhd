-------------------------------------------------------------------------------
-- Title      : 802.3x 1000base-X compatible synchronization detect unit
-- Project    : WhiteRabbit Switch
-------------------------------------------------------------------------------
-- File       : ep_sync_detect.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-Co-HT
-- Created    : 2010-05-28
-- Last update: 2012-04-27
-- Platform   : FPGA-generics
-- Standard   : VHDL
-------------------------------------------------------------------------------
-- Description: Module implements a link synchronization detect state machine
-- compatible with 802.3x spec.
-------------------------------------------------------------------------------
--
-- Copyright (c) 2010 CERN
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
-- 2010-05-28  1.0      twlostow        Created
-------------------------------------------------------------------------------



library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.endpoint_private_pkg.all;

entity ep_sync_detect is
  
  port (
-- reset, synchronous to rbclk_i, active LOW
    rst_n_i  : in  std_logic;
-- recovered byte clock
    rbclk_i  : in  std_logic;
-- enable, active HI
    en_i     : in  std_logic;
-- decoded data input, active HI
    data_i   : in  std_logic_vector(7 downto 0);
-- decoded K signal, active HI
    k_i      : in  std_logic;
-- 8b10b coding error indication, active HI
    err_i    : in  std_logic;
-- sync detect output, active HI
    synced_o : out std_logic;
-- odd/even field indicator (HI = even field)
    even_o   : out std_logic;

    cal_i: in std_logic
    );

end ep_sync_detect;

architecture behavioral of ep_sync_detect is

  type t_sync_fsm_state is (LOSS_OF_SYNC, COMMA_DETECT_1, ACQUIRE_SYNC_1, COMMA_DETECT_2, ACQUIRE_SYNC_2, COMMA_DETECT_3, SYNC_ACQUIRED_1, SYNC_ACQUIRED_2, SYNC_ACQUIRED_3, SYNC_ACQUIRED_4, SYNC_ACQUIRED_2A, SYNC_ACQUIRED_3A, SYNC_ACQUIRED_4A);

  signal state      : t_sync_fsm_state;
  signal cggood     : std_logic;
  signal cgbad      : std_logic;
  signal comma      : std_logic;
  signal rx_even    : std_logic;
  signal is_d       : std_logic;
  signal is_k       : std_logic;
  signal is_k28_5   : std_logic;
  signal good_cgs   : unsigned(2 downto 0);
  
  
begin  -- behavioral


  is_k28_5 <= '1' when data_i = c_k28_5 else '0';
  is_d     <= (not k_i);

  comma <= is_k28_5;

--  cgbad <= err_i or (k_i and comma and rx_even);

  cgbad  <= err_i or (k_i and comma and (not rx_even));
  cggood <= not (err_i or (k_i and comma and (not rx_even)));

  even_o <= rx_even;

-- fixme!

  -- 1000base-x sync detect state machine
  -- as defined in 802.3-2008, figure 36-9.


  sync_fsm : process (rbclk_i, rst_n_i)
  begin  -- process sync_fsm
    if rising_edge(rbclk_i) then
      if(rst_n_i = '0') then
        state      <= LOSS_OF_SYNC;
        synced_o   <= '0';
        rx_even    <= '0';
        good_cgs   <= (others => '0');
      else
        if(en_i = '0') then
          state      <= LOSS_OF_SYNC;
          synced_o   <= '0';
          rx_even    <= '0';
          good_cgs   <= (others => '0');
        else

          -- prevents from 
          if(cal_i = '0') then  
            
            case state is
              when LOSS_OF_SYNC =>
                synced_o <= '0';
                if(comma = '1') then
                  rx_even <= '0';
                  state <= COMMA_DETECT_1;
                else
                  rx_even  <= not rx_even;
                end if;

              when COMMA_DETECT_1 =>
                rx_even <= not rx_even;
                if(is_d = '1') then     -- got data
                  state <= ACQUIRE_SYNC_1;
                else
                  state <= LOSS_OF_SYNC;
                end if;

              when ACQUIRE_SYNC_1 =>
                if(cgbad = '1') then
                  state <= LOSS_OF_SYNC;
                  rx_even <= not rx_even;
                elsif (rx_even = '1' and comma = '1') then
                  rx_even <= '0';         -- was 1
                  state <= COMMA_DETECT_2;
                else
                  rx_even <= not rx_even;
                end if;

              when COMMA_DETECT_2 =>
                rx_even <= not rx_even;
                if(is_d = '1') then
                  state <= ACQUIRE_SYNC_2;
                else
                  state <= LOSS_OF_SYNC;
                end if;

              when ACQUIRE_SYNC_2 =>
                if(cgbad = '1') then
                  rx_even <= not rx_even;
                  state <= LOSS_OF_SYNC;
                elsif (rx_even = '1' and comma = '1') then
                  state <= COMMA_DETECT_3;
                  rx_even <= '0';
                else
                  rx_even <= not rx_even;
                end if;

              when COMMA_DETECT_3 =>
                rx_even <= not rx_even;
                if(is_d = '1') then
                  state <= SYNC_ACQUIRED_1;
                else
                  state <= LOSS_OF_SYNC;
                end if;

              when SYNC_ACQUIRED_1 =>
                synced_o <= '1';
                rx_even  <= not rx_even;

                if(cggood = '1') then
                  state <= SYNC_ACQUIRED_1;
                end if;

                if (cgbad = '1') then
                  state <= SYNC_ACQUIRED_2;
                end if;

              when SYNC_ACQUIRED_2 =>
                rx_even  <= not rx_even;
                good_cgs <= (others => '0');

                if(cggood = '1') then
                  state <= SYNC_ACQUIRED_2A;
                end if;

                if(cgbad = '1') then
                  state <= SYNC_ACQUIRED_3;
                end if;

              when SYNC_ACQUIRED_2A =>
                rx_even  <= not rx_even;
                good_cgs <= good_cgs + 1;

                if(good_cgs = "011" and cggood = '1') then
                  state <= SYNC_ACQUIRED_1;
                end if;

                if(cgbad = '1') then
                  state <= SYNC_ACQUIRED_3;
                end if;

              when SYNC_ACQUIRED_3 =>
                rx_even  <= not rx_even;
                good_cgs <= (others => '0');

                if(cggood = '1') then
                  state <= SYNC_ACQUIRED_3A;
                end if;

                if(cgbad = '1') then
                  state <= SYNC_ACQUIRED_4;
                end if;
                
              when SYNC_ACQUIRED_3A =>
                rx_even  <= not rx_even;
                good_cgs <= good_cgs + 1;

                if(good_cgs = "011" and cggood = '1') then
                  state <= SYNC_ACQUIRED_2;
                end if;

                if(cgbad = '1') then
                  state <= SYNC_ACQUIRED_4;
                end if;

              when SYNC_ACQUIRED_4 =>
                rx_even  <= not rx_even;
                good_cgs <= (others => '0');

                if(cggood = '1') then
                  state <= SYNC_ACQUIRED_4A;
                end if;

                if(cgbad = '1') then
                  state <= LOSS_OF_SYNC;
                end if;

              when SYNC_ACQUIRED_4A =>
                rx_even  <= not rx_even;
                good_cgs <= good_cgs + 1;

                if(good_cgs = "011" and cggood = '1') then
                  state <= SYNC_ACQUIRED_3;
                end if;

                if (cgbad = '1') then
                  state <= LOSS_OF_SYNC;
                end if;
              when others => null;
            end case;
          end if;
        end if;
      end if;
    end if;
  end process;
  

  

end behavioral;
