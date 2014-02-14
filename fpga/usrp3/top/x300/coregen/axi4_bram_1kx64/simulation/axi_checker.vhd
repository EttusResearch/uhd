--------------------------------------------------------------------------------
--
-- BLK MEM GEN v7_3 Core - Stimulus Generator for AXI FULL
--
--------------------------------------------------------------------------------
--
-- (c) Copyright 2006_2012 Xilinx, Inc. All rights reserved.
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
-- Filename: bmg_axi_full_stim_gen.vhd
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
 LIBRARY work;
 USE work.BMG_TB_PKG.ALL;

ENTITY AXI_CHECKER IS
      PORT (
            CLK : IN STD_LOGIC;
            RST : IN STD_LOGIC;
            EN  : IN STD_LOGIC; 
            DATA_IN : IN STD_LOGIC_VECTOR (63 DOWNTO 0);   --OUTPUT VECTOR          
            STATUS : OUT STD_LOGIC:= '0'
	   );
END AXI_CHECKER;

ARCHITECTURE CHECKER_ARCH OF AXI_CHECKER IS
  SIGNAL EXPECTED_DATA : STD_LOGIC_VECTOR(63 DOWNTO 0);
SIGNAL DATA_IN_I: STD_LOGIC_VECTOR(63 DOWNTO 0);
SIGNAL EN_R : STD_LOGIC := '0';

BEGIN

PROCESS(CLK)
BEGIN
  IF(RISING_EDGE(CLK)) THEN
    IF(RST='1') THEN
      DATA_IN_I<=(OTHERS=>'0');
    ELSE
      DATA_IN_I<=DATA_IN;
    END IF;
  END IF;
END PROCESS;


 PROCESS(CLK)
 BEGIN
   IF(RISING_EDGE(CLK)) THEN
     IF(RST= '1') THEN
       EN_R <= '0';
     ELSE
       EN_R <= EN;
      END IF;        
   END IF;
 END PROCESS;

  EXPECTED_DATA_GEN_INST:ENTITY work.DATA_GEN 
      GENERIC MAP ( DATA_GEN_WIDTH =>64,
                    DOUT_WIDTH => 64 ,
		            DATA_PART_CNT => 1,
	                SEED => 2
				  )
	        
      PORT MAP (
            CLK =>CLK,
			RST => RST,
            EN  => EN,
            DATA_OUT => EXPECTED_DATA
        	   );


   PROCESS(CLK)
   BEGIN
     IF(RISING_EDGE(CLK)) THEN
       IF(EN_R='1') THEN
     	 IF(EXPECTED_DATA = DATA_IN_I) THEN
	        STATUS<='0';
	     ELSE
	        STATUS <= '1';
    	 END IF;
       END IF;
     END IF;
   END PROCESS;

   

END ARCHITECTURE;
