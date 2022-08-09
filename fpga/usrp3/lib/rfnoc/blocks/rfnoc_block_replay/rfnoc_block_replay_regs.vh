//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_replay_regs (Header)
//
// Description:
//
//   This is a header file that contains the register descriptions for the
//   RFNoC Replay block.
//
//   Each RFNoC Replay block consists of NUM_PORTS independent replay engines.
//   Each one has its own address space that is REPLAY_ADDR_W bits wide. That
//   is, replay block N can be addressed starting at byte offset
//   N*(2**REPLAY_ADDR_W).
//
//   All 64-bit registers should be read/written least-significant word first
//   to guarantee coherence.
//

//-----------------------------------------------------------------------------
// Register Space
//-----------------------------------------------------------------------------

// The amount of address space taken up by each replay engine. That is, the
// address space for port N starts at N*(2^REPLAY_ADDR_W).
localparam REPLAY_ADDR_W = 20'h00008;


//-----------------------------------------------------------------------------
// Replay Register Descriptions
//-----------------------------------------------------------------------------

// REG_COMPAT (R)
//
// Compatibility version. This read-only register is used by software to
// determine if this block's version is compatible with the running software. A
// major version change indicates the software for the previous major version
// is no longer compatible. A minor version change means the previous version
// is compatible, but some new features may be unavailable.
//
// [31:16] Major version
// [15: 0] Minor version
//
localparam REG_COMPAT = 'h00;
//
localparam REG_MAJOR_POS = 16;
localparam REG_MAJOR_LEN = 16;
//
localparam REG_MINOR_POS =  0;
localparam REG_MINOR_LEN = 16;

// REG_MEM_SIZE (R)
//
// Returns information about the size of the attached memory. The address size
// allows software to determine what buffer size and base address values are
// valid.
//
// [31:16] : Memory Data Word Size. Returns the bit width of the RAM word size.
// [15: 0] : Memory Address Size. Returns the bit width of the RAM byte
//           address. That is, the memory is 2**VALUE bytes in size.
//
localparam REG_MEM_SIZE = 'h04;
//
localparam REG_DATA_SIZE_LEN = 16;
localparam REG_DATA_SIZE_POS = 16;
//
localparam REG_ADDR_SIZE_LEN = 16;
localparam REG_ADDR_SIZE_POS = 0;

// REG_REC_RESTART (W)
//
// Record Buffer Restart Register. Software must write to this register after
// updating the base address or buffer size. This will cause recording to
// restart at the indicated location. It does not matter what value you write.
//
localparam REG_REC_RESTART = 'h08;

// REG_REC_BASE_ADDR (R/W)
//
// Record Base Address Register. This is the byte address that controls where
// in the attached memory that recorded data should be stored. This must be a
// multiple of memory word size (REG_DATA_SIZE) in bytes.
//
localparam REG_REC_BASE_ADDR_LO = 'h10;
localparam REG_REC_BASE_ADDR_HI = 'h14;

// REG_REC_BUFFER_SIZE (R/W)
//
// Record Buffer Size Register. This controls the portion of the RAM allocated
// to the record buffer, in bytes. This must be a multiple of memory word size
// (REG_DATA_SIZE) in bytes.
//
localparam REG_REC_BUFFER_SIZE_LO = 'h18;
localparam REG_REC_BUFFER_SIZE_HI = 'h1C;

// REG_REC_FULLNESS (R)
//
// Record Fullness. Returns the number of bytes that have been recorded in the
// record buffer.
//
// This is is a 64-bit register in which the least-significant 32-bit word must
// be read first.
//
localparam REG_REC_FULLNESS_LO = 'h20;
localparam REG_REC_FULLNESS_HI = 'h24;

// REG_PLAY_BASE_ADDR (R/W)
//
// Playback Base Address Register. This is the byte address that controls where
// in the attached memory to read the data to be played back. This must be a
// multiple of memory word size (REG_DATA_SIZE) in bytes.
//
localparam REG_PLAY_BASE_ADDR_LO = 'h28;
localparam REG_PLAY_BASE_ADDR_HI = 'h2C;

