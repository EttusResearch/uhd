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
//   WARNING: The FFT block's configuration registers should only be changed
//   when the block is idle, otherwise data corruption will occur.
//

package fft_core_regs_pkg;

  // Amount of address space allocated to each FFT core.
  localparam int FFT_CORE_ADDR_W = 10;

  // Total address space required by the registers below.
  localparam int REG_ADDR_W = 7;


  //---------------------------------------------------------------------------
  // Register Descriptions
  //---------------------------------------------------------------------------

  // REG_COMPAT (Read-only)
  //
  // Compatibility register to indicate the version of the FPGA code. Returns a
  // 16-bit value where the upper 8 bits are the major and the lower 8 bits are
  // the minor compat number, with the current version being "major.minor". A
  // "major" change indicates a change that breaks backwards compatibility. A
  // "minor" change is one in which backwards compatibility has been preserved.
  //
  localparam int REG_COMPAT_ADDR  = 'h00;
  localparam int REG_COMPAT_WIDTH = 32;

  // REG_PORT_CONFIG (Read-only)
  //
  // Returns information about the the number of channels per FFT core. Each
  // FFT core has its own register space. So, this effectively tells you which
  // ports can be individually configured.
  //
  //   [31:16] : NUM_CORES. The number of FFT cores in the parent RFNoC block.
  //   [15: 0] : NUM_CHAN. The number of channels per FFT core.
  //
  localparam int REG_PORT_CONFIG_ADDR  = 'h04;
  localparam int REG_PORT_CONFIG_WIDTH = 32;
  //
  localparam int REG_NUM_CORES_W = 16;
  localparam int REG_NUM_CHAN_W  = 16;

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
  localparam int REG_CAPABILITIES_ADDR  = 'h08;
  localparam int REG_CAPABILITIES_WIDTH = 32;

  // REG_CAPABILITIES2 (Read-only)
  //
  // Returns information about the post-processing capabilities.
  //
  // [11:8] : Log base 2 of the number of items per clock cycle (NIPC)
  //          processed by this core. For example, a value of 3 in this field
  //          means that the NIPC is 2**3 == 8. Packet sizes and cyclic prefix
  //          lengths must be a multiple of the NIPC value.
  // [ 7:6] : Reserved
  // [   5] : CP_INSERTION. Indicates if cyclic-prefix insertion is available.
  // [   4] : CP_REMOVAL. Indicates if cyclic-prefix removal is available.
  // [   3] : MAGNITUDE_SQ. Indicates whether or not the magnitude-squared
  //          output capability is present in the core.
  // [   2] : MAGNITUDE. Indicates whether or not the magnitude output option
  //          is present in the core.
  // [   1] : FFT_ORDER. Indicates whether or not the FFT reorder capability is
  //          present in the core.
  // [   0] : FFT_BYPASS. Indicates whether or not the FFT bypass capability is
  //         present in the core.
  //
  localparam int REG_CAPABILITIES2_ADDR  = 'h0C;
  localparam int REG_CAPABILITIES2_WIDTH = 12;

  // REG_RESET (Write-only strobe)
  //
  // Any write to this register forces a reset of the block's internal logic.
  // This register is self-clearing.
  //
  localparam int REG_RESET_ADDR  = 'h10;
  localparam int REG_RESET_WIDTH = 1;

  // REG_LENGTH (Read/Write)
  //
  // Log base 2 of FFT size. Use this register to configure the desired FFT
  // size. Example: For 512 point FFT, set this register to 9. For a 4k FFT,
  // set this register to 12. This should never exceed the maximum FFT size
  // indicated by the REG_CAPABILITIES register.
  //
  localparam int REG_LENGTH_LOG2_ADDR = 'h14;

  // REG_SCALING (Read/Write)
  //
  // This is the FFT scaling word used by the Xilinx FFT core. This determines
  // the scale of the output and can be adjusted to prevent overflow in the FFT
  // computation. The value needed here depends on the FFT size, because that
  // determines the number of stages in the FFT core. see the Xilinx FFT
  // documentation PG901 for details. In general, it is a
  // 2*ceil(length_log2)-bit number. To to achieve 1/N scaling, it should be
  // 10...10 if the FFT size is a power of 4 (log2 of size is even) and it
  // should be 0110...10 if the FFT size is not a power of 4 (log2 of size is
  // odd). Example: A value of 0b_10_10_10_10_10_10 (2730) leads to 1/N scaling
  // for the 4K FFT (6 stages). A value of 0b_01_10_10_10_10_10 (1706) leads to
  // 1/N scaling for the 2K FFT (6 stages).
  //
  localparam int REG_SCALING_ADDR = 'h18;

  // REG_DIRECTION (Read/Write)
  //
  // Sets the FFT direction. Use 1 for forward and 0 for inverse FFT.
  //
  localparam int REG_DIRECTION_ADDR  = 'h1C;
  localparam int REG_DIRECTION_WIDTH = 1;

  // FFT direction constants
  localparam bit FFT_INVERSE = 0;
  localparam bit FFT_FORWARD = 1;

  // REG_CP_INS_LEN (Read/Write)
  //
  // Cyclic Prefix (CP) insertion length. This register holds the next CP
  // insertion length to be loaded into the CP insertion list. Write the value
  // to be loaded into this register then use the REG_CP_INS_LIST_LOAD register
  // to load it into the list.
  //
  localparam int REG_CP_INS_LEN_ADDR = 'h20;

  // REG_CP_INS_LIST_LOAD (Write-only strobe)
  //
  // Cyclic prefix insertion list load. Any write to this register will load
  // the value in the REG_CP_INS_LEN register into the cyclic prefix insertion
  // list.
  //
  localparam int REG_CP_INS_LIST_LOAD_ADDR  = 'h24;
  localparam int REG_CP_INS_LIST_LOAD_WIDTH = 1;

  // REG_CP_INS_LIST_CLR (Write-only strobe)
  //
  // Cyclic prefix insertion list clear. Any write to this register will clear
  // the cyclic prefix insertion list so that it becomes empty.
  //
  localparam int REG_CP_INS_LIST_CLR_ADDR  = 'h28;
  localparam int REG_CP_INS_LIST_CLR_WIDTH = 1;

  // REG_CP_INS_LIST_OCC (Read-only)
  //
  // Cyclic prefix insertion list occupied length. Returns the fullness of
  // cyclic prefix insertion list. You must not overfill the insertion list, so
  // this should never exceed the maximum list length indicated by the
  // REG_CAPABILITIES register.
  //
  localparam int REG_CP_INS_LIST_OCC_ADDR  = 'h2C;
  localparam int REG_CP_INS_LIST_OCC_WIDTH = 16;

  // REG_CP_REM_LEN (Read/Write)
  //
  // Cyclic Prefix (CP) removal length. This register holds the next CP removal
  // length to be loaded into the CP removal list. Write the value to be loaded
  // into this register then use the REG_CP_REM_LIST_LOAD register to load it
  // into the list.
  //
  localparam int REG_CP_REM_LEN_ADDR = 'h30;

  // REG_CP_REM_LIST_LOAD (Write-only strobe)
  //
  // Cyclic prefix removal list load. Any write to this register will load the
  // value in the REG_CP_REM_LEN register into the cyclic prefix removal list.
  //
  localparam int REG_CP_REM_LIST_LOAD_ADDR  = 'h34;
  localparam int REG_CP_REM_LIST_LOAD_WIDTH = 1;

  // REG_CP_REM_LIST_CLR (Write-only strobe)
  //
  // Cyclic prefix removal list clear. Any write to this register will clear
  // the cyclic prefix removal list so that it becomes empty.
  //
  localparam int REG_CP_REM_LIST_CLR_ADDR  = 'h38;
  localparam int REG_CP_REM_LIST_CLR_WIDTH = 1;

  // REG_CP_REM_LIST_OCC (Read-only)
  //
  // Cyclic prefix insertion list occupied length. Returns the fullness of
  // cyclic prefix insertion list. You must not overfill the insertion list, so
  // this should never exceed the maximum list length indicated by the
  // REG_CAPABILITIES register.
  //
  localparam int REG_CP_REM_LIST_OCC_ADDR  = 'h3C;
  localparam int REG_CP_REM_LIST_OCC_WIDTH = 16;

  // REG_OVERFLOW (Read-only)
  //
  // Returns the overflow status of the currently addressed FFT core. Each bit
  // position corresponds to a unique channel on the core. The least
  // significant bit (bit 0) corresponds to the first channel, and so on. A
  // value of 1 indicates that an overflow has occurred on the corresponding
  // channel since the register was last read. Value of zero indicates that an
  // overflow has not occurred since the register was last read. The register
  // is reset back to zero whenever the register is read.
  //
  localparam int REG_OVERFLOW_ADDR = 'h40;

  // REG_BYPASS
  //
  // Enable FFT bypass. Set to 1 to enable, 0 to disable. When enabled, the
  // data is passed through without performing the FFT/IFFT processing. Ensure
  // that REG_CAPABILITIES2 reports this logic is present before using.
  //
  localparam int REG_BYPASS_ADDR  = 'h44;
  localparam int REG_BYPASS_WIDTH = 1;

  // REG_ORDER (Read/Write)
  //
  // Configures the FFT data order. The default is NORMAL. Use NATURAL for
  // inverse FFT. The following values are allowed:
  //
  //   0 : NORMAL. Negative frequencies first, then positive frequencies. 0 Hz
  //       is in the center.
  //   1 : REVERSE. Reverse order of NORMAL. Positive frequencies first, then
  //       negative frequencies. 0 Hz in the center.
  //   2 : NATURAL. Positive frequencies are first, followed by negative
  //       frequencies. 0 Hz is on the left.
  //   3 : BIT_REVERSE. Like natural, but the bits of the indices are in
  //       reverse order. For example, for a size 16 FFT, bin 0000 is output
  //       first, followed by bin 1000, 0100, 1100, 0010, etc.
  //
  localparam int REG_ORDER_ADDR  = 'h48;
  localparam int REG_ORDER_WIDTH = 2;
  //
  localparam int FFT_ORDER_NORMAL      = 0;
  localparam int FFT_ORDER_REVERSE     = 1;
  localparam int FFT_ORDER_NATURAL     = 2;
  localparam int FFT_ORDER_BIT_REVERSE = 3;

  // REG_MAGNITUDE (Read/Write)
  //
  // Configures the magnitude computation. Normal complex output is the
  // default. Use normal mode for inverse FFT. Ensure that REG_CAPABILITIES2
  // reports the desired logic is present before using. The following values
  // are allowed:
  //
  //   0 - Normal complex output (no magnitude calculation)
  //   1 - Magnitude output
  //   2 - Magnitude squared output
  //
  localparam int REG_MAGNITUDE_ADDR  = 'h4C;
  localparam int REG_MAGNITUDE_WIDTH = 2;
  //
  localparam int MAG_SEL_NONE   = 0;
  localparam int MAG_SEL_MAG    = 1;
  localparam int MAG_SEL_MAG_SQ = 2;

endpackage : fft_core_regs_pkg
