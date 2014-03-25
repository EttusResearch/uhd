--------------------------------------------------------------------------------
--
-- BLK MEM GEN v7_3 Core - Protocol Checker for AXI 
--
--------------------------------------------------------------------------------
--
-- (c) Copyright 2006_3010 Xilinx, Inc. All rights reserved.
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
-- Filename: bmg_axi_protocol_chkr.vhd
--
-- Description:
--  Protocol Checker For AXI Configuration
--------------------------------------------------------------------------------
-- Author: IP Solutions Division
--
-- History: Sep 12, 2011 - First Release
--------------------------------------------------------------------------------
--
--------------------------------------------------------------------------------
-- Library Declarations
--------------------------------------------------------------------------------

LIBRARY IEEE;
USE IEEE.STD_LOGIC_1164.ALL;
USE IEEE.STD_LOGIC_ARITH.ALL;
USE IEEE.STD_LOGIC_UNSIGNED.ALL;
USE IEEE.STD_LOGIC_MISC.ALL;

LIBRARY work;
USE work.ALL;
USE work.BMG_TB_PKG.ALL;

ENTITY BMG_AXI_FULL_PROTOCOL_CHKR IS
  PORT (
    S_ACLK                         : IN  STD_LOGIC := '0';
    S_ARESETN                      : IN  STD_LOGIC := '0';
    S_AXI_AWID                     : IN  STD_LOGIC_VECTOR(0 DOWNTO 0) := (OTHERS => '0');
    S_AXI_BID                      : IN  STD_LOGIC_VECTOR(0 DOWNTO 0) := (OTHERS => '0');
    S_AXI_ARID                     : IN  STD_LOGIC_VECTOR(0 DOWNTO 0) := (OTHERS => '0');
    S_AXI_RID                      : IN  STD_LOGIC_VECTOR(0 DOWNTO 0) := (OTHERS => '0');
    S_AXI_AWADDR                   : IN  STD_LOGIC_VECTOR(31 DOWNTO 0) := (OTHERS => '0');
    S_AXI_AWLEN                    : IN  STD_LOGIC_VECTOR(7 DOWNTO 0) := (OTHERS => '0');
    S_AXI_WLAST                    : IN  STD_LOGIC := '0';
    S_AXI_ARLEN                    : IN  STD_LOGIC_VECTOR(8-1 DOWNTO 0) := (OTHERS => '0');
    S_AXI_RLAST                    : IN  STD_LOGIC;
    S_AXI_AWVALID                  : IN  STD_LOGIC := '0';
    S_AXI_AWREADY                  : IN  STD_LOGIC;
    S_AXI_WVALID                   : IN  STD_LOGIC := '0';
    S_AXI_WREADY                   : IN  STD_LOGIC;
    S_AXI_BRESP                    : IN  STD_LOGIC_VECTOR(1 DOWNTO 0);
    S_AXI_BVALID                   : IN  STD_LOGIC;
    S_AXI_BREADY                   : IN  STD_LOGIC := '0';
    S_AXI_ARADDR                   : IN  STD_LOGIC_VECTOR(31 DOWNTO 0) := (OTHERS => '0');
    S_AXI_ARVALID                  : IN  STD_LOGIC := '0';
    S_AXI_ARREADY                  : IN  STD_LOGIC;
    S_AXI_RRESP                    : IN  STD_LOGIC_VECTOR(2-1 DOWNTO 0);
    S_AXI_RVALID                   : IN  STD_LOGIC;
    S_AXI_RREADY                   : IN  STD_LOGIC := '0';
    ERROR_FLAG                     : OUT STD_LOGIC_VECTOR(6 DOWNTO 0) := (OTHERS=>'0')  --Some Signals are kept open intentionally to add logic if the user needs more checks on the AXI Protocol
  );
END BMG_AXI_FULL_PROTOCOL_CHKR;


