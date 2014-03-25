//----------------------------------------------------------------------------
// Title      : Verilog component declaration for block level 10GBASE-R core
// Project    : 10 Gigabit Ethernet PCS PMA Core
// File       : ten_gig_eth_pcs_pma_mod.v
// Author     : Xilinx Inc.
// Description: This module holds the top level component declaration for the
//              10Gb/E PCS/PMA core.
//---------------------------------------------------------------------------
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

module ten_gig_eth_pcs_pma
(
   input           reset,
   input           txreset322,
   input           rxreset322,
   input           dclk_reset,
   output          pma_resetout,
   output          pcs_resetout,
   input           clk156,
   input           txusrclk2,
   input           rxusrclk2,
   input           dclk,
   
   input  [63 : 0] xgmii_txd,
   input  [7 : 0]  xgmii_txc,
   output [63 : 0] xgmii_rxd,
   output [7 : 0]  xgmii_rxc,

   input           mdc,
   input           mdio_in,
   output          mdio_out,
   output          mdio_tri,
   input  [4 : 0]  prtad,
   output [7 : 0]  core_status,
   input  [2 : 0]  pma_pmd_type,
   output          drp_req,
   input           drp_gnt,
   output          drp_den,                                  
   output          drp_dwe,                                  
   output [15:0]   drp_daddr,                  
   input           drp_drdy,                
   input  [15:0]   drp_drpdo,               
   output [15:0]   drp_di,

   output [31 : 0] gt_txd,
   output [7 : 0]  gt_txc,
   input  [31 : 0] gt_rxd,
   input  [7 : 0]  gt_rxc,
   output          gt_slip,
   
   input           resetdone,
   output          tx_prbs31_en,
   output          rx_prbs31_en,
   output          clear_rx_prbs_err_count,
   output [2 : 0]  loopback_ctrl,

   input           signal_detect,
   input           tx_fault,
   output          tx_disable);
 
endmodule
