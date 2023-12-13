--
-- Copyright 2022 Ettus Research, a National Instruments Brand
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
    m_axis_tdata_3  : out std_logic_vector(kAxiWidth - 1 downto 0);

    s_axis_4_aclk   : in  std_logic;
    s_axis_tdata_4  : in  std_logic_vector(kAxiWidth - 1 downto 0);
    s_axis_tvalid_4 : in  std_logic;
    s_axis_tready_4 : out std_logic;
    m_axis_4_aclk   : in  std_logic;
    m_axis_tvalid_4 : out std_logic;
    m_axis_tdata_4  : out std_logic_vector(kAxiWidth - 1 downto 0);

    s_axis_5_aclk   : in  std_logic;
    s_axis_tdata_5  : in  std_logic_vector(kAxiWidth - 1 downto 0);
    s_axis_tvalid_5 : in  std_logic;
    s_axis_tready_5 : out std_logic;
    m_axis_5_aclk   : in  std_logic;
    m_axis_tvalid_5 : out std_logic;
    m_axis_tdata_5  : out std_logic_vector(kAxiWidth - 1 downto 0);

    s_axis_6_aclk   : in  std_logic;
    s_axis_tdata_6  : in  std_logic_vector(kAxiWidth - 1 downto 0);
    s_axis_tvalid_6 : in  std_logic;
    s_axis_tready_6 : out std_logic;
    m_axis_6_aclk   : in  std_logic;
    m_axis_tvalid_6 : out std_logic;
    m_axis_tdata_6  : out std_logic_vector(kAxiWidth - 1 downto 0);

    s_axis_7_aclk   : in  std_logic;
    s_axis_tdata_7  : in  std_logic_vector(kAxiWidth - 1 downto 0);
    s_axis_tvalid_7 : in  std_logic;
    s_axis_tready_7 : out std_logic;
    m_axis_7_aclk   : in  std_logic;
    m_axis_tvalid_7 : out std_logic;
    m_axis_tdata_7  : out std_logic_vector(kAxiWidth - 1 downto 0)
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
      mux_select    => mux_select(2),
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
      mux_select    => mux_select(3),
      s_axis_aclk   => s_axis_3_aclk,
      s_axis_tdata  => s_axis_tdata_3,
      s_axis_tvalid => s_axis_tvalid_3,
      s_axis_tready => s_axis_tready_3,
      m_axis_aclk   => m_axis_3_aclk,
      m_axis_tvalid => m_axis_tvalid_3,
      m_axis_tdata  => m_axis_tdata_3
    );

  axis_mux4: entity work.axis_mux (RTL)
    generic map (
      kGpioWidth => kGpioWidth,
      kAxiWidth  => kAxiWidth)
    port map (
      gpio          => gpio,
      mux_select    => mux_select(4),
      s_axis_aclk   => s_axis_4_aclk,
      s_axis_tdata  => s_axis_tdata_4,
      s_axis_tvalid => s_axis_tvalid_4,
      s_axis_tready => s_axis_tready_4,
      m_axis_aclk   => m_axis_4_aclk,
      m_axis_tvalid => m_axis_tvalid_4,
      m_axis_tdata  => m_axis_tdata_4
    );

  axis_mux5: entity work.axis_mux (RTL)
    generic map (
      kGpioWidth => kGpioWidth,
      kAxiWidth  => kAxiWidth)
    port map (
      gpio          => gpio,
      mux_select    => mux_select(5),
      s_axis_aclk   => s_axis_5_aclk,
      s_axis_tdata  => s_axis_tdata_5,
      s_axis_tvalid => s_axis_tvalid_5,
      s_axis_tready => s_axis_tready_5,
      m_axis_aclk   => m_axis_5_aclk,
      m_axis_tvalid => m_axis_tvalid_5,
      m_axis_tdata  => m_axis_tdata_5
    );

  axis_mux6: entity work.axis_mux (RTL)
    generic map (
      kGpioWidth => kGpioWidth,
      kAxiWidth  => kAxiWidth)
    port map (
      gpio          => gpio,
      mux_select    => mux_select(6),
      s_axis_aclk   => s_axis_6_aclk,
      s_axis_tdata  => s_axis_tdata_6,
      s_axis_tvalid => s_axis_tvalid_6,
      s_axis_tready => s_axis_tready_6,
      m_axis_aclk   => m_axis_6_aclk,
      m_axis_tvalid => m_axis_tvalid_6,
      m_axis_tdata  => m_axis_tdata_6
    );

  axis_mux7: entity work.axis_mux (RTL)
    generic map (
      kGpioWidth => kGpioWidth,
      kAxiWidth  => kAxiWidth)
    port map (
      gpio          => gpio,
      mux_select    => mux_select(7),
      s_axis_aclk   => s_axis_7_aclk,
      s_axis_tdata  => s_axis_tdata_7,
      s_axis_tvalid => s_axis_tvalid_7,
      s_axis_tready => s_axis_tready_7,
      m_axis_aclk   => m_axis_7_aclk,
      m_axis_tvalid => m_axis_tvalid_7,
      m_axis_tdata  => m_axis_tdata_7
    );


end RTL;
