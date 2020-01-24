-------------------------------------------------------------------------------
-- Title      : 802.3x 1000base-X compatible synchronization detect unit
-- Project    : WhiteRabbit Switch
-------------------------------------------------------------------------------
-- File       : ep_sync_detect.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-Co-HT
-- Created    : 2010-05-28
-- Last update: 2012-01-18
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

entity ep_sync_detect_16bit is
  
  port (
-- reset, synchronous to rbclk_i, active LOW
    rst_n_i  : in  std_logic;
-- recovered byte clock
    rbclk_i  : in  std_logic;
-- enable, active HI
    en_i     : in  std_logic;
-- decoded data input, active HI
    data_i   : in  std_logic_vector(15 downto 0);
-- decoded K signal, active HI
    k_i      : in  std_logic_vector(1 downto 0);
-- 8b10b coding error indication, active HI
    err_i    : in  std_logic;
-- sync detect output, active HI
    synced_o : out std_logic;

    cal_i : in std_logic
    );

end ep_sync_detect_16bit;

architecture behavioral of ep_sync_detect_16bit is

  type t_sync_fsm_state is (LOSS_OF_SYNC, CD_ACQ_1, CD_ACQ_2, CD_ACQ_3, SYNC_ACQUIRED_1, SYNC_ACQUIRED_2, SYNC_ACQUIRED_3, SYNC_ACQUIRED_4, SYNC_ACQUIRED_2A, SYNC_ACQUIRED_3A, SYNC_ACQUIRED_4A);

  
  function f_pick(sel : std_logic;
                  w1  : t_sync_fsm_state;
                  w0  : t_sync_fsm_state) return t_sync_fsm_state is

  begin
    if(sel = '1') then
      return w1;
    else
      return w0;
    end if;
  end f_pick;



  signal state    : t_sync_fsm_state;
  signal good_cgs : unsigned(2 downto 0);

  signal valid_idle   : std_logic;
  signal invalid_code : std_logic;
  signal valid_data   : std_logic;
  
begin  -- behavioral

  valid_idle   <= '1' when (k_i = "10" and data_i(15 downto 8) = c_k28_5 and err_i = '0')   else '0';
  valid_data   <= '1' when (k_i = "00" and err_i = '0')                                     else '0';
  invalid_code <= '1' when (err_i = '1' or (k_i(0) = '1' and data_i(7 downto 0) = c_k28_5)) else '0';


  sync_fsm : process (rbclk_i, rst_n_i)
  begin  -- process sync_fsm
    if rising_edge(rbclk_i) then
      if(rst_n_i = '0') then
        state    <= LOSS_OF_SYNC;
        synced_o <= '0';
        good_cgs <= (others => '0');
      else
        if(en_i = '0') then
          state    <= LOSS_OF_SYNC;
          synced_o <= '0';
          good_cgs <= (others => '0');
        else

          -- prevents from 
          if(cal_i = '0') then
            
            case state is
              when LOSS_OF_SYNC =>
                synced_o <= '0';
                state    <= f_pick(valid_idle, CD_ACQ_1, LOSS_OF_SYNC);
              when CD_ACQ_1 =>
                state <= f_pick(valid_idle or valid_data, CD_ACQ_2, LOSS_OF_SYNC);
              when CD_ACQ_2 =>
                state <= f_pick(valid_idle, CD_ACQ_3, LOSS_OF_SYNC);
              when CD_ACQ_3 =>
                state <= f_pick(valid_idle or valid_data, SYNC_ACQUIRED_1, LOSS_OF_SYNC);
                
              when SYNC_ACQUIRED_1 =>
                synced_o <= '1';
                state    <= f_pick(invalid_code, SYNC_ACQUIRED_2, SYNC_ACQUIRED_1);

              when SYNC_ACQUIRED_2 =>
                good_cgs <= (others => '0');
                state    <= f_pick(invalid_code, SYNC_ACQUIRED_3, SYNC_ACQUIRED_2A);

              when SYNC_ACQUIRED_2A =>
                good_cgs <= good_cgs + 1;

                if(good_cgs = "011" and invalid_code = '0') then
                  state <= SYNC_ACQUIRED_1;
                end if;

                if(invalid_code = '1') then
                  state <= SYNC_ACQUIRED_3;
                end if;

              when SYNC_ACQUIRED_3 =>
                good_cgs <= (others => '0');
                state    <= f_pick(invalid_code, SYNC_ACQUIRED_4, SYNC_ACQUIRED_3A);

                
              when SYNC_ACQUIRED_3A =>
                good_cgs <= good_cgs + 1;

                if(good_cgs = "011" and invalid_code = '0') then
                  state <= SYNC_ACQUIRED_2;
                end if;

                if(invalid_code = '1') then
                  state <= SYNC_ACQUIRED_4;
                end if;

              when SYNC_ACQUIRED_4 =>
                good_cgs <= (others => '0');
                state    <= f_pick(invalid_code, LOSS_OF_SYNC, SYNC_ACQUIRED_4A);
                
              when SYNC_ACQUIRED_4A =>
                good_cgs <= good_cgs + 1;

                if(good_cgs = "011" and invalid_code = '0') then
                  state <= SYNC_ACQUIRED_3;
                end if;

                if (invalid_code = '1') then
                  state <= LOSS_OF_SYNC;
                end if;
            end case;
          end if;
        end if;
      end if;
    end if;
  end process;




end behavioral;
