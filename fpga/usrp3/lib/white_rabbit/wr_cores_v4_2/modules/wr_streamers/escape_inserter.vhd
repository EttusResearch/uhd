-------------------------------------------------------------------------------
-- Title      : Escape insertion unit
-- Project    : WR Streamers
-- URL        : http://www.ohwr.org/projects/wr-cores/wiki/WR_Streamers
-------------------------------------------------------------------------------
-- File       : gc_escape_inserter.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2012-10-01
-- Platform   : FPGA-generic
-- Standard   : VHDL '93
------------------------------------------------------------------------------
-- Description: Unit for inserting escaped codes in a continuous data stream.
-- Allows for insertion of easily distinguishable control codes, such as start-
-- or end-of-frame markers. Given an input tuple (E[d_escape_i], D[d_i]), the
-- output (d_o) is:
-- - D when E == 0 and D != g_escape_code
-- - g_escape_code followed by 0 when E == 0 and D == g_escape_code
-- - g_escape_code followed by D when E == 1.
-- Note: When E == 1, D must not be 0.
------------------------------------------------------------------------------
--
-- Copyright (c) 2012 CERN
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

entity gc_escape_inserter is
  generic(
    -- data path width
    g_data_width  : integer;
    -- unique escape character (of g_data_width bits), must not be 0.
    g_escape_code : std_logic_vector
    );
  port(
    clk_i   : in std_logic;
    rst_n_i : in std_logic;

    -- data input (unescaped)
    d_i               : in  std_logic_vector(g_data_width-1 downto 0);

    -- when 1, escape insertion logic is enabled (i.e. if d_i == g_escape_code,
    -- it's translated to g_escape_code followed by 0 instead of just being passed
    -- through
    d_insert_enable_i : in  std_logic;

    -- when 1, d_i is treated as a escaped character
    d_escape_i        : in  std_logic;

    -- when 1, d_i and d_escape_i contain valid character
    d_valid_i         : in  std_logic;

    -- when 1, module can accept data in the following clock cycle.
    d_req_o           : out std_logic;

    -- data output
    d_o       : out std_logic_vector (g_data_width-1 downto 0);

    -- when 1, d_o contains a valid character
    d_valid_o : out std_logic;

    -- when 1, d_o/d_valid_o may output a character in the next clock cycle.
    d_req_i   : in  std_logic
    );

end gc_escape_inserter;

architecture behavioral of gc_escape_inserter is

  type t_state is (IDLE, INSERT_ESCAPE);

  signal d_prev     : std_logic_vector(g_data_width-1 downto 0);
  signal d_req_prev : std_logic;
  signal state      : t_state;
  signal match_esc_code : std_logic;
  
begin  -- behavioral

  match_esc_code <= '1' when d_i = g_escape_code else '0';

  -- stop the traffic if we need to insert an escaped sequence. This
  -- can happen when
  -- - the input character is an escape code (d_escape_i = '1')
  -- - the input character is not to be escaped, but it's equal to g_escape_code
  d_req_o <= d_req_i and not (d_valid_i and (d_escape_i or match_esc_code));

  d_o <= d_prev when (state = INSERT_ESCAPE) else
         d_i when d_escape_i = '0' else
         g_escape_code;
  
  d_valid_o <= d_valid_i when (state = IDLE) else
               d_req_prev;

  p_fsm : process(clk_i)
  begin
    if rising_edge(clk_i) then
      if rst_n_i = '0' or d_insert_enable_i = '0' then
        state      <= IDLE;
        d_req_prev <= '0';
      else
        d_req_prev <= d_req_i;
        case state is
          when IDLE =>
            -- case 1: escape the escape character sent as normal character
            if(d_i = g_escape_code and d_valid_i = '1' and d_escape_i = '0') then
              state  <= INSERT_ESCAPE;
              d_prev <= x"0000";
            -- case 2: send an escaped character
            elsif(d_escape_i = '1' and d_valid_i = '1') then
              state  <= INSERT_ESCAPE;
              d_prev <= d_i;
            end if;
            
          when INSERT_ESCAPE =>
            if(d_req_prev = '1') then
              state <= IDLE;
            end if;
        end case;
      end if;
    end if;
  end process;

end behavioral;
