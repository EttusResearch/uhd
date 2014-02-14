--------------------------------------------------------------------------------
--
-- BLK MEM GEN v7_3 Core - Stimulus Generator for AXI FULL
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
-- Filename: bmg_stim_gen.vhd
--
-- Description:
--  Stimulus Generator For AXI FULL Configuration
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

ENTITY BMG_STIM_GEN IS
   PORT (
      S_ACLK                         : IN  STD_LOGIC := '0';
      S_ARESETN                      : IN  STD_LOGIC := '0';
      S_AXI_AWID                     : OUT STD_LOGIC_VECTOR(0 DOWNTO 0) := (OTHERS => '0');
      S_AXI_AWADDR                   : OUT STD_LOGIC_VECTOR(31 DOWNTO 0) := (OTHERS => '0');
      S_AXI_AWLEN                    : OUT STD_LOGIC_VECTOR(7 DOWNTO 0) := (OTHERS => '0');
      S_AXI_AWSIZE                   : OUT STD_LOGIC_VECTOR(2 DOWNTO 0) := (OTHERS => '0');
      S_AXI_AWBURST                  : OUT STD_LOGIC_VECTOR(1 DOWNTO 0) := (OTHERS => '0');
      S_AXI_AWVALID                  : OUT STD_LOGIC := '0';
      S_AXI_AWREADY                  : IN  STD_LOGIC;
      S_AXI_WDATA                    : OUT STD_LOGIC_VECTOR(63 DOWNTO 0) := (OTHERS => '0');
      S_AXI_WSTRB                    : OUT STD_LOGIC_VECTOR(7 DOWNTO 0) := (OTHERS => '0');
      S_AXI_WLAST                    : OUT STD_LOGIC := '0';
      S_AXI_WVALID                   : OUT STD_LOGIC := '0';
      S_AXI_WREADY                   : IN  STD_LOGIC;
      S_AXI_BID                      : IN  STD_LOGIC_VECTOR(0 DOWNTO 0) := (OTHERS => '0');
      S_AXI_BRESP                    : IN  STD_LOGIC_VECTOR(1 DOWNTO 0);
      S_AXI_BVALID                   : IN  STD_LOGIC;
      S_AXI_BREADY                   : OUT STD_LOGIC := '0';
      S_AXI_ARID                     : OUT STD_LOGIC_VECTOR(0 DOWNTO 0) := (OTHERS => '0');
      S_AXI_ARADDR                   : OUT STD_LOGIC_VECTOR(31 DOWNTO 0) := (OTHERS => '0');
      S_AXI_ARLEN                    : OUT STD_LOGIC_VECTOR(7 DOWNTO 0) := (OTHERS => '0');
      S_AXI_ARSIZE                   : OUT STD_LOGIC_VECTOR(2 DOWNTO 0) := (OTHERS => '0');
      S_AXI_ARBURST                  : OUT STD_LOGIC_VECTOR(1 DOWNTO 0) := (OTHERS => '0');
      S_AXI_ARVALID                  : OUT STD_LOGIC := '0';
      S_AXI_ARREADY                  : IN  STD_LOGIC;
      S_AXI_RID                      : IN  STD_LOGIC_VECTOR(0 DOWNTO 0) := (OTHERS => '0');
      S_AXI_RDATA                    : IN  STD_LOGIC_VECTOR(63 DOWNTO 0); 
      S_AXI_RRESP                    : IN  STD_LOGIC_VECTOR(1 DOWNTO 0);
      S_AXI_RLAST                    : IN  STD_LOGIC;
      S_AXI_RVALID                   : IN  STD_LOGIC;
      S_AXI_RREADY                   : OUT STD_LOGIC := '0';
      CHECK_RDATA                    : OUT STD_LOGIC := '0';
      ERROR_FLAG                     : OUT STD_LOGIC_VECTOR(6 DOWNTO 0) := (OTHERS=>'0')
   );
END BMG_STIM_GEN;


ARCHITECTURE BEHAVIORAL OF BMG_STIM_GEN IS

