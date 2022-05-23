//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: eth_regs.vh (Header File)
//
// Description:
//
//   Defines the register offsets for the Ethernet interface. These depend on
//   the parameters below.
//
// Parameters:
//
//   REG_AWIDTH - Width of the address bus used to address these registers.
//   BASE       - Base address to be added to the offsets here.
//


//-----------------------------------------------------------------------------
// XGE/CGE Mac Registers
//-----------------------------------------------------------------------------

// Allocate one full page (0x1000 bytes) for MAC starting at the base address.
// This is the location of MAC address in XGE.
localparam [REG_AWIDTH-1:0] REG_MAC_LSB        = BASE + 'h0000;
localparam [REG_AWIDTH-1:0] REG_MAC_MSB        = BASE + 'h0004;


//-----------------------------------------------------------------------------
// UIO Registers
//-----------------------------------------------------------------------------

// Device's IP address and UDP port
localparam [REG_AWIDTH-1:0] REG_IP             = BASE + 'h1000;
localparam [REG_AWIDTH-1:0] REG_UDP            = BASE + 'h1004;

// Registers for the Ethernet bridge between CPU and FPGA
localparam [REG_AWIDTH-1:0] REG_BRIDGE_MAC_LSB = BASE + 'h1010;
localparam [REG_AWIDTH-1:0] REG_BRIDGE_MAC_MSB = BASE + 'h1014;
localparam [REG_AWIDTH-1:0] REG_BRIDGE_IP      = BASE + 'h1018;
localparam [REG_AWIDTH-1:0] REG_BRIDGE_UDP     = BASE + 'h101c;
localparam [REG_AWIDTH-1:0] REG_BRIDGE_ENABLE  = BASE + 'h1020;

// Dropped packet debug registers
localparam [REG_AWIDTH-1:0] REG_CHDR_DROPPED   = BASE + 'h1030;
localparam [REG_AWIDTH-1:0] REG_CPU_DROPPED    = BASE + 'h1034;

// Pause frame control
localparam [REG_AWIDTH-1:0] REG_PAUSE          = BASE + 'h1038;

//-------------------------------------
// Transport Adapter Registers
//-------------------------------------

// Compatibility Number (Read-Only)
// COMPAT[31:16] = <Reserved>
// COMPAT[15: 8] = Major
// COMPAT[ 7: 0] = Minor
localparam [REG_AWIDTH-1:0] REG_XPORT_COMPAT    = BASE + 'h1100;

// Capabilities (Read-Only)
// INFO[31:4] = <Reserved>
// INFO[3]    = <Reserved> TX CHDR header insertion
// INFO[2]    = <Reserved> TX KV-map
// INFO[1]    = RX CHDR header removal
// INFO[0]    = RX KV-map
localparam [REG_AWIDTH-1:0] REG_XPORT_INFO      = BASE + 'h1104;

// Node Instance (Read-Only)
// NODE_ISNT[31:0] = Node instance number for this transport adapter
localparam [REG_AWIDTH-1:0] REG_XPORT_NODE_INST = BASE + 'h1108;

// Key-Value Map Configuration
//
// You should poll KV_CFG and wait until the BUSY bit is 0 before updating
// these registers. A write to KV_CFG causes the key-value map entry described
// by these registers to be committed to the KV map. Therefore, the write to
// KV_CFG should be the last write when adding an entry.
//
// KV_MAC_LO[31:0] = MAC[31:0] (Write-Only)
localparam [REG_AWIDTH-1:0] REG_XPORT_KV_MAC_LO = BASE + 'h110C;
// KV_MAC_HI[31:0] = MAC[47:31] (Write-Only)
localparam [REG_AWIDTH-1:0] REG_XPORT_KV_MAC_HI = BASE + 'h1110;
// KV_IP[31:0] = IP Address (Write-Only)
localparam [REG_AWIDTH-1:0] REG_XPORT_KV_IP     = BASE + 'h1114;
// KV_UDP[31:0] = <Reserved>
// KV_UDP[15:0] = UDP Address (Write-Only)
localparam [REG_AWIDTH-1:0] REG_XPORT_KV_UDP    = BASE + 'h1118;
// KV_CFG[31]    = KV map is busy; do not write to KV registers. (Read-Only)
// KV_CFG[30:17] = <Reserved>
// KV_CFG[16]    = Mode (0 = CHDR, 1 = Raw Payload) (Write-only)
// KV_CFG[15:0]  = CHDR Endpoint ID (EPID) (Write-Only)
localparam [REG_AWIDTH-1:0] REG_XPORT_KV_CFG    = BASE + 'h111C;
