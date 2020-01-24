-------------------------------------------------------------------------------
-- Title      : Platform-dependent components needed for WR PTP Core on Xilinx
-- Project    : WR PTP Core
-- URL        : http://www.ohwr.org/projects/wr-cores/wiki/Wrpc_core
-------------------------------------------------------------------------------
-- File       : wrc_platform_xilinx.vhd
-- Author     : Maciej Lipinski, Grzegorz Daniluk, Dimitrios Lampridis
-- Company    : CERN
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description:
-- This module instantiates platform-specific modules that are needed by the
-- WR PTP Core (WRPC) to interface hardware on Xilinx FPGA. In particular it
-- contains:
-- * PHY
-- * PLLs
-- * buffers
--
-------------------------------------------------------------------------------
--
-- Copyright (c) 2016-2017 CERN / BE-CO-HT
--
-- This source file is free software; you can redistribute it
-- and/or modify it under the terms of the GNU Lesser General
-- Public License as published by the Free Software Foundation;
-- either version 2.1 of the License, or (at your option) any
-- later version
--
-- This source is distributed in the hope that it will be
-- useful, but WITHOUT ANY WARRANTY; without even the implied
-- warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
-- PURPOSE.  See the GNU Lesser General Public License for more
-- details
--
-- You should have received a copy of the GNU Lesser General
-- Public License along with this source; if not, download it
-- from http://www.gnu.org/licenses/lgpl-2.1.html
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

library work;
use work.endpoint_pkg.all;
use work.gencores_pkg.all;
use work.wr_xilinx_pkg.all;

library unisim;
use unisim.vcomponents.all;

entity xwrc_platform_xilinx is
  generic
    (
      -- Define the family/model of Xilinx FPGA
      -- (supported: for now only spartan6)
      g_fpga_family               : string  := "spartan6";
      -- Select whether to include external ref clock input
      g_with_external_clock_input : boolean := FALSE;
      -- Set to FALSE if you want to instantiate your own PLLs
      g_use_default_plls          : boolean := TRUE;
      -- Select GTP channel to use (only one can be active)
      g_gtp_enable_ch0            : integer := 0;
      g_gtp_enable_ch1            : integer := 1;
      -- Set to TRUE will speed up some initialization processes
      g_simulation                : integer := 0;
      g_use_ibufgds               : boolean := false
      );
  port (
    ---------------------------------------------------------------------------
    -- Asynchronous reset (active low)
    ---------------------------------------------------------------------------
    areset_n_i            : in  std_logic;
    ---------------------------------------------------------------------------
    -- 10MHz ext ref clock input (g_with_external_clock_input = TRUE)
    ---------------------------------------------------------------------------
    clk_10m_ext_i         : in  std_logic             := '0';
    ---------------------------------------------------------------------------
    -- 125 MHz GTP/GTX reference
    -- If g_use_ibufgds=false then connect buffered clock to clk_125m_gtp_p_i
    ---------------------------------------------------------------------------
    clk_125m_gtp_p_i      : in  std_logic;
    clk_125m_gtp_n_i      : in  std_logic;
    ---------------------------------------------------------------------------
    -- Clock inputs for default PLLs (g_use_default_plls = TRUE)
    ---------------------------------------------------------------------------
    -- 20MHz VCXO clock
    clk_20m_vcxo_i        : in  std_logic             := '0';
    -- 125.000 MHz PLL reference
    clk_125m_pllref_i     : in  std_logic             := '0';
    -- 124.992 MHz DMTD reference (CLBv3 reference design)
    clk_125m_dmtd_i       : in  std_logic             := '0';
    ---------------------------------------------------------------------------
    -- Clock inputs from custom PLLs (g_use_default_plls = FALSE)
    ---------------------------------------------------------------------------
    -- 62.5MHz DMTD offset clock and lock status
    clk_62m5_dmtd_i       : in  std_logic             := '0';
    clk_dmtd_locked_i     : in  std_logic             := '1';
    -- 62.5MHz Main system clock and lock status
    clk_62m5_sys_i        : in  std_logic             := '0';
    clk_sys_locked_i      : in  std_logic             := '1';
    -- 125MHz  Reference clock
    clk_125m_ref_i        : in  std_logic             := '0';
    clk_ref_locked_i      : in  std_logic             := '1';
    -- 125MHz derived from 10MHz external reference and lock status
    -- (when g_with_external_clock_input = TRUE)
    clk_125m_ext_i        : in  std_logic             := '0';
    clk_ext_locked_i      : in  std_logic             := '1';
    clk_ext_stopped_i     : in  std_logic             := '0';
    clk_ext_rst_o         : out std_logic;
    ---------------------------------------------------------------------------
    -- SFP
    ---------------------------------------------------------------------------
    sfp_txn_o             : out std_logic;
    sfp_txp_o             : out std_logic;
    sfp_rxn_i             : in  std_logic;
    sfp_rxp_i             : in  std_logic;
    sfp_tx_fault_i        : in  std_logic             := '0';
    sfp_los_i             : in  std_logic             := '0';
    sfp_tx_disable_o      : out std_logic;
    ---------------------------------------------------------------------------
    --Interface to WR PTP Core (WRPC)
    ---------------------------------------------------------------------------
    -- PLL outputs
    clk_62m5_sys_o        : out std_logic;
    clk_125m_ref_o        : out std_logic;
    clk_ref_locked_o      : out std_logic;
    clk_62m5_dmtd_o       : out std_logic;
    pll_locked_o          : out std_logic;
    clk_10m_ext_o         : out std_logic;
    -- PHY
    phy8_o                : out t_phy_8bits_to_wrc;
    phy8_i                : in  t_phy_8bits_from_wrc  := c_dummy_phy8_from_wrc;
    phy16_o               : out t_phy_16bits_to_wrc;
    phy16_i               : in  t_phy_16bits_from_wrc := c_dummy_phy16_from_wrc;
    -- External reference
    ext_ref_mul_o         : out std_logic;
    ext_ref_mul_locked_o  : out std_logic;
    ext_ref_mul_stopped_o : out std_logic;
    ext_ref_rst_i         : in  std_logic             := '0'
    );