CONSTANT WR_RD_DEEP_COUNT      : INTEGER:=2;
CONSTANT WAIT_ENABLE           : STD_LOGIC_VECTOR (1 DOWNTO 0) := "01";
CONSTANT SEND_ADDR_CMD         : STD_LOGIC_VECTOR (1 DOWNTO 0) := "10";
CONSTANT WAIT_BVALID           : STD_LOGIC_VECTOR (1 DOWNTO 0) := "11";
CONSTANT RD_WAIT_ENABLE        : STD_LOGIC_VECTOR (1 DOWNTO 0) := "01";
CONSTANT RD_SEND_ADDR_CMD      : STD_LOGIC_VECTOR (1 DOWNTO 0) := "10";
CONSTANT RD_WAIT_RVALID        : STD_LOGIC_VECTOR (1 DOWNTO 0) := "11";
CONSTANT CNT_ZERO              : STD_LOGIC_VECTOR(7 DOWNTO 0) := (OTHERS => '0');
SIGNAL   S_AXI_RREADY_OSC_I    : STD_LOGIC:= '0';
SIGNAL   RD_COUNT              : INTEGER:= WR_RD_DEEP_COUNT;
SIGNAL   WR_COUNT              : INTEGER:= WR_RD_DEEP_COUNT;
SIGNAL   AXI_WRITE_ADDR        : STD_LOGIC_VECTOR(31 DOWNTO 0) := (OTHERS => '0');
SIGNAL   S_AXI_AWADDR_INT      : STD_LOGIC_VECTOR(31 DOWNTO 0) := (OTHERS => '0');
SIGNAL   S_AXI_ARADDR_INT      : STD_LOGIC_VECTOR(31 DOWNTO 0) := (OTHERS => '0');
SIGNAL   AXI_READ_ADDR         : STD_LOGIC_VECTOR(31 DOWNTO 0) := (OTHERS => '0');
SIGNAL   AXI_ARID_INT          : STD_LOGIC_VECTOR(0 DOWNTO 0):=(OTHERS => '0');
SIGNAL   AXI_AWID_INT          : STD_LOGIC_VECTOR(0 DOWNTO 0):=(OTHERS => '0');
SIGNAL   WDATA_CNT             : STD_LOGIC_VECTOR(7 DOWNTO 0):=(OTHERS=>'0');
SIGNAL   LEN_COUNT             : STD_LOGIC_VECTOR(7 DOWNTO 0):=(OTHERS=>'0');
SIGNAL   RD_LEN_COUNT          : STD_LOGIC_VECTOR(7 DOWNTO 0):=(OTHERS=>'0');
SIGNAL   S_AXI_AWLEN_I         : STD_LOGIC_VECTOR(7 DOWNTO 0):=(OTHERS=>'0');
SIGNAL   S_AXI_ARLEN_I         : STD_LOGIC_VECTOR(7 DOWNTO 0):=(OTHERS=>'0');
SIGNAL   AXI_WRITE_GEN         : STD_LOGIC := '0';
SIGNAL   AXI_READ_GEN          : STD_LOGIC := '0';
SIGNAL   AXI_WDATA_GEN         : STD_LOGIC := '0';
SIGNAL   DO_WRITE              : STD_LOGIC := '0';
SIGNAL   DO_READ               : STD_LOGIC := '0';
SIGNAL   RD_CURRENT_STATE      : STD_LOGIC_VECTOR (1 DOWNTO 0) := RD_WAIT_ENABLE;
SIGNAL   RD_NEXT_STATE         : STD_LOGIC_VECTOR (1 DOWNTO 0) := RD_WAIT_ENABLE;
SIGNAL   CURRENT_STATE         : STD_LOGIC_VECTOR (1 DOWNTO 0) := WAIT_ENABLE;
SIGNAL   NEXT_STATE            : STD_LOGIC_VECTOR (1 DOWNTO 0) := WAIT_ENABLE;
SIGNAL   AXI_WDATA             : STD_LOGIC_VECTOR(63 DOWNTO 0):= (OTHERS => '0');
SIGNAL   S_AWVALID_I           : STD_LOGIC := '0';
SIGNAL   S_ARVALID_I           : STD_LOGIC := '0';
SIGNAL   S_WVALID_I            : STD_LOGIC := '0';
SIGNAL   RST_INT               : STD_LOGIC :='0';
SIGNAL   S_AXI_AWVALID_I       : STD_LOGIC:='0';
SIGNAL   S_AXI_WVALID_I        : STD_LOGIC:='0';
SIGNAL   S_AXI_RREADY_I        : STD_LOGIC:= '0';
SIGNAL   RREADY_COUNT          : STD_LOGIC_VECTOR(1 DOWNTO 0) := (OTHERS=>'0');
SIGNAL   RD_COMPLETE           : STD_LOGIC:='0';
SIGNAL   WR_COMPLETE           : STD_LOGIC:='0';
SIGNAL   S_AXI_WLAST_I         : STD_LOGIC:='0';
SIGNAL   AXI_NEXT_PKT_ADDR     : STD_LOGIC :='0';
SIGNAL   AXI_NEXT_RD_ADDR      : STD_LOGIC :='0';
SIGNAL   S_AXI_BREADY_I        : STD_LOGIC:='0';