ARCHITECTURE BEHAVIORAL OF BMG_AXI_FULL_PROTOCOL_CHKR IS
   SIGNAL axi_wready_cnt : STD_LOGIC_VECTOR(3 DOWNTO 0) :=(OTHERS=>'0');
   SIGNAL wready_timeout : STD_LOGIC :='0';
   SIGNAL axi_awready_cnt : STD_LOGIC_VECTOR(3 DOWNTO 0) :=(OTHERS=>'0');
   SIGNAL awready_timeout : STD_LOGIC :='0';
   SIGNAL axi_arready_cnt : STD_LOGIC_VECTOR(3 DOWNTO 0) :=(OTHERS=>'0');
   SIGNAL arready_timeout : STD_LOGIC :='0';
   type memory_type is array (3 downto 0) of std_logic_vector(0 downto 0);
   SIGNAL awid_mem : memory_type :=(others=>(others=>'0'));
   SIGNAL awid_wr_ptr : STD_LOGIC_VECTOR(1 DOWNTO 0):=(OTHERS=>'0');
   SIGNAL awid_rd_ptr : STD_LOGIC_VECTOR(1 DOWNTO 0):=(OTHERS=>'0');
   SIGNAL arid_mem : memory_type :=(others=>(others=>'0'));
   SIGNAL arid_wr_ptr : STD_LOGIC_VECTOR(1 DOWNTO 0):=(OTHERS=>'0');
   SIGNAL arid_rd_ptr : STD_LOGIC_VECTOR(1 DOWNTO 0):=(OTHERS=>'0');

BEGIN
--------------------------------------------------------------------------
-------                AWREADY TIMEOUT CHECKER              --------------
--------------------------------------------------------------------------
  
  awready_timeout <= AND_REDUCE(axi_awready_cnt);

AWREADY_ERROR_FLAG:PROCESS(S_ACLK,S_ARESETN)
  BEGIN
    IF(S_ARESETN = '0') THEN
      ERROR_FLAG(0) <='0';
    ELSIF(RISING_EDGE(S_ACLK)) THEN
       IF(awready_timeout='1') THEN
	 ERROR_FLAG(0)<='1';
       ELSIF(S_AXI_AWREADY='1') THEN
	 ERROR_FLAG(0)<='0';
       END IF;
    END IF;
  END PROCESS;

AWREADY_CNT_PROCESS:  PROCESS(S_ACLK,S_ARESETN)
  BEGIN
    IF(S_ARESETN = '0') THEN
      axi_awready_cnt <= (OTHERS=>'0');
    ELSIF(RISING_EDGE(S_ACLK)) THEN
      IF(S_AXI_AWVALID='1' AND S_AXI_AWREADY='0') THEN
        axi_awready_cnt <= axi_awready_cnt+1;
      ELSIF(S_AXI_AWVALID='1' AND S_AXI_AWREADY='1') THEN
        axi_awready_cnt <= (OTHERS=>'0');
      END IF;
    END IF;   
  END PROCESS;
  
--------------------------------------------------------------------------
-------                WREADY TIMEOUT CHECKER              --------------
--------------------------------------------------------------------------
  
  wready_timeout <= AND_REDUCE(axi_wready_cnt);

  PROCESS(S_ACLK,S_ARESETN)
  BEGIN
    IF(S_ARESETN = '0') THEN
      ERROR_FLAG(1) <='0';
    ELSIF(RISING_EDGE(S_ACLK)) THEN
       IF(wready_timeout='1') THEN
	 ERROR_FLAG(1)<='1';
       ELSIF(S_AXI_WREADY='1') THEN
	 ERROR_FLAG(1)<='0';
       END IF;
    END IF;
  END PROCESS;

  PROCESS(S_ACLK,S_ARESETN)
  BEGIN
    IF(S_ARESETN = '0') THEN
      axi_wready_cnt <= (OTHERS=>'0');
    ELSIF(RISING_EDGE(S_ACLK)) THEN
      IF(S_AXI_WVALID='1' AND S_AXI_WREADY='0') THEN
        axi_wready_cnt <= axi_wready_cnt+1;
      ELSIF(S_AXI_WVALID='1' AND S_AXI_WREADY='1') THEN
        axi_wready_cnt <= (OTHERS=>'0');
      END IF;
    END IF;   
  END PROCESS;
  

--------------------------------------------------------------------------
-------                AWID/BID  CHECKER                    --------------
--------------------------------------------------------------------------

