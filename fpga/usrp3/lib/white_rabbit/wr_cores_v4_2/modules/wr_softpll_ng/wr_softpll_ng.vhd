-------------------------------------------------------------------------------
-- Title      : White Rabbit Softcore PLL (new generation) - SoftPLL-ng
-- Project    : White Rabbit
-------------------------------------------------------------------------------
-- File       : wr_softpll_ng.vhd
-- Author     : Tomasz Włostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2011-01-29
-- Last update: 2017-02-20
-- Platform   : FPGA-generic
-- Standard   : VHDL'93
-------------------------------------------------------------------------------
-- Description: 
--
-- The hardware part of the revised softcore PLL. Incorporates a user-defined
-- number of DDMTD taggers, a FIFO allowing for sequential readout of
-- the phase tags and ports for driving oscillator tuning DACs.
-- The rest of the magic is done in the software.
-------------------------------------------------------------------------------
--
-- Copyright (c) 2012-2017 CERN
--
-- This source file is free software; you can redistribute it   
-- and/or modify it under the terms of the GNU Lesser General   
-- Public License as published by the Free Software Foundation; 
-- either version 2.1 of the License, or (at your option) any   
-- later version.                                               
--
-- This source is distributed in the hope that it will be       
-- useful, but WITHOUT ANY WARRANTY; without even the implied   
-- warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR      
-- PURPOSE.  See the GNU Lesser General Public License for more 
-- details.                                                     
--
-- You should have received a copy of the GNU Lesser General    
-- Public License along with this source; if not, download it   
-- from http://www.gnu.org/licenses/lgpl-2.1.html
--
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

use work.gencores_pkg.all;
use work.wishbone_pkg.all;
use work.softpll_pkg.all;
use work.spll_wbgen2_pkg.all;