CONSTANT NUM_OF_BYTES : INTEGER := 8 ;

CONSTANT C_RANGE : INTEGER := 3;

CONSTANT AXI_DEPTH: INTEGER:= 8192;



BEGIN

  S_AXI_AWID <= AXI_AWID_INT;
  S_AXI_ARID <= AXI_ARID_INT;

  RST_INT <= NOT S_ARESETN;


 

REGISTER_FSM: 
  PROCESS (S_ACLK, S_ARESETN)
  BEGIN
    IF(S_ARESETN = '0') THEN
      CURRENT_STATE    <= WAIT_ENABLE;
      RD_CURRENT_STATE <= RD_WAIT_ENABLE;
    ELSIF(RISING_EDGE(S_ACLK)) THEN
      CURRENT_STATE    <= NEXT_STATE;
      RD_CURRENT_STATE <= RD_NEXT_STATE;
    END IF;
  END PROCESS;

  PROCESS (S_ACLK, S_ARESETN)
  BEGIN
    IF(S_ARESETN = '0') THEN
      AXI_AWID_INT <= (OTHERS => '0');
      AXI_ARID_INT <= (OTHERS => '0');
    ELSIF(RISING_EDGE(S_ACLK)) THEN
      IF(AXI_WRITE_GEN='1') THEN
        AXI_AWID_INT <= AXI_AWID_INT + '1';
      END IF;
      IF(AXI_READ_GEN='1') THEN      
        AXI_ARID_INT <= AXI_ARID_INT + '1';
      END IF;
    END IF;
 END PROCESS;

WR_COMPLETE <= '1' WHEN (WR_COUNT=WR_RD_DEEP_COUNT AND S_AXI_WLAST_I='1') ELSE '0';

WR_COUNT_PACKET: PROCESS(S_ACLK, S_ARESETN)
BEGIN
   IF(S_ARESETN = '0') THEN
     WR_COUNT <= 0;
   ELSIF(RISING_EDGE(S_ACLK)) THEN
     IF(S_AXI_AWREADY='1' AND S_AXI_AWVALID_I='1') THEN
       WR_COUNT <= WR_COUNT+1;
     END IF;
     IF(WR_COUNT=WR_RD_DEEP_COUNT AND S_AXI_WLAST_I='1') THEN
       WR_COUNT<=0;
     END IF;
   END IF;
END PROCESS;

RD_COMPLETE <= '1' WHEN (RD_COUNT=WR_RD_DEEP_COUNT AND (S_AXI_RLAST='1' AND S_AXI_RVALID='1' AND S_AXI_RREADY_I='1')) ELSE '0';

RD_COUNT_PACKET: PROCESS(S_ACLK, S_ARESETN)
BEGIN
   IF(S_ARESETN = '0') THEN
       RD_COUNT <=0;
   ELSIF(RISING_EDGE(S_ACLK)) THEN
     IF(S_AXI_ARREADY='1' AND S_ARVALID_I='1') THEN
       RD_COUNT <= RD_COUNT+1;
     END IF;
     IF(RD_COUNT=WR_RD_DEEP_COUNT AND (S_AXI_RLAST='1' AND S_AXI_RVALID='1' AND S_AXI_RREADY_I='1')) THEN
       RD_COUNT<=0;
     END IF;
   END IF;