end entity xwrc_platform_xilinx;

architecture rtl of xwrc_platform_xilinx is

  -----------------------------------------------------------------------------
  -- Signals declaration
  -----------------------------------------------------------------------------

  signal pll_arst            : std_logic := '0';
  signal clk_125m_pllref_buf : std_logic;

begin  -- architecture rtl

  -----------------------------------------------------------------------------
  -- Check for unsupported features and/or misconfiguration
  -----------------------------------------------------------------------------
  gen_unknown_fpga : if (g_fpga_family /= "spartan6" and g_fpga_family /= "kintex7" and g_fpga_family /= "artix7") generate
    assert FALSE
      report "Xilinx FPGA family [" & g_fpga_family & "] is not supported"
      severity ERROR;
  end generate gen_unknown_fpga;

  gen_single_gtp_channel : if (g_gtp_enable_ch0 /= 0 and g_gtp_enable_ch1 /= 0)
  generate
    assert FALSE
      report "Cannot enable both GTP channels simultaneously"
      severity ERROR;
  end generate gen_single_gtp_channel;

  -----------------------------------------------------------------------------
  -- Clock PLLs
  -----------------------------------------------------------------------------

  -- active high async reset for PLLs
  pll_arst <= not areset_n_i;

  gen_default_plls : if (g_use_default_plls = TRUE) generate

    -- Default PLL setup consists of two PLLs.
    -- One takes a 125MHz clock signal as input and produces the
    -- 62.5MHz WR PTP core main system clock.
    -- The other PLL takes a 20MHz clock signal as input and produces the
    -- 62.5MHz DMTD clock.
    --
    -- A third PLL is instantiated if also g_with_external_clock_input = TRUE.
    -- In that case, a 10MHz external reference is multiplied to generate a
    -- 125MHz reference clock
    gen_spartan6_default_plls : if (g_fpga_family = "spartan6") generate

      signal clk_sys          : std_logic;
      signal clk_sys_out      : std_logic;
      signal clk_sys_fb       : std_logic;
      signal pll_sys_locked   : std_logic;
      signal clk_dmtd         : std_logic;
      signal clk_dmtd_fb      : std_logic;
      signal pll_dmtd_locked  : std_logic;
      signal clk_20m_vcxo_buf : std_logic;

    begin  --gen_spartan6_default_plls

      -- System PLL
      cmp_sys_clk_pll : PLL_BASE
        generic map (
          BANDWIDTH          => "OPTIMIZED",
          CLK_FEEDBACK       => "CLKFBOUT",
          COMPENSATION       => "INTERNAL",
          DIVCLK_DIVIDE      => 1,
          CLKFBOUT_MULT      => 8,
          CLKFBOUT_PHASE     => 0.000,
          CLKOUT0_DIVIDE     => 16,
          CLKOUT0_PHASE      => 0.000,
          CLKOUT0_DUTY_CYCLE => 0.500,
          CLKIN_PERIOD       => 8.0,
          REF_JITTER         => 0.016)
        port map (
          CLKFBOUT => clk_sys_fb,
          CLKOUT0  => clk_sys,
          LOCKED   => pll_sys_locked,
          RST      => pll_arst,
          CLKFBIN  => clk_sys_fb,
          CLKIN    => clk_125m_pllref_buf);

      -- System PLL input clock buffer
      cmp_clk_sys_buf_i : BUFG
        port map (
          O => clk_125m_pllref_buf,
          I => clk_125m_pllref_i);

      -- System PLL output clock buffer
      cmp_clk_sys_buf_o : BUFG
        port map (
          O => clk_sys_out,
          I => clk_sys);

      clk_62m5_sys_o <= clk_sys_out;
      clk_125m_ref_o <= clk_125m_pllref_buf;
      pll_locked_o   <= pll_sys_locked and pll_dmtd_locked;
      clk_ref_locked_o <= '1';

      -- DMTD PLL
      cmp_dmtd_clk_pll : PLL_BASE
        generic map (
          BANDWIDTH          => "OPTIMIZED",
          CLK_FEEDBACK       => "CLKFBOUT",
          COMPENSATION       => "INTERNAL",
          DIVCLK_DIVIDE      => 1,
          CLKFBOUT_MULT      => 50,
          CLKFBOUT_PHASE     => 0.000,
          CLKOUT0_DIVIDE     => 16,
          CLKOUT0_PHASE      => 0.000,
          CLKOUT0_DUTY_CYCLE => 0.500,
          CLKIN_PERIOD       => 50.0,
          REF_JITTER         => 0.016)
        port map (
          CLKFBOUT => clk_dmtd_fb,
          CLKOUT0  => clk_dmtd,
          LOCKED   => pll_dmtd_locked,
          RST      => pll_arst,
          CLKFBIN  => clk_dmtd_fb,
          CLKIN    => clk_20m_vcxo_buf);

      -- DMTD PLL input clock buffer
      cmp_clk_dmtd_buf_i : BUFG
        port map (
          O => clk_20m_vcxo_buf,
          I => clk_20m_vcxo_i);

      -- DMTD PLL output clock buffer
      cmp_clk_dmtd_buf_o : BUFG
        port map (
          O => clk_62m5_dmtd_o,
          I => clk_dmtd);


      gen_spartan6_ext_ref_pll : if (g_with_external_clock_input = TRUE) generate

        signal clk_ext_fbi  : std_logic;
        signal clk_ext_fbo  : std_logic;
        signal clk_ext_buf  : std_logic;
        signal clk_ext      : std_logic;
        signal clk_ext_stat : std_logic_vector(7 downto 0);
        signal pll_ext_rst  : std_logic;

      begin  --gen_spartan6_ext_ref_pll

        -- External reference DCM
        cmp_ext_ref_pll : DCM_SP
          generic map
          (CLKDV_DIVIDE       => 2.000,
           CLKFX_DIVIDE       => 2,
           CLKFX_MULTIPLY     => 25,
           CLKIN_DIVIDE_BY_2  => FALSE,
           CLKIN_PERIOD       => 100.0,
           CLKOUT_PHASE_SHIFT => "NONE",
           CLK_FEEDBACK       => "1X",
           DESKEW_ADJUST      => "SYSTEM_SYNCHRONOUS",
           PHASE_SHIFT        => 0,
           STARTUP_WAIT       => FALSE)
          port map
          -- Input clock
          (CLKIN    => clk_ext_buf,
           CLKFB    => clk_ext_fbi,
           -- Output clocks
           CLK0     => clk_ext_fbo,
           CLKFX    => clk_ext,
           -- Ports for dynamic phase shift
           PSCLK    => '0',
           PSEN     => '0',
           PSINCDEC => '0',
           PSDONE   => open,
           -- Other control and status signals
           LOCKED   => ext_ref_mul_locked_o,
           STATUS   => clk_ext_stat,
           RST      => pll_ext_rst,
           -- Unused pin, tie low
           DSSEN    => '0');

        ext_ref_mul_stopped_o <= clk_ext_stat(1);

        -- External reference input buffer
        cmp_clk_ext_buf_i : BUFG
          port map
          (O => clk_ext_buf,
           I => clk_10m_ext_i);

        clk_10m_ext_o <= clk_ext_buf;

        -- External reference feedback buffer
        cmp_clk_ext_buf_fb : BUFG
          port map
          (O => clk_ext_fbi,
           I => clk_ext_fbo);

        -- External reference output buffer
        cmp_clk_ext_buf_o : BUFG
          port map
          (O => ext_ref_mul_o,
           I => clk_ext);

        cmp_extend_ext_reset : gc_extend_pulse
          generic map (
            g_width => 1000)
          port map (
            clk_i      => clk_sys_out,
            rst_n_i    => pll_sys_locked,
            pulse_i    => ext_ref_rst_i,
            extended_o => pll_ext_rst);

      end generate gen_spartan6_ext_ref_pll;

    end generate gen_spartan6_default_plls;

    ---------------------------------------------------------------------------
    --   Kintex7 PLLs
    ---------------------------------------------------------------------------
    gen_kintex7_default_plls : if (g_fpga_family = "kintex7") generate

      signal clk_sys          : std_logic;
      signal clk_sys_out      : std_logic;
      signal clk_sys_fb       : std_logic;
      signal pll_sys_locked   : std_logic;
      signal clk_dmtd         : std_logic;
      signal clk_dmtd_fb      : std_logic;
      signal pll_dmtd_locked  : std_logic;
      signal clk_20m_vcxo_buf : std_logic;

    begin
      -- System PLL (125 MHz -> 62.5 MHz)
      cmp_sys_clk_pll : MMCME2_ADV
        generic map (
          BANDWIDTH            => "OPTIMIZED",
          CLKOUT4_CASCADE      => false,
          COMPENSATION         => "ZHOLD",
          STARTUP_WAIT         => false,
          DIVCLK_DIVIDE        => 1,
          CLKFBOUT_MULT_F      => 8.000,     -- 125 MHz x 8.
          CLKFBOUT_PHASE       => 0.000,
          CLKFBOUT_USE_FINE_PS => false,

          CLKOUT0_DIVIDE_F    => 16.000,     -- 62.5 MHz sys clock
          CLKOUT0_PHASE       => 0.000,
          CLKOUT0_DUTY_CYCLE  => 0.500,
          CLKOUT0_USE_FINE_PS => false,

          CLKIN1_PERIOD => 8.000,            -- 8 ns means 125 MHz
          REF_JITTER1   => 0.010)
        port map (
          -- Output clocks
          CLKFBOUT     => clk_sys_fb,
          CLKOUT0      => clk_sys,
          -- Input clock control
          CLKFBIN      => clk_sys_fb,
          CLKIN1       => clk_125m_pllref_buf,
          CLKIN2       => '0',
          -- Tied to always select the primary input clock
          CLKINSEL     => '1',
          -- Ports for dynamic reconfiguration
          DADDR        => (others => '0'),
          DCLK         => '0',
          DEN          => '0',
          DI           => (others => '0'),
          DO           => open,
          DRDY         => open,
          DWE          => '0',
          -- Ports for dynamic phase shift
          PSCLK        => '0',
          PSEN         => '0',
          PSINCDEC     => '0',
          PSDONE       => open,
          -- Other control and status signals
          LOCKED       => pll_sys_locked,
          CLKINSTOPPED => open,
          CLKFBSTOPPED => open,
          PWRDWN       => '0',
          RST          => pll_arst);

      -- System PLL output clock buffer
      cmp_clk_sys_buf_o : BUFG
      port map (
        I => clk_sys,
        O => clk_sys_out);

      clk_62m5_sys_o <= clk_sys_out;
      pll_locked_o   <= pll_dmtd_locked and pll_sys_locked;

      -- DMTD PLL (20 MHz -> ~62,5 MHz)
      cmp_dmtd_clk_pll : MMCME2_ADV
        generic map (
          BANDWIDTH            => "OPTIMIZED",
          CLKOUT4_CASCADE      => false,
          COMPENSATION         => "ZHOLD",
          STARTUP_WAIT         => false,
          DIVCLK_DIVIDE        => 1,
          CLKFBOUT_MULT_F      => 50.000,    -- 20 MHz -> 1 GHz
          CLKFBOUT_PHASE       => 0.000,
          CLKFBOUT_USE_FINE_PS => false,
          CLKOUT0_DIVIDE_F     => 16.000,    -- 1GHz/16 -> 62.5 MHz
          CLKOUT0_PHASE        => 0.000,
          CLKOUT0_DUTY_CYCLE   => 0.500,
          CLKOUT0_USE_FINE_PS  => false,
          CLKOUT1_DIVIDE       => 16,        -- 1GHz/16 -> 62.5 MHz
          CLKOUT1_PHASE        => 0.000,
          CLKOUT1_DUTY_CYCLE   => 0.500,
          CLKOUT1_USE_FINE_PS  => false,
          CLKIN1_PERIOD        => 50.000,    -- 50ns for 20 MHz
          REF_JITTER1          => 0.010)
        port map (
          -- Output clocks
          CLKFBOUT     => clk_dmtd_fb,
          CLKOUT0      => clk_dmtd,
          -- Input clock control
          CLKFBIN      => clk_dmtd_fb,
          CLKIN1       => clk_20m_vcxo_buf,
          CLKIN2       => '0',
          -- Tied to always select the primary input clock
          CLKINSEL     => '1',
          -- Ports for dynamic reconfiguration
          DADDR        => (others => '0'),
          DCLK         => '0',
          DEN          => '0',
          DI           => (others => '0'),
          DO           => open,
          DRDY         => open,
          DWE          => '0',
          -- Ports for dynamic phase shift
          PSCLK        => '0',
          PSEN         => '0',
          PSINCDEC     => '0',
          PSDONE       => open,
          -- Other control and status signals
          LOCKED       => pll_dmtd_locked,
          CLKINSTOPPED => open,
          CLKFBSTOPPED => open,
          PWRDWN       => '0',
          RST          => pll_arst);

      -- DMTD PLL input clock buffer
      cmp_clk_dmtd_buf_i : BUFG
        port map (
          O => clk_20m_vcxo_buf,
          I => clk_20m_vcxo_i);

      -- DMTD PLL output clock buffer
      cmp_clk_dmtd_buf_o : BUFG
        port map (
          O => clk_62m5_dmtd_o,
          I => clk_dmtd);

      -- External 10MHz reference PLL for Kintex7
      gen_kintex7_ext_ref_pll : if (g_with_external_clock_input = TRUE) generate
        
        signal clk_ext_fbi : std_logic;
        signal clk_ext_fbo : std_logic;
        signal clk_ext_buf : std_logic;
        signal clk_ext_mul : std_logic;
        signal pll_ext_rst : std_logic;

      begin
        mmcm_adv_inst : MMCME2_ADV
          generic map (
            BANDWIDTH            => "OPTIMIZED",
            CLKOUT4_CASCADE      => FALSE,
            COMPENSATION         => "ZHOLD",
            STARTUP_WAIT         => FALSE,
            DIVCLK_DIVIDE        => 1,
            CLKFBOUT_MULT_F      => 62.500,
            CLKFBOUT_PHASE       => 0.000,
            CLKFBOUT_USE_FINE_PS => FALSE,
            CLKOUT0_DIVIDE_F     => 10.000,
            CLKOUT0_PHASE        => 0.000,
            CLKOUT0_DUTY_CYCLE   => 0.500,
            CLKOUT0_USE_FINE_PS  => FALSE,
            CLKIN1_PERIOD        => 100.000,
            REF_JITTER1          => 0.005)
          port map (
            -- Output clocks
            CLKFBOUT  => clk_ext_fbo,
            CLKOUT0   => clk_ext_mul,
            -- Input clock control
            CLKFBIN   => clk_ext_fbi,
            CLKIN1    => clk_ext_buf,
            CLKIN2    => '0',
            -- Tied to always select the primary input clock
            CLKINSEL  => '1',
            -- Ports for dynamic reconfiguration
            DADDR     => (others => '0'),
            DCLK      => '0',
            DEN       => '0',
            DI        => (others => '0'),
            DO        => open,
            DRDY      => open,
            DWE       => '0',
            -- Ports for dynamic phase shift
            PSCLK     => '0',
            PSEN      => '0',
            PSINCDEC  => '0',
            PSDONE    => open, -- Other control and status signals
            LOCKED    => ext_ref_mul_locked_o,
            CLKINSTOPPED => ext_ref_mul_stopped_o,
            CLKFBSTOPPED => open,
            PWRDWN   => '0',
            RST      => pll_ext_rst);

        -- External reference input buffer
        cmp_clk_ext_buf_i : BUFG
          port map (
            O => clk_ext_buf,
            I => clk_10m_ext_i);

        clk_10m_ext_o <= clk_ext_buf;

        -- External reference feedback buffer
        cmp_clk_ext_buf_fb : BUFG
          port map (
            O => clk_ext_fbi,
            I => clk_ext_fbo);

        -- External reference output buffer
        cmp_clk_ext_buf_o : BUFG
          port map (
            O => ext_ref_mul_o,
            I => clk_ext_mul);

        cmp_extend_ext_reset : gc_extend_pulse
          generic map (
            g_width => 1000)
          port map (
            clk_i      => clk_sys_out,
            rst_n_i    => pll_sys_locked,
            pulse_i    => ext_ref_rst_i,
            extended_o => pll_ext_rst);

      end generate gen_kintex7_ext_ref_pll;

    end generate gen_kintex7_default_plls;

    ---------------------------------------------------------------------------
    --   Artix7 PLLs
    ---------------------------------------------------------------------------
    gen_artix7_default_plls : if (g_fpga_family = "artix7") generate

      signal clk_sys          : std_logic;
      signal clk_sys_out      : std_logic;
      signal clk_sys_fb       : std_logic;
      signal pll_sys_locked   : std_logic;
      signal clk_dmtd         : std_logic := '0'; -- initialize for simulation
