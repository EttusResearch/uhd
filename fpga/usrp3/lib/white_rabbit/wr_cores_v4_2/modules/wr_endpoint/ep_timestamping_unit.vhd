-------------------------------------------------------------------------------
-- Title      : Optical 1000base-X endpoint - IEEE1588/WhiteRabbit
--              timestamping unit
-- Project    : White Rabbit 
-------------------------------------------------------------------------------
-- File       : ep_timestamping_unit.vhd
-- Author     : Tomasz Wlostowski
-- Company    : CERN BE-CO-HT
-- Created    : 2009-06-22
-- Last update: 2017-02-03
-- Platform   : FPGA-generic
-- Standard   : VHDL'87
-------------------------------------------------------------------------------
-- Description: Timestamping unit. Takes both TX and RX timestamps upon
-- detection of rising edge on asynchronous timestamp strobe inputs.
-- There are 2 timestamps taken:
-- - rising edge timestamp (28 bits by default) - the main timestamp value
-- - falling edge timestamp (4 least significant bits of the TS counter) which
--   are used to detect metastabilities and setup/hold violations which may
--   occur during sampling asynchronous timestamp strobes.
-- Both timestamps are taken using refclk_i.
-------------------------------------------------------------------------------
--
-- Copyright (c) 2009 - 2012 CERN
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
-- 2009-06-22  0.1      twlostow        Created
-------------------------------------------------------------------------------



library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

library work;
use work.gencores_pkg.all;
use work.endpoint_private_pkg.all;
use work.ep_wbgen2_pkg.all;

entity ep_timestamping_unit is
  generic (
-- size of rising edge timestamp
    g_timestamp_bits_r : natural := 28;
-- size of falling edge timestamp
    g_timestamp_bits_f : natural := 4;
    g_ref_clock_rate   : integer := 125000000
    );

  port (
-- reference clock (for the timestamping counters)
    clk_ref_i : in std_logic;

-- reference / 2 (bus-side logic)
    clk_sys_i : in std_logic;

-- RX clock
    clk_rx_i : in std_logic;

    -- resets
    rst_n_rx_i  : in std_logic;
    rst_n_ref_i : in std_logic;
    rst_n_sys_i : in std_logic;

-- PPS pulse input (active HI for 1 clk_ref_i cycle) for internal TS counter synchronization
    pps_csync_p1_i : in std_logic;

    pps_valid_i : in std_logic;

-- asynchronous TX/RX timestamp triggers (from PCS)
    tx_timestamp_trigger_p_a_i : in std_logic;
    rx_timestamp_trigger_p_a_i : in std_logic;

-------------------------------------------------------------------------------
-- RX Timestamp output (clk_rx_i clock domain)
-------------------------------------------------------------------------------

    -- RX timestamp (to RX deframer)
    rxts_timestamp_o : out std_logic_vector(31 downto 0);

    -- RX timestamp strobe (back to the PCSr). When HI,
    -- rxts_timestamp_o and rxts_timestamp_valid_o contain information about
    -- the RX timestamp and its validity
    rxts_timestamp_stb_o : out std_logic;

    -- RX timestamp valid (to RX deframer)
    rxts_timestamp_valid_o : out std_logic;

-------------------------------------------------------------------------------
-- TX Timestamp output (clk_ref_i clock domain)
-------------------------------------------------------------------------------    

    -- TX timestamp output (to TXTSU/Framer)
    txts_timestamp_o : out std_logic_vector(31 downto 0);

    -- TX timestamp strobe (to TXTSU/Framer). When HI,
    -- txts_timestamp_o and txts_timestamp_valid_o contain information about
    -- the TX timestamp and its validity
    txts_timestamp_stb_o : out std_logic;

    -- TX timestamp valid (to TXTSU/Framer)
    txts_timestamp_valid_o : out std_logic;

    txts_o                 : out std_logic; 		-- 2013-Nov-28 peterj added for debugging/calibration
    rxts_o                 : out std_logic; 		-- 2013-Nov-28 peterj added for debugging/calibration

-------------------------------------------------------------------------------
-- Wishbone regs
-------------------------------------------------------------------------------

    regs_i : in  t_ep_out_registers;
    regs_o : out t_ep_in_registers
    );

end ep_timestamping_unit;



