--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: tb_adc_gearbox_8x4
--
-- Description:
--
--   Self-checking testbench for adc_gearbox_8x4.
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

entity tb_adc_gearbox_8x4 is
end tb_adc_gearbox_8x4;


architecture RTL of tb_adc_gearbox_8x4 is

  component adc_gearbox_8x4
    port (
      clk1x        : in  std_logic;
      reset_n_1x   : in  std_logic;
      adc_q_in_1x  : in  std_logic_vector(127 downto 0);
      adc_i_in_1x  : in  std_logic_vector(127 downto 0);
      valid_in_1x  : in  std_logic;
      enable_1x    : in  std_logic;
      clk2x        : in  std_logic;
      swap_iq_2x   : in  std_logic;
      adc_out_2x   : out std_logic_vector(127 downto 0);
      valid_out_2x : out std_logic);
  end component;

  signal cDataCheckNxtLo, cDataCheckLo: std_logic_vector(127 downto 0);
  signal cDataCheckNxtHi : std_logic_vector(127 downto 0);
  signal cDataCheckHi1, cDataCheckHi2:  std_logic_vector(127 downto 0);

  signal adc_i_in_1x  : std_logic_vector(127 downto 0);
  signal adc_out_2x   : std_logic_vector(127 downto 0);
  signal adc_q_in_1x  : std_logic_vector(127 downto 0);
  signal enable_1x    : std_logic;
  signal reset_n_1x   : std_logic;
  signal swap_iq_2x   : std_logic;
  signal valid_in_1x  : std_logic;
  signal valid_out_2x : std_logic;

  signal StopSim : boolean;
  constant kPer : time := 10 ns;

  signal Clk   : std_logic := '1';
  signal Clk2x : std_logic := '1';

  procedure ClkWait(X : positive := 1) is
  begin
    for i in 1 to X loop
      wait until rising_edge(Clk);
    end loop;
  end procedure ClkWait;