END PROCESS;

RD_FSM_EN: PROCESS(S_ACLK)
BEGIN
  IF(S_ARESETN='0') THEN
      DO_READ <='0';
  ELSIF(RISING_EDGE(S_ACLK)) THEN
      DO_READ <= (WR_COMPLETE);
  END IF;
END PROCESS;

WR_FSM_EN: PROCESS(S_ACLK)
BEGIN
  IF(S_ARESETN='0') THEN
      DO_WRITE <='1';
  ELSIF(RISING_EDGE(S_ACLK)) THEN
      DO_WRITE <=  (RD_COMPLETE);
  END IF;
END PROCESS;

--AXI_NEXT_PKT_ADDR <='1' WHEN (AXI_WRITE_GEN='1' AND ((AXI_WRITE_ADDR(11 DOWNTO 0)+((LEN_COUNT)*CONV_STD_LOGIC_VECTOR(NUM_OF_BYTES,6)))>=4096)) ELSE '0';
AXI_NEXT_PKT_ADDR <='1' WHEN (AXI_WRITE_GEN='1' AND (((AXI_WRITE_ADDR(11 DOWNTO 0)+((LEN_COUNT)*CONV_STD_LOGIC_VECTOR(NUM_OF_BYTES,6)))>=4096) OR ((AXI_WRITE_ADDR(11 DOWNTO 0)+((LEN_COUNT)*CONV_STD_LOGIC_VECTOR(NUM_OF_BYTES,6)))>=AXI_DEPTH))) ELSE '0';

AXI_WR_ADDR_GEN: PROCESS(S_ACLK,S_ARESETN) 
BEGIN
  IF(S_ARESETN='0') THEN
    AXI_WRITE_ADDR <= (OTHERS=>'0');
  ELSIF(RISING_EDGE(S_ACLK)) THEN
     IF((AXI_WRITE_ADDR>=AXI_DEPTH ) ) THEN
       AXI_WRITE_ADDR <= (OTHERS=>'0');
     ELSIF(S_AXI_WREADY='1' AND S_AXI_WVALID_I='1') THEN
       AXI_WRITE_ADDR <= AXI_WRITE_ADDR + NUM_OF_BYTES;
     END IF;
  END IF;
END PROCESS;  

PROCESS(S_ACLK,S_ARESETN)
BEGIN
  IF(S_ARESETN = '0') THEN
    S_AXI_AWLEN_I <= (OTHERS=> '0');
  ELSIF(RISING_EDGE(S_ACLK)) THEN
    IF(AXI_WRITE_GEN='1') THEN
      IF(AXI_NEXT_PKT_ADDR='0') THEN
	S_AXI_AWLEN_I(3 DOWNTO 0) <=  AXI_WRITE_ADDR(7 DOWNTO 4);
      ELSE
	S_AXI_AWLEN_I(3 DOWNTO 0) <= (OTHERS=> '0');
      END IF;	
    END IF;
  END IF;
END PROCESS;

-- AXI_NEXT_RD_ADDR <='1' WHEN (AXI_READ_GEN='1' AND ((AXI_READ_ADDR(11 DOWNTO 0)+((RD_LEN_COUNT)*CONV_STD_LOGIC_VECTOR(NUM_OF_BYTES,6)))>=4096)) ELSE '0';
AXI_NEXT_RD_ADDR <='1' WHEN (AXI_READ_GEN='1' AND (((AXI_READ_ADDR(11 DOWNTO 0)+((RD_LEN_COUNT)*CONV_STD_LOGIC_VECTOR(NUM_OF_BYTES,6)))>=4096) OR ((AXI_READ_ADDR(11 DOWNTO 0)+((RD_LEN_COUNT)*CONV_STD_LOGIC_VECTOR(NUM_OF_BYTES,6)))>=AXI_DEPTH))) ELSE '0';

