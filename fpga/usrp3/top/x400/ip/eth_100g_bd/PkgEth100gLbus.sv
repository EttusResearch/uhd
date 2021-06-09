//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: PkgEth100gLbus
//
// Description:
//
//   Package to define an Lbus record
//

//-----------------------------------------------------------------------------
// Lbus interface
//
//  This is the segmented local bus interface on the Xilinx CMAC IP
//  see Xilinx CMAC documentation for detail
//   https://www.xilinx.com/support/documentation/ip_documentation/cmac_usplus/v2_4/pg203-cmac-usplus.pdf
//-----------------------------------------------------------------------------

package PkgEth100gLbus;

  localparam DATA_WIDTH = 512;
  localparam NUM_SEG = 4;
  localparam SEG_DATA_WIDTH = DATA_WIDTH/NUM_SEG;

  typedef struct packed {
    logic [SEG_DATA_WIDTH-1:0]  data;
    logic [$clog2(SEG_DATA_WIDTH/8)-1:0]  mty;
    logic  sop;
    logic  eop;
    logic  err;
    logic  ena;
  } lbus_t;

endpackage : PkgEth100gLbus
