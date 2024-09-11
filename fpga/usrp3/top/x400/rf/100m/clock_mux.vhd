--
-- Copyright 2026 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: clock_mux
--
-- Description:
--
--   Simple 2 to 1 mux for selecting between two clock signals.
--   Selection signal needs to be a constant.
--

library IEEE;
  use IEEE.std_logic_1164.all;

Library UNISIM;
  use UNISIM.vcomponents.all;

entity clock_mux is
  port (
    clk0    : in std_logic;
    clk1    : in std_logic;
    sel     : in std_logic;
    clk_out : out std_logic
  );
end entity clock_mux;

architecture rtl of clock_mux is

  -- Adding clocking attributes for Vivado block diagram.
  -- Setting the output frequency to match the 184.32 MHz clock rfdc_clk_1x in the block diagram
  -- x410_ps_rfdc_bd.
  -- These attributes are specifically for the 100 MHz CLIP.
  ATTRIBUTE X_INTERFACE_INFO : STRING;
  ATTRIBUTE X_INTERFACE_PARAMETER : STRING;
  ATTRIBUTE X_INTERFACE_INFO of clk_out: SIGNAL is "xilinx.com:signal:clock:1.0 clk_out CLK";
  ATTRIBUTE X_INTERFACE_PARAMETER of clk_out: SIGNAL is "FREQ_HZ 184320000";

begin

  -- This implementation assumes a constant on sel in order to resolve this statement to just a
  -- connection without any logic element on it.
  -- Using as a BUFGCTRL would add additional delay on the clock signal.
  -- This delay introduces problems as the clock signals need to be closely aligned to move data
  -- synchronously between the two clock domains.
  clk_out <= clk0 when sel = '0' else clk1;

end architecture rtl;
