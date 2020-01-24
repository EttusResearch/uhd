-------------------------------------------------------------------------------
-- Title      : Parametrized synchronizer
-- Project    : General Cores
-------------------------------------------------------------------------------
-- File       : gc_sync_register.vhd
-- Company    : CERN
-- Platform   : FPGA-generics
-- Standard   : VHDL '93
-------------------------------------------------------------------------------
-- Copyright (c) 2014-2017 CERN
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
-------------------------------------------------------------------------------
--------------------------------------------------------------------------------
--  Modifications:
--      2016-08-24: by Jan Pospisil (j.pospisil@cern.ch)
--          * added ASYNC_REG attribute for better timing analysis/simulation
--            in Xilinx tools
--------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

entity gc_sync_register is
  generic (
    g_width : integer);
  port (
    clk_i     : in  std_logic;
    rst_n_a_i : in  std_logic;
    d_i       : in  std_logic_vector(g_width-1 downto 0);
    q_o       : out std_logic_vector(g_width-1 downto 0));

end gc_sync_register;

-- make Altera Quartus quiet regarding unknown attributes:
-- altera message_off 10335

architecture rtl of gc_sync_register is

  signal gc_sync_register_in : std_logic_vector(g_width-1 downto 0);
  signal sync0, sync1        : std_logic_vector(g_width-1 downto 0);

  attribute shreg_extract                        : string;
  attribute shreg_extract of gc_sync_register_in : signal is "no";
  attribute shreg_extract of sync0               : signal is "no";
  attribute shreg_extract of sync1               : signal is "no";

  attribute keep                        : string;
  attribute keep of gc_sync_register_in : signal is "true";
  attribute keep of sync0               : signal is "true";
  attribute keep of sync1               : signal is "true";
  
  attribute async_reg                        : string;
  attribute async_reg of gc_sync_register_in : signal is "true";
  attribute async_reg of sync0               : signal is "true";
  attribute async_reg of sync1               : signal is "true";

begin

  process(clk_i, rst_n_a_i)
  begin
    if(rst_n_a_i = '0') then
      sync1 <= (others => '0');
      sync0 <= (others => '0');
    elsif rising_edge(clk_i) then
      sync0 <= gc_sync_register_in;
      sync1 <= sync0;
    end if;
  end process;

  gc_sync_register_in <= d_i;
  q_o                 <= sync1;
  
end rtl;
