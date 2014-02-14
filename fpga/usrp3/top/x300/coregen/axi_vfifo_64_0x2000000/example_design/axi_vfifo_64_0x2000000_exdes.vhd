--------------------------------------------------------------------------------
--
-- AXI Virtual FIFO Controller Core - core top file for implementation
--
--------------------------------------------------------------------------------
--
-- (c) Copyright 2011 - 2012 Xilinx, Inc. All rights reserved.
-- 
-- This file contains confidential and proprietary information
-- of Xilinx, Inc. and is protected under U.S. and
-- international copyright and other intellectual property
-- laws.
-- 
-- DISCLAIMER
-- This disclaimer is not a license and does not grant any
-- rights to the materials distributed herewith. Except as
-- otherwise provided in a valid license issued to you by
-- Xilinx, and to the maximum extent permitted by applicable
-- law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
-- WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
-- AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
-- BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
-- INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
-- (2) Xilinx shall not be liable (whether in contract or tort,
-- including negligence, or under any other theory of
-- liability) for any loss or damage of any kind or nature
-- related to, arising under or in connection with these
-- materials, including for any direct, or any indirect,
-- special, incidental, or consequential loss or damage
-- (including loss of data, profits, goodwill, or any type of
-- loss or damage suffered as a result of any action brought
-- by a third party) even if such damage or loss was
-- reasonably foreseeable or Xilinx had been advised of the
-- possibility of the same.
-- 
-- CRITICAL APPLICATIONS
-- Xilinx products are not designed or intended to be fail-
-- safe, or for use in any application requiring fail-safe
-- performance, such as life-support or safety devices or
-- systems, Class III medical devices, nuclear facilities,
-- applications related to the deployment of airbags, or any
-- other applications that could lead to death, personal
-- injury, or severe property or environmental damage
-- (individually and collectively, "Critical
-- Applications"). Customer assumes the sole risk and
-- liability of any use of Xilinx products in Critical
-- Applications, subject only to applicable laws and
-- regulations governing limitations on product liability.
-- 
-- THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
-- PART OF THIS FILE AT ALL TIMES.
--------------------------------------------------------------------------------
--
-- Filename: axi_vfifo_64_0x2000000_exdes.vhd
--
-- Description:
--   This is the VFIFO core wrapper with BUFG instances for clock connections.
--
--------------------------------------------------------------------------------
-- Library Declarations
--------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;
use ieee.std_logic_arith.all;
use ieee.std_logic_unsigned.all;

library unisim;
use unisim.vcomponents.all;

--------------------------------------------------------------------------------
-- Entity Declaration
--------------------------------------------------------------------------------
entity axi_vfifo_64_0x2000000_exdes is
   PORT (

    -- AXI Stream Input Signals From Switch to AXI Virtual FIFO Controller (AVFC)
    S_AXIS_TVALID                       : IN  STD_LOGIC := '0';
    S_AXIS_TREADY                       : OUT STD_LOGIC := '0';
    S_AXIS_TDATA                        : IN  STD_LOGIC_VECTOR(64 - 1 DOWNTO 0)   := (OTHERS =>'0');
    S_AXIS_TSTRB                        : IN  STD_LOGIC_VECTOR(64/8 - 1 DOWNTO 0) := (OTHERS =>'0');
    S_AXIS_TKEEP                        : IN  STD_LOGIC_VECTOR(64/8 - 1 DOWNTO 0) := (OTHERS =>'0');
    S_AXIS_TLAST                        : IN  STD_LOGIC := '0';
    S_AXIS_TID                          : IN  STD_LOGIC_VECTOR(1 - 1 DOWNTO 0)   := (OTHERS =>'0');
    S_AXIS_TDEST                        : IN  STD_LOGIC_VECTOR(1 - 1 DOWNTO 0)   := (OTHERS =>'0');

    -- AXI Virtual FIFO Controller (AVFC) Output Signals To AXI Stream Switch
    M_AXIS_TVALID                       : OUT STD_LOGIC := '0';
    M_AXIS_TREADY                       : IN  STD_LOGIC := '0';
    M_AXIS_TDATA                        : OUT STD_LOGIC_VECTOR(64 - 1 DOWNTO 0)   := (OTHERS => '0');
    M_AXIS_TSTRB                        : OUT STD_LOGIC_VECTOR(64/8 - 1 DOWNTO 0) := (OTHERS => '1');
    M_AXIS_TKEEP                        : OUT STD_LOGIC_VECTOR(64/8 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXIS_TLAST                        : OUT STD_LOGIC := '0';
    M_AXIS_TID                          : OUT STD_LOGIC_VECTOR(1 - 1 DOWNTO 0)   := (OTHERS => '0');
    M_AXIS_TDEST                        : OUT STD_LOGIC_VECTOR(1 - 1 DOWNTO 0)   := (OTHERS => '0');

    -- Write Address Channel Signals
    M_AXI_AWID                          : OUT STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_AWADDR                        : OUT STD_LOGIC_VECTOR(32 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_AWLEN                         : OUT STD_LOGIC_VECTOR(8 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_AWSIZE                        : OUT STD_LOGIC_VECTOR(3 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_AWBURST                       : OUT STD_LOGIC_VECTOR(2 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_AWLOCK                        : OUT STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_AWCACHE                       : OUT STD_LOGIC_VECTOR(4 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_AWPROT                        : OUT STD_LOGIC_VECTOR(3 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_AWQOS                         : OUT STD_LOGIC_VECTOR(4 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_AWREGION                      : OUT STD_LOGIC_VECTOR(4 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_AWUSER                        : OUT STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_AWVALID                       : OUT STD_LOGIC := '0';
    M_AXI_AWREADY                       : IN  STD_LOGIC := '0';

    -- Write Data Channel Signals
    M_AXI_WDATA                         : OUT STD_LOGIC_VECTOR(64 - 1 DOWNTO 0)     := (OTHERS => '0');
    M_AXI_WSTRB                         : OUT STD_LOGIC_VECTOR(64 / 8 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_WLAST                         : OUT STD_LOGIC := '0';
    M_AXI_WUSER                         : OUT STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_WVALID                        : OUT STD_LOGIC := '0';
    M_AXI_WREADY                        : IN  STD_LOGIC := '0';

    -- Write Response Channel Signals
    M_AXI_BID                           : IN  STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS =>'0');
    M_AXI_BRESP                         : IN  STD_LOGIC_VECTOR(2 - 1 DOWNTO 0) := (OTHERS =>'0');
    M_AXI_BUSER                         : IN  STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS =>'0');
    M_AXI_BVALID                        : IN  STD_LOGIC := '0';
    M_AXI_BREADY                        : OUT STD_LOGIC := '1';

    -- Read Address Channel Signals
    M_AXI_ARID                          : OUT STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_ARADDR                        : OUT STD_LOGIC_VECTOR(32 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_ARLEN                         : OUT STD_LOGIC_VECTOR(8 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_ARSIZE                        : OUT STD_LOGIC_VECTOR(3 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_ARBURST                       : OUT STD_LOGIC_VECTOR(2 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_ARLOCK                        : OUT STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_ARCACHE                       : OUT STD_LOGIC_VECTOR(4 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_ARPROT                        : OUT STD_LOGIC_VECTOR(3 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_ARQOS                         : OUT STD_LOGIC_VECTOR(4 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_ARREGION                      : OUT STD_LOGIC_VECTOR(4 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_ARUSER                        : OUT STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS => '0');
    M_AXI_ARVALID                       : OUT STD_LOGIC := '0';
    M_AXI_ARREADY                       : IN  STD_LOGIC := '0';

    -- Read Data Channel Signals
    M_AXI_RID                           : IN  STD_LOGIC_VECTOR(1 - 1 DOWNTO 0)   := (OTHERS =>'0');
    M_AXI_RDATA                         : IN  STD_LOGIC_VECTOR(64 - 1 DOWNTO 0) := (OTHERS =>'0');
    M_AXI_RRESP                         : IN  STD_LOGIC_VECTOR(2 - 1 DOWNTO 0) := (OTHERS =>'0');
    M_AXI_RLAST                         : IN  STD_LOGIC := '0';
    M_AXI_RUSER                         : IN  STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS =>'0');
    M_AXI_RVALID                        : IN  STD_LOGIC := '0';
    M_AXI_RREADY                        : OUT STD_LOGIC := '0';

    -- External Interface Signals
    VFIFO_MM2S_CHANNEL_FULL             : IN  STD_LOGIC_VECTOR(2 - 1 DOWNTO 0) := (OTHERS =>'0');
    VFIFO_S2MM_CHANNEL_FULL             : OUT STD_LOGIC_VECTOR(2 - 1 DOWNTO 0) := (OTHERS =>'0');
    VFIFO_MM2S_CHANNEL_EMPTY            : OUT STD_LOGIC_VECTOR(2 - 1 DOWNTO 0) := (OTHERS =>'1');

    -- Status Signal
    VFIFO_IDLE                          : OUT STD_LOGIC_VECTOR(2 - 1 DOWNTO 0) := (OTHERS =>'1');

    -- Interrupt Signals
    VFIFO_MM2S_RRESP_ERR_INTR           : OUT STD_LOGIC := '0';
    VFIFO_S2MM_BRESP_ERR_INTR           : OUT STD_LOGIC := '0';
    VFIFO_S2MM_OVERRUN_ERR_INTR         : OUT STD_LOGIC := '0';

    -- Global Signals
    ACLK                                : IN  STD_LOGIC := '0';
    ARESETN                             : IN  STD_LOGIC := '0'
  );

end axi_vfifo_64_0x2000000_exdes;



architecture xilinx of axi_vfifo_64_0x2000000_exdes is

  signal s_aclk_i : std_logic;

  component axi_vfifo_64_0x2000000 is
    PORT (
 
     -- AXI Stream Input Signals From Switch to AXI Virtual FIFO Controller (AVFC)
     S_AXIS_TVALID                       : IN  STD_LOGIC := '0';
     S_AXIS_TREADY                       : OUT STD_LOGIC := '0';
     S_AXIS_TDATA                        : IN  STD_LOGIC_VECTOR(64 - 1 DOWNTO 0)   := (OTHERS =>'0');
     S_AXIS_TSTRB                        : IN  STD_LOGIC_VECTOR(64/8 - 1 DOWNTO 0) := (OTHERS =>'0');
     S_AXIS_TKEEP                        : IN  STD_LOGIC_VECTOR(64/8 - 1 DOWNTO 0) := (OTHERS =>'0');
     S_AXIS_TLAST                        : IN  STD_LOGIC := '0';
     S_AXIS_TID                          : IN  STD_LOGIC_VECTOR(1 - 1 DOWNTO 0)   := (OTHERS =>'0');
     S_AXIS_TDEST                        : IN  STD_LOGIC_VECTOR(1 - 1 DOWNTO 0)   := (OTHERS =>'0');
 
     -- AXI Virtual FIFO Controller (AVFC) Output Signals To AXI Stream Switch
     M_AXIS_TVALID                       : OUT STD_LOGIC := '0';
     M_AXIS_TREADY                       : IN  STD_LOGIC := '0';
     M_AXIS_TDATA                        : OUT STD_LOGIC_VECTOR(64 - 1 DOWNTO 0)   := (OTHERS => '0');
     M_AXIS_TSTRB                        : OUT STD_LOGIC_VECTOR(64/8 - 1 DOWNTO 0) := (OTHERS => '1');
     M_AXIS_TKEEP                        : OUT STD_LOGIC_VECTOR(64/8 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXIS_TLAST                        : OUT STD_LOGIC := '0';
     M_AXIS_TID                          : OUT STD_LOGIC_VECTOR(1 - 1 DOWNTO 0)   := (OTHERS => '0');
     M_AXIS_TDEST                        : OUT STD_LOGIC_VECTOR(1 - 1 DOWNTO 0)   := (OTHERS => '0');
 
     -- Write Address Channel Signals
     M_AXI_AWID                          : OUT STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_AWADDR                        : OUT STD_LOGIC_VECTOR(32 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_AWLEN                         : OUT STD_LOGIC_VECTOR(8 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_AWSIZE                        : OUT STD_LOGIC_VECTOR(3 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_AWBURST                       : OUT STD_LOGIC_VECTOR(2 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_AWLOCK                        : OUT STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_AWCACHE                       : OUT STD_LOGIC_VECTOR(4 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_AWPROT                        : OUT STD_LOGIC_VECTOR(3 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_AWQOS                         : OUT STD_LOGIC_VECTOR(4 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_AWREGION                      : OUT STD_LOGIC_VECTOR(4 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_AWUSER                        : OUT STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_AWVALID                       : OUT STD_LOGIC := '0';
     M_AXI_AWREADY                       : IN  STD_LOGIC := '0';
 
     -- Write Data Channel Signals
     M_AXI_WDATA                         : OUT STD_LOGIC_VECTOR(64 - 1 DOWNTO 0)     := (OTHERS => '0');
     M_AXI_WSTRB                         : OUT STD_LOGIC_VECTOR(64 / 8 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_WLAST                         : OUT STD_LOGIC := '0';
     M_AXI_WUSER                         : OUT STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_WVALID                        : OUT STD_LOGIC := '0';
     M_AXI_WREADY                        : IN  STD_LOGIC := '0';
 
     -- Write Response Channel Signals
     M_AXI_BID                           : IN  STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS =>'0');
     M_AXI_BRESP                         : IN  STD_LOGIC_VECTOR(2 - 1 DOWNTO 0) := (OTHERS =>'0');
     M_AXI_BUSER                         : IN  STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS =>'0');
     M_AXI_BVALID                        : IN  STD_LOGIC := '0';
     M_AXI_BREADY                        : OUT STD_LOGIC := '1';
 
     -- Read Address Channel Signals
     M_AXI_ARID                          : OUT STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_ARADDR                        : OUT STD_LOGIC_VECTOR(32 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_ARLEN                         : OUT STD_LOGIC_VECTOR(8 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_ARSIZE                        : OUT STD_LOGIC_VECTOR(3 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_ARBURST                       : OUT STD_LOGIC_VECTOR(2 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_ARLOCK                        : OUT STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_ARCACHE                       : OUT STD_LOGIC_VECTOR(4 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_ARPROT                        : OUT STD_LOGIC_VECTOR(3 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_ARQOS                         : OUT STD_LOGIC_VECTOR(4 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_ARREGION                      : OUT STD_LOGIC_VECTOR(4 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_ARUSER                        : OUT STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS => '0');
     M_AXI_ARVALID                       : OUT STD_LOGIC := '0';
     M_AXI_ARREADY                       : IN  STD_LOGIC := '0';
 
     -- Read Data Channel Signals
     M_AXI_RID                           : IN  STD_LOGIC_VECTOR(1 - 1 DOWNTO 0)   := (OTHERS =>'0');
     M_AXI_RDATA                         : IN  STD_LOGIC_VECTOR(64 - 1 DOWNTO 0) := (OTHERS =>'0');
     M_AXI_RRESP                         : IN  STD_LOGIC_VECTOR(2 - 1 DOWNTO 0) := (OTHERS =>'0');
     M_AXI_RLAST                         : IN  STD_LOGIC := '0';
     M_AXI_RUSER                         : IN  STD_LOGIC_VECTOR(1 - 1 DOWNTO 0) := (OTHERS =>'0');
     M_AXI_RVALID                        : IN  STD_LOGIC := '0';
     M_AXI_RREADY                        : OUT STD_LOGIC := '0';
 
     -- External Interface Signals
     VFIFO_MM2S_CHANNEL_FULL             : IN  STD_LOGIC_VECTOR(2 - 1 DOWNTO 0) := (OTHERS =>'0');
     VFIFO_S2MM_CHANNEL_FULL             : OUT STD_LOGIC_VECTOR(2 - 1 DOWNTO 0) := (OTHERS =>'0');
     VFIFO_MM2S_CHANNEL_EMPTY            : OUT STD_LOGIC_VECTOR(2 - 1 DOWNTO 0) := (OTHERS =>'1');

     -- Status Signal
     VFIFO_IDLE                          : OUT STD_LOGIC_VECTOR(2 - 1 DOWNTO 0) := (OTHERS =>'1');
 
     -- Interrupt Signals
     VFIFO_MM2S_RRESP_ERR_INTR           : OUT STD_LOGIC := '0';
     VFIFO_S2MM_BRESP_ERR_INTR           : OUT STD_LOGIC := '0';
     VFIFO_S2MM_OVERRUN_ERR_INTR         : OUT STD_LOGIC := '0';
 
     -- Global Signals
     ACLK                                : IN  STD_LOGIC := '0';
     ARESETN                             : IN  STD_LOGIC := '0'
   );

  end component;


begin

  s_aclk_buf: bufg
    PORT map(
      i => ACLK,
      o => s_aclk_i
      );

  exdes_inst : axi_vfifo_64_0x2000000 
    PORT MAP (

      S_AXIS_TVALID                       => S_AXIS_TVALID,
      S_AXIS_TREADY                       => S_AXIS_TREADY,
      S_AXIS_TDATA                        => S_AXIS_TDATA,
      S_AXIS_TSTRB                        => S_AXIS_TSTRB,
      S_AXIS_TKEEP                        => S_AXIS_TKEEP,
      S_AXIS_TLAST                        => S_AXIS_TLAST,
      S_AXIS_TID                          => S_AXIS_TID,
      S_AXIS_TDEST                        => S_AXIS_TDEST,

      M_AXIS_TVALID                       => M_AXIS_TVALID,
      M_AXIS_TREADY                       => M_AXIS_TREADY,
      M_AXIS_TDATA                        => M_AXIS_TDATA,
      M_AXIS_TSTRB                        => M_AXIS_TSTRB,
      M_AXIS_TKEEP                        => M_AXIS_TKEEP,
      M_AXIS_TLAST                        => M_AXIS_TLAST,
      M_AXIS_TID                          => M_AXIS_TID,
      M_AXIS_TDEST                        => M_AXIS_TDEST,

      M_AXI_AWID                          => M_AXI_AWID,
      M_AXI_AWADDR                        => M_AXI_AWADDR,
      M_AXI_AWLEN                         => M_AXI_AWLEN,
      M_AXI_AWSIZE                        => M_AXI_AWSIZE,
      M_AXI_AWBURST                       => M_AXI_AWBURST,
      M_AXI_AWLOCK                        => M_AXI_AWLOCK,
      M_AXI_AWCACHE                       => M_AXI_AWCACHE,
      M_AXI_AWPROT                        => M_AXI_AWPROT,
      M_AXI_AWQOS                         => M_AXI_AWQOS,
      M_AXI_AWREGION                      => M_AXI_AWREGION,
      M_AXI_AWUSER                        => M_AXI_AWUSER,
      M_AXI_AWVALID                       => M_AXI_AWVALID,
      M_AXI_AWREADY                       => M_AXI_AWREADY,

      M_AXI_WDATA                         => M_AXI_WDATA,
      M_AXI_WSTRB                         => M_AXI_WSTRB,
      M_AXI_WLAST                         => M_AXI_WLAST,
      M_AXI_WUSER                         => M_AXI_WUSER,
      M_AXI_WVALID                        => M_AXI_WVALID,
      M_AXI_WREADY                        => M_AXI_WREADY,

      M_AXI_BID                           => M_AXI_BID,
      M_AXI_BRESP                         => M_AXI_BRESP,
      M_AXI_BUSER                         => M_AXI_BUSER,
      M_AXI_BVALID                        => M_AXI_BVALID,
      M_AXI_BREADY                        => M_AXI_BREADY,

      M_AXI_ARID                          => M_AXI_ARID,
      M_AXI_ARADDR                        => M_AXI_ARADDR,
      M_AXI_ARLEN                         => M_AXI_ARLEN,
      M_AXI_ARSIZE                        => M_AXI_ARSIZE,
      M_AXI_ARBURST                       => M_AXI_ARBURST,
      M_AXI_ARLOCK                        => M_AXI_ARLOCK,
      M_AXI_ARCACHE                       => M_AXI_ARCACHE,
      M_AXI_ARPROT                        => M_AXI_ARPROT,
      M_AXI_ARQOS                         => M_AXI_ARQOS,
      M_AXI_ARREGION                      => M_AXI_ARREGION,
      M_AXI_ARUSER                        => M_AXI_ARUSER,
      M_AXI_ARVALID                       => M_AXI_ARVALID,
      M_AXI_ARREADY                       => M_AXI_ARREADY,

      M_AXI_RID                           => M_AXI_RID,
      M_AXI_RDATA                         => M_AXI_RDATA,
      M_AXI_RRESP                         => M_AXI_RRESP,
      M_AXI_RLAST                         => M_AXI_RLAST,
      M_AXI_RUSER                         => M_AXI_RUSER,
      M_AXI_RVALID                        => M_AXI_RVALID,
      M_AXI_RREADY                        => M_AXI_RREADY,

      -- External Interface Signals
      VFIFO_MM2S_CHANNEL_FULL             => VFIFO_MM2S_CHANNEL_FULL,
      VFIFO_MM2S_CHANNEL_EMPTY            => VFIFO_MM2S_CHANNEL_EMPTY,
      VFIFO_S2MM_CHANNEL_FULL             => VFIFO_S2MM_CHANNEL_FULL,

      -- Status Signal
      VFIFO_IDLE                          => VFIFO_IDLE,

      -- Interrupt Signals
      VFIFO_MM2S_RRESP_ERR_INTR           => VFIFO_MM2S_RRESP_ERR_INTR,
      VFIFO_S2MM_BRESP_ERR_INTR           => VFIFO_S2MM_BRESP_ERR_INTR,
      VFIFO_S2MM_OVERRUN_ERR_INTR         => VFIFO_S2MM_OVERRUN_ERR_INTR,

      ACLK                                => s_aclk_i,
      ARESETN                             => ARESETN
    );

end xilinx;
