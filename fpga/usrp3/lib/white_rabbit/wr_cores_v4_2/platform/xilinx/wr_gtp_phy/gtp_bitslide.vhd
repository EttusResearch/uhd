-------------------------------------------------------------------------------
-- Title      : Deterministic Xilinx GTP wrapper - bitslide state machine
-- Project    : White Rabbit Switch
-------------------------------------------------------------------------------
-- File       : gtp_bitslide.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2010-11-18
-- Last update: 2013-12-20
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: Module implements a manual bitslide alignment state machine and
-- provides the obtained bitslide value to the MAC.
-------------------------------------------------------------------------------
--
-- Original EASE design (c) 2010 NIKHEF / Peter Jansweijer and Henk Peek
-- VHDL port (c) 2010 CERN
--
-- This source file is free software; you can redistribute it
-- and/or modify it under the terms of the GNU Lesser General
-- Public License as published by the Free Software Foundation;
-- either version 2.1 of the License, or (at your option) any
-- later version
--
-- This source is distributed in the hope that it will be
-- useful, but WITHOUT ANY WARRANTY; without even the implied
-- warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
-- PURPOSE.  See the GNU Lesser General Public License for more
-- details
--
-- You should have received a copy of the GNU Lesser General
-- Public License along with this source; if not, download it
-- from http://www.gnu.org/licenses/lgpl-2.1.html
--
-------------------------------------------------------------------------------
-- Revisions  :
-- Date        Version  Author    Description
-- 2010-11-18  0.4      twlostow  Ported EASE design to VHDL 
-- 2011-02-07  0.5      twlostow  Verified on Spartan6 GTP
-- 2011-09-12  0.6      twlostow  Virtex6 port
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity gtp_bitslide is
  
  generic (
-- set to non-zero value to enable some simulation speedups (reduce delays)
    g_simulation : integer;
    g_target     : string := "spartan6");

  port (
    gtp_rst_i : in std_logic;

-- GTP
    gtp_rx_clk_i : in std_logic;

-- '1' indicates that the GTP has detected a comma in the incoming serial stream
    gtp_rx_comma_det_i : in std_logic;


    gtp_rx_byte_is_aligned_i : in std_logic;

-- GTP ready flag (PLL locked and RX signal present)
    serdes_ready_i : in std_logic;

-- GTP manual bitslip control line
    gtp_rx_slide_o : out std_logic;

-- GTP CDR reset, asserted when the link is lost to set the bitslide to a known
-- value
    gtp_rx_cdr_rst_o : out std_logic;

-- Current bitslide, in UIs
    bitslide_o : out std_logic_vector(4 downto 0);

-- '1' when the bitsliding has been completed and the link is up
    synced_o : out std_logic
    );

end gtp_bitslide;


architecture behavioral of gtp_bitslide is



  function f_eval_sync_detect_threshold
    return integer is
  begin
    if(g_simulation /= 0) then
      return 256;
    else
      return 4000000;
    end if;
  end f_eval_sync_detect_threshold;

  function f_eval_pause_tics return integer is
  begin
    if(g_target = "spartan6") then
      return 31;
    else
      return 63;
    end if;
  end f_eval_pause_tics;

  function f_max_bts return integer is
  begin
    if(g_target = "spartan6") then
      return 10;
    else
      return 20;
    end if;
  end f_max_bts;

  constant c_pause_tics            : integer := f_eval_pause_tics;
  constant c_sync_detect_threshold : integer := f_eval_sync_detect_threshold;
  constant c_max_bts               : integer := f_max_bts;


  type t_bitslide_fsm_state is (S_SYNC_LOST, S_STABILIZE, S_SLIDE, S_PAUSE, S_GOT_SYNC, S_RESET_CDR);
  signal cur_slide : unsigned(4 downto 0);
  signal state     : t_bitslide_fsm_state;
  signal counter   : unsigned(23 downto 0);

  signal commas_missed : unsigned(4 downto 0);
  
begin  -- behavioral

  p_do_slide : process(gtp_rx_clk_i, gtp_rst_i)
  begin
    if gtp_rst_i = '1' then
      state            <= S_SYNC_LOST;
      gtp_rx_slide_o   <= '0';
      counter          <= (others => '0');
      synced_o         <= '0';
      gtp_rx_cdr_rst_o <= '0';
    elsif rising_edge(gtp_rx_clk_i) then
      
      if(serdes_ready_i = '0') then
        state <= S_SYNC_LOST;
      end if;

      case state is

-- State: synchronization lost. Waits until a comma pattern is detected
        when S_SYNC_LOST =>
          cur_slide        <= (others => '0');
          counter          <= (others => '0');
          gtp_rx_slide_o   <= '0';
          synced_o         <= '0';
          gtp_rx_cdr_rst_o <= '0';
          commas_missed    <= (others => '0');

          if(gtp_rx_comma_det_i = '1') then
            state <= S_STABILIZE;
          end if;

-- State: stabilize: 
          
        when S_STABILIZE =>
          
          
          if(gtp_rx_comma_det_i = '1') then
            counter       <= counter + 1;
            commas_missed <= (others => '0');
          else

            commas_missed <= commas_missed + 1;
            if(commas_missed(3) = '1') then
              state <= S_SYNC_LOST;
            end if;
          end if;

          if(counter = to_unsigned(c_sync_detect_threshold, counter'length)) then
            counter <= (others => '0');
            state   <= S_PAUSE;
          end if;

          if(serdes_ready_i = '0') then
            state <= S_SYNC_LOST;
          end if;

        when S_SLIDE =>
          if (cur_slide < c_max_bts-1) then
            cur_slide <= cur_slide + 1;
          else
            cur_slide <= (others=>'0');
          end if;
          gtp_rx_slide_o <= '1';
          counter        <= (others => '0');

          state <= S_PAUSE;

          if(serdes_ready_i = '0') then
            state <= S_SYNC_LOST;
          end if;

        when S_PAUSE =>
          counter        <= counter + 1;
          gtp_rx_slide_o <= '0';

          if(counter = to_unsigned(c_pause_tics, counter'length)) then

            if(gtp_rx_byte_is_aligned_i = '0') then
              state <= S_SLIDE;
            else
              state <= S_GOT_SYNC;
            end if;
          end if;

        when S_GOT_SYNC =>
          gtp_rx_slide_o <= '0';
          bitslide_o     <= std_logic_vector(cur_slide(4 downto 0));
          synced_o       <= '1';
          if(gtp_rx_byte_is_aligned_i = '0' or serdes_ready_i = '0') then
            gtp_rx_cdr_rst_o <= '1';
            state            <= S_SYNC_LOST;
          end if;
        when others => null;
      end case;
    end if;
  end process;
  
  

end behavioral;
