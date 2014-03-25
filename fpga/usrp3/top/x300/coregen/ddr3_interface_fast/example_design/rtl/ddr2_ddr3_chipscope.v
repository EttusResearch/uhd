//*****************************************************************************
// (c) Copyright 2008 - 2010 Xilinx, Inc. All rights reserved.
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
// /___/  \  /    Vendor                : Xilinx
// \   \   \/     Version               : %version
//  \   \         Application           : MIG
//  /   /         Filename              : chipscope.v
// /___/   /\     Date Last Modified    : $date$
// \   \  /  \    Date Created          : Aug 07 2009
//  \___\/\___\
//
//Device            : 7-Series
//Design Name       : DDR2/3 SDRAM
//Purpose           : Chipscope cores declarations used if debug option is
//                    enabled in MIG when generating design. These are
//                    empty declarations to allow compilation to pass both in
//                    simulation and synthesis. The proper .ngc files must be
//                    referenced during the actual ISE build. 
//Reference         :
//Revision History  :
//*****************************************************************************

`timescale 1 ps / 1 ps

module ddr_icon
  (
   inout [35:0] CONTROL0,
   inout [35:0] CONTROL1,
   inout [35:0] CONTROL2,   
   inout [35:0] CONTROL3,
   inout [35:0] CONTROL4
   ) /* synthesis syn_black_box syn_noprune = 1 */;
endmodule // icon

module ddr_ila_basic
         (
          input  CLK,
          input [255:0] DATA,
          input [31:0] TRIG0,
          inout [35:0] CONTROL 
          )/* synthesis syn_black_box syn_noprune = 1 */;
endmodule
	  
module ddr_ila_wrpath
         (
          input CLK,
          input [255:0] DATA,
          input [31:0] TRIG0,
          inout [35:0] CONTROL 
          )/* synthesis syn_black_box syn_noprune = 1 */;
endmodule
	  
module ddr_ila_rdpath
         (
         input CLK,
         input [1023:0]DATA,
         input [63:0]TRIG0,
         inout [35:0] CONTROL 
          )/* synthesis syn_black_box syn_noprune = 1 */;
endmodule

module ddr_vio_sync_async_out72
  (
   output [71:0] SYNC_OUT,
   output [71:0] ASYNC_OUT,
   input         CLK,
   inout [35:0]  CONTROL
   ) /* synthesis syn_black_box syn_noprune = 1 */;
endmodule

module ddr_vio_async_in_sync_out
  (
   input  [127:0] ASYNC_IN,
   output [127:0] SYNC_OUT,
   input          CLK,
   inout [35:0]   CONTROL
   ) /* synthesis syn_black_box syn_noprune = 1 */;
endmodule