--      signal clk_dmtd_fb      : std_logic;
--      signal pll_dmtd_locked  : std_logic;
--      signal clk_20m_vcxo_buf : std_logic;

    begin
      -- System PLL (125 MHz -> 62.5 MHz)
      cmp_sys_clk_pll : MMCME2_ADV
        generic map (
          BANDWIDTH            => "OPTIMIZED",
          CLKOUT4_CASCADE      => false,
          COMPENSATION         => "ZHOLD",
          STARTUP_WAIT         => false,
          DIVCLK_DIVIDE        => 1,
          CLKFBOUT_MULT_F      => 8.000,     -- 125 MHz x 8.
          CLKFBOUT_PHASE       => 0.000,
          CLKFBOUT_USE_FINE_PS => false,

          CLKOUT0_DIVIDE_F    => 16.000,     -- 62.5 MHz sys clock
          CLKOUT0_PHASE       => 0.000,
          CLKOUT0_DUTY_CYCLE  => 0.500,
          CLKOUT0_USE_FINE_PS => false,

          CLKIN1_PERIOD => 8.000,            -- 8 ns means 125 MHz
          REF_JITTER1   => 0.010)
        port map (
          -- Output clocks
          CLKFBOUT     => clk_sys_fb,
          CLKOUT0      => clk_sys,
          -- Input clock control
          CLKFBIN      => clk_sys_fb,
          CLKIN1       => clk_125m_pllref_buf,
          CLKIN2       => '0',
          -- Tied to always select the primary input clock
          CLKINSEL     => '1',
          -- Ports for dynamic reconfiguration
          DADDR        => (others => '0'),
          DCLK         => '0',
          DEN          => '0',
          DI           => (others => '0'),
          DO           => open,
          DRDY         => open,
          DWE          => '0',
          -- Ports for dynamic phase shift
          PSCLK        => '0',
          PSEN         => '0',
          PSINCDEC     => '0',
          PSDONE       => open,
          -- Other control and status signals
          LOCKED       => pll_sys_locked,
          CLKINSTOPPED => open,
          CLKFBSTOPPED => open,
          PWRDWN       => '0',
          RST          => pll_arst);

      -- System PLL output clock buffer
      cmp_clk_sys_buf_o : BUFG
      port map (
        I => clk_sys,
        O => clk_sys_out);

      clk_62m5_sys_o <= clk_sys_out;
