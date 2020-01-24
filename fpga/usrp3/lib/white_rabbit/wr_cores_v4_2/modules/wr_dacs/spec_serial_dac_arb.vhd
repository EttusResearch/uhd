-------------------------------------------------------------------------------
-- Title      : SPEC serial DAC interface with arbiter
-- Project    : White Rabbit
-------------------------------------------------------------------------------
-- File       : spec_serial_dac.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-Co-HT
-- Platform   : fpga-generic
-- Standard   : VHDL'87
-------------------------------------------------------------------------------
--
-- Copyright (c) 2011 CERN
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


entity spec_serial_dac_arb is
  generic(
    g_invert_sclk    : boolean;
    g_num_extra_bits : integer
    );
  port(
    clk_i   : in std_logic;
    rst_n_i : in std_logic;

    val1_i  : in std_logic_vector(15 downto 0);
    load1_i : in std_logic;
    val2_i  : in std_logic_vector(15 downto 0);
    load2_i : in std_logic;

    dac_cs_n_o  : out std_logic_vector(1 downto 0);
    dac_clr_n_o : out std_logic;
    dac_sclk_o  : out std_logic;
    dac_din_o   : out std_logic);

end spec_serial_dac_arb;

architecture behavioral of spec_serial_dac_arb is

  component spec_serial_dac
    generic (
      g_num_data_bits  : integer;
      g_num_extra_bits : integer;
      g_num_cs_select  : integer);
    port (
      clk_i         : in  std_logic;
      rst_n_i       : in  std_logic;
      value_i       : in  std_logic_vector(g_num_data_bits-1 downto 0);
      cs_sel_i      : in  std_logic_vector(g_num_cs_select-1 downto 0);
      load_i        : in  std_logic;
      sclk_divsel_i : in  std_logic_vector(2 downto 0);
      dac_cs_n_o    : out std_logic_vector(g_num_cs_select-1 downto 0);
      dac_sclk_o    : out std_logic;
      dac_sdata_o   : out std_logic;
      xdone_o       : out std_logic);
  end component;

  signal d1, d2             : std_logic_vector(15 downto 0);
  signal d1_ready, d2_ready : std_logic;


  signal dac_data     : std_logic_vector(15 downto 0);
  signal dac_load     : std_logic;
  signal dac_cs_sel   : std_logic_vector(1 downto 0);
  signal dac_done     : std_logic;
  signal dac_sclk_int : std_logic;

  type t_state is (WAIT_DONE, LOAD_DAC, WAIT_DATA);

  signal state : t_state;

  signal trig0    : std_logic_vector(31 downto 0);
  signal trig1    : std_logic_vector(31 downto 0);
  signal trig2    : std_logic_vector(31 downto 0);
  signal trig3    : std_logic_vector(31 downto 0);
  signal CONTROL0 : std_logic_vector(35 downto 0);

begin  -- behavioral

  dac_clr_n_o <= '1';

  U_DAC : spec_serial_dac
    generic map (
      g_num_data_bits  => 16,
      g_num_extra_bits => g_num_extra_bits,
      g_num_cs_select  => 2)
    port map (
      clk_i         => clk_i,
      rst_n_i       => rst_n_i,
      value_i       => dac_data,
      cs_sel_i      => dac_cs_sel,
      load_i        => dac_load,
      sclk_divsel_i => "001",
      dac_cs_n_o    => dac_cs_n_o,
      dac_sclk_o    => dac_sclk_int,
      dac_sdata_o   => dac_din_o,
      xdone_o       => dac_done);


  p_drive_sclk: process(dac_sclk_int)
    begin
      if(g_invert_sclk) then
        dac_sclk_o <= not dac_sclk_int;
       else
        dac_sclk_o <= dac_sclk_int;
       end if;
      end process;

  process(clk_i)
  begin
    if rising_edge(clk_i) then
      if rst_n_i = '0' then
        d1         <= (others => '0');
        d1_ready   <= '0';
        d2         <= (others => '0');
        d2_ready   <= '0';
        dac_load   <= '0';
        dac_cs_sel <= (others => '0');
        state      <= WAIT_DATA;
      else

        if(load1_i = '1' or load2_i = '1') then
          
          if(load1_i = '1') then
            d1_ready <= '1';
            d1       <= val1_i;
          end if;

          if(load2_i = '1') then
            d2_ready <= '1';
            d2       <= val2_i;
          end if;
        else
          case state is
            when WAIT_DATA =>
              if(d1_ready = '1') then
                dac_cs_sel <= "01";
                dac_data   <= d1;
                dac_load   <= '1';
                d1_ready   <= '0';
                state      <= LOAD_DAC;
              elsif(d2_ready = '1') then
                dac_cs_sel <= "10";
                dac_data   <= d2;
                dac_load   <= '1';
                d2_ready   <= '0';
                state      <= LOAD_DAC;
              end if;

            when LOAD_DAC=>
              dac_load <= '0';
              state    <= WAIT_DONE;

            when WAIT_DONE =>
              if(dac_done = '1') then
                state <= WAIT_DATA;
              end if;
            when others => null;
          end case;
        end if;
      end if;
    end if;
  end process;
  

  
  
end behavioral;
