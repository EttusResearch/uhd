-------------------------------------------------------------------------------
-- Title      : Synchronizer chain
-- Project    : White Rabbit 
-------------------------------------------------------------------------------
-- File       : gc_sync_ffs.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-Co-HT
-- Created    : 2010-06-14
-- Last update: 2017-02-03
-- Platform   : FPGA-generic
-- Standard   : VHDL'87
-------------------------------------------------------------------------------
-- Description: Synchronizer chain and edge detector.
-------------------------------------------------------------------------------
--
-- Copyright (c) 2009 - 2017 CERN
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
-- 2010-06-14  1.0      twlostow        Created
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

entity gc_sync_ffs is
  generic(
    g_sync_edge : string := "positive"
    );
  port(
    clk_i    : in  std_logic;  -- clock from the destination clock domain
    rst_n_i  : in  std_logic;           -- reset
    data_i   : in  std_logic;           -- async input
    synced_o : out std_logic;           -- synchronized output
    npulse_o : out std_logic;  -- negative edge detect output (single-clock
    -- pulse)
    ppulse_o : out std_logic   -- positive edge detect output (single-clock
   -- pulse)
    );
end gc_sync_ffs;

-- make Altera Quartus quiet regarding unknown attributes:
-- altera message_off 10335

architecture behavioral of gc_sync_ffs is
  signal sync0, sync1, sync2 : std_logic;

  attribute shreg_extract : string;
  attribute shreg_extract of sync0  : signal is "no";
  attribute shreg_extract of sync1  : signal is "no";
  attribute shreg_extract of sync2  : signal is "no";

  attribute keep : string;
  attribute keep of sync0  : signal is "true";
  attribute keep of sync1  : signal is "true";

  -- synchronizer attribute for Vivado
  attribute ASYNC_REG : string;
  attribute ASYNC_REG of sync0 : signal is "true";
  attribute ASYNC_REG of sync1 : signal is "true";
  attribute ASYNC_REG of sync2 : signal is "true";

begin


  sync_posedge : if (g_sync_edge = "positive") generate
    process(clk_i, rst_n_i)
    begin
      if(rst_n_i = '0') then
        sync0    <= '0';
        sync1    <= '0';
        sync2    <= '0';
        synced_o <= '0';
        npulse_o <= '0';
        ppulse_o <= '0';
      elsif rising_edge(clk_i) then
        sync0    <= data_i;
        sync1    <= sync0;
        sync2    <= sync1;
        synced_o <= sync1;
        npulse_o <= sync2 and not sync1;
        ppulse_o <= not sync2 and sync1;
      end if;
    end process;
  end generate sync_posedge;

  sync_negedge : if(g_sync_edge = "negative") generate
    process(clk_i, rst_n_i)
    begin
      if(rst_n_i = '0') then
        sync0    <= '0';
        sync1    <= '0';
        sync2    <= '0';
        synced_o <= '0';
        npulse_o <= '0';
        ppulse_o <= '0';
      elsif falling_edge(clk_i) then
        sync0    <= data_i;
        sync1    <= sync0;
        sync2    <= sync1;
        synced_o <= sync1;
        npulse_o <= sync2 and not sync1;
        ppulse_o <= not sync2 and sync1;
      end if;
    end process;
  end generate sync_negedge;
  
end behavioral;