PROCESS(S_ACLK,S_ARESETN)
BEGIN
  IF(S_ARESETN = '0') THEN
    S_AXI_ARLEN_I <= (OTHERS=> '0');
  ELSIF(RISING_EDGE(S_ACLK)) THEN
    IF(AXI_READ_GEN='1') THEN
      IF(AXI_NEXT_RD_ADDR='0') THEN
	S_AXI_ARLEN_I(3 DOWNTO 0) <=  AXI_READ_ADDR(7 DOWNTO 4);
      ELSE
	S_AXI_ARLEN_I(3 DOWNTO 0) <= (OTHERS=> '0');
      END IF;	
    END IF;
  END IF;
END PROCESS;

LEN_COUNT(3 DOWNTO 0)    <= AXI_WRITE_ADDR(7 DOWNTO 4);
AXI_WDATA_GEN            <= S_AWVALID_I OR (S_AXI_WREADY AND S_AXI_WVALID_I AND (NOT (S_AXI_WLAST_I)))  ;
S_AXI_AWLEN              <= (OTHERS => '0') AFTER 50 ns WHEN (S_AWVALID_I ='0') ELSE S_AXI_AWLEN_I AFTER 50 ns; 
RD_LEN_COUNT(3 DOWNTO 0) <= AXI_READ_ADDR(7 DOWNTO 4);
S_AXI_ARLEN              <= (OTHERS => '0') AFTER 50 ns WHEN (S_ARVALID_I ='0') ELSE S_AXI_ARLEN_I AFTER 50 ns; 
S_AXI_AWSIZE             <= CONV_STD_LOGIC_VECTOR(C_RANGE,3);
S_AXI_ARSIZE             <= CONV_STD_LOGIC_VECTOR(C_RANGE,3);
S_AXI_AWBURST            <= "01";
S_AXI_ARBURST            <= "01";

WDATA_COUNT: PROCESS(S_ACLK,S_ARESETN)
BEGIN
  IF(S_ARESETN='0') THEN 
    WDATA_CNT<=(OTHERS=>'0');
  ELSIF(RISING_EDGE(S_ACLK))  THEN
    IF(S_AWVALID_I='1') THEN
      WDATA_CNT<=S_AXI_AWLEN_I;
    ELSIF((S_AXI_WREADY='1' AND S_AXI_WVALID_I='1'))  THEN
      WDATA_CNT<=WDATA_CNT-1;
    END IF;
  END IF;
END PROCESS;

S_AXI_WLAST_I<= '1' WHEN (WDATA_CNT =CNT_ZERO)  ELSE '0';
S_AXI_WLAST <= S_AXI_WLAST_I AFTER 50 ns;


AXI_WR_DATA_GEN_INST_A:ENTITY work.DATA_GEN 
   GENERIC MAP (
      DATA_GEN_WIDTH => 64,
      DOUT_WIDTH     => 64 ,
      DATA_PART_CNT  => 1,
      SEED           => 2
   )
   PORT MAP (
      CLK      =>S_ACLK,
      RST      => RST_INT,
      EN       => AXI_WDATA_GEN, 
      DATA_OUT => AXI_WDATA          
   );


AWVALID_GEN: PROCESS(S_ACLK,S_ARESETN)
BEGIN
   IF(S_ARESETN ='0') THEN
     S_AWVALID_I <= '0';
   ELSIF( RISING_EDGE(S_ACLK)) THEN
      IF(AXI_WRITE_GEN='1') THEN
	S_AWVALID_I <= '1';
      ELSIF(S_AXI_AWVALID_I='1' AND S_AXI_AWREADY='1') THEN
        S_AWVALID_I <='0';
      END IF;
   END IF;
END PROCESS;

WVALID_GEN: PROCESS(S_ACLK,S_ARESETN)
BEGIN
   IF(S_ARESETN ='0') THEN
      S_WVALID_I <= '0';
   ELSIF( RISING_EDGE(S_ACLK)) THEN
      IF(AXI_WDATA_GEN='1') THEN
	     S_WVALID_I <= '1';
      ELSIF(S_AXI_WVALID_I='1' AND S_AXI_WREADY='1') THEN
         S_WVALID_I <='0';
      END IF;
   END IF;
