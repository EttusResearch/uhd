--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: dac_1_3_clk_converter
--
-- Description:
--
--   This module transfers data from s_axis_aclk to m_axis_aclk. m_axis_aclk
--   must be three times the frequency of s_axis_aclk, and the two clocks must
--   be related (this module requires timing closure across the clock domain
--   boundary).
--

library IEEE;
  use IEEE.std_logic_1164.all;

entity dac_1_3_clk_converter is
  port(
    s_axis_aclk    : in  std_logic;
    s_axis_aresetn : in  std_logic;
    s_axis_tvalid  : in  std_logic;
    s_axis_tdata   : in  std_logic_vector(31 downto 0);
    s_axis_tready  : out std_logic := '1';

    m_axis_aclk    : in  std_logic;
    m_axis_aresetn : in  std_logic;
    m_axis_tready  : in  std_logic;
    m_axis_tdata   : out std_logic_vector(31 downto 0);
    m_axis_tvalid  : out std_logic
  );
end entity dac_1_3_clk_converter;

architecture RTL of dac_1_3_clk_converter is

  -- I was unable to think of a simple implementation that implements a correct
  -- AXIS handshake on both ports. All my ideas became equivalent to a two
  -- clock FIFO (although the clocks are synchronous, so the write-to-read
  -- latency would have been certain).
  --
  -- We don't expect the DAC to ever hold off incoming data, and dac_100m_bd
  -- already has the AXIS handshake disconnected: the FIR is configured to
  -- disallow back pressure - it has no m_axis_data_tready pin.
  --
  -- I'm going with the simple, but not strictly correct, implementation.
  -- s_axis_tready will be constantly true, even when it shouldn't be. The
  -- bottom line is this component is likely useless for any application but
  -- dac_100m_bd.

  type output_fsm is (
    idle,
    got_data,
    -- The recovery state of delay ensures that we don't re-use an old input
    -- valid signal (remember the output clock is 3x the frequency of the input
    -- clock)
    recovery
  );

  subtype word is std_logic_vector(s_axis_tdata'range);
  signal output_state_mclk : output_fsm;
  signal axis_tdata_sclk : word;
  signal axis_tvalid_sclk : std_logic;

  signal axis_tdata_mclk : word;
  signal axis_tvalid_mclk : std_logic;

begin

  s_axis_tready <= '1';

  input_valid_register:
  process(s_axis_aclk, s_axis_aresetn) is
  begin
    if s_axis_aresetn='0' then
      axis_tvalid_sclk <= '0';
    elsif rising_edge(s_axis_aclk) then
      axis_tvalid_sclk <= s_axis_tvalid;
    end if;
  end process;

  input_data_register:
  process (s_axis_aclk) is
  begin
    if rising_edge(s_axis_aclk) then
      axis_tdata_sclk <= s_axis_tdata;
    end if;
  end process input_data_register;

  -- These CDC registers will not become metastable because the two clock
  -- domains are related.
  cdc_input_valid_register:
  process (m_axis_aclk, m_axis_aresetn) is
  begin
    if m_axis_aresetn='0' then
      axis_tvalid_mclk <= '0';
    elsif rising_edge(m_axis_aclk) then
      axis_tvalid_mclk <= axis_tvalid_sclk;
    end if;
  end process cdc_input_valid_register;

  cdc_input_data_register:
  process (m_axis_aclk) is
  begin
    if rising_edge(m_axis_aclk) then
      axis_tdata_mclk <= axis_tdata_sclk;
    end if;
  end process cdc_input_data_register;

  output_data_register:
  process (m_axis_aclk) is
  begin
    if rising_edge(m_axis_aclk) then
      if output_state_mclk=idle then
        m_axis_tdata <= axis_tdata_mclk;
      end if;
    end if;
  end process output_data_register;

  fsm: process(m_axis_aresetn, m_axis_aclk) is
  begin
    if m_axis_aresetn='0' then
      output_state_mclk <= idle;
      m_axis_tvalid <= '0';
    elsif rising_edge(m_axis_aclk) then
      m_axis_tvalid <= '0';
      case output_state_mclk is
        when idle =>
          if axis_tvalid_mclk='1' then
            output_state_mclk <= got_data;
          end if;

        when got_data =>
          m_axis_tvalid <= '1';
          output_state_mclk <= recovery;

        when recovery =>
          output_state_mclk <= idle;
      end case;
    end if;
  end process fsm;

end RTL;
