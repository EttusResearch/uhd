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
//  /   /         Filename: iodelay_ctrl.v
// /___/   /\     Date Last Modified: $Date: 2011/06/02 08:34:56 $
// \   \  /  \    Date Created: Wed Aug 16 2006
//  \___\/\___\
//
//Device: Virtex-6
//Design Name: DDR3 SDRAM
//Purpose:
//   This module instantiates the IDELAYCTRL primitive, which continously
//   calibrates the IODELAY elements in the region to account for varying
//   environmental conditions. A 200MHz or 300MHz reference clock (depending
//   on the desired IODELAY tap resolution) must be supplied
//Reference:
//Revision History:
//*****************************************************************************

/******************************************************************************
**$Id: iodelay_ctrl.v,v 1.1 2011/06/02 08:34:56 mishra Exp $
**$Date: 2011/06/02 08:34:56 $
**$Author: mishra $
**$Revision: 1.1 $
**$Source: /devl/xcs/repo/env/Databases/ip/src2/O/mig_7series_v1_3/data/dlib/7series/ddr3_sdram/verilog/rtl/clocking/iodelay_ctrl.v,v $
******************************************************************************/

`timescale 1ps/1ps

module mig_7series_v1_8_iodelay_ctrl #
  (
   parameter TCQ              = 100,
                                // clk->out delay (sim only)
   parameter IODELAY_GRP      = "IODELAY_MIG",
                                // May be assigned unique name when
                                // multiple IP cores used in design
   parameter REFCLK_TYPE      = "DIFFERENTIAL",
                                // input clock type
                                // "DIFFERENTIAL","SINGLE_ENDED"
                                // NO_BUFFER, USE_SYSTEM_CLOCK
   parameter SYSCLK_TYPE      = "DIFFERENTIAL",
                                // input clock type
                                // DIFFERENTIAL, SINGLE_ENDED,
                                // NO_BUFFER
   parameter RST_ACT_LOW      = 1,
                                // Reset input polarity
                                // (0 = active high, 1 = active low)
   parameter DIFF_TERM_REFCLK = "TRUE"
                               // Differential Termination
   )
  (
   input  clk_ref_p,
   input  clk_ref_n,
   input  clk_ref_i,
   input  sys_rst,
   input  pll_locked,  // IJB. Added as per Xilinx code comments.
   output clk_ref,
   output iodelay_ctrl_rdy
   );

  // # of clock cycles to delay deassertion of reset. Needs to be a fairly
  // high number not so much for metastability protection, but to give time
  // for reset (i.e. stable clock cycles) to propagate through all state
  // machines and to all control signals (i.e. not all control signals have
  // resets, instead they rely on base state logic being reset, and the effect
  // of that reset propagating through the logic). Need this because we may not
  // be getting stable clock cycles while reset asserted (i.e. since reset
  // depends on DCM lock status)
  // COMMENTED, RC, 01/13/09 - causes pack error in MAP w/ larger #
  localparam RST_SYNC_NUM = 15;
  //  localparam RST_SYNC_NUM = 25;

  wire                   clk_ref_bufg;
  wire                   clk_ref_ibufg;
  wire                   rst_ref;
  (* keep = "true", max_fanout = 10 *) reg [RST_SYNC_NUM-1:0] rst_ref_sync_r /* synthesis syn_maxfan = 10 */;
  wire                   rst_tmp_idelay;
  wire                   sys_rst_act_hi;

  //***************************************************************************

  // Possible inversion of system reset as appropriate
  assign  sys_rst_act_hi = RST_ACT_LOW ? ~sys_rst: sys_rst;

  //***************************************************************************
  // 1) Input buffer for IDELAYCTRL reference clock - handle either a
  //    differential or single-ended input. Global clock buffer is used to
  //    drive the rest of FPGA logic.
  // 2) For NO_BUFFER option, Reference clock will be driven from internal
  //    clock i.e., clock is driven from fabric. Input buffers and Global
  //    clock buffers will not be instaitaed.
  // 3) For USE_SYSTEM_CLOCK, input buffer output of system clock will be used
  //    as the input reference clock. Global clock buffer is used to drive
  //    the rest of FPGA logic.
  //***************************************************************************

  generate
    if (REFCLK_TYPE == "DIFFERENTIAL") begin: diff_clk_ref
      IBUFGDS #
        (
         .DIFF_TERM    (DIFF_TERM_REFCLK),
         .IBUF_LOW_PWR ("FALSE")
         )
        u_ibufg_clk_ref
          (
           .I  (clk_ref_p),
           .IB (clk_ref_n),
           .O  (clk_ref_ibufg)
           );

      BUFG u_bufg_clk_ref
        (
         .O (clk_ref_bufg),
         .I (clk_ref_ibufg)
         );
    end else if (REFCLK_TYPE == "SINGLE_ENDED") begin : se_clk_ref
      IBUFG #
        (
         .IBUF_LOW_PWR ("FALSE")
         )
        u_ibufg_clk_ref
          (
           .I (clk_ref_i),
           .O (clk_ref_ibufg)
           );

      BUFG u_bufg_clk_ref
        (
         .O (clk_ref_bufg),
         .I (clk_ref_ibufg)
         );
    end else if ((REFCLK_TYPE == "NO_BUFFER") ||
                 (REFCLK_TYPE == "USE_SYSTEM_CLOCK" && SYSCLK_TYPE == "NO_BUFFER")) begin : clk_ref_noibuf_nobuf
//      assign clk_ref_bufg = clk_ref_i; //IJB
       BUFG u_bufg_clk_ref
        (
         .O (clk_ref_bufg),
         .I (clk_ref_i)
         );
    end else if (REFCLK_TYPE == "USE_SYSTEM_CLOCK" && SYSCLK_TYPE != "NO_BUFFER") begin : clk_ref_noibuf
      BUFG u_bufg_clk_ref
        (
         .O (clk_ref_bufg),
         .I (clk_ref_i)
         );
    end
  endgenerate

  //***************************************************************************
  // Global clock buffer for IDELAY reference clock
  //***************************************************************************


  assign clk_ref = clk_ref_bufg;

  //*****************************************************************
  // IDELAYCTRL reset
  // This assumes an external clock signal driving the IDELAYCTRL
  // blocks. Otherwise, if a PLL drives IDELAYCTRL, then the PLL
  // lock signal will need to be incorporated in this.
  //*****************************************************************

  // Add PLL lock if PLL drives IDELAYCTRL in user design
  assign rst_tmp_idelay = sys_rst_act_hi|| ~pll_locked; //IJB

  always @(posedge clk_ref_bufg or posedge rst_tmp_idelay)
    if (rst_tmp_idelay)
      rst_ref_sync_r <= #TCQ {RST_SYNC_NUM{1'b1}};
    else
      rst_ref_sync_r <= #TCQ rst_ref_sync_r << 1;

  assign rst_ref  = rst_ref_sync_r[RST_SYNC_NUM-1];

  //*****************************************************************

  (* IODELAY_GROUP = IODELAY_GRP *) IDELAYCTRL u_idelayctrl
    (
     .RDY    (iodelay_ctrl_rdy),
     .REFCLK (clk_ref_bufg),
     .RST    (rst_ref)
     );

endmodule
