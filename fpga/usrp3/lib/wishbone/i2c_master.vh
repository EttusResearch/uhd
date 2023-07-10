//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Header File: i2c_master.vh
//
// Description: Additional parameter definitions used by modules that instance
// the i2c_master_top wb<->I2C core.


//===============================================================================
// WB Registers defined in i2c_master_top
//===============================================================================

  localparam WB_PRER_LO = 3'b000;
  localparam WB_PRER_HI = 3'b001;
  localparam WB_CTR     = 3'b010;
  localparam WB_RXR     = 3'b011;
  localparam WB_TXR     = 3'b011;
  localparam WB_CR      = 3'b100;
  localparam WB_SR      = 3'b100;

//===============================================================================
// Valid CR (Command Register) commands
//===============================================================================
  localparam CR_START_AND_WRITE    = 'h90;
  localparam CR_WRITE              = 'h10;
  localparam CR_WRITE_AND_STOP     = 'h50;
  localparam CR_READ_AND_ACK       = 'h20;
  localparam CR_READ_AND_NACK      = 'h28;

//===============================================================================
// WB Core Enable command
//===============================================================================
  localparam WB_CORE_EN    = 'h80;
