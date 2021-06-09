--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: dac_2_1_clk_converter
--
-- Description:
--
--   This module transfers data from s_axis_aclk to m_axis_aclk. s_axis_aclk
--   must be two times the frequency of m_axis_aclk, and the two clocks must be
--   related (this module requires timing closure across the clock domain
--   boundary).
--

library IEEE;
  use IEEE.std_logic_1164.all;

entity dac_2_1_clk_converter is
  port (
    s_axis_aclk    : in  std_logic;
    s_axis_aresetn : in  std_logic;
    s_axis_tvalid  : in  std_logic;
    s_axis_tdata   : in  std_logic_vector(63 downto 0);

    m_axis_aclk    : in  std_logic;
    m_axis_aresetn : in  std_logic;
    m_axis_tready  : in  std_logic;
    m_axis_tvalid  : out std_logic;
    m_axis_tdata   : out std_logic_vector(63 downto 0)
  );
end entity dac_2_1_clk_converter;

architecture RTL of dac_2_1_clk_converter is

  -- To keep the implementation simple, this module does not implement a
  -- correct AXIS handshake - it ignores m_axis_tready. dac_100m_bd already had
  -- an assumption that the AXIS handshake is unneeded: duc_saturate does not
  -- accept _tready from the following component. Also, registered_dac_data has
  -- never accepted _tready from dac_2_1_clk_converter, so dac_100m_bd has
  -- never supported complete AXIS dataflow.

  subtype Word_t is std_logic_vector(s_axis_tdata'range);
  signal s_axis_tvalid_pipe : std_logic_vector(1 downto 0);
  signal s_axis_tdata_reg : Word_t;

  -- These _CDC signals will be sampled in the destination clock domain, but
  -- will not produce any metastability because the input clocks must be
  -- synchronous.
  --
  -- These signals must be driven by registers not to prevent glitches (as in
  -- an asynchronous CDC), but to improve timing closure.
  signal s_axis_tvalid_CDC : std_logic;
  signal s_axis_tdata_CDC : Word_t;

  -- m_axis_aclk and s_axis_aclk are nominally aligned by their rising edges.
  -- Because m_axis_aclk is more heavily loaded than s_axis_aclk, m_axis_aclk
  -- has a larger distribution delay, which causes a large hold violation using
  -- post-place timing estimates. The Ultrafast method (UG 949) recommends
  -- addressing such hold violations when WHS < -0.5 ns. By resampling on the
  -- falling edge of the destination clock, we get nominally half a period of
  -- setup and half a period of hold. The destination clock delay reduces the
  -- hold margin, and increases the setup margin.
  signal m_axis_tvalid_fall : std_logic;
  signal m_axis_tdata_fall : Word_t;

begin

  -- In the source clock domain, we capture incoming valid data and keep a
  -- history of _tvalid over the last three clock cycles. If s_axis_tvalid has
  -- been asserted once in the last three clock cycles, assert
  -- s_axis_tvalid_CDC to be sampled in the output clock domain. The length of
  -- s_axis_tvalid_pipe must match the ratio of the clock frequencies (2:1).
  InputSampling:
  process (s_axis_aclk) is
  begin
    if rising_edge(s_axis_aclk) then
      if s_axis_tvalid='1' then
        s_axis_tdata_reg <= s_axis_tdata;
      end if;
      s_axis_tdata_CDC <= s_axis_tdata_reg;
      if s_axis_aresetn='0' then
        s_axis_tvalid_pipe <= (others => '0');
        s_axis_tvalid_CDC <= '0';
      else
        s_axis_tvalid_pipe <= s_axis_tvalid_pipe(0) & s_axis_tvalid;
        if (s_axis_tvalid_pipe /= "00") then
          s_axis_tvalid_CDC <= '1';
        else
          s_axis_tvalid_CDC <= '0';
        end if;
      end if;
    end if;
  end process InputSampling;

  FallingEdgeSampling:
  process (m_axis_aclk) is
  begin
    if falling_edge(m_axis_aclk) then
      m_axis_tvalid_fall <= s_axis_tvalid_CDC;
      m_axis_tdata_fall <= s_axis_tdata_CDC;
    end if;
  end process FallingEdgeSampling;

  OutputRegisters:
  process (m_axis_aclk) is
  begin
    if rising_edge(m_axis_aclk) then
      m_axis_tdata <= m_axis_tdata_fall;
      if m_axis_aresetn='0' then
        m_axis_tvalid <= '0';
      else
        m_axis_tvalid <= m_axis_tvalid_fall;
      end if;
    end if;
  end process OutputRegisters;

end RTL;
