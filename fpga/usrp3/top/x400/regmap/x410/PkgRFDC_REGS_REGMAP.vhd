---------------------------------------------------------------------
--
-- Copyright 2025 Ettus Research, A National Instruments Brand
-- SPDX-License-Identifier: LGPL-3.0-or-later
--
-- Module: PkgRFDC_REGS_REGMAP.vhd
--
-- Purpose:
--   The constants in this file are autogenerated by XmlParse.
--
----------------------------------------------------------------------
library ieee;
  use ieee.std_logic_1164.all;
  use ieee.numeric_std.all;

package PkgRFDC_REGS_REGMAP is

--===============================================================================
-- A numerically ordered list of registers and their HDL source files
--===============================================================================

  -- MMCM                 : 0x0 (x410_rfdc_regs.v)
  -- INVERT_DB0_IQ_REG    : 0x10000 (x410_rfdc_regs.v)
  -- INVERT_DB1_IQ_REG    : 0x10800 (x410_rfdc_regs.v)
  -- MMCM_RESET_REG       : 0x11000 (x410_rfdc_regs.v)
  -- RF_RESET_CONTROL_REG : 0x12000 (x410_rfdc_regs.v)
  -- RF_RESET_STATUS_REG  : 0x12008 (x410_rfdc_regs.v)
  -- RF_AXI_STATUS_REG    : 0x13000 (x410_rfdc_regs.v)
  -- FABRIC_DSP_REG       : 0x13008 (x410_rfdc_regs.v)
  -- CALIBRATION_DATA     : 0x14000 (x410_rfdc_regs.v)
  -- CALIBRATION_ENABLE   : 0x14008 (x410_rfdc_regs.v)
  -- THRESHOLD_STATUS     : 0x15000 (x410_rfdc_regs.v)
  -- RF_PLL_CONTROL_REG   : 0x16000 (x410_rfdc_regs.v)
  -- RF_PLL_STATUS_REG    : 0x16008 (x410_rfdc_regs.v)
  -- RFDC_INFO_MEM        : 0x17000 (x410_rfdc_regs.v)
  -- RFDC_INFO_REG        : 0x18000 (x410_rfdc_regs.v)