entity wr_softpll_ng is
  generic(
-- Number of bits in phase tags produced by DDMTDs.
-- Must be large enough to cover at least a hundred of DDMTD periods to ensure
-- correct operation of the SoftPLL software servo algorithm - that
-- means, for a typical DMTD frequency offset N=16384, there number of tag bits
-- should be log2(N) + 7 == 21. Note: the value must match the TAG_BITS constant
-- in spll_defs.h file!
    g_tag_bits : integer;

-- These two are obvious:
    g_num_ref_inputs : integer := 1;
    g_num_outputs    : integer := 1;

-- When true, an additional FIFO is instantiated, providing a realtime record
-- of user-selectable SoftPLL parameters (e.g. tag values, phase error, DAC drive).
-- These values can be read by "spll_dbg_proxy" daemon for further analysis.
    g_with_debug_fifo : boolean := false;

-- When true, an additional accumulating bang-bang phase detector is instantiated
-- for wideband locking of the local oscillator to an external stable reference
-- (e.g. GPSDO/Cesium 10 MHz)
    g_with_ext_clock_input : boolean := false;

-- When true, DDMTD inputs are reversed (so that the DDMTD offset clocks is
-- being sampled by the measured clock). This is functionally equivalent to
-- "direct" operation, but may improve FPGA timing/routability.
    g_reverse_dmtds : boolean := true;

-- Divides the DDMTD clock inputs by 2, removing the "CLOCK_DEDICATED_ROUTE"
-- errors under ISE tools, at the cost of bandwidth reduction. Advanced option
-- use with care.
    g_divide_input_by_2 : boolean := false;

    g_ref_clock_rate : integer := 125000000;
    g_ext_clock_rate : integer := 10000000;


    g_interface_mode      : t_wishbone_interface_mode      := PIPELINED;
    g_address_granularity : t_wishbone_address_granularity := WORD
    );

  port(
    clk_sys_i    : in std_logic;
    rst_sys_n_i  : in std_logic;
    rst_ref_n_i  : in std_logic;
    rst_ext_n_i  : in std_logic;
    rst_dmtd_n_i : in std_logic;

-- Reference inputs (i.e. the RX clocks recovered by the PHYs)
    clk_ref_i : in std_logic_vector(g_num_ref_inputs-1 downto 0);

-- Feedback clocks (i.e. the outputs of the main or auxillary oscillator)
-- Note: clk_fb_i(0) must be always connected to the primary board's oscillator
-- (i.e. the one driving the PTP and Ethernet PHY) to ensure correct operation
-- of the PTP core.
    clk_fb_i : in std_logic_vector(g_num_outputs-1 downto 0);

-- DMTD Offset clock
    clk_dmtd_i : in std_logic;

-- External reference clock (e.g. 10 MHz from Cesium/GPSDO). Used only if
-- g_with_ext_clock_input == true
    clk_ext_i : in std_logic;

-- External clock, multiplied to 125 MHz using the FPGA's PLL
    clk_ext_mul_i : in std_logic;
    clk_ext_mul_locked_i : in std_logic := '1';
    clk_ext_stopped_i : in std_logic := '0';
    clk_ext_rst_o : out std_logic;

-- External clock sync/alignment singnal. SoftPLL will align clk_ext_i/clk_fb_i(0)
-- to match the edges immediately following the rising edge in sync_p_i.
    pps_csync_p1_i : in std_logic;
    pps_ext_a_i    : in std_logic;

-- DMTD oscillator drive
    dac_dmtd_data_o : out std_logic_vector(15 downto 0);
-- When HI, load the data from dac_dmtd_data_o to the DAC.
    dac_dmtd_load_o : out std_logic;

-- Output channel DAC value
    dac_out_data_o : out std_logic_vector(15 downto 0);
-- Output channel select (0 = Output channel 0, 1 == OC 1, etc...)
    dac_out_sel_o  : out std_logic_vector(3 downto 0);
    dac_out_load_o : out std_logic;

-- Output enable input: when HI, enables locking the output(s)
-- to the reference clock(s)
    out_enable_i : in  std_logic_vector(g_num_outputs-1 downto 0);
-- When HI, the respective clock output is locked.
    out_locked_o : out std_logic_vector(g_num_outputs-1 downto 0);

    wb_adr_i   : in  std_logic_vector(c_wishbone_address_width-1 downto 0);
    wb_dat_i   : in  std_logic_vector(c_wishbone_data_width-1 downto 0);
    wb_dat_o   : out std_logic_vector(c_wishbone_data_width-1 downto 0);
    wb_cyc_i   : in  std_logic;
    wb_sel_i   : in  std_logic_vector(c_wishbone_data_width/8-1 downto 0);
    wb_stb_i   : in  std_logic;
    wb_we_i    : in  std_logic;
    wb_ack_o   : out std_logic;
    wb_stall_o : out std_logic;
    wb_irq_o   : out std_logic;
    debug_o    : out std_logic_vector(5 downto 0);

-- Debug FIFO readout interrupt
    dbg_fifo_irq_o : out std_logic
    );

end wr_softpll_ng;

