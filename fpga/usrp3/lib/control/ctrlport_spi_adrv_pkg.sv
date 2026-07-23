//
// Copyright 2026 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Package: ctrlport_spi_adrv_pkg
//
// Description:
//
//   Shared constants and types for ctrlport_spi_adrv and its testbench.
//
//

package ctrlport_spi_adrv_pkg;

  //---------------------------------------------------------------------------
  // Register offsets (byte addresses relative to BASE_ADDRESS)
  //---------------------------------------------------------------------------

  localparam int REG_HALF_PER  = 'h00;
  localparam int REG_SI        = 'h04;
  localparam int REG_CONTROL   = 'h3C;
  localparam int REG_DATA      = 'h40;

  // Actual address space required for this block.
  localparam int REG_ADDR_W = 7;


  //---------------------------------------------------------------------------
  // CONTROL register field widths
  //---------------------------------------------------------------------------

  localparam int SPI_MODE_W      = 2;
  localparam int SPI_NUM_BYTES_W = 12;
  localparam int SPI_ADDR_W      = 16;


  //---------------------------------------------------------------------------
  // SPI direction encoding
  //---------------------------------------------------------------------------

  typedef enum logic {
    SPI_WRITE = 1'b0,
    SPI_READ  = 1'b1
  } spi_adrv_dir_t;


  //---------------------------------------------------------------------------
  // SPI mode encoding
  //---------------------------------------------------------------------------

  typedef enum logic [SPI_MODE_W-1:0] {
    MODE_RAW       = 0,  // Raw byte stream
    MODE_SI_SEQ    = 1,  // Single Instruction, sequential addresses
    MODE_SI_REP    = 2,  // Single Instruction, same address repeated
    MODE_STREAMING = 3   // Send address once, followed by all data bytes back-to-back
  } spi_adrv_mode_t;


  //---------------------------------------------------------------------------
  // CONTROL register
  //---------------------------------------------------------------------------

  typedef struct packed {
    spi_adrv_dir_t              dir;        // [31]
    logic                       rsvd1;      // [30]
    spi_adrv_mode_t             mode;       // [29:28]
    logic [SPI_NUM_BYTES_W-1:0] num_bytes;  // [27:16]
    logic [     SPI_ADDR_W-1:0] addr;       // [15:0]
  } spi_adrv_ctrl_t;

endpackage : ctrlport_spi_adrv_pkg