architecture syn of ep_timestamping_unit is


  component ep_ts_counter
    generic (
      g_num_bits_r : natural;
      g_num_bits_f : natural;
      g_init_value : natural;
      g_max_value  : natural);
    port (
      clk_i          : in  std_logic;
      rst_n_i        : in  std_logic;
      overflow_o     : out std_logic := '0';
      value_r_o      : out std_logic_vector(g_num_bits_r-1 downto 0);
      value_f_o      : out std_logic_vector(g_num_bits_f-1 downto 0);
      pps_p_i        : in  std_logic;
      sync_start_p_i : in  std_logic;
      sync_done_o    : out std_logic);
  end component;

  signal cntr_rx_r : std_logic_vector(g_timestamp_bits_r-1 downto 0);
  signal cntr_rx_f : std_logic_vector(g_timestamp_bits_f-1 downto 0);
  signal cntr_tx_r : std_logic_vector(g_timestamp_bits_r-1 downto 0);
  signal cntr_tx_f : std_logic_vector(g_timestamp_bits_f-1 downto 0);

  signal cntr_r : std_logic_vector(g_timestamp_bits_r-1 downto 0);
  signal cntr_f : std_logic_vector(g_timestamp_bits_f-1 downto 0);

  signal take_tx_synced_p, take_rx_synced_p             : std_logic;
  signal take_tx_synced_p_fedge, take_rx_synced_p_fedge : std_logic;

  signal tx_sync_delay : std_logic_vector(4 downto 0);
  signal rx_sync_delay : std_logic_vector(4 downto 0);
  signal rx_ts_done    : std_logic;
  signal tx_ts_done    : std_logic;

  signal got_tx_oob : std_logic;
  signal tx_oob_reg : std_logic_vector(15 downto 0);


  signal rx_oob_reg : std_logic_vector(47 downto 0);
  signal fid_valid  : std_logic;

  signal txts_valid : std_logic;

  signal valid_rx, valid_tx : std_logic;


  signal cal_count                                   : unsigned(5 downto 0);
  signal rx_trigger_mask, rx_trigger_a, rx_cal_pulse_a : std_logic;

  signal regs_o_tscr_cs_done       : std_logic;
  signal regs_o_tscr_rx_cal_result : std_logic;
  
