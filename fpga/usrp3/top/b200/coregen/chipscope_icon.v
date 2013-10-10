///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 2013 Xilinx, Inc.
// All Rights Reserved
///////////////////////////////////////////////////////////////////////////////
//   ____  ____
//  /   /\/   /
// /___/  \  /    Vendor     : Xilinx
// \   \   \/     Version    : 14.4
//  \   \         Application: Xilinx CORE Generator
//  /   /         Filename   : chipscope_icon.v
// /___/   /\     Timestamp  : Fri Mar 08 11:51:37 PST 2013
// \   \  /  \
//  \___\/\___\
//
// Design Name: Verilog Synthesis Wrapper
///////////////////////////////////////////////////////////////////////////////
// This wrapper is used to integrate with Project Navigator and PlanAhead

`timescale 1ns/1ps

module chipscope_icon(
    CONTROL0,
    CONTROL1) /* synthesis syn_black_box syn_noprune=1 */;


inout [35 : 0] CONTROL0;
inout [35 : 0] CONTROL1;

endmodule
