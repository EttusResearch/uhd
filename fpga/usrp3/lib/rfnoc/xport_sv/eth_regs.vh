//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: eth_constants (Header File)
//
// Description:
//   Holds register offsets for the Ethernet Interface
//   NOTE: These depend on the following parameters
//  Parameters:
//     REG_AWIDTH - How wide the register window is in bits
//     BASE       - Base address added to the offsets here
// REGISTER OFFSETS

// Allocate one full page for MAC starting ox 0x0000
// This is the location of MAC address in XGE.
// It is shadowed in the ethifc.
localparam [REG_AWIDTH-1:0] REG_MAC_LSB        = BASE + 'h0000;
localparam [REG_AWIDTH-1:0] REG_MAC_MSB        = BASE + 'h0004;

// UIO Registers
// Source IP address
localparam [REG_AWIDTH-1:0] REG_IP             = BASE + 'h1000;
// Source UDP Port
localparam [REG_AWIDTH-1:0] REG_UDP            = BASE + 'h1004;

// Registers for Internal/Bridge Network Mode in CPU
localparam [REG_AWIDTH-1:0] REG_BRIDGE_MAC_LSB = BASE + 'h1010;
localparam [REG_AWIDTH-1:0] REG_BRIDGE_MAC_MSB = BASE + 'h1014;
localparam [REG_AWIDTH-1:0] REG_BRIDGE_IP      = BASE + 'h1018;
localparam [REG_AWIDTH-1:0] REG_BRIDGE_UDP     = BASE + 'h101c;
localparam [REG_AWIDTH-1:0] REG_BRIDGE_ENABLE  = BASE + 'h1020;

localparam [REG_AWIDTH-1:0] REG_CHDR_DROPPED   = BASE + 'h1030;
localparam [REG_AWIDTH-1:0] REG_CPU_DROPPED    = BASE + 'h1034;
localparam [REG_AWIDTH-1:0] REG_PAUSE          = BASE + 'h1038;
