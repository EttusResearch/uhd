--------------------------------------------------------------------------------
--
-- FIFO Generator Core Demo Testbench 
--
--------------------------------------------------------------------------------
--
-- (c) Copyright 2009 - 2010 Xilinx, Inc. All rights reserved.
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
-- Filename: axi64_8k_2clk_fifo_synth.vhd
--
-- Description:
--   This is the demo testbench for fifo_generator core.
--
--------------------------------------------------------------------------------
-- Library Declarations
--------------------------------------------------------------------------------


LIBRARY ieee;
USE ieee.STD_LOGIC_1164.ALL;
USE ieee.STD_LOGIC_unsigned.ALL;
USE IEEE.STD_LOGIC_arith.ALL;
USE ieee.numeric_std.ALL;
USE ieee.STD_LOGIC_misc.ALL;

LIBRARY std;
USE std.textio.ALL;

LIBRARY work;
USE work.axi64_8k_2clk_fifo_pkg.ALL;

--------------------------------------------------------------------------------
-- Entity Declaration
--------------------------------------------------------------------------------
ENTITY axi64_8k_2clk_fifo_synth IS
  GENERIC(
  	   FREEZEON_ERROR : INTEGER := 0;
	   TB_STOP_CNT    : INTEGER := 0;
	   TB_SEED        : INTEGER := 1
	 );
  PORT(
	S_ACLK     :  IN  STD_LOGIC;
	M_ACLK     :  IN  STD_LOGIC;
        RESET      :  IN  STD_LOGIC;
        SIM_DONE   :  OUT STD_LOGIC;
        STATUS     :  OUT STD_LOGIC_VECTOR(7 DOWNTO 0)
      );
END ENTITY;

