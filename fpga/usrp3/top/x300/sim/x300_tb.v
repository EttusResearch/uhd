`timescale 1ps / 1ps

module x300_tb();

   /////////////////// START FROM XILINX TB ///////////////////////////
   
   //***************************************************************************
   // Traffic Gen related parameters
   //***************************************************************************
   parameter SIMULATION            = "TRUE";
   parameter BL_WIDTH              = 10;
   parameter PORT_MODE             = "BI_MODE";
   parameter DATA_MODE             = 4'b0010;
   parameter ADDR_MODE             = 4'b0011;
   parameter TST_MEM_INSTR_MODE    = "R_W_INSTR_MODE";
   parameter EYE_TEST              = "FALSE";
                                     // set EYE_TEST = "TRUE" to probe memory
                                     // signals. Traffic Generator will only
                                     // write to one single location and no
                                     // read transactions will be generated.
   parameter DATA_PATTERN          = "DGEN_ALL";
                                      // For small devices, choose one only.
                                      // For large device, choose "DGEN_ALL"
                                      // "DGEN_HAMMER", "DGEN_WALKING1",
                                      // "DGEN_WALKING0","DGEN_ADDR","
                                      // "DGEN_NEIGHBOR","DGEN_PRBS","DGEN_ALL"
   parameter CMD_PATTERN           = "CGEN_ALL";
                                      // "CGEN_PRBS","CGEN_FIXED","CGEN_BRAM",
                                      // "CGEN_SEQUENTIAL", "CGEN_ALL"
   parameter BEGIN_ADDRESS         = 32'h00000000;
   parameter END_ADDRESS           = 32'h00000fff;
   parameter PRBS_EADDR_MASK_POS   = 32'hff000000;
   parameter SEL_VICTIM_LINE       = 11;

   //***************************************************************************
   // The following parameters refer to width of various ports
   //***************************************************************************
   parameter BANK_WIDTH            = 3;
                                     // # of memory Bank Address bits.
   parameter CK_WIDTH              = 1;
                                     // # of CK/CK# outputs to memory.
   parameter COL_WIDTH             = 10;
                                     // # of memory Column Address bits.
   parameter CS_WIDTH              = 1;
                                     // # of unique CS outputs to memory.
   parameter nCS_PER_RANK          = 1;
                                     // # of unique CS outputs per rank for phy
   parameter CKE_WIDTH             = 1;
                                     // # of CKE outputs to memory.
   parameter DATA_BUF_ADDR_WIDTH   = 5;
   parameter DQ_CNT_WIDTH          = 5;
                                     // = ceil(log2(DQ_WIDTH))
   parameter DQ_PER_DM             = 8;
   parameter DM_WIDTH              = 4;
                                     // # of DM (data mask)
   parameter DQ_WIDTH              = 32;
                                     // # of DQ (data)
   parameter DQS_WIDTH             = 4;
   parameter DQS_CNT_WIDTH         = 2;
                                     // = ceil(log2(DQS_WIDTH))
   parameter DRAM_WIDTH            = 8;
                                     // # of DQ per DQS
   parameter ECC                   = "OFF";
   parameter nBANK_MACHS           = 4;
   parameter RANKS                 = 1;
                                     // # of Ranks.
   parameter ODT_WIDTH             = 1;
                                     // # of ODT outputs to memory.
   parameter ROW_WIDTH             = 15;
                                     // # of memory Row Address bits.
   parameter ADDR_WIDTH            = 29;
                                     // # = RANK_WIDTH + BANK_WIDTH
                                     //     + ROW_WIDTH + COL_WIDTH;
                                     // Chip Select is always tied to low for
                                     // single rank devices
   parameter USE_CS_PORT          = 1;
                                     // # = 1, When CS output is enabled
                                     //   = 0, When CS output is disabled
                                     // If CS_N disabled, user must connect
                                     // DRAM CS_N input(s) to ground
   parameter USE_DM_PORT           = 1;
                                     // # = 1, When Data Mask option is enabled
                                     //   = 0, When Data Mask option is disbaled
                                     // When Data Mask option is disabled in
                                     // MIG Controller Options page, the logic
                                     // related to Data Mask should not get
                                     // synthesized
   parameter USE_ODT_PORT          = 1;
                                     // # = 1, When ODT output is enabled
                                     //   = 0, When ODT output is disabled

   //***************************************************************************
   // The following parameters are mode register settings
   //***************************************************************************
   parameter AL                    = "0";
                                     // DDR3 SDRAM:
                                     // Additive Latency (Mode Register 1).
                                     // # = "0", "CL-1", "CL-2".
                                     // DDR2 SDRAM:
                                     // Additive Latency (Extended Mode Register).
   parameter nAL                   = 0;
                                     // # Additive Latency in number of clock
                                     // cycles.
   parameter BURST_MODE            = "8";
                                     // DDR3 SDRAM:
                                     // Burst Length (Mode Register 0).
                                     // # = "8", "4", "OTF".
                                     // DDR2 SDRAM:
                                     // Burst Length (Mode Register).
                                     // # = "8", "4".
   parameter BURST_TYPE            = "SEQ";
                                     // DDR3 SDRAM: Burst Type (Mode Register 0).
                                     // DDR2 SDRAM: Burst Type (Mode Register).
                                     // # = "SEQ" - (Sequential),
                                     //   = "INT" - (Interleaved).
   parameter CL                    = 7;
                                     // in number of clock cycles
                                     // DDR3 SDRAM: CAS Latency (Mode Register 0).
                                     // DDR2 SDRAM: CAS Latency (Mode Register).
   parameter CWL                   = 6;
                                     // in number of clock cycles
                                     // DDR3 SDRAM: CAS Write Latency (Mode Register 2).
                                     // DDR2 SDRAM: Can be ignored
   parameter OUTPUT_DRV            = "HIGH";
                                     // Output Driver Impedance Control (Mode Register 1).
                                     // # = "HIGH" - RZQ/7,
                                     //   = "LOW" - RZQ/6.
   parameter RTT_NOM               = "60";
                                     // RTT_NOM (ODT) (Mode Register 1).
                                     // # = "DISABLED" - RTT_NOM disabled,
                                     //   = "120" - RZQ/2,
                                     //   = "60"  - RZQ/4,
                                     //   = "40"  - RZQ/6.
   parameter RTT_WR                = "OFF";
                                     // RTT_WR (ODT) (Mode Register 2).
                                     // # = "OFF" - Dynamic ODT off,
                                     //   = "120" - RZQ/2,
                                     //   = "60"  - RZQ/4,
   parameter ADDR_CMD_MODE         = "1T" ;
                                     // # = "1T", "2T".
   parameter REG_CTRL              = "OFF";
                                     // # = "ON" - RDIMMs,
                                     //   = "OFF" - Components, SODIMMs, UDIMMs.
   parameter CA_MIRROR             = "OFF";
                                     // C/A mirror opt for DDR3 dual rank
   
   //***************************************************************************
   // The following parameters are multiplier and divisor factors for PLLE2.
   // Based on the selected design frequency these parameters vary.
   //***************************************************************************
   parameter CLKIN_PERIOD          = 10000;
                                     // Input Clock Period
   parameter CLKFBOUT_MULT         = 10;
                                     // write PLL VCO multiplier
   parameter DIVCLK_DIVIDE         = 1;
                                     // write PLL VCO divisor
   parameter CLKOUT0_DIVIDE        = 2;
                                     // VCO output divisor for PLL output clock (CLKOUT0)
   parameter CLKOUT1_DIVIDE        = 2;
                                     // VCO output divisor for PLL output clock (CLKOUT1)
   parameter CLKOUT2_DIVIDE        = 32;
                                     // VCO output divisor for PLL output clock (CLKOUT2)
   parameter CLKOUT3_DIVIDE        = 8;
                                     // VCO output divisor for PLL output clock (CLKOUT3)

   //***************************************************************************
   // Memory Timing Parameters. These parameters varies based on the selected
   // memory part.
   //***************************************************************************
   parameter tCKE                  = 5000;
                                     // memory tCKE paramter in pS
   parameter tFAW                  = 30000;
                                     // memory tRAW paramter in pS.
   parameter tRAS                  = 35000;
                                     // memory tRAS paramter in pS.
   parameter tRCD                  = 13750;
                                     // memory tRCD paramter in pS.
   parameter tREFI                 = 7800000;
                                     // memory tREFI paramter in pS.
   parameter tRFC                  = 300000;
                                     // memory tRFC paramter in pS.
   parameter tRP                   = 13750;
                                     // memory tRP paramter in pS.
   parameter tRRD                  = 6000;
                                     // memory tRRD paramter in pS.
   parameter tRTP                  = 7500;
                                     // memory tRTP paramter in pS.
   parameter tWTR                  = 7500;
                                     // memory tWTR paramter in pS.
   parameter tZQI                  = 128_000_000;
                                     // memory tZQI paramter in nS.
   parameter tZQCS                 = 64;
                                     // memory tZQCS paramter in clock cycles.

   //***************************************************************************
   // Simulation parameters
   //***************************************************************************
   parameter SIM_BYPASS_INIT_CAL   = "FAST";
                                     // # = "SIM_INIT_CAL_FULL" -  Complete
                                     //              memory init &
                                     //              calibration sequence
                                     // # = "SKIP" - Not supported
                                     // # = "FAST" - Complete memory init & use
                                     //              abbreviated calib sequence

   //***************************************************************************
   // The following parameters varies based on the pin out entered in MIG GUI.
   // Do not change any of these parameters directly by editing the RTL.
   // Any changes required should be done through GUI and the design regenerated.
   //***************************************************************************
   parameter BYTE_LANES_B0         = 4'b1111;
                                     // Byte lanes used in an IO column.
   parameter BYTE_LANES_B1         = 4'b1110;
                                     // Byte lanes used in an IO column.
   parameter BYTE_LANES_B2         = 4'b0000;
                                     // Byte lanes used in an IO column.
   parameter BYTE_LANES_B3         = 4'b0000;
                                     // Byte lanes used in an IO column.
   parameter BYTE_LANES_B4         = 4'b0000;
                                     // Byte lanes used in an IO column.
   parameter DATA_CTL_B0           = 4'b1111;
                                     // Indicates Byte lane is data byte lane
                                     // or control Byte lane. '1' in a bit
                                     // position indicates a data byte lane and
                                     // a '0' indicates a control byte lane
   parameter DATA_CTL_B1           = 4'b0000;
                                     // Indicates Byte lane is data byte lane
                                     // or control Byte lane. '1' in a bit
                                     // position indicates a data byte lane and
                                     // a '0' indicates a control byte lane
   parameter DATA_CTL_B2           = 4'b0000;
                                     // Indicates Byte lane is data byte lane
                                     // or control Byte lane. '1' in a bit
                                     // position indicates a data byte lane and
                                     // a '0' indicates a control byte lane
   parameter DATA_CTL_B3           = 4'b0000;
                                     // Indicates Byte lane is data byte lane
                                     // or control Byte lane. '1' in a bit
                                     // position indicates a data byte lane and
                                     // a '0' indicates a control byte lane
   parameter DATA_CTL_B4           = 4'b0000;
                                     // Indicates Byte lane is data byte lane
                                     // or control Byte lane. '1' in a bit
                                     // position indicates a data byte lane and
                                     // a '0' indicates a control byte lane
   parameter PHY_0_BITLANES        = 48'h3FE_3FE_3FE_2FF;
   parameter PHY_1_BITLANES        = 48'h3FF_FFF_C00_000;
   parameter PHY_2_BITLANES        = 48'h000_000_000_000;

   // control/address/data pin mapping parameters
   parameter CK_BYTE_MAP
     = 144'h00_00_00_00_00_00_00_00_00_00_00_00_00_00_00_00_00_13;
   parameter ADDR_MAP
     = 192'h000_139_138_137_136_135_134_133_132_131_130_129_128_127_126_12B;
   parameter BANK_MAP   = 36'h12A_125_124;
   parameter CAS_MAP    = 12'h122;
   parameter CKE_ODT_BYTE_MAP = 8'h00;
   parameter CKE_MAP    = 96'h000_000_000_000_000_000_000_11B;
   parameter ODT_MAP    = 96'h000_000_000_000_000_000_000_11A;
   parameter CS_MAP     = 120'h000_000_000_000_000_000_000_000_000_120;
   parameter PARITY_MAP = 12'h000;
   parameter RAS_MAP    = 12'h123;
   parameter WE_MAP     = 12'h121;
   parameter DQS_BYTE_MAP
     = 144'h00_00_00_00_00_00_00_00_00_00_00_00_00_00_00_01_02_03;
   parameter DATA0_MAP  = 96'h031_032_033_034_035_036_037_038;
   parameter DATA1_MAP  = 96'h021_022_023_024_025_026_027_028;
   parameter DATA2_MAP  = 96'h011_012_013_014_015_016_017_018;
   parameter DATA3_MAP  = 96'h000_001_002_003_004_005_006_007;
   parameter DATA4_MAP  = 96'h000_000_000_000_000_000_000_000;
   parameter DATA5_MAP  = 96'h000_000_000_000_000_000_000_000;
   parameter DATA6_MAP  = 96'h000_000_000_000_000_000_000_000;
   parameter DATA7_MAP  = 96'h000_000_000_000_000_000_000_000;
   parameter DATA8_MAP  = 96'h000_000_000_000_000_000_000_000;
   parameter DATA9_MAP  = 96'h000_000_000_000_000_000_000_000;
   parameter DATA10_MAP = 96'h000_000_000_000_000_000_000_000;
   parameter DATA11_MAP = 96'h000_000_000_000_000_000_000_000;
   parameter DATA12_MAP = 96'h000_000_000_000_000_000_000_000;
   parameter DATA13_MAP = 96'h000_000_000_000_000_000_000_000;
   parameter DATA14_MAP = 96'h000_000_000_000_000_000_000_000;
   parameter DATA15_MAP = 96'h000_000_000_000_000_000_000_000;
   parameter DATA16_MAP = 96'h000_000_000_000_000_000_000_000;
   parameter DATA17_MAP = 96'h000_000_000_000_000_000_000_000;
   parameter MASK0_MAP  = 108'h000_000_000_000_000_009_019_029_039;
   parameter MASK1_MAP  = 108'h000_000_000_000_000_000_000_000_000;

   parameter SLOT_0_CONFIG         = 8'b0000_0001;
                                     // Mapping of Ranks.
   parameter SLOT_1_CONFIG         = 8'b0000_0000;
                                     // Mapping of Ranks.
   parameter MEM_ADDR_ORDER
     = "BANK_ROW_COLUMN";

   //***************************************************************************
   // IODELAY and PHY related parameters
   //***************************************************************************
   parameter IODELAY_HP_MODE       = "ON";
                                     // to phy_top
   parameter IBUF_LPWR_MODE        = "OFF";
                                     // to phy_top
   parameter DATA_IO_IDLE_PWRDWN   = "ON";
                                     // # = "ON", "OFF"
   parameter DATA_IO_PRIM_TYPE     = "HP_LP";
                                     // # = "HP_LP", "HR_LP", "DEFAULT"
   parameter USER_REFRESH          = "OFF";
   parameter WRLVL                 = "ON";
                                     // # = "ON" - DDR3 SDRAM
                                     //   = "OFF" - DDR2 SDRAM.
   parameter ORDERING              = "NORM";
                                     // # = "NORM", "STRICT", "RELAXED".
   parameter CALIB_ROW_ADD         = 16'h0000;
                                     // Calibration row address will be used for
                                     // calibration read and write operations
   parameter CALIB_COL_ADD         = 12'h000;
                                     // Calibration column address will be used for
                                     // calibration read and write operations
   parameter CALIB_BA_ADD          = 3'h0;
                                     // Calibration bank address will be used for
                                     // calibration read and write operations
   parameter TCQ                   = 100;
   //***************************************************************************
   // IODELAY and PHY related parameters
   //***************************************************************************
   parameter IODELAY_GRP           = "IODELAY_MIG";
                                     // It is associated to a set of IODELAYs with
                                     // an IDELAYCTRL that have same IODELAY CONTROLLER
                                     // clock frequency.
   parameter SYSCLK_TYPE           = "SINGLE_ENDED";
                                     // System clock type DIFFERENTIAL, SINGLE_ENDED,
                                     // NO_BUFFER
   parameter REFCLK_TYPE           = "NO_BUFFER";
                                     // Reference clock type DIFFERENTIAL, SINGLE_ENDED,
                                     // NO_BUFFER, USE_SYSTEM_CLOCK
   parameter RST_ACT_LOW           = 1;
                                     // =1 for active low reset,
                                     // =0 for active high.
   parameter CAL_WIDTH             = "HALF";
   parameter STARVE_LIMIT          = 2;
                                     // # = 2,3,4.

   //***************************************************************************
   // Referece clock frequency parameters
   //***************************************************************************
   parameter REFCLK_FREQ           = 200.0;
                                     // IODELAYCTRL reference clock frequency
   //***************************************************************************
   // System clock frequency parameters
   //***************************************************************************
   parameter tCK                   = 2000;
                                     // memory tCK paramter.
                     // # = Clock Period in pS.
   parameter nCK_PER_CLK           = 4;
                                     // # of memory CKs per fabric CLK

   
   //***************************************************************************
   // AXI4 Shim parameters
   //***************************************************************************
   parameter C_S_AXI_ID_WIDTH              = 4;
                                             // Width of all master and slave ID signals.
                                             // # = >= 1.
   parameter C_S_AXI_ADDR_WIDTH            = 32;
                                             // Width of S_AXI_AWADDR, S_AXI_ARADDR, M_AXI_AWADDR and
                                             // M_AXI_ARADDR for all SI/MI slots.
                                             // # = 32.
   parameter C_S_AXI_DATA_WIDTH            = 128;
                                             // Width of WDATA and RDATA on SI slot.
                                             // Must be <= APP_DATA_WIDTH.
                                             // # = 32, 64, 128, 256.
   parameter C_MC_nCK_PER_CLK              = 4;
                                             // Indicates whether to instatiate upsizer
                                             // Range: 0, 1
   parameter C_S_AXI_SUPPORTS_NARROW_BURST = 1;
                                             // Indicates whether to instatiate upsizer
                                             // Range: 0, 1
   parameter C_RD_WR_ARB_ALGORITHM          = "ROUND_ROBIN";
                                             // Indicates the Arbitration
                                             // Allowed values - "TDM", "ROUND_ROBIN",
                                             // "RD_PRI_REG", "RD_PRI_REG_STARVE_LIMIT"
   parameter C_S_AXI_REG_EN0               = 20'h00000;
                                             // C_S_AXI_REG_EN0[00] = Reserved
                                             // C_S_AXI_REG_EN0[04] = AW CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN0[05] =  W CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN0[06] =  B CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN0[07] =  R CHANNEL REGISTER SLICE
                                             // C_S_AXI_REG_EN0[08] = AW CHANNEL UPSIZER REGISTER SLICE
                                             // C_S_AXI_REG_EN0[09] =  W CHANNEL UPSIZER REGISTER SLICE
                                             // C_S_AXI_REG_EN0[10] = AR CHANNEL UPSIZER REGISTER SLICE
                                             // C_S_AXI_REG_EN0[11] =  R CHANNEL UPSIZER REGISTER SLICE
   parameter C_S_AXI_REG_EN1                 = 20'h00000;
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

   //***************************************************************************
   // Debug and Internal parameters
   //***************************************************************************
   parameter DEBUG_PORT            = "OFF";
                                     // # = "ON" Enable debug signals/controls.
                                     //   = "OFF" Disable debug signals/controls.
   //***************************************************************************
   // Debug and Internal parameters
   //***************************************************************************
   parameter DRAM_TYPE             = "DDR3";

    

  //**************************************************************************//
  // Local parameters Declarations
  //**************************************************************************//

  localparam real TPROP_DQS          = 0.00;
                                       // Delay for DQS signal during Write Operation
  localparam real TPROP_DQS_RD       = 0.00;
                       // Delay for DQS signal during Read Operation
  localparam real TPROP_PCB_CTRL     = 0.00;
                       // Delay for Address and Ctrl signals
  localparam real TPROP_PCB_DATA     = 0.00;
                       // Delay for data signal during Write operation
  localparam real TPROP_PCB_DATA_RD  = 0.00;
                       // Delay for data signal during Read operation

  localparam MEMORY_WIDTH            = 16;
  localparam NUM_COMP                = DQ_WIDTH/MEMORY_WIDTH;

  localparam real REFCLK_PERIOD = (1000000.0/(2*REFCLK_FREQ));
  localparam RESET_PERIOD = 200000; //in pSec  
  localparam real SYSCLK_PERIOD = tCK;

   ///////////////// END FROM XILINX TB //////////////////////////////////////////
   
   wire [31:0] ddr3_dq;    // Data pins. Input for Reads, Output for Writes.
   wire [3:0] ddr3_dqs_n;  // Data Strobes. Input for Reads, Output for Writes.
   wire [3:0] ddr3_dqs_p;
   wire [14:0] ddr3_addr;  // Address
   wire [2:0] ddr3_ba;     // Bank Address
   wire ddr3_ras_n;        // Row Address Strobe.
   wire ddr3_cas_n;        // Column address select
   wire ddr3_we_n;         // Write Enable
   wire ddr3_reset_n;      // SDRAM reset pin.
   wire ddr3_ck_p;         // Differential clock
   wire ddr3_ck_n;
   wire ddr3_cke;          // Clock Enable
   wire ddr3_cs_n;          // Chip Select
   wire [3:0] ddr3_dm;     // Data Mask [3] = UDM.U26, [2] = LDM.U26, ...
   wire ddr3_odt;          // On-Die termination enable.
   reg sys_clk_i;         // 100MHz clock source to generate DDR3 clocking.
   reg FPGA_CLK_p, FPGA_CLK_n;
   reg FPGA_125MHz_CLK;

   ///////////////////////////////////////////////////////////
   //
   // 120MHz differential clock source
   //
   ///////////////////////////////////////////////////////////
   initial 
     begin
	FPGA_CLK_p <= 0;
	FPGA_CLK_n <= 1;
     end

   always #4000 begin
      	FPGA_CLK_p <= ~FPGA_CLK_p;
	FPGA_CLK_n <= ~FPGA_CLK_n;
     end
   
   
   ///////////////////////////////////////////////////////////
   //
   // 125MHz clock source
   //
   ///////////////////////////////////////////////////////////
   initial FPGA_125MHz_CLK <= 0;

   always #4000 FPGA_125MHz_CLK <= ~FPGA_125MHz_CLK;
   
   ///////////////////////////////////////////////////////////
   //
   // 100MHz clock source for DDR3
   //
   ///////////////////////////////////////////////////////////
   initial
     sys_clk_i <= 0;

   always #5000 sys_clk_i = ~sys_clk_i;

   ///////////////////////////////////////////////////////////
   //
   // 2x 4Gb DDR3 SDRAMS x16
   //
   ///////////////////////////////////////////////////////////
  
   ddr3_model sdram0 (
		      .rst_n(ddr3_reset_n),
		      .ck(ddr3_ck_p), 
		      .ck_n(ddr3_ck_n),
		      .cke(ddr3_cke), 
		      .cs_n(ddr3_cs_n),
		      .ras_n(ddr3_ras_n), 
		      .cas_n(ddr3_cas_n), 
		      .we_n(ddr3_we_n), 
		      .dm_tdqs(ddr3_dm[1:0]), 
		      .ba(ddr3_ba), 
		      .addr(ddr3_addr), 
		      .dq(ddr3_dq[15:0]), 
		      .dqs(ddr3_dqs_p[1:0]),
		      .dqs_n(ddr3_dqs_n[1:0]),
		      .tdqs_n(), // Unused on x16
		      .odt(ddr3_odt)	
		      );

   ddr3_model sdram1 (
		      .rst_n(ddr3_reset_n),
		      .ck(ddr3_ck_p), 
		      .ck_n(ddr3_ck_n),
		      .cke(ddr3_cke), 
		      .cs_n(ddr3_cs_n),
		      .ras_n(ddr3_ras_n), 
		      .cas_n(ddr3_cas_n), 
		      .we_n(ddr3_we_n), 
		      .dm_tdqs(ddr3_dm[3:2]), 
		      .ba(ddr3_ba), 
		      .addr(ddr3_addr), 
		      .dq(ddr3_dq[31:16]), 
		      .dqs(ddr3_dqs_p[3:2]),
		      .dqs_n(ddr3_dqs_n[3:2]),
		      .tdqs_n(), // Unused on x16
		      .odt(ddr3_odt)
		      );

   ///////////////////////////////////////////////////////////
   //
   // DUT
   //
   ///////////////////////////////////////////////////////////
   
   x300 #
    (

     .SIMULATION                (SIMULATION),
     .BL_WIDTH                  (BL_WIDTH),
     .PORT_MODE                 (PORT_MODE),
     .DATA_MODE                 (DATA_MODE),
     .ADDR_MODE                 (ADDR_MODE),
     .TST_MEM_INSTR_MODE        (TST_MEM_INSTR_MODE),
     .EYE_TEST                  (EYE_TEST),
     .DATA_PATTERN              (DATA_PATTERN),
     .CMD_PATTERN               (CMD_PATTERN),
     .BEGIN_ADDRESS             (BEGIN_ADDRESS),
     .END_ADDRESS               (END_ADDRESS),
     .PRBS_EADDR_MASK_POS       (PRBS_EADDR_MASK_POS),
     .SEL_VICTIM_LINE           (SEL_VICTIM_LINE),

     .BANK_WIDTH                (BANK_WIDTH),
     .CK_WIDTH                  (CK_WIDTH),
     .COL_WIDTH                 (COL_WIDTH),
     .CS_WIDTH                  (CS_WIDTH),
     .nCS_PER_RANK              (nCS_PER_RANK),
     .CKE_WIDTH                 (CKE_WIDTH),
     .DATA_BUF_ADDR_WIDTH       (DATA_BUF_ADDR_WIDTH),
     .DQ_CNT_WIDTH              (DQ_CNT_WIDTH),
     .DQ_PER_DM                 (DQ_PER_DM),
     .DM_WIDTH                  (DM_WIDTH),
    
     .DQ_WIDTH                  (DQ_WIDTH),
     .DQS_WIDTH                 (DQS_WIDTH),
     .DQS_CNT_WIDTH             (DQS_CNT_WIDTH),
     .DRAM_WIDTH                (DRAM_WIDTH),
     .ECC                       (ECC),
     .nBANK_MACHS               (nBANK_MACHS),
     .RANKS                     (RANKS),
     .ODT_WIDTH                 (ODT_WIDTH),
     .ROW_WIDTH                 (ROW_WIDTH),
     .ADDR_WIDTH                (ADDR_WIDTH),
     .USE_CS_PORT               (USE_CS_PORT),
     .USE_DM_PORT               (USE_DM_PORT),
     .USE_ODT_PORT              (USE_ODT_PORT),

     .AL                        (AL),
     .nAL                       (nAL),
     .BURST_MODE                (BURST_MODE),
     .BURST_TYPE                (BURST_TYPE),
     .CL                        (CL),
     .CWL                       (CWL),
     .OUTPUT_DRV                (OUTPUT_DRV),
     .RTT_NOM                   (RTT_NOM),
     .RTT_WR                    (RTT_WR),
     .ADDR_CMD_MODE             (ADDR_CMD_MODE),
     .REG_CTRL                  (REG_CTRL),
     .CA_MIRROR                 (CA_MIRROR),


     .CLKIN_PERIOD              (CLKIN_PERIOD),
     .CLKFBOUT_MULT             (CLKFBOUT_MULT),
     .DIVCLK_DIVIDE             (DIVCLK_DIVIDE),
     .CLKOUT0_DIVIDE            (CLKOUT0_DIVIDE),
     .CLKOUT1_DIVIDE            (CLKOUT1_DIVIDE),
     .CLKOUT2_DIVIDE            (CLKOUT2_DIVIDE),
     .CLKOUT3_DIVIDE            (CLKOUT3_DIVIDE),
    

     .tCKE                      (tCKE),
     .tFAW                      (tFAW),
     .tRAS                      (tRAS),
     .tRCD                      (tRCD),
     .tREFI                     (tREFI),
     .tRFC                      (tRFC),
     .tRP                       (tRP),
     .tRRD                      (tRRD),
     .tRTP                      (tRTP),
     .tWTR                      (tWTR),
     .tZQI                      (tZQI),
     .tZQCS                     (tZQCS),

     .SIM_BYPASS_INIT_CAL       (SIM_BYPASS_INIT_CAL),

     .BYTE_LANES_B0             (BYTE_LANES_B0),
     .BYTE_LANES_B1             (BYTE_LANES_B1),
     .BYTE_LANES_B2             (BYTE_LANES_B2),
     .BYTE_LANES_B3             (BYTE_LANES_B3),
     .BYTE_LANES_B4             (BYTE_LANES_B4),
     .DATA_CTL_B0               (DATA_CTL_B0),
     .DATA_CTL_B1               (DATA_CTL_B1),
     .DATA_CTL_B2               (DATA_CTL_B2),
     .DATA_CTL_B3               (DATA_CTL_B3),
     .DATA_CTL_B4               (DATA_CTL_B4),
     .PHY_0_BITLANES            (PHY_0_BITLANES),
     .PHY_1_BITLANES            (PHY_1_BITLANES),
     .PHY_2_BITLANES            (PHY_2_BITLANES),
     .CK_BYTE_MAP               (CK_BYTE_MAP),
     .ADDR_MAP                  (ADDR_MAP),
     .BANK_MAP                  (BANK_MAP),
     .CAS_MAP                   (CAS_MAP),
     .CKE_ODT_BYTE_MAP          (CKE_ODT_BYTE_MAP),
     .CKE_MAP                   (CKE_MAP),
     .ODT_MAP                   (ODT_MAP),
     .CS_MAP                    (CS_MAP),
     .PARITY_MAP                (PARITY_MAP),
     .RAS_MAP                   (RAS_MAP),
     .WE_MAP                    (WE_MAP),
     .DQS_BYTE_MAP              (DQS_BYTE_MAP),
     .DATA0_MAP                 (DATA0_MAP),
     .DATA1_MAP                 (DATA1_MAP),
     .DATA2_MAP                 (DATA2_MAP),
     .DATA3_MAP                 (DATA3_MAP),
     .DATA4_MAP                 (DATA4_MAP),
     .DATA5_MAP                 (DATA5_MAP),
     .DATA6_MAP                 (DATA6_MAP),
     .DATA7_MAP                 (DATA7_MAP),
     .DATA8_MAP                 (DATA8_MAP),
     .DATA9_MAP                 (DATA9_MAP),
     .DATA10_MAP                (DATA10_MAP),
     .DATA11_MAP                (DATA11_MAP),
     .DATA12_MAP                (DATA12_MAP),
     .DATA13_MAP                (DATA13_MAP),
     .DATA14_MAP                (DATA14_MAP),
     .DATA15_MAP                (DATA15_MAP),
     .DATA16_MAP                (DATA16_MAP),
     .DATA17_MAP                (DATA17_MAP),
     .MASK0_MAP                 (MASK0_MAP),
     .MASK1_MAP                 (MASK1_MAP),
     .SLOT_0_CONFIG             (SLOT_0_CONFIG),
     .SLOT_1_CONFIG             (SLOT_1_CONFIG),
     .MEM_ADDR_ORDER            (MEM_ADDR_ORDER),

     .IODELAY_HP_MODE           (IODELAY_HP_MODE),
     .IBUF_LPWR_MODE            (IBUF_LPWR_MODE),
     .DATA_IO_IDLE_PWRDWN       (DATA_IO_IDLE_PWRDWN),
     .DATA_IO_PRIM_TYPE         (DATA_IO_PRIM_TYPE),
     .USER_REFRESH              (USER_REFRESH),
     .WRLVL                     (WRLVL),
     .ORDERING                  (ORDERING),
     .CALIB_ROW_ADD             (CALIB_ROW_ADD),
     .CALIB_COL_ADD             (CALIB_COL_ADD),
     .CALIB_BA_ADD              (CALIB_BA_ADD),
     .TCQ                       (TCQ),

     
    .IODELAY_GRP               (IODELAY_GRP),
    .SYSCLK_TYPE               (SYSCLK_TYPE),
    .REFCLK_TYPE               (REFCLK_TYPE),
    .DRAM_TYPE                 (DRAM_TYPE),
    .CAL_WIDTH                 (CAL_WIDTH),
    .STARVE_LIMIT              (STARVE_LIMIT),
    
     
    .REFCLK_FREQ               (REFCLK_FREQ),
    
     
    .tCK                       (tCK),
    .nCK_PER_CLK               (nCK_PER_CLK),
    
     
     .C_S_AXI_ID_WIDTH          (C_S_AXI_ID_WIDTH),
     .C_S_AXI_ADDR_WIDTH        (C_S_AXI_ADDR_WIDTH),
     .C_S_AXI_DATA_WIDTH        (C_S_AXI_DATA_WIDTH),
     .C_MC_nCK_PER_CLK          (C_MC_nCK_PER_CLK),
     .C_S_AXI_SUPPORTS_NARROW_BURST (C_S_AXI_SUPPORTS_NARROW_BURST),
     .C_RD_WR_ARB_ALGORITHM      (C_RD_WR_ARB_ALGORITHM),
     .C_S_AXI_REG_EN0           (C_S_AXI_REG_EN0),
     .C_S_AXI_REG_EN1           (C_S_AXI_REG_EN1),
    
     .DEBUG_PORT                (DEBUG_PORT),
    
     .RST_ACT_LOW               (RST_ACT_LOW)
    )

x300 (
	      .FPGA_CLK_p(FPGA_CLK_p), 
	      .FPGA_CLK_n(FPGA_CLK_n),
	      .FPGA_125MHz_CLK(FPGA_125MHz_CLK),
	      // 1Gb SFP signals
	      .SFP_CLK_AC_p(), 
	      .SFP_CLK_AC_n(),   
	      .SFP_RX_p(), 
	      .SFP_RX_n(),
	      .SFP_TX_p(), 
	      .SFP_TX_n(),
	      // DDR3 I/F
	      .ddr3_addr(ddr3_addr),
	      .ddr3_ba(ddr3_ba),
	      .ddr3_ras_n(ddr3_ras_n),
	      .ddr3_cas_n(ddr3_cas_n),
	      .ddr3_we_n(ddr3_we_n),
	      .ddr3_reset_n(ddr3_reset_n),
	      .ddr3_ck_p(ddr3_ck_p),
	      .ddr3_ck_n(ddr3_ck_n),
	      .ddr3_cke(ddr3_cke),
	      .ddr3_cs_n(ddr3_cs_n),
	      .ddr3_dm(ddr3_dm),
	      .ddr3_odt(ddr3_odt),
	      .ddr3_dq(ddr3_dq),
	      .ddr3_dqs_n(ddr3_dqs_n),
	      .ddr3_dqs_p(ddr3_dqs_p),
	      .sys_clk_i(sys_clk_i),
	      // ADC0
	      .DB0_ADC_DCLK_P(), .DB0_ADC_DCLK_N(),
	      .DB0_ADC_DA0_P(), .DB0_ADC_DA0_N(), .DB0_ADC_DB0_P(), .DB0_ADC_DB0_N(),
	      .DB0_ADC_DA1_P(), .DB0_ADC_DA1_N(), .DB0_ADC_DB1_P(), .DB0_ADC_DB1_N(),
	      .DB0_ADC_DA2_P(), .DB0_ADC_DA2_N(), .DB0_ADC_DB2_P(), .DB0_ADC_DB2_N(),
	      .DB0_ADC_DA3_P(), .DB0_ADC_DA3_N(), .DB0_ADC_DB3_P(), .DB0_ADC_DB3_N(),
	      .DB0_ADC_DA4_P(), .DB0_ADC_DA4_N(), .DB0_ADC_DB4_P(), .DB0_ADC_DB4_N(),
	      .DB0_ADC_DA5_P(), .DB0_ADC_DA5_N(), .DB0_ADC_DB5_P(), .DB0_ADC_DB5_N(),
	      .DB0_ADC_DA6_P(), .DB0_ADC_DA6_N(), .DB0_ADC_DB6_P(), .DB0_ADC_DB6_N(),

	      // ADC1
      
	      .DB1_ADC_DCLK_P(), .DB1_ADC_DCLK_N(),
	      .DB1_ADC_DA0_P(), .DB1_ADC_DA0_N(), .DB1_ADC_DB0_P(), .DB1_ADC_DB0_N(),
	      .DB1_ADC_DA1_P(), .DB1_ADC_DA1_N(), .DB1_ADC_DB1_P(), .DB1_ADC_DB1_N(),
	      .DB1_ADC_DA2_P(), .DB1_ADC_DA2_N(), .DB1_ADC_DB2_P(), .DB1_ADC_DB2_N(),
	      .DB1_ADC_DA3_P(), .DB1_ADC_DA3_N(), .DB1_ADC_DB3_P(), .DB1_ADC_DB3_N(),
	      .DB1_ADC_DA4_P(), .DB1_ADC_DA4_N(), .DB1_ADC_DB4_P(), .DB1_ADC_DB4_N(),
	      .DB1_ADC_DA5_P(), .DB1_ADC_DA5_N(), .DB1_ADC_DB5_P(), .DB1_ADC_DB5_N(),
	      .DB1_ADC_DA6_P(), .DB1_ADC_DA6_N(), .DB1_ADC_DB6_P(), .DB1_ADC_DB6_N(),

	      // DAC0
	      .DB0_DAC_DCI_P(), .DB0_DAC_DCI_N(),
	      .DB0_DAC_FRAME_P(), .DB0_DAC_FRAME_N(),
	      .DB0_DAC_D0_P(), .DB0_DAC_D0_N(), .DB0_DAC_D1_P(), .DB0_DAC_D1_N(), 
	      .DB0_DAC_D2_P(), .DB0_DAC_D2_N(), .DB0_DAC_D3_P(), .DB0_DAC_D3_N(), 
	      .DB0_DAC_D4_P(), .DB0_DAC_D4_N(), .DB0_DAC_D5_P(), .DB0_DAC_D5_N(), 
	      .DB0_DAC_D6_P(), .DB0_DAC_D6_N(), .DB0_DAC_D7_P(), .DB0_DAC_D7_N(), 
	      .DB0_DAC_ENABLE(),

	      // DAC1
	      .DB1_DAC_DCI_P(), .DB1_DAC_DCI_N(),
	      .DB1_DAC_FRAME_P(), .DB1_DAC_FRAME_N(),
	      .DB1_DAC_D0_P(), .DB1_DAC_D0_N(), .DB1_DAC_D1_P(), .DB1_DAC_D1_N(), 
	      .DB1_DAC_D2_P(), .DB1_DAC_D2_N(), .DB1_DAC_D3_P(), .DB1_DAC_D3_N(), 
	      .DB1_DAC_D4_P(), .DB1_DAC_D4_N(), .DB1_DAC_D5_P(), .DB1_DAC_D5_N(), 
	      .DB1_DAC_D6_P(), .DB1_DAC_D6_N(), .DB1_DAC_D7_P(), .DB1_DAC_D7_N(), 
	      .DB1_DAC_ENABLE(),

	      // Daughter Board 0 - SPI
	      .DB0_SCLK(), .DB0_MOSI(),
	      .DB0_ADC_SEN(), .DB0_DAC_SEN(), .DB0_TX_SEN(), .DB0_RX_SEN(),
	      .DB0_RX_LSADC_SEN(), .DB0_RX_LSDAC_SEN(), .DB0_TX_LSADC_SEN(), .DB0_TX_LSDAC_SEN(),
	      .DB0_RX_LSADC_MISO(), .DB0_RX_MISO(), .DB0_TX_LSADC_MISO(), .DB0_TX_MISO(),

	      // Daughter Board1 - SPI
	      .DB1_SCLK(), .DB1_MOSI(),
	      .DB1_ADC_SEN(), .DB1_DAC_SEN(), .DB1_TX_SEN(), .DB1_RX_SEN(),
	      .DB1_RX_LSADC_SEN(), .DB1_RX_LSDAC_SEN(), .DB1_TX_LSADC_SEN(), .DB1_TX_LSDAC_SEN(),
	      .DB1_RX_LSADC_MISO(), .DB1_RX_MISO(), .DB1_TX_LSADC_MISO(), .DB1_TX_MISO(),
      
	      // DaughterBoard
	      .DB_DAC_SCLK(), .DB_DAC_MOSI(),
	      .DB_ADC_RESET(), .DB_DAC_RESET(),

	      // SFP+ I2C
	      .SFPP_SCL(), .SFPP_SDA(),

	      // Debug
	      .DebugClkIn(),
	      .DebugClkOut(),
	      .DebugIo(),

	      // Front Panel
	      .FrontPanelGpio(),
	      .LED_ACT(), .LED_LINK(),
	      .LED_PPS(), .LED_REFLOCK(),
	      .LED_RX1_RX(), .LED_RX2_RX(),
	      .LED_TXRX1_RX(), .LED_TXRX1_TX(),
	      .LED_TXRX2_RX(), .LED_TXRX2_TX(),

	      // GPIO
	      .DB0_TX_IO(),
	      .DB0_RX_IO(),
	      .DB1_TX_IO(),
	      .DB1_RX_IO(),

	      // Clocking
	      .AD9510Status(), .AD9510Function(),
	      .AD9510_SEN(), .AD9510_MOSI(), .AD9510_SCLK(), .AD9510_MISO(),
	      .ClockRefSelect(),
	      .GPS_SER_IN(), .GPS_SER_OUT(),
	      .GPS_PPS_OUT(), .EXT_PPS_IN(),
	      .MIMO_TIME_IN_P(), .MIMO_TIME_IN_N(),
	      .MIMO_TIME_OUT_P(), .MIMO_TIME_OUT_N(),

	      // SFP+ contorl/status
	      .SFPP_ModAbs(), .SFPP_RxLOS(), .SFPP_TxFault(),
	      .SFPP_RS0(), .SFPP_RS1(), .SFPP_TxDisable()  // These are actually open drain outputs
	      );
   
   
endmodule // x300_tb