--      pll_locked_o   <= pll_dmtd_locked and pll_sys_locked;
      pll_locked_o   <= pll_sys_locked;

--      -- DMTD PLL (20 MHz -> ~62,5 MHz)
--      cmp_dmtd_clk_pll : MMCME2_ADV
--        generic map (
--          BANDWIDTH            => "OPTIMIZED",
--          CLKOUT4_CASCADE      => false,
--          COMPENSATION         => "ZHOLD",
--          STARTUP_WAIT         => false,
--          DIVCLK_DIVIDE        => 1,
--          CLKFBOUT_MULT_F      => 50.000,    -- 20 MHz -> 1 GHz
--          CLKFBOUT_PHASE       => 0.000,
--          CLKFBOUT_USE_FINE_PS => false,
--          CLKOUT0_DIVIDE_F     => 16.000,    -- 1GHz/16 -> 62.5 MHz
--          CLKOUT0_PHASE        => 0.000,
--          CLKOUT0_DUTY_CYCLE   => 0.500,
--          CLKOUT0_USE_FINE_PS  => false,
--          CLKOUT1_DIVIDE       => 16,        -- 1GHz/16 -> 62.5 MHz
--          CLKOUT1_PHASE        => 0.000,
--          CLKOUT1_DUTY_CYCLE   => 0.500,
--          CLKOUT1_USE_FINE_PS  => false,
--          CLKIN1_PERIOD        => 50.000,    -- 50ns for 20 MHz
--          REF_JITTER1          => 0.010)
--        port map (
--          -- Output clocks
--          CLKFBOUT     => clk_dmtd_fb,
--          CLKOUT0      => clk_dmtd,
--          -- Input clock control
--          CLKFBIN      => clk_dmtd_fb,
--          CLKIN1       => clk_20m_vcxo_buf,
--          CLKIN2       => '0',
--          -- Tied to always select the primary input clock
--          CLKINSEL     => '1',
--          -- Ports for dynamic reconfiguration
--          DADDR        => (others => '0'),
--          DCLK         => '0',
--          DEN          => '0',
--          DI           => (others => '0'),
--          DO           => open,
--          DRDY         => open,
--          DWE          => '0',
--          -- Ports for dynamic phase shift
--          PSCLK        => '0',
--          PSEN         => '0',
--          PSINCDEC     => '0',
--          PSDONE       => open,
--          -- Other control and status signals
--          LOCKED       => pll_dmtd_locked,
--          CLKINSTOPPED => open,
--          CLKFBSTOPPED => open,
--          PWRDWN       => '0',
--          RST          => pll_arst);
--
--      -- DMTD PLL input clock buffer
--      cmp_clk_dmtd_buf_i : BUFG
--        port map (
--          O => clk_20m_vcxo_buf,
--          I => clk_20m_vcxo_i);
--

      -- DMTD Div2 (124.9920 MHz -> 62,496 MHz)
      process(clk_125m_dmtd_i)
      begin
        if rising_edge(clk_125m_dmtd_i) then
          clk_dmtd <= not clk_dmtd;
        end if;
      end process;

      -- DMTD PLL output clock buffer
      cmp_clk_dmtd_buf_o : BUFG
        port map (
          O => clk_62m5_dmtd_o,
          I => clk_dmtd);

      -- External 10MHz reference PLL for Artix7
      gen_artix7_ext_ref_pll : if (g_with_external_clock_input = TRUE) generate
        
        signal clk_ext_fbi : std_logic;
        signal clk_ext_fbo : std_logic;
        signal clk_ext_buf : std_logic;
        signal clk_ext_mul : std_logic;
        signal pll_ext_rst : std_logic;

      begin
        mmcm_adv_inst : MMCME2_ADV
          generic map (
            BANDWIDTH            => "OPTIMIZED",
            CLKOUT4_CASCADE      => FALSE,
            COMPENSATION         => "ZHOLD",
            STARTUP_WAIT         => FALSE,
            DIVCLK_DIVIDE        => 1,
            CLKFBOUT_MULT_F      => 62.500,
            CLKFBOUT_PHASE       => 0.000,
            CLKFBOUT_USE_FINE_PS => FALSE,
            CLKOUT0_DIVIDE_F     => 10.000,
            CLKOUT0_PHASE        => 0.000,
            CLKOUT0_DUTY_CYCLE   => 0.500,
            CLKOUT0_USE_FINE_PS  => FALSE,
            CLKIN1_PERIOD        => 100.000,
            REF_JITTER1          => 0.005)
          port map (
            -- Output clocks
            CLKFBOUT  => clk_ext_fbo,
            CLKOUT0   => clk_ext_mul,
            -- Input clock control
            CLKFBIN   => clk_ext_fbi,
            CLKIN1    => clk_ext_buf,
            CLKIN2    => '0',
            -- Tied to always select the primary input clock
            CLKINSEL  => '1',
            -- Ports for dynamic reconfiguration
            DADDR     => (others => '0'),
            DCLK      => '0',
            DEN       => '0',
            DI        => (others => '0'),
            DO        => open,
            DRDY      => open,
            DWE       => '0',
            -- Ports for dynamic phase shift
            PSCLK     => '0',
            PSEN      => '0',
            PSINCDEC  => '0',
            PSDONE    => open, -- Other control and status signals
            LOCKED    => ext_ref_mul_locked_o,
            CLKINSTOPPED => ext_ref_mul_stopped_o,
            CLKFBSTOPPED => open,
            PWRDWN   => '0',
            RST      => pll_ext_rst);

        -- External reference input buffer
        cmp_clk_ext_buf_i : BUFG
          port map (
            O => clk_ext_buf,
            I => clk_10m_ext_i);

        clk_10m_ext_o <= clk_ext_buf;

        -- External reference feedback buffer
        cmp_clk_ext_buf_fb : BUFG
          port map (
            O => clk_ext_fbi,
            I => clk_ext_fbo);

        -- External reference output buffer
        cmp_clk_ext_buf_o : BUFG
          port map (
            O => ext_ref_mul_o,
            I => clk_ext_mul);

        cmp_extend_ext_reset : gc_extend_pulse
          generic map (
            g_width => 1000)
          port map (
            clk_i      => clk_sys_out,
            rst_n_i    => pll_sys_locked,
            pulse_i    => ext_ref_rst_i,
            extended_o => pll_ext_rst);

      end generate gen_artix7_ext_ref_pll;

    end generate gen_artix7_default_plls;
    ---------------------------------------------------------------------------
    
    gen_no_ext_ref_pll : if (g_with_external_clock_input = FALSE) generate
      clk_10m_ext_o         <= '0';
      ext_ref_mul_o         <= '0';
      ext_ref_mul_locked_o  <= '1';
      ext_ref_mul_stopped_o <= '1';
    end generate gen_no_ext_ref_pll;

  end generate gen_default_plls;

  -- If external PLLs are used, just copy clock inputs to outputs
  gen_custom_plls : if (g_use_default_plls = FALSE) generate

    clk_62m5_sys_o  <= clk_62m5_sys_i;
    clk_62m5_dmtd_o <= clk_62m5_dmtd_i;
    clk_125m_ref_o  <= clk_125m_ref_i;

    pll_locked_o     <= clk_sys_locked_i and clk_dmtd_locked_i;
    clk_ref_locked_o <= clk_ref_locked_i;

    ext_ref_mul_o         <= clk_125m_ext_i;
    ext_ref_mul_locked_o  <= clk_ext_locked_i;
    ext_ref_mul_stopped_o <= clk_ext_stopped_i;

  end generate gen_custom_plls;

  -- always pass ext reference reset input to output, even when not used
  clk_ext_rst_o <= ext_ref_rst_i;

  -----------------------------------------------------------------------------
  -- Transceiver PHY
  -----------------------------------------------------------------------------

  gen_phy_spartan6 : if(g_fpga_family = "spartan6") generate

    signal clk_125m_gtp_buf : std_logic;

    signal ch0_phy8_out, ch1_phy8_out : t_phy_8bits_to_wrc;

    signal ch0_sfp_txn, ch0_sfp_txp : std_logic;
    signal ch1_sfp_txn, ch1_sfp_txp : std_logic;
    signal ch0_sfp_rxn, ch0_sfp_rxp : std_logic;
    signal ch1_sfp_rxn, ch1_sfp_rxp : std_logic;

  begin

    cmp_ibufgds_gtp : IBUFGDS
      generic map (
        DIFF_TERM    => TRUE,
        IBUF_LOW_PWR => TRUE,
        IOSTANDARD   => "DEFAULT")
      port map (
        O  => clk_125m_gtp_buf,
        I  => clk_125m_gtp_p_i,
        IB => clk_125m_gtp_n_i);


    cmp_gtp : wr_gtp_phy_spartan6
      generic map (
        g_simulation => g_simulation,
        g_enable_ch0 => g_gtp_enable_ch0,
        g_enable_ch1 => g_gtp_enable_ch1)
      port map (
        gtp_clk_i          => clk_125m_gtp_buf,
        ch0_ref_clk_i      => clk_125m_pllref_buf,
        ch0_tx_data_i      => phy8_i.tx_data,
        ch0_tx_k_i         => phy8_i.tx_k(0),
        ch0_tx_disparity_o => ch0_phy8_out.tx_disparity,
        ch0_tx_enc_err_o   => ch0_phy8_out.tx_enc_err,
        ch0_rx_data_o      => ch0_phy8_out.rx_data,
        ch0_rx_rbclk_o     => ch0_phy8_out.rx_clk,
        ch0_rx_k_o         => ch0_phy8_out.rx_k(0),
        ch0_rx_enc_err_o   => ch0_phy8_out.rx_enc_err,
        ch0_rx_bitslide_o  => ch0_phy8_out.rx_bitslide,
        ch0_rst_i          => phy8_i.rst,
        ch0_loopen_i       => phy8_i.loopen,
        ch0_loopen_vec_i   => phy8_i.loopen_vec,
        ch0_tx_prbs_sel_i  => phy8_i.tx_prbs_sel,
        ch0_rdy_o          => ch0_phy8_out.rdy,
        ch1_ref_clk_i      => clk_125m_pllref_buf,
        ch1_tx_data_i      => phy8_i.tx_data,
        ch1_tx_k_i         => phy8_i.tx_k(0),
        ch1_tx_disparity_o => ch1_phy8_out.tx_disparity,
        ch1_tx_enc_err_o   => ch1_phy8_out.tx_enc_err,
        ch1_rx_data_o      => ch1_phy8_out.rx_data,
        ch1_rx_rbclk_o     => ch1_phy8_out.rx_clk,
        ch1_rx_k_o         => ch1_phy8_out.rx_k(0),
        ch1_rx_enc_err_o   => ch1_phy8_out.rx_enc_err,
        ch1_rx_bitslide_o  => ch1_phy8_out.rx_bitslide,
        ch1_rst_i          => phy8_i.rst,
        ch1_loopen_i       => phy8_i.loopen,
        ch1_loopen_vec_i   => phy8_i.loopen_vec,
        ch1_tx_prbs_sel_i  => phy8_i.tx_prbs_sel,
        ch1_rdy_o          => ch1_phy8_out.rdy,
        pad_txn0_o         => ch0_sfp_txn,
        pad_txp0_o         => ch0_sfp_txp,
        pad_rxn0_i         => ch0_sfp_rxn,
        pad_rxp0_i         => ch0_sfp_rxp,
        pad_txn1_o         => ch1_sfp_txn,
        pad_txp1_o         => ch1_sfp_txp,
        pad_rxn1_i         => ch1_sfp_rxn,
        pad_rxp1_i         => ch1_sfp_rxp
        );

    gen_gtp_ch0 : if (g_gtp_enable_ch0 = 1) generate
      ch0_phy8_out.ref_clk      <= clk_125m_pllref_buf;
      ch0_phy8_out.sfp_tx_fault <= sfp_tx_fault_i;
      ch0_phy8_out.sfp_los      <= sfp_los_i;
      phy8_o                    <= ch0_phy8_out;
      sfp_txp_o                 <= ch0_sfp_txp;
      sfp_txn_o                 <= ch0_sfp_txn;
      ch0_sfp_rxp               <= sfp_rxp_i;
      ch0_sfp_rxn               <= sfp_rxn_i;
    end generate gen_gtp_ch0;

    gen_gtp_ch1 : if (g_gtp_enable_ch1 = 1) generate
      ch1_phy8_out.ref_clk      <= clk_125m_pllref_buf;
      ch1_phy8_out.sfp_tx_fault <= sfp_tx_fault_i;
      ch1_phy8_out.sfp_los      <= sfp_los_i;
      phy8_o                    <= ch1_phy8_out;
      sfp_txp_o                 <= ch1_sfp_txp;
      sfp_txn_o                 <= ch1_sfp_txn;
      ch1_sfp_rxp               <= sfp_rxp_i;
      ch1_sfp_rxn               <= sfp_rxn_i;
    end generate gen_gtp_ch1;

    sfp_tx_disable_o <= phy8_i.sfp_tx_disable;

    phy16_o <= c_dummy_phy16_to_wrc;

  end generate gen_phy_spartan6;

  ---------------------------------------------------------------------------
  --   Kintex7 PHY
  ---------------------------------------------------------------------------

  gen_phy_kintex7 : if (g_fpga_family = "kintex7") generate

    signal clk_ref          : std_logic;
    signal clk_125m_gtx_buf : std_logic;
    signal clk_ref_locked   : std_logic;

  begin

    gen_ibufgds: if (g_use_ibufgds=true) generate
    begin
      -- Dedicated GTX clock.
      cmp_gtp_dedicated_clk : IBUFDS_GTE2
        generic map(
          CLKCM_CFG    => true,
          CLKRCV_TRST  => true,
          CLKSWING_CFG => "11")
        port map (
          O     => clk_125m_gtx_buf,
          ODIV2 => open,
          CEB   => '0',
          I     => clk_125m_gtp_p_i,
          IB    => clk_125m_gtp_n_i);
    end generate gen_ibufgds;

    gen_no_ibufgds: if (g_use_ibufgds=false) generate
    begin
      clk_125m_gtx_buf <= clk_125m_gtp_p_i;
    end generate gen_no_ibufgds;


    -- System PLL input clock buffer
    cmp_clk_sys_buf_i : BUFG
      port map (
        I => clk_125m_gtx_buf,
        O => clk_125m_pllref_buf);

    cmp_gtx: wr_gtx_phy_family7
      generic map(
        g_simulation => g_simulation)
      port map(
        clk_gtx_i      => clk_125m_gtx_buf,
        tx_out_clk_o   => clk_ref,
        tx_data_i      => phy16_i.tx_data,
        tx_k_i         => phy16_i.tx_k,
        tx_disparity_o => phy16_o.tx_disparity,
        tx_enc_err_o   => phy16_o.tx_enc_err,
        rx_rbclk_o     => phy16_o.rx_clk,
        rx_data_o      => phy16_o.rx_data,
        rx_k_o         => phy16_o.rx_k,
        rx_enc_err_o   => phy16_o.rx_enc_err,
        rx_bitslide_o  => phy16_o.rx_bitslide,
        rst_i          => phy16_i.rst,
        loopen_i       => phy16_i.loopen_vec,
        tx_prbs_sel_i  => phy16_i.tx_prbs_sel,
        rdy_o          => phy16_o.rdy,

        pad_txn_o => sfp_txn_o,
        pad_txp_o => sfp_txp_o,
        pad_rxn_i => sfp_rxn_i,
        pad_rxp_i => sfp_rxp_i,

        tx_locked_o   => clk_ref_locked);

    clk_125m_ref_o       <= clk_ref;
    clk_ref_locked_o     <= clk_ref_locked;
    phy16_o.ref_clk      <= clk_ref;
    phy16_o.sfp_tx_fault <= sfp_tx_fault_i;
    phy16_o.sfp_los      <= sfp_los_i;
    sfp_tx_disable_o     <= phy16_i.sfp_tx_disable;

    phy8_o <= c_dummy_phy8_to_wrc;

  end generate gen_phy_kintex7;

  ---------------------------------------------------------------------------
  --   Artix7 PHY
  ---------------------------------------------------------------------------

  gen_phy_artix7 : if (g_fpga_family = "artix7") generate

    signal clk_ref          : std_logic;
    signal clk_125m_gtp_buf : std_logic;
    signal clk_ref_locked   : std_logic;

  begin

    -- Dedicated GTP clock.
    cmp_gtp_dedicated_clk : IBUFDS_GTE2
      generic map(
        CLKCM_CFG    => true,
        CLKRCV_TRST  => true,
        CLKSWING_CFG => "11")
      port map (
        O     => clk_125m_gtp_buf,
        ODIV2 => open,
        CEB   => '0',
        I     => clk_125m_gtp_p_i,
        IB    => clk_125m_gtp_n_i);

    -- System PLL input clock buffer
    cmp_clk_sys_buf_i : BUFG
      port map (
        I => clk_125m_gtp_buf,
        O => clk_125m_pllref_buf);

    cmp_gtp: wr_gtp_phy_family7
      generic map(
        g_simulation => g_simulation)
      port map(
        clk_gtp_i      => clk_125m_gtp_buf,
        tx_out_clk_o   => clk_ref,
        tx_data_i      => phy16_i.tx_data,
        tx_k_i         => phy16_i.tx_k,
        tx_disparity_o => phy16_o.tx_disparity,
        tx_enc_err_o   => phy16_o.tx_enc_err,
        rx_rbclk_o     => phy16_o.rx_clk,
        rx_data_o      => phy16_o.rx_data,
        rx_k_o         => phy16_o.rx_k,
        rx_enc_err_o   => phy16_o.rx_enc_err,
        rx_bitslide_o  => phy16_o.rx_bitslide,
        rst_i          => phy16_i.rst,
        loopen_i       => phy16_i.loopen_vec,
        tx_prbs_sel_i  => phy16_i.tx_prbs_sel,
        rdy_o          => phy16_o.rdy,

        pad_txn_o => sfp_txn_o,
        pad_txp_o => sfp_txp_o,
        pad_rxn_i => sfp_rxn_i,
        pad_rxp_i => sfp_rxp_i,

        tx_locked_o   => clk_ref_locked);

    clk_125m_ref_o       <= clk_ref;
    clk_ref_locked_o     <= clk_ref_locked;
    phy16_o.ref_clk      <= clk_ref;
    phy16_o.sfp_tx_fault <= sfp_tx_fault_i;
    phy16_o.sfp_los      <= sfp_los_i;
    sfp_tx_disable_o     <= phy16_i.sfp_tx_disable;

    phy8_o <= c_dummy_phy8_to_wrc;

  end generate gen_phy_artix7;

  ---------------------------------------------------------------------------

end architecture rtl;
