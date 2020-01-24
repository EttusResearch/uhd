//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module:  axi_ram_fifo_regs (Header)
//
// Description:  Header file for axi_ram_fifo_regs. All registers are 32-bit 
//               words from software's perspective.
//

// Address space size, per FIFO. That is, each FIFO is separated in the CTRL 
// Port address space by 2^FIFO_ADDR_W bytes.
localparam RAM_FIFO_ADDR_W = 7;


// REG_FIFO_INFO (R|W)
//
// Contains info/control bits for the FIFO.
//
// [31:16] : Returns the magic number 0xF1F0 (read-only)
//     [0] : Indicates if BIST logic is present (read-only)
//
localparam REG_FIFO_INFO = 'h0;
//
localparam REG_FIFO_MAGIC_POS      = 16;
localparam REG_FIFO_BIST_PRSNT_POS = 0;
//
localparam REG_FIFO_MAGIC_W = 16;


// REG_FIFO_READ_SUPPRESS (R|W)
//
// Controls the read suppression threshold. RAM reads will be disabled whenever
// the amount of free space in the input buffer (in units of RAM words) falls
// below this threshold. This is intended to prevent input buffer overflows
// caused by the RAM being too busy with reads. To disable the read suppression
// feature, set the threshold to 0. In general, the threshold should be set to
// a small value relative to the input FIFO buffer size (the IN_FIFO_SIZE
// field) so that it is only enabled when the input FIFO buffer is close to
// overflowing.
//

// [31:16] : Address width of input buffer. In other words, the input buffer is 
//           2**REG_FIFO_IN_FIFO_SIZE RAM words deep. (read-only)
// [15: 0] : Read suppression threshold, in RAM words (read/write)
//
localparam REG_FIFO_READ_SUPPRESS = 'h4;
//
localparam REG_FIFO_IN_FIFO_SIZE_POS    = 16;
localparam REG_FIFO_SUPPRESS_THRESH_POS = 0;
//
localparam REG_FIFO_IN_FIFO_SIZE_W    = 16;
localparam REG_FIFO_SUPPRESS_THRESH_W = 16;


// REG_FIFO_MEM_SIZE (R)
//
// Returns information about the size of the attached memory. The address size 
// allows software to determine what mask and base address values are valid.
//
// [31:16] : Returns the bit width of the RAM word size.
// [15: 0] : Returns the bit width of the RAM byte address size. That is, the 
//           addressable portion of the attached memory is 
//           2**REG_FIFO_ADDR_SIZE bytes.
//
localparam REG_FIFO_MEM_SIZE = 'h8;
//
localparam REG_FIFO_DATA_SIZE_POS = 16;
localparam REG_FIFO_ADDR_SIZE_POS = 0;
//
localparam REG_FIFO_DATA_SIZE_W = 16;
localparam REG_FIFO_ADDR_SIZE_W = 16;


// REG_FIFO_TIMEOUT (R/W)
//
// Programs the FIFO timeout, in memory interface clock cycles. For efficiency,
// we want the memory to read and write full bursts. But we also don't want
// smaller amounts of data to be stuck in the FIFO. This timeout determines how
// long we wait for new data before we go ahead and perform a smaller
// read/write. A longer timeout will make more efficient use of the memory, but
// will increase latency. The default value is set by a module parameter.
//
// [31:12] : <Reserved>
// [11: 0] : Timeout
//
localparam REG_FIFO_TIMEOUT = 'hC;
//
localparam REG_TIMEOUT_POS = 0;
localparam REG_TIMEOUT_W   = 12;


// REG_FIFO_FULLNESS (R)
//
// Returns the fullness of the FIFO in bytes. This is is a 64-bit register in 
// which the least-significant 32-bit word must be read first.
//
localparam REG_FIFO_FULLNESS_LO = 'h10;
localparam REG_FIFO_FULLNESS_HI = 'h14;


// REG_FIFO_ADDR_BASE (R|W)
//
// Sets the base byte address to use for this FIFO. This should only be updated 
// when the FIFO is idle. This should be set to a multiple of 
// REG_FIFO_ADDR_MASK+1. Depending on the size of the memory connected, upper 
// bits might be ignored. 
//
localparam REG_FIFO_ADDR_BASE_LO = 'h18;
localparam REG_FIFO_ADDR_BASE_HI = 'h1C;


