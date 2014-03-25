 
 
 

 




--------------------------------------------------------------------------------
--
-- BLK MEM GEN v7_3 Core - Synthesizable Testbench
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
-- Filename: axi4_bram_1kx64_synth.vhd
--
-- Description:
--  Synthesizable Testbench
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
USE IEEE.STD_LOGIC_UNSIGNED.ALL;
USE IEEE.STD_LOGIC_ARITH.ALL;
USE IEEE.NUMERIC_STD.ALL;
USE IEEE.STD_LOGIC_MISC.ALL;

LIBRARY STD;
USE STD.TEXTIO.ALL;

--LIBRARY unisim;
--USE unisim.vcomponents.ALL;

LIBRARY work;
USE work.ALL;
USE work.BMG_TB_PKG.ALL;

ENTITY axi4_bram_1kx64_synth IS
PORT(
	CLK_IN     : IN  STD_LOGIC;
    RESET_IN   : IN  STD_LOGIC;
    STATUS     : OUT STD_LOGIC_VECTOR(8 DOWNTO 0) := (OTHERS => '0')   --ERROR STATUS OUT OF FPGA
    );
END ENTITY;

ARCHITECTURE axi4_bram_1kx64_synth_ARCH OF axi4_bram_1kx64_synth IS


COMPONENT axi4_bram_1kx64_exdes 
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


  SIGNAL CLKA: STD_LOGIC := '0';
  SIGNAL RSTA: STD_LOGIC := '0';
  SIGNAL S_ACLK: STD_LOGIC := '0';
  SIGNAL S_ARESETN: STD_LOGIC := '0';
  SIGNAL S_AXI_AWID: STD_LOGIC_VECTOR(0 DOWNTO 0) := (OTHERS => '0');
  SIGNAL S_AXI_AWADDR: STD_LOGIC_VECTOR(31 DOWNTO 0) := (OTHERS => '0');
  SIGNAL S_AXI_AWLEN: STD_LOGIC_VECTOR(7 DOWNTO 0) := (OTHERS => '0');
  SIGNAL S_AXI_AWSIZE: STD_LOGIC_VECTOR(2 DOWNTO 0) := (OTHERS => '0');
  SIGNAL S_AXI_AWBURST: STD_LOGIC_VECTOR(1 DOWNTO 0) := (OTHERS => '0');
  SIGNAL S_AXI_AWVALID: STD_LOGIC := '0';
  SIGNAL S_AXI_AWREADY: STD_LOGIC;
  SIGNAL S_AXI_WDATA: STD_LOGIC_VECTOR( 63 DOWNTO 0) := (OTHERS => '0');
  SIGNAL S_AXI_WSTRB: STD_LOGIC_VECTOR(7 DOWNTO 0) := (OTHERS => '0');
  SIGNAL S_AXI_WLAST: STD_LOGIC := '0';
  SIGNAL S_AXI_WVALID: STD_LOGIC := '0';
  SIGNAL S_AXI_WREADY: STD_LOGIC;
  SIGNAL S_AXI_BID: STD_LOGIC_VECTOR(0 DOWNTO 0) := (OTHERS => '0');
  SIGNAL S_AXI_ARID: STD_LOGIC_VECTOR(0 DOWNTO 0) := (OTHERS => '0');
  SIGNAL S_AXI_RID: STD_LOGIC_VECTOR(0 DOWNTO 0) := (OTHERS => '0');
  SIGNAL S_AXI_BRESP: STD_LOGIC_VECTOR(1 DOWNTO 0);
  SIGNAL S_AXI_BVALID: STD_LOGIC;
  SIGNAL S_AXI_BREADY: STD_LOGIC := '0';
  SIGNAL S_AXI_ARLEN: STD_LOGIC_VECTOR(8-1 DOWNTO 0) := (OTHERS => '0');
  SIGNAL S_AXI_ARSIZE: STD_LOGIC_VECTOR(2 DOWNTO 0) := (OTHERS => '0');
  SIGNAL S_AXI_ARBURST: STD_LOGIC_VECTOR(1 DOWNTO 0) := (OTHERS => '0');
  SIGNAL S_AXI_RLAST: STD_LOGIC;
  SIGNAL S_AXI_ARADDR: STD_LOGIC_VECTOR(31 DOWNTO 0) := (OTHERS => '0');
  SIGNAL S_AXI_ARVALID: STD_LOGIC := '0';
  SIGNAL S_AXI_ARREADY: STD_LOGIC;
  SIGNAL S_AXI_RDATA: STD_LOGIC_VECTOR(63 DOWNTO 0);
  SIGNAL S_AXI_RRESP: STD_LOGIC_VECTOR(1 DOWNTO 0);
  SIGNAL S_AXI_RVALID: STD_LOGIC;
  SIGNAL S_AXI_RREADY: STD_LOGIC := '0';
  SIGNAL CHECKER_EN : STD_LOGIC:='0';
  SIGNAL CHECKER_EN_R : STD_LOGIC:='0';
  SIGNAL STIMULUS_FLOW : STD_LOGIC_VECTOR(22 DOWNTO 0) := (OTHERS =>'0');
  SIGNAL clk_in_i: STD_LOGIC;
  SIGNAL RESET_SYNC_R1 : STD_LOGIC;
  SIGNAL RESET_SYNC_R2 : STD_LOGIC;
  SIGNAL RESET_SYNC_R3 : STD_LOGIC;

  SIGNAL ITER_R0 : STD_LOGIC := '0';
  SIGNAL ITER_R1 : STD_LOGIC := '0';
  SIGNAL ITER_R2 : STD_LOGIC := '0';

  SIGNAL ISSUE_FLAG : STD_LOGIC_VECTOR(7 DOWNTO 0) := (OTHERS => '0');
  SIGNAL ISSUE_FLAG_STATUS : STD_LOGIC_VECTOR(7 DOWNTO 0) := (OTHERS => '0');

  BEGIN

