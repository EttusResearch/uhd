 
 
 

 



--------------------------------------------------------------------------------
--
-- BLK MEM GEN v7.1 Core - Top-level core wrapper
--
--------------------------------------------------------------------------------
--
-- (c) Copyright 2006-2010 Xilinx, Inc. All rights reserved.
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
-- Filename: axi4_bram_1kx64_exdes.vhd
--
-- Description:
--   This is the actual BMG core wrapper.
--
--------------------------------------------------------------------------------
-- Author: IP Solutions Division
--
-- History: August 31, 2005 - First Release
--------------------------------------------------------------------------------
--
--------------------------------------------------------------------------------
-- Library Declarations
--------------------------------------------------------------------------------

LIBRARY IEEE;
USE IEEE.STD_LOGIC_1164.ALL;
USE IEEE.STD_LOGIC_ARITH.ALL;
USE IEEE.STD_LOGIC_UNSIGNED.ALL;

LIBRARY UNISIM;
USE UNISIM.VCOMPONENTS.ALL;

--------------------------------------------------------------------------------
-- Entity Declaration
--------------------------------------------------------------------------------
ENTITY axi4_bram_1kx64_exdes IS
  PORT (
 -- AXI BMG Input and Output Port Declarations

    -- AXI Global Signals
    S_ACLK                         : IN  STD_LOGIC;

    -- AXI Full/Lite Slave Write (write side)
    S_AXI_AWID                     : IN  STD_LOGIC_VECTOR(0 DOWNTO 0);
    S_AXI_AWADDR                   : IN  STD_LOGIC_VECTOR(31 DOWNTO 0);
    S_AXI_AWLEN                    : IN  STD_LOGIC_VECTOR(7 DOWNTO 0);
    S_AXI_AWSIZE                   : IN  STD_LOGIC_VECTOR(2 DOWNTO 0);
    S_AXI_AWBURST                  : IN  STD_LOGIC_VECTOR(1 DOWNTO 0);
    S_AXI_AWVALID                  : IN  STD_LOGIC;
    S_AXI_AWREADY                  : OUT STD_LOGIC;
    S_AXI_WDATA                    : IN  STD_LOGIC_VECTOR(63  DOWNTO 0);
    S_AXI_WSTRB                    : IN  STD_LOGIC_VECTOR(7  DOWNTO 0);
    S_AXI_WLAST                    : IN  STD_LOGIC;
    S_AXI_WVALID                   : IN  STD_LOGIC;
    S_AXI_WREADY                   : OUT STD_LOGIC;
    S_AXI_BID                      : OUT STD_LOGIC_VECTOR(0  DOWNTO 0);
    S_AXI_BRESP                    : OUT STD_LOGIC_VECTOR(1 DOWNTO 0);
    S_AXI_BVALID                   : OUT STD_LOGIC;
    S_AXI_BREADY                   : IN  STD_LOGIC;

    -- AXI Full/Lite Slave Read (Write side)
    S_AXI_ARID                     : IN  STD_LOGIC_VECTOR(0  DOWNTO 0);
    S_AXI_ARADDR                   : IN  STD_LOGIC_VECTOR(31 DOWNTO 0);
    S_AXI_ARLEN                    : IN  STD_LOGIC_VECTOR(7 DOWNTO 0);
    S_AXI_ARSIZE                   : IN  STD_LOGIC_VECTOR(2 DOWNTO 0);
    S_AXI_ARBURST                  : IN  STD_LOGIC_VECTOR(1 DOWNTO 0);
    S_AXI_ARVALID                  : IN  STD_LOGIC;
    S_AXI_ARREADY                  : OUT STD_LOGIC;
    S_AXI_RID                      : OUT STD_LOGIC_VECTOR(0  DOWNTO 0);
    S_AXI_RDATA                    : OUT STD_LOGIC_VECTOR(63  DOWNTO 0);
    S_AXI_RRESP                    : OUT STD_LOGIC_VECTOR(1 DOWNTO 0);
    S_AXI_RLAST                    : OUT STD_LOGIC;
    S_AXI_RVALID                   : OUT STD_LOGIC;
    S_AXI_RREADY                   : IN  STD_LOGIC;

    -- AXI Full/Lite Sideband Signals
    S_ARESETN                      : IN  STD_LOGIC

  );

END axi4_bram_1kx64_exdes;


