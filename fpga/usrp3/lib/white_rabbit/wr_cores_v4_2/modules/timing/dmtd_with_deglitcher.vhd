-------------------------------------------------------------------------------
-- Title      : Digital DMTD Edge Tagger
-- Project    : White Rabbit
-------------------------------------------------------------------------------
-- File       : dmtd_with_deglitcher.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-Co-HT
-- Created    : 2010-02-25
-- Last update: 2014-07-15
-- Platform   : FPGA-generic
-- Standard   : VHDL '93
-------------------------------------------------------------------------------
-- Description: Single-channel DDMTD phase tagger with integrated bit-median
-- deglitcher. Contains a DDMTD detector, which output signal is deglitched and
-- tagged with a counter running in DMTD offset clock domain. Phase tags are
-- generated for each rising edge in DDMTD output with an internal counter
-------------------------------------------------------------------------------
--
-- Copyright (c) 2009 - 2011 CERN
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
-- Revisions  :
-- Date        Version  Author          Description
-- 2009-01-24  1.0      twlostow        Created
-- 2011-18-04  1.1      twlostow        Bit-median type deglitcher, comments
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.NUMERIC_STD.all;

library work;
use work.gencores_pkg.all;

entity dmtd_with_deglitcher is
  generic (
    -- Size of the phase tag counter. Must be big enough to cover at least one
    -- full period of the DDMTD detector output. Given the frequencies of clk_in_i
    -- and clk_dmtd_i are respectively f_in an f_dmtd, it can be calculated with
    -- the following formula:
    -- g_counter_bits = log2(f_in / abs(f_in - f_dmtd)) + 1
    g_counter_bits : natural := 17;
    g_chipscope    : boolean := false;

    -- Divides the inputs by 2 (effectively passing the clock through a flip flop)
    -- before it gets to the DMTD, effectively removing Place&Route warnings
    -- (at the cost of detector bandwidth)
    g_divide_input_by_2 : boolean := false;

    -- reversed mode: samples clk_dmtd with clk_in.
    g_reverse : boolean := false
    );
  port (
    -- resets for different clock domains
    rst_n_dmtdclk_i : in std_logic;
    rst_n_sysclk_i  : in std_logic;

    -- input clock
    clk_in_i : in std_logic;

    -- DMTD sampling clock
    clk_dmtd_i : in std_logic;

    -- system clock
    clk_sys_i : in std_logic;

    -- async counter resync input: resets only the DDMTD state machine and free
    -- running counter, synchronized to clk_dmtd_i
    resync_p_a_i : in std_logic := '0';

    -- [clk_dmtd_i] counter resync output, pulses when free_cntr == 0
    resync_p_o : out std_logic;

    -- [clk_sys_i] starts resynchronization
    resync_start_p_i : in std_logic := '0';

    -- [clk_sys_i] 1: resynchonization done
    resync_done_o : out std_logic;

    -- [clk_dmtd_i] phase shifter enable, HI level shifts the internal counter
    -- forward/backward by 1 clk_dmtd_i cycle, effectively shifting the tag
    -- value by +-1.
    shift_en_i : in std_logic := '0';

    -- [clk_dmtd_i] phase shift direction: 1 - forward, 0 - backward
    shift_dir_i : in std_logic := '0';

    -- DMTD clock enable, active high. Can be used to reduce the DMTD sampling
    -- frequency - for example, two 10 MHz signals cannot be sampled directly
    -- with a 125 MHz clock, but it's possible with a 5 MHz reference, obtained
    -- by asserting clk_dmtd_en_i every 25 clk_dmtd_i cycles.

    clk_dmtd_en_i : in std_logic := '1';

    -- [clk_dmtd_i] deglitcher threshold
    deglitch_threshold_i : in std_logic_vector(15 downto 0);

    -- [clk_dmtd_i] raw DDMTD output (for debugging purposes)
    dbg_dmtdout_o : out std_logic;

    -- [clk_sys_i] deglitched edge tag value
    tag_o : out std_logic_vector(g_counter_bits-1 downto 0);

    -- [clk_sys_i] pulse indicates new phase tag on tag_o
    tag_stb_p1_o : out std_logic;
    dbg_clk_d3_o : out std_logic
    );

