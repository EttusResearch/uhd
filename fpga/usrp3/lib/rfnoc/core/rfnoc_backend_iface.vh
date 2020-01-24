//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

// Each block has a backed interface that is 512 bits wide. This bus
// is split into 16 32-bit registers to it is preferable to have fields
// aligned at 32-bit boundaries

// Backend Config
localparam BEC_FLUSH_TIMEOUT_OFFSET = 0;
localparam BEC_FLUSH_TIMEOUT_WIDTH  = 32;
localparam BEC_FLUSH_EN_OFFSET      = BEC_FLUSH_TIMEOUT_OFFSET + BEC_FLUSH_TIMEOUT_WIDTH;
localparam BEC_FLUSH_EN_WIDTH       = 1;
localparam BEC_SOFT_CTRL_RST_OFFSET = BEC_FLUSH_EN_OFFSET + BEC_FLUSH_EN_WIDTH;
localparam BEC_SOFT_CTRL_RST_WIDTH  = 1;
localparam BEC_SOFT_CHDR_RST_OFFSET = BEC_SOFT_CTRL_RST_OFFSET + BEC_SOFT_CTRL_RST_WIDTH;
localparam BEC_SOFT_CHDR_RST_WIDTH  = 1;
localparam BEC_TOTAL_WIDTH          = BEC_SOFT_CHDR_RST_OFFSET + BEC_SOFT_CHDR_RST_WIDTH;

localparam [511:0] BEC_DEFAULT_VAL = {
  {(512-BEC_TOTAL_WIDTH){1'b0}},
  1'b1,     // BEC_SOFT_CHDR_RST
  1'b1,     // BEC_SOFT_CTRL_RST
  1'b0,     // BEC_FLUSH_EN
  32'd0     // BEC_FLUSH_TIMEOUT
};

// Backend Status
localparam BES_PROTO_VER_OFFSET           = 0;
localparam BES_PROTO_VER_WIDTH            = 6;
localparam BES_NUM_DATA_I_OFFSET          = BES_PROTO_VER_OFFSET + BES_PROTO_VER_WIDTH;
localparam BES_NUM_DATA_I_WIDTH           = 6;
localparam BES_NUM_DATA_O_OFFSET          = BES_NUM_DATA_I_OFFSET + BES_NUM_DATA_I_WIDTH;
localparam BES_NUM_DATA_O_WIDTH           = 6;
localparam BES_CTRL_FIFOSIZE_OFFSET       = BES_NUM_DATA_O_OFFSET + BES_NUM_DATA_O_WIDTH;
localparam BES_CTRL_FIFOSIZE_WIDTH        = 6;
localparam BES_CTRL_MAX_ASYNC_MSGS_OFFSET = BES_CTRL_FIFOSIZE_OFFSET + BES_CTRL_FIFOSIZE_WIDTH;
localparam BES_CTRL_MAX_ASYNC_MSGS_WIDTH  = 8;
localparam BES_NOC_ID_OFFSET              = BES_CTRL_MAX_ASYNC_MSGS_OFFSET + BES_CTRL_MAX_ASYNC_MSGS_WIDTH;
localparam BES_NOC_ID_WIDTH               = 32;
localparam BES_FLUSH_ACTIVE_OFFSET        = BES_NOC_ID_OFFSET + BES_NOC_ID_WIDTH;
localparam BES_FLUSH_ACTIVE_WIDTH         = 1;
localparam BES_FLUSH_DONE_OFFSET          = BES_FLUSH_ACTIVE_OFFSET + BES_FLUSH_ACTIVE_WIDTH;
localparam BES_FLUSH_DONE_WIDTH           = 1;
localparam BES_DATA_MTU_OFFSET            = BES_FLUSH_DONE_OFFSET + BES_FLUSH_DONE_WIDTH;
localparam BES_DATA_MTU_WIDTH             = 6;
localparam BES_TOTAL_WIDTH                = BES_DATA_MTU_OFFSET + BES_DATA_MTU_WIDTH;

// Protocol version for this definition
localparam [5:0] BACKEND_PROTO_VER  = 6'd1;