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
//  /   /         Filename: ddr_phy_oclkdelay_cal.v
// /___/   /\     Date Last Modified: $Date: 2011/02/25 02:07:40 $
// \   \  /  \    Date Created: Aug 03 2009 
//  \___\/\___\
//
//Device: 7 Series
//Design Name: DDR3 SDRAM
//Purpose: Center write DQS in write DQ valid window using Phaser_Out Stage3
//         delay
//Reference:
//Revision History:
//*****************************************************************************

`timescale 1ps/1ps

module mig_7series_v1_8_ddr_phy_oclkdelay_cal #
  (
   parameter TCQ             = 100,
   parameter tCK             = 2500,
   parameter nCK_PER_CLK     = 4,
   parameter DRAM_TYPE       = "DDR3",
   parameter DRAM_WIDTH      = 8,
   parameter DQS_CNT_WIDTH   = 3,
   parameter DQS_WIDTH       = 8,
   parameter DQ_WIDTH        = 64,
   parameter SIM_CAL_OPTION  = "NONE",
   parameter OCAL_EN         = "ON"
   )
  (
   input                              clk,
   input                              rst,
   // Start only after PO and PI FINE delay decremented
   input                              oclk_init_delay_start,
   input                              oclkdelay_calib_start,
   input [5:0]                        oclkdelay_init_val,
   // Detect write valid data edge during OCLKDELAY calib
   input                              phy_rddata_en,
   input [2*nCK_PER_CLK*DQ_WIDTH-1:0] rd_data,
   // Precharge done status from ddr_phy_init
   input                              prech_done,
   // Write Level signals during OCLKDELAY calibration
   input [6*DQS_WIDTH-1:0]            wl_po_fine_cnt,
   output reg                         wrlvl_final,
   // Inc/dec Phaser_Out fine delay line
   output reg                         po_stg3_incdec,
   output reg                         po_en_stg3,
   output reg                         po_stg23_sel,
   output reg                         po_stg23_incdec,
   output reg                         po_en_stg23,
   // Completed initial delay increment
   output                             oclk_init_delay_done,
   output [DQS_CNT_WIDTH:0]           oclkdelay_calib_cnt,
   output reg                         oclk_prech_req,
   output reg                         oclk_calib_resume,
   output reg                         ocal_if_rst,
   output                             oclkdelay_calib_done,
   output [255:0]                     dbg_phy_oclkdelay_cal,
   output [16*DRAM_WIDTH-1:0]         dbg_oclkdelay_rd_data
   );
   
   
   // Start with an initial delay of 0 on OCLKDELAY. This is required to
   // detect two valid data edges when possible. Two edges cannot be 
   // detected if write DQ and DQS are exactly edge aligned at stage3 tap0.
   localparam TAP_CNT  = 0;
   //(tCK <= 938)  ? 13 :
   //(tCK <= 1072) ? 14 :
   //(tCK <= 1250) ? 15 :
   //(tCK <= 1500) ? 16 : 17;
   
   localparam WAIT_CNT = 15;
   
   localparam [4:0] OCAL_IDLE          = 5'h00;
   localparam [4:0] OCAL_NEW_DQS_WAIT  = 5'h01;
   localparam [4:0] OCAL_STG3_SEL      = 5'h02;
   localparam [4:0] OCAL_STG3_SEL_WAIT = 5'h03;
   localparam [4:0] OCAL_STG3_EN_WAIT  = 5'h04;
   localparam [4:0] OCAL_STG3_DEC      = 5'h05;
   localparam [4:0] OCAL_STG3_WAIT     = 5'h06;
   localparam [4:0] OCAL_STG3_CALC     = 5'h07;
   localparam [4:0] OCAL_STG3_INC      = 5'h08;
   localparam [4:0] OCAL_STG3_INC_WAIT = 5'h09;
   localparam [4:0] OCAL_STG2_SEL      = 5'h0A;
   localparam [4:0] OCAL_STG2_WAIT     = 5'h0B;
   localparam [4:0] OCAL_STG2_INC      = 5'h0C;
   localparam [4:0] OCAL_STG2_DEC      = 5'h0D;
   localparam [4:0] OCAL_STG2_DEC_WAIT = 5'h0E;
   localparam [4:0] OCAL_NEXT_DQS      = 5'h0F;
   localparam [4:0] OCAL_NEW_DQS_READ  = 5'h10;
   localparam [4:0] OCAL_INC_DONE_WAIT = 5'h11;
   localparam [4:0] OCAL_STG3_DEC_WAIT = 5'h12;
   localparam [4:0] OCAL_DEC_DONE_WAIT = 5'h13;
   localparam [4:0] OCAL_DONE          = 5'h14;
   
                        
   integer i;
   
   reg                      oclk_init_delay_start_r;
   reg [3:0]                count;
   reg                      delay_done;
   reg                      delay_done_r1;
   reg                      delay_done_r2;
   reg                      delay_done_r3;
   reg                      delay_done_r4;
   reg [5:0]                delay_cnt_r;
   reg                      po_stg3_dec;
   
   wire [DQ_WIDTH-1:0]      rd_data_rise0;  
   wire [DQ_WIDTH-1:0]      rd_data_fall0;
   wire [DQ_WIDTH-1:0]      rd_data_rise1;
   wire [DQ_WIDTH-1:0]      rd_data_fall1;
   wire [DQ_WIDTH-1:0]      rd_data_rise2;
   wire [DQ_WIDTH-1:0]      rd_data_fall2;
   wire [DQ_WIDTH-1:0]      rd_data_rise3;
   wire [DQ_WIDTH-1:0]      rd_data_fall3;
   
   reg [DQS_CNT_WIDTH:0]    cnt_dqs_r;
   wire [DQS_CNT_WIDTH+2:0] cnt_dqs_w;
   reg [DQS_CNT_WIDTH:0]    mux_sel_r;
   reg [DRAM_WIDTH-1:0]     sel_rd_rise0_r;
   reg [DRAM_WIDTH-1:0]     sel_rd_fall0_r;
   reg [DRAM_WIDTH-1:0]     sel_rd_rise1_r;
   reg [DRAM_WIDTH-1:0]     sel_rd_fall1_r;
   reg [DRAM_WIDTH-1:0]     sel_rd_rise2_r;
   reg [DRAM_WIDTH-1:0]     sel_rd_fall2_r;
   reg [DRAM_WIDTH-1:0]     sel_rd_rise3_r;
   reg [DRAM_WIDTH-1:0]     sel_rd_fall3_r;
   reg [DRAM_WIDTH-1:0]     prev_rd_rise0_r;
   reg [DRAM_WIDTH-1:0]     prev_rd_fall0_r;
   reg [DRAM_WIDTH-1:0]     prev_rd_rise1_r;
   reg [DRAM_WIDTH-1:0]     prev_rd_fall1_r;
   reg [DRAM_WIDTH-1:0]     prev_rd_rise2_r;
   reg [DRAM_WIDTH-1:0]     prev_rd_fall2_r;
   reg [DRAM_WIDTH-1:0]     prev_rd_rise3_r;
   reg [DRAM_WIDTH-1:0]     prev_rd_fall3_r;
   reg                      rd_active_r;
   reg                      rd_active_r1;
   reg                      rd_active_r2;
   reg                      rd_active_r3;
   reg                      rd_active_r4;
   reg [DRAM_WIDTH-1:0]     pat_match_fall0_r;
   reg                      pat_match_fall0_and_r;
   reg [DRAM_WIDTH-1:0]     pat_match_fall1_r;
   reg                      pat_match_fall1_and_r;
   reg [DRAM_WIDTH-1:0]     pat_match_fall2_r;
   reg                      pat_match_fall2_and_r;
   reg [DRAM_WIDTH-1:0]     pat_match_fall3_r;
   reg                      pat_match_fall3_and_r;
   reg [DRAM_WIDTH-1:0]     pat_match_rise0_r;
   reg                      pat_match_rise0_and_r;
   reg [DRAM_WIDTH-1:0]     pat_match_rise1_r;
   reg                      pat_match_rise1_and_r;
   reg [DRAM_WIDTH-1:0]     pat_match_rise2_r;
   reg                      pat_match_rise2_and_r;
   reg [DRAM_WIDTH-1:0]     pat_match_rise3_r;
   reg                      pat_match_rise3_and_r;
   reg                      pat_data_match_r;
   reg                      pat_data_match_valid_r;
   reg                      pat_data_match_valid_r1;
   reg [3:0]                stable_stg3_cnt;
   reg                      stable_eye_r;
   reg                      wait_cnt_en_r;
   reg [3:0]                wait_cnt_r;
   reg                      cnt_next_state;
   reg                      oclkdelay_calib_start_r;
   reg [5:0]                stg3_tap_cnt;
   reg [5:0]                stg3_incdec_limit;
   reg                      stg3_dec2inc;
   reg [5:0]                stg2_tap_cnt;
   reg [1:0]                stg2_inc2_cnt;
   reg [1:0]                stg2_dec2_cnt;
   reg [5:0]                stg2_dec_cnt;
   reg                      stg3_dec;
   reg                      stg3_dec_r;
   reg [4:0]                ocal_state_r;
   reg [5:0]                ocal_final_cnt_r;
   reg [5:0]                ocal_inc_cnt;
   reg [5:0]                ocal_dec_cnt;
   reg                      ocal_stg3_inc_en;
   reg                      ocal_edge1_found;
   reg                      ocal_edge2_found;
   reg [5:0]                ocal_edge1_taps;
   reg [5:0]                ocal_edge2_taps;
   reg [5:0]                ocal_right_edge;
   reg                      ocal_byte_done;
   reg                      ocal_wrlvl_done;
   reg                      ocal_wrlvl_done_r;
(* keep = "true", max_fanout = 10 *) reg   ocal_done_r /* synthesis syn_maxfan = 10 */;
   reg [5:0]                ocal_tap_cnt_r[0:DQS_WIDTH-1];
   reg                      prech_done_r;
  

   // timing registers 
  reg  stg3_tap_cnt_eq_oclkdelay_init_val;
  reg  stg3_tap_cnt_eq_0;
  reg  stg3_tap_cnt_gt_20;
  reg  stg3_tap_cnt_eq_63;
  reg  stg3_tap_cnt_less_oclkdelay_init_val;
  reg  stg3_limit;
   
   //**************************************************************************
   // Debug signals
   //**************************************************************************
   
   genvar dqs_i;
   generate
     for (dqs_i=0; dqs_i < DQS_WIDTH; dqs_i = dqs_i + 1) begin: oclkdelay_tap_cnt
       assign dbg_phy_oclkdelay_cal[6*dqs_i+:6] = ocal_tap_cnt_r[dqs_i][5:0];
     end
   endgenerate
   
   assign dbg_phy_oclkdelay_cal[57:54]   = cnt_dqs_r;
   assign dbg_phy_oclkdelay_cal[58]      = ocal_edge1_found;
   assign dbg_phy_oclkdelay_cal[59]      = ocal_edge2_found;
   assign dbg_phy_oclkdelay_cal[65:60]   = ocal_edge1_taps;
   assign dbg_phy_oclkdelay_cal[71:66]   = ocal_edge2_taps;
   assign dbg_phy_oclkdelay_cal[76:72]   = ocal_state_r;
   assign dbg_phy_oclkdelay_cal[77]      = pat_data_match_valid_r;
   assign dbg_phy_oclkdelay_cal[78]      = pat_data_match_r;
   assign dbg_phy_oclkdelay_cal[84:79]   = stg3_tap_cnt;
   assign dbg_phy_oclkdelay_cal[88:85]   = stable_stg3_cnt;
   assign dbg_phy_oclkdelay_cal[89]      = stable_eye_r;
   assign dbg_phy_oclkdelay_cal[97:90]   = prev_rd_rise0_r;
   assign dbg_phy_oclkdelay_cal[105:98]  = prev_rd_fall0_r;
   assign dbg_phy_oclkdelay_cal[113:106] = prev_rd_rise1_r;
   assign dbg_phy_oclkdelay_cal[121:114] = prev_rd_fall1_r;
   assign dbg_phy_oclkdelay_cal[129:122] = prev_rd_rise2_r;
   assign dbg_phy_oclkdelay_cal[137:130] = prev_rd_fall2_r;
   assign dbg_phy_oclkdelay_cal[145:138] = prev_rd_rise3_r;
   assign dbg_phy_oclkdelay_cal[153:146] = prev_rd_fall3_r;
   assign dbg_phy_oclkdelay_cal[154]     = rd_active_r;
   assign dbg_phy_oclkdelay_cal[162:155] = sel_rd_rise0_r;
   assign dbg_phy_oclkdelay_cal[170:163] = sel_rd_fall0_r;
   assign dbg_phy_oclkdelay_cal[178:171] = sel_rd_rise1_r;
   assign dbg_phy_oclkdelay_cal[186:179] = sel_rd_fall1_r;
   assign dbg_phy_oclkdelay_cal[194:187] = sel_rd_rise2_r;
   assign dbg_phy_oclkdelay_cal[202:195] = sel_rd_fall2_r;
   assign dbg_phy_oclkdelay_cal[210:203] = sel_rd_rise3_r;
   assign dbg_phy_oclkdelay_cal[218:211] = sel_rd_fall3_r;
   assign dbg_phy_oclkdelay_cal[219+:6]  = stg2_tap_cnt;

   assign dbg_oclkdelay_rd_data[DRAM_WIDTH*1 -1:0]              = prev_rd_rise0_r;
   assign dbg_oclkdelay_rd_data[DRAM_WIDTH*2 -1:DRAM_WIDTH*1]   = prev_rd_fall0_r;
   assign dbg_oclkdelay_rd_data[DRAM_WIDTH*3 -1:DRAM_WIDTH*2]   = prev_rd_rise1_r;
   assign dbg_oclkdelay_rd_data[DRAM_WIDTH*4 -1:DRAM_WIDTH*3]   = prev_rd_fall1_r;
   assign dbg_oclkdelay_rd_data[DRAM_WIDTH*5 -1:DRAM_WIDTH*4]   = prev_rd_rise2_r;
   assign dbg_oclkdelay_rd_data[DRAM_WIDTH*6 -1:DRAM_WIDTH*5]   = prev_rd_fall2_r;
   assign dbg_oclkdelay_rd_data[DRAM_WIDTH*7 -1:DRAM_WIDTH*6]   = prev_rd_rise3_r;
   assign dbg_oclkdelay_rd_data[DRAM_WIDTH*8 -1:DRAM_WIDTH*7]   = prev_rd_fall3_r;
   assign dbg_oclkdelay_rd_data[DRAM_WIDTH*9 -1:DRAM_WIDTH*8]   = sel_rd_rise0_r;
   assign dbg_oclkdelay_rd_data[DRAM_WIDTH*10 -1:DRAM_WIDTH*9]  = sel_rd_fall0_r;
   assign dbg_oclkdelay_rd_data[DRAM_WIDTH*11 -1:DRAM_WIDTH*10] = sel_rd_rise1_r;
   assign dbg_oclkdelay_rd_data[DRAM_WIDTH*12 -1:DRAM_WIDTH*11] = sel_rd_fall1_r;
   assign dbg_oclkdelay_rd_data[DRAM_WIDTH*13 -1:DRAM_WIDTH*12] = sel_rd_rise2_r;
   assign dbg_oclkdelay_rd_data[DRAM_WIDTH*14 -1:DRAM_WIDTH*13] = sel_rd_fall2_r;
   assign dbg_oclkdelay_rd_data[DRAM_WIDTH*15 -1:DRAM_WIDTH*14] = sel_rd_rise3_r;
   assign dbg_oclkdelay_rd_data[DRAM_WIDTH*16 -1:DRAM_WIDTH*15] = sel_rd_fall3_r;
   
   assign oclk_init_delay_done = ((SIM_CAL_OPTION  == "FAST_CAL") || (DRAM_TYPE!="DDR3")) ? 1'b1 : delay_done_r4; //(SIM_CAL_OPTION  != "NONE")
   assign oclkdelay_calib_cnt  = cnt_dqs_r;
   assign oclkdelay_calib_done = (OCAL_EN == "ON") ? ocal_done_r : 1'b1;
   
   assign cnt_dqs_w = {2'b00, cnt_dqs_r};

   always @(posedge clk)
     oclk_init_delay_start_r <= #TCQ oclk_init_delay_start;
   
   always @(posedge clk) begin
     if (rst || po_stg3_dec)
       count <= #TCQ WAIT_CNT;
     else if (oclk_init_delay_start && (count > 'd0))
       count <= #TCQ count - 1;
   end
   
   always @(posedge clk) begin
     if (rst || (delay_cnt_r == 'd0))
       po_stg3_dec      <= #TCQ 1'b0;
     else if (count == 'd1)
       po_stg3_dec      <= #TCQ 1'b1;
     else
       po_stg3_dec      <= #TCQ 1'b0;
   end
   
   //po_stg3_incdec and po_en_stg3 asserted for all data byte lanes                  
   always @(posedge clk) begin
     if (rst) begin
       po_stg3_incdec <= #TCQ 1'b0;
       po_en_stg3     <= #TCQ 1'b0;
     end else if (po_stg3_dec) begin
       po_stg3_incdec <= #TCQ 1'b0;
       po_en_stg3     <= #TCQ 1'b1;
     end else begin
       po_stg3_incdec <= #TCQ 1'b0;
       po_en_stg3     <= #TCQ 1'b0;
     end
   end

   // delay counter to count TAP_CNT cycles
   always @(posedge clk) begin  
     // load delay counter with init value of TAP_CNT
     if (rst)
       delay_cnt_r  <= #TCQ TAP_CNT;
     else if (po_stg3_dec && (delay_cnt_r > 6'd0))
       delay_cnt_r  <= #TCQ delay_cnt_r - 1;
   end
   
   // when all the ctl_lanes have their output phase shifted by 1/4 cycle, delay shifting is done.
   always @(posedge clk) begin
     if (rst)  begin
       delay_done    <= #TCQ 1'b0;
     end else if ((TAP_CNT == 6'd0) || ((delay_cnt_r == 6'd1) &&
                  (count == 'd1))) begin
       delay_done    <= #TCQ 1'b1;
     end
   end

   always @(posedge clk) begin
     delay_done_r1 <= #TCQ delay_done;
     delay_done_r2 <= #TCQ delay_done_r1;
     delay_done_r3 <= #TCQ delay_done_r2;
     delay_done_r4 <= #TCQ delay_done_r3;
   end
   
   //**************************************************************************
   // OCLKDELAY Calibration
   //**************************************************************************
   
   generate
    if (nCK_PER_CLK == 4) begin: gen_rd_data_div4
      assign rd_data_rise0 = rd_data[DQ_WIDTH-1:0];
      assign rd_data_fall0 = rd_data[2*DQ_WIDTH-1:DQ_WIDTH];
      assign rd_data_rise1 = rd_data[3*DQ_WIDTH-1:2*DQ_WIDTH];
      assign rd_data_fall1 = rd_data[4*DQ_WIDTH-1:3*DQ_WIDTH];
      assign rd_data_rise2 = rd_data[5*DQ_WIDTH-1:4*DQ_WIDTH];
      assign rd_data_fall2 = rd_data[6*DQ_WIDTH-1:5*DQ_WIDTH];
      assign rd_data_rise3 = rd_data[7*DQ_WIDTH-1:6*DQ_WIDTH];
      assign rd_data_fall3 = rd_data[8*DQ_WIDTH-1:7*DQ_WIDTH];
    end else if (nCK_PER_CLK == 2) begin: gen_rd_data_div2
      assign rd_data_rise0 = rd_data[DQ_WIDTH-1:0];
      assign rd_data_fall0 = rd_data[2*DQ_WIDTH-1:DQ_WIDTH];
      assign rd_data_rise1 = rd_data[3*DQ_WIDTH-1:2*DQ_WIDTH];
      assign rd_data_fall1 = rd_data[4*DQ_WIDTH-1:3*DQ_WIDTH];
    end
   endgenerate
   
   
   always @(posedge clk) begin
     mux_sel_r               <= #TCQ cnt_dqs_r;
     oclkdelay_calib_start_r <= #TCQ oclkdelay_calib_start;
     ocal_wrlvl_done_r       <= #TCQ ocal_wrlvl_done;
     rd_active_r             <= #TCQ phy_rddata_en;
     rd_active_r1            <= #TCQ rd_active_r;
     rd_active_r2            <= #TCQ rd_active_r1;
     rd_active_r3            <= #TCQ rd_active_r2;
     rd_active_r4            <= #TCQ rd_active_r3;
     stg3_dec_r              <= #TCQ stg3_dec;
   end
   


  // Register outputs for improved timing.
  // All bits in selected DQS group are checked in aggregate
  generate
    genvar mux_j;
    for (mux_j = 0; mux_j < DRAM_WIDTH; mux_j = mux_j + 1) begin: gen_mux_rd
      always @(posedge clk) begin
        if (phy_rddata_en) begin
          sel_rd_rise0_r[mux_j] <= #TCQ rd_data_rise0[DRAM_WIDTH*mux_sel_r + 
                                                      mux_j];
          sel_rd_fall0_r[mux_j] <= #TCQ rd_data_fall0[DRAM_WIDTH*mux_sel_r + 
                                                      mux_j];
          sel_rd_rise1_r[mux_j] <= #TCQ rd_data_rise1[DRAM_WIDTH*mux_sel_r + 
                                                      mux_j];
          sel_rd_fall1_r[mux_j] <= #TCQ rd_data_fall1[DRAM_WIDTH*mux_sel_r + 
                                                      mux_j];
          sel_rd_rise2_r[mux_j] <= #TCQ rd_data_rise2[DRAM_WIDTH*mux_sel_r + 
                                                      mux_j];
          sel_rd_fall2_r[mux_j] <= #TCQ rd_data_fall2[DRAM_WIDTH*mux_sel_r + 
                                                      mux_j];
          sel_rd_rise3_r[mux_j] <= #TCQ rd_data_rise3[DRAM_WIDTH*mux_sel_r + 
                                                      mux_j];
          sel_rd_fall3_r[mux_j] <= #TCQ rd_data_fall3[DRAM_WIDTH*mux_sel_r + 
                                                      mux_j];     
        end
      end
    end
  endgenerate
  
  always @(posedge clk)
    if (((stg3_tap_cnt_eq_oclkdelay_init_val) && rd_active_r) |
        rd_active_r4) begin
      prev_rd_rise0_r <= #TCQ sel_rd_rise0_r;
      prev_rd_fall0_r <= #TCQ sel_rd_fall0_r;
      prev_rd_rise1_r <= #TCQ sel_rd_rise1_r;
      prev_rd_fall1_r <= #TCQ sel_rd_fall1_r;
      prev_rd_rise2_r <= #TCQ sel_rd_rise2_r;
      prev_rd_fall2_r <= #TCQ sel_rd_fall2_r;
      prev_rd_rise3_r <= #TCQ sel_rd_rise3_r;
      prev_rd_fall3_r <= #TCQ sel_rd_fall3_r;
    end
  
  
  // Each bit of each byte is compared with previous data to
  // detect an edge
  generate
    genvar pt_j;
    if (nCK_PER_CLK == 4) begin: gen_pat_match_div4
      for (pt_j = 0; pt_j < DRAM_WIDTH; pt_j = pt_j + 1) begin: gen_pat_match
        always @(posedge clk) begin
          if (sel_rd_rise0_r[pt_j] == prev_rd_rise0_r[pt_j])
            pat_match_rise0_r[pt_j] <= #TCQ 1'b1;
          else
            pat_match_rise0_r[pt_j] <= #TCQ 1'b0;
          
          if (sel_rd_fall0_r[pt_j] == prev_rd_fall0_r[pt_j])
            pat_match_fall0_r[pt_j] <= #TCQ 1'b1;
          else
            pat_match_fall0_r[pt_j] <= #TCQ 1'b0;
          
          if (sel_rd_rise1_r[pt_j] == prev_rd_rise1_r[pt_j])
            pat_match_rise1_r[pt_j] <= #TCQ 1'b1;
          else
            pat_match_rise1_r[pt_j] <= #TCQ 1'b0;
          
          if (sel_rd_fall1_r[pt_j] == prev_rd_fall1_r[pt_j])
            pat_match_fall1_r[pt_j] <= #TCQ 1'b1;
          else
            pat_match_fall1_r[pt_j] <= #TCQ 1'b0;
          
          if (sel_rd_rise2_r[pt_j] == prev_rd_rise2_r[pt_j])
            pat_match_rise2_r[pt_j] <= #TCQ 1'b1;
          else
            pat_match_rise2_r[pt_j] <= #TCQ 1'b0;
          
          if (sel_rd_fall2_r[pt_j] == prev_rd_fall2_r[pt_j])
            pat_match_fall2_r[pt_j] <= #TCQ 1'b1;
          else
            pat_match_fall2_r[pt_j] <= #TCQ 1'b0;
          
          if (sel_rd_rise3_r[pt_j] == prev_rd_rise3_r[pt_j])
            pat_match_rise3_r[pt_j] <= #TCQ 1'b1;
          else
            pat_match_rise3_r[pt_j] <= #TCQ 1'b0;

          if (sel_rd_fall3_r[pt_j] == prev_rd_fall3_r[pt_j])
            pat_match_fall3_r[pt_j] <= #TCQ 1'b1;
          else
            pat_match_fall3_r[pt_j] <= #TCQ 1'b0;
        end
      end
  

       always @(posedge clk) begin
         pat_match_rise0_and_r <= #TCQ &pat_match_rise0_r;
         pat_match_fall0_and_r <= #TCQ &pat_match_fall0_r;
         pat_match_rise1_and_r <= #TCQ &pat_match_rise1_r;
         pat_match_fall1_and_r <= #TCQ &pat_match_fall1_r;
         pat_match_rise2_and_r <= #TCQ &pat_match_rise2_r;
         pat_match_fall2_and_r <= #TCQ &pat_match_fall2_r;
         pat_match_rise3_and_r <= #TCQ &pat_match_rise3_r;
         pat_match_fall3_and_r <= #TCQ &pat_match_fall3_r;
         pat_data_match_r <= #TCQ (//pat_match_rise0_and_r &&
                                   //pat_match_fall0_and_r &&
                                   pat_match_rise1_and_r &&
                                   pat_match_fall1_and_r &&
                                   pat_match_rise2_and_r &&
                                   pat_match_fall2_and_r &&
                                   pat_match_rise3_and_r &&
                                   pat_match_fall3_and_r);
         pat_data_match_valid_r <= #TCQ rd_active_r2;
       end
       
       always @(posedge clk) begin
         pat_data_match_valid_r1 <= #TCQ pat_data_match_valid_r;
       end

    end else if (nCK_PER_CLK == 2) begin: gen_pat_match_div2
      for (pt_j = 0; pt_j < DRAM_WIDTH; pt_j = pt_j + 1) begin: gen_pat_match

        always @(posedge clk) begin
          if (sel_rd_rise0_r[pt_j] == prev_rd_rise0_r[pt_j])
            pat_match_rise0_r[pt_j] <= #TCQ 1'b1;
          else
            pat_match_rise0_r[pt_j] <= #TCQ 1'b0;
          
          if (sel_rd_fall0_r[pt_j] == prev_rd_fall0_r[pt_j])
            pat_match_fall0_r[pt_j] <= #TCQ 1'b1;
          else
            pat_match_fall0_r[pt_j] <= #TCQ 1'b0;
          
          if (sel_rd_rise1_r[pt_j] == prev_rd_rise1_r[pt_j])
            pat_match_rise1_r[pt_j] <= #TCQ 1'b1;
          else
            pat_match_rise1_r[pt_j] <= #TCQ 1'b0;
          
          if (sel_rd_fall1_r[pt_j] == prev_rd_fall1_r[pt_j])
            pat_match_fall1_r[pt_j] <= #TCQ 1'b1;
          else
            pat_match_fall1_r[pt_j] <= #TCQ 1'b0;
        end
      end
  
        always @(posedge clk) begin
          pat_match_rise0_and_r <= #TCQ &pat_match_rise0_r;
          pat_match_fall0_and_r <= #TCQ &pat_match_fall0_r;
          pat_match_rise1_and_r <= #TCQ &pat_match_rise1_r;
          pat_match_fall1_and_r <= #TCQ &pat_match_fall1_r;
          pat_data_match_r <= #TCQ (//pat_match_rise0_and_r &&
                                    //pat_match_fall0_and_r &&
                                    pat_match_rise1_and_r &&
                                    pat_match_fall1_and_r);
          pat_data_match_valid_r <= #TCQ rd_active_r2;
        end
        
        always @(posedge clk) begin
         pat_data_match_valid_r1 <= #TCQ pat_data_match_valid_r;
       end
  
    end
  endgenerate
  
   // Stable count of 16 PO Stage3 taps at 2x the resolution of stage2 taps
   // Required to inhibit false edge detection due to clock jitter
   always @(posedge clk)begin
      if (rst | (pat_data_match_valid_r & ~pat_data_match_r &
          (ocal_state_r == OCAL_NEW_DQS_WAIT)) | 
		  (ocal_state_r == OCAL_STG3_CALC))
        stable_stg3_cnt <= #TCQ 'd0;
      else if ((!stg3_tap_cnt_eq_oclkdelay_init_val) & 
               pat_data_match_valid_r & pat_data_match_r &
               (ocal_state_r == OCAL_NEW_DQS_WAIT) &
               (stable_stg3_cnt < 'd8))
          stable_stg3_cnt <= #TCQ stable_stg3_cnt + 1;
   end
   
   always @(posedge clk) begin
     if (rst | (stable_stg3_cnt != 'd8))
       stable_eye_r <= #TCQ 1'b0;
     else if (stable_stg3_cnt == 'd8)
       stable_eye_r <= #TCQ 1'b1;
   end
   
   always @(posedge clk)
    if ((ocal_state_r == OCAL_STG3_SEL_WAIT) ||
        (ocal_state_r == OCAL_STG3_EN_WAIT) ||
        (ocal_state_r == OCAL_STG3_WAIT) ||
        (ocal_state_r == OCAL_STG3_INC_WAIT) ||
        (ocal_state_r == OCAL_STG3_DEC_WAIT) ||
        (ocal_state_r == OCAL_STG2_WAIT) ||
        (ocal_state_r == OCAL_STG2_DEC_WAIT) ||
        (ocal_state_r == OCAL_INC_DONE_WAIT) ||
        (ocal_state_r == OCAL_DEC_DONE_WAIT))
      wait_cnt_en_r <= #TCQ 1'b1;
    else
      wait_cnt_en_r <= #TCQ 1'b0;
   
   always @(posedge clk)
    if (!wait_cnt_en_r) begin
      wait_cnt_r      <= #TCQ 'b0;
      cnt_next_state  <= #TCQ 1'b0;
    end else begin
      if (wait_cnt_r != WAIT_CNT - 1) begin
        wait_cnt_r     <= #TCQ wait_cnt_r + 1;
        cnt_next_state <= #TCQ 1'b0;
      end else begin
        // Need to reset to 0 to handle the case when there are two
        // different WAIT states back-to-back
        wait_cnt_r     <= #TCQ 'b0;        
        cnt_next_state <= #TCQ 1'b1;
      end
    end
    
   always @(posedge clk) begin
     if (rst) begin
       for (i=0; i < DQS_WIDTH; i = i + 1) begin: rst_ocal_tap_cnt
         ocal_tap_cnt_r[i] <= #TCQ 'b0;
       end
     end else if (stg3_dec_r && ~stg3_dec)
       ocal_tap_cnt_r[cnt_dqs_r][5:0] <= #TCQ stg3_tap_cnt;
   end 
    
   always @(posedge clk) begin
     if (rst || (ocal_state_r == OCAL_NEW_DQS_READ) ||
         (ocal_state_r == OCAL_STG3_CALC) ||
         (ocal_state_r == OCAL_DONE))
       prech_done_r <= #TCQ 1'b0;
     else if (prech_done)
       prech_done_r <= #TCQ 1'b1;
   end 
  



   // setting stg3_tap_cnt == oclkdelay_int_val

   always @(posedge clk) begin
    if (rst || (ocal_state_r == OCAL_NEXT_DQS)) begin
      stg3_tap_cnt_eq_oclkdelay_init_val <= #TCQ 1'b1;
    end else begin
      if (ocal_state_r == OCAL_DONE)
        stg3_tap_cnt_eq_oclkdelay_init_val <= #TCQ 1'b0;
      else if (ocal_state_r == OCAL_STG3_DEC)
         stg3_tap_cnt_eq_oclkdelay_init_val <= #TCQ (stg3_tap_cnt == oclkdelay_init_val+1);
      else if (ocal_state_r == OCAL_STG3_INC)
           stg3_tap_cnt_eq_oclkdelay_init_val <= #TCQ (stg3_tap_cnt == oclkdelay_init_val-1);
     end // else: !if((rst || (ocal_state_r == OCAL_IDLE)) begin...
   end // always @ (posedge clk)

// setting sg3_tap_cng > 20 
   always @(posedge clk) begin
    if ((rst)|| (ocal_state_r == OCAL_NEXT_DQS)) begin
      stg3_tap_cnt_gt_20 <= #TCQ 1'b0;
    end else begin // if (rst)
      if (ocal_state_r == OCAL_STG3_DEC)
         stg3_tap_cnt_gt_20 <= #TCQ (stg3_tap_cnt >= 'd22);
      else if (ocal_state_r == OCAL_STG3_INC)
           stg3_tap_cnt_gt_20 <= #TCQ (stg3_tap_cnt >= 'd20);
     end // else: !if((rst || (ocal_state_r == OCAL_IDLE)) begin...
   end // always @ (posedge clk)

// setting sg3_tap_cnt == 0 
   always @(posedge clk) begin
    if ((rst)|| (ocal_state_r == OCAL_NEXT_DQS) || (ocal_state_r == OCAL_STG3_INC) ) begin
      stg3_tap_cnt_eq_0 <= #TCQ 1'b0;
    end else begin // if (rst)
      if (ocal_state_r == OCAL_STG3_DEC)
         stg3_tap_cnt_eq_0 <= #TCQ (stg3_tap_cnt == 'd1);
    end // else: !if((rst || (ocal_state_r == OCAL_IDLE)) begin...
   end // always @ (posedge clk)

// setting sg3_tap_cnt == 63
   always @(posedge clk) begin
    if ((rst)|| (ocal_state_r == OCAL_NEXT_DQS)) begin
      stg3_tap_cnt_eq_63 <= #TCQ 1'b0;
    end else begin // if (rst)
     if (ocal_state_r == OCAL_STG3_INC)
           stg3_tap_cnt_eq_63 <= #TCQ (stg3_tap_cnt >= 'd62);
     else if (ocal_state_r == OCAL_STG3_DEC)
            stg3_tap_cnt_eq_63 <= #TCQ 1'b0;
     end // else: !if((rst || (ocal_state_r == OCAL_IDLE)) begin...
   end // always @ (posedge clk)

// setting sg3_tap_cnt < ocaldelay_init_val
   always @(posedge clk) begin
    if ((rst)|| (ocal_state_r == OCAL_NEXT_DQS)) begin
      stg3_tap_cnt_less_oclkdelay_init_val <= #TCQ 1'b0;
    end else begin // if (rst)
      if (ocal_state_r == OCAL_STG3_DEC)
         stg3_tap_cnt_less_oclkdelay_init_val <= #TCQ (stg3_tap_cnt <= oclkdelay_init_val);
      else if (ocal_state_r == OCAL_STG3_INC)
           stg3_tap_cnt_less_oclkdelay_init_val <= #TCQ (stg3_tap_cnt <= oclkdelay_init_val-2);
     end // else: !if((rst || (ocal_state_r == OCAL_IDLE)) begin...
   end // always @ (posedge clk)
   
// setting stg3_incdec_limit == 15
   always @(posedge clk) begin
     if (rst || (ocal_state_r == OCAL_NEXT_DQS) || (ocal_state_r == OCAL_DONE)) begin
	   stg3_limit <= #TCQ 1'b0;
	 end else if ((ocal_state_r == OCAL_STG3_WAIT) || (ocal_state_r == OCAL_STG2_WAIT)) begin
	   stg3_limit <= #TCQ (stg3_incdec_limit == 'd14);
	 end
   end
	 

   // State Machine
   always @(posedge clk) begin
     if (rst) begin
       ocal_state_r      <= #TCQ OCAL_IDLE;
       cnt_dqs_r         <= #TCQ 'd0;
       stg3_tap_cnt      <= #TCQ oclkdelay_init_val;
       stg3_incdec_limit <= #TCQ 'd0;
       stg3_dec2inc      <= #TCQ 1'b0;
       stg2_tap_cnt      <= #TCQ 'd0;
       stg2_inc2_cnt     <= #TCQ 2'b00;
       stg2_dec2_cnt     <= #TCQ 2'b00;
       stg2_dec_cnt      <= #TCQ 'd0;
       stg3_dec          <= #TCQ 1'b0;
       wrlvl_final       <= #TCQ 1'b0;
       oclk_calib_resume <= #TCQ 1'b0;
       oclk_prech_req    <= #TCQ 1'b0;
       ocal_final_cnt_r  <= #TCQ 'd0;
       ocal_inc_cnt      <= #TCQ 'd0;
       ocal_dec_cnt      <= #TCQ 'd0;
       ocal_stg3_inc_en  <= #TCQ 1'b0;
       ocal_edge1_found  <= #TCQ 1'b0;
       ocal_edge2_found  <= #TCQ 1'b0;
       ocal_right_edge   <= #TCQ 'd0;
       ocal_edge1_taps   <= #TCQ 'd0;
       ocal_edge2_taps   <= #TCQ 'd0;
       ocal_byte_done    <= #TCQ 1'b0;
       ocal_wrlvl_done   <= #TCQ 1'b0;
       ocal_if_rst       <= #TCQ 1'b0;
       ocal_done_r       <= #TCQ 1'b0;
       po_stg23_sel      <= #TCQ 1'b0;
       po_en_stg23       <= #TCQ 1'b0;
       po_stg23_incdec   <= #TCQ 1'b0;
     end else begin
       case (ocal_state_r)
        
        OCAL_IDLE: begin
          if (oclkdelay_calib_start && ~oclkdelay_calib_start_r) begin
            ocal_state_r <= #TCQ OCAL_NEW_DQS_WAIT;
            stg3_tap_cnt <= #TCQ oclkdelay_init_val;
            stg2_tap_cnt <= #TCQ wl_po_fine_cnt[((cnt_dqs_w << 2) + 
                                               (cnt_dqs_w << 1))+:6];
          end
        end
        
        OCAL_NEW_DQS_READ: begin
          oclk_prech_req    <= #TCQ 1'b0;
          oclk_calib_resume <= #TCQ 1'b0;
          if (pat_data_match_valid_r)
            ocal_state_r <= #TCQ OCAL_NEW_DQS_WAIT;
        end
        
        OCAL_NEW_DQS_WAIT: begin
          oclk_calib_resume <= #TCQ 1'b0;
          oclk_prech_req    <= #TCQ 1'b0;
          po_en_stg23       <= #TCQ 1'b0;
          po_stg23_incdec   <= #TCQ 1'b0;
          if (pat_data_match_valid_r && !stg3_tap_cnt_eq_oclkdelay_init_val) begin
            if ((stg3_limit && ~ocal_stg3_inc_en)  ||
                (stg3_tap_cnt == 0)) begin
              // No write levling performed to avoid stage 2 coarse dec.
              // Therefore stage 3 taps can only be decremented by an
              // additional 15 taps after stage 2 taps reach 63.
              ocal_state_r      <= #TCQ OCAL_STG3_SEL;
              ocal_stg3_inc_en  <= #TCQ 1'b1;
              stg3_incdec_limit <= #TCQ 'd0;
            // An edge was detected
            end else if (~pat_data_match_r) begin
              // Sticky bit - asserted after we encounter an edge, although
              // the current edge may not be considered the "first edge" this
              // just means we found at least one edge
              if (~ocal_stg3_inc_en)
                ocal_edge1_found <= #TCQ 1'b1;
              // Sarting point was in the jitter region close to the right edge
              if (~stable_eye_r && ~ocal_stg3_inc_en && stg3_tap_cnt_gt_20) begin
                ocal_right_edge <= #TCQ stg3_tap_cnt;
                ocal_state_r    <= #TCQ OCAL_STG3_SEL;
              // Starting point was in the valid window close to the right edge
              // Or away from the right edge hence no stable_eye_r condition
              // Or starting point was in the right jitter region and ocal_right_edge
              // is detected
              end else if (ocal_stg3_inc_en) begin
                // Both edges found
                ocal_state_r     <= #TCQ OCAL_STG3_CALC;
                ocal_edge2_found <= #TCQ 1'b1;
                ocal_edge2_taps  <= #TCQ stg3_tap_cnt - 1;
              // Starting point in the valid window away from left edge
              // Assuming starting point will not be in valid window close to
              // left edge
//              end else if (~ocal_edge1_found && stable_eye_r) begin
              end else if (stable_eye_r) begin

                ocal_edge1_taps   <= #TCQ stg3_tap_cnt + 1;
                ocal_state_r     <= #TCQ OCAL_STG3_SEL;
                ocal_stg3_inc_en  <= #TCQ 1'b1;
                stg3_incdec_limit <= #TCQ 'd0;
              end else
                ocal_state_r     <= #TCQ OCAL_STG3_SEL;
            end else
              ocal_state_r   <= #TCQ OCAL_STG3_SEL;
          end else if (stg3_tap_cnt_eq_oclkdelay_init_val)
              ocal_state_r     <= #TCQ OCAL_STG3_SEL;
          else if ((stg3_limit && ocal_stg3_inc_en) ||
                   (stg3_tap_cnt_eq_63)) begin
            ocal_state_r      <= #TCQ OCAL_STG3_CALC;
            stg3_incdec_limit <= #TCQ 'd0;
          end
        end
       
        OCAL_STG3_SEL: begin
          po_stg23_sel    <= #TCQ 1'b1;
          ocal_wrlvl_done <= #TCQ 1'b0;
          ocal_state_r    <= #TCQ OCAL_STG3_SEL_WAIT;
        end
          
        OCAL_STG3_SEL_WAIT: begin
          if (cnt_next_state) begin
            ocal_state_r      <= #TCQ OCAL_STG3_EN_WAIT;
            if (ocal_stg3_inc_en) begin
              po_stg23_incdec <= #TCQ 1'b1;
              if (stg3_tap_cnt_less_oclkdelay_init_val) begin
                ocal_inc_cnt   <= #TCQ oclkdelay_init_val - stg3_tap_cnt;
                stg3_dec2inc   <= #TCQ 1'b1;
                oclk_prech_req <= #TCQ 1'b1;
              end                  
            end else begin
              po_stg23_incdec <= #TCQ 1'b0;
              if (stg3_dec)
                ocal_dec_cnt  <= #TCQ ocal_final_cnt_r;
            end
          end
        end
        
        OCAL_STG3_EN_WAIT: begin
          if (cnt_next_state) begin
            if (ocal_stg3_inc_en)
              ocal_state_r    <= #TCQ OCAL_STG3_INC;
            else
              ocal_state_r    <= #TCQ OCAL_STG3_DEC;
          end
        end
        
        OCAL_STG3_DEC: begin
          po_en_stg23     <= #TCQ 1'b1;
          stg3_tap_cnt    <= #TCQ stg3_tap_cnt - 1;
          if (ocal_dec_cnt == 1) begin
            ocal_byte_done <= #TCQ 1'b1;
            ocal_state_r   <= #TCQ OCAL_DEC_DONE_WAIT;
            ocal_dec_cnt   <= #TCQ ocal_dec_cnt - 1;
          end else if (ocal_dec_cnt > 'd0) begin
            ocal_state_r   <= #TCQ OCAL_STG3_DEC_WAIT;
            ocal_dec_cnt   <= #TCQ ocal_dec_cnt - 1;
          end else
          ocal_state_r <= #TCQ OCAL_STG3_WAIT;
        end
        
        OCAL_STG3_DEC_WAIT: begin
          po_en_stg23     <= #TCQ 1'b0;
          if (cnt_next_state) begin
            if (ocal_dec_cnt > 'd0)
              ocal_state_r  <= #TCQ OCAL_STG3_DEC;
            else
              ocal_state_r    <= #TCQ OCAL_DEC_DONE_WAIT;
          end
        end
        
        OCAL_DEC_DONE_WAIT: begin
        // Required to make sure that po_stg23_incdec
        // de-asserts some time after de-assertion of
        // po_en_stg23
        po_en_stg23     <= #TCQ 1'b0;        
        if (cnt_next_state) begin
          // Final stage 3 decrement completed, proceed
          // to stage 2 tap decrement
          ocal_state_r    <= #TCQ OCAL_STG2_SEL;          
          po_stg23_incdec <= #TCQ 1'b0;
          stg3_dec        <= #TCQ 1'b0;
        end
        end
        
        OCAL_STG3_WAIT: begin
          po_en_stg23     <= #TCQ 1'b0;
          if (cnt_next_state) begin
            po_stg23_incdec <= #TCQ 1'b0;
            if ((stg2_tap_cnt != 6'd63) || (stg2_tap_cnt != 6'd0))
            ocal_state_r    <= #TCQ OCAL_STG2_SEL;
            else begin
              oclk_calib_resume <= #TCQ 1'b1;
              ocal_state_r      <= #TCQ OCAL_NEW_DQS_WAIT;
              stg3_incdec_limit <= #TCQ stg3_incdec_limit + 1;
            end              
          end
        end
        
        OCAL_STG2_SEL: begin
          po_stg23_sel    <= #TCQ 1'b0;
          po_en_stg23     <= #TCQ 1'b0;
          po_stg23_incdec <= #TCQ 1'b0;
          ocal_state_r    <= #TCQ OCAL_STG2_WAIT;
          stg2_inc2_cnt   <= #TCQ 2'b01;
          stg2_dec2_cnt   <= #TCQ 2'b01;
        end
        
        OCAL_STG2_WAIT: begin
          po_en_stg23     <= #TCQ 1'b0;
          po_stg23_incdec <= #TCQ 1'b0;
          if (cnt_next_state) begin
            if (ocal_byte_done) begin
              if (stg2_tap_cnt > 'd0) begin
                // Decrement stage 2 taps to '0' before
                // final write level is performed
                ocal_state_r <= #TCQ OCAL_STG2_DEC;
                stg2_dec_cnt <= #TCQ stg2_tap_cnt;
              end else begin
                ocal_state_r   <= #TCQ OCAL_NEXT_DQS;
                ocal_byte_done <= #TCQ 1'b0;
              end                
            end else if (stg3_dec2inc && (stg2_tap_cnt > 'd0)) begin
              // Decrement stage 2 tap to initial value before
              // edge 2 detection begins
              ocal_state_r <= #TCQ OCAL_STG2_DEC;
              stg2_dec_cnt <= #TCQ stg2_tap_cnt -
                              wl_po_fine_cnt[((cnt_dqs_w << 2) + 
                                             (cnt_dqs_w << 1))+:6];
            end else if (~ocal_stg3_inc_en && (stg2_tap_cnt < 6'd63)) begin
              // Increment stage 2 taps by 2 for every stage 3 tap decrement
              // as part of edge 1 detection to avoid tDQSS violation between
              // write DQS and CK 
              ocal_state_r <= #TCQ OCAL_STG2_INC;              
            end else if (ocal_stg3_inc_en && (stg2_tap_cnt > 6'd0)) begin
              // Decrement stage 2 taps by 2 for every stage 3 tap increment
              // as part of edge 2 detection to avoid tDQSS violation between
              // write DQS and CK
              ocal_state_r    <= #TCQ OCAL_STG2_DEC;
            end else begin
              oclk_calib_resume <= #TCQ 1'b1;
              ocal_state_r      <= #TCQ OCAL_NEW_DQS_WAIT;
              stg3_incdec_limit <= #TCQ stg3_incdec_limit + 1;
            end
          end
        end
        
        OCAL_STG2_INC: begin
          po_en_stg23     <= #TCQ 1'b1;
          po_stg23_incdec <= #TCQ 1'b1;
          stg2_tap_cnt    <= #TCQ stg2_tap_cnt + 1;
          if (stg2_inc2_cnt > 2'b00) begin
            stg2_inc2_cnt <= stg2_inc2_cnt - 1;
            ocal_state_r  <= #TCQ OCAL_STG2_WAIT;
          end else if (stg2_tap_cnt == 6'd62) begin
            ocal_state_r    <= #TCQ OCAL_STG2_WAIT;
          end else begin
            oclk_calib_resume <= #TCQ 1'b1;
            ocal_state_r      <= #TCQ OCAL_NEW_DQS_WAIT;
          end
        end

        OCAL_STG2_DEC: begin
          po_en_stg23     <= #TCQ 1'b1;
          po_stg23_incdec <= #TCQ 1'b0;
          stg2_tap_cnt    <= #TCQ stg2_tap_cnt - 1;
          if (stg2_dec_cnt > 6'd0) begin
            stg2_dec_cnt  <= #TCQ stg2_dec_cnt - 1;
          ocal_state_r    <= #TCQ OCAL_STG2_DEC_WAIT;
          end else if (stg2_dec2_cnt > 2'b00) begin
            stg2_dec2_cnt <= stg2_dec2_cnt - 1;
            ocal_state_r  <= #TCQ OCAL_STG2_WAIT;
          end else if (stg2_tap_cnt == 6'd1)
            ocal_state_r  <= #TCQ OCAL_STG2_WAIT;
          else begin
            oclk_calib_resume <= #TCQ 1'b1;
            ocal_state_r      <= #TCQ OCAL_NEW_DQS_WAIT;
          end
        end
        
        OCAL_STG2_DEC_WAIT: begin
          po_en_stg23     <= #TCQ 1'b0;
          po_stg23_incdec <= #TCQ 1'b0;
          if (cnt_next_state) begin 
            if (stg2_dec_cnt > 6'd0) begin            
              ocal_state_r <= #TCQ OCAL_STG2_DEC;
            end else if (ocal_byte_done) begin
              ocal_state_r   <= #TCQ OCAL_NEXT_DQS;
              ocal_byte_done <= #TCQ 1'b0;
            end else if (prech_done_r && stg3_dec2inc) begin
              stg3_dec2inc      <= #TCQ 1'b0;
              if (stg3_tap_cnt_eq_63)
                ocal_state_r   <= #TCQ OCAL_STG3_CALC;
              else begin
                ocal_state_r      <= #TCQ OCAL_NEW_DQS_READ;
                oclk_calib_resume <= #TCQ 1'b1;
              end
            end
          end
        end
        
        OCAL_STG3_CALC: begin
          // ocal_right_edge is asserted when an edge is detected
          // within 8 tap decrements from the initial 30 taps.
          if (|ocal_right_edge) begin
            // edge2 is either not detected or edge2 is less than
            // 8 taps away from ocal_right_edge
		    if (~ocal_edge2_found | (ocal_edge2_found & (((ocal_edge2_taps - ocal_right_edge) < 'd8) | (ocal_edge1_taps > 'd0))))
              ocal_final_cnt_r 
                <=  #TCQ ((ocal_right_edge - ocal_edge1_taps)>>1) + 1 +
                          (stg3_tap_cnt - ocal_right_edge);
	        // edge2 is detected more than 8 taps away from ocal_right_edge
            else
			  ocal_final_cnt_r 
                <=  #TCQ ((ocal_edge2_taps - ocal_right_edge)>>1) + 1;
		  // Both edges found. The left edge is the first
          // edge since we start from 0 and increment until
          // second edge or 63 taps
          end else if (ocal_edge2_found && ocal_edge1_found)
            ocal_final_cnt_r 
              <=  #TCQ ((ocal_edge2_taps -
                         ocal_edge1_taps)>>1) + 1;
          else if (ocal_edge2_found && ~ocal_edge1_found)
            ocal_final_cnt_r 
              <=  #TCQ (ocal_edge2_taps>>1) + 1;
          else if (~ocal_edge2_found && ocal_edge1_found)
            // This case is possible if write level stg2
            // tap values are very small 
            ocal_final_cnt_r 
              <=  #TCQ ((stg3_tap_cnt - ocal_edge1_taps)>>1);
          else
            ocal_final_cnt_r <= #TCQ (stg3_tap_cnt>>1);
          // Now use the value we just calculated place stage3 taps
          // to the desired calibration point
          //if (~ocal_edge2_found && ~ocal_edge1_found) begin
          //  ocal_state_r   <= #TCQ OCAL_STG2_SEL;
          //  ocal_byte_done <= #TCQ 1'b1;
          //  stg3_dec       <= #TCQ 1'b0;
          //end else begin
          ocal_state_r <= #TCQ OCAL_STG3_SEL;
          stg3_dec         <= #TCQ 1'b1;
          //end
          ocal_stg3_inc_en <= #TCQ 1'b0;
        end
        
        OCAL_STG3_INC: begin
          po_en_stg23     <= #TCQ 1'b1;
          stg3_tap_cnt    <= #TCQ stg3_tap_cnt + 1;
          if (ocal_inc_cnt > 'd0)
          ocal_inc_cnt    <= #TCQ ocal_inc_cnt - 1;
          if (ocal_inc_cnt == 1)
            ocal_state_r   <= #TCQ OCAL_INC_DONE_WAIT;
          else
            ocal_state_r    <= #TCQ OCAL_STG3_INC_WAIT;
        end
        
        OCAL_STG3_INC_WAIT: begin
          po_en_stg23     <= #TCQ 1'b0;
          po_stg23_incdec <= #TCQ 1'b1;
          if (cnt_next_state) begin
            if (ocal_inc_cnt > 'd0)
            ocal_state_r  <= #TCQ OCAL_STG3_INC;
            else begin
              ocal_state_r    <= #TCQ OCAL_STG2_SEL;
              po_stg23_incdec <= #TCQ 1'b0;
            end
          end
        end
        
        OCAL_INC_DONE_WAIT: begin
        // Required to make sure that po_stg23_incdec
        // de-asserts some time after de-assertion of
        // po_en_stg23
        po_en_stg23     <= #TCQ 1'b0;
        oclk_prech_req  <= #TCQ 1'b0;        
        if (cnt_next_state) begin
          ocal_state_r <= #TCQ OCAL_STG2_SEL;
          
          po_stg23_incdec <= #TCQ 1'b0;
        end
        end        
        
        OCAL_NEXT_DQS: begin
          po_en_stg23       <= #TCQ 1'b0;
          po_stg23_incdec   <= #TCQ 1'b0;
          stg3_tap_cnt      <= #TCQ 6'd0;
          ocal_edge1_found  <= #TCQ 1'b0;
          ocal_edge2_found  <= #TCQ 1'b0;
          ocal_edge1_taps   <= #TCQ 'd0;
          ocal_edge2_taps   <= #TCQ 'd0;
          ocal_right_edge   <= #TCQ 'd0;
		  stg3_incdec_limit <= #TCQ 'd0;
          oclk_prech_req    <= #TCQ 1'b1;
          if (cnt_dqs_r == DQS_WIDTH-1)
            wrlvl_final <= #TCQ 1'b1;
          if (prech_done) begin
            if (cnt_dqs_r == DQS_WIDTH-1)
              // If the last DQS group was just finished,
              // then end of calibration
              ocal_state_r <= #TCQ OCAL_DONE;
            else begin
              // Continue to next DQS group
              cnt_dqs_r    <= #TCQ cnt_dqs_r + 1;
              ocal_state_r <= #TCQ OCAL_NEW_DQS_READ;
              stg3_tap_cnt <= #TCQ oclkdelay_init_val;
              stg2_tap_cnt <= #TCQ wl_po_fine_cnt[(((cnt_dqs_w+1) << 2) + 
                                               ((cnt_dqs_w+1) << 1))+:6];
            end
          end
        end  
        
        OCAL_DONE: begin
          oclk_prech_req <= #TCQ 1'b0;
          po_stg23_sel   <= #TCQ 1'b0;
          ocal_done_r    <= #TCQ 1'b1;
        end
       endcase
     end
   end
   
endmodule
