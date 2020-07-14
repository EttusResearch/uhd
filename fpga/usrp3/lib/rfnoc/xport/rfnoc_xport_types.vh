//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

// Add all new transport types here
localparam [7:0] NODE_SUBTYPE_XPORT_GENERIC        = 8'd0;
localparam [7:0] NODE_SUBTYPE_XPORT_IPV4_CHDR64    = 8'd1;
// localparam [7:0] NODE_SUBTYPE_XPORT_LIBERIO_CHDR64 = 8'd2;
// Subtype 2 used to be Liberio, is now reserved to avoid conflict.
localparam [7:0] NODE_SUBTYPE_XPORT_NIRIO_CHDR64   = 8'd3;
