--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: tb_adc_gearbox_2x4
--
-- Description:
--
--   Self-checking testbench for the gearbox that expands the data width from 2
--   SPC to 4 SPC.
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

entity tb_adc_gearbox_2x4 is
end tb_adc_gearbox_2x4;


architecture RTL of tb_adc_gearbox_2x4 is

  component adc_gearbox_2x4
    port (
      Clk1x          : in  std_logic;
      Clk3x          : in  std_logic;
      ac1Reset_n     : in  std_logic;
      ac3Reset_n     : in  std_logic;
      c3DataIn       : in  std_logic_vector(95 downto 0);
      c3DataValidIn  : in  std_logic;
      c1DataOut      : out std_logic_vector(191 downto 0);
      c1DataValidOut : out std_logic);
  end component;

  signal aTestReset : boolean;

  signal ac1Reset_n     : std_logic := '1';
  signal ac3Reset_n     : std_logic := '1';
  signal c3DataIn       : std_logic_vector( 95 downto 0) := (others => '0');
  signal c3DataValidIn  : std_logic := '0';
  signal c1ExpectedData : std_logic_vector(191 downto 0) := (others => '0');
  signal c1DataOut      : std_logic_vector(191 downto 0) := (others => '0');
  signal c1DataValidOut : std_logic;

  signal StopSim : boolean;
  constant kPer : time := 12 ns;

  signal Clk1x : std_logic := '1';
  signal Clk3x : std_logic := '1';

  procedure Clk3xWait(X : positive := 1) is
  begin
    for i in 1 to X loop
      wait until rising_edge(Clk3x);
    end loop;
  end procedure Clk3xWait;

  procedure Clk1xWait(X : positive := 1) is
  begin
    for i in 1 to X loop
      wait until rising_edge(Clk1x);
    end loop;
  end procedure Clk1xWait;

begin

  Clk1x <= not Clk1x after kPer/2 when not StopSim else '0';
  Clk3x <= not Clk3x after kPer/6 when not StopSim else '0';

  dut: adc_gearbox_2x4
    port map (
      Clk1x          => Clk1x,
      Clk3x          => Clk3x,
      ac1Reset_n     => ac1Reset_n,
      ac3Reset_n     => ac3Reset_n,
      c3DataIn       => c3DataIn,
      c3DataValidIn  => c3DataValidIn,
      c1DataOut      => c1DataOut,
      c1DataValidOut => c1DataValidOut
    );

  main: process
    procedure PhaseTest(WaitCycles : positive := 1) is
    begin
      -- Stop data generation by asserting this reset.
      aTestReset <= true;
      Clk1xWait;
      ac1Reset_n <= '0';
      ac3Reset_n <= '0';
      Clk1xWait;
      ac1Reset_n <= '1';
      ac3Reset_n <= '1';

      -- This wait is in Clk3x domain. This is used to change phase in which
      -- data valid is asserted with respect to Clk3x and Clk1x rising edge.
      -- Wait an additional 12 Clk3x cycles for the output data valid to be
      -- de-asserted.
      Clk3xWait(WaitCycles+12);

      -- De-asserting test reset will start data generation.
      aTestReset  <= false;

      -- Wait for a random time before we stop the test.
      Clk3xWait(1000);
    end procedure;

  begin
    -- Change phase between Clk1x and Clk3x. See details in the DUT.
    -- The wait in each phase test is used to move the de-assertion of data
    -- generation logic reset. By doing this, we can change data valid
    -- assertion phase between Clk3x and Clk1x.
    -- p0.
    PhaseTest(1);

    -- p1
    PhaseTest(2);

    -- p2.
    PhaseTest(6);

    -- Stop simulation
    StopSim <= true;
    wait;
  end process;

  -- Process to generate data to the DUT.
  driver: process(Clk3x, aTestReset)
    variable tempQdata : integer := 1;
    variable tempIdata : integer := 128;
    variable dataCount : integer := 0;
  begin
    if aTestReset then
      tempQdata := 1;
      tempIdata := 128;
      dataCount := 0;
      c3DataIn  <= (others => '0');
      c3DataValidIn <= '0';
    elsif rising_edge(Clk3x) then

      if dataCount < 2 then
        c3DataIn <= "0000000" & std_logic_vector(to_unsigned(tempQdata+1,17)) &
                    "0000000" & std_logic_vector(to_unsigned(tempIdata+1,17)) &
                    "0000000" & std_logic_vector(to_unsigned(tempQdata+0,17)) &
                    "0000000" & std_logic_vector(to_unsigned(tempIdata+0,17));
        dataCount := dataCount + 1;
        c3DataValidIn <= '1';
        tempQdata := tempQdata +2;
        tempIdata := tempIdata +2;
      elsif dataCount = 2 then
        c3DataIn <= (others => '0');
        dataCount := 0;
        c3DataValidIn <= '0';
      end if;

    end if;
  end process;

  -- Process to generate expected data that is used to verify the DUT output.
  expected_data: process(Clk1x)
    variable tempQdata : integer := 1;
    variable tempIdata : integer := 128;
  begin
    if rising_edge(Clk1x) then

      if aTestReset and c1DataValidOut = '0' then
        tempQdata := 1;
        tempIdata := 128;
      elsif c1DataValidOut = '1' then
        tempQdata := tempQdata+4;
        tempIdata := tempIdata+4;
      end if;
      c1ExpectedData <= "0000000" & std_logic_vector(to_unsigned(tempQdata+3,17)) &
                        "0000000" & std_logic_vector(to_unsigned(tempIdata+3,17)) &
                        "0000000" & std_logic_vector(to_unsigned(tempQdata+2,17)) &
                        "0000000" & std_logic_vector(to_unsigned(tempIdata+2,17)) &
                        "0000000" & std_logic_vector(to_unsigned(tempQdata+1,17)) &
                        "0000000" & std_logic_vector(to_unsigned(tempIdata+1,17)) &
                        "0000000" & std_logic_vector(to_unsigned(tempQdata+0,17)) &
                        "0000000" & std_logic_vector(to_unsigned(tempIdata+0,17));

    end if;
  end process;

  -- Process to continuously check output data from the DUT.
  checker: process(Clk1x)
  begin
    if falling_edge(Clk1x) then
      if c1DataValidOut = '1' then
        assert c1DataOut = c1ExpectedData
          report "ADC data out mismatch from expected"
          severity error;
      end if;
    end if;
  end process;

end RTL;
