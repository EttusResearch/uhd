//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Package: PkgAxiLite
// Description:
//   AXI4-Lite is an ARM standard for lighter weight registers
//   axis based on the AXI4 protocol. For more information
//   on the spec see - https://developer.arm.com/docs/ihi0022/d
//
//   This package contains types used for AxiLiteIf.
//

//-----------------------------------------------------------------------------
// AXI4-Lite Package
//-----------------------------------------------------------------------------

package PkgAxiLite;
  typedef enum logic [1:0] {OKAY=0,SLVERR=2,DECERR=3} resp_t;
endpackage : PkgAxiLite
