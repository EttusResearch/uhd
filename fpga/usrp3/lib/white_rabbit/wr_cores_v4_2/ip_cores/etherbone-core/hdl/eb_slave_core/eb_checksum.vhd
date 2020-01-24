--! @file eb_checksum.vhd
--! @brief IP checksum generator for EtherBone
--!
--! Copyright (C) 2011-2012 GSI Helmholtz Centre for Heavy Ion Research GmbH 
--!
--! Important details about its implementation
--! should go in these comments.
--!
--! @author Mathias Kreider <m.kreider@gsi.de>
--!
--! @bug No know bugs.
--!
--------------------------------------------------------------------------------
--! This library is free software; you can redistribute it and/or
--! modify it under the terms of the GNU Lesser General Public
--! License as published by the Free Software Foundation; either
--! version 3 of the License, or (at your option) any later version.
--!
--! This library is distributed in the hope that it will be useful,
--! but WITHOUT ANY WARRANTY; without even the implied warranty of
--! MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
--! Lesser General Public License for more details.
--!  
--! You should have received a copy of the GNU Lesser General Public
--! License along with this library. If not, see <http://www.gnu.org/licenses/>.
---------------------------------------------------------------------------------

---! Standard library
library IEEE;
--! Standard packages    
use IEEE.std_logic_1164.all;
use IEEE.numeric_std.all;

--! Additional library
library work;
--! Additional packages    
use work.eb_hdr_pkg.all;

entity eb_checksum is 
  port(
    clk_i  : in  std_logic;
    nRst_i : in  std_logic;
    en_i   : in  std_logic; 
    data_i : in  std_logic_vector(15 downto 0);
    done_o : out std_logic;
    sum_o  : out std_logic_vector(15 downto 0));
end eb_checksum;

architecture behavioral of eb_checksum is
  constant c_width : natural := 28;
  type t_state is (S_IDLE, S_ADDUP);

  signal state : t_state := S_IDLE;
  signal sum   : unsigned(c_width-1 downto 0);

  function f_wrap(sum : unsigned) return unsigned is
    variable sum0, sum1 : unsigned(16 downto 0);
  begin
    sum0 := resize(sum(15 downto 0), 17) + resize(sum(c_width-1 downto 16), 17);
    sum1 := resize(sum(15 downto 0), 17) + resize(sum(c_width-1 downto 16), 17) + 1;
    if sum0(16) = '1' then
      return sum1(15 downto 0);
    else
      return sum0(15 downto 0);
    end if;
  end f_wrap;
  
begin

  sum_o <= std_logic_vector(sum(15 downto 0));
  
  adder: process(clk_i, nRst_i)
  begin
    if nRst_i = '0' then
      done_o <= '0';
      state  <= S_IDLE;
      sum    <= (others => '0');
    elsif rising_edge(clk_i) then
      case state is
        
        when S_IDLE =>
          done_o <= '0';
          if en_i = '1' then
            state <= S_ADDUP;
            sum <= resize(unsigned(data_i), c_width);
          end if;
        
        when S_ADDUP =>
          if en_i = '0' then
            sum <= resize(f_wrap(sum), c_width);
            done_o <= '1';
            state <= S_IDLE;
          else 
            sum <= sum + resize(unsigned(data_i), c_width);
          end if;
        
      end case;
    end if;
  end process;
  
end behavioral;
