--
-- Copyright 2018 Ettus Research, A National Instruments Company
--
-- SPDX-License-Identifier: LGPL-3.0
--
-- Module: bitq_fsm
-- Description: Simple IP to shift bits in/out (primarily for JTAG)
-- bitq_fsm implements the state machine underlying the IP

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity bitq_fsm is
port (
  clk  : in std_logic;
  rstn : in std_logic;
  prescalar : in std_logic_vector(7 downto 0);

  bit_clk  : inout std_logic;
  bit_in   : in    std_logic;
  bit_out  : inout std_logic;
  bit_stb  : inout std_logic;
  start    : in std_logic;
  ready    : out std_logic;
  len      : in std_logic_vector(4 downto 0);
  wr_data  : in std_logic_vector(31 downto 0);
  stb_data : in std_logic_vector(31 downto 0);
  rd_data  : out std_logic_vector(31 downto 0)

);

end bitq_fsm;

architecture arch of bitq_fsm is
  type bitq_state_t is (IDLE, LOW, HIGH);
  signal bitq_state : bitq_state_t;

  signal bit_clk_count  : unsigned(7 downto 0);
  signal bit_count  : unsigned(5 downto 0);

  signal bit_out_r  : std_logic;
  signal bit_stb_r  : std_logic;

  signal rd_data_r  : std_logic_vector(31 downto 0);

begin
  rd_data <= rd_data_r;

  gen_io : process (bitq_state, bit_count, bit_out_r, bit_stb_r)
  begin
    case (bitq_state) is
    when IDLE =>
      bit_clk <= 'Z';
      bit_out <= 'Z';
      bit_stb <= 'Z';
      ready   <= '1';
    when LOW =>
      bit_clk <= '0';
      bit_out <= bit_out_r;
      bit_stb <= bit_stb_r;
      ready <= '0';
    when HIGH =>
      bit_clk <= '1';
      bit_out <= bit_out_r;
      bit_stb <= bit_stb_r;
      ready <= '0';
    when others =>
      bit_clk <= 'Z';
      bit_out <= 'Z';
      bit_stb <= 'Z';
      ready   <= '1';
    end case;
  end process;

  bit_clk_gen : process (clk)
  begin
  if rising_edge(clk) then
    if (rstn = '0') or (bitq_state = IDLE) or
       (bit_clk_count = 0) then
      bit_clk_count <= unsigned(prescalar);
    elsif (bit_clk_count /= 0) then
      bit_clk_count <= bit_clk_count - 1;
    end if;
  end if;
  end process bit_clk_gen;

  fsm : process (clk)
  begin
  if rising_edge(clk) then
    if (rstn = '0') then
      bitq_state <= IDLE;
      bit_count <= to_unsigned(0, bit_count'length);
      rd_data_r <= (others => '0');
    else
      case bitq_state is
      when IDLE =>
        bit_count <= to_unsigned(0, bit_count'length);

        if (start = '1') then
          bitq_state <= LOW;
          rd_data_r <= (others => '0');
          bit_out_r <= wr_data(0);
          bit_stb_r <= stb_data(0);
        end if;
      when LOW =>
        if (bit_clk_count = 0) then
          rd_data_r(to_integer(bit_count)) <= bit_in;
          bit_count <= bit_count + 1;
          bitq_state <= HIGH; --Rising edge
        end if;
      when HIGH =>
        if (bit_clk_count = 0) then
          if (bit_count > unsigned('0' & len)) then
            bitq_state <= IDLE;
          else
            bit_out_r <= wr_data(to_integer(bit_count));
            bit_stb_r <= stb_data(to_integer(bit_count));
            bitq_state <= LOW; --Falling edge
          end if;
        end if;
      end case;
    end if;
  end if;
  end process fsm;

end arch;

