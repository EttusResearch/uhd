--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: gpio_to_axis_mux
--
-- Description:
--
--   This module either drives the AXIS outputs with the corresponding AXIS
--   slave inputs, or it drives the output AXIS with data provided by the GPIO
--   lines. This allows the calibration process to drive a constant value to
--   the DAC's. Although every AXIS interface has its own clock, all the clocks
--   must be connected to the same source. Independent clock inputs allows the
--   block design editor to automatically detect the clock domain of the
--   corresponding interface.
--
--   kAxiWidth must be an integer multiple of kGpioWidth. A concurrent assert
--   statement in axis_mux checks this assumption and should produce a
--   synthesis warning if that requirement is not met.
--
-- Parameters:
--
--   kGpioWidth : GPIO width.
--   kAxiWidth  : AXI bus width. Must be an integer multiple of kGpioWidth
--

library IEEE;
  use IEEE.std_logic_1164.all;

entity gpio_to_axis_mux is
  generic (
    kGpioWidth : natural := 32;
    kAxiWidth  : natural := 256
  );
  port(
    gpio : in std_logic_vector(kGpioWidth-1 downto 0);

    -- mux_select(n) chooses the data source for AXIS interface n.
    -- '0' chooses s_axis_tdata_n. '1' chooses gpio as the data source.
    -- The only used bits are 0, 1, 4, 5. The remaining bits are reserved for
    -- future expansion.
    mux_select : in std_logic_vector(7 downto 0);

    s_axis_0_aclk   : in  std_logic;
    s_axis_tdata_0  : in  std_logic_vector(kAxiWidth - 1 downto 0);
    s_axis_tvalid_0 : in  std_logic;
    s_axis_tready_0 : out std_logic;
    m_axis_0_aclk   : in  std_logic;
    m_axis_tvalid_0 : out std_logic;
    m_axis_tdata_0  : out std_logic_vector(kAxiWidth - 1 downto 0);

    s_axis_1_aclk   : in  std_logic;
    s_axis_tdata_1  : in  std_logic_vector(kAxiWidth - 1 downto 0);
    s_axis_tvalid_1 : in  std_logic;
    s_axis_tready_1 : out std_logic;
    m_axis_1_aclk   : in  std_logic;
    m_axis_tvalid_1 : out std_logic;
    m_axis_tdata_1  : out std_logic_vector(kAxiWidth - 1 downto 0);

    s_axis_2_aclk   : in  std_logic;
    s_axis_tdata_2  : in  std_logic_vector(kAxiWidth - 1 downto 0);
    s_axis_tvalid_2 : in  std_logic;
    s_axis_tready_2 : out std_logic;
    m_axis_2_aclk   : in  std_logic;
    m_axis_tvalid_2 : out std_logic;
    m_axis_tdata_2  : out std_logic_vector(kAxiWidth - 1 downto 0);

    s_axis_3_aclk   : in  std_logic;
    s_axis_tdata_3  : in  std_logic_vector(kAxiWidth - 1 downto 0);
    s_axis_tvalid_3 : in  std_logic;
    s_axis_tready_3 : out std_logic;
    m_axis_3_aclk   : in  std_logic;
    m_axis_tvalid_3 : out std_logic;
    m_axis_tdata_3  : out std_logic_vector(kAxiWidth - 1 downto 0)
  );
end entity;

architecture RTL of gpio_to_axis_mux is

begin

  axis_mux0: entity work.axis_mux (RTL)
    generic map (
      kGpioWidth => kGpioWidth,
      kAxiWidth  => kAxiWidth)
    port map (
      gpio          => gpio,
      mux_select    => mux_select(0),
      s_axis_aclk   => s_axis_0_aclk,
      s_axis_tdata  => s_axis_tdata_0,
      s_axis_tvalid => s_axis_tvalid_0,
      s_axis_tready => s_axis_tready_0,
      m_axis_aclk   => m_axis_0_aclk,
      m_axis_tvalid => m_axis_tvalid_0,
      m_axis_tdata  => m_axis_tdata_0
    );

  axis_mux1: entity work.axis_mux (RTL)
    generic map (
      kGpioWidth => kGpioWidth,
      kAxiWidth  => kAxiWidth)
    port map (
      gpio          => gpio,
      mux_select    => mux_select(1),
      s_axis_aclk   => s_axis_1_aclk,
      s_axis_tdata  => s_axis_tdata_1,
      s_axis_tvalid => s_axis_tvalid_1,
      s_axis_tready => s_axis_tready_1,
      m_axis_aclk   => m_axis_1_aclk,
      m_axis_tvalid => m_axis_tvalid_1,
      m_axis_tdata  => m_axis_tdata_1
    );

  axis_mux2: entity work.axis_mux (RTL)
    generic map (
      kGpioWidth => kGpioWidth,
      kAxiWidth  => kAxiWidth)
    port map (
      gpio          => gpio,
      mux_select    => mux_select(4),
      s_axis_aclk   => s_axis_2_aclk,
      s_axis_tdata  => s_axis_tdata_2,
      s_axis_tvalid => s_axis_tvalid_2,
      s_axis_tready => s_axis_tready_2,
      m_axis_aclk   => m_axis_2_aclk,
      m_axis_tvalid => m_axis_tvalid_2,
      m_axis_tdata  => m_axis_tdata_2
    );

  axis_mux3: entity work.axis_mux (RTL)
    generic map (
      kGpioWidth => kGpioWidth,
      kAxiWidth  => kAxiWidth)
    port map (
      gpio          => gpio,
      mux_select    => mux_select(5),
      s_axis_aclk   => s_axis_3_aclk,
      s_axis_tdata  => s_axis_tdata_3,
      s_axis_tvalid => s_axis_tvalid_3,
      s_axis_tready => s_axis_tready_3,
      m_axis_aclk   => m_axis_3_aclk,
      m_axis_tvalid => m_axis_tvalid_3,
      m_axis_tdata  => m_axis_tdata_3
    );

end RTL;