--  clk_buf: bufg
--    PORT map(
--      i => CLK_IN,
--      o => clk_in_i
--    );
   clk_in_i <= CLK_IN;
   CLKA <= clk_in_i;

   RSTA <= RESET_SYNC_R3 AFTER 50 ns;

   S_AClk <= clk_in_i;
   S_ARESETN<=NOT RESET_SYNC_R3 AFTER 50 ns;

   PROCESS(clk_in_i)
   BEGIN
      IF(RISING_EDGE(clk_in_i)) THEN
		 RESET_SYNC_R1 <= RESET_IN;
		 RESET_SYNC_R2 <= RESET_SYNC_R1;
		 RESET_SYNC_R3 <= RESET_SYNC_R2;
	  END IF;
   END PROCESS;


PROCESS(CLKA)
BEGIN
  IF(RISING_EDGE(CLKA)) THEN
    IF(RESET_SYNC_R3='1') THEN
        ISSUE_FLAG_STATUS<= (OTHERS => '0'); 
	  ELSE
        ISSUE_FLAG_STATUS <= ISSUE_FLAG_STATUS OR ISSUE_FLAG;
   END IF;
  END IF;
END PROCESS;

STATUS(7 DOWNTO 0) <= ISSUE_FLAG_STATUS;

   BMG_DATA_CHECKER_INST: ENTITY work.AXI_CHECKER
      PORT MAP (
        CLK     => S_AClk,
	    RST     => RSTA, 
        EN      => CHECKER_EN,
        DATA_IN => S_AXI_RDATA,
        STATUS  => ISSUE_FLAG(0)
	  );




