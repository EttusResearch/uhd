--
-- Copyright 2025 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: b310_jesd204b
--
-- Description:
--   This module implements the JESD204B interface for the B310.
--   This includes the implementation of the physical layer of
--   the interface via MGT control, as well as the link layer
--   as specified by the JESD204B standard.
--

library ieee;
   use ieee.std_logic_1164.all;

entity b310_jesd204b is
  port (
    bclk_rst : in std_logic; 
    bus_clk : in std_logic; 
    clk_40mhz : in std_logic; 
    sample_clk_1x : in std_logic; 
    sample_clk_2x : in std_logic; 
    bclk_fpga_clocks_stable : in std_logic; 
    jesd_ref_clk_p : in std_logic; 
    jesd_ref_clk_n : in std_logic; 
    bclk_jesd_ref_clk_present : out std_logic; 
    lmk_sync : out std_logic; 
    bclk_ctrlport_req_wr : in std_logic; 
    bclk_ctrlport_req_rd : in std_logic; 
    bclk_ctrlport_req_addr : in std_logic_vector(19 downto 0); 
    bclk_ctrlport_req_port_id : in std_logic_vector(9 downto 0); 
    bclk_ctrlport_req_rem_epid : in std_logic_vector(15 downto 0); 
    bclk_ctrlport_req_rem_portid : in std_logic_vector(9 downto 0); 
    bclk_ctrlport_req_data : in std_logic_vector(31 downto 0); 
    bclk_ctrlport_req_byte_en : in std_logic_vector(3 downto 0); 
    bclk_ctrlport_req_has_time : in std_logic; 
    bclk_ctrlport_req_time : in std_logic_vector(63 downto 0); 
    bclk_ctrlport_resp_ack : out std_logic; 
    bclk_ctrlport_resp_status : out std_logic_vector(1 downto 0); 
    bclk_ctrlport_resp_data : out std_logic_vector(31 downto 0); 
    capture_sysref_clk : in std_logic; 
    sysref_in_p : in std_logic; 
    sysref_in_n : in std_logic; 
    sclk_sysref_out : out std_logic; 
    adc_rx_p : in std_logic_vector(1 downto 0); 
    adc_rx_n : in std_logic_vector(1 downto 0); 
    adc_sync_out_n : out std_logic; 
    dac_tx_p : out std_logic_vector(1 downto 0); 
    dac_tx_n : out std_logic_vector(1 downto 0); 
    dac_sync_in_n : in std_logic; 
    sclk_adc_data_flatter : out std_logic_vector(63 downto 0); 
    sclk_dac_data_flatter : in std_logic_vector(63 downto 0); 
    sclk_adc_data_valid : out std_logic; 
    sclk_dac_ready_for_input : out std_logic; 
    dac_sync_out : out std_logic; 
    adc_sync_out : out std_logic
  );
end entity b310_jesd204b;

architecture RTL of b310_jesd204b is
begin
end architecture RTL;