ARCHITECTURE xilinx OF axi4_bram_1kx64_exdes IS

  COMPONENT BUFG IS
  PORT (
     I      : IN STD_ULOGIC;
     O      : OUT STD_ULOGIC
  );
  END COMPONENT;

  COMPONENT axi4_bram_1kx64 IS
  PORT (

 -- AXI BMG Input and Output Port Declarations

    -- AXI Global Signals
    S_ACLK                         : IN  STD_LOGIC;

    -- AXI Full/Lite Slave Write (write side)
    S_AXI_AWID                     : IN  STD_LOGIC_VECTOR(0 DOWNTO 0);
    S_AXI_AWADDR                   : IN  STD_LOGIC_VECTOR(31 DOWNTO 0);
    S_AXI_AWLEN                    : IN  STD_LOGIC_VECTOR(7 DOWNTO 0);
    S_AXI_AWSIZE                   : IN  STD_LOGIC_VECTOR(2 DOWNTO 0);
    S_AXI_AWBURST                  : IN  STD_LOGIC_VECTOR(1 DOWNTO 0);
    S_AXI_AWVALID                  : IN  STD_LOGIC;
    S_AXI_AWREADY                  : OUT STD_LOGIC;
    S_AXI_WDATA                    : IN  STD_LOGIC_VECTOR(63  DOWNTO 0);
    S_AXI_WSTRB                    : IN  STD_LOGIC_VECTOR(7  DOWNTO 0);
    S_AXI_WLAST                    : IN  STD_LOGIC;
    S_AXI_WVALID                   : IN  STD_LOGIC;
    S_AXI_WREADY                   : OUT STD_LOGIC;
    S_AXI_BID                      : OUT STD_LOGIC_VECTOR(0  DOWNTO 0);
    S_AXI_BRESP                    : OUT STD_LOGIC_VECTOR(1 DOWNTO 0);
    S_AXI_BVALID                   : OUT STD_LOGIC;
    S_AXI_BREADY                   : IN  STD_LOGIC;

    -- AXI Full/Lite Slave Read (Write side)
    S_AXI_ARID                     : IN  STD_LOGIC_VECTOR(0  DOWNTO 0);
    S_AXI_ARADDR                   : IN  STD_LOGIC_VECTOR(31 DOWNTO 0);
    S_AXI_ARLEN                    : IN  STD_LOGIC_VECTOR(7 DOWNTO 0);
    S_AXI_ARSIZE                   : IN  STD_LOGIC_VECTOR(2 DOWNTO 0);
    S_AXI_ARBURST                  : IN  STD_LOGIC_VECTOR(1 DOWNTO 0);
    S_AXI_ARVALID                  : IN  STD_LOGIC;
    S_AXI_ARREADY                  : OUT STD_LOGIC;
    S_AXI_RID                      : OUT STD_LOGIC_VECTOR(0  DOWNTO 0);
    S_AXI_RDATA                    : OUT STD_LOGIC_VECTOR(63  DOWNTO 0);
    S_AXI_RRESP                    : OUT STD_LOGIC_VECTOR(1 DOWNTO 0);
    S_AXI_RLAST                    : OUT STD_LOGIC;
    S_AXI_RVALID                   : OUT STD_LOGIC;
    S_AXI_RREADY                   : IN  STD_LOGIC;

    -- AXI Full/Lite Sideband Signals
    S_ARESETN                      : IN  STD_LOGIC


  );
  END COMPONENT;

  SIGNAL CLKA_buf     : STD_LOGIC;
  SIGNAL CLKB_buf     : STD_LOGIC;
  SIGNAL S_ACLK_buf   : STD_LOGIC;

BEGIN


  bufg_A : BUFG
    PORT MAP (
     I => S_ACLK,
     O => S_ACLK_buf
     );


  bmg0 : axi4_bram_1kx64
    PORT MAP (
    -- AXI BMG Input and Output Port Declarations
      --Global Signals
      S_AClk              => S_ACLK_buf,

      --AXI Full/Lite Slav=>
    S_AXI_AWID   =>  S_AXI_AWID, 
    S_AXI_AWADDR                =>S_AXI_AWADDR,
    S_AXI_AWLEN                =>S_AXI_AWLEN,
    S_AXI_AWSIZE             => S_AXI_AWSIZE,
    S_AXI_AWBURST          =>  S_AXI_AWBURST,
    S_AXI_AWVALID           =>S_AXI_AWVALID,
    S_AXI_AWREADY          =>S_AXI_AWREADY,
    S_AXI_WDATA           =>S_AXI_WDATA,
    S_AXI_WSTRB          =>S_AXI_WSTRB,
    S_AXI_WLAST         =>S_AXI_WLAST,
    S_AXI_WVALID       =>S_AXI_WVALID,
    S_AXI_WREADY      =>S_AXI_WREADY,
    S_AXI_BID        =>S_AXI_BID,
    S_AXI_BRESP     =>S_AXI_BRESP,
    S_AXI_BVALID   =>S_AXI_BVALID,
    S_AXI_BREADY  =>S_AXI_BREADY,

    -- AXI Full/Lite Slave Read (Write side)
    S_AXI_ARID   =>S_AXI_ARID,
    S_AXI_ARADDR  =>S_AXI_ARADDR,
    S_AXI_ARLEN  =>S_AXI_ARLEN,
    S_AXI_ARSIZE =>S_AXI_ARSIZE,
    S_AXI_ARBURST  =>S_AXI_ARBURST,
    S_AXI_ARVALID =>S_AXI_ARVALID,
    S_AXI_ARREADY =>S_AXI_ARREADY,
    S_AXI_RID    =>S_AXI_RID,
    S_AXI_RDATA =>S_AXI_RDATA,
    S_AXI_RRESP  =>S_AXI_RRESP,
    S_AXI_RLAST  => S_AXI_RLAST,
    S_AXI_RVALID => S_AXI_RVALID,
    S_AXI_RREADY =>S_AXI_RREADY, 

      -- AXI Full/Lite Sid=>
      S_ARESETN           => S_ARESETN


    );

END xilinx;
