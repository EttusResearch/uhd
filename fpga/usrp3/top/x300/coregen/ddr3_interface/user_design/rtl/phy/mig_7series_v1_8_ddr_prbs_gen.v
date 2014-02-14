//*****************************************************************************
// (c) Copyright 2009 - 2012 Xilinx, Inc. All rights reserved.
//
// This file contains confidential and proprietary information
// of Xilinx, Inc. and is protected under U.S. and
// international copyright and other intellectual property
// laws.
//
// DISCLAIMER
// This disclaimer is not a license and does not grant any
// rights to the materials distributed herewith. Except as
// otherwise provided in a valid license issued to you by
// Xilinx, and to the maximum extent permitted by applicable
// law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
// WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
// AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
// BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
// (2) Xilinx shall not be liable (whether in contract or tort,
// including negligence, or under any other theory of
// liability) for any loss or damage of any kind or nature
// related to, arising under or in connection with these
// materials, including for any direct, or any indirect,
// special, incidental, or consequential loss or damage
// (including loss of data, profits, goodwill, or any type of
// loss or damage suffered as a result of any action brought
// by a third party) even if such damage or loss was
// reasonably foreseeable or Xilinx had been advised of the
// possibility of the same.
//
// CRITICAL APPLICATIONS
// Xilinx products are not designed or intended to be fail-
// safe, or for use in any application requiring fail-safe
// performance, such as life-support or safety devices or
// systems, Class III medical devices, nuclear facilities,
// applications related to the deployment of airbags, or any
// other applications that could lead to death, personal
// injury, or severe property or environmental damage
// (individually and collectively, "Critical
// Applications"). Customer assumes the sole risk and
// liability of any use of Xilinx products in Critical
// Applications, subject only to applicable laws and
// regulations governing limitations on product liability.
//
// THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
// PART OF THIS FILE AT ALL TIMES.
// 
//*****************************************************************************
//   ____  ____
//  /   /\/   /
// /___/  \  /    Vendor: Xilinx
// \   \   \/     Version: %version
//  \   \         Application: MIG
//  /   /         Filename: ddr_prbs_gen.v
// /___/   /\     Date Last Modified: $Date: 2011/06/02 08:35:10 $
// \   \  /  \    Date Created: 05/12/10
//  \___\/\___\
//
//Device: 7 Series
//Design Name: ddr_prbs_gen
// Overview:
//  Implements a "pseudo-PRBS" generator. Basically this is a standard
//  PRBS generator (using an linear feedback shift register) along with
//  logic to force the repetition of the sequence after 2^PRBS_WIDTH
//  samples (instead of 2^PRBS_WIDTH - 1). The LFSR is based on the design
//  from Table 1 of XAPP 210. Note that only 8- and 10-tap long LFSR chains
//  are supported in this code
// Parameter Requirements:
//  1. PRBS_WIDTH = 8 or 10
//  2. PRBS_WIDTH >= 2*nCK_PER_CLK
// Output notes:
//  The output of this module consists of 2*nCK_PER_CLK bits, these contain
//  the value of the LFSR output for the next 2*CK_PER_CLK bit times. Note
//  that prbs_o[0] contains the bit value for the "earliest" bit time. 
//
//Reference:
//Revision History:
// 
//*****************************************************************************

/******************************************************************************
**$Id: ddr_prbs_gen.v,v 1.1 2011/06/02 08:35:10 mishra Exp $
**$Date: 2011/06/02 08:35:10 $
**$Author: mishra $
**$Revision: 1.1 $
**$Source: /devl/xcs/repo/env/Databases/ip/src2/O/mig_7series_v1_3/data/dlib/7series/ddr3_sdram/verilog/rtl/phy/ddr_prbs_gen.v,v $
******************************************************************************/