--===============================================================================
-- RegTypes
--===============================================================================

  -- FABRIC_DSP_REGTYPE Type (from common_regs.v)
  constant kFABRIC_DSP_REGTYPESize: integer := 32;
  constant kFABRIC_DSP_REGTYPEMask : std_logic_vector(31 downto 0) := X"ffffffff";
  constant kFABRIC_DSP_RX_CNTSize       : integer := 4;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_RX_CNT
  constant kFABRIC_DSP_RX_CNTMsb        : integer := 3;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_RX_CNT
  constant kFABRIC_DSP_RX_CNT           : integer := 0;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_RX_CNT
  constant kFABRIC_DSP_TX_CNTSize       : integer := 4;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_TX_CNT
  constant kFABRIC_DSP_TX_CNTMsb        : integer := 7;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_TX_CNT
  constant kFABRIC_DSP_TX_CNT           : integer := 4;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_TX_CNT
  constant kFABRIC_DSP_RESERVEDSize       : integer := 2;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_RESERVED
  constant kFABRIC_DSP_RESERVEDMsb        : integer := 9;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_RESERVED
  constant kFABRIC_DSP_RESERVED           : integer := 8;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_RESERVED
  constant kFABRIC_DSP_RX_CNT_DB1Size       : integer :=  4;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_RX_CNT_DB1
  constant kFABRIC_DSP_RX_CNT_DB1Msb        : integer := 13;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_RX_CNT_DB1
  constant kFABRIC_DSP_RX_CNT_DB1           : integer := 10;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_RX_CNT_DB1
  constant kFABRIC_DSP_TX_CNT_DB1Size       : integer :=  4;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_TX_CNT_DB1
  constant kFABRIC_DSP_TX_CNT_DB1Msb        : integer := 17;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_TX_CNT_DB1
  constant kFABRIC_DSP_TX_CNT_DB1           : integer := 14;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_TX_CNT_DB1
  constant kFABRIC_DSP_RESERVED_DB1Size       : integer :=  2;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_RESERVED_DB1
  constant kFABRIC_DSP_RESERVED_DB1Msb        : integer := 19;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_RESERVED_DB1
  constant kFABRIC_DSP_RESERVED_DB1           : integer := 18;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_RESERVED_DB1
  constant kFABRIC_DSP_BWSize       : integer := 12;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_BW
  constant kFABRIC_DSP_BWMsb        : integer := 31;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_BW
  constant kFABRIC_DSP_BW           : integer := 20;  --FABRIC_DSP_REGTYPE:FABRIC_DSP_BW

  -- RF_AXI_STATUS_REGTYPE Type (from common_regs.v)
  constant kRF_AXI_STATUS_REGTYPESize: integer := 32;
  constant kRF_AXI_STATUS_REGTYPEMask : std_logic_vector(31 downto 0) := X"ffffffff";
  constant kRFDC_DAC_TREADYSize       : integer := 2;  --RF_AXI_STATUS_REGTYPE:RFDC_DAC_TREADY
  constant kRFDC_DAC_TREADYMsb        : integer := 1;  --RF_AXI_STATUS_REGTYPE:RFDC_DAC_TREADY
  constant kRFDC_DAC_TREADY           : integer := 0;  --RF_AXI_STATUS_REGTYPE:RFDC_DAC_TREADY
  constant kRFDC_DAC_TVALIDSize       : integer := 2;  --RF_AXI_STATUS_REGTYPE:RFDC_DAC_TVALID
  constant kRFDC_DAC_TVALIDMsb        : integer := 3;  --RF_AXI_STATUS_REGTYPE:RFDC_DAC_TVALID
  constant kRFDC_DAC_TVALID           : integer := 2;  --RF_AXI_STATUS_REGTYPE:RFDC_DAC_TVALID
  constant kRFDC_ADC_Q_TREADYSize       : integer := 2;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_Q_TREADY
  constant kRFDC_ADC_Q_TREADYMsb        : integer := 5;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_Q_TREADY
  constant kRFDC_ADC_Q_TREADY           : integer := 4;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_Q_TREADY
  constant kRFDC_ADC_I_TREADYSize       : integer := 2;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_I_TREADY
  constant kRFDC_ADC_I_TREADYMsb        : integer := 7;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_I_TREADY
  constant kRFDC_ADC_I_TREADY           : integer := 6;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_I_TREADY
  constant kRFDC_ADC_Q_TVALIDSize       : integer := 2;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_Q_TVALID
  constant kRFDC_ADC_Q_TVALIDMsb        : integer := 9;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_Q_TVALID
  constant kRFDC_ADC_Q_TVALID           : integer := 8;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_Q_TVALID
  constant kRFDC_ADC_I_TVALIDSize       : integer :=  2;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_I_TVALID
  constant kRFDC_ADC_I_TVALIDMsb        : integer := 11;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_I_TVALID
  constant kRFDC_ADC_I_TVALID           : integer := 10;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_I_TVALID
  constant kUSER_ADC_TVALIDSize       : integer :=  2;  --RF_AXI_STATUS_REGTYPE:USER_ADC_TVALID
  constant kUSER_ADC_TVALIDMsb        : integer := 13;  --RF_AXI_STATUS_REGTYPE:USER_ADC_TVALID
  constant kUSER_ADC_TVALID           : integer := 12;  --RF_AXI_STATUS_REGTYPE:USER_ADC_TVALID
  constant kUSER_ADC_TREADYSize       : integer :=  2;  --RF_AXI_STATUS_REGTYPE:USER_ADC_TREADY
  constant kUSER_ADC_TREADYMsb        : integer := 15;  --RF_AXI_STATUS_REGTYPE:USER_ADC_TREADY
  constant kUSER_ADC_TREADY           : integer := 14;  --RF_AXI_STATUS_REGTYPE:USER_ADC_TREADY
  constant kRFDC_DAC_TREADY_DB1Size       : integer :=  2;  --RF_AXI_STATUS_REGTYPE:RFDC_DAC_TREADY_DB1
  constant kRFDC_DAC_TREADY_DB1Msb        : integer := 17;  --RF_AXI_STATUS_REGTYPE:RFDC_DAC_TREADY_DB1
  constant kRFDC_DAC_TREADY_DB1           : integer := 16;  --RF_AXI_STATUS_REGTYPE:RFDC_DAC_TREADY_DB1
  constant kRFDC_DAC_TVALID_DB1Size       : integer :=  2;  --RF_AXI_STATUS_REGTYPE:RFDC_DAC_TVALID_DB1
  constant kRFDC_DAC_TVALID_DB1Msb        : integer := 19;  --RF_AXI_STATUS_REGTYPE:RFDC_DAC_TVALID_DB1
  constant kRFDC_DAC_TVALID_DB1           : integer := 18;  --RF_AXI_STATUS_REGTYPE:RFDC_DAC_TVALID_DB1
  constant kRFDC_ADC_Q_TREADY_DB1Size       : integer :=  2;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_Q_TREADY_DB1
  constant kRFDC_ADC_Q_TREADY_DB1Msb        : integer := 21;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_Q_TREADY_DB1
  constant kRFDC_ADC_Q_TREADY_DB1           : integer := 20;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_Q_TREADY_DB1
  constant kRFDC_ADC_I_TREADY_DB1Size       : integer :=  2;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_I_TREADY_DB1
  constant kRFDC_ADC_I_TREADY_DB1Msb        : integer := 23;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_I_TREADY_DB1
  constant kRFDC_ADC_I_TREADY_DB1           : integer := 22;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_I_TREADY_DB1
  constant kRFDC_ADC_Q_TVALID_DB1Size       : integer :=  2;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_Q_TVALID_DB1
  constant kRFDC_ADC_Q_TVALID_DB1Msb        : integer := 25;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_Q_TVALID_DB1
  constant kRFDC_ADC_Q_TVALID_DB1           : integer := 24;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_Q_TVALID_DB1
  constant kRFDC_ADC_I_TVALID_DB1Size       : integer :=  2;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_I_TVALID_DB1
  constant kRFDC_ADC_I_TVALID_DB1Msb        : integer := 27;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_I_TVALID_DB1
  constant kRFDC_ADC_I_TVALID_DB1           : integer := 26;  --RF_AXI_STATUS_REGTYPE:RFDC_ADC_I_TVALID_DB1
  constant kUSER_ADC_TVALID_DB1Size       : integer :=  2;  --RF_AXI_STATUS_REGTYPE:USER_ADC_TVALID_DB1
  constant kUSER_ADC_TVALID_DB1Msb        : integer := 29;  --RF_AXI_STATUS_REGTYPE:USER_ADC_TVALID_DB1
  constant kUSER_ADC_TVALID_DB1           : integer := 28;  --RF_AXI_STATUS_REGTYPE:USER_ADC_TVALID_DB1
  constant kUSER_ADC_TREADY_DB1Size       : integer :=  2;  --RF_AXI_STATUS_REGTYPE:USER_ADC_TREADY_DB1
  constant kUSER_ADC_TREADY_DB1Msb        : integer := 31;  --RF_AXI_STATUS_REGTYPE:USER_ADC_TREADY_DB1
  constant kUSER_ADC_TREADY_DB1           : integer := 30;  --RF_AXI_STATUS_REGTYPE:USER_ADC_TREADY_DB1

  -- RF_RESET_CONTROL_REGTYPE Type (from common_regs.v)
  constant kRF_RESET_CONTROL_REGTYPESize: integer := 32;
  constant kRF_RESET_CONTROL_REGTYPEMask : std_logic_vector(31 downto 0) := X"00000771";
  constant kFSM_RESETSize       : integer := 1;  --RF_RESET_CONTROL_REGTYPE:FSM_RESET
  constant kFSM_RESETMsb        : integer := 0;  --RF_RESET_CONTROL_REGTYPE:FSM_RESET
  constant kFSM_RESET           : integer := 0;  --RF_RESET_CONTROL_REGTYPE:FSM_RESET
  constant kADC_RESETSize       : integer := 1;  --RF_RESET_CONTROL_REGTYPE:ADC_RESET
  constant kADC_RESETMsb        : integer := 4;  --RF_RESET_CONTROL_REGTYPE:ADC_RESET
  constant kADC_RESET           : integer := 4;  --RF_RESET_CONTROL_REGTYPE:ADC_RESET
  constant kADC_ENABLESize       : integer := 1;  --RF_RESET_CONTROL_REGTYPE:ADC_ENABLE
  constant kADC_ENABLEMsb        : integer := 5;  --RF_RESET_CONTROL_REGTYPE:ADC_ENABLE
  constant kADC_ENABLE           : integer := 5;  --RF_RESET_CONTROL_REGTYPE:ADC_ENABLE
  constant kADC_GEARBOX_RESETSize       : integer := 1;  --RF_RESET_CONTROL_REGTYPE:ADC_GEARBOX_RESET
  constant kADC_GEARBOX_RESETMsb        : integer := 6;  --RF_RESET_CONTROL_REGTYPE:ADC_GEARBOX_RESET
  constant kADC_GEARBOX_RESET           : integer := 6;  --RF_RESET_CONTROL_REGTYPE:ADC_GEARBOX_RESET
  constant kDAC_RESETSize       : integer := 1;  --RF_RESET_CONTROL_REGTYPE:DAC_RESET
  constant kDAC_RESETMsb        : integer := 8;  --RF_RESET_CONTROL_REGTYPE:DAC_RESET
  constant kDAC_RESET           : integer := 8;  --RF_RESET_CONTROL_REGTYPE:DAC_RESET
  constant kDAC_ENABLESize       : integer := 1;  --RF_RESET_CONTROL_REGTYPE:DAC_ENABLE
  constant kDAC_ENABLEMsb        : integer := 9;  --RF_RESET_CONTROL_REGTYPE:DAC_ENABLE
  constant kDAC_ENABLE           : integer := 9;  --RF_RESET_CONTROL_REGTYPE:DAC_ENABLE
  constant kDAC_GEARBOX_RESETSize       : integer :=  1;  --RF_RESET_CONTROL_REGTYPE:DAC_GEARBOX_RESET
  constant kDAC_GEARBOX_RESETMsb        : integer := 10;  --RF_RESET_CONTROL_REGTYPE:DAC_GEARBOX_RESET
  constant kDAC_GEARBOX_RESET           : integer := 10;  --RF_RESET_CONTROL_REGTYPE:DAC_GEARBOX_RESET

  -- RF_RESET_STATUS_REGTYPE Type (from common_regs.v)
  constant kRF_RESET_STATUS_REGTYPESize: integer := 32;
  constant kRF_RESET_STATUS_REGTYPEMask : std_logic_vector(31 downto 0) := X"00000888";
  constant kFSM_RESET_DONESize       : integer := 1;  --RF_RESET_STATUS_REGTYPE:FSM_RESET_DONE
  constant kFSM_RESET_DONEMsb        : integer := 3;  --RF_RESET_STATUS_REGTYPE:FSM_RESET_DONE
  constant kFSM_RESET_DONE           : integer := 3;  --RF_RESET_STATUS_REGTYPE:FSM_RESET_DONE
  constant kADC_SEQ_DONESize       : integer := 1;  --RF_RESET_STATUS_REGTYPE:ADC_SEQ_DONE
  constant kADC_SEQ_DONEMsb        : integer := 7;  --RF_RESET_STATUS_REGTYPE:ADC_SEQ_DONE
  constant kADC_SEQ_DONE           : integer := 7;  --RF_RESET_STATUS_REGTYPE:ADC_SEQ_DONE
  constant kDAC_SEQ_DONESize       : integer :=  1;  --RF_RESET_STATUS_REGTYPE:DAC_SEQ_DONE
  constant kDAC_SEQ_DONEMsb        : integer := 11;  --RF_RESET_STATUS_REGTYPE:DAC_SEQ_DONE
  constant kDAC_SEQ_DONE           : integer := 11;  --RF_RESET_STATUS_REGTYPE:DAC_SEQ_DONE

  -- RFDC_INFO_MEMTYPE Type (from rfdc_info_pkg.sv)
  constant kRFDC_INFO_MEMTYPESize: integer := 32;
  constant kRFDC_INFO_MEMTYPEMask : std_logic_vector(31 downto 0) := X"00000fff";
  constant kBLOCK_MODESize       : integer := 2;  --RFDC_INFO_MEMTYPE:BLOCK_MODE
  constant kBLOCK_MODEMsb        : integer := 1;  --RFDC_INFO_MEMTYPE:BLOCK_MODE
  constant kBLOCK_MODE           : integer := 0;  --RFDC_INFO_MEMTYPE:BLOCK_MODE
  constant kBLOCKSize       : integer := 2;  --RFDC_INFO_MEMTYPE:BLOCK
  constant kBLOCKMsb        : integer := 3;  --RFDC_INFO_MEMTYPE:BLOCK
  constant kBLOCK           : integer := 2;  --RFDC_INFO_MEMTYPE:BLOCK
  constant kTILESize       : integer := 2;  --RFDC_INFO_MEMTYPE:TILE
  constant kTILEMsb        : integer := 5;  --RFDC_INFO_MEMTYPE:TILE
  constant kTILE           : integer := 4;  --RFDC_INFO_MEMTYPE:TILE
  constant kRESERVED2Size       : integer := 2;  --RFDC_INFO_MEMTYPE:RESERVED2
  constant kRESERVED2Msb        : integer := 7;  --RFDC_INFO_MEMTYPE:RESERVED2
  constant kRESERVED2           : integer := 6;  --RFDC_INFO_MEMTYPE:RESERVED2
  constant kCHANNELSize       : integer := 2;  --RFDC_INFO_MEMTYPE:CHANNEL
  constant kCHANNELMsb        : integer := 9;  --RFDC_INFO_MEMTYPE:CHANNEL
  constant kCHANNEL           : integer := 8;  --RFDC_INFO_MEMTYPE:CHANNEL
  constant kDBSize       : integer :=  1;  --RFDC_INFO_MEMTYPE:DB
  constant kDBMsb        : integer := 10;  --RFDC_INFO_MEMTYPE:DB
  constant kDB           : integer := 10;  --RFDC_INFO_MEMTYPE:DB
  constant kIS_ADCSize       : integer :=  1;  --RFDC_INFO_MEMTYPE:IS_ADC
  constant kIS_ADCMsb        : integer := 11;  --RFDC_INFO_MEMTYPE:IS_ADC
  constant kIS_ADC           : integer := 11;  --RFDC_INFO_MEMTYPE:IS_ADC

  -- RFDC_INFO_REGTYPE Type (from common_regs.v)
  constant kRFDC_INFO_REGTYPESize: integer := 32;
  constant kRFDC_INFO_REGTYPEMask : std_logic_vector(31 downto 0) := X"03ff03ff";
  constant kRFDC_INFO_XTRA_RESAMPSize       : integer := 4;  --RFDC_INFO_REGTYPE:RFDC_INFO_XTRA_RESAMP
  constant kRFDC_INFO_XTRA_RESAMPMsb        : integer := 3;  --RFDC_INFO_REGTYPE:RFDC_INFO_XTRA_RESAMP
  constant kRFDC_INFO_XTRA_RESAMP           : integer := 0;  --RFDC_INFO_REGTYPE:RFDC_INFO_XTRA_RESAMP
  constant kRFDC_INFO_SPC_RXSize       : integer := 3;  --RFDC_INFO_REGTYPE:RFDC_INFO_SPC_RX
  constant kRFDC_INFO_SPC_RXMsb        : integer := 6;  --RFDC_INFO_REGTYPE:RFDC_INFO_SPC_RX
  constant kRFDC_INFO_SPC_RX           : integer := 4;  --RFDC_INFO_REGTYPE:RFDC_INFO_SPC_RX
  constant kRFDC_INFO_SPC_TXSize       : integer := 3;  --RFDC_INFO_REGTYPE:RFDC_INFO_SPC_TX
  constant kRFDC_INFO_SPC_TXMsb        : integer := 9;  --RFDC_INFO_REGTYPE:RFDC_INFO_SPC_TX
  constant kRFDC_INFO_SPC_TX           : integer := 7;  --RFDC_INFO_REGTYPE:RFDC_INFO_SPC_TX
  constant kRFDC_INFO_XTRA_RESAMP_DB1Size       : integer :=  4;  --RFDC_INFO_REGTYPE:RFDC_INFO_XTRA_RESAMP_DB1
  constant kRFDC_INFO_XTRA_RESAMP_DB1Msb        : integer := 19;  --RFDC_INFO_REGTYPE:RFDC_INFO_XTRA_RESAMP_DB1
  constant kRFDC_INFO_XTRA_RESAMP_DB1           : integer := 16;  --RFDC_INFO_REGTYPE:RFDC_INFO_XTRA_RESAMP_DB1
  constant kRFDC_INFO_SPC_RX_DB1Size       : integer :=  3;  --RFDC_INFO_REGTYPE:RFDC_INFO_SPC_RX_DB1
  constant kRFDC_INFO_SPC_RX_DB1Msb        : integer := 22;  --RFDC_INFO_REGTYPE:RFDC_INFO_SPC_RX_DB1
  constant kRFDC_INFO_SPC_RX_DB1           : integer := 20;  --RFDC_INFO_REGTYPE:RFDC_INFO_SPC_RX_DB1
  constant kRFDC_INFO_SPC_TX_DB1Size       : integer :=  3;  --RFDC_INFO_REGTYPE:RFDC_INFO_SPC_TX_DB1
  constant kRFDC_INFO_SPC_TX_DB1Msb        : integer := 25;  --RFDC_INFO_REGTYPE:RFDC_INFO_SPC_TX_DB1
  constant kRFDC_INFO_SPC_TX_DB1           : integer := 23;  --RFDC_INFO_REGTYPE:RFDC_INFO_SPC_TX_DB1

