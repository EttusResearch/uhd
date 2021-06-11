--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: tb_capture_sysref
--
-- Description:
--
--   Self-checking testbench for tb_capture_sysref.
--

library IEEE;
  use IEEE.std_logic_1164.all;
  use IEEE.numeric_std.all;

entity tb_capture_sysref is
end tb_capture_sysref;


architecture RTL of tb_capture_sysref is

  component capture_sysref
    port (
      pll_ref_clk     : in  std_logic;
      rfdc_clk        : in  std_logic;
      sysref_in       : in  std_logic;
      enable_rclk     : in  std_logic;
      sysref_out_pclk : out std_logic;
      sysref_out_rclk : out std_logic);
  end component;

  signal enable_rclk     : std_logic := '0';
  signal sysref_out_pclk : std_logic := '0';
  signal sysref_out_rclk : std_logic := '0';
  signal sysref_in       : std_logic := '0';

  signal SysrefDly, SysrefDlyDly, rSysref : std_logic := '0';

  signal StopSim : boolean;
  constant kPerPRC : time := 30 ns;
  constant kPerRF  : time := 10 ns;

  signal PllRefClk : std_logic := '1';
  signal RfdcClk   : std_logic := '1';

  procedure ClkWait(X : positive := 1) is
  begin
    for i in 1 to X loop
      wait until rising_edge(PllRefClk);
    end loop;
  end procedure ClkWait;

begin

  PllRefClk <= not PllRefClk after kPerPRC/2 when not StopSim else '0';
  RfdcClk   <= not RfdcClk   after kPerRF/2  when not StopSim else '0';

  dut: capture_sysref
    port map (
      pll_ref_clk     => PllRefClk,
      rfdc_clk        => RfdcClk,
      sysref_in       => sysref_in,
      enable_rclk     => enable_rclk,
      sysref_out_pclk => sysref_out_pclk,
      sysref_out_rclk => sysref_out_rclk
    );

  main: process
  begin
    enable_rclk <= '1';
    ClkWait(100);
    wait until falling_edge(sysref_out_rclk);
    ClkWait;
    wait until falling_edge(RfdcClk);
    enable_rclk <= '0';
    ClkWait(100);
    wait until falling_edge(RfdcClk);
    enable_rclk <= '1';
    ClkWait(100);

    StopSim <= true;
    wait;
  end process;

  sysref: process(PllRefClk)
    variable count : integer := 1;
  begin
    if rising_edge(PllRefClk) then
      count := count +1;
      if count = 10 then
        sysref_in <= not sysref_in;
        count := 1;
      end if;
    end if;
  end process;

  checker_pll_ref_clk: process(PllRefClk)
  begin
    if falling_edge(PllRefClk) then
      SysrefDly    <= sysref_in;
      SysrefDlyDly <= SysrefDly;
      assert SysrefDlyDly = sysref_out_pclk
        report "SYSREF incorrectly captured in the PllRefClk domain"
        severity error;
    end if;
  end process;

  checker_rfdc_clk: process(RfdcClk)
  begin
    if falling_edge(RfdcClk) then
      rSysref    <= sysref_out_pclk;
      assert (rSysref = sysref_out_rclk) or (enable_rclk = '0')
        report "SYSREF incorrectly captured in the RfdcClk domain."
        severity error;
    end if;
  end process;

end RTL;
