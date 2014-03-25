//*****************************************************************************
// (c) Copyright 2009 - 2011 Xilinx, Inc. All rights reserved.
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
// /___/  \  /    Vendor             : Xilinx
// \   \   \/     Version            : 1.2
//  \   \         Application        : MIG
//  /   /         Filename           : example_top.v
// /___/   /\     Date Last Modified : $Date: 2011/06/23 21:17:41 $
// \   \  /  \    Date Created       : Mon Jun 23 2008
//  \___\/\___\
//
// Device           : 7 Series
// Design Name      : DDR3 SDRAM
// Purpose          :
//   Checks DDR window margins under ChipScope control.Adapted from V6
//   design from JLogue
// Reference        :
// Revision History :
//*****************************************************************************

`timescale 1ps/1ps

module mig_7series_v1_8_chk_win #
  (
   parameter TCQ            = 100,    // clk->q delay (simulation only)
   parameter nCK_PER_CLK    = 4,      // # of memory CKs per fabric CLK
   parameter DLY_WIDTH      = 16,     // delay counter width
   parameter DQ_PER_DQS     = 8,      // # of DQ per DQS strobe
   parameter DQ_WIDTH       = 64,     // # of total DQ bits
   parameter SC_WIDTH       = 3,      // spread counter width
   parameter SDC_WIDTH      = 5,      // short delay counter width
   parameter WIN_SIZE       = 6,      // delay tap counter width
   parameter SIM_OPTION     = "FALSE"   // reduce state delays for sim only
   )
  (
   input                                  clk,
   input                                  rst,
   input                                  mem_pattern_init_done,
   input                                  win_start,
   input                                  win_sel_pi_pon,
   input                                  read_valid,     // data valid 
   input [6:0]                            win_bit_select, // readback select
   input [6:0]                            win_byte_select, // readback select
   input [(2*nCK_PER_CLK*DQ_WIDTH)-1:0]   cmp_data,       // expected data
   input [(2*nCK_PER_CLK*DQ_WIDTH)-1:0]   rd_data,        // actual data
   input [5:0]                            pi_curr_tap_cnt,   // current delay taps
   input [8:0]                            po_curr_tap_cnt,   // current delay taps
   output [5:0]                           pi_left_ram_out,
   output [5:0]                           pi_right_ram_out,
   output [8:0]                           po_left_ram_out,
   output [8:0]                           po_right_ram_out,
   output reg                             win_active,     // in-progr indicator
   output reg                             win_clr_error,  // clear error flag
   output reg                             pi_win_up,         // inc delay taps
   output reg                             pi_win_down,       // dec delay taps
   output reg                             po_win_up,         // inc delay taps
   output reg                             po_stg23_sel,         // inc delay taps
   output reg                             po_win_down,       // dec delay taps
   output reg                             po_win_tg_rst,
   output reg [6:0]                       win_current_bit,  // status indicator
   output reg [3:0]                       win_current_byte, // status indicator
   output [63:0]                          dbg_win_chk,
   input                                  dbg_clear_error
   );

  //***************************************************************************
  
  function integer clogb2 (input integer size);
    begin
      size = size - 1;
      for (clogb2=1; size>1; clogb2=clogb2+1)
            size = size >> 1;
    end
  endfunction
  
  // Delay counter counts abbreviated number of reads for simulation
  localparam DLY_CNTR_WIDTH = (SIM_OPTION == "TRUE") ? 6 : DLY_WIDTH;

  localparam DQ_BITS         = clogb2(DQ_WIDTH);  
  localparam DQ_PER_DQS_BITS = clogb2(DQ_PER_DQS);
  // Always ensure that selector for current byte has at least one bit
  localparam DQS_SELECT_BITS 
             = (DQ_BITS > DQ_PER_DQS_BITS) ? (DQ_BITS - DQ_PER_DQS_BITS) : 1;
  localparam BYTE = DQ_WIDTH/DQ_PER_DQS;

  //***************************************************************************
  // pd state assignments
  //***************************************************************************

  localparam WIN_IDLE         = 4'd0;
  localparam WIN_INC_TAP      = 4'd1;
  localparam WIN_SHORT_DLY    = 4'd2;
  localparam WIN_WAIT_INC     = 4'd3;
  localparam WIN_CHK_ERR_R    = 4'd4;
  localparam WIN_BACK_R       = 4'd5;
  localparam WIN_SS_R         = 4'd6;
  localparam WIN_RTN_TAP_R    = 4'd7;
  localparam WIN_DEC_TAP      = 4'd8;
  localparam WIN_WAIT_DEC     = 4'd9;
  localparam WIN_CHK_ERR_L    = 4'd10;
  localparam WIN_BACK_L       = 4'd11;
  localparam WIN_SS_L         = 4'd12;
  localparam WIN_RTN_TAP_L    = 4'd13;
  localparam WIN_CK_DONE      = 4'd14;

//  reg [6:0]                            bit_cntr;
  reg [3:0]                            byte_cntr;
//  reg [4:0]                            simple_bit_cntr;
//  reg [3:0]                            bit_select_cntr;    
//  wire [DQ_PER_DQS_BITS-1:0]           bit_select;  
  reg                                  clr_delay_cntr;  
  reg                                  clr_error;
  reg                                  clr_win_active;
  reg [(2*nCK_PER_CLK*DQ_PER_DQS)-1:0] cmp_data_r;
  reg [5:0]                   pi_curr_tap_cnt_r;  
  reg [8:0]                   po_curr_tap_cnt_r;  
  reg [8:0]                   po_curr_tap_cnt_r2;  
  wire                                 delay_cntr_ce;
  reg [DLY_CNTR_WIDTH-1:0]             delay_cntr;
  wire                                 delay_cntr_done;
  wire                                 dec_offset;
  reg                                  dec_offset_cntr;
  reg                                  dec_tap;
  //wire [DQS_SELECT_BITS-1:0]           dqs_select;
  reg [DQS_SELECT_BITS-1:0]            dqs_select; 
  wire                                 eod;
  wire                                 pi_eod;
  wire                                 po_eod;
//  reg                                  inc_bit_cntr;
  reg                                  inc_byte_cntr;
  wire                                 inc_offset;
  reg                                  inc_offset_cntr;
  reg                                  inc_tap;
  wire                                 init_offset;
  wire [DQ_PER_DQS-1:0]                lane_error_or [0:2*nCK_PER_CLK];
  reg [DQ_PER_DQS-1:0]                 lane_error_r;
  wire                                 left_side_spread;
  reg [8:0]                            offset_cntr;
  reg                                  read_valid_r;
  reg [(2*nCK_PER_CLK*DQ_PER_DQS)-1:0] rd_data_r;
  wire                                 right_side_spread;
  wire                                 sc_ce;
  wire                                 sc_middle;
  reg                                  set_win_active;
  reg [SDC_WIDTH-1:0]                  short_dly_cntr;
  wire                                 short_dly_done;
  reg [SC_WIDTH-1:0]                   spread_cntr;
  reg                                  spread_cntr_msb_r;
  wire                                 spread_done;
  wire                                 spread_state;
  wire                                 error;
  reg                                  ss_left;
  reg                                  ss_right;
  reg                                  pi_ss_left_r;
  reg                                  pi_ss_right_r;
  reg                                  po_ss_left_r;
  reg                                  po_ss_right_r;
  reg                                  start_short_dly;
  reg [3:0]                            win_next_state;  
  reg [3:0]                            win_state_r;
  reg [DQ_PER_DQS-1:0]                 word_error_r [0:2*nCK_PER_CLK-1];
  reg                                  error_dqs; //ss
  reg                                  win_sel_pi_pon_r;
  reg                                  mem_pattern_init_done_r;
  reg                                  po_win_tg_rst_d;
  reg [8:0]                            po_rst_cntr;
  reg                                  po_stg23_sel_d;

  wire                                 mem_pattern_init_done_edge;
  
  //***************************************************************************
  // some assignments
  //***************************************************************************

  // Register current tap count - extra one clock pipe delay is not an issue
  always @(posedge clk)
  begin
    pi_curr_tap_cnt_r <= #TCQ pi_curr_tap_cnt;
    po_curr_tap_cnt_r <= #TCQ po_curr_tap_cnt;
    mem_pattern_init_done_r <= #TCQ mem_pattern_init_done;
  end

  always @(posedge clk)
  begin
    if (po_win_up|po_win_down)
      po_curr_tap_cnt_r2 <= #TCQ po_curr_tap_cnt_r;
  end


  assign mem_pattern_init_done_edge = mem_pattern_init_done & (!mem_pattern_init_done_r);

  always @(posedge clk)
  begin
    if (win_start)
    win_sel_pi_pon_r <= #TCQ win_sel_pi_pon;
  end

  // End of delay occurs when either the current tap count is 0 or when
  // it's reached the maximum 
  assign pi_eod   = (pi_curr_tap_cnt_r[5:0] == {6{1'b0}}) ||
                 (pi_curr_tap_cnt_r[5:0] == {6{1'b1}});

// stage 3 has 64 taps. MSB are used for stage2 taps count
  assign po_eod   = (po_curr_tap_cnt_r == {6{1'b0}}) ||
                 (po_curr_tap_cnt_r == {6{1'b1}});


/* assign po_eod   = (&offset_cntr [3:0]) |
                   (po_curr_tap_cnt_r == po_curr_tap_cnt_r2);
*/
  assign eod = win_sel_pi_pon_r? pi_eod: po_eod;

  // Status - indicates current bit being processed
  always @(posedge clk) begin
//    win_current_bit  <= #TCQ bit_cntr;
    win_current_byte <= #TCQ byte_cntr;
  end
  
  //***************************************************************************
  // win_active
  //***************************************************************************

  always @(posedge clk)
    if (rst) 
      win_active <= #TCQ 1'b0;
    else     
      win_active <= #TCQ ~clr_win_active & (win_active | set_win_active);

  //***************************************************************************
  // win_clr_error register
  //***************************************************************************

  always @(posedge clk)
    if (rst) 
      win_clr_error <= #TCQ 1'b0;
    else     
      win_clr_error <= #TCQ clr_error | (mem_pattern_init_done_edge & (!win_sel_pi_pon_r));

  //***************************************************************************
  // Bit counter
  //***************************************************************************
/*
  always @(posedge clk)
    if (rst | win_start)   
      bit_cntr <= #TCQ 'b0;
    else if (inc_bit_cntr) begin
      if (bit_cntr == DQ_WIDTH-1)
        bit_cntr <= #TCQ 'b0;
      else      
        bit_cntr <= #TCQ bit_cntr + 1;
    end
    
  always @(posedge clk)
    if (rst | win_start) begin
      bit_select_cntr <= #TCQ 'b0;
      dqs_select      <= #TCQ 'b0;
    end else if (inc_bit_cntr) begin
      if (bit_select_cntr == (DQ_PER_DQS-1)) begin
        bit_select_cntr <= #TCQ 'b0;
        dqs_select      <= #TCQ dqs_select + 1; //select next dqs group
      end else begin     
        bit_select_cntr <= #TCQ bit_select_cntr + 1;
        dqs_select      <= #TCQ dqs_select;
      end
    end
*/
  // Portions of bit_cntr used to select: (1) current DQS group, (2) current
  // bit within that DQS group. Used for various MUXes in error detection. 
  //assign bit_select = bit_cntr[DQ_PER_DQS_BITS-1:0];
/*  assign bit_select = bit_select_cntr[DQ_PER_DQS_BITS-1:0];
generate
    if (DQ_BITS > DQ_PER_DQS_BITS) begin: gen_dqs_select_dqs_gt1
      assign dqs_select = bit_cntr[DQ_BITS-1:DQ_BITS-DQ_PER_DQS_BITS];
    end else begin: gen_dqs_select_dqs_eq1
      assign dqs_select = 1'b0;
    end
  endgenerate*/
  
  //***************************************************************************
  // Byte counter
  //***************************************************************************
  /*
  always @(posedge clk)
    if (rst | win_start) begin 
      simple_bit_cntr <= #TCQ 'b0;
      inc_byte_cntr   <= #TCQ 1'b0;
    end else if (inc_bit_cntr) begin
      if (simple_bit_cntr == DQ_PER_DQS - 1) begin
        simple_bit_cntr <= #TCQ 'b0;
        inc_byte_cntr   <= #TCQ 1'b1;
      end else  begin     
        simple_bit_cntr <= #TCQ simple_bit_cntr + 1;
        inc_byte_cntr   <= #TCQ 1'b0;
      end
    end else begin
      inc_byte_cntr   <= #TCQ 1'b0;
    end
  */
  //simple way to keep track of which byte lane
  always @(posedge clk)
    if (rst | win_start)   
      byte_cntr <= #TCQ 'b0;
    else if (inc_byte_cntr) begin
      if (byte_cntr == BYTE-1)
        byte_cntr <= #TCQ 'b0;
      else      
        byte_cntr <= #TCQ byte_cntr + 1;
    end
  
  
  //***************************************************************************
  // Offset counter
  //***************************************************************************

  assign inc_offset  = inc_offset_cntr;
  assign dec_offset  = dec_offset_cntr | sc_middle;

  always @(posedge clk)
    if (rst)            
      offset_cntr <= #TCQ 'b0;
    else if (inc_offset) 
      offset_cntr <= #TCQ offset_cntr + 1;
    else if (dec_offset) 
      offset_cntr <= #TCQ offset_cntr - 1;

  assign win_offset_cntr = offset_cntr;

  //***************************************************************************
  // Delay counter
  //***************************************************************************

  always @(posedge clk)
    if (rst) 
      read_valid_r <= #TCQ 1'b0;
    else     
      read_valid_r <= #TCQ read_valid;

  assign delay_cntr_ce = ~delay_cntr_done & read_valid_r & mem_pattern_init_done_r;

  always @(posedge clk)
    if (rst)                 
      delay_cntr <= #TCQ {1'b1, {DLY_CNTR_WIDTH-1{1'b0}}};
    else if (clr_delay_cntr) 
      delay_cntr <= #TCQ 'b0;
    else if (delay_cntr_ce)   
      delay_cntr <= #TCQ delay_cntr + 1;

  assign delay_cntr_done = delay_cntr[DLY_CNTR_WIDTH-1];

  //***************************************************************************
  // Pipeline ss_left and ss_right for speed
  //***************************************************************************

  always @(posedge clk) begin
    if (rst) begin
      pi_ss_right_r <= #TCQ 1'b0;
      pi_ss_left_r  <= #TCQ 1'b0;
    end else begin
      pi_ss_right_r <= #TCQ ss_right & win_sel_pi_pon_r;
      pi_ss_left_r  <= #TCQ ss_left & win_sel_pi_pon_r;
    end
  end

 //***************************************************************************
  // Pipeline ss_left and ss_right for speed
  //***************************************************************************

  always @(posedge clk) begin
    if (rst) begin
      po_ss_right_r <= #TCQ 1'b0;
      po_ss_left_r  <= #TCQ 1'b0;
      po_win_tg_rst = 1'b0;
    end else begin
      po_ss_right_r <= #TCQ ss_right & !win_sel_pi_pon_r;
      po_ss_left_r  <= #TCQ ss_left & !win_sel_pi_pon_r;
      po_win_tg_rst = /*(po_rst_cntr != 9'h1ff)*/po_win_tg_rst_d & !win_sel_pi_pon_r;
    end
  end

  always @ (posedge clk)
  begin
    if (rst)
      po_rst_cntr <= 9'h1ff;
    else if (po_win_tg_rst_d & !win_sel_pi_pon_r)
      po_rst_cntr <= 9'h0;
    else if (po_rst_cntr != 9'h1ff)
      po_rst_cntr <= po_rst_cntr + 1'b1;
  end      

  //***************************************************************************
  // instantiate distributed RAM to hold left and right margins for each bit
  //***************************************************************************

  generate
    genvar i_pi;
    for (i_pi= 0; i_pi< 6; i_pi= i_pi+1) begin: gen_ram_pi
      RAM128X1D #
        (
         .INIT (64'h0000000000000000)
         )
        u_pi_left_ram
          (
           .DPO   (pi_left_ram_out[i_pi]),
           .SPO   (),
           .A     (byte_cntr),
           .D     (offset_cntr[i_pi]),
           .DPRA  (win_byte_select),
           .WCLK  (clk),
           .WE    (pi_ss_left_r)
           );

      RAM128X1D #
        (
         .INIT (64'h0000000000000000)
         )
        u_pi_right_ram
          (
           .DPO   (pi_right_ram_out[i_pi]),
           .SPO   (),
           .A     (byte_cntr),
           .D     (offset_cntr[i_pi]),
           .DPRA  (win_byte_select),
           .WCLK  (clk),
           .WE    (pi_ss_right_r)
           );
    end

  endgenerate

  generate
    genvar i_po;
    for (i_po = 0; i_po < 9; i_po = i_po+1) begin: gen_ram_po
      RAM128X1D #
        (
         .INIT (64'h0000000000000000)
         )
        u_po_left_ram
          (
           .DPO   (po_left_ram_out[i_po]),
           .SPO   (),
           .A     (byte_cntr),
           .D     (offset_cntr[i_po]),
           .DPRA  (win_byte_select),
           .WCLK  (clk),
           .WE    (po_ss_left_r)
           );

      RAM128X1D #
        (
         .INIT (64'h0000000000000000)
         )
        u_po_right_ram
          (
           .DPO   (po_right_ram_out[i_po]),
           .SPO   (),
           .A     (byte_cntr),
           .D     (offset_cntr[i_po]),
           .DPRA  (win_byte_select),
           .WCLK  (clk),
           .WE    (po_ss_right_r)
           );
    end

  endgenerate



     //                          36           35                34               33:25      24:21    20      19              18             17          16          15:12                                  [11:8]       7           6         5:0
assign dbg_win_chk = {28'h0,po_win_tg_rst,win_clr_error,mem_pattern_init_done_edge,offset_cntr,byte_cntr,eod,po_ss_left_r, po_ss_right_r, delay_cntr_ce,  error_dqs,delay_cntr[DLY_WIDTH-1:DLY_WIDTH-4], win_state_r, pi_ss_left_r,pi_ss_right_r,po_rst_cntr[8:3] };
  //***************************************************************************
  // win_up_fps and win_down_fps
  //***************************************************************************

  always @(posedge clk)
    if (rst) 
      pi_win_up <= #TCQ 1'b0;
    else     
      pi_win_up <= #TCQ (inc_tap | (sc_middle & left_side_spread)) & win_sel_pi_pon_r;

  always @(posedge clk)
    if (rst) 
      pi_win_down <= #TCQ 1'b0;
    else     
      pi_win_down <= #TCQ (dec_tap | (sc_middle & right_side_spread)) & win_sel_pi_pon_r;


  //***************************************************************************
  // win_up_fps and win_down_fps
  //***************************************************************************

  always @(posedge clk)
    if (rst) 
      po_win_up <= #TCQ 1'b0;
    else     
      po_win_up <= #TCQ (inc_tap | (sc_middle & left_side_spread)) & !win_sel_pi_pon_r;

  always @(posedge clk)
    if (rst) 
      po_win_down <= #TCQ 1'b0;
    else     
      po_win_down <= #TCQ (dec_tap | (sc_middle & right_side_spread)) & !win_sel_pi_pon_r;

  always @(posedge clk)
    if (rst) 
      po_stg23_sel <= #TCQ 1'b0;
    else if (win_start)    
      po_stg23_sel <= #TCQ !win_sel_pi_pon;
    else if (win_state_r == WIN_IDLE)    
      po_stg23_sel <= #TCQ 1'b0;



  //***************************************************************************
  // Spread out inc and dec pulses
  //***************************************************************************

  assign right_side_spread = (win_state_r == WIN_RTN_TAP_R);
  assign left_side_spread  = (win_state_r == WIN_RTN_TAP_L);

  assign spread_done = (offset_cntr == 0) & (spread_cntr == 0);

  assign spread_state = right_side_spread | left_side_spread;

  assign sc_ce = (spread_cntr != 0) | (spread_state & (offset_cntr != 0));

  always @(posedge clk)
    if (rst)       
      spread_cntr <= #TCQ 'b0;
    else if (sc_ce) 
      spread_cntr <= #TCQ spread_cntr + 1;

  always @(posedge clk)
    if (rst) 
      spread_cntr_msb_r <= #TCQ 1'b0;
    else     
      spread_cntr_msb_r <= #TCQ spread_cntr[SC_WIDTH-1];

  assign sc_middle = spread_cntr[SC_WIDTH-1] & ~spread_cntr_msb_r;

  //***************************************************************************
  // Short delay
  //***************************************************************************

  assign short_dly_done = (short_dly_cntr == 0);

  always @(posedge clk)
    if (rst)                 
      short_dly_cntr <= #TCQ 'b0;
    else if (start_short_dly) 
      short_dly_cntr <= #TCQ {SDC_WIDTH{1'b1}};
    else if (!short_dly_done) 
      short_dly_cntr <= #TCQ short_dly_cntr - 1;

  //***************************************************************************
  // Comparison logic
  //***************************************************************************

  // Select only current DQS group to route to error detection logic
  generate
    if (nCK_PER_CLK == 2) begin: gen_div2_rddata_mux
      always @(posedge clk) begin
        rd_data_r[0+:DQ_PER_DQS] 
          <= #TCQ rd_data[DQ_PER_DQS*byte_cntr           +:DQ_PER_DQS];
        rd_data_r[DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ rd_data[DQ_PER_DQS*byte_cntr+  DQ_WIDTH+:DQ_PER_DQS];
        rd_data_r[2*DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ rd_data[DQ_PER_DQS*byte_cntr+2*DQ_WIDTH+:DQ_PER_DQS];
        rd_data_r[3*DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ rd_data[DQ_PER_DQS*byte_cntr+3*DQ_WIDTH+:DQ_PER_DQS];
        cmp_data_r[0+:DQ_PER_DQS] 
          <= #TCQ cmp_data[DQ_PER_DQS*byte_cntr           +:DQ_PER_DQS];
        cmp_data_r[DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ cmp_data[DQ_PER_DQS*byte_cntr+  DQ_WIDTH+:DQ_PER_DQS];
        cmp_data_r[2*DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ cmp_data[DQ_PER_DQS*byte_cntr+2*DQ_WIDTH+:DQ_PER_DQS];
        cmp_data_r[3*DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ cmp_data[DQ_PER_DQS*byte_cntr+3*DQ_WIDTH+:DQ_PER_DQS];      
      end
    end else if (nCK_PER_CLK == 4) begin: gen_div4_rddata_mux
      always @(posedge clk) begin
        rd_data_r[0+:DQ_PER_DQS] 
          <= #TCQ rd_data[DQ_PER_DQS*byte_cntr           +:DQ_PER_DQS];
        rd_data_r[DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ rd_data[DQ_PER_DQS*byte_cntr+  DQ_WIDTH+:DQ_PER_DQS];
        rd_data_r[2*DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ rd_data[DQ_PER_DQS*byte_cntr+2*DQ_WIDTH+:DQ_PER_DQS];
        rd_data_r[3*DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ rd_data[DQ_PER_DQS*byte_cntr+3*DQ_WIDTH+:DQ_PER_DQS];
        rd_data_r[4*DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ rd_data[DQ_PER_DQS*byte_cntr+4*DQ_WIDTH+:DQ_PER_DQS];
        rd_data_r[5*DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ rd_data[DQ_PER_DQS*byte_cntr+5*DQ_WIDTH+:DQ_PER_DQS];
        rd_data_r[6*DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ rd_data[DQ_PER_DQS*byte_cntr+6*DQ_WIDTH+:DQ_PER_DQS];
        rd_data_r[7*DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ rd_data[DQ_PER_DQS*byte_cntr+7*DQ_WIDTH+:DQ_PER_DQS];      
        cmp_data_r[0+:DQ_PER_DQS] 
          <= #TCQ cmp_data[DQ_PER_DQS*byte_cntr           +:DQ_PER_DQS];
        cmp_data_r[DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ cmp_data[DQ_PER_DQS*byte_cntr+  DQ_WIDTH+:DQ_PER_DQS];
        cmp_data_r[2*DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ cmp_data[DQ_PER_DQS*byte_cntr+2*DQ_WIDTH+:DQ_PER_DQS];
        cmp_data_r[3*DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ cmp_data[DQ_PER_DQS*byte_cntr+3*DQ_WIDTH+:DQ_PER_DQS];
        cmp_data_r[4*DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ cmp_data[DQ_PER_DQS*byte_cntr+4*DQ_WIDTH+:DQ_PER_DQS];
        cmp_data_r[5*DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ cmp_data[DQ_PER_DQS*byte_cntr+5*DQ_WIDTH+:DQ_PER_DQS];
        cmp_data_r[6*DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ cmp_data[DQ_PER_DQS*byte_cntr+6*DQ_WIDTH+:DQ_PER_DQS];
        cmp_data_r[7*DQ_PER_DQS+:DQ_PER_DQS]
          <= #TCQ cmp_data[DQ_PER_DQS*byte_cntr+7*DQ_WIDTH+:DQ_PER_DQS];      
      end
    end    
  endgenerate

        always @(posedge clk) begin
        if (win_clr_error)
          error_dqs <= #TCQ 1'b0;
        else if (read_valid_r)
          // Note that delayed version of read_valid is used here in
          // order to match corresponding pipe delay on rd/cmp_data
          error_dqs <= #TCQ (rd_data_r !== cmp_data_r) | error_dqs;
      end
/*
  generate
    genvar j;
    for (j = 0; j < 2*nCK_PER_CLK; j = j + 1) begin: gen_word_error
      always @(posedge clk) begin
        if (win_clr_error)
          word_error_r[j] <= #TCQ {DQ_PER_DQS{1'b0}};
        else if (read_valid_r)
          // Note that delayed version of read_valid is used here in
          // order to match corresponding pipe delay on rd/cmp_data
          word_error_r[j] <= #TCQ word_error_r[j] |
                             (rd_data_r[DQ_PER_DQS*j+:DQ_PER_DQS] !==
                              cmp_data_r[DQ_PER_DQS*j+:DQ_PER_DQS]);
      end
    end
  endgenerate

  assign lane_error_or[0] = {DQ_PER_DQS{1'b0}};
  
  generate
    genvar k;
    for (k = 0; k < 2*nCK_PER_CLK; k = k + 1) begin: gen_lane_error_or
      assign lane_error_or[k + 1] = word_error_r[k] | lane_error_or[k];
    end
  endgenerate

  always @(posedge clk)  
    lane_error_r <= #TCQ lane_error_or[2*nCK_PER_CLK];

  // Select error signal for current bit to route to error detection logic
  assign error = lane_error_r[byte_cntr*DQ_PER_DQS];    
  */
  //***************************************************************************
  // State register
  //***************************************************************************

  always @(posedge clk)
    if (rst) 
      win_state_r <= #TCQ 'b0;
    else     
      win_state_r <= #TCQ win_next_state;

  //***************************************************************************
  // Next pd state
  //***************************************************************************

  always @(*) begin
    win_next_state  = WIN_IDLE; // default state is idle
    set_win_active  = 1'b0;     // default zero all combinatoral signals
    start_short_dly = 1'b0;
    clr_win_active  = 1'b0;
    inc_tap         = 1'b0;
    dec_tap         = 1'b0;
    inc_offset_cntr = 1'b0;
    dec_offset_cntr = 1'b0;
    clr_delay_cntr  = 1'b0;
    ss_right        = 1'b0;
    ss_left         = 1'b0;
    inc_byte_cntr    = 1'b0;
    clr_error       = 1'b0;
    po_win_tg_rst_d = 1'b0;
    po_stg23_sel_d  = 1'b0;
    
    case (win_state_r)
      
      // Wait for win_start
      WIN_IDLE: begin 
        if (win_start) begin
          set_win_active  = 1'b1;
          start_short_dly = 1'b1;
          clr_error       = 1'b1;
          win_next_state  = WIN_SHORT_DLY;
          po_stg23_sel_d  = 1'b1;
        end else if (dbg_clear_error) begin
          clr_error       = 1'b1;
        end
      end
      
      // Give time for win_active to propagate, since win_active is used to
      // pass control of external control signals to this module
      WIN_SHORT_DLY: begin 
        if (short_dly_done) 
          win_next_state = WIN_INC_TAP;
        else
          win_next_state = WIN_SHORT_DLY;
      end
      
      // Increment tap count  
      WIN_INC_TAP: begin
        inc_tap         = 1'b1;
        inc_offset_cntr = 1'b1;
        clr_delay_cntr  = 1'b1;
        po_win_tg_rst_d = 1'b1;
        win_next_state  = WIN_WAIT_INC;
      end
      
      // Allow large # of reads to occur with the new tap delay value
      WIN_WAIT_INC: begin
        if (delay_cntr_done) 
          win_next_state = WIN_CHK_ERR_R;
        else                
          win_next_state = WIN_WAIT_INC;
      end
      
      // Now check to see if errors occured during WIN_WAIT_INC interval
      WIN_CHK_ERR_R: begin
        if (error_dqs | eod) 
          win_next_state = WIN_BACK_R;
        else            
          win_next_state = WIN_INC_TAP;
      end
      
      // Decrement delay tap and record tap count if we reached maximum
      // possible delay
      WIN_BACK_R: begin
        // if eod only, sample before dec_offset_cntr
        if (~error_dqs) 
          ss_right = 1'b1;      
        dec_tap         = 1'b1;
        dec_offset_cntr = 1'b1;
        win_next_state  = WIN_SS_R;
      end
        
      // Sample current delay count if we had previously recorded an error
      WIN_SS_R: begin
        // if error, sample after dec_offset_cntr
        if (error_dqs) 
          ss_right = 1'b1;  
        win_next_state = WIN_RTN_TAP_R;
      end

      // Return to center of window
      WIN_RTN_TAP_R: begin
        if (spread_done) begin
          clr_error      = 1'b1;
          win_next_state = WIN_DEC_TAP;
        end else begin
          win_next_state  = WIN_RTN_TAP_R;
        end
      end

      // Decrement tap count
      WIN_DEC_TAP: begin
        dec_tap         = 1'b1;
        inc_offset_cntr = 1'b1;
        clr_delay_cntr  = 1'b1;
        po_win_tg_rst_d = 1'b1;
        win_next_state  = WIN_WAIT_DEC;
      end

      // Allow large # of reads to occur with the new tap delay value 
      WIN_WAIT_DEC: begin
        if (delay_cntr_done) 
          win_next_state = WIN_CHK_ERR_L;
        else                
          win_next_state = WIN_WAIT_DEC;
      end

      // Now check to see if errors occured during WIN_WAIT_DEC interval
      WIN_CHK_ERR_L: begin
        if (error_dqs | eod) 
          win_next_state = WIN_BACK_L;
        else            
          win_next_state = WIN_DEC_TAP;
      end
      
      // Increment delay tap and record tap count if we reached minimum
      // possible delay        
      WIN_BACK_L: begin
        // if eod only, sample before dec_offset_cntr
        if (~error_dqs) 
          ss_left = 1'b1;  
        inc_tap         = 1'b1;
        dec_offset_cntr = 1'b1;
        win_next_state  = WIN_SS_L;
      end
      
      // Sample current delay count if we had previously recorded an error
      WIN_SS_L: begin
        // if error, sample after dec_offset_cntr
        if (error_dqs) 
          ss_left = 1'b1;   
        win_next_state = WIN_RTN_TAP_L;
      end

      // Return to center of window   
      WIN_RTN_TAP_L: begin
        if (spread_done) begin
          clr_error      = 1'b1;
          win_next_state = WIN_CK_DONE;
        end else begin
          win_next_state = WIN_RTN_TAP_L;
        end
      end

      // Current window check finished. Repeat again for next bit, or 
      // return to IDLE if all bits have been checked 
      WIN_CK_DONE: begin
        if (byte_cntr < (BYTE-1)) begin
          inc_byte_cntr   = 1'b1;
          start_short_dly = 1'b1;
          win_next_state  = WIN_SHORT_DLY;
        end else begin
          clr_win_active = 1'b1;
          win_next_state = WIN_IDLE;
//          $stop;
        end
      end
        
      default: begin
        win_next_state = WIN_IDLE;
      end

    endcase
  end

endmodule