architecture rtl of wr_softpll_ng is

  alias rst_n_i : std_logic is rst_sys_n_i;

  constant c_log2_replication : integer := 2;
  constant c_use_multi_dmtd   : boolean := false;

  constant c_DBG_FIFO_THRESHOLD : integer := 8180;
  constant c_DBG_FIFO_COALESCE  : integer := 100;
  constant c_BB_ERROR_BITS      : integer := 16;

  component dmtd_with_deglitcher
    generic (
      g_counter_bits      : natural;
      g_divide_input_by_2 : boolean;
			g_reverse           :	boolean);
    port (
      rst_n_dmtdclk_i      : in  std_logic;
      rst_n_sysclk_i       : in  std_logic;
      clk_in_i             : in  std_logic;
      clk_dmtd_i           : in  std_logic;
      clk_sys_i            : in  std_logic;
      resync_p_a_i         : in  std_logic := '0';
      resync_p_o           : out std_logic;
      resync_start_p_i     : in  std_logic;
      resync_done_o        : out std_logic;
      shift_en_i           : in  std_logic;
      shift_dir_i          : in  std_logic;
      clk_dmtd_en_i        : in  std_logic := '1';
      deglitch_threshold_i : in  std_logic_vector(15 downto 0);
      dbg_dmtdout_o        : out std_logic;
      tag_o                : out std_logic_vector(g_counter_bits-1 downto 0);
      tag_stb_p1_o         : out std_logic;
			dbg_clk_d3_o         : out std_logic);
  end component;

  component spll_wb_slave
    generic (
      g_with_debug_fifo : integer);
    port (
      rst_n_i    : in  std_logic;
      clk_sys_i  : in  std_logic;
      wb_adr_i   : in  std_logic_vector(5 downto 0);
      wb_dat_i   : in  std_logic_vector(31 downto 0);
      wb_dat_o   : out std_logic_vector(31 downto 0);
      wb_cyc_i   : in  std_logic;
      wb_sel_i   : in  std_logic_vector(3 downto 0);
      wb_stb_i   : in  std_logic;
      wb_we_i    : in  std_logic;
      wb_ack_o   : out std_logic;
      wb_stall_o : out std_logic;
      wb_int_o   : out std_logic;
      irq_tag_i  : in  std_logic;
      regs_i     : in  t_spll_in_registers;
      regs_o     : out t_spll_out_registers);
  end component;

  component spll_aligner
    generic (
      g_counter_width  : integer;
      g_ref_clock_rate : integer;
      g_in_clock_rate  : integer;
      g_sample_rate    : integer);
    port (
      clk_sys_i      : in  std_logic;
      clk_in_i       : in  std_logic;
      clk_ref_i      : in  std_logic;
      rst_n_sys_i    : in  std_logic;
      rst_n_ref_i    : in  std_logic;
      rst_n_ext_i    : in  std_logic;
      pps_ext_a_i    : in  std_logic;
      pps_csync_p1_i : in  std_logic;
      sample_cref_o  : out std_logic_vector(g_counter_width-1 downto 0);
      sample_cin_o   : out std_logic_vector(g_counter_width-1 downto 0);
      sample_valid_o : out std_logic;
      sample_ack_i   : in  std_logic);
  end component;
  function f_num_total_channels
    return integer is
  begin
    if(g_with_ext_clock_input) then
      return g_num_ref_inputs + g_num_outputs + 1;
    else
      return g_num_ref_inputs + g_num_outputs;
    end if;
  end f_num_total_channels;

  function f_pick (
    cond     : boolean;
    if_true  : std_logic;
    if_false : std_logic
    ) return std_logic is
  begin
    if(cond) then
      return if_true;
    else
      return if_false;
    end if;
  end f_pick;

  function f_pick (
    cond     : boolean;
    if_true  : integer;
    if_false : integer
    ) return integer is
  begin
    if(cond) then
      return if_true;
    else
      return if_false;
    end if;
  end f_pick;

  function resize(x : std_logic_vector; new_length : integer) return std_logic_vector is
    variable tmp : std_logic_vector(new_length-1 downto 0);
  begin
    tmp                       := (others => '0');
    tmp (x'length-1 downto 0) := x;
    return tmp;
  end resize;

  type t_tag_array is array (0 to f_num_total_channels-1) of std_logic_vector(g_tag_bits-1 downto 0);

  type t_phase_error_array is array(0 to g_num_outputs-1) of std_logic_vector(c_BB_ERROR_BITS-1 downto 0);

  signal tags, tags_masked                          : t_tag_array;
  signal tags_grant_p, tags_p, tags_req, tags_grant : std_logic_vector(f_num_total_channels-1 downto 0);
  signal tag_muxed                                  : std_logic_vector(g_tag_bits-1 downto 0);
  signal tag_src, tag_src_pre                       : std_logic_vector (5 downto 0);
  signal tag_valid, tag_valid_pre                   : std_logic;

  signal deglitch_thr_slv : std_logic_vector(15 downto 0);

  signal irq_tag : std_logic;

  signal rcer_int : std_logic_vector(g_num_ref_inputs-1 downto 0);
  signal ocer_int : std_logic_vector(g_num_outputs-1 downto 0);

  signal wb_irq_out : std_logic;

  signal wb_out   : t_wishbone_slave_out;
  signal wb_in    : t_wishbone_slave_in;
  signal regs_in  : t_SPLL_out_registers;
  signal regs_out : t_SPLL_in_registers;

  -- Debug FIFO signals
  signal dbg_fifo_almostfull   : std_logic;
  signal dbg_seq_id            : unsigned(15 downto 0);
  signal dbg_fifo_permit_write : std_logic;
  signal dbg_fifo_irq          : std_logic := '0';

  -- Temporary vectors for DDMTD clock selection (straight/reversed)
  signal dmtd_ref_clk_in, dmtd_ref_clk_dmtd : std_logic_vector(g_num_ref_inputs-1 downto 0);
  signal rst_n_dmtd_ref_clk                 : std_logic_vector(g_num_ref_inputs-1 downto 0);

  signal dmtd_fb_clk_in, dmtd_fb_clk_dmtd : std_logic_vector(g_num_outputs-1 downto 0);
  signal rst_n_dmtd_fb_clk                : std_logic_vector(g_num_outputs-1 downto 0);

  signal ext_ref_present : std_logic;
  signal fb_resync_out   : std_logic_vector(g_num_outputs-1 downto 0);

  signal ref_resync_start_p : std_logic_vector(31 downto 0);
  signal fb_resync_start_p  : std_logic_vector(15 downto 0);

  type t_aligner_sample_array is array(0 to g_num_outputs) of std_logic_vector(27 downto 0);

  signal aligner_sample_valid, aligner_sample_ack : std_logic_vector(g_num_outputs downto 0);
  signal aligner_sample_cref, aligner_sample_cin  : t_aligner_sample_array;
  
begin  -- rtl

  U_Adapter : wb_slave_adapter
    generic map(
      g_master_use_struct  => true,
      g_master_mode        => CLASSIC,
      g_master_granularity => WORD,
      g_slave_use_struct   => false,
      g_slave_mode         => g_interface_mode,
      g_slave_granularity  => g_address_granularity)
    port map (
      clk_sys_i  => clk_sys_i,
      rst_n_i    => rst_n_i,
      master_i   => wb_out,
      master_o   => wb_in,
      sl_adr_i   => wb_adr_i,
      sl_dat_i   => wb_dat_i,
      sl_sel_i   => wb_sel_i,
      sl_cyc_i   => wb_cyc_i,
      sl_stb_i   => wb_stb_i,
      sl_we_i    => wb_we_i,
      sl_dat_o   => wb_dat_o,
      sl_ack_o   => wb_ack_o,
      sl_stall_o => wb_stall_o);

  U_Meas_DMTD_Freq: gc_frequency_meter
    generic map (
      g_with_internal_timebase => false,
      g_clk_sys_freq           => 1,
      g_counter_bits           => 28)
    port map (
      clk_sys_i    => clk_sys_i,
      clk_in_i     => clk_dmtd_i,
      rst_n_i      => rst_n_i,
      pps_p1_i     => pps_ext_a_i,
      freq_o       => regs_out.f_dmtd_freq_i,
      freq_valid_o => open);            -- fixme

  U_Meas_REF_Freq: gc_frequency_meter
    generic map (
      g_with_internal_timebase => false,
      g_clk_sys_freq           => 1,
      g_counter_bits           => 28)
    port map (
      clk_sys_i    => clk_sys_i,
      clk_in_i     => clk_fb_i(0),
      rst_n_i      => rst_n_i,
      pps_p1_i     => pps_ext_a_i,
      freq_o       => regs_out.f_ref_freq_i,
      freq_valid_o => open);            -- fixme

  U_Meas_EXT_Freq: gc_frequency_meter
    generic map (
      g_with_internal_timebase => false,
      g_clk_sys_freq           => 1,
      g_counter_bits           => 28)
    port map (
      clk_sys_i    => clk_sys_i,
      clk_in_i     => clk_ext_i,
      rst_n_i      => rst_n_i,
      pps_p1_i     => pps_ext_a_i,
      freq_o       => regs_out.f_ext_freq_i,
      freq_valid_o => open);            -- fixme
  

  gen_ref_dmtds : for i in 0 to g_num_ref_inputs-1 generate

    DMTD_REF : dmtd_with_deglitcher
      generic map (
        g_counter_bits      => g_tag_bits,
        g_divide_input_by_2 => g_divide_input_by_2,
        g_reverse	    => g_reverse_dmtds)
      port map (
        rst_n_dmtdclk_i => rst_dmtd_n_i,
        rst_n_sysclk_i  => rst_n_i,

        clk_dmtd_i    => clk_dmtd_i,
        clk_dmtd_en_i => '1',

        clk_sys_i => clk_sys_i,
        clk_in_i  => clk_ref_i(i),

        resync_done_o    => open,
        resync_start_p_i => '0',
        resync_p_a_i     => fb_resync_out(0),
        resync_p_o       => open,

        tag_o                => tags(i),
        tag_stb_p1_o         => tags_p(i),
        shift_en_i           => '0',
        shift_dir_i          => '0',
        deglitch_threshold_i => deglitch_thr_slv,
        dbg_dmtdout_o        => open);


  end generate gen_ref_dmtds;

  gen_feedback_dmtds : for i in 0 to g_num_outputs-1 generate
    
    DMTD_FB : dmtd_with_deglitcher
      generic map (
        g_counter_bits      => g_tag_bits,
        g_divide_input_by_2 => g_divide_input_by_2,
        g_reverse           => g_reverse_dmtds)
      port map (
        rst_n_dmtdclk_i => rst_dmtd_n_i,
        rst_n_sysclk_i  => rst_n_i,

        clk_dmtd_i    => clk_dmtd_i,
        clk_dmtd_en_i => '1',

        clk_sys_i => clk_sys_i,
        clk_in_i  => clk_fb_i(i),

        resync_done_o    => open,
        resync_start_p_i => '0',
        resync_p_a_i     => fb_resync_out(0),
        resync_p_o       => fb_resync_out(i),

        tag_o        => tags(i+g_num_ref_inputs),
        tag_stb_p1_o => tags_p(i+g_num_ref_inputs),
        shift_en_i   => '0',
        shift_dir_i  => '0',

        deglitch_threshold_i => deglitch_thr_slv,
        dbg_dmtdout_o        => open,
				dbg_clk_d3_o         => open); --debug_o(4));


  end generate gen_feedback_dmtds;

  -- drive unused debug output
  debug_o(4) <= '0';

  gen_with_ext_clock_input : if(g_with_ext_clock_input) generate

    debug_o(0) <= fb_resync_out(0);
    debug_o(1) <= tags_p(g_num_ref_inputs + g_num_outputs);
    debug_o(2) <= tags_p(g_num_ref_inputs);
    
    U_DMTD_EXT : dmtd_with_deglitcher
      generic map (
        g_counter_bits      => g_tag_bits,
        g_divide_input_by_2 => g_divide_input_by_2,
        g_reverse	    => g_reverse_dmtds)
      port map (
        rst_n_dmtdclk_i => rst_dmtd_n_i,
        rst_n_sysclk_i  => rst_n_i,
        clk_dmtd_i      => clk_dmtd_i,
        clk_dmtd_en_i   => '1',

        clk_sys_i => clk_sys_i,
        clk_in_i  => clk_ext_mul_i,

        resync_done_o    => open,
        resync_start_p_i => '0',
        resync_p_a_i     => fb_resync_out(0),
        resync_p_o       => open,

        tag_o        => tags(g_num_ref_inputs + g_num_outputs),
        tag_stb_p1_o => tags_p(g_num_ref_inputs + g_num_outputs),
        shift_en_i   => '0',
        shift_dir_i  => '0',

        deglitch_threshold_i => deglitch_thr_slv,
        dbg_dmtdout_o        => debug_o(3),
        dbg_clk_d3_o         => debug_o(5));

    U_Aligner_EXT : spll_aligner
      generic map (
        g_counter_width  => 28,
        g_ref_clock_rate => g_ref_clock_rate,
        g_in_clock_rate  => g_ext_clock_rate,
        g_sample_rate    => 100)
      port map (
        clk_sys_i      => clk_sys_i,
        clk_in_i       => clk_ext_i,
        clk_ref_i      => clk_fb_i(0),
        rst_n_sys_i    => rst_n_i,
        rst_n_ref_i    => rst_ref_n_i,
        rst_n_ext_i    => rst_ext_n_i,
        pps_ext_a_i    => pps_ext_a_i,
        pps_csync_p1_i => pps_csync_p1_i,
        sample_cref_o  => aligner_sample_cref(g_num_outputs),
        sample_cin_o   => aligner_sample_cin(g_num_outputs),
        sample_valid_o => aligner_sample_valid(g_num_outputs),
        sample_ack_i   => aligner_sample_ack(g_num_outputs)
        );

    aligner_sample_valid(g_num_outputs-1 downto 0) <= (others => '0');

    aligner_sample_cref(0 to g_num_outputs-1) <= (others => (others => '0'));
    aligner_sample_cin(0 to g_num_outputs-1)  <= (others => (others => '0'));

    regs_out.eccr_ext_supported_i   <= '1' when g_with_ext_clock_input else '0';
    regs_out.eccr_ext_ref_locked_i  <= clk_ext_mul_locked_i;
    regs_out.eccr_ext_ref_stopped_i <= clk_ext_stopped_i;
    clk_ext_rst_o <= regs_in.eccr_ext_ref_pllrst_o;
  end generate gen_with_ext_clock_input;

  
  gen_without_ext_clock_input : if(not g_with_ext_clock_input) generate
    aligner_sample_valid <= (others => '0');
    aligner_sample_cref  <= (others => (others => '0'));
    aligner_sample_cin   <= (others => (others => '0'));

    regs_out.eccr_ext_supported_i            <= '0';
    regs_out.eccr_ext_ref_locked_i           <= '0';
    regs_out.eccr_ext_ref_stopped_i          <= '0';
    clk_ext_rst_o <= '0';
    -- drive unused debug outputs
    debug_o(0) <= '0';
    debug_o(1) <= '0';
    debug_o(2) <= '0';
    debug_o(3) <= '0';
    debug_o(5) <= '0';
  end generate gen_without_ext_clock_input;

  p_ack_aligner_samples: process(regs_in, aligner_sample_valid)
  begin
    regs_out.al_cr_valid_i <= (others => '0');
    for i in 0 to g_num_outputs loop
      aligner_sample_ack(i)     <= regs_in.al_cr_valid_o(i) and regs_in.al_cr_valid_load_o;
      regs_out.al_cr_valid_i(i) <= aligner_sample_valid(i);
    end loop;  -- i in 0 to g_num_outputs
  end process;

  p_mux_aligner_samples: process(clk_sys_i)
    begin
      if rising_edge(clk_sys_i) then
        for i in 0 to g_num_outputs loop
          if(aligner_sample_ack(i) = '1') then
            regs_out.al_cref_i <= resize( aligner_sample_cref(i), 32 );
            regs_out.al_cin_i <= resize( aligner_sample_cin(i), 32 );
          end if;
        end loop;
      end if;
    end process;

  
  U_WB_SLAVE : spll_wb_slave
    generic map (
      g_with_debug_fifo => f_pick(g_with_debug_fifo, 1, 0))
    port map (
      clk_sys_i  => clk_sys_i,
      rst_n_i    => rst_n_i,
      wb_adr_i   => wb_in.adr(5 downto 0),
      wb_dat_i   => wb_in.dat,
      wb_dat_o   => wb_out.dat,
      wb_cyc_i   => wb_in.cyc,
      wb_sel_i   => wb_in.sel,
      wb_stb_i   => wb_in.stb,
      wb_we_i    => wb_in.we,
      wb_ack_o   => wb_out.ack,
      wb_int_o   => wb_irq_out,
      wb_stall_o => open,

      regs_o => regs_in,
      regs_i => regs_out,

      irq_tag_i => irq_tag);

    -- drive unused outputs
    wb_out.err   <= '0';
    wb_out.rty   <= '0';
    wb_out.stall <= '0';
    wb_out.int   <= '0';

  p_ocer_rcer_regs : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        ocer_int <= (others => '0');
        rcer_int <= (others => '0');
      else
        if(regs_in.ocer_load_o = '1') then
          ocer_int <= regs_in.ocer_o(g_num_outputs -1 downto 0);
        end if;

        if(regs_in.rcer_load_o = '1') then
          rcer_int <= regs_in.rcer_o(g_num_ref_inputs -1 downto 0);
        end if;
      end if;
    end if;
  end process;

  regs_out.ocer_i(g_num_outputs-1 downto 0) <= ocer_int;
  regs_out.ocer_i(7 downto g_num_outputs)   <= (others => '0');

  regs_out.rcer_i(g_num_ref_inputs-1 downto 0) <= rcer_int;
  regs_out.rcer_i(31 downto g_num_ref_inputs)  <= (others => '0');

  p_latch_tags : process(clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if(rst_n_i = '0') then
        tags_req   <= (others => '0');
        tags_grant <= (others => '0');
      else
        f_rr_arbitrate(tags_req, tags_grant, tags_grant);

        for i in 0 to g_num_ref_inputs-1 loop
          if(tags_p(i) = '1') then
            tags_req(i) <= rcer_int(i);
          elsif(tags_grant(i) = '1') then
            tags_req(i) <= '0';
          end if;
        end loop;  -- i

        for i in 0 to g_num_outputs-1 loop
          if(tags_p(i + g_num_ref_inputs) = '1') then
            tags_req(i + g_num_ref_inputs) <= ocer_int(i);
          elsif(tags_grant(i + g_num_ref_inputs) = '1') then
            tags_req(i + g_num_ref_inputs) <= '0';
          end if;
        end loop;  -- i

        if(g_with_ext_clock_input and tags_p(f_num_total_channels-1) = '1') then
          tags_req(f_num_total_channels-1) <= regs_in.eccr_ext_en_o;
        elsif(g_with_ext_clock_input and tags_grant(f_num_total_channels-1) = '1') then
          tags_req(f_num_total_channels-1) <= '0';
        end if;
        
      end if;
    end if;
  end process;

  tags_grant_p <= tags_req and tags_grant;

  p_mux_tags : process(clk_sys_i)
    variable muxed  : std_logic_vector(g_tag_bits-1 downto 0);
    variable src_id : std_logic_vector(5 downto 0);
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        tag_muxed     <= (others => '0');
        tag_src_pre   <= (others => '0');
        tag_src       <= (others => '0');
        tag_valid_pre <= '0';
        tag_valid     <= '0';
      else
        
        for i in 0 to f_num_total_channels-1 loop
          if(tags_grant_p(i) = '1') then
            tags_masked(i) <= tags(i);
          else
            tags_masked(i) <= (others => '0');
          end if;
        end loop;  -- i

        if(unsigned(tags_grant_p) /= 0) then
          tag_valid_pre <= '1';
        else
          tag_valid_pre <= '0';
        end if;

        tag_valid <= tag_valid_pre;

        tag_src_pre <= f_onehot_decode(tags_grant_p, tag_src_pre'length);
        tag_src     <= tag_src_pre;

        muxed := (others => '0');

        for i in 0 to f_num_total_channels-1 loop
          muxed := muxed or tags_masked(i);
        end loop;

        tag_muxed <= muxed;
        
      end if;
    end if;
  end process;

  regs_out.trr_wr_req_i  <= tag_valid and not regs_in.trr_wr_full_o;
  regs_out.trr_chan_id_i <= '0'&tag_src;

  regs_out.trr_value_i(g_tag_bits-1 downto 0) <= tag_muxed;
  regs_out.trr_value_i(23 downto g_tag_bits)  <= (others => '0');
  
  regs_out.occr_out_en_i(g_num_outputs-1 downto 0) <= out_enable_i;
  regs_out.occr_out_en_i(7 downto g_num_outputs)   <= (others => '0');

  out_locked_o <= regs_in.occr_out_lock_o(g_num_outputs-1 downto 0);

  irq_tag <= not regs_in.trr_wr_empty_o;

  deglitch_thr_slv <= regs_in.deglitch_thr_o;



  -----------------------------------------------------------------------------
  -- Debugging FIFO
  -----------------------------------------------------------------------------

  gen_with_debug_fifo : if(g_with_debug_fifo = true) generate
    
    dbg_fifo_almostfull <= '1' when unsigned(regs_in.dfr_host_wr_usedw_o) > 8180 else '0';

    p_request_counter : process(clk_sys_i)
    begin
      if rising_edge(clk_sys_i) then
        if rst_n_i = '0' then
          dbg_seq_id <= (others => '0');
        else
          if(regs_in.dfr_spll_eos_o = '1' and regs_in.dfr_spll_eos_wr_o = '1') then
            dbg_seq_id <= dbg_seq_id + 1;
          end if;
        end if;
      end if;
    end process;

    p_fifo_permit_write : process(clk_sys_i)
    begin
      if rising_edge(clk_sys_i) then
        if rst_n_i = '0' then
          dbg_fifo_permit_write <= '1';
        else
          if(dbg_fifo_almostfull = '0') then
            dbg_fifo_permit_write <= '1';
          elsif(regs_in.dfr_spll_eos_o = '1' and regs_in.dfr_spll_eos_wr_o = '1') then
            dbg_fifo_permit_write <= '0';
          end if;
        end if;
      end if;
    end process;

    p_coalesce_fifo_irq : process(clk_sys_i)
    begin
      if rising_edge(clk_sys_i) then
        if rst_n_i = '0' then
          dbg_fifo_irq <= '0';
        else
          if(unsigned(regs_in.dfr_host_wr_usedw_o) = 0) then
            dbg_fifo_irq <= '0';
          elsif(unsigned(regs_in.dfr_host_wr_usedw_o) = c_DBG_FIFO_COALESCE) then
            dbg_fifo_irq <= '1';
          end if;
        end if;
      end if;
    end process;

    regs_out.dfr_host_wr_req_i <= regs_in.dfr_spll_value_wr_o and dbg_fifo_permit_write;
    regs_out.dfr_host_value_i  <= regs_in.dfr_spll_eos_o & regs_in.dfr_spll_value_o;
    regs_out.dfr_host_seq_id_i <= std_logic_vector(dbg_seq_id);

  end generate gen_with_debug_fifo;

  gen_without_debug_fifo : if(g_with_debug_fifo = false) generate
    dbg_fifo_irq               <= '0';
    regs_out.dfr_host_wr_req_i <= '0';
    regs_out.dfr_host_value_i  <= (others => '0');
    regs_out.dfr_host_seq_id_i <= (others => '0');
  end generate gen_without_debug_fifo;

  dbg_fifo_irq_o <= dbg_fifo_irq;

  -----------------------------------------------------------------------------
  -- CSR N_OUT/N_REF fields
  -----------------------------------------------------------------------------

  regs_out.csr_n_ref_i <= std_logic_vector(to_unsigned(g_num_ref_inputs, regs_out.csr_n_ref_i'length));
  regs_out.csr_n_out_i <= std_logic_vector(to_unsigned(g_num_outputs, regs_out.csr_n_out_i'length));

  dac_dmtd_load_o <= regs_in.dac_hpll_wr_o;
  dac_dmtd_data_o <= regs_in.dac_hpll_o;

  dac_out_data_o <= regs_in.dac_main_value_o;
  dac_out_sel_o  <= regs_in.dac_main_dac_sel_o;
  dac_out_load_o <= regs_in.dac_main_value_wr_o;

  wb_irq_o <= wb_irq_out;

  regs_out.al_cr_required_i    <= (others => '0');
  regs_out.csr_dbg_supported_i <= '0';
  regs_out.f_dmtd_valid_i      <= '0';
  regs_out.f_ref_valid_i       <= '0';
  regs_out.f_ext_valid_i       <= '0';
  regs_out.trr_disc_i          <= '0';

end rtl;
