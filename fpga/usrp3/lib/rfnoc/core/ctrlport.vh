//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport.vh
// Description:
// Defines constants for the control port interface.
//
// Requires rfnoc_axis_ctrl_utils.vh in same directory to be
// included first.

//---------------------------------------------------------------
// Signal widths
//---------------------------------------------------------------
localparam CTRLPORT_ADDR_W = 20;
localparam CTRLPORT_DATA_W = 32;
localparam CTRLPORT_STS_W  =  2;

//---------------------------------------------------------------
// Status values
//---------------------------------------------------------------
localparam [1:0] CTRL_STS_OKAY    = 2'b00;
localparam [1:0] CTRL_STS_CMDERR  = 2'b01;
localparam [1:0] CTRL_STS_TSERR   = 2'b10;
localparam [1:0] CTRL_STS_WARNING = 2'b11;
