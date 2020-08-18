//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: ctrlport.vh
//
// Description:
//
//   Defines constants for the control port interface. See also
//   rfnoc_axis_ctrl_utils.vh for related AXIS-Ctrl definitions.
//

//---------------------------------------------------------------
// Signal widths
//---------------------------------------------------------------
localparam CTRLPORT_ADDR_W     = 20;
localparam CTRLPORT_DATA_W     = 32;
localparam CTRLPORT_STS_W      = 2;
localparam CTRLPORT_PORTID_W   = 10;
localparam CTRLPORT_REM_EPID_W = 16;
localparam CTRLPORT_BYTE_EN_W  =  4;
localparam CTRLPORT_TIME_W     = 64;

//---------------------------------------------------------------
// Status values
//---------------------------------------------------------------
localparam [1:0] CTRL_STS_OKAY    = 2'b00;
localparam [1:0] CTRL_STS_CMDERR  = 2'b01;
localparam [1:0] CTRL_STS_TSERR   = 2'b10;
localparam [1:0] CTRL_STS_WARNING = 2'b11;