END PROCESS;



S_AXI_AWADDR_INT <= (OTHERS=>'0') WHEN ( S_AWVALID_I = '0') ELSE AXI_WRITE_ADDR; 
S_AXI_AWADDR     <= S_AXI_AWADDR_INT AFTER 50 ns;
S_AXI_AWVALID_I  <= S_AWVALID_I;
S_AXI_AWVALID    <= S_AXI_AWVALID_I AFTER 50 ns;
S_AXI_WVALID_I   <= S_WVALID_I;
S_AXI_WVALID     <= S_AXI_WVALID_I AFTER 50 ns;
S_AXI_WDATA      <= (OTHERS=>'0')  AFTER 50 ns WHEN ( S_WVALID_I ='0') ELSE AXI_WDATA AFTER 50 ns;
S_AXI_BREADY_I   <= '1';
S_AXI_BREADY     <= S_AXI_BREADY_I;
S_AXI_WSTRB      <= (OTHERS => '1');

 WRITE_FSM: PROCESS(CURRENT_STATE,DO_WRITE,S_AXI_BVALID,S_AXI_WREADY,S_AXI_WVALID_I,WR_COUNT,S_AXI_WLAST_I)
 BEGIN
   CASE CURRENT_STATE IS 
     WHEN WAIT_ENABLE =>
       AXI_WRITE_GEN <= '0';
       IF(DO_WRITE = '1') THEN
	      NEXT_STATE <= SEND_ADDR_CMD;
       ELSE
         NEXT_STATE<= WAIT_ENABLE;
       END IF;

     WHEN SEND_ADDR_CMD =>
       AXI_WRITE_GEN <= '1';
	   NEXT_STATE <= WAIT_BVALID;
	
     WHEN WAIT_BVALID =>
       AXI_WRITE_GEN <= '0';
       IF(S_AXI_WREADY='1' AND S_AXI_WVALID_I='1' AND S_AXI_WLAST_I='1') THEN
	      IF(WR_COUNT=WR_RD_DEEP_COUNT) THEN
            NEXT_STATE <= WAIT_ENABLE;
	      ELSE
	        NEXT_STATE <= SEND_ADDR_CMD;
	      END IF;
	   ELSE 
	     NEXT_STATE <= WAIT_BVALID;
	   END IF; 

     WHEN OTHERS => 
          NEXT_STATE <= WAIT_ENABLE;
          AXI_WRITE_GEN <= '0';

   END CASE;
 END PROCESS;

 AXI_RD_ADDR_GEN: PROCESS(S_ACLK,S_ARESETN) 
 BEGIN
   IF(S_ARESETN='0') THEN
     AXI_READ_ADDR <= (OTHERS=>'0');
   ELSIF(RISING_EDGE(S_ACLK)) THEN
     IF((AXI_READ_ADDR>=AXI_DEPTH ) ) THEN
        AXI_READ_ADDR <= (OTHERS=>'0');
      ELSIF(S_AXI_RREADY_I='1' AND S_AXI_RVALID='1') THEN
        AXI_READ_ADDR <= AXI_READ_ADDR + NUM_OF_BYTES;
      END IF;
   END IF;
 END PROCESS;  

S_AXI_RREADY  <= S_AXI_RREADY_I AFTER 50 ns;
S_AXI_RREADY_I<= S_AXI_RREADY_OSC_I;

PROCESS(S_ACLK,S_ARESETN)
BEGIN
  IF(S_ARESETN = '0') THEN
    RREADY_COUNT <=(OTHERS=>'0');
    S_AXI_RREADY_OSC_I <='0';
  ELSIF(RISING_EDGE(S_ACLK)) THEN
    S_AXI_RREADY_OSC_I <=RREADY_COUNT(0) AND RREADY_COUNT(1);
    RREADY_COUNT<= RREADY_COUNT +1;
  END IF;
END PROCESS;