end dmtd_with_deglitcher;

architecture rtl of dmtd_with_deglitcher is

  type t_state is (WAIT_STABLE_0, WAIT_EDGE, GOT_EDGE);

  signal state : t_state;

  signal stab_cntr : unsigned(15 downto 0);
  signal free_cntr : unsigned(g_counter_bits-1 downto 0);

  signal s_one        : std_logic;

  signal clk_in                                           : std_logic;
  signal clk_i_d0, clk_i_d1, clk_i_d2, clk_i_d3, clk_i_dx : std_logic;

  attribute keep             : string;
  attribute keep of clk_in   : signal is "true";
  attribute keep of clk_i_d0 : signal is "true";
  attribute keep of clk_i_d1 : signal is "true";
  attribute keep of clk_i_d2 : signal is "true";
  attribute keep of clk_i_d3 : signal is "true";

  signal new_edge_sreg : std_logic_vector(5 downto 0);
  signal new_edge_p    : std_logic;

  signal tag_int       : unsigned(g_counter_bits-1 downto 0);
  signal resync_p_dmtd : std_logic;

  signal resync_start_p_dmtd, resync_done_dmtd, resync_p_int : std_logic;
  
begin  -- rtl

  U_Sync_Resync_Pulse : gc_sync_ffs
    generic map (
      g_sync_edge => "positive")
    port map (
      clk_i    => clk_dmtd_i,
      rst_n_i  => rst_n_dmtdclk_i,
      data_i   => resync_p_a_i,
      synced_o => resync_p_dmtd);

  U_Sync_Start_Pulse : gc_pulse_synchronizer
    port map (
      clk_in_i  => clk_sys_i,
      clk_out_i => clk_dmtd_i,
      rst_n_i   => rst_n_dmtdclk_i,
      d_ready_o => open,
      d_p_i     => resync_start_p_i,
      q_p_o     => resync_start_p_dmtd);

  U_Sync_Resync_Done : gc_sync_ffs
    generic map (
      g_sync_edge => "positive")
    port map (
      clk_i    => clk_sys_i,
      rst_n_i  => rst_n_sysclk_i,
      data_i   => resync_done_dmtd,
      synced_o => resync_done_o);


  gen_straight : if(g_reverse = false) generate
    
    gen_input_div2 : if(g_divide_input_by_2 = true) generate
      p_divide_input_clock : process(clk_in_i, rst_n_sysclk_i)
      begin
        if rst_n_sysclk_i = '0' then
          clk_in <= '0';
        elsif rising_edge(clk_in_i) then
          clk_in <= not clk_in;
        end if;
      end process;
    end generate gen_input_div2;

    gen_input_straight : if(g_divide_input_by_2 = false) generate
      clk_in <= clk_in_i;
    end generate gen_input_straight;

    p_the_dmtd_itself : process(clk_dmtd_i)
    begin
      if rising_edge(clk_dmtd_i) then
        clk_i_d0 <= clk_in;
        clk_i_d1 <= clk_i_d0;
        clk_i_d2 <= clk_i_d1;
        clk_i_d3 <= clk_i_d2;
      end if;
    end process;

  end generate gen_straight;

  gen_reverse : if(g_reverse = true) generate

    assert (not g_divide_input_by_2) report "dmtd_with_deglitcher: g_reverse implies g_divide_input_by_2 == false" severity failure;

    clk_in <= clk_in_i;

    p_the_dmtd_itself : process(clk_in)
    begin
      if rising_edge(clk_in) then
        clk_i_d0 <= clk_dmtd_i;
        clk_i_d1 <= clk_i_d0;
      end if;
    end process;

    p_sync : process(clk_dmtd_i)
    begin
      if rising_edge(clk_dmtd_i) then
        clk_i_dx <= clk_i_d1;
        clk_i_d2 <= not clk_i_dx;
        clk_i_d3 <= clk_i_d2;
      end if;
    end process;

  end generate gen_reverse;

