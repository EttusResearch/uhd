--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: tb_dac_gearbox_4x2
--
-- Description:
--
-- Self-checking testbench used to test the gearbox that reduces a 4 SPC data
-- into a 2 SPC data.
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

entity tb_dac_gearbox_4x2 is
end tb_dac_gearbox_4x2;


architecture RTL of tb_dac_gearbox_4x2 is

  component dac_gearbox_4x2
    port (
      clk1x        : in  std_logic;
      reset_n_1x   : in  std_logic;
      data_in_1x   : in  std_logic_vector(127 downto 0);
      valid_in_1x  : in  std_logic;
      ready_out_1x : out std_logic;
      clk2x        : in  std_logic;
      data_out_2x  : out std_logic_vector(63 downto 0);
      valid_out_2x : out std_logic);
  end component;

  signal TestStart : boolean;

  signal data_in_1x   : std_logic_vector(127 downto 0);
  signal data_out_2x  : std_logic_vector(63 downto 0);
  signal ready_out_1x : std_logic;
  signal reset_n_1x   : std_logic;
  signal valid_in_1x  : std_logic;
  signal valid_out_2x : std_logic;

  signal StopSim : boolean;
  constant kPer : time := 10 ns;

  signal Clk: std_logic := '1';
  signal Clk2x: std_logic := '1';

  signal c2DataToCheck, c2DataToCheckDly0, c2DataToCheckDly1, c2DataToCheckDly2
         : std_logic_vector(63 downto 0) := (others => '0');

  procedure ClkWait(X : positive := 1) is
  begin
    for i in 1 to X loop
      wait until rising_edge(Clk);
    end loop;
  end procedure ClkWait;

begin

  Clk   <= not Clk   after kPer/2 when not StopSim else '0';
  Clk2x <= not Clk2x after kPer/4 when not StopSim else '0';

  dut: dac_gearbox_4x2
    port map (
      clk1x        => Clk,
      reset_n_1x   => reset_n_1x,
      data_in_1x   => data_in_1x,
      valid_in_1x  => valid_in_1x,
      ready_out_1x => ready_out_1x,
      clk2x        => Clk2x,
      data_out_2x  => data_out_2x,
      valid_out_2x => valid_out_2x
    );

  main: process
  begin
    reset_n_1x <= '0';
    TestStart  <= false;
    ClkWait(5);
    reset_n_1x <= '1';
    ClkWait(5);

    -- Ensure the outputs are quiet.
    ClkWait(20);
    assert valid_out_2x'stable(kPer*20) and valid_out_2x = '0'
      report "valid not stable at de-asserted at startup"
      severity error;
    assert data_out_2x'stable(kPer*20) and (data_out_2x = x"0000000000000000")
      report "data not stable at zero at startup"
      severity error;

    -- Valid asserted, Enable asserted, Enable de-asserted, Valid de-asserted.

    ClkWait(10);
    TestStart <= true;

    ClkWait(110);
    assert valid_out_2x'stable(kPer*100) and valid_out_2x = '1'
      report "valid not stable at asserted"
      severity error;

    TestStart <= false;
    ClkWait(10);
    StopSim <= true;
    wait;
  end process;

  -- Process to generate input data to DUT.
  driver: process(Clk)
    variable tempQdata : integer := 1;
    variable tempIdata : integer := 128;
  begin
    if rising_edge(Clk) then
      valid_in_1x <= '0';
      if TestStart then
        valid_in_1x <= '1';
        data_in_1x  <= std_logic_vector(to_unsigned(tempQdata+3,16))  & std_logic_vector(to_unsigned(tempIdata+3,16)) &
                       std_logic_vector(to_unsigned(tempQdata+2,16))  & std_logic_vector(to_unsigned(tempIdata+2,16)) &
                       std_logic_vector(to_unsigned(tempQdata+1,16))  & std_logic_vector(to_unsigned(tempIdata+1,16)) &
                       std_logic_vector(to_unsigned(tempQdata+0,16))  & std_logic_vector(to_unsigned(tempIdata+0,16));
        tempQdata := tempQdata+4;
        tempIdata := tempIdata+4;
      end if;
    end if;
  end process;

  -- Process to generate expected data out of the DUT.
  ExpectedData: process(Clk2x)
    variable qDataOut : unsigned(15 downto 0) := x"0001";
    variable iDataOut : unsigned(15 downto 0) := x"0080";
  begin
    if rising_edge(Clk2x) then
      if TestStart then
        c2DataToCheck <= std_logic_vector((qDataOut+1) & (iDataOut+1)  &
                                          (qDataOut+0) & (iDataOut+0));

        qDataOut := qDataOut+2;
        iDataOut := iDataOut+2;
      else
        qDataOut := x"0001";
        iDataOut := x"0080";
      end if;
      c2DataToCheckDly0 <= c2DataToCheck;
      c2DataToCheckDly1 <= c2DataToCheckDly0;
      c2DataToCheckDly2 <= c2DataToCheckDly1;
    end if;
  end process;

  -- Process to check DUT output data with expected data.
  checker: process(Clk2x)
  begin
    if falling_edge(Clk2x) then
      if valid_out_2x = '1' then
        assert data_out_2x = c2DataToCheckDly2
          report "DAC data out mismatch from expected"
          severity error;
      end if;
      assert ready_out_1x = '1'
        report "Ready for output is not asserted"
        severity error;

    end if;
  end process;

end RTL;