CHECK_RDATA <= S_AXI_RREADY_I AND S_AXI_RVALID;

 READ_FSM: PROCESS(RD_CURRENT_STATE,DO_READ,S_AXI_RVALID,S_AXI_RREADY_I,RD_COUNT,S_AXI_RLAST)
 BEGIN
   CASE RD_CURRENT_STATE IS 
 
     WHEN RD_WAIT_ENABLE =>
       AXI_READ_GEN <= '0';
       IF(DO_READ = '1') THEN
     	 RD_NEXT_STATE <= RD_SEND_ADDR_CMD;
       ELSE
         RD_NEXT_STATE <= RD_WAIT_ENABLE;
       END IF;

     WHEN RD_SEND_ADDR_CMD =>
       AXI_READ_GEN <= '1';
	   RD_NEXT_STATE <= RD_WAIT_RVALID;
	
     WHEN RD_WAIT_RVALID =>
       AXI_READ_GEN <= '0';
       IF(S_AXI_RVALID='1' AND S_AXI_RREADY_I='1' AND S_AXI_RLAST='1') THEN
	     IF(RD_COUNT = WR_RD_DEEP_COUNT ) THEN
           RD_NEXT_STATE <= RD_WAIT_ENABLE;
	     ELSE
           RD_NEXT_STATE <= RD_SEND_ADDR_CMD;
	     END IF;
	   ELSE
         RD_NEXT_STATE <= RD_WAIT_RVALID;
	   END IF;

     WHEN OTHERS => 
       AXI_READ_GEN <= '0';
       RD_NEXT_STATE <= RD_WAIT_ENABLE;

   END CASE;
END PROCESS;
 
S_AXI_ARADDR_INT <= (OTHERS=>'0') WHEN ( S_ARVALID_I = '0') ELSE AXI_READ_ADDR; 
S_AXI_ARADDR     <= S_AXI_ARADDR_INT AFTER 50 ns; 
S_AXI_ARVALID    <= S_ARVALID_I AFTER 50 ns;

ARVALID_GEN: PROCESS(S_ACLK,S_ARESETN)
BEGIN
   IF(S_ARESETN ='0') THEN
     S_ARVALID_I <= '0';
   ELSIF( RISING_EDGE(S_ACLK)) THEN
      IF(AXI_READ_GEN='1') THEN
	    S_ARVALID_I <= '1';
      ELSIF(S_ARVALID_I='1' AND S_AXI_ARREADY='1') THEN
        S_ARVALID_I <='0';
      END IF;
   END IF;
END PROCESS;

BMG_AXI_FULL_PROT_CHKER_INST: ENTITY work.BMG_AXI_FULL_PROTOCOL_CHKR
   PORT MAP (
     S_ACLK         => S_ACLK,
     S_ARESETN      => S_ARESETN,
     S_AXI_AWADDR   => S_AXI_AWADDR_INT,
     S_AXI_AWVALID  => S_AXI_AWVALID_I,
     S_AXI_AWREADY  => S_AXI_AWREADY,
     S_AXI_AWID     => AXI_AWID_INT, 
     S_AXI_BID      => S_AXI_BID,
     S_AXI_ARID     => AXI_ARID_INT, 
     S_AXI_RID      => S_AXI_RID,
     S_AXI_AWLEN    => S_AXI_AWLEN_I,
     S_AXI_WLAST    => S_AXI_WLAST_I,
     S_AXI_WVALID   => S_AXI_WVALID_I,
     S_AXI_WREADY   => S_AXI_WREADY,
     S_AXI_BRESP    => S_AXI_BRESP,
     S_AXI_BVALID   => S_AXI_BVALID,
     S_AXI_BREADY   => '1',
     S_AXI_ARADDR   => S_AXI_ARADDR_INT,
     S_AXI_ARVALID  => S_ARVALID_I,
     S_AXI_ARREADY  => S_AXI_ARREADY,
     S_AXI_ARLEN    => S_AXI_ARLEN_I,
     S_AXI_RRESP    => S_AXI_RRESP,
     S_AXI_RVALID   => S_AXI_RVALID,
     S_AXI_RREADY   => S_AXI_RREADY_I,
     S_AXI_RLAST    => S_AXI_RLAST,
     ERROR_FLAG     => ERROR_FLAG
   );
END ARCHITECTURE;
