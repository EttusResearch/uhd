-------------------------------------------------------------------------------
-- Title      : PPS Generator & UTC Realtime clock
-- Project    : WhiteRabbit Switch
-------------------------------------------------------------------------------
-- File       : wr_pps_gen.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN (BE-CO-HT)
-- Created    : 2010-09-02
-- Last update: 2017-02-20
-- Platform   : FPGA-generics
-- Standard   : VHDL
-------------------------------------------------------------------------------
-- Description:
-------------------------------------------------------------------------------
--
-- Copyright (c) 2010-2017 CERN
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
-- 2010-09-02  1.0      twlostow        Created
-- 2011-05-09  1.1      twlostow        Added external PPS input
-- 2011-10-26  1.2      greg.d          Added wb slave adapter
-------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.gencores_pkg.all;
use work.genram_pkg.all;
use work.wishbone_pkg.all;

entity wr_pps_gen is
  generic(
    g_interface_mode       : t_wishbone_interface_mode      := CLASSIC;
    g_address_granularity  : t_wishbone_address_granularity := WORD;
    g_ref_clock_rate       : integer                        := 125000000;
    g_ext_clock_rate       : integer                        := 10000000;
    g_with_ext_clock_input : boolean                        := false
    );
  port (
    clk_ref_i   : in std_logic;
    clk_sys_i   : in std_logic;
    rst_ref_n_i : in std_logic;
    rst_sys_n_i : in std_logic;

    wb_adr_i   : in  std_logic_vector(4 downto 0);
    wb_dat_i   : in  std_logic_vector(31 downto 0);
    wb_dat_o   : out std_logic_vector(31 downto 0);
    wb_cyc_i   : in  std_logic;
    wb_sel_i   : in  std_logic_vector(3 downto 0);
    wb_stb_i   : in  std_logic;
    wb_we_i    : in  std_logic;
    wb_ack_o   : out std_logic;
    wb_stall_o : out std_logic;

    -- Link status indicator (used for fast masking of PPS output when link
    -- goes down
    link_ok_i : in std_logic;

    -- External PPS input. Warning! This signal is treated as synchronous to
    -- the clk_ref_i (or the external 10 MHz reference) to prevent sync chain
    -- delay uncertainities. Setup/hold times must be respected!
    pps_in_i : in std_logic;

    -- Single-pulse PPS output for synchronizing endpoints to
    pps_csync_o : out std_logic;
    pps_out_o   : out std_logic;
    pps_led_o   : out std_logic;

    pps_valid_o : out std_logic;

    tm_utc_o        : out std_logic_vector(39 downto 0);
    tm_cycles_o     : out std_logic_vector(27 downto 0);
    tm_time_valid_o : out std_logic
    );
end wr_pps_gen;

architecture behavioral of wr_pps_gen is

  alias rst_n_i : std_logic is rst_sys_n_i;

  constant c_PERIOD : integer := g_ref_clock_rate;

  component pps_gen_wb is
    port (
      rst_n_i                : in  std_logic;
      clk_sys_i              : in  std_logic;
      wb_adr_i               : in  std_logic_vector(2 downto 0);
      wb_dat_i               : in  std_logic_vector(31 downto 0);
      wb_dat_o               : out std_logic_vector(31 downto 0);
      wb_cyc_i               : in  std_logic;
      wb_sel_i               : in  std_logic_vector(3 downto 0);
      wb_stb_i               : in  std_logic;
      wb_we_i                : in  std_logic;
      wb_ack_o               : out std_logic;
      wb_stall_o             : out std_logic;
      refclk_i               : in  std_logic;
      ppsg_cr_cnt_rst_o      : out std_logic;
      ppsg_cr_cnt_en_o       : out std_logic;
      ppsg_cr_cnt_adj_o      : out std_logic;
      ppsg_cr_cnt_adj_i      : in  std_logic;
      ppsg_cr_cnt_adj_load_o : out std_logic;
      ppsg_cr_cnt_set_o      : out std_logic;
      ppsg_cr_pwidth_o       : out std_logic_vector(27 downto 0);
      ppsg_cntr_nsec_i       : in  std_logic_vector(27 downto 0);
      ppsg_cntr_utclo_i      : in  std_logic_vector(31 downto 0);
      ppsg_cntr_utchi_i      : in  std_logic_vector(7 downto 0);
      ppsg_adj_nsec_o        : out std_logic_vector(27 downto 0);
      ppsg_adj_nsec_wr_o     : out std_logic;
      ppsg_adj_utclo_o       : out std_logic_vector(31 downto 0);
      ppsg_adj_utclo_wr_o    : out std_logic;
      ppsg_adj_utchi_o       : out std_logic_vector(7 downto 0);
      ppsg_adj_utchi_wr_o    : out std_logic;
      ppsg_escr_sync_o       : out std_logic;
      ppsg_escr_sync_i       : in  std_logic;
      ppsg_escr_sync_load_o  : out std_logic;
      ppsg_escr_pps_valid_o  : out std_logic;
      ppsg_escr_tm_valid_o   : out std_logic;
      ppsg_escr_sec_set_o    : out std_logic;
      ppsg_escr_nsec_set_o   : out std_logic;
      ppsg_escr_pps_unmask_o : out std_logic);
  end component pps_gen_wb;

-- Wisbone slave signals
  signal ppsg_cr_cnt_rst : std_logic;
  signal ppsg_cr_cnt_en  : std_logic;

  signal ppsg_cr_cnt_adj_o    : std_logic;
  signal ppsg_cr_cnt_adj_i    : std_logic;
  signal ppsg_cr_cnt_adj_load : std_logic;

  signal ppsg_cr_cnt_set_p : std_logic;
  signal ppsg_cr_pwidth    : std_logic_vector(27 downto 0);

  signal ppsg_cntr_nsec  : std_logic_vector(27 downto 0);
  signal ppsg_cntr_utclo : std_logic_vector(31 downto 0);
  signal ppsg_cntr_utchi : std_logic_vector(7 downto 0);

  signal ppsg_adj_nsec       : std_logic_vector(27 downto 0);
  signal ppsg_adj_nsec_wr    : std_logic;
  signal ppsg_adj_utclo      : std_logic_vector(31 downto 0);
  signal ppsg_adj_utclo_wr   : std_logic;
  signal ppsg_adj_utchi      : std_logic_vector(7 downto 0);
  signal ppsg_adj_utchi_wr   : std_logic;
  signal ppsg_escr_sync_load : std_logic;
  signal ppsg_escr_sync_in   : std_logic;
  signal ppsg_escr_sync_out  : std_logic;
  signal ppsg_escr_sec_set   : std_logic;
  signal ppsg_escr_nsec_set  : std_logic;

  signal ppsg_escr_pps_valid  : std_logic;
  signal ppsg_escr_tm_valid   : std_logic;
  signal ppsg_escr_pps_unmask : std_logic;

  signal cntr_nsec    : unsigned (27 downto 0);
  signal cntr_utc     : unsigned (39 downto 0);
  signal cntr_pps_ext : unsigned (24 downto 0);

  signal ns_overflow     : std_logic;
  signal ns_overflow_adv : std_logic;
  signal cntr_adjust_p   : std_logic;

  signal adj_nsec : unsigned(27 downto 0);
  signal adj_utc  : unsigned(39 downto 0);

  signal adjust_in_progress_nsec : std_logic;

  signal adjust_in_progress_utc : std_logic;

  signal width_cntr : unsigned(27 downto 0);

  signal sync_in_progress : std_logic;
  signal ext_sync_p       : std_logic;

  signal resized_addr : std_logic_vector(c_wishbone_address_width-1 downto 0);
  signal wb_out       : t_wishbone_slave_out;
  signal wb_in        : t_wishbone_slave_in;

  signal ns_overflow_2nd       : std_logic;
  signal pps_in_d0, pps_ext_d0 : std_logic;

  signal retime_counter : unsigned(4 downto 0);
  signal pps_valid_int  : std_logic;

  signal pps_out_int   : std_logic;
  signal pps_in_refclk : std_logic;



begin  -- behavioral


  resized_addr(4 downto 0)                          <= wb_adr_i;
  resized_addr(c_wishbone_address_width-1 downto 5) <= (others => '0');

  U_Adapter : wb_slave_adapter
    generic map (
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
      sl_adr_i   => resized_addr,
      sl_dat_i   => wb_dat_i,
      sl_sel_i   => wb_sel_i,
      sl_cyc_i   => wb_cyc_i,
      sl_stb_i   => wb_stb_i,
      sl_we_i    => wb_we_i,
      sl_dat_o   => wb_dat_o,
      sl_ack_o   => wb_ack_o,
      sl_stall_o => wb_stall_o);


  U_Sync_pps_refclk : gc_sync_ffs
    generic map (
      g_sync_edge => "positive")
    port map (
      clk_i    => clk_ref_i,
      rst_n_i  => '1',
      data_i   => pps_in_i,
      ppulse_o => pps_in_refclk);


  ppsg_cntr_nsec  <= std_logic_vector(cntr_nsec);
  ppsg_cntr_utclo <= std_logic_vector(cntr_utc(31 downto 0));
  ppsg_cntr_utchi <= std_logic_vector(cntr_utc(39 downto 32));


  -- loads adjustment values into internal regsiters
  p_wishbone_loads : process(clk_sys_i, rst_n_i)
  begin
    if rising_edge(clk_sys_i) then
      if rst_n_i = '0' then
        adj_nsec <= (others => '0');
        adj_utc  <= (others => '0');
      else
        if(ppsg_adj_utchi_wr = '1') then
          adj_utc(39 downto 32) <= unsigned(ppsg_adj_utchi);
        end if;

        if(ppsg_adj_utclo_wr = '1') then
          adj_utc(31 downto 0) <= unsigned(ppsg_adj_utclo);
        end if;

        if(ppsg_adj_nsec_wr = '1') then
          adj_nsec <= unsigned(ppsg_adj_nsec);
        end if;
      end if;
    end if;
  end process;

  gen_without_external_clock_input : if(not g_with_ext_clock_input) generate
    ext_sync_p        <= '0';
    sync_in_progress  <= '0';
    ppsg_escr_sync_in <= '0';
  end generate gen_without_external_clock_input;

  gen_with_external_clock_input : if(g_with_ext_clock_input) generate

    p_external_sync : process(clk_ref_i)
    begin
      if falling_edge(clk_ref_i) then
        if(rst_ref_n_i = '0') then
          sync_in_progress  <= '0';
          ppsg_escr_sync_in <= '0';
        else
          if(ppsg_escr_sync_load = '1') then
            sync_in_progress  <= ppsg_escr_sync_out;
            ppsg_escr_sync_in <= '0';
          else
            if(sync_in_progress = '1' and pps_in_refclk = '1')
            then
              ext_sync_p        <= '1';
              sync_in_progress  <= '0';
              ppsg_escr_sync_in <= '1';
            else
              ext_sync_p <= '0';
            end if;
          end if;
        end if;
      end if;
    end process;

  end generate gen_with_external_clock_input;
-- Nanosecond counter. Counts from 0 to c_PERIOD-1 every clk_ref_i cycle.

  p_count_nsec : process(clk_ref_i)
  begin
    if rising_edge(clk_ref_i) then
      if rst_ref_n_i = '0' or ppsg_cr_cnt_rst = '1' then
        cntr_nsec               <= (others => '0');
        ns_overflow             <= '0';
        ns_overflow_adv         <= '0';
        adjust_in_progress_nsec <= '0';

        -- counter is enabled?
      elsif(ppsg_cr_cnt_en = '1') then

        -- got ADJUST OFFSET command
        if(cntr_adjust_p = '1') then

-- start waiting for next counter overflow
          adjust_in_progress_nsec <= '1';
        end if;

-- got SET TIME command - load the counter with new value
        if(ppsg_cr_cnt_set_p = '1' or ext_sync_p = '1' or ppsg_escr_nsec_set = '1') then
          cntr_nsec        <= adj_nsec;
          ns_overflow      <= '0';
          ns_overflow_adv  <= '0';

-- got counter overflow:
        elsif(cntr_nsec = to_unsigned(c_PERIOD-3, cntr_nsec'length)) then
          ns_overflow     <= '0';
          ns_overflow_adv <= '1';
          cntr_nsec       <= cntr_nsec + 1;
        elsif(cntr_nsec = to_unsigned(c_PERIOD-2, cntr_nsec'length)) then
          ns_overflow     <= '1';
          ns_overflow_adv <= '0';
          cntr_nsec       <= cntr_nsec + 1;
        elsif(cntr_nsec = to_unsigned(c_PERIOD-1, cntr_nsec'length)) then
          ns_overflow     <= '0';
          ns_overflow_adv <= '0';
          -- we're in the middle of offset adjustment - load the counter with
          -- offset value instead of resetting it. This equals to subtracting the offset
          -- but takes less logic.
          if(adjust_in_progress_nsec = '1') then
            cntr_nsec               <= adj_nsec;
            adjust_in_progress_nsec <= '0';
          else
            -- normal counter reset. Generate overflow pulse.
            cntr_nsec <= (others => '0');
          end if;
        else
          ns_overflow     <= '0';
          ns_overflow_adv <= '0';
          cntr_nsec       <= cntr_nsec + 1;
        end if;
      end if;
    end if;
  end process;


  p_drive_pps_valid : process(clk_ref_i)
  begin
    if rising_edge(clk_ref_i) then
      if rst_ref_n_i = '0' or ppsg_cr_cnt_rst = '1' then
        pps_valid_int   <= '0';
        ns_overflow_2nd <= '0';
      else
        if(sync_in_progress = '1' or adjust_in_progress_nsec = '1' or adjust_in_progress_utc = '1') then
          pps_valid_int   <= '0';
          ns_overflow_2nd <= '0';
        elsif(adjust_in_progress_utc = '0' and adjust_in_progress_nsec = '0' and sync_in_progress = '0') then

          if(ns_overflow = '1') then
            ns_overflow_2nd <= '1';
            if(ns_overflow_2nd = '1') then
              pps_valid_int <= '1';
            end if;
          end if;
        end if;
      end if;
    end if;
  end process;

  p_count_utc : process(clk_ref_i)
  begin
    if rising_edge(clk_ref_i) then
      if rst_ref_n_i = '0' or ppsg_cr_cnt_rst = '1' then
        cntr_utc               <= (others => '0');
        adjust_in_progress_utc <= '0';
      elsif(ppsg_cr_cnt_en = '1') then

        if(ppsg_cr_cnt_set_p = '1' or ppsg_escr_sec_set = '1') then
          cntr_utc        <= adj_utc;
        elsif(cntr_adjust_p = '1') then
          adjust_in_progress_utc <= '1';

          if(ns_overflow = '1') then
            cntr_utc <= cntr_utc +1;
          end if;

        elsif(adjust_in_progress_utc = '1' and ns_overflow = '1') then
          cntr_utc               <= cntr_utc + adj_utc + 1;
          adjust_in_progress_utc <= '0';
        elsif(ns_overflow = '1') then
          cntr_utc <= cntr_utc + 1;
        end if;
      end if;
    end if;
  end process;

-- generate single-cycle PPS pulses for synchronizing endpoint TS counters
  pps_csync_o <= ns_overflow;

  -- generates variable-width PPS pulses for PPS external output
  p_gen_pps_out : process(clk_ref_i)
  begin
    if rising_edge(clk_ref_i) then
      if rst_ref_n_i = '0' then
        pps_out_int <= '0';
        pps_led_o   <= '0';
        width_cntr  <= (others => '0');
      else

        if(ns_overflow_adv = '1') then
          pps_out_int <= ppsg_escr_pps_valid and
                         (link_ok_i or ppsg_escr_pps_unmask);
          width_cntr  <= unsigned(ppsg_cr_pwidth);
        elsif(ns_overflow = '1') then
          pps_led_o <= ppsg_escr_pps_valid;
        else
          if(width_cntr = to_unsigned(0, width_cntr'length)) then
            pps_out_int <= '0';
            pps_led_o   <= '0';
          else
            width_cntr <= width_cntr -1;
          end if;
        end if;
      end if;
    end if;

  end process;

  process(clk_ref_i)
  begin
    if rising_edge(clk_ref_i) then
      if rst_ref_n_i = '0' then
        pps_out_o <= '0';
      else
        pps_out_o <= pps_out_int;
      end if;
    end if;
  end process;

  Uwb_slave : pps_gen_wb
    port map (
      rst_n_i                => rst_n_i,
      clk_sys_i              => clk_sys_i,
      wb_adr_i               => wb_in.adr(2 downto 0),
      wb_dat_i               => wb_in.dat,
      wb_dat_o               => wb_out.dat,
      wb_cyc_i               => wb_in.cyc,
      wb_sel_i               => wb_in.sel,
      wb_stb_i               => wb_in.stb,
      wb_we_i                => wb_in.we,
      wb_ack_o               => wb_out.ack,
      refclk_i               => clk_ref_i,
      ppsg_cr_cnt_rst_o      => ppsg_cr_cnt_rst,
      ppsg_cr_cnt_en_o       => ppsg_cr_cnt_en,
      ppsg_cr_cnt_adj_o      => ppsg_cr_cnt_adj_o,
      ppsg_cr_cnt_adj_i      => ppsg_cr_cnt_adj_i,
      ppsg_cr_cnt_adj_load_o => ppsg_cr_cnt_adj_load,
      ppsg_escr_sync_o       => ppsg_escr_sync_out,
      ppsg_escr_sync_i       => ppsg_escr_sync_in,
      ppsg_escr_sync_load_o  => ppsg_escr_sync_load,
      ppsg_cr_cnt_set_o      => ppsg_cr_cnt_set_p,
      ppsg_cr_pwidth_o       => ppsg_cr_pwidth,
      ppsg_cntr_nsec_i       => ppsg_cntr_nsec,
      ppsg_cntr_utclo_i      => ppsg_cntr_utclo,
      ppsg_cntr_utchi_i      => ppsg_cntr_utchi,
      ppsg_adj_nsec_o        => ppsg_adj_nsec,
      ppsg_adj_nsec_wr_o     => ppsg_adj_nsec_wr,
      ppsg_adj_utclo_o       => ppsg_adj_utclo,
      ppsg_adj_utclo_wr_o    => ppsg_adj_utclo_wr,
      ppsg_adj_utchi_o       => ppsg_adj_utchi,
      ppsg_adj_utchi_wr_o    => ppsg_adj_utchi_wr,
      ppsg_escr_pps_valid_o  => ppsg_escr_pps_valid,
      ppsg_escr_tm_valid_o   => ppsg_escr_tm_valid,
      ppsg_escr_sec_set_o    => ppsg_escr_sec_set,
      ppsg_escr_nsec_set_o   => ppsg_escr_nsec_set,
      ppsg_escr_pps_unmask_o => ppsg_escr_pps_unmask);

-- drive unused signals
  wb_out.rty   <= '0';
  wb_out.stall <= '0';
  wb_out.int   <= '0';
  wb_out.err   <= '0';

-- start the adjustment upon write of 1 to CNT_ADJ bit
  cntr_adjust_p <= ppsg_cr_cnt_adj_load and ppsg_cr_cnt_adj_o;

-- drive the readout value of CNT_ADJ to 1 when the adjustment is over
  ppsg_cr_cnt_adj_i <= pps_valid_int;

  pps_valid_o <= pps_valid_int;

  tm_utc_o        <= std_logic_vector(cntr_utc);
  tm_cycles_o     <= std_logic_vector(cntr_nsec);
  tm_time_valid_o <= ppsg_escr_tm_valid;

end behavioral;
