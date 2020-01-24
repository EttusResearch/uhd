------------------------------------------------------------------------------
-- Title      : Simple Wishbone UART - tranmitter
-- Project    : General Cores Collection (gencores) library
------------------------------------------------------------------------------
-- File       : uart_async_tx.vhd
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

entity uart_async_tx is
  
  port (
    clk_sys_i : in std_logic;
    rst_n_i   : in std_logic;

    baud_tick_i: in std_logic;
    
    txd_o        : out std_logic;
    tx_start_p_i : in  std_logic;
    tx_data_i    : in  std_logic_vector(7 downto 0);
    tx_busy_o    : out std_logic

    );

end uart_async_tx;

architecture behavioral of uart_async_tx is

signal BaudTick : std_logic;
  signal TxD_busy         : std_logic;
  signal TxD_ready        : std_logic;
  signal state            : std_logic_vector(3 downto 0);

  signal TxD_dataReg : std_logic_vector(7 downto 0);
  signal TxD_dataD   : std_logic_vector(7 downto 0);
  signal muxbit      : std_logic;
  signal TxD         : std_logic;

begin  -- behavioral




  TxD_ready <= '1' when state = "0000" else '0';
  TxD_busy  <= not TxD_ready;
  BaudTick  <= baud_tick_i;
  

  process(clk_sys_i, rst_n_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        TxD_dataReg <= (others => '0');
      elsif TxD_ready = '1' and tx_start_p_i = '1' then
        TxD_dataReg <= tx_data_i;
      end if;
    end if;
  end process;

  TxD_dataD <= TxD_dataReg;

  process(clk_sys_i, rst_n_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        state <= "0000";
      else
        case state is
          when "0000" =>
            if (tx_start_p_i = '1') then
              state <= "0001";
            end if;

          when "0001" =>
            if (BaudTick = '1') then
              state <= "0100";
            end if;
          when "0100" =>
            if (BaudTick = '1') then
              state <= "1000";
            end if;
          when "1000" =>
            if (BaudTick = '1') then
              state <= "1001";
            end if;
          when "1001" =>
            if (BaudTick = '1') then
              state <= "1010";
            end if;

          when "1010" =>
            if (BaudTick = '1') then
              state <= "1011";
            end if;
          when "1011" =>
            if (BaudTick = '1') then
              state <= "1100";
            end if;
          when "1100" =>
            if (BaudTick = '1') then
              state <= "1101";
            end if;
          when "1101" =>
            if (BaudTick = '1') then
              state <= "1110";
            end if;
          when "1110" =>
            if (BaudTick = '1') then
              state <= "1111";
            end if;

          when "1111" =>
            if (BaudTick = '1') then
              state <= "0010";
            end if;
          when "0010" =>
            if (BaudTick = '1') then
              state <= "0011";
            end if;

          when "0011" =>
            if (BaudTick = '1') then
              state <= "0000";
            end if;
          when others =>
            state <= "0000";
        end case;
      end if;
    end if;
  end process;


  process(TxD_dataD, state)
  begin
    case state(2 downto 0) is
      when "000"      => muxbit <= TxD_dataD(0);
      when "001"      => muxbit <= TxD_dataD(1);
      when "010"      => muxbit <= TxD_dataD(2);
      when "011"      => muxbit <= TxD_dataD(3);
      when "100"      => muxbit <= TxD_dataD(4);
      when "101"      => muxbit <= TxD_dataD(5);
      when "110"      => muxbit <= TxD_dataD(6);
      when "111"      => muxbit <= TxD_dataD(7);
      when others => null;
    end case;
  end process;

  process(clk_sys_i, rst_n_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        TxD <= '1';
      else
        if(unsigned(state) < to_unsigned(4, state'length) or (state(3) = '1' and muxbit = '1')) then
          TxD <= '1';
        else
          TxD <= '0';
        end if;
      end if;
    end if;
  end process;

  txd_o      <= TxD;
  tx_busy_o <= TxD_busy;

end behavioral;
