//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Package: PkgAxi
// Description:
//   AXI4 is an ARM standard for memory mapped burst transfers
//   For more information on the spec see
//     - https://developer.arm.com/docs/ihi0022/d
//
//   This package contains types used for AxiIf.
//

//-----------------------------------------------------------------------------
// AXI4 Package
//-----------------------------------------------------------------------------

package PkgAxi;
  typedef enum logic [1:0] {OKAY=0,SLVERR=2,DECERR=3} axi_resp_t;
  // Len - Burst Length = LEN+1
  // Size - Bytes in transfer = 2**SIZE
  typedef enum logic [1:0] {FIXED=0,INCR=1,WRAP=2} axi_burst_t;
  // Cache - just see the docs. (too tangled!)
  // Prot[0] - Privleged
  // Prot[1] - Secure
  // Prot[2] - 0=Data / 1=Instruction
  // QOS - 0 = no protocol - Others not specified in spec

endpackage : PkgAxi
