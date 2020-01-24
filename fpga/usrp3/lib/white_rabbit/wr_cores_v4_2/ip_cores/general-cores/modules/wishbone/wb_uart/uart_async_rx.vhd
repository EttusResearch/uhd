------------------------------------------------------------------------------
-- Title      : Simple Wishbone UART - receiver
-- Project    : General Cores Collection (gencores) library
------------------------------------------------------------------------------
-- File       : uart_async_rx.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
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
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity uart_async_rx is


  port (
    clk_sys_i : in std_logic;
    rst_n_i   : in std_logic;

    baud8_tick_i: in std_logic;

    rxd_i      : in  std_logic;
    rx_ready_o : out std_logic;
    rx_error_o : out std_logic;
    rx_data_o  : out std_logic_vector(7 downto 0)

    );

end uart_async_rx;


architecture behavioral of uart_async_rx is

  signal Baud8Tick : std_logic;
  
  signal RxD_sync_inv      : std_logic_vector(1 downto 0);
  signal RxD_cnt_inv       : unsigned(1 downto 0);
  signal RxD_bit_inv       : std_logic;

  signal state       : std_logic_vector(3 downto 0);
  signal bit_spacing : std_logic_vector(3 downto 0);
  signal next_bit    : std_logic;

  signal RxD_data       : std_logic_vector(7 downto 0);
  signal RxD_data_ready : std_logic;
  signal RxD_data_error : std_logic;

  

begin  -- behavioral

  Baud8Tick <= baud8_tick_i;
               

  process(clk_sys_i, rst_n_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        RxD_sync_inv <= (others => '0');
      else
        if(Baud8Tick = '1') then
          RxD_sync_inv <= RxD_sync_inv(0) & (not rxd_i);
        end if;
      end if;
    end if;
  end process;

  process(clk_sys_i, rst_n_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        RxD_bit_inv <= '0';
        RxD_cnt_inv <= (others => '0');
      else
        if(Baud8Tick = '1') then
          if(RxD_sync_inv(1) = '1' and RxD_cnt_inv /= "11") then
            RxD_cnt_inv <= RxD_cnt_inv + 1;
          elsif (RxD_sync_inv(1) = '0' and RxD_cnt_inv /= "00") then
            RxD_cnt_inv <= RxD_cnt_inv - 1;
          end if;

          if(RxD_cnt_inv = "00") then
            RxD_bit_inv <= '0';
          elsif(RxD_cnt_inv = "11") then
            RxD_bit_inv <= '1';
          end if;
        end if;
      end if;
    end if;
  end process;


  next_bit <= '1' when (bit_spacing = x"a") else '0';

  process(clk_sys_i, rst_n_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        bit_spacing <= (others => '0');
      else
        if(state = x"0") then
          bit_spacing <= "0000";
        elsif(Baud8Tick = '1') then
--          bit_spacing <= std_logic_vector(resize((unsigned(bit_spacing(2 downto 0)) + 1), 4))
          bit_spacing <= std_logic_vector(unsigned('0' & bit_spacing(2 downto 0)) + 1)
                         or (bit_spacing(3) & "000");
        end if;
      end if;
    end if;
  end process;

  process(clk_sys_i, rst_n_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        state <= (others => '0');
      else
        if(Baud8Tick = '1') then
          case state is
            when "0000" =>
              if(RxD_bit_inv = '1') then  -- start bit
                state <= "1000";
              end if;
            when "1000" =>
              if(next_bit = '1') then
                state <= "1001";          -- bit 0
              end if;
            when "1001" =>
              if(next_bit = '1') then
                state <= "1010";          -- bit 1
              end if;
            when "1010" =>
              if(next_bit = '1') then
                state <= "1011";          -- bit 2
              end if;
            when "1011" =>
              if(next_bit = '1') then
                state <= "1100";          -- bit 3
              end if;
            when "1100" =>
              if(next_bit = '1') then
                state <= "1101";          -- bit 4
              end if;
            when "1101" =>
              if(next_bit = '1') then
                state <= "1110";          -- bit 5
              end if;
            when "1110" =>
              if(next_bit = '1') then
                state <= "1111";          -- bit 6
              end if;
            when "1111" =>
              if(next_bit = '1') then
                state <= "0001";          -- bit 7
              end if;
            when "0001" =>
              if(next_bit = '1') then
                state <= "0000";          -- bit stop
              end if;
            when others => state <= "0000";
          end case;
        end if;
      end if;
    end if;
  end process;


  process(clk_sys_i, rst_n_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        RxD_data <= (others => '0');
      else
        if(Baud8Tick = '1' and next_bit = '1' and state(3) = '1') then
          RxD_data <= (not RxD_bit_inv) & RxD_data(7 downto 1);
        end if;
      end if;
    end if;
  end process;

  process(clk_sys_i, rst_n_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        RxD_data_error <= '0';
        RxD_data_ready <= '0';
      else
        if(Baud8Tick = '1' and next_bit = '1' and state = "0001" and RxD_bit_inv = '0') then
          RxD_data_ready <= '1';
        else
          RxD_data_ready <= '0';
        end if;

        if(Baud8Tick = '1' and next_bit = '1' and state = "0001" and RxD_bit_inv = '1') then
          RxD_data_error <= '1';
        else
          RxD_data_error <= '0';
        end if;
      end if;
    end if;
  end process;

  rx_data_o <= RxD_data;
  rx_ready_o  <= RxD_data_ready;
  rx_error_o <= RxD_data_error;
  

end behavioral;