`timescale 1ps/1ps

module mig_7series_v1_8_ddr_prbs_gen #
  (
   parameter TCQ         = 100,        // clk->out delay (sim only)
   parameter PRBS_WIDTH  = 64          // LFSR shift register length
   )
  (
   input                      clk_i,          // input clock
   input                      clk_en_i,       // clock enable 
   input                      rst_i,          // synchronous reset
   input [PRBS_WIDTH-1:0]     prbs_seed_i,    // initial LFSR seed
   input                      phy_if_empty,   // IN_FIFO empty flag
   input                      prbs_rdlvl_start, // PRBS read lveling start
   output [PRBS_WIDTH-1:0]    prbs_o // generated pseudo random data
  );

  //***************************************************************************

  function integer clogb2 (input integer size);
    begin
      size = size - 1;
      for (clogb2=1; size>1; clogb2=clogb2+1)
        size = size >> 1;
    end
  endfunction
  
  // Number of internal clock cycles before the PRBS sequence will repeat
  localparam PRBS_SEQ_LEN_CYCLES = 128;
  localparam PRBS_SEQ_LEN_CYCLES_BITS = clogb2(PRBS_SEQ_LEN_CYCLES);
  
  reg                                 phy_if_empty_r;
  reg                                 reseed_prbs_r;
  reg [PRBS_SEQ_LEN_CYCLES_BITS-1:0]  sample_cnt_r;
  reg [PRBS_WIDTH - 1 :0]             prbs;  
  reg [PRBS_WIDTH :1]                 lfsr_q;
  
  //***************************************************************************
  always @(posedge clk_i) begin
    phy_if_empty_r <= #TCQ phy_if_empty;
  end

  //***************************************************************************
  // Generate PRBS reset signal to ensure that PRBS sequence repeats after
  // every 2**PRBS_WIDTH samples. Basically what happens is that we let the
  // LFSR run for an extra cycle after "truly PRBS" 2**PRBS_WIDTH - 1
  // samples have past. Once that extra cycle is finished, we reseed the LFSR
  always @(posedge clk_i)
  begin
    if (rst_i || ~clk_en_i) begin
      sample_cnt_r    <= #TCQ 'b0;
      reseed_prbs_r   <= #TCQ 1'b0;
    end else if (clk_en_i && (~phy_if_empty_r || ~prbs_rdlvl_start)) begin
      // The rollver count should always be [(power of 2) - 1]
      sample_cnt_r    <= #TCQ sample_cnt_r + 1;
      // Assert PRBS reset signal so that it is simultaneously with the
      // last sample of the sequence
      if (sample_cnt_r == PRBS_SEQ_LEN_CYCLES - 2)
        reseed_prbs_r <= #TCQ 1'b1;
      else
        reseed_prbs_r <= #TCQ 1'b0;
    end
  end

  always @ (posedge clk_i)
  begin
//reset it to a known good state to prevent it locks up
    if ((reseed_prbs_r && clk_en_i) || rst_i || ~clk_en_i) begin
      lfsr_q[4:1]          <= #TCQ prbs_seed_i[3:0] | 4'h5;
      lfsr_q[PRBS_WIDTH:5] <= #TCQ prbs_seed_i[PRBS_WIDTH-1:4];
    end
    else if (clk_en_i && (~phy_if_empty_r || ~prbs_rdlvl_start)) begin
      lfsr_q[PRBS_WIDTH:31] <= #TCQ lfsr_q[PRBS_WIDTH-1:30];
      lfsr_q[30]            <= #TCQ lfsr_q[16] ^ lfsr_q[13] ^ lfsr_q[5]  ^ lfsr_q[1];
      lfsr_q[29:9]          <= #TCQ lfsr_q[28:8];
      lfsr_q[8]             <= #TCQ lfsr_q[32] ^ lfsr_q[7];
      lfsr_q[7]             <= #TCQ lfsr_q[32] ^ lfsr_q[6];
      lfsr_q[6:4]           <= #TCQ lfsr_q[5:3];
      lfsr_q[3]             <= #TCQ lfsr_q[32] ^ lfsr_q[2];
      lfsr_q[2]             <= #TCQ lfsr_q[1] ;
      lfsr_q[1]             <= #TCQ lfsr_q[32];
    end
  end
 
  always @ (lfsr_q[PRBS_WIDTH:1]) begin
    prbs = lfsr_q[PRBS_WIDTH:1];
  end

  assign prbs_o = prbs;

endmodule
   
         
