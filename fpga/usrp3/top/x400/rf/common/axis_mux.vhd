--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: axis_mux
--
-- Description:
--
--   This module implements a data mux for a single AXIS bus. When
--   mux_select='0' m_axis_tdata comes from s_axis_tdata. mux_select='1'
--   chooses GPIO as the output data.
--
--   This module IS NOT useful for crossing clock domain boundaries s_axis_aclk
--   and m_axis_mclk must be connected to the same clock.
--
--   This mux is intended for muxing in constant calibration data from gpio.
--   gpio and mux_select are expected to be asynchronous to s_axis_aclk, but
--   this module includes no synchronization logic. When mux_select or gpio
--   change, m_axis_tvalid and m_axis_tdata are undefined in the first few
--   clock cycles. You must wait for bad axis cycles to flush through the
--   remainder of the pipeline before performing calibration and again after
--   exiting calibration mode.
--
--   kAxiWidth must be an integer multiple of kGpioWidth. A concurrent assert
--   statement checks this assumption and should produce a synthesis warning if
--   that requirement is not met.
--
-- Parameters:
--
--   kGpioWidth : GPIO width.
--   kAxiWidth  : AXI bus width. Must be an integer multiple of kGpioWidth
--

library IEEE;
  use IEEE.std_logic_1164.all;

entity axis_mux is
  generic (
    kGpioWidth : natural := 32;
    kAxiWidth  : natural := 256
  );
  port(
    gpio          : in  std_logic_vector(kGpioWidth-1 downto 0);
    mux_select    : in  std_logic;

    -- s_axis_aclk MUST be the same as m_axis_aclk.
    -- Declaring an unused clock allows the BD tool to identify the
    -- synchronicity of the slave AXIS port signals.
    s_axis_aclk   : in  std_logic;
    s_axis_tdata  : in  std_logic_vector(kAxiWidth - 1 downto 0);
    s_axis_tvalid : in  std_logic;
    s_axis_tready : out std_logic;
    m_axis_aclk   : in  std_logic;
    m_axis_tvalid : out std_logic;
    m_axis_tdata  : out std_logic_vector(kAxiWidth - 1 downto 0)
  );
end entity axis_mux;

architecture RTL of axis_mux is

  constant kWordSize  : natural := gpio'length;
  constant kWordCount : natural := kAxiWidth / kWordSize;

  subtype AxiData_t is std_logic_vector(kAxiWidth - 1 downto 0);

  impure function ConcatenatedData return AxiData_t is
    variable rval : AxiData_t;
  begin
      for i in 0 to kWordCount - 1 loop
        rval(i*kWordSize + kWordSize - 1 downto i*kWordSize) := gpio;
      end loop;
      return rval;
  end function ConcatenatedData;

begin

  assert kWordSize * kWordCount = kAxiWidth
    report "m_axis_tdata'length is not an integer multiple of gpio'length"
    severity failure;

  MuxOutputRegister:
  process (m_axis_aclk) is
  begin
    if rising_edge(m_axis_aclk) then
      if mux_select='1' then
        m_axis_tdata  <= ConcatenatedData;
        m_axis_tvalid <= '1';
      else
        m_axis_tdata  <= s_axis_tdata;
        m_axis_tvalid <= s_axis_tvalid;
      end if;
    end if;
  end process MuxOutputRegister;

  s_axis_tready <= '1';

end RTL;