AWID_WR_TO_MEM: PROCESS(S_AClk,S_ARESETN)
  BEGIN
    IF(S_ARESETN = '0') THEN
      awid_wr_ptr <=(OTHERS=>'0');
    ELSIF(RISING_EDGE(S_ACLK)) THEN
      IF(S_AXI_AWVALID='1' AND S_AXI_AWREADY='1') THEN
     	awid_mem(conv_integer(awid_wr_ptr)) <= S_AXI_AWID(0 downto 0);
	    awid_wr_ptr <= awid_wr_ptr+'1';
    END IF;
  END IF;
END PROCESS;

AWID_WID_CHECK: PROCESS(S_ACLK,S_ARESETN)
  BEGIN
    IF(S_ARESETN = '0') THEN
      ERROR_FLAG(2) <='0';
      awid_rd_ptr <= (OTHERS=>'0');
    ELSIF(RISING_EDGE(S_ACLK)) THEN
      IF(S_AXI_BVALID='1') THEN
    	IF(S_AXI_BID = awid_mem(conv_integer(awid_rd_ptr))) THEN
	       awid_rd_ptr <= awid_rd_ptr+'1';
     	ELSE
	       ERROR_FLAG(2) <='1';
    	END IF;
      END IF;
   END IF;
 END PROCESS;




	
ARID_WR_TO_MEM: PROCESS(S_AClk,S_ARESETN)
  BEGIN
    IF(S_ARESETN = '0') THEN
      arid_wr_ptr <=(OTHERS=>'0');
    ELSIF(RISING_EDGE(S_ACLK)) THEN
      IF(S_AXI_ARVALID='1' AND S_AXI_ARREADY='1') THEN
    	arid_mem(conv_integer(arid_wr_ptr)) <= S_AXI_ARID(0 downto 0);
	    arid_wr_ptr <= arid_wr_ptr+'1';
    END IF;
  END IF;
END PROCESS;

ARID_RID_CHECK: PROCESS(S_ACLK,S_ARESETN)
BEGIN
  IF(S_ARESETN = '0') THEN
    ERROR_FLAG(3) <='0';
    arid_rd_ptr <= (OTHERS=>'0');
  ELSIF(RISING_EDGE(S_ACLK)) THEN
    IF(S_AXI_RVALID='1' AND S_AXI_RREADY='1') THEN
      IF(S_AXI_RID = arid_mem(conv_integer(arid_rd_ptr))) THEN
        IF(S_AXI_RLAST='1') THEN
          arid_rd_ptr <= arid_rd_ptr+'1';
        END IF;
      ELSE
         ERROR_FLAG(3) <='1';
      END IF;
    END IF;
  END IF;
END PROCESS;
 
--------------------------------------------------------------------------
-------                ARREADY TIMEOUT CHECKER              --------------
--------------------------------------------------------------------------
  
arready_timeout <= AND_REDUCE(axi_arready_cnt);

ARREADY_ERROR_FLAG:PROCESS(S_ACLK,S_ARESETN)
BEGIN
  IF(S_ARESETN = '0') THEN
    ERROR_FLAG(4) <='0';
  ELSIF(RISING_EDGE(S_ACLK)) THEN
     IF(arready_timeout='1') THEN
        ERROR_FLAG(4)<='1';
     ELSIF(S_AXI_ARREADY='1') THEN
        ERROR_FLAG(4)<='0';
     END IF;
  END IF;
END PROCESS;

ARREADY_CNT_PROCESS:  PROCESS(S_ACLK,S_ARESETN)
BEGIN
  IF(S_ARESETN = '0') THEN
    axi_arready_cnt <= (OTHERS=>'0');
  ELSIF(RISING_EDGE(S_ACLK)) THEN
    IF(S_AXI_ARVALID='1' AND S_AXI_ARREADY='0') THEN
      axi_arready_cnt <= axi_arready_cnt+1;
    ELSIF(S_AXI_ARVALID='1' AND S_AXI_ARREADY='1') THEN
      axi_arready_cnt <= (OTHERS=>'0');
    END IF;
  END IF;   
END PROCESS;

END ARCHITECTURE;
