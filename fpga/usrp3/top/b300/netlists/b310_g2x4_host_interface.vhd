--
-- Copyright 2026 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: b310_g2x4_host_interface
--
-- Description:
--   Allows connection from B310 to the host system over PCIe.
--   This module also implements DMA FIFOs for data transfer.
--   Finally, this module provides register access for specific
--   address windows within the BAR space.
--

library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;

entity b310_g2x4_host_interface is
  port (
    pcie_rx_p : in std_logic_vector(3 downto 0);
    pcie_rx_n : in std_logic_vector(3 downto 0);
    pcie_tx_p : out std_logic_vector(3 downto 0);
    pcie_tx_n : out std_logic_vector(3 downto 0);
    pcie_ref_clk_p : in std_logic;
    pcie_ref_clk_n : in std_logic;
    clk_40mhz : in std_logic;
    bus_clk : in std_logic;
    dma_clk : out std_logic;
    pcie_arst : in std_logic;
    bus_arst : out boolean;
    host_dma_rx_tdata : in std_logic_vector(639 downto 0);
    host_dma_rx_tready : out std_logic_vector(4 downto 0);
    host_dma_rx_tvalid : in std_logic_vector(4 downto 0);
    host_dma_tx_tdata : out std_logic_vector(639 downto 0);
    host_dma_tx_tready : in std_logic_vector(4 downto 0);
    host_dma_tx_tvalid : out std_logic_vector(4 downto 0);
    pcie_usr_ctrlport_req_wr : out std_logic;
    pcie_usr_ctrlport_req_rd : out std_logic;
    pcie_usr_ctrlport_req_addr : out std_logic_vector(19 downto 0);
    pcie_usr_ctrlport_req_port_id : out std_logic_vector(9 downto 0);
    pcie_usr_ctrlport_req_rem_epid : out std_logic_vector(15 downto 0);
    pcie_usr_ctrlport_req_rem_portid : out std_logic_vector(9 downto 0);
    pcie_usr_ctrlport_req_data : out std_logic_vector(31 downto 0);
    pcie_usr_ctrlport_req_byte_en : out std_logic_vector(3 downto 0);
    pcie_usr_ctrlport_req_has_time : out std_logic;
    pcie_usr_ctrlport_req_time : out std_logic_vector(63 downto 0);
    pcie_usr_ctrlport_resp_ack : in std_logic;
    pcie_usr_ctrlport_resp_status : in std_logic_vector(1 downto 0);
    pcie_usr_ctrlport_resp_data : in std_logic_vector(31 downto 0);
    cpld_int_report_in : out std_logic_vector(50 downto 0);
    cpld_int_report_out : in std_logic_vector(33 downto 0);
    core_ctrlport_req_wr : out std_logic;
    core_ctrlport_req_rd : out std_logic;
    core_ctrlport_req_addr : out std_logic_vector(19 downto 0);
    core_ctrlport_req_port_id : out std_logic_vector(9 downto 0);
    core_ctrlport_req_rem_epid : out std_logic_vector(15 downto 0);
    core_ctrlport_req_rem_portid : out std_logic_vector(9 downto 0);
    core_ctrlport_req_data : out std_logic_vector(31 downto 0);
    core_ctrlport_req_byte_en : out std_logic_vector(3 downto 0);
    core_ctrlport_req_has_time : out std_logic;
    core_ctrlport_req_time : out std_logic_vector(63 downto 0);
    core_ctrlport_resp_ack : in std_logic;
    core_ctrlport_resp_status : in std_logic_vector(1 downto 0);
    core_ctrlport_resp_data : in std_logic_vector(31 downto 0);
    auth_sda_in : in std_logic;
    auth_sda_out : out std_logic
  );
end entity b310_g2x4_host_interface;

architecture RTL of b310_g2x4_host_interface is
begin
end architecture RTL;