-- glitchproof DMTD output edge detection
  p_deglitch : process (clk_dmtd_i)
  begin  -- process deglitch

    if rising_edge(clk_dmtd_i) then     -- rising clock edge

      if (rst_n_dmtdclk_i = '0' or (resync_p_dmtd = '1' and resync_done_dmtd = '0')) then  -- synchronous reset (active low)
        stab_cntr     <= (others => '0');
        state         <= WAIT_STABLE_0;
        free_cntr     <= (others => '0');
        new_edge_sreg <= (others => '0');
      elsif(clk_dmtd_en_i = '1') then
        
        if (shift_en_i = '0') then      -- phase shifter
          free_cntr <= free_cntr + 1;
        elsif (shift_dir_i = '1') then
          free_cntr <= free_cntr + 2;
        end if;

        case state is
          when WAIT_STABLE_0 =>         -- out-of-sync
            new_edge_sreg <= '0' & new_edge_sreg(new_edge_sreg'length-1 downto 1);

            if clk_i_d3 /= '0' then
              stab_cntr <= (others => '0');
            else
              stab_cntr <= stab_cntr + 1;
            end if;

            -- DMTD output stable counter hit the LOW level threshold?
            if stab_cntr = unsigned(deglitch_threshold_i) then
              state <= WAIT_EDGE;
            end if;

          when WAIT_EDGE =>
            if (clk_i_d3 /= '0') then   -- got a glitch?
              state     <= GOT_EDGE;
              tag_int   <= free_cntr;
              stab_cntr <= (others => '0');
            end if;

          when GOT_EDGE =>
            if (clk_i_d3 = '0') then
              tag_int <= tag_int + 1;
            end if;

            if stab_cntr = unsigned(deglitch_threshold_i) then
              state         <= WAIT_STABLE_0;
              tag_o         <= std_logic_vector(tag_int);
              new_edge_sreg <= (others => '1');
              stab_cntr     <= (others => '0');
            elsif (clk_i_d3 = '0') then
              stab_cntr <= (others => '0');
            else
              stab_cntr <= stab_cntr + 1;
            end if;

            
        end case;
      end if;
    end if;
  end process p_deglitch;

  p_resync_pulse_output : process(clk_dmtd_i)
  begin
    if rising_edge(clk_dmtd_i) then
      if(unsigned(free_cntr(free_cntr'length-1 downto 3)) = 0) then
        resync_p_o <= '1';
      else
        resync_p_o <= '0';
      end if;
    end if;
  end process;

  p_resync_pulse_trigger : process(clk_dmtd_i)
  begin
    if rising_edge(clk_dmtd_i) then
      if rst_n_dmtdclk_i = '0' then
        resync_done_dmtd <= '1';
      else
        if(resync_start_p_dmtd = '1') then
          resync_done_dmtd <= '0';
        elsif(resync_p_dmtd = '1') then
          resync_done_dmtd <= '1';
        end if;
      end if;
    end if;
  end process;

  U_sync_tag_strobe : gc_sync_ffs
    generic map (
      g_sync_edge => "positive")
    port map (
      clk_i    => clk_sys_i,
      rst_n_i  => rst_n_sysclk_i,
      data_i   => new_edge_sreg(0),
      synced_o => open,
      npulse_o => open,
      ppulse_o => new_edge_p);

  tag_stb_p1_o <= new_edge_p;

  U_Extend_Debug_Pulses : gc_extend_pulse
    generic map (
      g_width => 3000)
    port map (
      clk_i      => clk_sys_i,
      rst_n_i    => rst_n_sysclk_i,
      pulse_i    => new_edge_p,
      extended_o => dbg_dmtdout_o);

	dbg_clk_d3_o <= clk_i_d3;

end rtl;