begin  -- syn

  -- Instatniation of the timestamping counter
  U_counter : ep_ts_counter
    generic map (
      g_num_bits_r => g_timestamp_bits_r,
      g_num_bits_f => g_timestamp_bits_f,
      g_init_value => 0,
      g_max_value  => g_ref_clock_rate-1)
    port map (

      clk_i          => clk_ref_i,
      rst_n_i        => rst_n_ref_i,
      pps_p_i        => pps_csync_p1_i,
      overflow_o     => open,
      value_r_o      => cntr_r,
      value_f_o      => cntr_f,
      sync_start_p_i => regs_i.tscr_cs_start_o,
      sync_done_o    => regs_o_tscr_cs_done
      );


  p_gen_regs_o: process (regs_o_tscr_cs_done, regs_o_tscr_rx_cal_result) is
  begin
    -- initial values
    regs_o <= c_ep_in_registers_init_value;

    -- override initial values
    regs_o.tscr_cs_done_i       <= regs_o_tscr_cs_done;
    regs_o.tscr_rx_cal_result_i <= regs_o_tscr_rx_cal_result;

  end process p_gen_regs_o;

  p_rx_timestamper_calibration : process(clk_rx_i)
  begin
    if rising_edge(clk_rx_i) then
      if rst_n_rx_i = '0' then
        cal_count       <= (others => '0');
        rx_cal_pulse_a  <= '0';
        rx_trigger_mask <= '1';
        
      elsif(regs_i.tscr_rx_cal_start_o = '1') then
        cal_count       <= to_unsigned(1, 6);
        rx_trigger_mask <= '0';
      elsif(cal_count /= 0) then
        cal_count <= cal_count + 1;

        if(rx_ts_done = '1') then
          if(cntr_rx_f /= cntr_rx_r(g_timestamp_bits_f-1 downto 0)) then
            regs_o_tscr_rx_cal_result <= '1';
          else
            regs_o_tscr_rx_cal_result <= '0';
          end if;
        end if;

      else
        
        rx_trigger_mask <= '1';
      end if;

      if(cal_count (5 downto 4) = x"01") then
        rx_cal_pulse_a <= '1';
      else
        rx_cal_pulse_a <= '0';
      end if;
      
    end if;
  end process;


  rx_trigger_a       <= (rx_timestamp_trigger_p_a_i and rx_trigger_mask) or rx_cal_pulse_a;
  -- Sync chains for timestamp strobes: 4 combinations - (TX-RX) -> (rising/falling)
  sync_ffs_tx_r : gc_sync_ffs
    generic map (
      g_sync_edge => "positive")
    port map (
      clk_i    => clk_ref_i,
      rst_n_i  => rst_n_ref_i,
      data_i   => tx_timestamp_trigger_p_a_i,
      synced_o => open,
      npulse_o => open,
      ppulse_o => take_tx_synced_p);


  
  sync_ffs_rx_r : gc_sync_ffs
    generic map (
      g_sync_edge => "positive")
    port map (
      clk_i    => clk_ref_i,
      rst_n_i  => rst_n_ref_i,
      data_i   => rx_trigger_a,
      synced_o => open,
      npulse_o => open,
      ppulse_o => take_rx_synced_p);


  sync_ffs_tx_f : gc_sync_ffs
    generic map (
      g_sync_edge => "negative")
    port map (
      clk_i    => clk_ref_i,
      rst_n_i  => rst_n_ref_i,
      data_i   => tx_timestamp_trigger_p_a_i,
      synced_o => open,
      npulse_o => open,
      ppulse_o => take_tx_synced_p_fedge);

  sync_ffs_rx_f : gc_sync_ffs
    generic map (
      g_sync_edge => "negative")
    port map (
      clk_i    => clk_ref_i,
      rst_n_i  => rst_n_ref_i,
      data_i   => rx_trigger_a,
      synced_o => open,
      npulse_o => open,
      ppulse_o => take_rx_synced_p_fedge);

  

  take_r : process(clk_ref_i)
  begin
    if rising_edge(clk_ref_i) then
      if(rst_n_ref_i = '0') then
        cntr_rx_r <= (others => '0');
        cntr_tx_r <= (others => '0');

        rx_sync_delay <= (others => '0');
        tx_sync_delay <= (others => '0');
      else
        -- shift reg

        if take_rx_synced_p = '1' then
          cntr_rx_r                                                           <= cntr_r;
          valid_rx                                                            <= pps_valid_i;
          rx_sync_delay(rx_sync_delay'length-1 downto rx_sync_delay'length-4) <= (others => '1');
        else
          rx_sync_delay <= '0' & rx_sync_delay(rx_sync_delay'length-1 downto 1);
        end if;

        if take_tx_synced_p = '1' then
          cntr_tx_r                                                           <= cntr_r;
          valid_tx                                                            <= pps_valid_i;
          tx_sync_delay(tx_sync_delay'length-1 downto tx_sync_delay'length-4) <= (others => '1');
        else
          tx_sync_delay <= '0' & tx_sync_delay(tx_sync_delay'length-1 downto 1);
        end if;
        
      end if;
    end if;
  end process;

  take_f : process(clk_ref_i)
  begin
    if falling_edge(clk_ref_i) then
      if rst_n_ref_i = '0' then
        cntr_rx_f <= (others => '0');
        cntr_tx_f <= (others => '0');
      else
        if take_rx_synced_p_fedge = '1' then
          cntr_rx_f <= cntr_f;
        end if;
        if take_tx_synced_p_fedge = '1' then
          cntr_tx_f <= cntr_f;
        end if;
      end if;
    end if;
  end process;


  -- timestamping "done" signals sync chains (clk_ref -> clk_sys)
  tx_done_gen : gc_sync_ffs
    generic map (
      g_sync_edge => "positive")
    port map (
      clk_i    => clk_sys_i,
      rst_n_i  => rst_n_sys_i,
      data_i   => tx_sync_delay(0),
      synced_o => open,
      npulse_o => tx_ts_done,
      ppulse_o => open);

  -- timestamping "done" signals sync chains (clk_rx -> clk_sys)
  rx_done_gen : gc_sync_ffs
    generic map (
      g_sync_edge => "positive")
    port map (
      clk_i    => clk_rx_i,
      rst_n_i  => rst_n_rx_i,
      data_i   => rx_sync_delay(0),
      synced_o => open,
      npulse_o => rx_ts_done,
      ppulse_o => open);

  txts_o <= tx_ts_done; 		-- 2013-Nov-28 peterj added for debugging/calibration
  rxts_o <= rx_ts_done; 		-- 2013-Nov-28 peterj added for debugging/calibration

  p_output_rx_ts : process (clk_rx_i)
  begin
    if rising_edge(clk_rx_i) then
      if(rst_n_rx_i = '0') then
        rxts_timestamp_stb_o   <= '0';
        rxts_timestamp_o       <= (others => '0');
        rxts_timestamp_valid_o <= '0';
      else
        if(regs_i. tscr_en_rxts_o = '0') then
          rxts_timestamp_stb_o <= '0';
        elsif(rx_ts_done = '1' and regs_i.tscr_en_rxts_o = '1') then
          rxts_timestamp_stb_o   <= '1';
          rxts_timestamp_valid_o <= valid_rx;
          rxts_timestamp_o       <= cntr_rx_f & cntr_rx_r;
        end if;
      end if;
    end if;
  end process;


  p_output_tx_ts : process (clk_sys_i)
  begin
    if rising_edge(clk_sys_i) then
      if(rst_n_sys_i = '0') then
        txts_timestamp_o       <= (others => '0');
        txts_timestamp_stb_o   <= '0';
        txts_timestamp_valid_o <= '0';
      elsif(tx_ts_done = '1' and regs_i.tscr_en_txts_o = '1') then
        txts_timestamp_o       <= cntr_tx_f & cntr_tx_r;
        txts_timestamp_stb_o   <= '1';
        txts_timestamp_valid_o <= valid_tx;
      end if;
    end if;
  end process;

end syn;
