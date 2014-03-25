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
// \   \   \/     Version:
//  \   \         Application: MIG
//  /   /         Filename: ddr_phy_prbs_rdlvl.v
// /___/   /\     Date Last Modified: $Date: 2011/06/24 14:49:00 $
// \   \  /  \    Date Created:
//  \___\/\___\
//
//Device: 7 Series
//Design Name: DDR3 SDRAM
//Purpose:
//  PRBS Read leveling calibration logic
//  NOTES:
//    1. Window detection with PRBS pattern.
//Reference:
//Revision History:
//*****************************************************************************

/******************************************************************************
**$Id: ddr_phy_prbs_rdlvl.v,v 1.2 2011/06/24 14:49:00 mgeorge Exp $
**$Date: 2011/06/24 14:49:00 $
**$Author: mgeorge $
**$Revision: 1.2 $
**$Source: /devl/xcs/repo/env/Databases/ip/src2/O/mig_7series_v1_3/data/dlib/7series/ddr3_sdram/verilog/rtl/phy/ddr_phy_prbs_rdlvl.v,v $
******************************************************************************/

`timescale 1ps/1ps

module mig_7series_v1_8_ddr_phy_prbs_rdlvl #
  (
   parameter TCQ             = 100,    // clk->out delay (sim only)
   parameter nCK_PER_CLK     = 2,      // # of memory clocks per CLK
   parameter DQ_WIDTH        = 64,     // # of DQ (data)
   parameter DQS_CNT_WIDTH   = 3,      // = ceil(log2(DQS_WIDTH))
   parameter DQS_WIDTH       = 8,      // # of DQS (strobe)
   parameter DRAM_WIDTH      = 8,      // # of DQ per DQS
   parameter RANKS           = 1,      // # of DRAM ranks
   parameter SIM_CAL_OPTION  = "NONE", // Skip various calibration steps
   parameter PRBS_WIDTH      = 8       // PRBS generator output width
   )
  (
   input                        clk,
   input                        rst,
   // Calibration status, control signals
   input                        prbs_rdlvl_start,
   output reg                   prbs_rdlvl_done,
   output reg                   prbs_last_byte_done,
   output reg                   prbs_rdlvl_prech_req,
   input                        prech_done,
   input                        phy_if_empty,
   // Captured data in fabric clock domain
   input [2*nCK_PER_CLK*DQ_WIDTH-1:0] rd_data,
   //Expected data from PRBS generator
   input [2*nCK_PER_CLK*PRBS_WIDTH-1:0] compare_data,
   // Decrement initial Phaser_IN Fine tap delay
   input [5:0]                  pi_counter_read_val,
   // Stage 1 calibration outputs
   output reg                   pi_en_stg2_f,
   output reg                   pi_stg2_f_incdec,
   output [255:0]               dbg_prbs_rdlvl,
   output [DQS_CNT_WIDTH:0]     pi_stg2_prbs_rdlvl_cnt   
   );



  
  localparam [5:0] PRBS_IDLE                 = 6'h00;
  localparam [5:0] PRBS_NEW_DQS_WAIT         = 6'h01;
  localparam [5:0] PRBS_PAT_COMPARE          = 6'h02;
  localparam [5:0] PRBS_DEC_DQS              = 6'h03;
  localparam [5:0] PRBS_DEC_DQS_WAIT         = 6'h04;
  localparam [5:0] PRBS_INC_DQS              = 6'h05;
  localparam [5:0] PRBS_INC_DQS_WAIT         = 6'h06;
  localparam [5:0] PRBS_CALC_TAPS            = 6'h07;
  localparam [5:0] PRBS_NEXT_DQS             = 6'h08;
  localparam [5:0] PRBS_NEW_DQS_PREWAIT      = 6'h09;
  localparam [5:0] PRBS_DONE                 = 6'h0A;

  
  localparam [11:0] NUM_SAMPLES_CNT  = (SIM_CAL_OPTION == "NONE") ? 12'hFFF : 12'h001; 
  localparam [11:0] NUM_SAMPLES_CNT1 = (SIM_CAL_OPTION == "NONE") ? 12'hFFF : 12'h001;
  localparam [11:0] NUM_SAMPLES_CNT2 = (SIM_CAL_OPTION == "NONE") ? 12'hFFF : 12'h001;
  
  
  wire [DQS_CNT_WIDTH+2:0]prbs_dqs_cnt_timing;
  reg [DQS_CNT_WIDTH+2:0] prbs_dqs_cnt_timing_r;
  reg [DQS_CNT_WIDTH:0]   prbs_dqs_cnt_r;
  reg                     prbs_prech_req_r;
  reg [5:0]               prbs_state_r;
  reg [5:0]               prbs_state_r1;
  reg                     wait_state_cnt_en_r;
  reg [3:0]               wait_state_cnt_r;
  reg                     cnt_wait_state;
  reg                     found_edge_r;
  reg                     prbs_found_1st_edge_r;
  reg                     prbs_found_2nd_edge_r;
  reg [5:0]               prbs_1st_edge_taps_r;
  reg                     found_stable_eye_r;
  reg [5:0]               prbs_dqs_tap_cnt_r;
  reg                     prbs_dqs_tap_limit_r;
  reg [5:0]               prbs_inc_tap_cnt;
  reg [5:0]               prbs_dec_tap_cnt;
  reg [DRAM_WIDTH-1:0]    mux_rd_fall0_r1;
  reg [DRAM_WIDTH-1:0]    mux_rd_fall1_r1;
  reg [DRAM_WIDTH-1:0]    mux_rd_rise0_r1;
  reg [DRAM_WIDTH-1:0]    mux_rd_rise1_r1;
  reg [DRAM_WIDTH-1:0]    mux_rd_fall2_r1;
  reg [DRAM_WIDTH-1:0]    mux_rd_fall3_r1;
  reg [DRAM_WIDTH-1:0]    mux_rd_rise2_r1;
  reg [DRAM_WIDTH-1:0]    mux_rd_rise3_r1;
  reg [DRAM_WIDTH-1:0]    mux_rd_fall0_r2;
  reg [DRAM_WIDTH-1:0]    mux_rd_fall1_r2;
  reg [DRAM_WIDTH-1:0]    mux_rd_rise0_r2;
  reg [DRAM_WIDTH-1:0]    mux_rd_rise1_r2;
  reg [DRAM_WIDTH-1:0]    mux_rd_fall2_r2;
  reg [DRAM_WIDTH-1:0]    mux_rd_fall3_r2;
  reg [DRAM_WIDTH-1:0]    mux_rd_rise2_r2;
  reg [DRAM_WIDTH-1:0]    mux_rd_rise3_r2;
  reg                     mux_rd_valid_r;
  reg                     rd_valid_r1;
  reg                     rd_valid_r2;
  reg                     rd_valid_r3;
  reg                     new_cnt_dqs_r;
  reg                     prbs_tap_en_r;
  reg                     prbs_tap_inc_r;
  reg                             pi_en_stg2_f_timing;
  reg                             pi_stg2_f_incdec_timing;
  wire [DQ_WIDTH-1:0]     rd_data_rise0;
  wire [DQ_WIDTH-1:0]     rd_data_fall0;
  wire [DQ_WIDTH-1:0]     rd_data_rise1;
  wire [DQ_WIDTH-1:0]     rd_data_fall1;
  wire [DQ_WIDTH-1:0]     rd_data_rise2;
  wire [DQ_WIDTH-1:0]     rd_data_fall2;
  wire [DQ_WIDTH-1:0]     rd_data_rise3;
  wire [DQ_WIDTH-1:0]     rd_data_fall3;
  wire [PRBS_WIDTH-1:0]   compare_data_r0;
  wire [PRBS_WIDTH-1:0]   compare_data_f0;
  wire [PRBS_WIDTH-1:0]   compare_data_r1;
  wire [PRBS_WIDTH-1:0]   compare_data_f1;
  wire [PRBS_WIDTH-1:0]   compare_data_r2;
  wire [PRBS_WIDTH-1:0]   compare_data_f2;
  wire [PRBS_WIDTH-1:0]   compare_data_r3;
  wire [PRBS_WIDTH-1:0]   compare_data_f3;
  reg [DQS_CNT_WIDTH:0]   rd_mux_sel_r;
  reg [5:0]               prbs_2nd_edge_taps_r;
  
  reg [6*DQS_WIDTH*RANKS-1:0] prbs_final_dqs_tap_cnt_r;
  reg [1:0]               rnk_cnt_r;
  reg [5:0]               rdlvl_cpt_tap_cnt;
  
  reg                     prbs_rdlvl_start_r;
  
  reg                     compare_err;
  reg                     compare_err_r0;
  reg                     compare_err_f0;
  reg                     compare_err_r1;
  reg                     compare_err_f1;
  reg                     compare_err_r2;
  reg                     compare_err_f2;
  reg                     compare_err_r3;
  reg                     compare_err_f3;
  
  reg                     samples_cnt_en_r;
  reg                     samples_cnt1_en_r;
  reg                     samples_cnt2_en_r;
  reg [11:0]              samples_cnt_r;
  reg [11:0]              samples_cnt1_r;
  reg [11:0]              samples_cnt2_r;
  reg                     num_samples_done_r;

  

  
   //**************************************************************************
   // DQS count to hard PHY during write calibration using Phaser_OUT Stage2
   // coarse delay 
   //**************************************************************************
   assign pi_stg2_prbs_rdlvl_cnt = prbs_dqs_cnt_r; 
   assign dbg_prbs_rdlvl = {prbs_2nd_edge_taps_r, prbs_1st_edge_taps_r, rdlvl_cpt_tap_cnt, prbs_dqs_cnt_r,
                            prbs_rdlvl_done, prbs_rdlvl_start, phy_if_empty, compare_err, prbs_found_2nd_edge_r, prbs_found_1st_edge_r, prbs_dqs_tap_cnt_r, pi_counter_read_val,
                            mux_rd_fall3_r2, mux_rd_rise3_r2, mux_rd_fall2_r2, mux_rd_rise2_r2, mux_rd_fall1_r2, mux_rd_rise1_r2, mux_rd_fall0_r2, mux_rd_rise0_r2,
                            compare_data_f3, compare_data_r3, compare_data_f2, compare_data_r2, compare_data_f1, compare_data_r1, compare_data_f0, compare_data_r0};


  
  //***************************************************************************
  // Data mux to route appropriate bit to calibration logic - i.e. calibration
  // is done sequentially, one bit (or DQS group) at a time
  //***************************************************************************

  generate
    if (nCK_PER_CLK == 4) begin: rd_data_div4_logic_clk
      assign rd_data_rise0 = rd_data[DQ_WIDTH-1:0];
      assign rd_data_fall0 = rd_data[2*DQ_WIDTH-1:DQ_WIDTH];
      assign rd_data_rise1 = rd_data[3*DQ_WIDTH-1:2*DQ_WIDTH];
      assign rd_data_fall1 = rd_data[4*DQ_WIDTH-1:3*DQ_WIDTH];
      assign rd_data_rise2 = rd_data[5*DQ_WIDTH-1:4*DQ_WIDTH];
      assign rd_data_fall2 = rd_data[6*DQ_WIDTH-1:5*DQ_WIDTH];
      assign rd_data_rise3 = rd_data[7*DQ_WIDTH-1:6*DQ_WIDTH];
      assign rd_data_fall3 = rd_data[8*DQ_WIDTH-1:7*DQ_WIDTH];
      assign compare_data_r0 = compare_data[PRBS_WIDTH-1:0];
      assign compare_data_f0 = compare_data[2*PRBS_WIDTH-1:PRBS_WIDTH];
      assign compare_data_r1 = compare_data[3*PRBS_WIDTH-1:2*PRBS_WIDTH];
      assign compare_data_f1 = compare_data[4*PRBS_WIDTH-1:3*PRBS_WIDTH];
      assign compare_data_r2 = compare_data[5*PRBS_WIDTH-1:4*PRBS_WIDTH];
      assign compare_data_f2 = compare_data[6*PRBS_WIDTH-1:5*PRBS_WIDTH];
      assign compare_data_r3 = compare_data[7*PRBS_WIDTH-1:6*PRBS_WIDTH];
      assign compare_data_f3 = compare_data[8*PRBS_WIDTH-1:7*PRBS_WIDTH];
    end else begin: rd_data_div2_logic_clk
      assign rd_data_rise0 = rd_data[DQ_WIDTH-1:0];
      assign rd_data_fall0 = rd_data[2*DQ_WIDTH-1:DQ_WIDTH];
      assign rd_data_rise1 = rd_data[3*DQ_WIDTH-1:2*DQ_WIDTH];
      assign rd_data_fall1 = rd_data[4*DQ_WIDTH-1:3*DQ_WIDTH];
      assign compare_data_r0 = compare_data[PRBS_WIDTH-1:0];
      assign compare_data_f0 = compare_data[2*PRBS_WIDTH-1:PRBS_WIDTH];
      assign compare_data_r1 = compare_data[3*PRBS_WIDTH-1:2*PRBS_WIDTH];
      assign compare_data_f1 = compare_data[4*PRBS_WIDTH-1:3*PRBS_WIDTH];
    end
  endgenerate

  always @(posedge clk) begin
    rd_mux_sel_r <= #TCQ prbs_dqs_cnt_r;
  end

  // Register outputs for improved timing.
  // NOTE: Will need to change when per-bit DQ deskew is supported.
  //       Currenly all bits in DQS group are checked in aggregate
  generate
    genvar mux_i;
    for (mux_i = 0; mux_i < DRAM_WIDTH; mux_i = mux_i + 1) begin: gen_mux_rd
      always @(posedge clk) begin
        mux_rd_rise0_r1[mux_i] <= #TCQ rd_data_rise0[DRAM_WIDTH*rd_mux_sel_r + 
                                                    mux_i];
        mux_rd_fall0_r1[mux_i] <= #TCQ rd_data_fall0[DRAM_WIDTH*rd_mux_sel_r + 
                                                    mux_i];
        mux_rd_rise1_r1[mux_i] <= #TCQ rd_data_rise1[DRAM_WIDTH*rd_mux_sel_r + 
                                                    mux_i];
        mux_rd_fall1_r1[mux_i] <= #TCQ rd_data_fall1[DRAM_WIDTH*rd_mux_sel_r + 
                                                    mux_i];
        mux_rd_rise2_r1[mux_i] <= #TCQ rd_data_rise2[DRAM_WIDTH*rd_mux_sel_r + 
                                                    mux_i];
        mux_rd_fall2_r1[mux_i] <= #TCQ rd_data_fall2[DRAM_WIDTH*rd_mux_sel_r + 
                                                    mux_i];
        mux_rd_rise3_r1[mux_i] <= #TCQ rd_data_rise3[DRAM_WIDTH*rd_mux_sel_r + 
                                                    mux_i];
        mux_rd_fall3_r1[mux_i] <= #TCQ rd_data_fall3[DRAM_WIDTH*rd_mux_sel_r + 
                                                    mux_i];     
      end
    end
  endgenerate
  
  generate
    genvar muxr2_i;
    if (nCK_PER_CLK == 4) begin: gen_mux_div4
        for (muxr2_i = 0; muxr2_i < DRAM_WIDTH; muxr2_i = muxr2_i + 1) begin: gen_rd_4
          always @(posedge clk) begin
            if (mux_rd_valid_r) begin
              mux_rd_rise0_r2[muxr2_i] <= #TCQ mux_rd_rise0_r1[muxr2_i];
              mux_rd_fall0_r2[muxr2_i] <= #TCQ mux_rd_fall0_r1[muxr2_i];
              mux_rd_rise1_r2[muxr2_i] <= #TCQ mux_rd_rise1_r1[muxr2_i];
              mux_rd_fall1_r2[muxr2_i] <= #TCQ mux_rd_fall1_r1[muxr2_i];
              mux_rd_rise2_r2[muxr2_i] <= #TCQ mux_rd_rise2_r1[muxr2_i];
              mux_rd_fall2_r2[muxr2_i] <= #TCQ mux_rd_fall2_r1[muxr2_i];
              mux_rd_rise3_r2[muxr2_i] <= #TCQ mux_rd_rise3_r1[muxr2_i];
              mux_rd_fall3_r2[muxr2_i] <= #TCQ mux_rd_fall3_r1[muxr2_i];
            end
          end
                end
    end else if (nCK_PER_CLK == 2) begin: gen_mux_div2
        for (muxr2_i = 0; muxr2_i < DRAM_WIDTH; muxr2_i = muxr2_i + 1) begin: gen_rd_2
          always @(posedge clk) begin
            if (mux_rd_valid_r) begin
              mux_rd_rise0_r2[muxr2_i] <= #TCQ mux_rd_rise0_r1[muxr2_i];
              mux_rd_fall0_r2[muxr2_i] <= #TCQ mux_rd_fall0_r1[muxr2_i];
              mux_rd_rise1_r2[muxr2_i] <= #TCQ mux_rd_rise1_r1[muxr2_i];
              mux_rd_fall1_r2[muxr2_i] <= #TCQ mux_rd_fall1_r1[muxr2_i];      
            end
                  end
        end
    end
  endgenerate

  
  // Registered signal indicates when mux_rd_rise/fall_r is valid
  always @(posedge clk) begin
    mux_rd_valid_r <= #TCQ ~phy_if_empty && prbs_rdlvl_start;
    rd_valid_r1    <= #TCQ mux_rd_valid_r;
    rd_valid_r2    <= #TCQ rd_valid_r1;
    rd_valid_r3    <= #TCQ rd_valid_r2;
  end
  
  // Signal to enable counter to count number of samples compared
  always @(posedge clk) begin
    samples_cnt_en_r <= #TCQ mux_rd_valid_r;
  end
  
  // Counter counts # of samples compared
  always @(posedge clk)
    if (rst || !samples_cnt_en_r)
      samples_cnt_r <= #TCQ 'b0;
    else begin
      if ((prbs_state_r == PRBS_DEC_DQS_WAIT) ||
              (prbs_state_r == PRBS_INC_DQS_WAIT) ||
              (prbs_state_r == PRBS_DEC_DQS) ||
                  (prbs_state_r == PRBS_INC_DQS))
        // Reset sample counter when not "sampling"
        samples_cnt_r <= #TCQ 'b0;
      else if (rd_valid_r1 && (samples_cnt_r < NUM_SAMPLES_CNT))
        // Otherwise, count # of samples compared
        samples_cnt_r <= #TCQ samples_cnt_r + 1;
    end

  // Counter #2 enable generation
  always @(posedge clk)
    if (rst)
      samples_cnt1_en_r <= #TCQ 1'b0;
    else begin 
      // Assert pulse when correct number of samples compared
      if ((samples_cnt_r == NUM_SAMPLES_CNT) && rd_valid_r1)
        samples_cnt1_en_r <= #TCQ 1'b1;
      else
        samples_cnt1_en_r <= #TCQ 1'b0;
    end

  // Counter #2
  always @(posedge clk)
    if (rst || !samples_cnt1_en_r)
      samples_cnt1_r <= #TCQ 'b0;
    else if (samples_cnt1_en_r && (samples_cnt1_r < NUM_SAMPLES_CNT1))
      samples_cnt1_r <= #TCQ samples_cnt1_r + 1;

  // Counter #3 enable generation
  always @(posedge clk)
    if (rst)
      samples_cnt2_en_r <= #TCQ 1'b0;
    else begin 
      // Assert pulse when correct number of samples compared
      if ((samples_cnt1_r == NUM_SAMPLES_CNT1) && rd_valid_r1)
        samples_cnt2_en_r <= #TCQ 1'b1;
      else
        samples_cnt2_en_r <= #TCQ 1'b0;
    end

  // Counter #3
  always @(posedge clk)
    if (rst || !samples_cnt2_en_r)
      samples_cnt2_r <= #TCQ 'b0;
    else if (samples_cnt2_en_r)
      samples_cnt2_r <= #TCQ samples_cnt2_r + 1;          

  always @(posedge clk)
    if (rst)
      num_samples_done_r <= #TCQ 1'b0;
    else begin 
      if (!samples_cnt_en_r ||
              (prbs_state_r == PRBS_DEC_DQS) ||
                  (prbs_state_r == PRBS_INC_DQS))
        num_samples_done_r <= #TCQ 'b0;
      else begin
        if (samples_cnt2_r == NUM_SAMPLES_CNT2-1)
          num_samples_done_r <= #TCQ 1'b1;
      end
    end
  
  
  //***************************************************************************
  // Compare Read Data for the byte being Leveled with Expected data from PRBS
  // generator. Resulting compare_err signal used to determine read data valid
  // edge.
  //***************************************************************************
  generate
    if (nCK_PER_CLK == 4) begin: cmp_err_4to1
      always @ (posedge clk) begin
        if (rst || new_cnt_dqs_r) begin
              compare_err    <= #TCQ 1'b0;
              compare_err_r0 <= #TCQ 1'b0;
              compare_err_f0 <= #TCQ 1'b0;
              compare_err_r1 <= #TCQ 1'b0;
              compare_err_f1 <= #TCQ 1'b0;
              compare_err_r2 <= #TCQ 1'b0;
              compare_err_f2 <= #TCQ 1'b0;
              compare_err_r3 <= #TCQ 1'b0;
              compare_err_f3 <= #TCQ 1'b0;
            end else if (rd_valid_r1) begin
              compare_err_r0  <= #TCQ (mux_rd_rise0_r2 != compare_data_r0);
              compare_err_f0  <= #TCQ (mux_rd_fall0_r2 != compare_data_f0);
              compare_err_r1  <= #TCQ (mux_rd_rise1_r2 != compare_data_r1);
              compare_err_f1  <= #TCQ (mux_rd_fall1_r2 != compare_data_f1);
              compare_err_r2  <= #TCQ (mux_rd_rise2_r2 != compare_data_r2);
              compare_err_f2  <= #TCQ (mux_rd_fall2_r2 != compare_data_f2);
              compare_err_r3  <= #TCQ (mux_rd_rise3_r2 != compare_data_r3);
              compare_err_f3  <= #TCQ (mux_rd_fall3_r2 != compare_data_f3);
              compare_err     <= #TCQ (compare_err_r0 | compare_err_f0 |
                                       compare_err_r1 | compare_err_f1 |
                                                           compare_err_r2 | compare_err_f2 |
                                                           compare_err_r3 | compare_err_f3);
            end
      end
        end else begin: cmp_err_2to1
          always @ (posedge clk) begin
        if (rst || new_cnt_dqs_r) begin
              compare_err    <= #TCQ 1'b0;
              compare_err_r0 <= #TCQ 1'b0;
              compare_err_f0 <= #TCQ 1'b0;
              compare_err_r1 <= #TCQ 1'b0;
              compare_err_f1 <= #TCQ 1'b0;
            end else if (rd_valid_r1) begin
              compare_err_r0  <= #TCQ (mux_rd_rise0_r2 != compare_data_r0);
              compare_err_f0  <= #TCQ (mux_rd_fall0_r2 != compare_data_f0);
              compare_err_r1  <= #TCQ (mux_rd_rise1_r2 != compare_data_r1);
              compare_err_f1  <= #TCQ (mux_rd_fall1_r2 != compare_data_f1);
              compare_err     <= #TCQ (compare_err_r0 | compare_err_f0 |
                                       compare_err_r1 | compare_err_f1);
            end
      end
        end
  endgenerate
          
  
  
  //***************************************************************************
  // Decrement initial Phaser_IN fine delay value before proceeding with
  // read calibration
  //***************************************************************************
  
  
  //***************************************************************************
  // Demultiplexor to control Phaser_IN delay values
  //***************************************************************************

  // Read DQS
  always @(posedge clk) begin
    if (rst) begin
      pi_en_stg2_f_timing     <= #TCQ 'b0;
      pi_stg2_f_incdec_timing <= #TCQ 'b0;
    end else if (prbs_tap_en_r) begin
      // Change only specified DQS
      pi_en_stg2_f_timing     <= #TCQ 1'b1;  
      pi_stg2_f_incdec_timing <= #TCQ prbs_tap_inc_r;
    end else begin
      pi_en_stg2_f_timing     <= #TCQ 'b0;
      pi_stg2_f_incdec_timing <= #TCQ 'b0;
    end
  end

  // registered for timing 
  always @(posedge clk) begin
    pi_en_stg2_f     <= #TCQ pi_en_stg2_f_timing;
    pi_stg2_f_incdec <= #TCQ pi_stg2_f_incdec_timing;
  end
  
  
  //***************************************************************************
  // generate request to PHY_INIT logic to issue precharged. Required when
  // calibration can take a long time (during which there are only constant
  // reads present on this bus). In this case need to issue perioidic
  // precharges to avoid tRAS violation. This signal must meet the following
  // requirements: (1) only transition from 0->1 when prech is first needed,
  // (2) stay at 1 and only transition 1->0 when RDLVL_PRECH_DONE asserted
  //***************************************************************************

  always @(posedge clk)
    if (rst)
      prbs_rdlvl_prech_req <= #TCQ 1'b0;
    else
      prbs_rdlvl_prech_req <= #TCQ prbs_prech_req_r;

 
  
  //*****************************************************************
  // keep track of edge tap counts found, and current capture clock
  // tap count
  //*****************************************************************

  always @(posedge clk)
    if (rst) begin
      prbs_dqs_tap_cnt_r   <= #TCQ 'b0;
      rdlvl_cpt_tap_cnt    <= #TCQ 'b0;
    end else if (new_cnt_dqs_r) begin
      prbs_dqs_tap_cnt_r   <= #TCQ pi_counter_read_val;
      rdlvl_cpt_tap_cnt    <= #TCQ pi_counter_read_val;
    end else if (prbs_tap_en_r) begin
      if (prbs_tap_inc_r)
        prbs_dqs_tap_cnt_r <= #TCQ prbs_dqs_tap_cnt_r + 1;
      else if (prbs_dqs_tap_cnt_r != 'd0)
        prbs_dqs_tap_cnt_r <= #TCQ prbs_dqs_tap_cnt_r - 1;
    end
    
  always @(posedge clk)
    if (rst || new_cnt_dqs_r)
      prbs_dqs_tap_limit_r <= #TCQ 1'b0;
    else if (prbs_dqs_tap_cnt_r == 6'd63)
      prbs_dqs_tap_limit_r <= #TCQ 1'b1;

  // Temp wire for timing.
   // The following in the always block below causes timing issues
   // due to DSP block inference
   // 6*prbs_dqs_cnt_r.
   // replacing this with two left shifts + one left shift  to avoid
   // DSP multiplier.

  assign prbs_dqs_cnt_timing = {2'd0, prbs_dqs_cnt_r};


  always @(posedge clk)
    prbs_dqs_cnt_timing_r <= #TCQ prbs_dqs_cnt_timing;
    

   // Storing DQS tap values at the end of each DQS read leveling
   always @(posedge clk) begin
     if (rst) begin
       prbs_final_dqs_tap_cnt_r <= #TCQ 'b0;
     end else if (prbs_state_r1 == PRBS_NEXT_DQS) begin
        prbs_final_dqs_tap_cnt_r[(((prbs_dqs_cnt_timing_r <<2) + (prbs_dqs_cnt_timing_r <<1))
         +(rnk_cnt_r*DQS_WIDTH*6))+:6]
           <= #TCQ prbs_dqs_tap_cnt_r;
     end
   end




  //*****************************************************************
  
  always @(posedge clk) begin
    prbs_state_r1      <= #TCQ prbs_state_r;
    prbs_rdlvl_start_r <= #TCQ prbs_rdlvl_start;
  end
    
    
  // Wait counter for wait states
   always @(posedge clk)
    if ((prbs_state_r == PRBS_NEW_DQS_WAIT) ||
        (prbs_state_r == PRBS_INC_DQS_WAIT) ||
        (prbs_state_r == PRBS_DEC_DQS_WAIT) ||
        (prbs_state_r == PRBS_NEW_DQS_PREWAIT))
      wait_state_cnt_en_r <= #TCQ 1'b1;
    else
      wait_state_cnt_en_r <= #TCQ 1'b0;
   
   always @(posedge clk)
    if (!wait_state_cnt_en_r) begin
      wait_state_cnt_r <= #TCQ 'b0;
      cnt_wait_state   <= #TCQ 1'b0;
    end else begin
      if (wait_state_cnt_r < 'd15) begin
        wait_state_cnt_r <= #TCQ wait_state_cnt_r + 1;
        cnt_wait_state   <= #TCQ 1'b0;
      end else begin
        // Need to reset to 0 to handle the case when there are two
        // different WAIT states back-to-back
        wait_state_cnt_r <= #TCQ 'b0;        
        cnt_wait_state   <= #TCQ 1'b1;
      end
    end
    
  // PRBS Read Level State Machine
  
  always @(posedge clk)
    if (rst) begin
      prbs_dqs_cnt_r        <= #TCQ 'b0;
      prbs_tap_en_r         <= #TCQ 1'b0;
      prbs_tap_inc_r        <= #TCQ 1'b0;
      prbs_prech_req_r      <= #TCQ 1'b0;
      prbs_state_r          <= #TCQ PRBS_IDLE;
      prbs_found_1st_edge_r <= #TCQ 1'b0;
      prbs_found_2nd_edge_r <= #TCQ 1'b0;
      prbs_1st_edge_taps_r  <= #TCQ 6'bxxxxxx;
      prbs_inc_tap_cnt      <= #TCQ 'b0;
      prbs_dec_tap_cnt      <= #TCQ 'b0;
      new_cnt_dqs_r         <= #TCQ 1'b0;
      if (SIM_CAL_OPTION == "FAST_CAL")
        prbs_rdlvl_done       <= #TCQ 1'b1;
      else
        prbs_rdlvl_done       <= #TCQ 1'b0;
      prbs_2nd_edge_taps_r  <= #TCQ 6'bxxxxxx;
      prbs_last_byte_done   <= #TCQ 1'b0;
      rnk_cnt_r             <= #TCQ 2'b00; 
    end else begin
      
      case (prbs_state_r)
        
        PRBS_IDLE: begin
          prbs_last_byte_done  <= #TCQ 1'b0;
          prbs_prech_req_r     <= #TCQ 1'b0;
          if (prbs_rdlvl_start && ~prbs_rdlvl_start_r) begin
            if (SIM_CAL_OPTION == "SKIP_CAL")
              prbs_state_r  <= #TCQ PRBS_DONE;
            else begin
              new_cnt_dqs_r <= #TCQ 1'b1;             
              prbs_state_r  <= #TCQ PRBS_NEW_DQS_WAIT;
            end
          end
        end
        
        // Wait for the new DQS group to change
        // also gives time for the read data IN_FIFO to
        // output the updated data for the new DQS group
        PRBS_NEW_DQS_WAIT: begin
          prbs_last_byte_done <= #TCQ 1'b0;
          prbs_prech_req_r    <= #TCQ 1'b0;
          if (cnt_wait_state) begin
            new_cnt_dqs_r <= #TCQ 1'b0;
            prbs_state_r  <= #TCQ PRBS_PAT_COMPARE;
          end
        end

        

        // Check for presence of data eye edge. During this state, we
        // sample the read data multiple times, and look for changes
        // in the read data, specifically:
        //   1. A change in the read data compared with the value of
        //      read data from the previous delay tap. This indicates 
        //      that the most recent tap delay increment has moved us
        //      into either a new window, or moved/kept us in the
        //      transition/jitter region between windows. Note that this
        //      condition only needs to be checked for once, and for
        //      logistical purposes, we check this soon after entering
        //      this state (see comment in PRBS_PAT_COMPARE below for 
        //      why this is done)
        //   2. A change in the read data while we are in this state
        //      (i.e. in the absence of a tap delay increment). This
        //      indicates that we're close enough to a window edge that
        //      jitter will cause the read data to change even in the
        //      absence of a tap delay change 
        PRBS_PAT_COMPARE: begin

          // Continue to sample read data and look for edges until the
          // appropriate time interval (shorter for simulation-only, 
          // much, much longer for actual h/w) has elapsed
          if (num_samples_done_r || compare_err) begin
            if (prbs_dqs_tap_limit_r)
              // Only one edge detected and ran out of taps since only one
              // bit time worth of taps available for window detection. This
              // can happen if at tap 0 DQS is in previous window which results
              // in only left edge being detected. Or at tap 0 DQS is in the
              // current window resulting in only right edge being detected.
              // Depending on the frequency this case can also happen if at
              // tap 0 DQS is in the left noise region resulting in only left
              // edge being detected.
              prbs_state_r <= #TCQ PRBS_CALC_TAPS;
            else if (compare_err || (prbs_dqs_tap_cnt_r == 'd0)) begin 
              // Sticky bit - asserted after we encounter an edge, although
              // the current edge may not be considered the "first edge" this
              // just means we found at least one edge
              prbs_found_1st_edge_r <= #TCQ 1'b1;
              
              // Both edges of data valid window found:
              // If we've found a second edge after a region of stability
              // then we must have just passed the second ("right" edge of
              // the window. Record this second_edge_taps = current tap-1, 
              // because we're one past the actual second edge tap, where 
              // the edge taps represent the extremes of the data valid 
              // window (i.e. smallest & largest taps where data still valid
              if (prbs_found_1st_edge_r) begin
                prbs_found_2nd_edge_r <= #TCQ 1'b1;
                prbs_2nd_edge_taps_r  <= #TCQ prbs_dqs_tap_cnt_r - 1;
                prbs_state_r          <= #TCQ PRBS_CALC_TAPS;          
              end else begin
                // Otherwise, an edge was found (just not the "second" edge)
                // Assuming DQS is in the correct window at tap 0 of Phaser IN
                // fine tap. The first edge found is the right edge of the valid
                // window and is the beginning of the jitter region hence done!
                if (compare_err)
                  prbs_1st_edge_taps_r <= #TCQ prbs_dqs_tap_cnt_r + 1;
                else
                  prbs_1st_edge_taps_r <= #TCQ 'd0;
                
                prbs_inc_tap_cnt     <= #TCQ rdlvl_cpt_tap_cnt - prbs_dqs_tap_cnt_r;           
                prbs_state_r         <= #TCQ PRBS_INC_DQS;
              end
            end else begin
              // Otherwise, if we haven't found an edge.... 
              // If we still have taps left to use, then keep incrementing
              if (prbs_found_1st_edge_r)
                prbs_state_r  <= #TCQ PRBS_INC_DQS;
              else
                prbs_state_r  <= #TCQ PRBS_DEC_DQS;
            end
          end
        end
        
        // Increment Phaser_IN delay for DQS
        PRBS_INC_DQS: begin
          prbs_state_r        <= #TCQ PRBS_INC_DQS_WAIT;
          if (prbs_inc_tap_cnt > 'd0)
            prbs_inc_tap_cnt <= #TCQ prbs_inc_tap_cnt - 1;
          if (~prbs_dqs_tap_limit_r) begin
            prbs_tap_en_r    <= #TCQ 1'b1;
            prbs_tap_inc_r   <= #TCQ 1'b1;
          end else begin
            prbs_tap_en_r    <= #TCQ 1'b0;
            prbs_tap_inc_r   <= #TCQ 1'b0;
          end
        end

        // Wait for Phaser_In to settle, before checking again for an edge 
        PRBS_INC_DQS_WAIT: begin
          prbs_tap_en_r    <= #TCQ 1'b0;
          prbs_tap_inc_r   <= #TCQ 1'b0; 
          if (cnt_wait_state) begin
            if (prbs_inc_tap_cnt > 'd0)
              prbs_state_r <= #TCQ PRBS_INC_DQS;
            else
              prbs_state_r <= #TCQ PRBS_PAT_COMPARE;
          end
        end
            
        // Calculate final value of Phaser_IN taps. At this point, one or both
        // edges of data eye have been found, and/or all taps have been
        // exhausted looking for the edges
        // NOTE: The amount to be decrement by is calculated, not the
        //  absolute setting for DQS.
        PRBS_CALC_TAPS: begin
          if (prbs_found_2nd_edge_r && prbs_found_1st_edge_r)
          // Both edges detected
            prbs_dec_tap_cnt 
              <=  #TCQ ((prbs_2nd_edge_taps_r -
                         prbs_1st_edge_taps_r)>>1) + 1;
          else if (~prbs_found_2nd_edge_r && prbs_found_1st_edge_r)
          // Only left edge detected 
            prbs_dec_tap_cnt 
              <=  #TCQ ((prbs_dqs_tap_cnt_r - prbs_1st_edge_taps_r)>>1);
          else
          // No edges detected
            prbs_dec_tap_cnt 
              <=  #TCQ (prbs_dqs_tap_cnt_r>>1);
          // Now use the value we just calculated to decrement CPT taps
          // to the desired calibration point
          prbs_state_r <= #TCQ PRBS_DEC_DQS;  
        end

        // decrement capture clock for final adjustment - center
        // capture clock in middle of data eye. This adjustment will occur
        // only when both the edges are found usign CPT taps. Must do this
        // incrementally to avoid clock glitching (since CPT drives clock
        // divider within each ISERDES)
        PRBS_DEC_DQS: begin
          prbs_tap_en_r  <= #TCQ 1'b1;
          prbs_tap_inc_r <= #TCQ 1'b0;
          // once adjustment is complete, we're done with calibration for
          // this DQS, repeat for next DQS
          if (prbs_dec_tap_cnt > 'd0)
                    prbs_dec_tap_cnt <= #TCQ prbs_dec_tap_cnt - 1;
          if (prbs_dec_tap_cnt == 6'b000001)
            prbs_state_r <= #TCQ PRBS_NEXT_DQS;
          else
            prbs_state_r <= #TCQ PRBS_DEC_DQS_WAIT;
        end

        PRBS_DEC_DQS_WAIT: begin
          prbs_tap_en_r  <= #TCQ 1'b0;
          prbs_tap_inc_r <= #TCQ 1'b0;
          if (cnt_wait_state) begin
            if (prbs_dec_tap_cnt > 'd0)
              prbs_state_r <= #TCQ PRBS_DEC_DQS;
            else 
              prbs_state_r <= #TCQ PRBS_PAT_COMPARE;
          end
        end

        // Determine whether we're done, or have more DQS's to calibrate
        // Also request precharge after every byte, as appropriate
        PRBS_NEXT_DQS: begin
          prbs_prech_req_r  <= #TCQ 1'b1;
          prbs_tap_en_r  <= #TCQ 1'b0;
          prbs_tap_inc_r <= #TCQ 1'b0;
          // Prepare for another iteration with next DQS group
          prbs_found_1st_edge_r <= #TCQ 1'b0;
          prbs_found_2nd_edge_r <= #TCQ 1'b0;
          prbs_1st_edge_taps_r  <= #TCQ 'd0;
          prbs_2nd_edge_taps_r  <= #TCQ 'd0;
          if (prbs_dqs_cnt_r >= DQS_WIDTH-1) begin
            prbs_last_byte_done <= #TCQ 1'b1;
          end
           
          // Wait until precharge that occurs in between calibration of
          // DQS groups is finished
          if (prech_done) begin
                    prbs_prech_req_r <= #TCQ 1'b0;
                        if (prbs_dqs_cnt_r >= DQS_WIDTH-1) begin
              if (rnk_cnt_r == RANKS-1) begin
                // All DQS groups in all ranks done
                prbs_state_r <= #TCQ PRBS_DONE;
              end else begin
                // Process DQS groups in next rank
                rnk_cnt_r      <= #TCQ rnk_cnt_r + 1;
                new_cnt_dqs_r  <= #TCQ 1'b1;
                prbs_dqs_cnt_r <= #TCQ 'b0;
                prbs_state_r   <= #TCQ PRBS_IDLE;
              end
            end else begin
              // Process next DQS group
              new_cnt_dqs_r  <= #TCQ 1'b1;
              prbs_dqs_cnt_r <= #TCQ prbs_dqs_cnt_r + 1;
              prbs_state_r   <= #TCQ PRBS_NEW_DQS_PREWAIT;
            end
          end
        end
        
        PRBS_NEW_DQS_PREWAIT: begin
          if (cnt_wait_state) begin
            prbs_state_r <= #TCQ PRBS_NEW_DQS_WAIT;
          end
        end
        
        
        // Done with this stage of calibration
        PRBS_DONE: begin
          prbs_prech_req_r    <= #TCQ 1'b0;
                  prbs_last_byte_done <= #TCQ 1'b0;
          prbs_rdlvl_done     <= #TCQ 1'b1;
        end

      endcase
    end

 
 


endmodule
