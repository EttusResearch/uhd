//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_radio_regs (Header)
//
// Description: Header file for RFNoC radio functionality. This includes 
// register offsets, bitfields and constants for the radio components.
//


//-----------------------------------------------------------------------------
// Shared Register Offsets (One Set Per Radio NoC Block)
//-----------------------------------------------------------------------------

localparam SHARED_BASE_ADDR = 20'h00;  // Base address for shared radio registers
localparam SHARED_ADDR_W    = 4;       // Address space size for shared registers

localparam REG_COMPAT_NUM = 'h00;  // Compatibility number register offset


//-----------------------------------------------------------------------------
// Radio Core Register Offsets (One Set Per Radio Port)
//-----------------------------------------------------------------------------
//
// These registers are replicated depending on the number of radio channels 
// requested. They start at BASE_ADDR_RADIO and repeat every RADIO_ADDR_SPACE 
// bytes.
//
// WARNING: All registers larger than a single 32-bit word must be read and 
//          written least significant word first to guarantee coherency.
//
//-----------------------------------------------------------------------------

localparam RADIO_BASE_ADDR = 20'h1000; // Base address of first radio. Choose a 
                                       // nice big power of 2 so we can just pass 
                                       // the lower bits to the radio cores.
localparam RADIO_ADDR_W    = 7;        // Address space size per radio

// General Radio Registers
localparam REG_LOOPBACK_EN   = 'h00;   // Loopback enable (connect Tx output to Rx input)
localparam REG_RADIO_WIDTH   = 'h04;   // Upper 16 bits is sample width, lower 16 bits is NSPC

// RX Control Registers
localparam REG_RX_STATUS            = 'h10; // Status of Rx radio
localparam REG_RX_CMD               = 'h14; // The next radio command to execute
localparam REG_RX_CMD_NUM_WORDS_LO  = 'h18; // Number of radio words for the next command (low word)
localparam REG_RX_CMD_NUM_WORDS_HI  = 'h1C; // Number of radio words for the next command (high word)
localparam REG_RX_CMD_TIME_LO       = 'h20; // Time for the next command (low word)
localparam REG_RX_CMD_TIME_HI       = 'h24; // Time for the next command (high word)
localparam REG_RX_MAX_WORDS_PER_PKT = 'h28; // Maximum packet length to build from Rx data
localparam REG_RX_ERR_PORT          = 'h2C; // Port ID for error reporting
localparam REG_RX_ERR_REM_PORT      = 'h30; // Remote port ID for error reporting
localparam REG_RX_ERR_REM_EPID      = 'h34; // Remote EPID (endpoint ID) for error reporting
localparam REG_RX_ERR_ADDR          = 'h38; // Offset to write error code to
localparam REG_RX_DATA              = 'h3C; // Read the current Rx output of the radio
localparam REG_RX_HAS_TIME          = 'h70; // Controls whether or not a channel has timestamps

// TX Control Registers
localparam REG_TX_IDLE_VALUE   = 'h40; // Value to output when transmitter is idle
localparam REG_TX_ERROR_POLICY = 'h44; // Tx error policy
localparam REG_TX_ERR_PORT     = 'h48; // Port ID for error reporting
localparam REG_TX_ERR_REM_PORT = 'h4C; // Remote port ID for error reporting
localparam REG_TX_ERR_REM_EPID = 'h50; // Remote EPID (endpoint ID) for error reporting
localparam REG_TX_ERR_ADDR     = 'h54; // Offset to write error code to


//-----------------------------------------------------------------------------
// Register Bit Fields
//-----------------------------------------------------------------------------

// REG_RX_CMD bit fields
localparam RX_CMD_POS       = 0;  // Location of the command bit field
localparam RX_CMD_LEN       = 2;  // Bit length of the command bit field
localparam RX_CMD_TIMED_POS = 31; // Location of the bit indicating if this is
                                  // a timed command or not.

// REG_RX_CMD_NUM_WORDS_HI/LO length field
localparam RX_CMD_NUM_WORDS_LEN = 48; // Number of bits that are used in the 64-bit
                                      // NUM_WORDS register (must be in range [33:64]).

// REG_RX_STATUS bit fields
localparam CMD_FIFO_SPACE_POS = 0;  // Indicates if radio is busy executing a command.
localparam CMD_FIFO_SPACE_LEN = 6;  // Length of the FIFO_SPACE field
localparam CMD_FIFO_SPACE_MAX = 32; // Size of command FIFO

// REG_TX_ERROR_POLICY bit fields
localparam TX_ERR_POLICY_LEN  = 2; // Length of error policy bit field


//-----------------------------------------------------------------------------
// Rx Radio Commands
//-----------------------------------------------------------------------------

localparam [RX_CMD_LEN-1:0] RX_CMD_STOP       = 0; // Stop acquiring at end of next packet
localparam [RX_CMD_LEN-1:0] RX_CMD_FINITE     = 1; // Acquire NUM_SAMPS then stop
localparam [RX_CMD_LEN-1:0] RX_CMD_CONTINUOUS = 2; // Acquire until stopped


//-----------------------------------------------------------------------------
// Tx Error Policies
//-----------------------------------------------------------------------------

localparam TX_ERR_POLICY_PACKET = 1;  // Wait for end of packet after error
localparam TX_ERR_POLICY_BURST  = 2;  // Wait for end of burst after error


//-----------------------------------------------------------------------------
// Error Codes
//-----------------------------------------------------------------------------

// Rx Error Codes
localparam ERR_RX_CODE_W = 2;  // Bit width of error code values
//
localparam ERR_RX_LATE_CMD = 1;  // Late command (arrived after indicated time)
localparam ERR_RX_OVERRUN  = 2;  // FIFO overflow


// Tx Error Codes
localparam ERR_TX_CODE_W = 2;  // Bit width of error code values
//
localparam ERR_TX_UNDERRUN  = 1;  // Data underflow (data not available when needed)
localparam ERR_TX_LATE_DATA = 2;  // Late data (arrived after indicated time)
localparam ERR_TX_EOB_ACK   = 3;  // Acknowledge end-of-burst (this is not an error)