--===============================================================================
-- Register Group RFDC_REGS
--===============================================================================

  -- Enumerated type FABRIC_DSP_BW_ENUM
  constant kFABRIC_DSP_BW_ENUMSize : integer := 5;
  constant kFABRIC_DSP_BW_NONE : integer := 0;  -- FABRIC_DSP_BW_ENUM:FABRIC_DSP_BW_NONE
  constant kFABRIC_DSP_BW_100M : integer := 100;  -- FABRIC_DSP_BW_ENUM:FABRIC_DSP_BW_100M
  constant kFABRIC_DSP_BW_200M : integer := 200;  -- FABRIC_DSP_BW_ENUM:FABRIC_DSP_BW_200M
  constant kFABRIC_DSP_BW_400M : integer := 400;  -- FABRIC_DSP_BW_ENUM:FABRIC_DSP_BW_400M
  constant kFABRIC_DSP_BW_FULL : integer := 1000;  -- FABRIC_DSP_BW_ENUM:FABRIC_DSP_BW_FULL

  -- Enumerated type RFDC_BLOCK_INFO_ENUM
  constant kRFDC_BLOCK_INFO_ENUMSize : integer := 2;
  constant kENABLED  : integer := 0;  -- RFDC_BLOCK_INFO_ENUM:ENABLED
  constant kDISABLED : integer := 3;  -- RFDC_BLOCK_INFO_ENUM:DISABLED

  -- MMCM Window (from x410_rfdc_regs.v)
  constant kMMCM : integer := 16#0#; -- Window Offset
  constant kMMCMSize: integer := 16#10000#;  -- size in bytes

  -- INVERT_DB0_IQ_REG Register (from x410_rfdc_regs.v)
  constant kINVERT_DB0_IQ_REG : integer := 16#10000#; -- Register Offset
  constant kINVERT_DB0_IQ_REGSize: integer := 32;  -- register width in bits
  constant kINVERT_DB0_IQ_REGMask : std_logic_vector(31 downto 0) := X"00000f0f";
  constant kINVERT_DB0_ADC0_IQSize       : integer := 1;  --INVERT_DB0_IQ_REG:INVERT_DB0_ADC0_IQ
  constant kINVERT_DB0_ADC0_IQMsb        : integer := 0;  --INVERT_DB0_IQ_REG:INVERT_DB0_ADC0_IQ
  constant kINVERT_DB0_ADC0_IQ           : integer := 0;  --INVERT_DB0_IQ_REG:INVERT_DB0_ADC0_IQ
  constant kINVERT_DB0_ADC1_IQSize       : integer := 1;  --INVERT_DB0_IQ_REG:INVERT_DB0_ADC1_IQ
  constant kINVERT_DB0_ADC1_IQMsb        : integer := 1;  --INVERT_DB0_IQ_REG:INVERT_DB0_ADC1_IQ
  constant kINVERT_DB0_ADC1_IQ           : integer := 1;  --INVERT_DB0_IQ_REG:INVERT_DB0_ADC1_IQ
  constant kINVERT_DB0_ADC2_IQSize       : integer := 1;  --INVERT_DB0_IQ_REG:INVERT_DB0_ADC2_IQ
  constant kINVERT_DB0_ADC2_IQMsb        : integer := 2;  --INVERT_DB0_IQ_REG:INVERT_DB0_ADC2_IQ
  constant kINVERT_DB0_ADC2_IQ           : integer := 2;  --INVERT_DB0_IQ_REG:INVERT_DB0_ADC2_IQ
  constant kINVERT_DB0_ADC3_IQSize       : integer := 1;  --INVERT_DB0_IQ_REG:INVERT_DB0_ADC3_IQ
  constant kINVERT_DB0_ADC3_IQMsb        : integer := 3;  --INVERT_DB0_IQ_REG:INVERT_DB0_ADC3_IQ
  constant kINVERT_DB0_ADC3_IQ           : integer := 3;  --INVERT_DB0_IQ_REG:INVERT_DB0_ADC3_IQ
  constant kINVERT_DB0_DAC0_IQSize       : integer := 1;  --INVERT_DB0_IQ_REG:INVERT_DB0_DAC0_IQ
  constant kINVERT_DB0_DAC0_IQMsb        : integer := 8;  --INVERT_DB0_IQ_REG:INVERT_DB0_DAC0_IQ
  constant kINVERT_DB0_DAC0_IQ           : integer := 8;  --INVERT_DB0_IQ_REG:INVERT_DB0_DAC0_IQ
  constant kINVERT_DB0_DAC1_IQSize       : integer := 1;  --INVERT_DB0_IQ_REG:INVERT_DB0_DAC1_IQ
  constant kINVERT_DB0_DAC1_IQMsb        : integer := 9;  --INVERT_DB0_IQ_REG:INVERT_DB0_DAC1_IQ
  constant kINVERT_DB0_DAC1_IQ           : integer := 9;  --INVERT_DB0_IQ_REG:INVERT_DB0_DAC1_IQ
  constant kINVERT_DB0_DAC2_IQSize       : integer :=  1;  --INVERT_DB0_IQ_REG:INVERT_DB0_DAC2_IQ
  constant kINVERT_DB0_DAC2_IQMsb        : integer := 10;  --INVERT_DB0_IQ_REG:INVERT_DB0_DAC2_IQ
  constant kINVERT_DB0_DAC2_IQ           : integer := 10;  --INVERT_DB0_IQ_REG:INVERT_DB0_DAC2_IQ
  constant kINVERT_DB0_DAC3_IQSize       : integer :=  1;  --INVERT_DB0_IQ_REG:INVERT_DB0_DAC3_IQ
  constant kINVERT_DB0_DAC3_IQMsb        : integer := 11;  --INVERT_DB0_IQ_REG:INVERT_DB0_DAC3_IQ
  constant kINVERT_DB0_DAC3_IQ           : integer := 11;  --INVERT_DB0_IQ_REG:INVERT_DB0_DAC3_IQ

  -- INVERT_DB1_IQ_REG Register (from x410_rfdc_regs.v)
  constant kINVERT_DB1_IQ_REG : integer := 16#10800#; -- Register Offset
  constant kINVERT_DB1_IQ_REGSize: integer := 32;  -- register width in bits
  constant kINVERT_DB1_IQ_REGMask : std_logic_vector(31 downto 0) := X"00000f0f";
  constant kINVERT_DB1_ADC0_IQSize       : integer := 1;  --INVERT_DB1_IQ_REG:INVERT_DB1_ADC0_IQ
  constant kINVERT_DB1_ADC0_IQMsb        : integer := 0;  --INVERT_DB1_IQ_REG:INVERT_DB1_ADC0_IQ
  constant kINVERT_DB1_ADC0_IQ           : integer := 0;  --INVERT_DB1_IQ_REG:INVERT_DB1_ADC0_IQ
  constant kINVERT_DB1_ADC1_IQSize       : integer := 1;  --INVERT_DB1_IQ_REG:INVERT_DB1_ADC1_IQ
  constant kINVERT_DB1_ADC1_IQMsb        : integer := 1;  --INVERT_DB1_IQ_REG:INVERT_DB1_ADC1_IQ
  constant kINVERT_DB1_ADC1_IQ           : integer := 1;  --INVERT_DB1_IQ_REG:INVERT_DB1_ADC1_IQ
  constant kINVERT_DB1_ADC2_IQSize       : integer := 1;  --INVERT_DB1_IQ_REG:INVERT_DB1_ADC2_IQ
  constant kINVERT_DB1_ADC2_IQMsb        : integer := 2;  --INVERT_DB1_IQ_REG:INVERT_DB1_ADC2_IQ
  constant kINVERT_DB1_ADC2_IQ           : integer := 2;  --INVERT_DB1_IQ_REG:INVERT_DB1_ADC2_IQ
  constant kINVERT_DB1_ADC3_IQSize       : integer := 1;  --INVERT_DB1_IQ_REG:INVERT_DB1_ADC3_IQ
  constant kINVERT_DB1_ADC3_IQMsb        : integer := 3;  --INVERT_DB1_IQ_REG:INVERT_DB1_ADC3_IQ
  constant kINVERT_DB1_ADC3_IQ           : integer := 3;  --INVERT_DB1_IQ_REG:INVERT_DB1_ADC3_IQ
  constant kINVERT_DB1_DAC0_IQSize       : integer := 1;  --INVERT_DB1_IQ_REG:INVERT_DB1_DAC0_IQ
  constant kINVERT_DB1_DAC0_IQMsb        : integer := 8;  --INVERT_DB1_IQ_REG:INVERT_DB1_DAC0_IQ
  constant kINVERT_DB1_DAC0_IQ           : integer := 8;  --INVERT_DB1_IQ_REG:INVERT_DB1_DAC0_IQ
  constant kINVERT_DB1_DAC1_IQSize       : integer := 1;  --INVERT_DB1_IQ_REG:INVERT_DB1_DAC1_IQ
  constant kINVERT_DB1_DAC1_IQMsb        : integer := 9;  --INVERT_DB1_IQ_REG:INVERT_DB1_DAC1_IQ
  constant kINVERT_DB1_DAC1_IQ           : integer := 9;  --INVERT_DB1_IQ_REG:INVERT_DB1_DAC1_IQ
  constant kINVERT_DB1_DAC2_IQSize       : integer :=  1;  --INVERT_DB1_IQ_REG:INVERT_DB1_DAC2_IQ
  constant kINVERT_DB1_DAC2_IQMsb        : integer := 10;  --INVERT_DB1_IQ_REG:INVERT_DB1_DAC2_IQ
  constant kINVERT_DB1_DAC2_IQ           : integer := 10;  --INVERT_DB1_IQ_REG:INVERT_DB1_DAC2_IQ
  constant kINVERT_DB1_DAC3_IQSize       : integer :=  1;  --INVERT_DB1_IQ_REG:INVERT_DB1_DAC3_IQ
  constant kINVERT_DB1_DAC3_IQMsb        : integer := 11;  --INVERT_DB1_IQ_REG:INVERT_DB1_DAC3_IQ
  constant kINVERT_DB1_DAC3_IQ           : integer := 11;  --INVERT_DB1_IQ_REG:INVERT_DB1_DAC3_IQ

  -- MMCM_RESET_REG Register (from x410_rfdc_regs.v)
  constant kMMCM_RESET_REG : integer := 16#11000#; -- Register Offset
  constant kMMCM_RESET_REGSize: integer := 32;  -- register width in bits
  constant kMMCM_RESET_REGMask : std_logic_vector(31 downto 0) := X"00000001";
  constant kRESET_MMCMSize       : integer := 1;  --MMCM_RESET_REG:RESET_MMCM
  constant kRESET_MMCMMsb        : integer := 0;  --MMCM_RESET_REG:RESET_MMCM
  constant kRESET_MMCM           : integer := 0;  --MMCM_RESET_REG:RESET_MMCM

  -- RF_RESET_CONTROL_REG Register (from x410_rfdc_regs.v)
  constant kRF_RESET_CONTROL_REG : integer := 16#12000#; -- Register Offset
  constant kRF_RESET_CONTROL_REGSize: integer := 32;  -- register width in bits

  -- RF_RESET_STATUS_REG Register (from x410_rfdc_regs.v)
  constant kRF_RESET_STATUS_REG : integer := 16#12008#; -- Register Offset
  constant kRF_RESET_STATUS_REGSize: integer := 32;  -- register width in bits

  -- RF_AXI_STATUS_REG Register (from x410_rfdc_regs.v)
  constant kRF_AXI_STATUS_REG : integer := 16#13000#; -- Register Offset
  constant kRF_AXI_STATUS_REGSize: integer := 32;  -- register width in bits

  -- FABRIC_DSP_REG Register (from x410_rfdc_regs.v)
  constant kFABRIC_DSP_REG : integer := 16#13008#; -- Register Offset
  constant kFABRIC_DSP_REGSize: integer := 32;  -- register width in bits

  -- CALIBRATION_DATA Register (from x410_rfdc_regs.v)
  constant kCALIBRATION_DATA : integer := 16#14000#; -- Register Offset
  constant kCALIBRATION_DATASize: integer := 32;  -- register width in bits
  constant kCALIBRATION_DATAMask : std_logic_vector(31 downto 0) := X"ffffffff";
  constant kI_DATASize       : integer := 16;  --CALIBRATION_DATA:I_DATA
  constant kI_DATAMsb        : integer := 15;  --CALIBRATION_DATA:I_DATA
  constant kI_DATA           : integer :=  0;  --CALIBRATION_DATA:I_DATA
  constant kQ_DATASize       : integer := 16;  --CALIBRATION_DATA:Q_DATA
  constant kQ_DATAMsb        : integer := 31;  --CALIBRATION_DATA:Q_DATA
  constant kQ_DATA           : integer := 16;  --CALIBRATION_DATA:Q_DATA

  -- CALIBRATION_ENABLE Register (from x410_rfdc_regs.v)
  constant kCALIBRATION_ENABLE : integer := 16#14008#; -- Register Offset
  constant kCALIBRATION_ENABLESize: integer := 32;  -- register width in bits
  constant kCALIBRATION_ENABLEMask : std_logic_vector(31 downto 0) := X"00000033";
  constant kENABLE_CALIBRATION_DATA_0Size       : integer := 1;  --CALIBRATION_ENABLE:ENABLE_CALIBRATION_DATA_0
  constant kENABLE_CALIBRATION_DATA_0Msb        : integer := 0;  --CALIBRATION_ENABLE:ENABLE_CALIBRATION_DATA_0
  constant kENABLE_CALIBRATION_DATA_0           : integer := 0;  --CALIBRATION_ENABLE:ENABLE_CALIBRATION_DATA_0
  constant kENABLE_CALIBRATION_DATA_1Size       : integer := 1;  --CALIBRATION_ENABLE:ENABLE_CALIBRATION_DATA_1
  constant kENABLE_CALIBRATION_DATA_1Msb        : integer := 1;  --CALIBRATION_ENABLE:ENABLE_CALIBRATION_DATA_1
  constant kENABLE_CALIBRATION_DATA_1           : integer := 1;  --CALIBRATION_ENABLE:ENABLE_CALIBRATION_DATA_1
  constant kENABLE_CALIBRATION_DATA_2Size       : integer := 1;  --CALIBRATION_ENABLE:ENABLE_CALIBRATION_DATA_2
  constant kENABLE_CALIBRATION_DATA_2Msb        : integer := 4;  --CALIBRATION_ENABLE:ENABLE_CALIBRATION_DATA_2
  constant kENABLE_CALIBRATION_DATA_2           : integer := 4;  --CALIBRATION_ENABLE:ENABLE_CALIBRATION_DATA_2
  constant kENABLE_CALIBRATION_DATA_3Size       : integer := 1;  --CALIBRATION_ENABLE:ENABLE_CALIBRATION_DATA_3
  constant kENABLE_CALIBRATION_DATA_3Msb        : integer := 5;  --CALIBRATION_ENABLE:ENABLE_CALIBRATION_DATA_3
  constant kENABLE_CALIBRATION_DATA_3           : integer := 5;  --CALIBRATION_ENABLE:ENABLE_CALIBRATION_DATA_3

  -- THRESHOLD_STATUS Register (from x410_rfdc_regs.v)
  constant kTHRESHOLD_STATUS : integer := 16#15000#; -- Register Offset
  constant kTHRESHOLD_STATUSSize: integer := 32;  -- register width in bits
  constant kTHRESHOLD_STATUSMask : std_logic_vector(31 downto 0) := X"00000f0f";
  constant kADC0_01_THRESHOLD1Size       : integer := 1;  --THRESHOLD_STATUS:ADC0_01_THRESHOLD1
  constant kADC0_01_THRESHOLD1Msb        : integer := 0;  --THRESHOLD_STATUS:ADC0_01_THRESHOLD1
  constant kADC0_01_THRESHOLD1           : integer := 0;  --THRESHOLD_STATUS:ADC0_01_THRESHOLD1
  constant kADC0_01_THRESHOLD2Size       : integer := 1;  --THRESHOLD_STATUS:ADC0_01_THRESHOLD2
  constant kADC0_01_THRESHOLD2Msb        : integer := 1;  --THRESHOLD_STATUS:ADC0_01_THRESHOLD2
  constant kADC0_01_THRESHOLD2           : integer := 1;  --THRESHOLD_STATUS:ADC0_01_THRESHOLD2
  constant kADC0_23_THRESHOLD1Size       : integer := 1;  --THRESHOLD_STATUS:ADC0_23_THRESHOLD1
  constant kADC0_23_THRESHOLD1Msb        : integer := 2;  --THRESHOLD_STATUS:ADC0_23_THRESHOLD1
  constant kADC0_23_THRESHOLD1           : integer := 2;  --THRESHOLD_STATUS:ADC0_23_THRESHOLD1
  constant kADC0_23_THRESHOLD2Size       : integer := 1;  --THRESHOLD_STATUS:ADC0_23_THRESHOLD2
  constant kADC0_23_THRESHOLD2Msb        : integer := 3;  --THRESHOLD_STATUS:ADC0_23_THRESHOLD2
  constant kADC0_23_THRESHOLD2           : integer := 3;  --THRESHOLD_STATUS:ADC0_23_THRESHOLD2
  constant kADC2_01_THRESHOLD1Size       : integer := 1;  --THRESHOLD_STATUS:ADC2_01_THRESHOLD1
  constant kADC2_01_THRESHOLD1Msb        : integer := 8;  --THRESHOLD_STATUS:ADC2_01_THRESHOLD1
  constant kADC2_01_THRESHOLD1           : integer := 8;  --THRESHOLD_STATUS:ADC2_01_THRESHOLD1
  constant kADC2_01_THRESHOLD2Size       : integer := 1;  --THRESHOLD_STATUS:ADC2_01_THRESHOLD2
  constant kADC2_01_THRESHOLD2Msb        : integer := 9;  --THRESHOLD_STATUS:ADC2_01_THRESHOLD2
  constant kADC2_01_THRESHOLD2           : integer := 9;  --THRESHOLD_STATUS:ADC2_01_THRESHOLD2
  constant kADC2_23_THRESHOLD1Size       : integer :=  1;  --THRESHOLD_STATUS:ADC2_23_THRESHOLD1
  constant kADC2_23_THRESHOLD1Msb        : integer := 10;  --THRESHOLD_STATUS:ADC2_23_THRESHOLD1
  constant kADC2_23_THRESHOLD1           : integer := 10;  --THRESHOLD_STATUS:ADC2_23_THRESHOLD1
  constant kADC2_23_THRESHOLD2Size       : integer :=  1;  --THRESHOLD_STATUS:ADC2_23_THRESHOLD2
  constant kADC2_23_THRESHOLD2Msb        : integer := 11;  --THRESHOLD_STATUS:ADC2_23_THRESHOLD2
  constant kADC2_23_THRESHOLD2           : integer := 11;  --THRESHOLD_STATUS:ADC2_23_THRESHOLD2

  -- RF_PLL_CONTROL_REG Register (from x410_rfdc_regs.v)
  constant kRF_PLL_CONTROL_REG : integer := 16#16000#; -- Register Offset
  constant kRF_PLL_CONTROL_REGSize: integer := 32;  -- register width in bits
  constant kRF_PLL_CONTROL_REGMask : std_logic_vector(31 downto 0) := X"00011111";
  constant kENABLE_DATA_CLKSize       : integer := 1;  --RF_PLL_CONTROL_REG:ENABLE_DATA_CLK
  constant kENABLE_DATA_CLKMsb        : integer := 0;  --RF_PLL_CONTROL_REG:ENABLE_DATA_CLK
  constant kENABLE_DATA_CLK           : integer := 0;  --RF_PLL_CONTROL_REG:ENABLE_DATA_CLK
  constant kENABLE_DATA_CLK_2XSize       : integer := 1;  --RF_PLL_CONTROL_REG:ENABLE_DATA_CLK_2X
  constant kENABLE_DATA_CLK_2XMsb        : integer := 4;  --RF_PLL_CONTROL_REG:ENABLE_DATA_CLK_2X
  constant kENABLE_DATA_CLK_2X           : integer := 4;  --RF_PLL_CONTROL_REG:ENABLE_DATA_CLK_2X
  constant kENABLE_RF_CLKSize       : integer := 1;  --RF_PLL_CONTROL_REG:ENABLE_RF_CLK
  constant kENABLE_RF_CLKMsb        : integer := 8;  --RF_PLL_CONTROL_REG:ENABLE_RF_CLK
  constant kENABLE_RF_CLK           : integer := 8;  --RF_PLL_CONTROL_REG:ENABLE_RF_CLK
  constant kENABLE_RF_CLK_2XSize       : integer :=  1;  --RF_PLL_CONTROL_REG:ENABLE_RF_CLK_2X
  constant kENABLE_RF_CLK_2XMsb        : integer := 12;  --RF_PLL_CONTROL_REG:ENABLE_RF_CLK_2X
  constant kENABLE_RF_CLK_2X           : integer := 12;  --RF_PLL_CONTROL_REG:ENABLE_RF_CLK_2X
  constant kCLEAR_DATA_CLK_UNLOCKEDSize       : integer :=  1;  --RF_PLL_CONTROL_REG:CLEAR_DATA_CLK_UNLOCKED
  constant kCLEAR_DATA_CLK_UNLOCKEDMsb        : integer := 16;  --RF_PLL_CONTROL_REG:CLEAR_DATA_CLK_UNLOCKED
  constant kCLEAR_DATA_CLK_UNLOCKED           : integer := 16;  --RF_PLL_CONTROL_REG:CLEAR_DATA_CLK_UNLOCKED

  -- RF_PLL_STATUS_REG Register (from x410_rfdc_regs.v)
  constant kRF_PLL_STATUS_REG : integer := 16#16008#; -- Register Offset
  constant kRF_PLL_STATUS_REGSize: integer := 32;  -- register width in bits
  constant kRF_PLL_STATUS_REGMask : std_logic_vector(31 downto 0) := X"00110000";
  constant kDATA_CLK_PLL_UNLOCKED_STICKYSize       : integer :=  1;  --RF_PLL_STATUS_REG:DATA_CLK_PLL_UNLOCKED_STICKY
  constant kDATA_CLK_PLL_UNLOCKED_STICKYMsb        : integer := 16;  --RF_PLL_STATUS_REG:DATA_CLK_PLL_UNLOCKED_STICKY
  constant kDATA_CLK_PLL_UNLOCKED_STICKY           : integer := 16;  --RF_PLL_STATUS_REG:DATA_CLK_PLL_UNLOCKED_STICKY
  constant kDATA_CLK_PLL_LOCKEDSize       : integer :=  1;  --RF_PLL_STATUS_REG:DATA_CLK_PLL_LOCKED
  constant kDATA_CLK_PLL_LOCKEDMsb        : integer := 20;  --RF_PLL_STATUS_REG:DATA_CLK_PLL_LOCKED
  constant kDATA_CLK_PLL_LOCKED           : integer := 20;  --RF_PLL_STATUS_REG:DATA_CLK_PLL_LOCKED

  -- RFDC_INFO_MEM Register (from x410_rfdc_regs.v)
  function kRFDC_INFO_MEM (i:integer) return integer; -- Register Offset function
  constant kRFDC_INFO_MEMCount : integer := 16; -- Number of elements in array
  constant kRFDC_INFO_MEMSize: integer := 32; -- Register bit width (not array size)

  -- RFDC_INFO_REG Register (from x410_rfdc_regs.v)
  constant kRFDC_INFO_REG : integer := 16#18000#; -- Register Offset
  constant kRFDC_INFO_REGSize: integer := 32;  -- register width in bits

end package;

package body PkgRFDC_REGS_REGMAP is














  -- Return the offset of an element of register array kRFDC_INFO_MEM
  function kRFDC_INFO_MEM (i:integer) return integer is
  begin
    --synopsys translate_off
    assert i>=0 and i<=15 report "kRFDC_INFO_MEM i=" & integer'image(i) & " is out of range" severity error;
    --synopsys translate_on
    return (i * 4) + 16#17000#;
  end function kRFDC_INFO_MEM;



end package body;
