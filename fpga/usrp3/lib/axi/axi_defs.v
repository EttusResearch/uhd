//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
//
// AXI4 Burst enumeration
//
`define AXI4_BURST_FIXED 2'b00
`define AXI4_BURST_INCR 2'b01
`define AXI4_BURST_WRAP 2'b10
`define AXI4_BURST_RSVD 2'b11
//
// AXI4 response code enumeration
//
`define AXI4_RESP_OKAY 2'b00
`define AXI4_RESP_EXOKAY 2'b01
`define AXI4_RESP_SLVERR 2'b10
`define AXI4_RESP_DECERR 2'b11
//
// AXI4 lock enumeration
//
`define AXI4_LOCK_NORMAL 1'b0
`define AXI4_LOCK_EXCLUSIVE 1'b1
//
// AXI4 memory attrubutes
//
`define AXI4_CACHE_ALLOCATE 4'h8
`define AXI4_CACHE_OTHER_ALLOCATE 4'h4
`define AXI4_CACHE_MODIFIABLE 4'h2
`define AXI4_CACHE_BUFFERABLE 4'h1
//
// AXI4 PROT attributes
//
`define AXI4_PROT_PRIVILEDGED 3'h1
`define AXI4_PROT_NON_SECURE 3'h2
`define AXI4_PROT_INSTRUCTION 3'h4


