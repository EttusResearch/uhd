//*****************************************************************************
// (c) Copyright 2009 - 2010 Xilinx, Inc. All rights reserved.
//
// This file contains confidential and proprietary information
// of Xilinx, Inc. and is protected under U.S. and
// international copyright and other intellectual property
// laws.
//
// DISCLAIMER
// This disclaimer is not a license and does not grant any
// rights to the materials distributed herewith. Except as
// otherwise provided in a valid license issued to you by
// Xilinx, and to the maximum extent permitted by applicable
// law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
// WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
// AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
// BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
// INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
// (2) Xilinx shall not be liable (whether in contract or tort,
// including negligence, or under any other theory of
// liability) for any loss or damage of any kind or nature
// related to, arising under or in connection with these
// materials, including for any direct, or any indirect,
// special, incidental, or consequential loss or damage
// (including loss of data, profits, goodwill, or any type of
// loss or damage suffered as a result of any action brought
// by a third party) even if such damage or loss was
// reasonably foreseeable or Xilinx had been advised of the
// possibility of the same.
//
// CRITICAL APPLICATIONS
// Xilinx products are not designed or intended to be fail-
// safe, or for use in any application requiring fail-safe
// performance, such as life-support or safety devices or
// systems, Class III medical devices, nuclear facilities,
// applications related to the deployment of airbags, or any
// other applications that could lead to death, personal
// injury, or severe property or environmental damage
// (individually and collectively, "Critical
// Applications"). Customer assumes the sole risk and
// liability of any use of Xilinx products in Critical
// Applications, subject only to applicable laws and
// regulations governing limitations on product liability.
//
// THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
// PART OF THIS FILE AT ALL TIMES.
//
//*****************************************************************************
//   ____  ____
//  /   /\/   /
// /___/  \  /    Vendor             : Xilinx
// \   \   \/     Version            : 1.8
//  \   \         Application        : MIG
//  /   /         Filename           : ddr3_interface.v
// /___/   /\     Date Last Modified : $Date: 2011/06/02 08:35:03 $
// \   \  /  \    Date Created       : Tue Sept 21 2010
//  \___\/\___\
//
// Device           : 7 Series
// Design Name      : DDR3 SDRAM
// Purpose          :
//   Top-level  module. This module can be instantiated in the
//   system and interconnect as shown in example design (example_top module).
//   In addition to the memory controller, the module instantiates:
//     1. Clock generation/distribution, reset logic
//     2. IDELAY control block
//     3. Debug logic
// Reference        :
// Revision History :
//*****************************************************************************

