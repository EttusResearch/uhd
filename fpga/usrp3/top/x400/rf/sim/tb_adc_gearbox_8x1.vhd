--
-- Copyright 2026 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: tb_adc_gearbox_8x1
--
-- Description:
--
--   Self-checking testbench for adc_gearbox_8x1.
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

entity tb_adc_gearbox_8x1 is
end tb_adc_gearbox_8x1;


architecture RTL of tb_adc_gearbox_8x1 is

  component adc_gearbox_8x1
    port (
      clk1x        : in  std_logic;
      reset_n_1x   : in  std_logic;
      adc_q_in_1x  : in  std_logic_vector(127 downto 0);
      adc_i_in_1x  : in  std_logic_vector(127 downto 0);
      valid_in_1x  : in  std_logic;
      enable_1x    : in  std_logic;
      clk8x        : in  std_logic;
      swap_iq_8x   : in  std_logic;
      adc_out_8x   : out std_logic_vector(31 downto 0);
      valid_out_8x : out std_logic);
  end component;

  signal adc_i_in_1x  : std_logic_vector(127 downto 0);
  signal adc_out_8x   : std_logic_vector(31 downto 0);
  signal adc_q_in_1x  : std_logic_vector(127 downto 0);
  signal enable_1x    : std_logic;
  signal reset_n_1x   : std_logic;
  signal swap_iq_8x   : std_logic;
  signal valid_in_1x  : std_logic;
  signal valid_out_8x : std_logic;

  signal StopSim : boolean;
  constant kPer : time := 10 ns;

  signal Clk   : std_logic := '1';
  signal Clk8x : std_logic := '1';

  constant iStartIndex : integer := 1;
  constant qStartIndex : integer := 1000;

  signal counter : integer := 0;

  procedure ClkWait(X : positive := 1) is
  begin
    for i in 1 to X loop
      wait until rising_edge(Clk);
    end loop;
  end procedure ClkWait;

begin

  Clk   <= not Clk after kPer/2 when not StopSim else '0';
  Clk8x <= not Clk8x after kPer/16 when not StopSim else '0';

  dut: adc_gearbox_8x1
    port map (
      clk1x        => Clk,
      reset_n_1x   => reset_n_1x,
      adc_q_in_1x  => adc_q_in_1x,
      adc_i_in_1x  => adc_i_in_1x,
      valid_in_1x  => valid_in_1x,
      enable_1x    => enable_1x,
      clk8x        => Clk8x,
      swap_iq_8x   => swap_iq_8x,
      adc_out_8x   => adc_out_8x,
      valid_out_8x => valid_out_8x
    );

  main: process
  begin
    swap_iq_8x  <= '0';
    valid_in_1x <= '0';
    enable_1x   <= '0';
    reset_n_1x  <= '0';
    ClkWait(5);
    reset_n_1x  <= '1';
    ClkWait(5);

    -- Ensure the outputs are quiet.
    ClkWait(20);
    assert valid_out_8x'stable(kPer*20) and valid_out_8x = '0'
      report "valid not stable at de-asserted at startup"
      severity error;
    assert adc_out_8x'stable(kPer*20) and (adc_out_8x = x"00000000")
      report "data not stable at zero at startup"
      severity error;

    -- Valid asserted, Enable asserted, Enable de-asserted, Valid de-asserted.

    ClkWait(10);
    valid_in_1x <= '1';
    ClkWait(10);
    enable_1x   <= '1';

    ClkWait(110);
    assert valid_out_8x'stable(kPer*100) and valid_out_8x = '1'
      report "valid not stable at asserted"
      severity error;

    ClkWait(10);
    enable_1x   <= '0';
    ClkWait(10);
    valid_in_1x <= '0';

    ClkWait(110);
    assert valid_out_8x'stable(kPer*100) and valid_out_8x = '0'
      report "valid not stable at de-asserted"
      severity error;

    -- Enable asserted, Valid asserted, Enable de-asserted, Valid de-asserted.

    ClkWait(10);
    enable_1x   <= '1';
    ClkWait(10);
    valid_in_1x <= '1';

    ClkWait(110);
    assert valid_out_8x'stable(kPer*100) and valid_out_8x = '1'
      report "valid not stable at asserted"
      severity error;

    ClkWait(10);
    enable_1x   <= '0';
    ClkWait(10);
    valid_in_1x <= '0';

    ClkWait(110);
    assert valid_out_8x'stable(kPer*100) and valid_out_8x = '0'
      report "valid not stable at de-asserted"
      severity error;

    StopSim <= true;
    wait;
  end process;


  counterProcess: process(Clk)
  begin
    if rising_edge(Clk) then
      if valid_in_1x = '1' and enable_1x = '1' then
        counter <= counter + 1;
      end if;
    end if;
  end process;

  driver: process(counter)
    variable tempQdata : integer := qStartIndex;
    variable tempIdata : integer := iStartIndex;
  begin
    for i in 0 to 7 loop
      adc_i_in_1x((i+1)*16-1 downto i*16) <= std_logic_vector(to_unsigned(iStartIndex + i + counter*8,16));
      adc_q_in_1x((i+1)*16-1 downto i*16) <= std_logic_vector(to_unsigned(qStartIndex + i + counter*8,16));
    end loop;
  end process;


  checker: process(Clk8x)
    variable expectedI : integer := iStartIndex;
    variable expectedQ : integer := qStartIndex;
    variable ExpectedData : std_logic_vector(31 downto 0) := (others => '0');
  begin
    if falling_edge(Clk8x) then
      if valid_out_8x = '1' then
        ExpectedData(15 downto 0)  := std_logic_vector(to_unsigned(expectedI,16));
        ExpectedData(31 downto 16) := std_logic_vector(to_unsigned(expectedQ,16));
        expectedI := expectedI + 1;
        expectedQ := expectedQ + 1;
        assert adc_out_8x = ExpectedData
          report "ADC data out mismatch from expected"
          severity error;
      end if;
    end if;
  end process;

end RTL;
