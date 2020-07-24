//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  rfnoc_block_switchboard_regs (Header)
//
// Description:  Register definitions for the switchboard RFNoC block.
//

//-----------------------------------------------------------------------------
// Register Space
//-----------------------------------------------------------------------------

// The amount of address space taken up by each switchboard port.
// That is, the address space for output port N starts at N*(2^SWITCH_ADDR_W).
localparam SWITCH_ADDR_W = 'h3;

// REG_DEMUX_SELECT (R/W)
//
// Contains the zero-index of which output port each demux is connected to.
//
localparam REG_DEMUX_SELECT = 'h0;

// REG_MUX_SELECT (R/W)
//
// Contains the zero-index of which input port each mux is connected to.
//
localparam REG_MUX_SELECT = 'h4;