begin

  Clk   <= not Clk   after kPer/2 when not StopSim else '0';
  Clk2x <= not Clk2x after kPer/4 when not StopSim else '0';

  dut: adc_gearbox_8x4
    port map (
      clk1x        => Clk,
      reset_n_1x   => reset_n_1x,
      adc_q_in_1x  => adc_q_in_1x,
      adc_i_in_1x  => adc_i_in_1x,
      valid_in_1x  => valid_in_1x,
      enable_1x    => enable_1x,
      clk2x        => Clk2x,
      swap_iq_2x   => swap_iq_2x,
      adc_out_2x   => adc_out_2x,
      valid_out_2x => valid_out_2x
    );

  main: process
  begin
    swap_iq_2x  <= '0';
    valid_in_1x <= '0';
    enable_1x   <= '0';
    reset_n_1x  <= '0';
    ClkWait(5);
    reset_n_1x  <= '1';
    ClkWait(5);

    -- Ensure the outputs are quiet.
    ClkWait(20);
    assert valid_out_2x'stable(kPer*20) and valid_out_2x = '0'
      report "valid not stable at de-asserted at startup"
      severity error;
    assert adc_out_2x'stable(kPer*20) and (adc_out_2x = std_logic_vector(to_unsigned(0,128)))
      report "data not stable at zero at startup"
      severity error;

    -- Valid asserted, Enable asserted, Enable de-asserted, Valid de-asserted.

    ClkWait(10);
    valid_in_1x <= '1';
    ClkWait(10);
    enable_1x   <= '1';

    ClkWait(110);
    assert valid_out_2x'stable(kPer*100) and valid_out_2x = '1'
      report "valid not stable at asserted"
      severity error;

    ClkWait(10);
    enable_1x   <= '0';
    ClkWait(10);
    valid_in_1x <= '0';

    ClkWait(110);
    assert valid_out_2x'stable(kPer*100) and valid_out_2x = '0'
      report "valid not stable at de-asserted"
      severity error;

    -- Enable asserted, Valid asserted, Enable de-asserted, Valid de-asserted.

    ClkWait(10);
    enable_1x   <= '1';
    ClkWait(10);
    valid_in_1x <= '1';

    ClkWait(110);
    assert valid_out_2x'stable(kPer*100) and valid_out_2x = '1'
      report "valid not stable at asserted"
      severity error;

    ClkWait(10);
    enable_1x   <= '0';
    ClkWait(10);
    valid_in_1x <= '0';

    ClkWait(110);
    assert valid_out_2x'stable(kPer*100) and valid_out_2x = '0'
      report "valid not stable at de-asserted"
      severity error;

    StopSim <= true;
    wait;
  end process;

  -- Process to generate input data to DUT and expected output data.
  driver: process(Clk)
    variable tempQdata : integer := 1;
    variable tempIdata : integer := 128;
    variable qData8spc : std_logic_vector(127 downto 0);
    variable iData8spc : std_logic_vector(127 downto 0);
  begin
    if rising_edge(Clk) then
      qdata8Spc   := std_logic_vector(to_unsigned(tempQdata+7,16)) & std_logic_vector(to_unsigned(tempQdata+6,16)) &
                     std_logic_vector(to_unsigned(tempQdata+5,16)) & std_logic_vector(to_unsigned(tempQdata+4,16)) &
                     std_logic_vector(to_unsigned(tempQdata+3,16)) & std_logic_vector(to_unsigned(tempQdata+2,16)) &
                     std_logic_vector(to_unsigned(tempQdata+1,16)) & std_logic_vector(to_unsigned(tempQdata  ,16));
      adc_q_in_1x <= qData8Spc;

      iData8spc   := std_logic_vector(to_unsigned(tempIdata+7,16)) & std_logic_vector(to_unsigned(tempIdata+6,16)) &
                     std_logic_vector(to_unsigned(tempIdata+5,16)) & std_logic_vector(to_unsigned(tempIdata+4,16)) &
                     std_logic_vector(to_unsigned(tempIdata+3,16)) & std_logic_vector(to_unsigned(tempIdata+2,16)) &
                     std_logic_vector(to_unsigned(tempIdata+1,16)) & std_logic_vector(to_unsigned(tempIdata  ,16));
      adc_i_in_1x <= iData8Spc;


      cDataCheckNxtLo <= qData8spc( 63 downto  48) & iData8spc( 63 downto  48) &
                         qData8spc( 47 downto  32) & iData8spc( 47 downto  32) &
                         qData8spc( 31 downto  16) & iData8spc( 31 downto  16) &
                         qData8spc( 15 downto   0) & iData8spc( 15 downto   0);
      cDataCheckNxtHi <= qData8spc(127 downto 112) & iData8spc(127 downto 112) &
                         qData8spc(111 downto  96) & iData8spc(111 downto  96) &
                         qData8spc( 95 downto  80) & iData8spc( 95 downto  80) &
                         qData8spc( 79 downto  64) & iData8spc( 79 downto  64);
      tempQdata := tempQdata+8;
      tempIdata := tempIdata+8;
    end if;
  end process;

  -- Process to check DUT output with expected data.
  checker: process(Clk2x)
    variable tempout : integer := 1;
    variable ExpectedData : std_logic_vector(127 downto 0) := (others => '0');
  begin
    if falling_edge(Clk2x) then
      if Clk = '1' then
        ExpectedData := cDataCheckLo;
      else
        ExpectedData := cDataCheckHi2;
      end if;
      if valid_out_2x = '1' then
        assert adc_out_2x = ExpectedData
          report "ADC data out mismatch from expected"
          severity error;
        tempout := tempout +1;
      end if;
      cDataCheckLo  <= cDataCheckNxtLo;
      cDataCheckHi1 <= cDataCheckNxtHi;
      cDataCheckHi2 <= cDataCheckHi1;
    end if;
  end process;

end RTL;
