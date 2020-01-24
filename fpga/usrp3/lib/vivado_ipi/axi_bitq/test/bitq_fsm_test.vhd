--
-- Copyright 2018 Ettus Research, A National Instruments Company
--
-- SPDX-License-Identifier: LGPL-3.0
--
-- Module: bitq_fsm_test
-- Description: Manually-checked tester for bitq_fsm
--

library ieee;
use ieee.std_logic_1164.all;

library work;
use work.bitq_fsm;

entity bitq_fsm_test is
end bitq_fsm_test;

architecture sim of bitq_fsm_test is
  signal clk  : std_logic := '0';
  signal rstn : std_logic;

  signal wr_data   : std_logic_vector(31 downto 0);
  signal stb_data  : std_logic_vector(31 downto 0);
  signal rd_data   : std_logic_vector(31 downto 0);
  signal prescalar : std_logic_vector(7 downto 0);
  signal len       : std_logic_vector(4 downto 0);
  signal ready     : std_logic;
  signal start     : std_logic;

  signal bit_clk : std_logic;
  signal bit_in : std_logic;
  signal bit_out : std_logic;
  signal bit_stb : std_logic;

  constant HALFCYCLE : time := 5 ns;
  constant CYCLE : time := 2*HALFCYCLE;

begin

  process
  begin
    wait for HALFCYCLE;
    clk <= not clk;
  end process;

  process
  begin
    rstn <= '0';
    start <= '0';
    len <= "11111";
    bit_in <= '0';
    prescalar <= X"02";
    wait for CYCLE;
    rstn <= '1';
    wait for CYCLE;
    wr_data <= X"ABCDEF01";
    stb_data <= X"FF7F7700";
    wait for CYCLE;
    start <= '1';
    wait for CYCLE;
    start <= '0';
    wait until ready = '1';
    wait for CYCLE;
    start <= '1';
    wait for CYCLE;
    start <= '0';
    bit_in <= '1';
    wait until ready = '1';
    wait for CYCLE;
    bit_in <= '0';
    wait for CYCLE;
    report "End of Test";
  end process;

  dut : entity work.bitq_fsm
  port map (
    clk  => clk,
    rstn => rstn,
    prescalar => prescalar,
  
    bit_clk  => bit_clk,
    bit_in   => bit_in,
    bit_out  => bit_out,
    bit_stb  => bit_stb,
    start    => start,
    len      => len,
    ready    => ready,
    wr_data  => wr_data,
    stb_data => stb_data,
    rd_data  => rd_data
  );

end sim;