`timescale 1ps/1ps

module ddr3_interface #
  (


   //***************************************************************************
   // The following parameters refer to width of various ports
   //***************************************************************************
   parameter BANK_WIDTH            = 3,
                                     // # of memory Bank Address bits.
   parameter CK_WIDTH              = 1,
                                     // # of CK/CK# outputs to memory.
   parameter COL_WIDTH             = 10,
                                     // # of memory Column Address bits.
   parameter CS_WIDTH              = 1,
                                     // # of unique CS outputs to memory.
   parameter nCS_PER_RANK          = 1,
                                     // # of unique CS outputs per rank for phy
   parameter CKE_WIDTH             = 1,
                                     // # of CKE outputs to memory.
   parameter DATA_BUF_ADDR_WIDTH   = 5,
   parameter DQ_CNT_WIDTH          = 5,
                                     // = ceil(log2(DQ_WIDTH))
   parameter DQ_PER_DM             = 8,
   parameter DM_WIDTH              = 4,
                                     // # of DM (data mask)
   parameter DQ_WIDTH              = 32,
                                     // # of DQ (data)
   parameter DQS_WIDTH             = 4,
   parameter DQS_CNT_WIDTH         = 2,
                                     // = ceil(log2(DQS_WIDTH))
   parameter DRAM_WIDTH            = 8,
                                     // # of DQ per DQS
   parameter ECC                   = "OFF",
   parameter DATA_WIDTH            = 32,
   parameter ECC_TEST              = "OFF",
   parameter PAYLOAD_WIDTH         = (ECC_TEST == "OFF") ? DATA_WIDTH : DQ_WIDTH,
   parameter ECC_WIDTH             = 8,
   parameter MC_ERR_ADDR_WIDTH     = 31,
      
   parameter nBANK_MACHS           = 4,
   parameter RANKS                 = 1,
                                     // # of Ranks.
   parameter ODT_WIDTH             = 1,
                                     // # of ODT outputs to memory.
   parameter ROW_WIDTH             = 15,
                                     // # of memory Row Address bits.
   parameter ADDR_WIDTH            = 29,
                                     // # = RANK_WIDTH + BANK_WIDTH
                                     //     + ROW_WIDTH + COL_WIDTH;
                                     // Chip Select is always tied to low for
                                     // single rank devices
   parameter USE_CS_PORT          = 1,
                                     // # = 1, When Chip Select (CS#) output is enabled
                                     //   = 0, When Chip Select (CS#) output is disabled
                                     // If CS_N disabled, user must connect
                                     // DRAM CS_N input(s) to ground
   parameter USE_DM_PORT           = 1,
                                     // # = 1, When Data Mask option is enabled
                                     //   = 0, When Data Mask option is disbaled
                                     // When Data Mask option is disabled in
                                     // MIG Controller Options page, the logic
                                     // related to Data Mask should not get
                                     // synthesized
   parameter USE_ODT_PORT          = 1,
                                     // # = 1, When ODT output is enabled
                                     //   = 0, When ODT output is disabled
   parameter PHY_CONTROL_MASTER_BANK = 1,
                                     // The bank index where master PHY_CONTROL resides,
                                     // equal to the PLL residing bank
   parameter MEM_DENSITY           = "4Gb",
                                     // Indicates the density of the Memory part
                                     // Added for the sake of Vivado simulations
   parameter MEM_SPEEDGRADE        = "125",
                                     // Indicates the Speed grade of Memory Part
                                     // Added for the sake of Vivado simulations
   parameter MEM_DEVICE_WIDTH      = 16,
                                     // Indicates the device width of the Memory Part
                                     // Added for the sake of Vivado simulations

   //***************************************************************************
   // The following parameters are mode register settings
   //***************************************************************************
   parameter AL                    = "0",
                                     // DDR3 SDRAM:
                                     // Additive Latency (Mode Register 1).
                                     // # = "0", "CL-1", "CL-2".
                                     // DDR2 SDRAM:
                                     // Additive Latency (Extended Mode Register).
   parameter nAL                   = 0,
                                     // # Additive Latency in number of clock
                                     // cycles.
   parameter BURST_MODE            = "8",
                                     // DDR3 SDRAM:
                                     // Burst Length (Mode Register 0).
                                     // # = "8", "4", "OTF".
                                     // DDR2 SDRAM:
                                     // Burst Length (Mode Register).
                                     // # = "8", "4".
   parameter BURST_TYPE            = "SEQ",
                                     // DDR3 SDRAM: Burst Type (Mode Register 0).
                                     // DDR2 SDRAM: Burst Type (Mode Register).
                                     // # = "SEQ" - (Sequential),
                                     //   = "INT" - (Interleaved).
   parameter CL                    = 7,
                                     // in number of clock cycles
                                     // DDR3 SDRAM: CAS Latency (Mode Register 0).
                                     // DDR2 SDRAM: CAS Latency (Mode Register).
   parameter CWL                   = 6,
                                     // in number of clock cycles
                                     // DDR3 SDRAM: CAS Write Latency (Mode Register 2).
                                     // DDR2 SDRAM: Can be ignored
   parameter OUTPUT_DRV            = "HIGH",
                                     // Output Driver Impedance Control (Mode Register 1).
                                     // # = "HIGH" - RZQ/7,
                                     //   = "LOW" - RZQ/6.
   parameter RTT_NOM               = "60",
                                     // RTT_NOM (ODT) (Mode Register 1).
                                     //   = "120" - RZQ/2,
                                     //   = "60"  - RZQ/4,
                                     //   = "40"  - RZQ/6.
   parameter RTT_WR                = "OFF",
                                     // RTT_WR (ODT) (Mode Register 2).
                                     // # = "OFF" - Dynamic ODT off,
                                     //   = "120" - RZQ/2,
                                     //   = "60"  - RZQ/4,
   parameter ADDR_CMD_MODE         = "1T" ,
                                     // # = "1T", "2T".
   parameter REG_CTRL              = "OFF",
                                     // # = "ON" - RDIMMs,
                                     //   = "OFF" - Components, SODIMMs, UDIMMs.
   parameter CA_MIRROR             = "OFF",
                                     // C/A mirror opt for DDR3 dual rank
   
   //***************************************************************************
   // The following parameters are multiplier and divisor factors for PLLE2.
   // Based on the selected design frequency these parameters vary.
   //***************************************************************************
   parameter CLKIN_PERIOD          = 10000,
                                     // Input Clock Period
   parameter CLKFBOUT_MULT         = 10,
                                     // write PLL VCO multiplier
   parameter DIVCLK_DIVIDE         = 1,
                                     // write PLL VCO divisor
   parameter CLKOUT0_PHASE         = 337.5,
                                     // Phase for PLL output clock (CLKOUT0)
   parameter CLKOUT0_DIVIDE        = 2,
                                     // VCO output divisor for PLL output clock (CLKOUT0)
   parameter CLKOUT1_DIVIDE        = 2,
                                     // VCO output divisor for PLL output clock (CLKOUT1)
   parameter CLKOUT2_DIVIDE        = 32,
                                     // VCO output divisor for PLL output clock (CLKOUT2)
   parameter CLKOUT3_DIVIDE        = 8,
                                     // VCO output divisor for PLL output clock (CLKOUT3)
   parameter CLKOUT5_DIVIDE        = 5, // IJB
                                     // VCO output divisor for PLL output clock (CLKOUT5)


   //***************************************************************************
   // Memory Timing Parameters. These parameters varies based on the selected
   // memory part.
   //***************************************************************************
   parameter tCKE                  = 5000,
                                     // memory tCKE paramter in pS
   parameter tFAW                  = 30000,
                                     // memory tRAW paramter in pS.
   parameter tPRDI                 = 1_000_000,
                                     // memory tPRDI paramter in pS.
   parameter tRAS                  = 35000,
                                     // memory tRAS paramter in pS.
   parameter tRCD                  = 13750,
                                     // memory tRCD paramter in pS.
   parameter tREFI                 = 7800000,
                                     // memory tREFI paramter in pS.
   parameter tRFC                  = 300000,
                                     // memory tRFC paramter in pS.
   parameter tRP                   = 13750,
                                     // memory tRP paramter in pS.
   parameter tRRD                  = 6000,
                                     // memory tRRD paramter in pS.
   parameter tRTP                  = 7500,
                                     // memory tRTP paramter in pS.
   parameter tWTR                  = 7500,
                                     // memory tWTR paramter in pS.
   parameter tZQI                  = 128_000_000,
                                     // memory tZQI paramter in nS.
   parameter tZQCS                 = 64,
                                     // memory tZQCS paramter in clock cycles.

   //***************************************************************************
   // Simulation parameters
   //***************************************************************************
   parameter SIM_BYPASS_INIT_CAL   = "OFF",
                                     // # = "OFF" -  Complete memory init &
                                     //              calibration sequence
                                     // # = "SKIP" - Not supported
                                     // # = "FAST" - Complete memory init & use
                                     //              abbreviated calib sequence

   parameter SIMULATION            = "FALSE",
                                     // Should be TRUE during design simulations and
                                     // FALSE during implementations

   //***************************************************************************
   // The following parameters varies based on the pin out entered in MIG GUI.
   // Do not change any of these parameters directly by editing the RTL.
   // Any changes required should be done through GUI and the design regenerated.
   //***************************************************************************
   parameter BYTE_LANES_B0         = 4'b1111,
                                     // Byte lanes used in an IO column.
   parameter BYTE_LANES_B1         = 4'b1110,
                                     // Byte lanes used in an IO column.
   parameter BYTE_LANES_B2         = 4'b0000,
                                     // Byte lanes used in an IO column.
   parameter BYTE_LANES_B3         = 4'b0000,
                                     // Byte lanes used in an IO column.
   parameter BYTE_LANES_B4         = 4'b0000,
                                     // Byte lanes used in an IO column.
   parameter DATA_CTL_B0           = 4'b1111,
                                     // Indicates Byte lane is data byte lane
                                     // or control Byte lane. '1' in a bit
                                     // position indicates a data byte lane and
                                     // a '0' indicates a control byte lane
   parameter DATA_CTL_B1           = 4'b0000,
                                     // Indicates Byte lane is data byte lane
                                     // or control Byte lane. '1' in a bit
                                     // position indicates a data byte lane and
                                     // a '0' indicates a control byte lane
   parameter DATA_CTL_B2           = 4'b0000,
                                     // Indicates Byte lane is data byte lane
                                     // or control Byte lane. '1' in a bit
                                     // position indicates a data byte lane and
                                     // a '0' indicates a control byte lane
   parameter DATA_CTL_B3           = 4'b0000,
                                     // Indicates Byte lane is data byte lane
                                     // or control Byte lane. '1' in a bit
                                     // position indicates a data byte lane and
                                     // a '0' indicates a control byte lane
   parameter DATA_CTL_B4           = 4'b0000,
                                     // Indicates Byte lane is data byte lane
                                     // or control Byte lane. '1' in a bit
                                     // position indicates a data byte lane and
                                     // a '0' indicates a control byte lane
   parameter PHY_0_BITLANES        = 48'h3FE_3FE_3FE_2FF,
   parameter PHY_1_BITLANES        = 48'h3FF_FFF_C00_000,
   parameter PHY_2_BITLANES        = 48'h000_000_000_000,

   // control/address/data pin mapping parameters
   parameter CK_BYTE_MAP
     = 144'h00_00_00_00_00_00_00_00_00_00_00_00_00_00_00_00_00_13,
   parameter ADDR_MAP
     = 192'h000_139_138_137_136_135_134_133_132_131_130_129_128_127_126_12B,
   parameter BANK_MAP   = 36'h12A_125_124,
   parameter CAS_MAP    = 12'h122,
   parameter CKE_ODT_BYTE_MAP = 8'h00,
   parameter CKE_MAP    = 96'h000_000_000_000_000_000_000_11B,
   parameter ODT_MAP    = 96'h000_000_000_000_000_000_000_11A,
   parameter CS_MAP     = 120'h000_000_000_000_000_000_000_000_000_120,
   parameter PARITY_MAP = 12'h000,
   parameter RAS_MAP    = 12'h123,
   parameter WE_MAP     = 12'h121,
   parameter DQS_BYTE_MAP
     = 144'h00_00_00_00_00_00_00_00_00_00_00_00_00_00_00_01_02_03,
   parameter DATA0_MAP  = 96'h031_032_033_034_035_036_037_038,
   parameter DATA1_MAP  = 96'h021_022_023_024_025_026_027_028,
   parameter DATA2_MAP  = 96'h011_012_013_014_015_016_017_018,
   parameter DATA3_MAP  = 96'h000_001_002_003_004_005_006_007,
   parameter DATA4_MAP  = 96'h000_000_000_000_000_000_000_000,
   parameter DATA5_MAP  = 96'h000_000_000_000_000_000_000_000,
   parameter DATA6_MAP  = 96'h000_000_000_000_000_000_000_000,
   parameter DATA7_MAP  = 96'h000_000_000_000_000_000_000_000,
   parameter DATA8_MAP  = 96'h000_000_000_000_000_000_000_000,
   parameter DATA9_MAP  = 96'h000_000_000_000_000_000_000_000,
   parameter DATA10_MAP = 96'h000_000_000_000_000_000_000_000,
   parameter DATA11_MAP = 96'h000_000_000_000_000_000_000_000,
   parameter DATA12_MAP = 96'h000_000_000_000_000_000_000_000,
   parameter DATA13_MAP = 96'h000_000_000_000_000_000_000_000,
   parameter DATA14_MAP = 96'h000_000_000_000_000_000_000_000,
   parameter DATA15_MAP = 96'h000_000_000_000_000_000_000_000,
   parameter DATA16_MAP = 96'h000_000_000_000_000_000_000_000,
   parameter DATA17_MAP = 96'h000_000_000_000_000_000_000_000,
   parameter MASK0_MAP  = 108'h000_000_000_000_000_009_019_029_039,
   parameter MASK1_MAP  = 108'h000_000_000_000_000_000_000_000_000,

   parameter SLOT_0_CONFIG         = 8'b0000_0001,
                                     // Mapping of Ranks.
   parameter SLOT_1_CONFIG         = 8'b0000_0000,
                                     // Mapping of Ranks.
   parameter MEM_ADDR_ORDER
     = "BANK_ROW_COLUMN",

   //***************************************************************************
   // IODELAY and PHY related parameters
   //***************************************************************************
   parameter IODELAY_HP_MODE       = "ON",
                                     // to phy_top
   parameter IBUF_LPWR_MODE        = "OFF",
                                     // to phy_top
   parameter DATA_IO_IDLE_PWRDWN   = "ON",
                                     // # = "ON", "OFF"
   parameter BANK_TYPE             = "HP_IO",
                                     // # = "HP_IO", "HPL_IO", "HR_IO", "HRL_IO"
   parameter DATA_IO_PRIM_TYPE     = "HP_LP",
                                     // # = "HP_LP", "HR_LP", "DEFAULT"
   parameter CKE_ODT_AUX           = "FALSE",
   parameter USER_REFRESH          = "OFF",
   parameter WRLVL                 = "ON",
                                     // # = "ON" - DDR3 SDRAM
                                     //   = "OFF" - DDR2 SDRAM.
   parameter ORDERING              = "NORM",
                                     // # = "NORM", "STRICT", "RELAXED".
   parameter CALIB_ROW_ADD         = 16'h0000,
                                     // Calibration row address will be used for
                                     // calibration read and write operations
   parameter CALIB_COL_ADD         = 12'h000,
                                     // Calibration column address will be used for
                                     // calibration read and write operations
   parameter CALIB_BA_ADD          = 3'h0,
                                     // Calibration bank address will be used for
                                     // calibration read and write operations
   parameter TCQ                   = 100,
   parameter IODELAY_GRP           = "IODELAY_MIG",
                                     // It is associated to a set of IODELAYs with
                                     // an IDELAYCTRL that have same IODELAY CONTROLLER
                                     // clock frequency.
   parameter SYSCLK_TYPE           = "SINGLE_ENDED",
                                     // System clock type DIFFERENTIAL, SINGLE_ENDED,
                                     // NO_BUFFER
   parameter REFCLK_TYPE           = "NO_BUFFER",
                                     // Reference clock type DIFFERENTIAL, SINGLE_ENDED,
                                     // NO_BUFFER, USE_SYSTEM_CLOCK
      
   parameter CMD_PIPE_PLUS1        = "ON",
                                     // add pipeline stage between MC and PHY
   parameter DRAM_TYPE             = "DDR3",
   parameter CAL_WIDTH             = "HALF",
   parameter STARVE_LIMIT          = 2,
                                     // # = 2,3,4.

   //***************************************************************************
   // Referece clock frequency parameters
   //***************************************************************************
   parameter REFCLK_FREQ           = 200.0,
                                     // IODELAYCTRL reference clock frequency
   parameter DIFF_TERM_REFCLK      = "TRUE",
                                     // Differential Termination for idelay
                                     // reference clock input pins
   //***************************************************************************
   // System clock frequency parameters
   //***************************************************************************
   parameter tCK                   = 2000,
                                     // memory tCK paramter.
                                     // # = Clock Period in pS.
   parameter nCK_PER_CLK           = 4,
                                     // # of memory CKs per fabric CLK
   parameter DIFF_TERM_SYSCLK      = "TRUE",
                                     // Differential Termination for System
                                     // clock input pins

   
   //***************************************************************************
   // AXI4 Shim parameters
   //***************************************************************************
   parameter C_S_AXI_ID_WIDTH              = 4,
                                             // Width of all master and slave ID signals.
                                             // # = >= 1.
   parameter C_S_AXI_MEM_SIZE              = "2147483648",
                                     // Address Space required for this component
   parameter C_S_AXI_ADDR_WIDTH            = 32,
                                             // Width of S_AXI_AWADDR, S_AXI_ARADDR, M_AXI_AWADDR and
                                             // M_AXI_ARADDR for all SI/MI slots.
                                             // # = 32.
   parameter C_S_AXI_DATA_WIDTH            = 128,
                                             // Width of WDATA and RDATA on SI slot.
                                             // Must be <= APP_DATA_WIDTH.
                                             // # = 32, 64, 128, 256.
   parameter C_MC_nCK_PER_CLK              = 4,
                                             // Indicates whether to instatiate upsizer
                                             // Range: 0, 1
   parameter C_S_AXI_SUPPORTS_NARROW_BURST = 1,
                                             // Indicates whether to instatiate upsizer
                                             // Range: 0, 1
   parameter C_RD_WR_ARB_ALGORITHM          = "ROUND_ROBIN",
                                             // Indicates the Arbitration
                                             // Allowed values - "TDM", "ROUND_ROBIN",
                                             // "RD_PRI_REG", "RD_PRI_REG_STARVE_LIMIT"
                                             // "WRITE_PRIORITY", "WRITE_PRIORITY_REG"
   parameter C_S_AXI_REG_EN0               = 20'h00000,
                                             // C_S_AXI_REG_EN0[00] = Reserved
                                             // C_S_AXI_REG_EN0[04] = AW CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN0[05] =  W CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN0[06] =  B CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN0[07] =  R CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN0[08] = AW CHANNEL UPSIZER REGISTER SLICE
                                             // C_S_AXI_REG_EN0[09] =  W CHANNEL UPSIZER REGISTER SLICE
                                             // C_S_AXI_REG_EN0[10] = AR CHANNEL UPSIZER REGISTER SLICE
                                             // C_S_AXI_REG_EN0[11] =  R CHANNEL UPSIZER REGISTER SLICE
   parameter C_S_AXI_REG_EN1               = 20'h00000,
                                             // Instatiates register slices after the upsizer.
                                             // The type of register is specified for each channel
                                             // in a vector. 4 bits per channel are used.
                                             // C_S_AXI_REG_EN1[03:00] = AW CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN1[07:04] =  W CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN1[11:08] =  B CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN1[15:12] = AR CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN1[20:16] =  R CHANNEL REGISTER SLICE
                                             // Possible values for each channel are:
                                             //
                                             //   0 => BYPASS    = The channel is just wired through the
                                             //                    module.
                                             //   1 => FWD       = The master VALID and payload signals
                                             //                    are registrated.
                                             //   2 => REV       = The slave ready signal is registrated
                                             //   3 => FWD_REV   = Both FWD and REV
                                             //   4 => SLAVE_FWD = All slave side signals and master
                                             //                    VALID and payload are registrated.
                                             //   5 => SLAVE_RDY = All slave side signals and master
                                             //                    READY are registrated.
                                             //   6 => INPUTS    = Slave and Master side inputs are
                                             //                    registrated.
                                             //   7 => ADDRESS   = Optimized for address channel
   parameter C_S_AXI_CTRL_ADDR_WIDTH       = 32,
                                             // Width of AXI-4-Lite address bus
   parameter C_S_AXI_CTRL_DATA_WIDTH       = 32,
                                             // Width of AXI-4-Lite data buses
   parameter C_S_AXI_BASEADDR              = 32'h0000_0000,
                                             // Base address of AXI4 Memory Mapped bus.
   parameter C_ECC_ONOFF_RESET_VALUE       = 1,
                                             // Controls ECC on/off value at startup/reset
   parameter C_ECC_CE_COUNTER_WIDTH        = 8,
                                             // The external memory to controller clock ratio.

   //***************************************************************************
   // Debug parameters
   //***************************************************************************
   parameter DEBUG_PORT            = "ON",
                                     // # = "ON" Enable debug signals/controls.
                                     //   = "OFF" Disable debug signals/controls.

   //***************************************************************************
   // Temparature monitor parameter
   //***************************************************************************
   parameter TEMP_MON_CONTROL                          = "INTERNAL",
                                     // # = "INTERNAL", "EXTERNAL"
      
   parameter RST_ACT_LOW           = 1
                                     // =1 for active low reset,
                                     // =0 for active high.
   )
  (

   // Inouts
   inout [DQ_WIDTH-1:0]                         ddr3_dq,
   inout [DQS_WIDTH-1:0]                        ddr3_dqs_n,
   inout [DQS_WIDTH-1:0]                        ddr3_dqs_p,

   // Outputs
   output [ROW_WIDTH-1:0]                       ddr3_addr,
   output [BANK_WIDTH-1:0]                      ddr3_ba,
   output                                       ddr3_ras_n,
   output                                       ddr3_cas_n,
   output                                       ddr3_we_n,
   output                                       ddr3_reset_n,
   output [CK_WIDTH-1:0]                        ddr3_ck_p,
   output [CK_WIDTH-1:0]                        ddr3_ck_n,
   output [CKE_WIDTH-1:0]                       ddr3_cke,
   output [CS_WIDTH*nCS_PER_RANK-1:0]           ddr3_cs_n,
   output [DM_WIDTH-1:0]                        ddr3_dm,
   output [ODT_WIDTH-1:0]                       ddr3_odt,

   // Inputs
   // Single-ended system clock
   input                                        sys_clk_i,
   // Single-ended iodelayctrl clk (reference clock)
   // input                                        clk_ref_i, // IJB. Commented.
   // user interface signals
   output                                       ui_clk,
   output                                       ui_clk_sync_rst,
   
   input                                        aresetn,
   input                                        app_sr_req,
   output                                       app_sr_active,
   input                                        app_ref_req,
   output                                       app_ref_ack,
   input                                        app_zq_req,
   output                                       app_zq_ack,

   // Slave Interface Write Address Ports
   input  [C_S_AXI_ID_WIDTH-1:0]                s_axi_awid,
   input  [C_S_AXI_ADDR_WIDTH-1:0]              s_axi_awaddr,
   input  [7:0]                                 s_axi_awlen,
   input  [2:0]                                 s_axi_awsize,
   input  [1:0]                                 s_axi_awburst,
   input  [0:0]                                 s_axi_awlock,
   input  [3:0]                                 s_axi_awcache,
   input  [2:0]                                 s_axi_awprot,
   input  [3:0]                                 s_axi_awqos,
   input                                        s_axi_awvalid,
   output                                       s_axi_awready,
   // Slave Interface Write Data Ports
   input  [C_S_AXI_DATA_WIDTH-1:0]              s_axi_wdata,
   input  [C_S_AXI_DATA_WIDTH/8-1:0]            s_axi_wstrb,
   input                                        s_axi_wlast,
   input                                        s_axi_wvalid,
   output                                       s_axi_wready,
   // Slave Interface Write Response Ports
   input                                        s_axi_bready,
   output [C_S_AXI_ID_WIDTH-1:0]                s_axi_bid,
   output [1:0]                                 s_axi_bresp,
   output                                       s_axi_bvalid,
   // Slave Interface Read Address Ports
   input  [C_S_AXI_ID_WIDTH-1:0]                s_axi_arid,
   input  [C_S_AXI_ADDR_WIDTH-1:0]              s_axi_araddr,
   input  [7:0]                                 s_axi_arlen,
   input  [2:0]                                 s_axi_arsize,
   input  [1:0]                                 s_axi_arburst,
   input  [0:0]                                 s_axi_arlock,
   input  [3:0]                                 s_axi_arcache,
   input  [2:0]                                 s_axi_arprot,
   input  [3:0]                                 s_axi_arqos,
   input                                        s_axi_arvalid,
   output                                       s_axi_arready,
   // Slave Interface Read Data Ports
   input                                        s_axi_rready,
   output [C_S_AXI_ID_WIDTH-1:0]                s_axi_rid,
   output [C_S_AXI_DATA_WIDTH-1:0]              s_axi_rdata,
   output [1:0]                                 s_axi_rresp,
   output                                       s_axi_rlast,
   output                                       s_axi_rvalid,

   
   // debug signals
   output [255:0]                               ddr3_ila_wrpath,
   output [1023:0]                              ddr3_ila_rdpath,
   output [119:0]                               ddr3_ila_basic,
   input [35:0]                                 ddr3_vio_sync_out, // input from VIO

   input [DQS_CNT_WIDTH:0]                      dbg_byte_sel,
   input                                        dbg_sel_pi_incdec,
   input                                        dbg_pi_f_inc,
   input                                        dbg_pi_f_dec,
   input                                        dbg_sel_po_incdec,
   input                                        dbg_po_f_inc,
   input                                        dbg_po_f_stg23_sel,
   input                                        dbg_po_f_dec,
   output [5:0]                                 dbg_pi_counter_read_val,
   output [8:0]                                 dbg_po_counter_read_val,
   
   output                                       init_calib_complete,
   
      

   // System reset
   input                                        sys_rst
   );

  function integer clogb2 (input integer size);
    begin
      size = size - 1;
      for (clogb2=1; size>1; clogb2=clogb2+1)
        size = size >> 1;
    end
  endfunction // clogb2


  localparam BM_CNT_WIDTH = clogb2(nBANK_MACHS);
  localparam RANK_WIDTH = clogb2(RANKS);

  localparam APP_DATA_WIDTH        = 2 * nCK_PER_CLK * PAYLOAD_WIDTH;
  localparam APP_MASK_WIDTH        = APP_DATA_WIDTH / 8;
  localparam TEMP_MON_EN           = (SIMULATION == "FALSE") ? "ON" : "OFF";
                                                 // Enable or disable the temp monitor module
  localparam tTEMPSAMPLE           = 10000000;   // sample every 10 us
  localparam XADC_CLK_PERIOD       = 5000;       // Use 200 MHz IODELAYCTRL clock

      
      

  // Wire declarations
      
  wire [BM_CNT_WIDTH-1:0]           bank_mach_next;
  wire                              clk;
  wire                              clk_ref;
  wire 			            clk_ref_i;    // IJB. Added.
  wire                              idelay_ctrl_rdy;
  wire                              clk_ref_in;
  wire                              freq_refclk ;
  wire                              mem_refclk ;
  wire                              pll_lock ;
  wire                              sync_pulse;
  wire                              ref_dll_lock;
  wire                              rst_phaser_ref;

  wire                              rst;
  
  wire [2*nCK_PER_CLK-1:0]            app_ecc_multiple_err;
  wire                                ddr3_parity;
      // AXI CTRL port
  wire                              s_axi_ctrl_awvalid;
  wire                              s_axi_ctrl_awready;
  wire  [C_S_AXI_CTRL_ADDR_WIDTH-1:0] s_axi_ctrl_awaddr;
  // Slave Interface Write Data Ports
  wire                              s_axi_ctrl_wvalid;
  wire                              s_axi_ctrl_wready;
  wire  [C_S_AXI_CTRL_DATA_WIDTH-1:0] s_axi_ctrl_wdata;
  // Slave Interface Write Response Ports
  wire                              s_axi_ctrl_bvalid;
  wire                              s_axi_ctrl_bready;
  wire [1:0]                        s_axi_ctrl_bresp;
  // Slave Interface Read Address Ports
  wire                              s_axi_ctrl_arvalid;
  wire                              s_axi_ctrl_arready;
  wire  [C_S_AXI_CTRL_ADDR_WIDTH-1:0]  s_axi_ctrl_araddr;
  // Slave Interface Read Data Ports
  wire                              s_axi_ctrl_rvalid;
  wire                              s_axi_ctrl_rready;
  wire [C_S_AXI_CTRL_DATA_WIDTH-1:0]   s_axi_ctrl_rdata;
  wire [1:0]                        s_axi_ctrl_rresp;

  // Interrupt output
  wire                              interrupt;

  wire                              sys_clk_p;
  wire                              sys_clk_n;
  wire                              mmcm_clk;
  wire                              clk_ref_p;
  wire                              clk_ref_n;
  wire [11:0]                       device_temp;
  wire [11:0]                       device_temp_i;

  // Debug port signals
  wire                              dbg_idel_down_all;
  wire                              dbg_idel_down_cpt;
  wire                              dbg_idel_up_all;
  wire                              dbg_idel_up_cpt;
  wire                              dbg_sel_all_idel_cpt;
  wire [DQS_CNT_WIDTH-1:0]          dbg_sel_idel_cpt;
  wire [6*DQS_WIDTH*RANKS-1:0]      dbg_cpt_tap_cnt;
  wire [5*DQS_WIDTH*RANKS-1:0]      dbg_dq_idelay_tap_cnt;
  wire [255:0]                      dbg_calib_top;
  wire [6*DQS_WIDTH*RANKS-1:0]      dbg_cpt_first_edge_cnt;
  wire [6*DQS_WIDTH*RANKS-1:0]      dbg_cpt_second_edge_cnt;
  wire [6*RANKS-1:0]                dbg_rd_data_offset;
  wire [255:0]                      dbg_phy_rdlvl;
  wire [99:0]                       dbg_phy_wrcal;
  wire [6*DQS_WIDTH-1:0]            dbg_final_po_fine_tap_cnt;
  wire [3*DQS_WIDTH-1:0]            dbg_final_po_coarse_tap_cnt;
  wire [255:0]                      dbg_phy_wrlvl;
  wire [255:0]                      dbg_phy_init;
  wire [255:0]                      dbg_prbs_rdlvl;
  wire [255:0]                      dbg_dqs_found_cal;
  wire                              dbg_pi_phaselock_start;
  wire                              dbg_pi_phaselocked_done;
  wire                              dbg_pi_phaselock_err;
  wire                              dbg_pi_dqsfound_start;
  wire                              dbg_pi_dqsfound_done;
  wire                              dbg_pi_dqsfound_err;
  wire                              dbg_wrcal_start;
  wire                              dbg_wrcal_done;
  wire                              dbg_wrcal_err;
  wire [11:0]                       dbg_pi_dqs_found_lanes_phy4lanes;
  wire [11:0]                       dbg_pi_phase_locked_phy4lanes;
  wire                              dbg_oclkdelay_calib_start;
  wire                              dbg_oclkdelay_calib_done;
  wire [255:0]                      dbg_phy_oclkdelay_cal;
  wire [DRAM_WIDTH*16 -1:0]         dbg_oclkdelay_rd_data;
  wire [DQS_WIDTH-1:0]              dbg_rd_data_edge_detect;
  wire [2*nCK_PER_CLK*DQ_WIDTH-1:0] dbg_rddata;
  wire                              dbg_rddata_valid;
  wire [1:0]                        dbg_rdlvl_done;
  wire [1:0]                        dbg_rdlvl_err;
  wire [1:0]                        dbg_rdlvl_start;
  wire [6*DQS_WIDTH-1:0]            dbg_wrlvl_fine_tap_cnt;
  wire [3*DQS_WIDTH-1:0]            dbg_wrlvl_coarse_tap_cnt;
  wire [5:0]                        dbg_tap_cnt_during_wrlvl;
  wire                              dbg_wl_edge_detect_valid;
  wire                              dbg_wrlvl_done;
  wire                              dbg_wrlvl_err;
  wire                              dbg_wrlvl_start;
  reg [63:0]                        dbg_rddata_r;
  reg                               dbg_rddata_valid_r;
  wire [53:0]                       ocal_tap_cnt;
  wire [3:0]                        dbg_dqs;
  wire [8:0]                        dbg_bit;
  wire [8:0]                        rd_data_edge_detect_r;
  wire [53:0]                       wl_po_fine_cnt;
  wire [26:0]                       wl_po_coarse_cnt;
  wire [6*RANKS-1:0]                dbg_calib_rd_data_offset_1;
  wire [6*RANKS-1:0]                dbg_calib_rd_data_offset_2;
  wire [5:0]                        dbg_data_offset;
  wire [5:0]                        dbg_data_offset_1;
  wire [5:0]                        dbg_data_offset_2;
      

//***************************************************************************



  assign ui_clk = clk;
  assign ui_clk_sync_rst = rst;
  
  assign sys_clk_p = 1'b0;
  assign sys_clk_n = 1'b0;
  assign clk_ref_p = 1'b0;
  assign clk_ref_n = 1'b0;
      

  generate
    if (REFCLK_TYPE == "USE_SYSTEM_CLOCK")
      assign clk_ref_in = mmcm_clk;
    else
      assign clk_ref_in = clk_ref_i;
  endgenerate

  mig_7series_v1_8_iodelay_ctrl #
    (
     .TCQ              (TCQ),
     .IODELAY_GRP      (IODELAY_GRP),
     .REFCLK_TYPE      (REFCLK_TYPE),
     .SYSCLK_TYPE      (SYSCLK_TYPE),
     .RST_ACT_LOW      (RST_ACT_LOW),
     .DIFF_TERM_REFCLK (DIFF_TERM_REFCLK)
     )
    u_mig_7series_v1_8_iodelay_ctrl
      (
       // Outputs
       .iodelay_ctrl_rdy (iodelay_ctrl_rdy),
       .clk_ref          (clk_ref),
       // Inputs
       .clk_ref_p        (clk_ref_p),
       .clk_ref_n        (clk_ref_n),
       .clk_ref_i        (clk_ref_in),
       .sys_rst          (sys_rst),
       .pll_locked       (pll_locked) //IJB.
       );
  mig_7series_v1_8_clk_ibuf #
    (
     .SYSCLK_TYPE      (SYSCLK_TYPE),
     .DIFF_TERM_SYSCLK (DIFF_TERM_SYSCLK)
     )
    u_ddr3_clk_ibuf
      (
       .sys_clk_p        (sys_clk_p),
       .sys_clk_n        (sys_clk_n),
       .sys_clk_i        (sys_clk_i),
       .mmcm_clk         (mmcm_clk)
       );
  // Temperature monitoring logic

  generate
    if (TEMP_MON_EN == "ON") begin: temp_mon_enabled

      mig_7series_v1_8_tempmon #
        (
         .TCQ              (TCQ),
         .TEMP_MON_CONTROL (TEMP_MON_CONTROL),
         .XADC_CLK_PERIOD  (XADC_CLK_PERIOD),
         .tTEMPSAMPLE      (tTEMPSAMPLE)
         )
        u_mig_7series_v1_8_tempmon
          (
           .clk            (clk),
           .xadc_clk       (clk_ref),
           .rst            (rst),
           .device_temp_i  (device_temp_i),
           .device_temp    (device_temp)
          );
    end else begin: temp_mon_disabled

      assign device_temp = 'b0;

    end
  endgenerate
         

  mig_7series_v1_8_infrastructure #
    (
     .TCQ                (TCQ),
     .nCK_PER_CLK        (nCK_PER_CLK),
     .CLKIN_PERIOD       (CLKIN_PERIOD),
     .SYSCLK_TYPE        (SYSCLK_TYPE),
     .CLKFBOUT_MULT      (CLKFBOUT_MULT),
     .DIVCLK_DIVIDE      (DIVCLK_DIVIDE),
     .CLKOUT0_PHASE      (CLKOUT0_PHASE),
     .CLKOUT0_DIVIDE     (CLKOUT0_DIVIDE),
     .CLKOUT1_DIVIDE     (CLKOUT1_DIVIDE),
     .CLKOUT2_DIVIDE     (CLKOUT2_DIVIDE),
     .CLKOUT3_DIVIDE     (CLKOUT3_DIVIDE),
     .CLKOUT5_DIVIDE     (CLKOUT5_DIVIDE), // IJB. Added.
     .RST_ACT_LOW        (RST_ACT_LOW)
     )
    u_ddr3_infrastructure
      (
       // Outputs
       .rstdiv0          (rst),
       .clk              (clk),
       .mem_refclk       (mem_refclk),
       .freq_refclk      (freq_refclk),
       .sync_pulse       (sync_pulse),
       .auxout_clk       (),
       .clk_ref_i        (clk_ref_i), // IJB
       .pll_locked       (pll_locked),
       .rst_phaser_ref   (rst_phaser_ref),
       // Inputs
       .mmcm_clk         (mmcm_clk),
       .sys_rst          (sys_rst),
       .iodelay_ctrl_rdy (iodelay_ctrl_rdy),
       .ref_dll_lock     (ref_dll_lock)
       );

  mig_7series_v1_8_memc_ui_top_axi #
    (
     .TCQ                              (TCQ),
     .ADDR_CMD_MODE                    (ADDR_CMD_MODE),
     .AL                               (AL),
     .PAYLOAD_WIDTH                    (PAYLOAD_WIDTH),
     .BANK_WIDTH                       (BANK_WIDTH),
     .BM_CNT_WIDTH                     (BM_CNT_WIDTH),
     .BURST_MODE                       (BURST_MODE),
     .BURST_TYPE                       (BURST_TYPE),
     .CA_MIRROR                        (CA_MIRROR),
     .CK_WIDTH                         (CK_WIDTH),
     .COL_WIDTH                        (COL_WIDTH),
     .CMD_PIPE_PLUS1                   (CMD_PIPE_PLUS1),
     .CS_WIDTH                         (CS_WIDTH),
     .nCS_PER_RANK                     (nCS_PER_RANK),
     .CKE_WIDTH                        (CKE_WIDTH),
     .DATA_WIDTH                       (DATA_WIDTH),
     .DATA_BUF_ADDR_WIDTH              (DATA_BUF_ADDR_WIDTH),
     .DM_WIDTH                         (DM_WIDTH),
     .DQ_CNT_WIDTH                     (DQ_CNT_WIDTH),
     .DQ_WIDTH                         (DQ_WIDTH),
     .DQS_CNT_WIDTH                    (DQS_CNT_WIDTH),
     .DQS_WIDTH                        (DQS_WIDTH),
     .DRAM_TYPE                        (DRAM_TYPE),
     .DRAM_WIDTH                       (DRAM_WIDTH),
     .ECC                              (ECC),
     .ECC_WIDTH                        (ECC_WIDTH),
     .ECC_TEST                         (ECC_TEST),
     .MC_ERR_ADDR_WIDTH                (MC_ERR_ADDR_WIDTH),
     .REFCLK_FREQ                      (REFCLK_FREQ),
     .nAL                              (nAL),
     .nBANK_MACHS                      (nBANK_MACHS),
     .CKE_ODT_AUX                      (CKE_ODT_AUX),
     .nCK_PER_CLK                      (nCK_PER_CLK),
     .ORDERING                         (ORDERING),
     .OUTPUT_DRV                       (OUTPUT_DRV),
     .IBUF_LPWR_MODE                   (IBUF_LPWR_MODE),
     .IODELAY_HP_MODE                  (IODELAY_HP_MODE),
     .DATA_IO_IDLE_PWRDWN              (DATA_IO_IDLE_PWRDWN),
     .BANK_TYPE                        (BANK_TYPE),
     .DATA_IO_PRIM_TYPE                (DATA_IO_PRIM_TYPE),
     .IODELAY_GRP                      (IODELAY_GRP),
     .REG_CTRL                         (REG_CTRL),
     .RTT_NOM                          (RTT_NOM),
     .RTT_WR                           (RTT_WR),
     .CL                               (CL),
     .CWL                              (CWL),
     .tCK                              (tCK),
     .tCKE                             (tCKE),
     .tFAW                             (tFAW),
     .tPRDI                            (tPRDI),
     .tRAS                             (tRAS),
     .tRCD                             (tRCD),
     .tREFI                            (tREFI),
     .tRFC                             (tRFC),
     .tRP                              (tRP),
     .tRRD                             (tRRD),
     .tRTP                             (tRTP),
     .tWTR                             (tWTR),
     .tZQI                             (tZQI),
     .tZQCS                            (tZQCS),
     .USER_REFRESH                     (USER_REFRESH),
     .TEMP_MON_EN                      (TEMP_MON_EN),
     .WRLVL                            (WRLVL),
     .DEBUG_PORT                       (DEBUG_PORT),
     .CAL_WIDTH                        (CAL_WIDTH),
     .RANK_WIDTH                       (RANK_WIDTH),
     .RANKS                            (RANKS),
     .ODT_WIDTH                        (ODT_WIDTH),
     .ROW_WIDTH                        (ROW_WIDTH),
     .ADDR_WIDTH                       (ADDR_WIDTH),
     .APP_DATA_WIDTH                   (APP_DATA_WIDTH),
     .APP_MASK_WIDTH                   (APP_MASK_WIDTH),
     .SIM_BYPASS_INIT_CAL              (SIM_BYPASS_INIT_CAL),
     .BYTE_LANES_B0                    (BYTE_LANES_B0),
     .BYTE_LANES_B1                    (BYTE_LANES_B1),
     .BYTE_LANES_B2                    (BYTE_LANES_B2),
     .BYTE_LANES_B3                    (BYTE_LANES_B3),
     .BYTE_LANES_B4                    (BYTE_LANES_B4),
     .DATA_CTL_B0                      (DATA_CTL_B0),
     .DATA_CTL_B1                      (DATA_CTL_B1),
     .DATA_CTL_B2                      (DATA_CTL_B2),
     .DATA_CTL_B3                      (DATA_CTL_B3),
     .DATA_CTL_B4                      (DATA_CTL_B4),
     .PHY_0_BITLANES                   (PHY_0_BITLANES),
     .PHY_1_BITLANES                   (PHY_1_BITLANES),
     .PHY_2_BITLANES                   (PHY_2_BITLANES),
     .CK_BYTE_MAP                      (CK_BYTE_MAP),
     .ADDR_MAP                         (ADDR_MAP),
     .BANK_MAP                         (BANK_MAP),
     .CAS_MAP                          (CAS_MAP),
     .CKE_ODT_BYTE_MAP                 (CKE_ODT_BYTE_MAP),
     .CKE_MAP                          (CKE_MAP),
     .ODT_MAP                          (ODT_MAP),
     .CS_MAP                           (CS_MAP),
     .PARITY_MAP                       (PARITY_MAP),
     .RAS_MAP                          (RAS_MAP),
     .WE_MAP                           (WE_MAP),
     .DQS_BYTE_MAP                     (DQS_BYTE_MAP),
     .DATA0_MAP                        (DATA0_MAP),
     .DATA1_MAP                        (DATA1_MAP),
     .DATA2_MAP                        (DATA2_MAP),
     .DATA3_MAP                        (DATA3_MAP),
     .DATA4_MAP                        (DATA4_MAP),
     .DATA5_MAP                        (DATA5_MAP),
     .DATA6_MAP                        (DATA6_MAP),
     .DATA7_MAP                        (DATA7_MAP),
     .DATA8_MAP                        (DATA8_MAP),
     .DATA9_MAP                        (DATA9_MAP),
     .DATA10_MAP                       (DATA10_MAP),
     .DATA11_MAP                       (DATA11_MAP),
     .DATA12_MAP                       (DATA12_MAP),
     .DATA13_MAP                       (DATA13_MAP),
     .DATA14_MAP                       (DATA14_MAP),
     .DATA15_MAP                       (DATA15_MAP),
     .DATA16_MAP                       (DATA16_MAP),
     .DATA17_MAP                       (DATA17_MAP),
     .MASK0_MAP                        (MASK0_MAP),
     .MASK1_MAP                        (MASK1_MAP),
     .CALIB_ROW_ADD                    (CALIB_ROW_ADD),
     .CALIB_COL_ADD                    (CALIB_COL_ADD),
     .CALIB_BA_ADD                     (CALIB_BA_ADD),
     .SLOT_0_CONFIG                    (SLOT_0_CONFIG),
     .SLOT_1_CONFIG                    (SLOT_1_CONFIG),
     .MEM_ADDR_ORDER                   (MEM_ADDR_ORDER),
     .STARVE_LIMIT                     (STARVE_LIMIT),
     .C_S_AXI_ID_WIDTH                 (C_S_AXI_ID_WIDTH),
     .C_S_AXI_ADDR_WIDTH               (C_S_AXI_ADDR_WIDTH),
     .C_S_AXI_DATA_WIDTH               (C_S_AXI_DATA_WIDTH),
     .C_S_AXI_SUPPORTS_NARROW_BURST    (C_S_AXI_SUPPORTS_NARROW_BURST),
     .C_RD_WR_ARB_ALGORITHM            (C_RD_WR_ARB_ALGORITHM),
     .C_S_AXI_REG_EN0                  (C_S_AXI_REG_EN0),
     .C_S_AXI_REG_EN1                  (C_S_AXI_REG_EN1),
     .C_S_AXI_CTRL_ADDR_WIDTH          (C_S_AXI_CTRL_ADDR_WIDTH),
     .C_S_AXI_CTRL_DATA_WIDTH          (C_S_AXI_CTRL_DATA_WIDTH),
     .C_S_AXI_BASEADDR                 (C_S_AXI_BASEADDR),
     .C_ECC_ONOFF_RESET_VALUE          (C_ECC_ONOFF_RESET_VALUE),
     .C_ECC_CE_COUNTER_WIDTH           (C_ECC_CE_COUNTER_WIDTH),
     .USE_CS_PORT                      (USE_CS_PORT),
     .USE_DM_PORT                      (USE_DM_PORT),
     .USE_ODT_PORT                     (USE_ODT_PORT),
     .MASTER_PHY_CTL                   (PHY_CONTROL_MASTER_BANK)
     )
    u_mig_7series_v1_8_memc_ui_top_axi
      (
       .clk                              (clk),
       .clk_ref                          (clk_ref),
       .mem_refclk                       (mem_refclk), //memory clock
       .freq_refclk                      (freq_refclk),
       .pll_lock                         (pll_locked),
       .sync_pulse                       (sync_pulse),
       .rst                              (rst),
       .rst_phaser_ref                   (rst_phaser_ref),
       .ref_dll_lock                     (ref_dll_lock),

// Memory interface ports
       .ddr_dq                           (ddr3_dq),
       .ddr_dqs_n                        (ddr3_dqs_n),
       .ddr_dqs                          (ddr3_dqs_p),
       .ddr_addr                         (ddr3_addr),
       .ddr_ba                           (ddr3_ba),
       .ddr_cas_n                        (ddr3_cas_n),
       .ddr_ck_n                         (ddr3_ck_n),
       .ddr_ck                           (ddr3_ck_p),
       .ddr_cke                          (ddr3_cke),
       .ddr_cs_n                         (ddr3_cs_n),
       .ddr_dm                           (ddr3_dm),
       .ddr_odt                          (ddr3_odt),
       .ddr_ras_n                        (ddr3_ras_n),
       .ddr_reset_n                      (ddr3_reset_n),
       .ddr_parity                       (ddr3_parity),
       .ddr_we_n                         (ddr3_we_n),
       .bank_mach_next                   (bank_mach_next),

// Application interface ports
       .app_ecc_multiple_err_o           (),

       .device_temp                      (device_temp),

// Debug logic ports
       .dbg_idel_up_all                  (dbg_idel_up_all),
       .dbg_idel_down_all                (dbg_idel_down_all),
       .dbg_idel_up_cpt                  (dbg_idel_up_cpt),
       .dbg_idel_down_cpt                (dbg_idel_down_cpt),
       .dbg_sel_idel_cpt                 (dbg_sel_idel_cpt),
       .dbg_sel_all_idel_cpt             (dbg_sel_all_idel_cpt),
       .dbg_sel_pi_incdec                (dbg_sel_pi_incdec),
       .dbg_sel_po_incdec                (dbg_sel_po_incdec),
       .dbg_byte_sel                     (dbg_byte_sel),
       .dbg_pi_f_inc                     (dbg_pi_f_inc),
       .dbg_pi_f_dec                     (dbg_pi_f_dec),
       .dbg_po_f_inc                     (dbg_po_f_inc),
       .dbg_po_f_stg23_sel               (dbg_po_f_stg23_sel),
       .dbg_po_f_dec                     (dbg_po_f_dec),
       .dbg_cpt_tap_cnt                  (dbg_cpt_tap_cnt),
       .dbg_dq_idelay_tap_cnt            (dbg_dq_idelay_tap_cnt),
       .dbg_calib_top                    (dbg_calib_top),
       .dbg_cpt_first_edge_cnt           (dbg_cpt_first_edge_cnt),
       .dbg_cpt_second_edge_cnt          (dbg_cpt_second_edge_cnt),
       .dbg_rd_data_offset               (dbg_rd_data_offset),
       .dbg_phy_rdlvl                    (dbg_phy_rdlvl),
       .dbg_phy_wrcal                    (dbg_phy_wrcal),
       .dbg_final_po_fine_tap_cnt        (dbg_final_po_fine_tap_cnt),
       .dbg_final_po_coarse_tap_cnt      (dbg_final_po_coarse_tap_cnt),
       .dbg_rd_data_edge_detect          (dbg_rd_data_edge_detect),
       .dbg_rddata                       (dbg_rddata),
       .dbg_rddata_valid                 (dbg_rddata_valid),
       .dbg_rdlvl_done                   (dbg_rdlvl_done),
       .dbg_rdlvl_err                    (dbg_rdlvl_err),
       .dbg_rdlvl_start                  (dbg_rdlvl_start),
       .dbg_wrlvl_fine_tap_cnt           (dbg_wrlvl_fine_tap_cnt),
       .dbg_wrlvl_coarse_tap_cnt         (dbg_wrlvl_coarse_tap_cnt),
       .dbg_tap_cnt_during_wrlvl         (dbg_tap_cnt_during_wrlvl),
       .dbg_wl_edge_detect_valid         (dbg_wl_edge_detect_valid),
       .dbg_wrlvl_done                   (dbg_wrlvl_done),
       .dbg_wrlvl_err                    (dbg_wrlvl_err),
       .dbg_wrlvl_start                  (dbg_wrlvl_start),
       .dbg_phy_wrlvl                    (dbg_phy_wrlvl),
       .dbg_phy_init                     (dbg_phy_init),
       .dbg_prbs_rdlvl                   (dbg_prbs_rdlvl),
       .dbg_pi_counter_read_val          (dbg_pi_counter_read_val),
       .dbg_po_counter_read_val          (dbg_po_counter_read_val),
       .dbg_pi_phaselock_start           (dbg_pi_phaselock_start),
       .dbg_pi_phaselocked_done          (dbg_pi_phaselocked_done),
       .dbg_pi_phaselock_err             (dbg_pi_phaselock_err),
       .dbg_pi_phase_locked_phy4lanes    (dbg_pi_phase_locked_phy4lanes),
       .dbg_pi_dqsfound_start            (dbg_pi_dqsfound_start),
       .dbg_pi_dqsfound_done             (dbg_pi_dqsfound_done),
       .dbg_pi_dqsfound_err              (dbg_pi_dqsfound_err),
       .dbg_pi_dqs_found_lanes_phy4lanes (dbg_pi_dqs_found_lanes_phy4lanes),
       .dbg_calib_rd_data_offset_1       (dbg_calib_rd_data_offset_1),
       .dbg_calib_rd_data_offset_2       (dbg_calib_rd_data_offset_2),
       .dbg_data_offset                  (dbg_data_offset),
       .dbg_data_offset_1                (dbg_data_offset_1),
       .dbg_data_offset_2                (dbg_data_offset_2),
       .dbg_wrcal_start                  (dbg_wrcal_start),
       .dbg_wrcal_done                   (dbg_wrcal_done),
       .dbg_wrcal_err                    (dbg_wrcal_err),
       .dbg_phy_oclkdelay_cal            (dbg_phy_oclkdelay_cal),
       .dbg_oclkdelay_rd_data            (dbg_oclkdelay_rd_data),
       .dbg_oclkdelay_calib_start        (dbg_oclkdelay_calib_start),
       .dbg_oclkdelay_calib_done         (dbg_oclkdelay_calib_done),
       .dbg_dqs_found_cal                (dbg_dqs_found_cal),  
       .aresetn                          (aresetn),
       .app_sr_req                       (app_sr_req),
       .app_sr_active                    (app_sr_active),
       .app_ref_req                      (app_ref_req),
       .app_ref_ack                      (app_ref_ack),
       .app_zq_req                       (app_zq_req),
       .app_zq_ack                       (app_zq_ack),

       // Slave Interface Write Address Ports
       .s_axi_awid                       (s_axi_awid),
       .s_axi_awaddr                     (s_axi_awaddr),
       .s_axi_awlen                      (s_axi_awlen),
       .s_axi_awsize                     (s_axi_awsize),
       .s_axi_awburst                    (s_axi_awburst),
       .s_axi_awlock                     (s_axi_awlock),
       .s_axi_awcache                    (s_axi_awcache),
       .s_axi_awprot                     (s_axi_awprot),
       .s_axi_awqos                      (s_axi_awqos),
       .s_axi_awvalid                    (s_axi_awvalid),
       .s_axi_awready                    (s_axi_awready),
       // Slave Interface Write Data Ports
       .s_axi_wdata                      (s_axi_wdata),
       .s_axi_wstrb                      (s_axi_wstrb),
       .s_axi_wlast                      (s_axi_wlast),
       .s_axi_wvalid                     (s_axi_wvalid),
       .s_axi_wready                     (s_axi_wready),
       // Slave Interface Write Response Ports
       .s_axi_bid                        (s_axi_bid),
       .s_axi_bresp                      (s_axi_bresp),
       .s_axi_bvalid                     (s_axi_bvalid),
       .s_axi_bready                     (s_axi_bready),
       // Slave Interface Read Address Ports
       .s_axi_arid                       (s_axi_arid),
       .s_axi_araddr                     (s_axi_araddr),
       .s_axi_arlen                      (s_axi_arlen),
       .s_axi_arsize                     (s_axi_arsize),
       .s_axi_arburst                    (s_axi_arburst),
       .s_axi_arlock                     (s_axi_arlock),
       .s_axi_arcache                    (s_axi_arcache),
       .s_axi_arprot                     (s_axi_arprot),
       .s_axi_arqos                      (s_axi_arqos),
       .s_axi_arvalid                    (s_axi_arvalid),
       .s_axi_arready                    (s_axi_arready),
       // Slave Interface Read Data Ports
       .s_axi_rid                        (s_axi_rid),
       .s_axi_rdata                      (s_axi_rdata),
       .s_axi_rresp                      (s_axi_rresp),
       .s_axi_rlast                      (s_axi_rlast),
       .s_axi_rvalid                     (s_axi_rvalid),
       .s_axi_rready                     (s_axi_rready),
       // AXI CTRL port
       .s_axi_ctrl_awvalid               (s_axi_ctrl_awvalid),
       .s_axi_ctrl_awready               (s_axi_ctrl_awready),
       .s_axi_ctrl_awaddr                (s_axi_ctrl_awaddr),
       // Slave Interface Write Data Ports
       .s_axi_ctrl_wvalid                (s_axi_ctrl_wvalid),
       .s_axi_ctrl_wready                (s_axi_ctrl_wready),
       .s_axi_ctrl_wdata                 (s_axi_ctrl_wdata),
       // Slave Interface Write Response Ports
       .s_axi_ctrl_bvalid                (s_axi_ctrl_bvalid),
       .s_axi_ctrl_bready                (s_axi_ctrl_bready),
       .s_axi_ctrl_bresp                 (s_axi_ctrl_bresp),
       // Slave Interface Read Address Ports
       .s_axi_ctrl_arvalid               (s_axi_ctrl_arvalid),
       .s_axi_ctrl_arready               (s_axi_ctrl_arready),
       .s_axi_ctrl_araddr                (s_axi_ctrl_araddr),
       // Slave Interface Read Data Ports
       .s_axi_ctrl_rvalid                (s_axi_ctrl_rvalid),
       .s_axi_ctrl_rready                (s_axi_ctrl_rready),
       .s_axi_ctrl_rdata                 (s_axi_ctrl_rdata),
       .s_axi_ctrl_rresp                 (s_axi_ctrl_rresp),
       // Interrupt output
       .interrupt                        (interrupt),
       .init_calib_complete              (init_calib_complete)
       );

      




  //*****************************************************************
  // PHY Debug Port Example:
  //  * This provides a limited Chipscope Interface for observing
  //    PHY signals (outputs of read and write timing calibration,
  //    general status, synchronized read data), and a mechanism for
  //    dynamically changing some of the IODELAY elements used to
  //    adjust timing in the read data path.
  //  * This logic supports up to the first 72 DQ and 9 DQS groups.
  //    Larger interfaces will require manual modification and
  //    additional Chipscope blocks to support monitoring of the
  //    additional DQS groups. Smaller interfaces can also obviously
  //    use smaller ILA cores (user will have to generate these
  //    themselves) if resources or timing is a concern
  //*****************************************************************

   // Connect these to VIO if changing output IODELAY taps desired
   // IODELAY taps desired
   assign dbg_idel_down_all    = 1'b0;
   assign dbg_idel_down_cpt    = 1'b0;
   assign dbg_idel_up_all      = 1'b0;
   assign dbg_idel_up_cpt      = 1'b0;
   assign dbg_sel_all_idel_cpt = 1'b0;
   assign dbg_sel_idel_cpt     = 'b0;

   //*******************************************************
   //     - ILA for monitoring basic set of phy signals,
   //       and synchronized read data
   //*******************************************************
   assign dbg_bit = ddr3_vio_sync_out[8:0];
   assign dbg_dqs = ddr3_vio_sync_out[12:9];

   assign ddr3_ila_basic[0] = init_calib_complete;

   assign ddr3_ila_basic[1] = dbg_wrlvl_start;
   assign ddr3_ila_basic[2] = dbg_wrlvl_done;
   assign ddr3_ila_basic[3] = dbg_wrlvl_err;

   assign ddr3_ila_basic[4] = dbg_pi_phaselock_start;
   assign ddr3_ila_basic[5] = dbg_pi_phaselocked_done;
   assign ddr3_ila_basic[6] = dbg_pi_phaselock_err;

   assign ddr3_ila_basic[7] = dbg_pi_dqsfound_start;
   assign ddr3_ila_basic[8] = dbg_pi_dqsfound_done;
   assign ddr3_ila_basic[9] = dbg_pi_dqsfound_err;

   assign ddr3_ila_basic[11:10] = dbg_rdlvl_start;
   assign ddr3_ila_basic[13:12] = dbg_rdlvl_done;
   assign ddr3_ila_basic[15:14] = dbg_rdlvl_err;

   assign ddr3_ila_basic[16] = dbg_oclkdelay_calib_start;
   assign ddr3_ila_basic[17] = dbg_oclkdelay_calib_done;
   assign ddr3_ila_basic[18] = 1'b0;

   assign ddr3_ila_basic[19] = dbg_wrcal_start;
   assign ddr3_ila_basic[20] = dbg_wrcal_done;
   assign ddr3_ila_basic[21] = dbg_wrcal_err;

   assign ddr3_ila_basic[27:22]  = dbg_phy_init[5:0];
   assign ddr3_ila_basic[28]     = dbg_rddata_valid_r;
   assign ddr3_ila_basic[29+:64] = dbg_rddata_r;

   // additional signals required for debug
   assign ddr3_ila_basic[93]     = dbg_dqs_found_cal[14]; // fine_adjust_done_r
   assign ddr3_ila_basic[119:94] = 'b0;

   always @(posedge clk) begin
     dbg_rddata_valid_r <= dbg_rddata_valid;
   end

   always @(posedge clk) begin
     dbg_rddata_r[7:0]   <= #TCQ dbg_rddata[(8*dbg_dqs)+:8];
     dbg_rddata_r[15:8]  <= #TCQ dbg_rddata[(8*dbg_dqs)+DQ_WIDTH+:8];
     dbg_rddata_r[23:16] <= #TCQ dbg_rddata[(8*dbg_dqs)+2*DQ_WIDTH+:8];
     dbg_rddata_r[31:24] <= #TCQ dbg_rddata[(8*dbg_dqs)+3*DQ_WIDTH+:8];
     dbg_rddata_r[39:32] <= #TCQ (nCK_PER_CLK == 2 && DQ_WIDTH == 8) ? 8'h0 :
                                  dbg_rddata[(8*dbg_dqs)+4*DQ_WIDTH+:8];
     dbg_rddata_r[47:40] <= #TCQ (nCK_PER_CLK == 2 && DQ_WIDTH == 8) ? 8'h0 :
                                  dbg_rddata[(8*dbg_dqs)+5*DQ_WIDTH+:8];
     dbg_rddata_r[55:48] <= #TCQ (nCK_PER_CLK == 2 && DQ_WIDTH == 8) ? 8'h0 :
                                  dbg_rddata[(8*dbg_dqs)+6*DQ_WIDTH+:8];
     dbg_rddata_r[63:56] <= #TCQ (nCK_PER_CLK == 2 && DQ_WIDTH == 8) ? 8'h0 :
                                  dbg_rddata[(8*dbg_dqs)+7*DQ_WIDTH+:8];
   end

   //*******************************************************
   //     - ILA for monitoring write path signals,
   //       and synchronized read data
   //*******************************************************

   assign rd_data_edge_detect_r  = dbg_phy_wrlvl[66+:9];
   assign wl_po_fine_cnt         = dbg_phy_wrlvl[75+:54];
   assign wl_po_coarse_cnt       = dbg_phy_wrlvl[129+:27];

   // write-leveling calibration debug data
   assign ddr3_ila_wrpath[0+:5]  = dbg_phy_wrlvl [27+:5]; // wl_state_r
   assign ddr3_ila_wrpath[6+:4]  = dbg_phy_wrlvl [32+:4]; // dqs_cnt_r
   assign ddr3_ila_wrpath[10]    = dbg_phy_wrlvl[59]; // wl_edge_detect_valid_r
   assign ddr3_ila_wrpath[11]    = rd_data_edge_detect_r[dbg_dqs];
   assign ddr3_ila_wrpath[12+:6] = wl_po_fine_cnt[(dbg_dqs*6)+:6];
   assign ddr3_ila_wrpath[18+:3] = wl_po_coarse_cnt[(dbg_dqs*3)+:3];
   assign ddr3_ila_wrpath[21+:11] = 'b0; // reserved

   // oclk delay calibration debug data
   assign ocal_tap_cnt           = dbg_phy_oclkdelay_cal [53:0];
   assign ddr3_ila_wrpath[32+:6] = ocal_tap_cnt[(dbg_dqs*6)+:6];
   assign ddr3_ila_wrpath[38]    = dbg_phy_oclkdelay_cal[58]; // ocal_edge1_found
   assign ddr3_ila_wrpath[39]    = dbg_phy_oclkdelay_cal[59]; // ocal_edge2_found
   assign ddr3_ila_wrpath[40+:6] = dbg_phy_oclkdelay_cal[65:60]; // ocal_edge1_taps
   assign ddr3_ila_wrpath[46+:6] = dbg_phy_oclkdelay_cal[71:66]; // ocal_edge2_taps
   assign ddr3_ila_wrpath[52+:5] = dbg_phy_oclkdelay_cal[76:72]; // ocal_state_r
   assign ddr3_ila_wrpath[57+:6] = dbg_phy_oclkdelay_cal[219+:6]; // stg2_tap_cnt
   assign ddr3_ila_wrpath[63]    = 'b0; // reserved

   // write calibration stage debug signals
   assign ddr3_ila_wrpath[64]    = dbg_phy_wrcal[0]; // pat_data_match_r
   assign ddr3_ila_wrpath[65]    = dbg_phy_wrcal[8]; // pat_data_match_valid_r
   assign ddr3_ila_wrpath[66+:4] = dbg_phy_wrcal[13+:DQS_CNT_WIDTH]; // wrcal_dqs_cnt_r
   assign ddr3_ila_wrpath[70+:5] = dbg_phy_wrcal[5:1]; // cal2_state_r
   assign ddr3_ila_wrpath[75+:5] = dbg_phy_wrcal[70:66]; // not_empty_wait_cnt
   assign ddr3_ila_wrpath[80]    = dbg_phy_wrcal[71]; // early1_data
   assign ddr3_ila_wrpath[81]    = dbg_phy_wrcal[72]; // early2_data
   assign ddr3_ila_wrpath[87:82] = 'b0; // reserved

   assign ddr3_ila_wrpath[91:88]  = dbg_phy_oclkdelay_cal[57:54];
   assign ddr3_ila_wrpath[95:92]  = 'b0; //reserved

   assign ddr3_ila_wrpath[96+:54] = dbg_phy_wrlvl[75+:54];
   assign ddr3_ila_wrpath[150+:27]= dbg_phy_wrlvl[129+:27];

   assign ddr3_ila_wrpath[177+:8] = dbg_phy_rdlvl[177:170]; // mux_rd_rise0_r
   assign ddr3_ila_wrpath[185+:8] = dbg_phy_rdlvl[185:178]; // mux_rd_fall0_r
   assign ddr3_ila_wrpath[193+:8] = dbg_phy_rdlvl[193:186]; // mux_rd_rise1_r
   assign ddr3_ila_wrpath[201+:8] = dbg_phy_rdlvl[201:194]; // mux_rd_fall1_r
   assign ddr3_ila_wrpath[209+:8] = dbg_phy_rdlvl[209:202]; // mux_rd_rise2_r
   assign ddr3_ila_wrpath[217+:8] = dbg_phy_rdlvl[217:210]; // mux_rd_fall2_r
   assign ddr3_ila_wrpath[225+:8] = dbg_phy_rdlvl[225:218]; // mux_rd_rise3_r
   assign ddr3_ila_wrpath[233+:8] = dbg_phy_rdlvl[233:226]; // mux_rd_fall3_r

   assign ddr3_ila_wrpath[255:241]= 'b0;//reserved


   //*******************************************************
   //     - ILA for monitoring read path signals,
   //       and synchronized read data
   //*******************************************************

   // PHASER_IN debug signals
   assign ddr3_ila_rdpath[0+:12]  = dbg_pi_phase_locked_phy4lanes;
   assign ddr3_ila_rdpath[12+:12] = dbg_pi_dqs_found_lanes_phy4lanes;
   assign ddr3_ila_rdpath[24+:12] = dbg_rd_data_offset;
   assign ddr3_ila_rdpath[39:36]  = 'b0; //reserved

   // read-leveling calibration debug data
   assign ddr3_ila_rdpath[40+:6] = dbg_phy_rdlvl[14:9]; // cal1_state_r
   assign ddr3_ila_rdpath[46+:4] = dbg_phy_rdlvl[64:61]; // cal1_cnt_cpt_r
   assign ddr3_ila_rdpath[50+:8] = dbg_phy_rdlvl[177:170]; // mux_rd_rise0_r
   assign ddr3_ila_rdpath[58+:8] = dbg_phy_rdlvl[185:178]; // mux_rd_fall0_r
   assign ddr3_ila_rdpath[66+:8] = dbg_phy_rdlvl[193:186]; // mux_rd_rise1_r
   assign ddr3_ila_rdpath[74+:8] = dbg_phy_rdlvl[201:194]; // mux_rd_fall1_r
   assign ddr3_ila_rdpath[82+:8] = dbg_phy_rdlvl[209:202]; // mux_rd_rise2_r
   assign ddr3_ila_rdpath[90+:8] = dbg_phy_rdlvl[217:210]; // mux_rd_fall2_r
   assign ddr3_ila_rdpath[98+:8] = dbg_phy_rdlvl[225:218]; // mux_rd_rise3_r
   assign ddr3_ila_rdpath[106+:8] = dbg_phy_rdlvl[233:226]; // mux_rd_fall3_r
   assign ddr3_ila_rdpath[114]   = dbg_phy_rdlvl[1]; // pat_data_match_r
   assign ddr3_ila_rdpath[115]   = dbg_phy_rdlvl[2]; // mux_rd_valid_r
   assign ddr3_ila_rdpath[116+:6] = dbg_cpt_first_edge_cnt[(dbg_dqs*6)+:6];
   assign ddr3_ila_rdpath[122+:6] = dbg_cpt_second_edge_cnt[(dbg_dqs*6)+:6];
   assign ddr3_ila_rdpath[128+:6] = dbg_cpt_tap_cnt[(dbg_dqs*6)+:6];
   assign ddr3_ila_rdpath[134+:5] = dbg_dq_idelay_tap_cnt[(dbg_dqs*5)+:5];
   assign ddr3_ila_rdpath[163:139] = 'b0;

   // data offset values for PHASER_IN
   assign ddr3_ila_rdpath[164+:12] = dbg_calib_rd_data_offset_1;
   assign ddr3_ila_rdpath[176+:12] = dbg_calib_rd_data_offset_2;
   assign ddr3_ila_rdpath[188+:6]  = dbg_data_offset;
   assign ddr3_ila_rdpath[194+:6]  = dbg_data_offset_1;
   assign ddr3_ila_rdpath[200+:6]  = dbg_data_offset_2;

   assign ddr3_ila_rdpath[206+:108] = dbg_cpt_first_edge_cnt;
   assign ddr3_ila_rdpath[314+:108] = dbg_cpt_second_edge_cnt;
   assign ddr3_ila_rdpath[422+:108] = dbg_cpt_tap_cnt;
   assign ddr3_ila_rdpath[530+:90]  = dbg_dq_idelay_tap_cnt;
   assign ddr3_ila_rdpath[620+:255]  = dbg_prbs_rdlvl;


      

endmodule
