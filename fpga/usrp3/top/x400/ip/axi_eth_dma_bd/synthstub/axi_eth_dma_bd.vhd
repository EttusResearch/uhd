--
-- Copyright 2021 Ettus Research, a National Instruments Brand
--
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: axi_eth_dma_bd
--
-- Description:
--
--   This is an automatically generated stub file to match the entity
--   declaration for 'axi_eth_dma_bd'. This file was created using
--   niBdExportStub Do not modify this file directly!
--

library ieee;
use ieee.std_logic_1164.all;
library unisim;
use unisim.vcomponents.all;

entity axi_eth_dma_bd is
port (
    clk40 : in STD_LOGIC;
    clk40_rstn : in STD_LOGIC;
    eth_rx_irq : out STD_LOGIC;
    eth_tx_irq : out STD_LOGIC;
    c2e_tdata : out STD_LOGIC_VECTOR ( 63 downto 0 );
    c2e_tkeep : out STD_LOGIC_VECTOR ( 7 downto 0 );
    c2e_tlast : out STD_LOGIC;
    c2e_tready : in STD_LOGIC;
    c2e_tvalid : out STD_LOGIC;
    e2c_tdata : in STD_LOGIC_VECTOR ( 63 downto 0 );
    e2c_tkeep : in STD_LOGIC_VECTOR ( 7 downto 0 );
    e2c_tlast : in STD_LOGIC;
    e2c_tready : out STD_LOGIC;
    e2c_tvalid : in STD_LOGIC;
    axi_eth_dma_araddr : in STD_LOGIC_VECTOR ( 9 downto 0 );
    axi_eth_dma_arready : out STD_LOGIC;
    axi_eth_dma_arvalid : in STD_LOGIC;
    axi_eth_dma_awaddr : in STD_LOGIC_VECTOR ( 9 downto 0 );
    axi_eth_dma_awready : out STD_LOGIC;
    axi_eth_dma_awvalid : in STD_LOGIC;
    axi_eth_dma_bready : in STD_LOGIC;
    axi_eth_dma_bresp : out STD_LOGIC_VECTOR ( 1 downto 0 );
    axi_eth_dma_bvalid : out STD_LOGIC;
    axi_eth_dma_rdata : out STD_LOGIC_VECTOR ( 31 downto 0 );
    axi_eth_dma_rready : in STD_LOGIC;
    axi_eth_dma_rresp : out STD_LOGIC_VECTOR ( 1 downto 0 );
    axi_eth_dma_rvalid : out STD_LOGIC;
    axi_eth_dma_wdata : in STD_LOGIC_VECTOR ( 31 downto 0 );
    axi_eth_dma_wready : out STD_LOGIC;
    axi_eth_dma_wvalid : in STD_LOGIC;
    axi_hp_awaddr : out STD_LOGIC_VECTOR ( 48 downto 0 );
    axi_hp_awlen : out STD_LOGIC_VECTOR ( 7 downto 0 );
    axi_hp_awsize : out STD_LOGIC_VECTOR ( 2 downto 0 );
    axi_hp_awburst : out STD_LOGIC_VECTOR ( 1 downto 0 );
    axi_hp_awlock : out STD_LOGIC_VECTOR ( 0 to 0 );
    axi_hp_awcache : out STD_LOGIC_VECTOR ( 3 downto 0 );
    axi_hp_awprot : out STD_LOGIC_VECTOR ( 2 downto 0 );
    axi_hp_awqos : out STD_LOGIC_VECTOR ( 3 downto 0 );
    axi_hp_awvalid : out STD_LOGIC;
    axi_hp_awready : in STD_LOGIC;
    axi_hp_wdata : out STD_LOGIC_VECTOR ( 127 downto 0 );
    axi_hp_wstrb : out STD_LOGIC_VECTOR ( 15 downto 0 );
    axi_hp_wlast : out STD_LOGIC;
    axi_hp_wvalid : out STD_LOGIC;
    axi_hp_wready : in STD_LOGIC;
    axi_hp_bresp : in STD_LOGIC_VECTOR ( 1 downto 0 );
    axi_hp_bvalid : in STD_LOGIC;
    axi_hp_bready : out STD_LOGIC;
    axi_hp_araddr : out STD_LOGIC_VECTOR ( 48 downto 0 );
    axi_hp_arlen : out STD_LOGIC_VECTOR ( 7 downto 0 );
    axi_hp_arsize : out STD_LOGIC_VECTOR ( 2 downto 0 );
    axi_hp_arburst : out STD_LOGIC_VECTOR ( 1 downto 0 );
    axi_hp_arlock : out STD_LOGIC_VECTOR ( 0 to 0 );
    axi_hp_arcache : out STD_LOGIC_VECTOR ( 3 downto 0 );
    axi_hp_arprot : out STD_LOGIC_VECTOR ( 2 downto 0 );
    axi_hp_arqos : out STD_LOGIC_VECTOR ( 3 downto 0 );
    axi_hp_arvalid : out STD_LOGIC;
    axi_hp_arready : in STD_LOGIC;
    axi_hp_rdata : in STD_LOGIC_VECTOR ( 127 downto 0 );
    axi_hp_rresp : in STD_LOGIC_VECTOR ( 1 downto 0 );
    axi_hp_rlast : in STD_LOGIC;
    axi_hp_rvalid : in STD_LOGIC;
    axi_hp_rready : out STD_LOGIC
  );
  end entity axi_eth_dma_bd;

architecture stub of axi_eth_dma_bd is
begin
end architecture stub;