BMG_STIM_GEN_INST: ENTITY work.BMG_STIM_GEN
  PORT MAP (
     S_AClk         => S_AClk,
     S_ARESETN      => S_ARESETN,
     S_AXI_AWADDR   =>S_AXI_AWADDR,
     S_AXI_AWVALID  =>S_AXI_AWVALID,
     S_AXI_AWREADY  =>S_AXI_AWREADY,
     s_axi_awid    => S_AXI_AWID, 
     S_AXI_BID     => S_AXI_BID,
     s_axi_arid    => S_AXI_ARID, 
     S_AXI_RID =>S_AXI_RID,
     S_AXI_AWLEN   => S_AXI_AWLEN,
     S_AXI_AWSIZE  => S_AXI_AWSIZE,
     S_AXI_AWBURST => S_AXI_AWBURST,
     S_AXI_WDATA   =>S_AXI_WDATA,
     S_AXI_WSTRB   =>S_AXI_WSTRB,
     S_AXI_WLAST   => S_AXI_WLAST,
     S_AXI_WVALID   =>S_AXI_WVALID,
     S_AXI_WREADY   =>S_AXI_WREADY,
     S_AXI_BRESP    =>S_AXI_BRESP,
     S_AXI_BVALID   =>S_AXI_BVALID,
     S_AXI_BREADY  =>S_AXI_BREADY,
     S_AXI_ARADDR  =>S_AXI_ARADDR,
     S_AXI_ARVALID =>S_AXI_ARVALID,
     S_AXI_ARREADY =>S_AXI_ARREADY,
     S_AXI_ARLEN   => S_AXI_ARLEN,
     S_AXI_ARSIZE  => S_AXI_ARSIZE,
     S_AXI_ARBURST => S_AXI_ARBURST,
     S_AXI_RDATA =>S_AXI_RDATA,
     S_AXI_RRESP  =>S_AXI_RRESP,
     S_AXI_RVALID => S_AXI_RVALID,
     S_AXI_RREADY =>S_AXI_RREADY,
     S_AXI_RLAST   => S_AXI_RLAST,
     CHECK_RDATA => CHECKER_EN,
     ERROR_FLAG => ISSUE_FLAG(7 DOWNTO 1)
  );

      PROCESS(S_AClk)
      BEGIN
        IF(RISING_EDGE(S_AClk)) THEN
		  IF(RESET_SYNC_R3='1') THEN
			STATUS(8) <= '0';
			iter_r2 <= '0';
			iter_r1 <= '0';
			iter_r0 <= '0';
		  ELSE
			STATUS(8) <= iter_r2;
			iter_r2 <= iter_r1;
			iter_r1 <= iter_r0;
			iter_r0 <= STIMULUS_FLOW(8);
	      END IF;
	    END IF;
      END PROCESS;


      PROCESS(S_AClk)
      BEGIN
        IF(RISING_EDGE(S_AClk)) THEN
		  IF(RESET_SYNC_R3='1') THEN
		      STIMULUS_FLOW <= (OTHERS => '0'); 
           ELSIF(S_AXI_AWVALID='1') THEN
		      STIMULUS_FLOW <= STIMULUS_FLOW+1;
         END IF;
	    END IF;
      END PROCESS;




    BMG_PORT: axi4_bram_1kx64_exdes PORT MAP ( 
    -- AXI BMG Input and Output Port Declarations
      --Global Signals
      S_ACLK     => S_ACLK,
      --AXI Full/Lite Slave
    S_AXI_AWID   =>  S_AXI_AWID, 
    S_AXI_AWADDR =>S_AXI_AWADDR,
    S_AXI_AWLEN  =>S_AXI_AWLEN,
    S_AXI_AWSIZE  => S_AXI_AWSIZE,
    S_AXI_AWBURST =>  S_AXI_AWBURST,
    S_AXI_AWVALID =>S_AXI_AWVALID,
    S_AXI_AWREADY =>S_AXI_AWREADY,
    S_AXI_WDATA   =>S_AXI_WDATA,
    S_AXI_WSTRB   =>S_AXI_WSTRB,
    S_AXI_WLAST   =>S_AXI_WLAST,
    S_AXI_WVALID  =>S_AXI_WVALID,
    S_AXI_WREADY  =>S_AXI_WREADY,
    S_AXI_BID     =>S_AXI_BID,
    S_AXI_BRESP   =>S_AXI_BRESP,
    S_AXI_BVALID  =>S_AXI_BVALID,
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
END ARCHITECTURE;