// REG_FIFO_ADDR_MASK (R|W)
//
// The byte address mask that controls the portion of the memory address that 
// is allocated to this FIFO. For example, set to 0xFFFF for a 64 KiB memory. 
//
// This should only be updated when the FIFO is idle. It must be equal to a 
// power-of-2 minus 1. It should be no smaller than FIFO_ADDR_MASK_MIN, defined 
// in axi_ram_fifo.v, otherwise it will be coerced up to that size. 
//
// This is is a 64-bit register in which the least-significant 32-bit word must 
// be read/written first. Depending on the size of the memory connected, the 
// upper bits might be ignored.
//
localparam REG_FIFO_ADDR_MASK_LO = 'h20;
localparam REG_FIFO_ADDR_MASK_HI = 'h24;


// REG_FIFO_PACKET_CNT (R)
//
// Returns the number of packets transferred out of the FIFO block.
//
localparam REG_FIFO_PACKET_CNT = 'h28;


//-----------------------------------------------------------------------------
// BIST Registers
//-----------------------------------------------------------------------------
//
// Only read these registers if the BIST component is included.
//
//-----------------------------------------------------------------------------

// REG_BIST_CTRL (R|W)
//
// Control register for the BIST component. 
//
// [4] : BIST is running. Changes to 1 after a test is started, then returns to 
//       0 when BIST is complete.
//
// [3] : Continuous mode (run until stopped). When set to 1, test will continue 
//       to run until Stop bit is set.
//
// [2] : Clear the BIST counters (i.e., the TX, RX, cycle, and error counters)
//
// [1] : Stop BIST  (strobe). Write a 1 to this bit to stop the test that is 
//       currently running 
//
// [0] : Start BIST (strobe). Write a 1 to this bit to start a test using the 
//       configured NUM_BYTES and continuous mode setting.
//
localparam REG_BIST_CTRL = 'h30;
//
localparam REG_BIST_RUNNING_POS = 4;
localparam REG_BIST_CONT_POS    = 3;
localparam REG_BIST_CLEAR_POS   = 2;  // Strobe
localparam REG_BIST_STOP_POS    = 1;  // Strobe
localparam REG_BIST_START_POS   = 0;  // Strobe


// REG_BIST_CLOCK_RATE (R)
//
// Reports the clock rate of the BIST component in Hz. This can be used with 
// REG_BIST_CYCLE_COUNT to calculate throughput.
//
localparam REG_BIST_CLK_RATE = 'h34;


// REG_BIST_NUM_BYTES (R|W)
//
// Number of bytes to generate for the next BIST run. THis is not used if the 
// REG_BIST_CONT_POS bit is set. This register should not be updated while the 
// BIST is running.
//
localparam REG_BIST_NUM_BYTES_LO = 'h38;
localparam REG_BIST_NUM_BYTES_HI = 'h3C;


// REG_BIST_TX_BYTE_COUNT (R)
//
// Reports the number of bytes transmitted by the BIST component. This should 
// always be read least-significant word first to ensure coherency. Once BIST 
// is complete, the TX count will equal the RX count.
//
localparam REG_BIST_TX_BYTE_COUNT_LO = 'h40;
localparam REG_BIST_TX_BYTE_COUNT_HI = 'h44;


// REG_BIST_RX_BYTE_COUNT (R)
//
// Reports the number of bytes received by the BIST component. This should 
// always be read least-significant word first to ensure coherency. Once BIST 
// is complete, the TX count will equal the RX count.
//
localparam REG_BIST_RX_BYTE_COUNT_LO = 'h48;
localparam REG_BIST_RX_BYTE_COUNT_HI = 'h4C;


// REG_BIST_ERROR_COUNT (R)
//
// Reports the number of words in which the BIST component detected errors.
// This should always be read least-significant word first to ensure coherency.
//
localparam REG_BIST_ERROR_COUNT_LO = 'h50;
localparam REG_BIST_ERROR_COUNT_HI = 'h54;


// REG_BIST_CYCLE_COUNT (R)
//
// Reports the number of clock cycles that have elapsed while the BIST was 
// running. This can be used to calculate throughput. This should always be 
// read least-significant word first to ensure coherency.
//
localparam REG_BIST_CYCLE_COUNT_LO = 'h58;
localparam REG_BIST_CYCLE_COUNT_HI = 'h5C;