// REG_PLAY_BUFFER_SIZE (R/W)
//
// Playback Buffer Size Register. This controls the size, in bytes, of the
// playback buffer in the attached memory. This must be a multiple of memory
// word size (REG_DATA_SIZE) in bytes.
//
localparam REG_PLAY_BUFFER_SIZE_LO = 'h30;
localparam REG_PLAY_BUFFER_SIZE_HI = 'h34;

// REG_PLAY_CMD_NUM_WORDS (R/W)
//
// Playback Command Number of Words. This register controls the number of
// memory data words to play back.
//
localparam REG_PLAY_CMD_NUM_WORDS_LO = 'h38;
localparam REG_PLAY_CMD_NUM_WORDS_HI = 'h3C;
//
localparam REG_CMD_NUM_WORDS_LEN = 64;

// REG_PLAY_CMD_TIME (R/W)
//
// Playback Command Time. This register indicates the timestamp to attach to
// the first packet that is played back, if timed playback is enabled.
// Subsequent packets will have the correctly incremented timestamp attached.
//
localparam REG_PLAY_CMD_TIME_LO = 'h40;
localparam REG_PLAY_CMD_TIME_HI = 'h44;
//
localparam REG_CMD_TIME_LEN = 64;

// REG_PLAY_CMD (W)
//
// Playback Command Register. This register mirrors the behavior of the RFNoC
// RX radio block. All commands are queued up in the replay command FIFO. The
// fields are as follows.
//
//   [31] : Timed flag. Indicates if the command is timed (1) or not (0).
//
//   [1:0] : Command field. The command indicates what you want the playback to
//           do. It can be one of the following:
//
//           0 (PLAY_CMD_STOP)       : Stop playing back data
//           1 (PLAY_CMD_FINITE)     : Acquire NUM_SAMPS then stop
//           2 (PLAY_CMD_CONTINUOUS) : Play back continuously until stopped.
//
localparam REG_PLAY_CMD = 'h48;
//
localparam REG_PLAY_TIMED_POS = 31;
localparam REG_PLAY_TIMED_LEN =  1;
//
localparam REG_PLAY_CMD_POS = 0;
localparam REG_PLAY_CMD_LEN = 2;

// REG_PLAY_WORDS_PER_PKT (R/W)
//
// [15:0] Words Per Packet. This registers controls how many memory data words
//        (REG_DATA_SIZE bits each) are inserted into each packet during
//        playback. Effectively, it controls the samples-per-packet (SPP), but
//        the replay block is sample-size agnostic.
//
//        This value should never be set such that the total RFNoC packet size
//        would exceed the system MTU or the maximum packet size allowed by
//        RFNoC (2^16 bytes). Also note that the last packet of a command may
//        be less than this size.
//
localparam REG_PLAY_WORDS_PER_PKT = 'h4C;
//
localparam REG_PLAY_WORDS_PER_PKT_LEN = 16;
//
localparam REG_PLAY_WORDS_PER_PKT_INIT = 160;

// REG_PLAY_ITEM_SIZE (R/W)
//
// [7:0] Number of bytes per item. This controls how much time is incremented
//       for each memory word of data. This must be a power of 2.
//
localparam REG_PLAY_ITEM_SIZE = 'h50;
//
localparam REG_ITEM_SIZE_POS = 0;
localparam REG_ITEM_SIZE_LEN = 8;

// REG_REC_POS (R)
//
// Returns the byte address of the record pointer.
//
localparam REG_REC_POS_LO = 'h54;
localparam REG_REC_POS_HI = 'h58;

// REG_PLAY_POS (R)
//
// Returns the byte address of the play pointer.
//
localparam REG_PLAY_POS_LO = 'h5C;
localparam REG_PLAY_POS_HI = 'h60;

// REG_PLAY_CMD_FIFO_SPACE (R)
//
// Returns remaining space in the command FIFO
//
localparam REG_PLAY_CMD_FIFO_SPACE = 'h64;

//-----------------------------------------------------------------------------
// Playback Commands
//-----------------------------------------------------------------------------

localparam PLAY_CMD_STOP       = 2'h0;
localparam PLAY_CMD_FINITE     = 2'h1;
localparam PLAY_CMD_CONTINUOUS = 2'h2;