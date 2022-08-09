--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: tb_ddc_400m_saturate
--
-- Description:
--
-- Self-checking testbench used to check the saturation logic needed in DDC.
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

library WORK;
  use WORK.PkgRf.all;

entity tb_ddc_400m_saturate is
end tb_ddc_400m_saturate;


architecture RTL of tb_ddc_400m_saturate is

  component ddc_400m_saturate
    port (
      Clk            : in  std_logic;
      cDataIn        : in  std_logic_vector(191 downto 0);
      cDataValidIn   : in  std_logic;
      cDataOut       : out std_logic_vector(127 downto 0);
      cDataValidOut  : out std_logic);
  end component;

  signal TestStart : boolean := false;

  signal cDataIn       : std_logic_vector(191 downto 0);
  signal cDataOut      : std_logic_vector(127 downto 0);
  signal cDataValidIn  : std_logic;
  signal cDataValidOut : std_logic;

  signal StopSim : boolean;
  constant kPer : time := 10 ns;
  constant kSamplesPerClock : integer := 8;

  signal Clk: std_logic := '1';

  procedure ClkWait(X : positive := 1) is
  begin
    for i in 1 to X loop
      wait until rising_edge(Clk);
    end loop;
  end procedure ClkWait;

begin

  Clk <= not Clk after kPer/2 when not StopSim else '0';

  dut: ddc_400m_saturate
    port map (
      Clk            => Clk,
      cDataIn        => cDataIn,
      cDataValidIn   => cDataValidIn,
      cDataOut       => cDataOut,
      cDataValidOut  => cDataValidOut);

  main: process
  begin

    ClkWait;
    TestStart <= false;
    ClkWait;
    TestStart <= true;

    -- This wait is needed to sweep through the entire range of 17 bits signed
    -- value. Since we operate the saturation logic with 8 samples per cycle,
    -- we need to wait for 2^kDdcDataOutWidth/8. We are adding an extra 10
    -- clock cycles wait just as a buffer for the DUT latency.
    ClkWait(2**kDdcDataOutWidth/kSamplesPerClock + 10);
    StopSim <= true;
    wait;
  end process;

  -- Process to generate 17-bit signed data.
  DataGen: process(Clk)
    variable Sample : Sample17_t := kSmallest17;
  begin
    if falling_edge(Clk) then
      if TestStart then
        cDataValidIn <= '1';
        cDataIn <= "0000000" & std_logic_vector(Sample+kSamplesPerClock-1) &
                   "0000000" & std_logic_vector(Sample+kSamplesPerClock-2) &
                   "0000000" & std_logic_vector(Sample+kSamplesPerClock-3) &
                   "0000000" & std_logic_vector(Sample+kSamplesPerClock-4) &
                   "0000000" & std_logic_vector(Sample+kSamplesPerClock-5) &
                   "0000000" & std_logic_vector(Sample+kSamplesPerClock-6) &
                   "0000000" & std_logic_vector(Sample+kSamplesPerClock-7) &
                   "0000000" & std_logic_vector(Sample+kSamplesPerClock-8);
        Sample := Sample +8;
      end if;
    end if;
  end process;

  -- Check if saturation and data packing is done correctly.
  DataCheck: process(Clk)
    variable Sample : Sample17_t := kSmallest17;
    variable ExpectedData : std_logic_vector(15 downto 0);

  begin
    if falling_edge(Clk) then
      if cDataValidOut then
        for i in 1 to 8 loop
          ExpectedData := tb_saturate(std_logic_vector(Sample));
          assert cDataOut(kSatDataWidth*i-1 downto kSatDataWidth*(i-1)) = ExpectedData
            report "Saturation data out mismatch in index : " & to_string(i) & LF &
                   "Expected data is : " & to_hstring(ExpectedData) & LF &
                   "Received data is : " & to_hstring(cDataOut(kSatDataWidth*i-1 downto kSatDataWidth*(i-1)))
            severity error;
          Sample := Sample+1;
        end loop;
      end if;
    end if;
  end process;

end RTL;