ARCHITECTURE simulation_arch OF axi64_8k_2clk_fifo_synth IS
    CONSTANT TDATA_OFFSET      : INTEGER := if_then_else(1 = 1,69-64,69);
    CONSTANT TSTRB_OFFSET      : INTEGER := if_then_else(0 = 1,TDATA_OFFSET-8,TDATA_OFFSET);
    CONSTANT TKEEP_OFFSET      : INTEGER := if_then_else(0 = 1,TSTRB_OFFSET-8,TSTRB_OFFSET);
    CONSTANT TID_OFFSET        : INTEGER := if_then_else(0 = 1,TKEEP_OFFSET-8,TKEEP_OFFSET);
    CONSTANT TDEST_OFFSET      : INTEGER := if_then_else(0 = 1,TID_OFFSET-4,TID_OFFSET);
    CONSTANT TLAST_OFFSET      : INTEGER := if_then_else(1 = 1,TDEST_OFFSET-4,TDEST_OFFSET);

    -- FIFO interface signal declarations
    SIGNAL s_aresetn                      :   STD_LOGIC;
    SIGNAL m_axis_tvalid                  :   STD_LOGIC;
    SIGNAL m_axis_tready                  :   STD_LOGIC;
    SIGNAL m_axis_tdata                   :   STD_LOGIC_VECTOR(64-1 DOWNTO 0);
    SIGNAL m_axis_tlast                   :   STD_LOGIC;
    SIGNAL m_axis_tuser                   :   STD_LOGIC_VECTOR(4-1 DOWNTO 0);
    SIGNAL s_axis_tvalid                  :   STD_LOGIC;
    SIGNAL s_axis_tready                  :   STD_LOGIC;
    SIGNAL s_axis_tdata                   :   STD_LOGIC_VECTOR(64-1 DOWNTO 0);
    SIGNAL s_axis_tlast                   :   STD_LOGIC;
    SIGNAL s_axis_tuser                   :   STD_LOGIC_VECTOR(4-1 DOWNTO 0);
    SIGNAL axis_wr_data_count             :   STD_LOGIC_VECTOR(10 DOWNTO 0);
    SIGNAL axis_rd_data_count             :   STD_LOGIC_VECTOR(10 DOWNTO 0);
    SIGNAL m_aclk_i		          :   STD_LOGIC;
    SIGNAL s_aclk_i		          :   STD_LOGIC;
   -- TB Signals
    SIGNAL prc_we_i                       :   STD_LOGIC := '0';
    SIGNAL prc_re_i                       :   STD_LOGIC := '0';
    SIGNAL dout_chk_i                     :   STD_LOGIC := '0';
    SIGNAL rst_int_rd                     :   STD_LOGIC := '0';
    SIGNAL rst_int_wr                     :   STD_LOGIC := '0';
    SIGNAL rst_s_wr1                      :   STD_LOGIC := '0';
    SIGNAL rst_s_wr2                      :   STD_LOGIC := '0';
    SIGNAL rst_gen_rd                     :   STD_LOGIC_VECTOR(7 DOWNTO 0) := (OTHERS => '0');
    SIGNAL rst_s_wr3                      :   STD_LOGIC := '0';
    SIGNAL rst_s_rd                       :   STD_LOGIC := '0';
    SIGNAL reset_en                       :   STD_LOGIC := '0';
    SIGNAL din_axis                       :   STD_LOGIC_VECTOR(69-1 DOWNTO 0);
    SIGNAL dout_axis                      :   STD_LOGIC_VECTOR(69-1 DOWNTO 0);
    SIGNAL wr_en_axis                     :   STD_LOGIC := '0';
    SIGNAL rd_en_axis                     :   STD_LOGIC := '0';
    SIGNAL full_axis                      :   STD_LOGIC := '0';
    SIGNAL empty_axis                     :   STD_LOGIC := '0';
    SIGNAL status_axis                    :   STD_LOGIC_VECTOR(7 DOWNTO 0):="00000000";
    SIGNAL rst_async_wr1                  :   STD_LOGIC := '0'; 
    SIGNAL rst_async_wr2                  :   STD_LOGIC := '0'; 
    SIGNAL rst_async_wr3                  :   STD_LOGIC := '0'; 
    SIGNAL rst_async_rd1                  :   STD_LOGIC := '0'; 
    SIGNAL rst_async_rd2                  :   STD_LOGIC := '0'; 
    SIGNAL rst_async_rd3                  :   STD_LOGIC := '0'; 


 BEGIN  

   ---- Reset generation logic -----
   rst_int_wr          <= rst_async_wr3 OR rst_s_wr3;
   rst_int_rd          <= rst_async_rd3 OR rst_s_rd;

   --Testbench reset synchronization
   PROCESS(m_aclk_i,RESET)
   BEGIN
     IF(RESET = '1') THEN
       rst_async_rd1    <= '1';
       rst_async_rd2    <= '1';
       rst_async_rd3    <= '1';
     ELSIF(m_aclk_i'event AND m_aclk_i='1') THEN
       rst_async_rd1    <= RESET;
       rst_async_rd2    <= rst_async_rd1;
       rst_async_rd3    <= rst_async_rd2;
     END IF;
   END PROCESS;

   PROCESS(s_aclk_i,RESET)
   BEGIN
     IF(RESET = '1') THEN
       rst_async_wr1  <= '1';
       rst_async_wr2  <= '1';
       rst_async_wr3  <= '1';
     ELSIF(s_aclk_i'event AND s_aclk_i='1') THEN
       rst_async_wr1  <= RESET;
       rst_async_wr2  <= rst_async_wr1;
       rst_async_wr3  <= rst_async_wr2;
     END IF;
   END PROCESS;

   --Soft reset for core and testbench
   PROCESS(m_aclk_i)
   BEGIN 
     IF(m_aclk_i'event AND m_aclk_i='1') THEN
       rst_gen_rd      <= rst_gen_rd + "1";
       IF(reset_en = '1' AND AND_REDUCE(rst_gen_rd) = '1') THEN
         rst_s_rd      <= '1';
         assert false
         report "Reset applied..Memory Collision checks are not valid"
         severity note;
       ELSE
         IF(AND_REDUCE(rst_gen_rd)  = '1' AND rst_s_rd = '1') THEN
           rst_s_rd    <= '0';
         END IF;
       END IF;
     END IF;
   END PROCESS;
   
   PROCESS(s_aclk_i)
   BEGIN 
       IF(s_aclk_i'event AND s_aclk_i='1') THEN
         rst_s_wr1   <= rst_s_rd; 
         rst_s_wr2   <= rst_s_wr1; 
         rst_s_wr3   <= rst_s_wr2;
         IF(rst_s_wr3 = '1' AND rst_s_wr2 = '0') THEN
           assert false
           report "Reset removed..Memory Collision checks are valid"
           severity note;
         END IF;
       END IF;
   END PROCESS;
   ------------------
   
   ---- Clock buffers for testbench ----
  s_aclk_i <= S_ACLK;
  m_aclk_i <= M_ACLK;
   ------------------
    
    s_aresetn 	              <= NOT (RESET OR rst_s_rd) AFTER 12 ns;




    S_AXIS_TVALID             <= wr_en_axis;
    M_AXIS_TREADY             <= rd_en_axis;
    full_axis                 <= NOT s_axis_tready;
    empty_axis                <= NOT m_axis_tvalid;

    fg_dg_axis: axi64_8k_2clk_fifo_dgen
      GENERIC MAP (  
          	 C_DIN_WIDTH      => 69,
		 C_DOUT_WIDTH     => 69,
		 TB_SEED          => TB_SEED, 
 		 C_CH_TYPE        => 0
                  )
      PORT MAP (  
                RESET            => rst_int_wr,
                WR_CLK           => s_aclk_i,
		PRC_WR_EN        => prc_we_i,
                FULL             => full_axis,
                WR_EN            => wr_en_axis,
                WR_DATA          => din_axis
	       );

   fg_dv_axis: axi64_8k_2clk_fifo_dverif
    GENERIC MAP (  
	       C_DOUT_WIDTH       => 69,
	       C_DIN_WIDTH        => 69,
	       C_USE_EMBEDDED_REG => 0,
	       TB_SEED            => TB_SEED, 
 	       C_CH_TYPE          => 0	 
	       )
     PORT MAP(
              RESET               => rst_int_rd,
              RD_CLK              => m_aclk_i,
	      PRC_RD_EN           => prc_re_i,
              RD_EN               => rd_en_axis,
	      EMPTY               => empty_axis,
	      DATA_OUT            => dout_axis,
	      DOUT_CHK            => dout_chk_i
	    );

    fg_pc_axis: axi64_8k_2clk_fifo_pctrl
    GENERIC MAP (  
              AXI_CHANNEL         => "AXI4_Stream",
              C_APPLICATION_TYPE  => 0,
	      C_DOUT_WIDTH        => 69,
	      C_DIN_WIDTH         => 69,
	      C_WR_PNTR_WIDTH     => 10,
    	      C_RD_PNTR_WIDTH     => 10,
 	      C_CH_TYPE           => 0,
              FREEZEON_ERROR      => FREEZEON_ERROR,
	      TB_SEED             => TB_SEED, 
              TB_STOP_CNT         => TB_STOP_CNT
	     )
     PORT MAP(
              RESET_WR            => rst_int_wr,
              RESET_RD            => rst_int_rd,
	      RESET_EN            => reset_en,
              WR_CLK              => s_aclk_i,
              RD_CLK              => m_aclk_i,
              PRC_WR_EN           => prc_we_i,
              PRC_RD_EN           => prc_re_i,
	      FULL                => full_axis,
	      EMPTY               => empty_axis,
              ALMOST_FULL         => '0',
              ALMOST_EMPTY        => '0',
	      DATA_IN             => din_axis,
	      DATA_OUT            => dout_axis,
	      DOUT_CHK            => dout_chk_i,
	      SIM_DONE            => SIM_DONE,
	      STATUS              => STATUS
	    );
       s_axis_tdata    <= din_axis(69-1 DOWNTO TDATA_OFFSET);
       dout_axis(69-1 DOWNTO TDATA_OFFSET) <= m_axis_tdata;
       s_axis_tuser    <= DIN_AXIS(TDEST_OFFSET-1 DOWNTO TLAST_OFFSET);
       dout_axis(TDEST_OFFSET-1 DOWNTO TLAST_OFFSET) <= m_axis_tuser;
       s_axis_tlast    <= din_axis(0);
       dout_axis(0)    <= m_axis_tlast;

  axi64_8k_2clk_fifo_inst : axi64_8k_2clk_fifo_exdes 
    PORT MAP (
           S_ARESETN                 => s_aresetn,
           M_AXIS_TVALID             => m_axis_tvalid,
           M_AXIS_TREADY             => m_axis_tready,
           M_AXIS_TDATA              => m_axis_tdata,
           M_AXIS_TLAST              => m_axis_tlast,
           M_AXIS_TUSER              => m_axis_tuser,
           S_AXIS_TVALID             => s_axis_tvalid,
           S_AXIS_TREADY             => s_axis_tready,
           S_AXIS_TDATA              => s_axis_tdata,
           S_AXIS_TLAST              => s_axis_tlast,
           S_AXIS_TUSER              => s_axis_tuser,
           AXIS_WR_DATA_COUNT        => axis_wr_data_count,
           AXIS_RD_DATA_COUNT        => axis_rd_data_count,
           M_ACLK                    => m_aclk_i,
           S_ACLK                    => s_aclk_i);

END ARCHITECTURE;
