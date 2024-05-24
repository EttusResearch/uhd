//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: fft_core_regs_pkg
//
// Description:
//
//   Package file for registers and register documentation for the fft_core
//   module.
//

package fft_core_regs_pkg;

  // Total address space required by the registers below
  localparam int REG_ADDR_W = 4;


  //---------------------------------------------------------------------------
  // Register Descriptions
  //---------------------------------------------------------------------------

  // REG_COMPAT (Read-only)
  //
  // Compatibility register to indicate the version of the FPGA code. Returns a
  // 16-bit value where the upper 8 bits are the major and the lower 8 bits are
  // the minor compat number, with the current version being "major.minor".
  //
  localparam int REG_COMPAT_ADDR  = 'h0;
  localparam int REG_COMPAT_WIDTH = 32;

  // REG_CAPABILITIES (Read-only)
  //
  // Returns the capabilities of the core.
  //
  // [31:24] Log base 2 of the maximum cyclic prefix list length for insertion.
  //         The maximum CP list length is 2**this-1. Example: A value of 5
  //         means 31 is the maximum list length.
  //
  // [23:16] Log base 2 of the maximum cyclic prefix list length for removal.
  //         The maximum CP list length is 2**this-1. Example: A value of 5
  //         means 31 is the maximum list length.
  //
  // [15: 8] Log base 2 of the maximum cyclic prefix length (for both insertion
  //         and removal). The maximum cyclic prefix is 2**this-1. Example: A
  //         value of 12 means 4095 is the maximum cyclic prefix length.
  //
  // [ 7: 0] Log base 2 of the maximum supported FFT size. Example: A value of
  //         12 means 4096 is the maximum FFT size. The minimum supported FFT
  //         size is always 8 for the Xilinx FFT core.
  //
  localparam int REG_CAPABILITIES_ADDR  = 'h1;
  localparam int REG_CAPABILITIES_WIDTH = 32;

  // REG_USER_RESET (Write-only strobe)
  //
  // Any write to this register forces a reset of the block 16 clock cycles.
  // This register is self-clearing.
  //
  localparam int REG_USER_RESET_ADDR  = 'h2;
  localparam int REG_USER_RESET_WIDTH = 1;

  // REG_FFT_SIZE (Read/Write)
  //
  // Log base 2 of FFT size. Use this register to configure the desired FFT
  // size. Example: For 512 point FFT, set this register to 9. For a 4k FFT,
  // set this register to 12. This should never exceed the maximum FFT size
  // indicated by the REG_CAPABILITIES register.
  //
  localparam int REG_FFT_SIZE_LOG2_ADDR = 'h3;

  // FFT_SCALING (Read/Write)
  //
  // This is the FFT scaling word used by the Xilinx FFT core. This determines
  // the scale of the output and can be adjusted to prevent overflow in the FFT
  // computation. The value needed here depends on the FFT size, because that
  // determines the number of stages in the FFT core. see the Xilinx FFT
  // documentation PG901 for details. A value of 0b_10_10_10_10_10_10 (2730)
  // leads to 1/N scaling for the 4K FFT (6 stages).
  //
  localparam int REG_FFT_SCALING_ADDR = 'h4;

  // REG_FFT_DIRECTION (Read/Write)
  //
  // Sets the FFT direction. Use 1 for forward and 0 for inverse FFT.
  //
  localparam int REG_FFT_DIRECTION_ADDR  = 'h5;
  localparam int REG_FFT_DIRECTION_WIDTH = 1;

  // REG_CP_INS_LEN (Read/Write)
  //
  // Cyclic Prefix (CP) insertion length. This register holds the next CP
  // insertion length to be loaded into the CP insertion list. Write the value
  // to be loaded into this register then use the REG_CP_INS_LIST_LOAD register
  // to load it into the list.
  //
  localparam int REG_CP_INS_LEN_ADDR = 'h6;

  // REG_CP_INS_LIST_LOAD (Write-only strobe)
  //
  // Cyclic prefix insertion list LOAD. Any write to this register will load
  // the value in the REG_CP_INS_LEN register into the cyclic prefix insertion
  // list.
  //
  localparam int REG_CP_INS_LIST_LOAD_ADDR  = 'h7;
  localparam int REG_CP_INS_LIST_LOAD_WIDTH = 1;

  // REG_CP_INS_LIST_CLR (Write-only strobe)
  //
  // Cyclic prefix insertion list clear. Any write to this register will clear
  // the cyclic prefix insertion list so that it becomes empty.
  //
  localparam int REG_CP_INS_LIST_CLR_ADDR  = 'h8;
  localparam int REG_CP_INS_LIST_CLR_WIDTH = 1;

  // REG_CP_INS_LIST_OCC (Read-only)
  //
  // Cyclic prefix insertion list occupied length. Returns the fullness of
  // cyclic prefix insertion list. You must not overfill the insertion list, so
  // this should never exceed the maximum list length indicated by the
  // REG_CAPABILITIES register.
  //
  localparam int REG_CP_INS_LIST_OCC_ADDR  = 'h9;
  localparam int REG_CP_INS_LIST_OCC_WIDTH = 16;

  // REG_CP_REM_LEN (Read/Write)
  //
  // Cyclic Prefix (CP) removal length. This register holds the next CP removal
  // length to be loaded into the CP removal list. Write the value to be loaded
  // into this register then use the REG_CP_REM_LIST_LOAD register to load it
  // into the list.
  //
  localparam int REG_CP_REM_LEN_ADDR = 'hA;

  // REG_CP_REM_LIST_LOAD (Write-only strobe)
  //
  // Cyclic prefix removal list LOAD. Any write to this register will load the
  // value in the REG_CP_REM_LEN register into the cyclic prefix removal list.
  //
  localparam int REG_CP_REM_LIST_LOAD_ADDR  = 'hB;
  localparam int REG_CP_REM_LIST_LOAD_WIDTH = 1;

  // REG_CP_REM_LIST_CLR (Write-only strobe)
  //
  // Cyclic prefix removal list clear. Any write to this register will clear
  // the cyclic prefix removal list so that it becomes empty.
  //
  localparam int REG_CP_REM_LIST_CLR_ADDR  = 'hC;
  localparam int REG_CP_REM_LIST_CLR_WIDTH = 1;

  // REG_CP_INS_LIST_OCC (Read-only)
  //
  // Cyclic prefix insertion list occupied length. Returns the fullness of
  // cyclic prefix insertion list. You must not overfill the insertion list, so
  // this should never exceed the maximum list length indicated by the
  // REG_CAPABILITIES register.
  //
  localparam int REG_CP_REM_LIST_OCC_ADDR  = 'hD;
  localparam int REG_CP_REM_LIST_OCC_WIDTH = 16;

  // REG_BYPASS (Read-only)
  //
  // FFT bypass. When set to 1, causes the FFT core to be bypassed so that
  // input data is passed through unmodified. When 0 (default), the FFT core is
  // used. This should only be changed when the FFT core is idle.
  //
  // Note that cyclic prefix insertion will not work in bypass mode, because
  // insertion is handled by the FFT core, but removal will work.
  //
  localparam int REG_BYPASS_ADDR  = 'hE;
  localparam int REG_BYPASS_WIDTH = 1;

endpackage : fft_core_regs_pkg
