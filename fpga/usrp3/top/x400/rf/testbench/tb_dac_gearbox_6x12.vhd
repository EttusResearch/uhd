--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: tb_dac_gearbox_6x12
--
-- Description:
--
--   Self-checking testbench used to test the gearbox that expands a 6 SPC data
--   into a 12 SPC data.
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

entity tb_dac_gearbox_6x12 is
end tb_dac_gearbox_6x12;


architecture RTL of tb_dac_gearbox_6x12 is

  signal TestStart : boolean;

  signal ac1Reset_n     : std_logic;
  signal ac2Reset_n     : std_logic;
  signal c1DataOut      : std_logic_vector(383 downto 0);
  signal c1DataValidOut : std_logic;
  signal c2DataIn       : std_logic_vector(191 downto 0) := (others => '0');
  signal c2DataValidIn  : std_logic := '0';
  signal InPhase        : boolean := false;

  signal c1DataToCheck, c1DataToCheckDly0, c1DataToCheckDly1, c1DataToCheckDly2
         : std_logic_vector(383 downto 0) := (others => '0');

  signal StopSim : boolean;
  constant kPer : time := 12 ns;

  signal Clk1x: std_logic := '1';
  signal Clk2x: std_logic := '1';

  procedure Clk2xWait(X : positive := 1) is
  begin
    for i in 1 to X loop
      wait until rising_edge(Clk2x);
    end loop;
  end procedure Clk2xWait;

begin

  Clk1x  <= not Clk1x after kPer/4 when not StopSim else '0';
  Clk2x  <= not Clk2x after kPer/8 when not StopSim else '0';

  dut: entity WORK.dac_gearbox_6x12 (RTL)
    port map (
      Clk1x          => Clk1x,
      Clk2x          => Clk2x,
      ac1Reset_n     => ac1Reset_n,
      ac2Reset_n     => ac2Reset_n,
      c2DataIn       => c2DataIn,
      c2DataValidIn  => c2DataValidIn,
      c1DataOut      => c1DataOut,
      c1DataValidOut => c1DataValidOut
    );


  main: process

    -- Procedure to start and stop data generation.
    -- WaitCycles : This is a wait in Clk2x cycle. This is used to shift data
    --              valid assertion. Depending on the Clk2x cycle, data valid
    --              will be asserted either when both Clk1x and Clk2x are phase
    --              aligned or when both clocks are not phase aligned.
    -- Phase      : This input is used in the logic that is used to check
    --              output data with expected data. If data valid was asserted
    --              when both clocks were phase aligned, then this input is
    --              set to true and vice versa.
    procedure PhaseTest(WaitCycles : positive := 1;
                        Phase      : boolean  := false) is
    begin
      -- Wait for certain Clk2x cycles before starting the test.
      Clk2xWait(WaitCycles);
      InPhase   <= Phase;
      TestStart <= true;
      Clk2xWait(1000); -- Random wait.
      TestStart <= false;
      -- wait for few clock cycles for the output data valid to de-assert.
      Clk2xWait(10);
    end procedure;

  begin

    -- Assert and de-assert reset.
    ac1Reset_n <= '0';
    ac2Reset_n <= '0';
    TestStart <= false;
    Clk2xWait(5);
    ac1Reset_n <= '1';
    ac2Reset_n <= '1';

    PhaseTest(1, true);
    PhaseTest(3, false);
    PhaseTest(5, true);

    -- Stop data input to the DUT and wait for few clock cycles for the output
    -- data valid to be de-asserted.
    TestStart <= false;
    Clk2xWait(10);

    StopSim <= true;
    wait;
  end process;

  driver: process(Clk2x)
    variable tempQdata : unsigned(15 downto 0) := x"0001";
    variable tempIdata : unsigned(15 downto 0) := x"0080";
  begin
    if rising_edge(Clk2x) then
      c2DataValidIn <= '0';
      if TestStart then
        c2DataValidIn <= '1';
        c2DataIn <= std_logic_vector((tempQdata+5) & (tempIdata+5) &
                                     (tempQdata+4) & (tempIdata+4) &
                                     (tempQdata+3) & (tempIdata+3) &
                                     (tempQdata+2) & (tempIdata+2) &
                                     (tempQdata+1) & (tempIdata+1) &
                                     (tempQdata+0) & (tempIdata+0));
        tempQdata := tempQdata +6;
        tempIdata := tempIdata +6;
      else
        c2DataValidIn <= '0';
        tempQdata := x"0001";
        tempIdata := x"0080";
      end if;
    end if;
  end process;

  -- Process to generate expected data out of the DUT.
  ExpectedData: process(Clk1x)
    variable qDataOut : unsigned(15 downto 0) := x"0001";
    variable iDataOut : unsigned(15 downto 0) := x"0080";
  begin
    if rising_edge(Clk1x) then
      if TestStart then
        c1DataToCheck <= std_logic_vector((qDataOut+11) & (iDataOut+11)  &
                                          (qDataOut+10) & (iDataOut+10)  &
                                          (qDataOut+9)  & (iDataOut+9)   &
                                          (qDataOut+8)  & (iDataOut+8)   &
                                          (qDataOut+7)  & (iDataOut+7)   &
                                          (qDataOut+6)  & (iDataOut+6)   &
                                          (qDataOut+5)  & (iDataOut+5)   &
                                          (qDataOut+4)  & (iDataOut+4)   &
                                          (qDataOut+3)  & (iDataOut+3)   &
                                          (qDataOut+2)  & (iDataOut+2)   &
                                          (qDataOut+1)  & (iDataOut+1)   &
                                          (qDataOut+0)  & (iDataOut+0));

        qDataOut := qDataOut+12;
        iDataOut := iDataOut+12;
      else
        qDataOut := x"0001";
        iDataOut := x"0080";
      end if;
      c1DataToCheckDly0 <= c1DataToCheck;
      c1DataToCheckDly1 <= c1DataToCheckDly0;
      c1DataToCheckDly2 <= c1DataToCheckDly1;
    end if;
  end process;

  -- Process to check output data with expected data.
  checker: process(Clk1x)
  begin
    if falling_edge(Clk1x) then
      if c1DataValidOut = '1' and InPhase then
        assert c1DataOut = c1DataToCheckDly1
          report "ADC data out mismatch from expected"
          severity warning;
      elsif c1DataValidOut = '1' and (not InPhase) then
        assert c1DataOut = c1DataToCheckDly2
          report "ADC data out mismatch from expected"
          severity warning;
      end if;
    end if;
  end process;

end RTL;
