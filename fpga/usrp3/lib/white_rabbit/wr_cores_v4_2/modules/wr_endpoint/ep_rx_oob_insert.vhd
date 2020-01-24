-------------------------------------------------------------------------------
-- Title      : RX OOB inserter
-- Project    : White Rabbit 
-------------------------------------------------------------------------------
-- File       : ep_rx_oob_insert.vhd
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

library work;
use work.gencores_pkg.all;              -- for gc_crc_gen
use work.endpoint_private_pkg.all;
use work.endpoint_pkg.all;
use work.ep_wbgen2_pkg.all;
use work.wr_fabric_pkg.all;


entity ep_rx_oob_insert is
  port(clk_sys_i : in std_logic;
       rst_n_i   : in std_logic;

       snk_fab_i  : in  t_ep_internal_fabric;
       snk_dreq_o : out std_logic;

       src_fab_o  : out t_ep_internal_fabric;
       src_dreq_i : in  std_logic;

       regs_i : in t_ep_out_registers
       );

end ep_rx_oob_insert;

architecture behavioral of ep_rx_oob_insert is

  type t_state is (WAIT_OOB, OOB);
  signal state : t_state;

  signal src_dreq_d0 : std_logic;

  --component chipscope_ila
  --  port (
  --    CONTROL : inout std_logic_vector(35 downto 0);
  --    CLK     : in    std_logic;
  --    TRIG0   : in    std_logic_vector(31 downto 0);
  --    TRIG1   : in    std_logic_vector(31 downto 0);
  --    TRIG2   : in    std_logic_vector(31 downto 0);
  --    TRIG3   : in    std_logic_vector(31 downto 0));
  --end component;

  --component chipscope_icon
  --  port (
  --    CONTROL0 : inout std_logic_vector (35 downto 0));
  --end component;
  
  --signal CONTROL : std_logic_vector(35 downto 0);
  --signal CLK     : std_logic;
  --signal TRIG0   : std_logic_vector(31 downto 0);
  --signal TRIG1   : std_logic_vector(31 downto 0);
  --signal TRIG2   : std_logic_vector(31 downto 0);
  --signal TRIG3   : std_logic_vector(31 downto 0);
  
begin
  --chipscope_ila_1 : chipscope_ila
  --  port map (
  --    CONTROL => CONTROL,
  --    CLK     => clk_sys_i,
  --    TRIG0   => TRIG0,
  --    TRIG1   => TRIG1,
  --    TRIG2   => TRIG2,
  --    TRIG3   => TRIG3);

  --chipscope_icon_1 : chipscope_icon
  --  port map (
  --    CONTROL0 => CONTROL);

  --TRIG0(15 downto 0) <= snk_fab_i.data;
  --trig0(16) <= snk_fab_i.sof;
  --trig0(17) <= snk_fab_i.eof;
  --trig0(18) <= snk_fab_i.error;
  --trig0(19) <= snk_fab_i.bytesel;
  --trig0(20) <= snk_fab_i.has_rx_timestamp;
  --trig0(21) <= snk_fab_i.dvalid;
  --trig0(22) <= '1' when state = WAIT_OOB else '0';
  --trig0(24 downto 23) <= snk_fab_i.addr;

  snk_dreq_o                   <= src_dreq_i;
  src_fab_o.sof                <= snk_fab_i.sof;
  src_fab_o.eof                <= snk_fab_i.eof;
  src_fab_o.ERROR              <= snk_fab_i.ERROR;
  src_fab_o.bytesel            <= snk_fab_i.bytesel;
  src_fab_o.has_rx_timestamp   <= snk_fab_i.has_rx_timestamp;
  src_fab_o.rx_timestamp_valid <= snk_fab_i.rx_timestamp_valid;
  
  p_comb_src : process (state, snk_fab_i, src_dreq_i, regs_i)
  begin

    if(snk_fab_i.has_rx_timestamp = '1')then
      src_fab_o.data   <= c_WRF_OOB_TYPE_RX & (not snk_fab_i.rx_timestamp_valid) & "000000" & regs_i.ecr_portid_o;
      src_fab_o.dvalid <= '1';
      src_fab_o.addr   <= c_WRF_OOB;
    else
      if(state = WAIT_OOB) then
        src_fab_o.addr <= c_WRF_DATA;
      else
        src_fab_o.addr <= c_WRF_OOB;
      end if;
      src_fab_o.data   <= snk_fab_i.data;
      src_fab_o.dvalid <= snk_fab_i.dvalid;
    end if;
  end process;

  p_fsm : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' or regs_i.ecr_rx_en_o = '0' then
        state <= WAIT_OOB;
      else

        if(snk_fab_i.error = '1' or snk_fab_i.sof = '1') then
          state <= WAIT_OOB;
        else

          case state is
            when WAIT_OOB =>
              if(snk_fab_i.has_rx_timestamp = '1') then
                state <= OOB;
              end if;

            when OOB =>
              if(snk_fab_i.eof = '1') then
                state <= WAIT_OOB;
              end if;
              
          end case;
        end if;
      end if;
    end if;
  end process;


end behavioral;




