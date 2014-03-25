////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1995-2012 Xilinx, Inc.  All rights reserved.
////////////////////////////////////////////////////////////////////////////////
//   ____  ____
//  /   /\/   /
// /___/  \  /    Vendor: Xilinx
// \   \   \/     Version: P.49d
//  \   \         Application: netgen
//  /   /         Filename: gige_sfp_mdio.v
// /___/   /\     Timestamp: Fri Aug 16 14:13:06 2013
// \   \  /  \ 
//  \___\/\___\
//             
// Command	: -w -sim -ofmt verilog /home/ianb/fpgadev/usrp3/top/x300/coregen/tmp/_cg/gige_sfp_mdio.ngc /home/ianb/fpgadev/usrp3/top/x300/coregen/tmp/_cg/gige_sfp_mdio.v 
// Device	: 7k410tffg900-2
// Input file	: /home/ianb/fpgadev/usrp3/top/x300/coregen/tmp/_cg/gige_sfp_mdio.ngc
// Output file	: /home/ianb/fpgadev/usrp3/top/x300/coregen/tmp/_cg/gige_sfp_mdio.v
// # of Modules	: 1
// Design Name	: gige_sfp_mdio
// Xilinx        : /opt/Xilinx/14.4/ISE_DS/ISE/
//             
// Purpose:    
//     This verilog netlist is a verification model and uses simulation 
//     primitives which may not represent the true implementation of the 
//     device, however the netlist is functionally correct and should not 
//     be modified. This file cannot be synthesized and should only be used 
//     with supported simulation tools.
//             
// Reference:  
//     Command Line Tools User Guide, Chapter 23 and Synthesis and Simulation Design Guide, Chapter 6
//             
////////////////////////////////////////////////////////////////////////////////

`timescale 1 ns/1 ps

module gige_sfp_mdio (
  reset, signal_detect, userclk, userclk2, dcm_locked, txbuferr, gmii_tx_en, gmii_tx_er, mdc, mdio_in, configuration_valid, mgt_rx_reset, mgt_tx_reset
, powerdown, txchardispmode, txchardispval, txcharisk, enablealign, gmii_rx_dv, gmii_rx_er, gmii_isolate, mdio_out, mdio_tri, rxbufstatus, 
rxchariscomma, rxcharisk, rxclkcorcnt, rxdata, rxdisperr, rxnotintable, rxrundisp, gmii_txd, phyad, configuration_vector, txdata, gmii_rxd, 
status_vector
)/* synthesis syn_black_box syn_noprune=1 */;
  input reset;
  input signal_detect;
  input userclk;
  input userclk2;
  input dcm_locked;
  input txbuferr;
  input gmii_tx_en;
  input gmii_tx_er;
  input mdc;
  input mdio_in;
  input configuration_valid;
  output mgt_rx_reset;
  output mgt_tx_reset;
  output powerdown;
  output txchardispmode;
  output txchardispval;
  output txcharisk;
  output enablealign;
  output gmii_rx_dv;
  output gmii_rx_er;
  output gmii_isolate;
  output mdio_out;
  output mdio_tri;
  input [1 : 0] rxbufstatus;
  input [0 : 0] rxchariscomma;
  input [0 : 0] rxcharisk;
  input [2 : 0] rxclkcorcnt;
  input [7 : 0] rxdata;
  input [0 : 0] rxdisperr;
  input [0 : 0] rxnotintable;
  input [0 : 0] rxrundisp;
  input [7 : 0] gmii_txd;
  input [4 : 0] phyad;
  input [4 : 0] configuration_vector;
  output [7 : 0] txdata;
  output [7 : 0] gmii_rxd;
  output [15 : 0] status_vector;
  
  // synthesis translate_off
  
  wire \U0/gpcs_pma_inst/SYNC_SIGNAL_DETECT/data_in ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/SYNC_MDC/data_in ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/SYNC_MDIO_IN/data_in ;
  wire \U0/gpcs_pma_inst/RXNOTINTABLE_REG_60 ;
  wire \U0/gpcs_pma_inst/RXDISPERR_REG_61 ;
  wire \NlwRenamedSig_OI_U0/gpcs_pma_inst/RECEIVER/RX_INVALID ;
  wire \U0/gpcs_pma_inst/RECEIVER/RUDI_I_63 ;
  wire \U0/gpcs_pma_inst/RECEIVER/RUDI_C_64 ;
  wire \NlwRenamedSignal_U0/gpcs_pma_inst/STATUS_VECTOR_0 ;
  wire \NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ;
  wire \NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ;
  wire \NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/POWERDOWN_REG ;
  wire \U0/gpcs_pma_inst/TXCHARDISPMODE_69 ;
  wire \U0/gpcs_pma_inst/TXCHARDISPVAL_70 ;
  wire \U0/gpcs_pma_inst/TXCHARISK_71 ;
  wire \NlwRenamedSig_OI_U0/gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN ;
  wire \NlwRenamedSig_OI_U0/gpcs_pma_inst/RECEIVER/RX_DV ;
  wire \U0/gpcs_pma_inst/RECEIVER/RX_ER_74 ;
  wire \NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_OUT_76 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_TRI_77 ;
  wire N0;
  wire \U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd1_79 ;
  wire \U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd2_80 ;
  wire \U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd3_81 ;
  wire \U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd4_82 ;
  wire \U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd1-In ;
  wire \U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd2-In ;
  wire \U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd3-In ;
  wire \U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd1_86 ;
  wire \U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd2_87 ;
  wire \U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd3_88 ;
  wire \U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd4_89 ;
  wire \U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd1-In ;
  wire \U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd2-In ;
  wire \U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd3-In ;
  wire \U0/gpcs_pma_inst/SYNC_SIGNAL_DETECT/data_sync1 ;
  wire \U0/gpcs_pma_inst/SRESET_PIPE_PWR_15_o_MUX_1_o ;
  wire \U0/gpcs_pma_inst/RESET_INT_RXBUFSTATUS_INT[1]_OR_126_o ;
  wire \U0/gpcs_pma_inst/RESET_INT_TXBUFERR_INT_OR_125_o ;
  wire \U0/gpcs_pma_inst/TXCHARDISPVAL_INT_GND_15_o_MUX_288_o ;
  wire \U0/gpcs_pma_inst/TXCHARDISPMODE_INT_TXEVEN_MUX_287_o ;
  wire \U0/gpcs_pma_inst/TXCHARISK_INT_TXEVEN_MUX_286_o ;
  wire \U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<0> ;
  wire \U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<1> ;
  wire \U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<2> ;
  wire \U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<3> ;
  wire \U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<4> ;
  wire \U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<5> ;
  wire \U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<6> ;
  wire \U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<7> ;
  wire \U0/gpcs_pma_inst/RX_RST_SM[3]_GND_15_o_Mux_13_o ;
  wire \U0/gpcs_pma_inst/TX_RST_SM[3]_GND_15_o_Mux_9_o ;
  wire \U0/gpcs_pma_inst/TXBUFERR_INT_110 ;
  wire \U0/gpcs_pma_inst/RXNOTINTABLE_INT_115 ;
  wire \U0/gpcs_pma_inst/RXDISPERR_INT_116 ;
  wire \U0/gpcs_pma_inst/RXCHARISK_INT_125 ;
  wire \U0/gpcs_pma_inst/RXCHARISCOMMA_INT_126 ;
  wire \U0/gpcs_pma_inst/SRESET_127 ;
  wire \U0/gpcs_pma_inst/SRESET_PIPE_128 ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_129 ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/EVEN_130 ;
  wire \U0/gpcs_pma_inst/RXNOTINTABLE_SRL ;
  wire \U0/gpcs_pma_inst/RXDISPERR_SRL ;
  wire \U0/gpcs_pma_inst/RESET_INT_PIPE_133 ;
  wire \U0/gpcs_pma_inst/RESET_INT_134 ;
  wire \U0/gpcs_pma_inst/SIGNAL_DETECT_REG ;
  wire \U0/gpcs_pma_inst/DCM_LOCKED_SOFT_RESET_OR_2_o ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/RESET_REG_138 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/TXCHARDISPVAL_139 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/TXCHARDISPMODE_140 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/TXCHARISK_141 ;
  wire \U0/gpcs_pma_inst/RXNOTINTABLE[0]_GND_15_o_MUX_276_o ;
  wire \U0/gpcs_pma_inst/RXDISPERR[0]_GND_15_o_MUX_277_o ;
  wire \U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<0> ;
  wire \U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<1> ;
  wire \U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<2> ;
  wire \U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<3> ;
  wire \U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<4> ;
  wire \U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<5> ;
  wire \U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<6> ;
  wire \U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<7> ;
  wire \U0/gpcs_pma_inst/RXCLKCORCNT[2]_GND_15_o_mux_18_OUT<0> ;
  wire \U0/gpcs_pma_inst/RXCLKCORCNT[2]_GND_15_o_mux_18_OUT<1> ;
  wire \U0/gpcs_pma_inst/RXCLKCORCNT[2]_GND_15_o_mux_18_OUT<2> ;
  wire \U0/gpcs_pma_inst/RXCHARISK[0]_TXCHARISK_INT_MUX_279_o ;
  wire \U0/gpcs_pma_inst/RXCHARISCOMMA[0]_TXCHARISK_INT_MUX_280_o ;
  wire \U0/gpcs_pma_inst/RXBUFSTATUS[1]_GND_15_o_mux_17_OUT<1> ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT511 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/Mram_CODE_GRP_CNT[1]_GND_26_o_Mux_5_o ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/TX_EN_TRIGGER_S_OR_25_o_0 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT[1]_TX_CONFIG[15]_wide_mux_4_OUT<7> ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/DISP5 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/TX_EN_TRIGGER_T_OR_27_o ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/TX_EN_EVEN_AND_25_o ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_CODE_GRP_CNT[1]_MUX_186_o ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_GND_26_o_MUX_192_o ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<0> ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<1> ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<2> ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<3> ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<4> ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<5> ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<6> ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<7> ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_EVEN_AND_59_o ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<0> ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<1> ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<2> ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<3> ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<4> ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<5> ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<6> ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<7> ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/DISPARITY_196 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/V_197 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/R_198 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_199 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_200 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/XMIT_DATA_INT_201 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/C1_OR_C2_202 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_204 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_205 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/TRIGGER_T_206 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/T_207 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/TRIGGER_S_208 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/S_209 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/TX_ER_REG1_222 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_223 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/SYNC_MDC/data_sync1 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/SYNC_MDIO_IN/data_sync1 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/_n0162_inv ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/UNIDIRECTIONAL_ENABLE_REG_DATA_WR[5]_MUX_126_o ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG_DATA_WR[10]_MUX_124_o ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_DATA_WR[14]_MUX_120_o ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/GND_22_o_MDIO_WE_AND_14_o ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_IN_REG2 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_REG2 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/UNIDIRECTIONAL_ENABLE_REG_242 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_IN_REG4_243 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_IN_REG3_244 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_REG3_245 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/CONFIGURATION_VALID_EN_REG_246 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/CONFIGURATION_VALID_EN_247 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_248 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/WE_249 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG3_250 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_IN_REG_258 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT_xor<3>14 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3-In1 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mmux_STATE[3]_PWR_20_o_Mux_36_o11_270 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB2_271 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In1 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux1011 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT3 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT2 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT1 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0141_inv ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1-In ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2-In ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3-In_281 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0147_inv_283 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0155_inv ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE[3]_PWR_20_o_Mux_37_o ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE[3]_PWR_20_o_Mux_36_o ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/GND_24_o_GND_24_o_MUX_62_o ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG1_PWR_20_o_AND_3_o ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_IN_LAST_DATA_SHIFT_OR_9_o ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/LAST_DATA_SHIFT_302 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1_306 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_307 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG2_308 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG1_309 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<0> ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<1> ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<2> ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<3> ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<4> ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<5> ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<6> ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<7> ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<8> ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<9> ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<10> ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<11> ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<12> ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<13> ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<14> ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<15> ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In1_0 ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_327 ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_328 ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_329 ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_330 ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In2_331 ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1-In2 ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2-In2 ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3-In3 ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/_n0103_inv ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_PWR_23_o_equal_19_o ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_GND_27_o_mux_30_OUT<0> ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_GND_27_o_mux_30_OUT<1> ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/CGBAD_GND_27_o_AND_69_o ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/CGBAD ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/SIGNAL_DETECT_REG_343 ;
  wire \U0/gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_128_o1_344 ;
  wire \U0/gpcs_pma_inst/RECEIVER/K28p51_345 ;
  wire \U0/gpcs_pma_inst/RECEIVER/C_REG2_346 ;
  wire \U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_D21p5_AND_133_o_norst ;
  wire \U0/gpcs_pma_inst/RECEIVER/IDLE_REG[1]_IDLE_REG[2]_OR_124_o ;
  wire \U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG[0]_RX_CONFIG_VALID_REG[3]_OR_123_o ;
  wire \U0/gpcs_pma_inst/RECEIVER/C_REG1_C_REG3_OR_69_o_350 ;
  wire \U0/gpcs_pma_inst/RECEIVER/I_REG_T_REG2_OR_74_o ;
  wire \U0/gpcs_pma_inst/RECEIVER/D0p0_352 ;
  wire \U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o ;
  wire \U0/gpcs_pma_inst/RECEIVER/EXTEND_REG3_EXT_ILLEGAL_K_REG2_OR_93_o ;
  wire \U0/gpcs_pma_inst/RECEIVER/EOP_REG1_SYNC_STATUS_OR_97_o ;
  wire \U0/gpcs_pma_inst/RECEIVER/EOP_EXTEND_OR_75_o ;
  wire \U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<0> ;
  wire \U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<1> ;
  wire \U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<2> ;
  wire \U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<3> ;
  wire \U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<4> ;
  wire \U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<5> ;
  wire \U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<6> ;
  wire \U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<7> ;
  wire \U0/gpcs_pma_inst/RECEIVER/RXCHARISK_REG1_K28p5_REG1_AND_184_o ;
  wire \U0/gpcs_pma_inst/RECEIVER/S_WAIT_FOR_K_AND_161_o ;
  wire \U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1_ISOLATE_AND_199_o_367 ;
  wire \U0/gpcs_pma_inst/RECEIVER/SYNC_STATUS_C_REG1_AND_142_o_368 ;
  wire \U0/gpcs_pma_inst/RECEIVER/EVEN_RXCHARISK_AND_132_o_369 ;
  wire \U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_EVEN_AND_144_o ;
  wire \U0/gpcs_pma_inst/RECEIVER/K28p5 ;
  wire \U0/gpcs_pma_inst/RECEIVER/RXDATA[7]_RXNOTINTABLE_AND_228_o ;
  wire \U0/gpcs_pma_inst/RECEIVER/K23p7 ;
  wire \U0/gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_128_o ;
  wire \U0/gpcs_pma_inst/RECEIVER/K29p7 ;
  wire \U0/gpcs_pma_inst/RECEIVER/RXFIFO_ERR_RXDISPERR_OR_46_o ;
  wire \U0/gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_61_o ;
  wire \U0/gpcs_pma_inst/RECEIVER/EXTEND_378 ;
  wire \U0/gpcs_pma_inst/RECEIVER/RECEIVE_379 ;
  wire \U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_380 ;
  wire \U0/gpcs_pma_inst/RECEIVER/WAIT_FOR_K_381 ;
  wire \U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_389 ;
  wire \U0/gpcs_pma_inst/RECEIVER/FALSE_K_390 ;
  wire \U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_391 ;
  wire \U0/gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_REG2_392 ;
  wire \U0/gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_REG1_393 ;
  wire \U0/gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_394 ;
  wire \U0/gpcs_pma_inst/RECEIVER/EXTEND_ERR_395 ;
  wire \U0/gpcs_pma_inst/RECEIVER/ILLEGAL_K_REG2_396 ;
  wire \U0/gpcs_pma_inst/RECEIVER/ILLEGAL_K_REG1_397 ;
  wire \U0/gpcs_pma_inst/RECEIVER/ILLEGAL_K_398 ;
  wire \U0/gpcs_pma_inst/RECEIVER/RX_DATA_ERROR_399 ;
  wire \U0/gpcs_pma_inst/RECEIVER/EOP_REG1_400 ;
  wire \U0/gpcs_pma_inst/RECEIVER/EOP_401 ;
  wire \U0/gpcs_pma_inst/RECEIVER/SOP_402 ;
  wire \U0/gpcs_pma_inst/RECEIVER/FROM_RX_CX_403 ;
  wire \U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_405 ;
  wire \U0/gpcs_pma_inst/RECEIVER/SYNC_STATUS_REG_406 ;
  wire \U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_INT_407 ;
  wire \U0/gpcs_pma_inst/RECEIVER/CGBAD_REG3_408 ;
  wire \U0/gpcs_pma_inst/RECEIVER/CGBAD_409 ;
  wire \U0/gpcs_pma_inst/RECEIVER/R_410 ;
  wire \U0/gpcs_pma_inst/RECEIVER/EXTEND_REG3_419 ;
  wire \U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1_420 ;
  wire \U0/gpcs_pma_inst/RECEIVER/SOP_REG3_421 ;
  wire \U0/gpcs_pma_inst/RECEIVER/SOP_REG2_422 ;
  wire \U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG2 ;
  wire \U0/gpcs_pma_inst/RECEIVER/C_HDR_REMOVED_REG_424 ;
  wire \U0/gpcs_pma_inst/RECEIVER/CGBAD_REG2 ;
  wire \U0/gpcs_pma_inst/RECEIVER/RXCHARISK_REG1_426 ;
  wire \U0/gpcs_pma_inst/RECEIVER/C_REG3_427 ;
  wire \U0/gpcs_pma_inst/RECEIVER/C_REG1_428 ;
  wire \U0/gpcs_pma_inst/RECEIVER/I_REG_429 ;
  wire \U0/gpcs_pma_inst/RECEIVER/R_REG1_430 ;
  wire \U0/gpcs_pma_inst/RECEIVER/T_REG2_431 ;
  wire \U0/gpcs_pma_inst/RECEIVER/T_REG1_432 ;
  wire \U0/gpcs_pma_inst/RECEIVER/D0p0_REG_433 ;
  wire \U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_434 ;
  wire \U0/gpcs_pma_inst/RECEIVER/C_435 ;
  wire \U0/gpcs_pma_inst/RECEIVER/I_436 ;
  wire \U0/gpcs_pma_inst/RECEIVER/T_437 ;
  wire \U0/gpcs_pma_inst/RECEIVER/S_438 ;
  wire N2;
  wire N6;
  wire N8;
  wire \U0/gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_XMIT_DATA_INT_AND_37_o1_442 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_XMIT_DATA_INT_AND_37_o2_443 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT2 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT1 ;
  wire N10;
  wire N12;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB1_448 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB3_449 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB4_450 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB5_451 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB6_452 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB7_453 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB8_454 ;
  wire N14;
  wire N16;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In3_457 ;
  wire N18;
  wire N20;
  wire N22;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3-In31_461 ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2-In21_462 ;
  wire N28;
  wire N32;
  wire N34;
  wire \U0/gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_48_o ;
  wire \U0/gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_48_o1_467 ;
  wire N36;
  wire N38;
  wire \U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o1_470 ;
  wire \U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o2_471 ;
  wire \U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o3_472 ;
  wire \U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o4_473 ;
  wire \U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_118_o1 ;
  wire \U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_118_o12_475 ;
  wire \U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_118_o13_476 ;
  wire \U0/gpcs_pma_inst/RECEIVER/I_REG_T_REG2_OR_74_o1_477 ;
  wire \U0/gpcs_pma_inst/RECEIVER/T_REG2_R_REG1_OR_89_o2_478 ;
  wire N40;
  wire \U0/gpcs_pma_inst/TRANSMITTER/V_glue_set_480 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_glue_set_481 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/R_glue_set_482 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/DISPARITY_glue_rst_483 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/POWERDOWN_REG_glue_set_484 ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN_glue_set_485 ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_glue_rst_486 ;
  wire \U0/gpcs_pma_inst/SYNCHRONISATION/EVEN_glue_set_487 ;
  wire \U0/gpcs_pma_inst/RECEIVER/RECEIVE_glue_set_488 ;
  wire \U0/gpcs_pma_inst/RECEIVER/RX_INVALID_glue_set_489 ;
  wire \U0/gpcs_pma_inst/RECEIVER/RX_DV_glue_set_490 ;
  wire \U0/gpcs_pma_inst/RECEIVER/EXTEND_glue_set_491 ;
  wire \U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_glue_set_492 ;
  wire \U0/gpcs_pma_inst/RECEIVER/WAIT_FOR_K_glue_set_493 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/C1_OR_C2_rstpot_494 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/WE_rstpot_495 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_rstpot_496 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/RESET_REG_rstpot_497 ;
  wire \U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd4_rstpot_498 ;
  wire \U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd4_rstpot_499 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_rstpot_500 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/TXCHARDISPVAL_rstpot_501 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/TRIGGER_T_rstpot_502 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/S_rstpot_503 ;
  wire \U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA_0_rstpot_504 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/CONFIGURATION_VALID_EN_rstpot_505 ;
  wire \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_rstpot_506 ;
  wire \U0/gpcs_pma_inst/RECEIVER/C_HDR_REMOVED_REG_rstpot_507 ;
  wire \U0/gpcs_pma_inst/RECEIVER/C_rstpot_508 ;
  wire \U0/gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_rstpot_509 ;
  wire \U0/gpcs_pma_inst/RECEIVER/RX_DATA_ERROR_rstpot_510 ;
  wire \U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_rstpot_511 ;
  wire N47;
  wire N49;
  wire N55;
  wire N57;
  wire N59;
  wire N61;
  wire N63;
  wire N64;
  wire N65;
  wire N66;
  wire N67;
  wire N68;
  wire N69;
  wire N70;
  wire \U0/gpcs_pma_inst/Mshreg_STATUS_VECTOR_0_526 ;
  wire \U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_7_527 ;
  wire \U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_6_528 ;
  wire \U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_5_529 ;
  wire \U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_2_530 ;
  wire \U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_4_531 ;
  wire \U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_3_532 ;
  wire \U0/gpcs_pma_inst/RECEIVER/Mshreg_EXTEND_REG3_533 ;
  wire \U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_1_534 ;
  wire \U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_0_535 ;
  wire \U0/gpcs_pma_inst/RECEIVER/Mshreg_SOP_REG2_536 ;
  wire \NLW_U0/gpcs_pma_inst/Mshreg_STATUS_VECTOR_0_Q15_UNCONNECTED ;
  wire \NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_7_Q15_UNCONNECTED ;
  wire \NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_6_Q15_UNCONNECTED ;
  wire \NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_5_Q15_UNCONNECTED ;
  wire \NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_2_Q15_UNCONNECTED ;
  wire \NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_4_Q15_UNCONNECTED ;
  wire \NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_3_Q15_UNCONNECTED ;
  wire \NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_EXTEND_REG3_Q15_UNCONNECTED ;
  wire \NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_1_Q15_UNCONNECTED ;
  wire \NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_0_Q15_UNCONNECTED ;
  wire \NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_CGBAD_REG2_Q15_UNCONNECTED ;
  wire \NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_SOP_REG2_Q15_UNCONNECTED ;
  wire \NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_FALSE_CARRIER_REG2_Q15_UNCONNECTED ;
  wire [7 : 0] \U0/gpcs_pma_inst/TXDATA ;
  wire [7 : 0] \U0/gpcs_pma_inst/RECEIVER/RXD ;
  wire [2 : 0] \U0/gpcs_pma_inst/RXCLKCORCNT_INT ;
  wire [1 : 1] \U0/gpcs_pma_inst/RXBUFSTATUS_INT ;
  wire [7 : 0] \U0/gpcs_pma_inst/RXDATA_INT ;
  wire [1 : 0] \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT ;
  wire [7 : 0] \U0/gpcs_pma_inst/TRANSMITTER/TXDATA ;
  wire [1 : 0] \U0/gpcs_pma_inst/TRANSMITTER/Result ;
  wire [1 : 1] \U0/gpcs_pma_inst/TRANSMITTER/_n0235 ;
  wire [3 : 0] \U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA ;
  wire [7 : 0] \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP ;
  wire [7 : 0] \U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1 ;
  wire [6 : 6] \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/DATA_RD ;
  wire [15 : 0] \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG ;
  wire [4 : 0] \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDR_WR ;
  wire [1 : 0] \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/OPCODE ;
  wire [3 : 0] \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT ;
  wire [1 : 0] \U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS ;
  wire [2 : 0] \U0/gpcs_pma_inst/RECEIVER/IDLE_REG ;
  wire [3 : 0] \U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG ;
  wire [7 : 7] NlwRenamedSig_OI_status_vector;
  wire [7 : 0] \U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5 ;
  assign
    \U0/gpcs_pma_inst/SYNC_SIGNAL_DETECT/data_in  = signal_detect,
    \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/SYNC_MDC/data_in  = mdc,
    \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/SYNC_MDIO_IN/data_in  = mdio_in,
    txdata[7] = \U0/gpcs_pma_inst/TXDATA [7],
    txdata[6] = \U0/gpcs_pma_inst/TXDATA [6],
    txdata[5] = \U0/gpcs_pma_inst/TXDATA [5],
    txdata[4] = \U0/gpcs_pma_inst/TXDATA [4],
    txdata[3] = \U0/gpcs_pma_inst/TXDATA [3],
    txdata[2] = \U0/gpcs_pma_inst/TXDATA [2],
    txdata[1] = \U0/gpcs_pma_inst/TXDATA [1],
    txdata[0] = \U0/gpcs_pma_inst/TXDATA [0],
    gmii_rxd[7] = \U0/gpcs_pma_inst/RECEIVER/RXD [7],
    gmii_rxd[6] = \U0/gpcs_pma_inst/RECEIVER/RXD [6],
    gmii_rxd[5] = \U0/gpcs_pma_inst/RECEIVER/RXD [5],
    gmii_rxd[4] = \U0/gpcs_pma_inst/RECEIVER/RXD [4],
    gmii_rxd[3] = \U0/gpcs_pma_inst/RECEIVER/RXD [3],
    gmii_rxd[2] = \U0/gpcs_pma_inst/RECEIVER/RXD [2],
    gmii_rxd[1] = \U0/gpcs_pma_inst/RECEIVER/RXD [1],
    gmii_rxd[0] = \U0/gpcs_pma_inst/RECEIVER/RXD [0],
    status_vector[15] = NlwRenamedSig_OI_status_vector[7],
    status_vector[14] = NlwRenamedSig_OI_status_vector[7],
    status_vector[13] = NlwRenamedSig_OI_status_vector[7],
    status_vector[12] = NlwRenamedSig_OI_status_vector[7],
    status_vector[11] = NlwRenamedSig_OI_status_vector[7],
    status_vector[10] = NlwRenamedSig_OI_status_vector[7],
    status_vector[9] = NlwRenamedSig_OI_status_vector[7],
    status_vector[8] = NlwRenamedSig_OI_status_vector[7],
    status_vector[7] = NlwRenamedSig_OI_status_vector[7],
    status_vector[6] = \U0/gpcs_pma_inst/RXNOTINTABLE_REG_60 ,
    status_vector[5] = \U0/gpcs_pma_inst/RXDISPERR_REG_61 ,
    status_vector[4] = \NlwRenamedSig_OI_U0/gpcs_pma_inst/RECEIVER/RX_INVALID ,
    status_vector[3] = \U0/gpcs_pma_inst/RECEIVER/RUDI_I_63 ,
    status_vector[2] = \U0/gpcs_pma_inst/RECEIVER/RUDI_C_64 ,
    status_vector[1] = \NlwRenamedSignal_U0/gpcs_pma_inst/STATUS_VECTOR_0 ,
    status_vector[0] = \NlwRenamedSignal_U0/gpcs_pma_inst/STATUS_VECTOR_0 ,
    mgt_rx_reset = \NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ,
    mgt_tx_reset = \NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ,
    powerdown = \NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/POWERDOWN_REG ,
    txchardispmode = \U0/gpcs_pma_inst/TXCHARDISPMODE_69 ,
    txchardispval = \U0/gpcs_pma_inst/TXCHARDISPVAL_70 ,
    txcharisk = \U0/gpcs_pma_inst/TXCHARISK_71 ,
    enablealign = \NlwRenamedSig_OI_U0/gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN ,
    gmii_rx_dv = \NlwRenamedSig_OI_U0/gpcs_pma_inst/RECEIVER/RX_DV ,
    gmii_rx_er = \U0/gpcs_pma_inst/RECEIVER/RX_ER_74 ,
    gmii_isolate = \NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ,
    mdio_out = \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_OUT_76 ,
    mdio_tri = \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_TRI_77 ;
  VCC   XST_VCC (
    .P(N0)
  );
  GND   XST_GND (
    .G(NlwRenamedSig_OI_status_vector[7])
  );
  SRL16 #(
    .INIT ( 16'h0000 ))
  \U0/gpcs_pma_inst/DELAY_RXNOTINTABLE  (
    .A0(NlwRenamedSig_OI_status_vector[7]),
    .A1(NlwRenamedSig_OI_status_vector[7]),
    .A2(N0),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CLK(userclk2),
    .D(\U0/gpcs_pma_inst/RXNOTINTABLE_INT_115 ),
    .Q(\U0/gpcs_pma_inst/RXNOTINTABLE_SRL )
  );
  SRL16 #(
    .INIT ( 16'h0000 ))
  \U0/gpcs_pma_inst/DELAY_RXDISPERR  (
    .A0(NlwRenamedSig_OI_status_vector[7]),
    .A1(NlwRenamedSig_OI_status_vector[7]),
    .A2(N0),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CLK(userclk2),
    .D(\U0/gpcs_pma_inst/RXDISPERR_INT_116 ),
    .Q(\U0/gpcs_pma_inst/RXDISPERR_SRL )
  );
  FDR   \U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd2  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd2-In ),
    .R(\U0/gpcs_pma_inst/RESET_INT_RXBUFSTATUS_INT[1]_OR_126_o ),
    .Q(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd2_80 )
  );
  FDR   \U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd3  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd3-In ),
    .R(\U0/gpcs_pma_inst/RESET_INT_RXBUFSTATUS_INT[1]_OR_126_o ),
    .Q(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd3_81 )
  );
  FDR   \U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd1-In ),
    .R(\U0/gpcs_pma_inst/RESET_INT_RXBUFSTATUS_INT[1]_OR_126_o ),
    .Q(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd1_79 )
  );
  FDR   \U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd1-In ),
    .R(\U0/gpcs_pma_inst/RESET_INT_TXBUFERR_INT_OR_125_o ),
    .Q(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd1_86 )
  );
  FDR   \U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd2  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd2-In ),
    .R(\U0/gpcs_pma_inst/RESET_INT_TXBUFERR_INT_OR_125_o ),
    .Q(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd2_87 )
  );
  FDR   \U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd3  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd3-In ),
    .R(\U0/gpcs_pma_inst/RESET_INT_TXBUFERR_INT_OR_125_o ),
    .Q(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd3_88 )
  );
  FD #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/SYNC_SIGNAL_DETECT/data_sync  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/SYNC_SIGNAL_DETECT/data_in ),
    .Q(\U0/gpcs_pma_inst/SYNC_SIGNAL_DETECT/data_sync1 )
  );
  FD #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/SYNC_SIGNAL_DETECT/data_sync_reg  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/SYNC_SIGNAL_DETECT/data_sync1 ),
    .Q(\U0/gpcs_pma_inst/SIGNAL_DETECT_REG )
  );
  FDR   \U0/gpcs_pma_inst/TXCHARDISPVAL  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TXCHARDISPVAL_INT_GND_15_o_MUX_288_o ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TXCHARDISPVAL_70 )
  );
  FDR   \U0/gpcs_pma_inst/TXCHARDISPMODE  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TXCHARDISPMODE_INT_TXEVEN_MUX_287_o ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TXCHARDISPMODE_69 )
  );
  FDR   \U0/gpcs_pma_inst/TXCHARISK  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TXCHARISK_INT_TXEVEN_MUX_286_o ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TXCHARISK_71 )
  );
  FDR   \U0/gpcs_pma_inst/RXCLKCORCNT_INT_2  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RXCLKCORCNT[2]_GND_15_o_mux_18_OUT<2> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RXCLKCORCNT_INT [2])
  );
  FDR   \U0/gpcs_pma_inst/RXCLKCORCNT_INT_1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RXCLKCORCNT[2]_GND_15_o_mux_18_OUT<1> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RXCLKCORCNT_INT [1])
  );
  FDR   \U0/gpcs_pma_inst/RXCLKCORCNT_INT_0  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RXCLKCORCNT[2]_GND_15_o_mux_18_OUT<0> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RXCLKCORCNT_INT [0])
  );
  FDR   \U0/gpcs_pma_inst/TXDATA_7  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<7> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TXDATA [7])
  );
  FDR   \U0/gpcs_pma_inst/TXDATA_6  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<6> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TXDATA [6])
  );
  FDR   \U0/gpcs_pma_inst/TXDATA_5  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<5> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TXDATA [5])
  );
  FDR   \U0/gpcs_pma_inst/TXDATA_4  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<4> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TXDATA [4])
  );
  FDR   \U0/gpcs_pma_inst/TXDATA_3  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<3> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TXDATA [3])
  );
  FDR   \U0/gpcs_pma_inst/TXDATA_2  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<2> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TXDATA [2])
  );
  FDR   \U0/gpcs_pma_inst/TXDATA_1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<1> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TXDATA [1])
  );
  FDR   \U0/gpcs_pma_inst/TXDATA_0  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<0> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TXDATA [0])
  );
  FDR   \U0/gpcs_pma_inst/RXDATA_INT_7  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<7> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RXDATA_INT [7])
  );
  FDR   \U0/gpcs_pma_inst/RXDATA_INT_6  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<6> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RXDATA_INT [6])
  );
  FDR   \U0/gpcs_pma_inst/RXDATA_INT_5  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<5> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RXDATA_INT [5])
  );
  FDR   \U0/gpcs_pma_inst/RXDATA_INT_4  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<4> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RXDATA_INT [4])
  );
  FDR   \U0/gpcs_pma_inst/RXDATA_INT_3  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<3> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RXDATA_INT [3])
  );
  FDR   \U0/gpcs_pma_inst/RXDATA_INT_2  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<2> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RXDATA_INT [2])
  );
  FDR   \U0/gpcs_pma_inst/RXDATA_INT_1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<1> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RXDATA_INT [1])
  );
  FDR   \U0/gpcs_pma_inst/RXDATA_INT_0  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<0> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RXDATA_INT [0])
  );
  FDR   \U0/gpcs_pma_inst/RXCHARISK_INT  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RXCHARISK[0]_TXCHARISK_INT_MUX_279_o ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RXCHARISK_INT_125 )
  );
  FDR   \U0/gpcs_pma_inst/RXCHARISCOMMA_INT  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RXCHARISCOMMA[0]_TXCHARISK_INT_MUX_280_o ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RXCHARISCOMMA_INT_126 )
  );
  FD   \U0/gpcs_pma_inst/RXNOTINTABLE_REG  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RXNOTINTABLE_SRL ),
    .Q(\U0/gpcs_pma_inst/RXNOTINTABLE_REG_60 )
  );
  FD   \U0/gpcs_pma_inst/RXDISPERR_REG  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RXDISPERR_SRL ),
    .Q(\U0/gpcs_pma_inst/RXDISPERR_REG_61 )
  );
  FD   \U0/gpcs_pma_inst/SRESET  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/SRESET_PIPE_PWR_15_o_MUX_1_o ),
    .Q(\U0/gpcs_pma_inst/SRESET_127 )
  );
  FDR   \U0/gpcs_pma_inst/TXBUFERR_INT  (
    .C(userclk2),
    .D(txbuferr),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TXBUFERR_INT_110 )
  );
  FDR   \U0/gpcs_pma_inst/RXBUFSTATUS_INT_1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RXBUFSTATUS[1]_GND_15_o_mux_17_OUT<1> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RXBUFSTATUS_INT [1])
  );
  FDR   \U0/gpcs_pma_inst/RXNOTINTABLE_INT  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RXNOTINTABLE[0]_GND_15_o_MUX_276_o ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RXNOTINTABLE_INT_115 )
  );
  FDR   \U0/gpcs_pma_inst/RXDISPERR_INT  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RXDISPERR[0]_GND_15_o_MUX_277_o ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RXDISPERR_INT_116 )
  );
  FD   \U0/gpcs_pma_inst/SRESET_PIPE  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RESET_INT_134 ),
    .Q(\U0/gpcs_pma_inst/SRESET_PIPE_128 )
  );
  FDS   \U0/gpcs_pma_inst/MGT_RX_RESET_INT  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RX_RST_SM[3]_GND_15_o_Mux_13_o ),
    .S(\U0/gpcs_pma_inst/RESET_INT_RXBUFSTATUS_INT[1]_OR_126_o ),
    .Q(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT )
  );
  FDS   \U0/gpcs_pma_inst/MGT_TX_RESET_INT  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TX_RST_SM[3]_GND_15_o_Mux_9_o ),
    .S(\U0/gpcs_pma_inst/RESET_INT_TXBUFERR_INT_OR_125_o ),
    .Q(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT )
  );
  FDP   \U0/gpcs_pma_inst/RESET_INT  (
    .C(userclk),
    .D(\U0/gpcs_pma_inst/RESET_INT_PIPE_133 ),
    .PRE(\U0/gpcs_pma_inst/DCM_LOCKED_SOFT_RESET_OR_2_o ),
    .Q(\U0/gpcs_pma_inst/RESET_INT_134 )
  );
  FDP   \U0/gpcs_pma_inst/RESET_INT_PIPE  (
    .C(userclk),
    .D(NlwRenamedSig_OI_status_vector[7]),
    .PRE(\U0/gpcs_pma_inst/DCM_LOCKED_SOFT_RESET_OR_2_o ),
    .Q(\U0/gpcs_pma_inst/RESET_INT_PIPE_133 )
  );
  FDS   \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT_1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/Result [1]),
    .S(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [1])
  );
  FDS   \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT_0  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/Result [0]),
    .S(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0])
  );
  FDR   \U0/gpcs_pma_inst/TRANSMITTER/TXDATA_7  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<7> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [7])
  );
  FDR   \U0/gpcs_pma_inst/TRANSMITTER/TXDATA_6  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<6> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [6])
  );
  FDR   \U0/gpcs_pma_inst/TRANSMITTER/TXDATA_5  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<5> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [5])
  );
  FDR   \U0/gpcs_pma_inst/TRANSMITTER/TXDATA_4  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<4> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [4])
  );
  FDR   \U0/gpcs_pma_inst/TRANSMITTER/TXDATA_3  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<3> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [3])
  );
  FDR   \U0/gpcs_pma_inst/TRANSMITTER/TXDATA_2  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<2> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [2])
  );
  FDR   \U0/gpcs_pma_inst/TRANSMITTER/TXDATA_1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<1> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [1])
  );
  FDR   \U0/gpcs_pma_inst/TRANSMITTER/TXDATA_0  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<0> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [0])
  );
  FDR   \U0/gpcs_pma_inst/TRANSMITTER/TXCHARISK  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_GND_26_o_MUX_192_o ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TXCHARISK_141 )
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_7  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<7> ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [7])
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_6  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<6> ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [6])
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_5  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<5> ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [5])
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_4  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<4> ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [4])
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_3  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<3> ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [3])
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_2  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<2> ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [2])
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<1> ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [1])
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_0  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<0> ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [0])
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_CODE_GRP_CNT[1]_MUX_186_o ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_205 )
  );
  FDSE   \U0/gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/TRANSMITTER/Mram_CODE_GRP_CNT[1]_GND_26_o_Mux_5_o ),
    .D(NlwRenamedSig_OI_status_vector[7]),
    .S(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_200 )
  );
  FDS   \U0/gpcs_pma_inst/TRANSMITTER/TXCHARDISPMODE  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_EVEN_AND_59_o ),
    .S(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TXCHARDISPMODE_140 )
  );
  FDRE   \U0/gpcs_pma_inst/TRANSMITTER/XMIT_DATA_INT  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/TRANSMITTER/Mram_CODE_GRP_CNT[1]_GND_26_o_Mux_5_o ),
    .D(N0),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/XMIT_DATA_INT_201 )
  );
  FDR   \U0/gpcs_pma_inst/TRANSMITTER/TRIGGER_S  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/TX_EN_EVEN_AND_25_o ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TRIGGER_S_208 )
  );
  FDR   \U0/gpcs_pma_inst/TRANSMITTER/T  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/TX_EN_TRIGGER_T_OR_27_o ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/T_207 )
  );
  FDR   \U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA_3  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/Mram_CODE_GRP_CNT[1]_GND_26_o_Mux_5_o ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [3])
  );
  FDR   \U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA_2  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT[1]_TX_CONFIG[15]_wide_mux_4_OUT<7> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [2])
  );
  FDR   \U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA_1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/_n0235 [1]),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [1])
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/TX_ER_REG1  (
    .C(userclk2),
    .D(gmii_tx_er),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TX_ER_REG1_222 )
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/TX_EN_REG1  (
    .C(userclk2),
    .D(gmii_tx_en),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_223 )
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1_7  (
    .C(userclk2),
    .D(gmii_txd[7]),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1 [7])
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1_6  (
    .C(userclk2),
    .D(gmii_txd[6]),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1 [6])
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1_5  (
    .C(userclk2),
    .D(gmii_txd[5]),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1 [5])
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1_4  (
    .C(userclk2),
    .D(gmii_txd[4]),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1 [4])
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1_3  (
    .C(userclk2),
    .D(gmii_txd[3]),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1 [3])
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1_2  (
    .C(userclk2),
    .D(gmii_txd[2]),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1 [2])
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1_1  (
    .C(userclk2),
    .D(gmii_txd[1]),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1 [1])
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1_0  (
    .C(userclk2),
    .D(gmii_txd[0]),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1 [0])
  );
  FD #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/SYNC_MDC/data_sync  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/SYNC_MDC/data_in ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/SYNC_MDC/data_sync1 )
  );
  FD #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/SYNC_MDC/data_sync_reg  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/SYNC_MDC/data_sync1 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_REG2 )
  );
  FD #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/SYNC_MDIO_IN/data_sync  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/SYNC_MDIO_IN/data_in ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/SYNC_MDIO_IN/data_sync1 )
  );
  FD #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/SYNC_MDIO_IN/data_sync_reg  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/SYNC_MDIO_IN/data_sync1 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_IN_REG2 )
  );
  FDS   \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_IN_REG4  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_IN_REG3_244 ),
    .S(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_IN_REG4_243 )
  );
  FDSE #(
    .INIT ( 1'b1 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/_n0162_inv ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG_DATA_WR[10]_MUX_124_o ),
    .S(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG )
  );
  FDRE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/UNIDIRECTIONAL_ENABLE_REG  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/_n0162_inv ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/UNIDIRECTIONAL_ENABLE_REG_DATA_WR[5]_MUX_126_o ),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/UNIDIRECTIONAL_ENABLE_REG_242 )
  );
  FDRE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/_n0162_inv ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_DATA_WR[14]_MUX_120_o ),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 )
  );
  FDS   \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_IN_REG3  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_IN_REG2 ),
    .S(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_IN_REG3_244 )
  );
  FDR   \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_REG3  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_REG2 ),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_REG3_245 )
  );
  FDR   \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/CONFIGURATION_VALID_EN_REG  (
    .C(userclk2),
    .D(configuration_valid),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/CONFIGURATION_VALID_EN_REG_246 )
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT_3  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0141_inv ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT3 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [3])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT_2  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0141_inv ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT2 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [2])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT_1  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0141_inv ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT1 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [1])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT_0  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0141_inv ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [0])
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1-In ),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1_306 )
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2-In ),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 )
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3-In_281 ),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 )
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In ),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 )
  );
  FDSE   \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_TRI  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_248 ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE[3]_PWR_20_o_Mux_37_o ),
    .S(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_TRI_77 )
  );
  FDSE   \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_OUT  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_248 ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE[3]_PWR_20_o_Mux_36_o ),
    .S(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_OUT_76 )
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG3  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG2_308 ),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG3_250 )
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/OPCODE_1  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0147_inv_283 ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [1]),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/OPCODE [1])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/OPCODE_0  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0147_inv_283 ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [0]),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/OPCODE [0])
  );
  FDRE   \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDR_WR_4  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0155_inv ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [3]),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDR_WR [4])
  );
  FDRE   \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDR_WR_3  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0155_inv ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [2]),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDR_WR [3])
  );
  FDRE   \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDR_WR_2  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0155_inv ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [1]),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDR_WR [2])
  );
  FDRE   \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDR_WR_1  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0155_inv ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [0]),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDR_WR [1])
  );
  FDRE   \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDR_WR_0  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0155_inv ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_IN_REG_258 ),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDR_WR [0])
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG2  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG1_309 ),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG2_308 )
  );
  FD #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/LAST_DATA_SHIFT  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG1_PWR_20_o_AND_3_o ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/LAST_DATA_SHIFT_302 )
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG_15  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_IN_LAST_DATA_SHIFT_OR_9_o ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<15> ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [15])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG_14  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_IN_LAST_DATA_SHIFT_OR_9_o ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<14> ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [14])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG_13  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_IN_LAST_DATA_SHIFT_OR_9_o ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<13> ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [13])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG_12  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_IN_LAST_DATA_SHIFT_OR_9_o ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<12> ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [12])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG_11  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_IN_LAST_DATA_SHIFT_OR_9_o ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<11> ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [11])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG_10  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_IN_LAST_DATA_SHIFT_OR_9_o ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<10> ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [10])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG_9  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_IN_LAST_DATA_SHIFT_OR_9_o ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<9> ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [9])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG_8  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_IN_LAST_DATA_SHIFT_OR_9_o ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<8> ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [8])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG_7  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_IN_LAST_DATA_SHIFT_OR_9_o ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<7> ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [7])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG_6  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_IN_LAST_DATA_SHIFT_OR_9_o ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<6> ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [6])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG_5  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_IN_LAST_DATA_SHIFT_OR_9_o ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<5> ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [5])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG_4  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_IN_LAST_DATA_SHIFT_OR_9_o ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<4> ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [4])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG_3  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_IN_LAST_DATA_SHIFT_OR_9_o ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<3> ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [3])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG_2  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_IN_LAST_DATA_SHIFT_OR_9_o ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<2> ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [2])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG_1  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_IN_LAST_DATA_SHIFT_OR_9_o ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<1> ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [1])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG_0  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_IN_LAST_DATA_SHIFT_OR_9_o ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<0> ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [0])
  );
  FDSE #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_IN_REG  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_248 ),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_IN_REG4_243 ),
    .S(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_IN_REG_258 )
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_248 ),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG1_309 )
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1-In2 ),
    .R(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In1_0 ),
    .Q(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_327 )
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2-In2 ),
    .R(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In1_0 ),
    .Q(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_328 )
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In2_331 ),
    .R(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In1_0 ),
    .Q(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_330 )
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3-In3 ),
    .R(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In1_0 ),
    .Q(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_329 )
  );
  FDRE   \U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS_1  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/SYNCHRONISATION/_n0103_inv ),
    .D(\U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_GND_27_o_mux_30_OUT<1> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS [1])
  );
  FDRE   \U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS_0  (
    .C(userclk2),
    .CE(\U0/gpcs_pma_inst/SYNCHRONISATION/_n0103_inv ),
    .D(\U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_GND_27_o_mux_30_OUT<0> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS [0])
  );
  FD   \U0/gpcs_pma_inst/SYNCHRONISATION/SIGNAL_DETECT_REG  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/SIGNAL_DETECT_REG ),
    .Q(\U0/gpcs_pma_inst/SYNCHRONISATION/SIGNAL_DETECT_REG_343 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/RXD_7  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<7> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RXD [7])
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/RXD_6  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<6> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RXD [6])
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/RXD_5  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<5> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RXD [5])
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/RXD_4  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<4> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RXD [4])
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/RXD_3  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<3> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RXD [3])
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/RXD_2  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<2> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RXD [2])
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/RXD_1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<1> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RXD [1])
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/RXD_0  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<0> ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RXD [0])
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/C_REG3  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/C_REG2_346 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/C_REG3_427 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG2 ),
    .R(\U0/gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_61_o ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_405 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/CGBAD_REG3  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/CGBAD_REG2 ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/CGBAD_REG3_408 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/SOP_REG3  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/SOP_REG2_422 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/SOP_REG3_421 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/C_REG2  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/C_REG1_428 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/C_REG2_346 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/IDLE_REG_2  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/IDLE_REG [1]),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/IDLE_REG [2])
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/IDLE_REG_1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/IDLE_REG [0]),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/IDLE_REG [1])
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/IDLE_REG_0  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/I_REG_429 ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/IDLE_REG [0])
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_REG2  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_REG1_393 ),
    .R(\U0/gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_61_o ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_REG2_392 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/ILLEGAL_K_REG2  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/ILLEGAL_K_REG1_397 ),
    .R(\U0/gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_61_o ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/ILLEGAL_K_REG2_396 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/C_REG1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/C_435 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/C_REG1_428 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/T_REG2  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/T_REG1_432 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/T_REG2_431 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG_3  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [2]),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [3])
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG_2  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [1]),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [2])
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG_1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [0]),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [1])
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG_0  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_INT_407 ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [0])
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_REG1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_394 ),
    .R(\U0/gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_61_o ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_REG1_393 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/ILLEGAL_K_REG1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/ILLEGAL_K_398 ),
    .R(\U0/gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_61_o ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/ILLEGAL_K_REG1_397 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/EXTEND_378 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1_420 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/I_REG  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/I_436 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/I_REG_429 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/R_REG1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/R_410 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/R_REG1_430 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/T_REG1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/T_437 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/T_REG1_432 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/RUDI_I  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/IDLE_REG[1]_IDLE_REG[2]_OR_124_o ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RUDI_I_63 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/RUDI_C  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG[0]_RX_CONFIG_VALID_REG[3]_OR_123_o ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RUDI_C_64 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/FALSE_K  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RXDATA[7]_RXNOTINTABLE_AND_228_o ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/FALSE_K_390 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/FALSE_DATA  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_391 )
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/RECEIVER/RX_ER  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1_ISOLATE_AND_199_o_367 ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RX_ER_74 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/EXTEND_ERR  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/EXTEND_REG3_EXT_ILLEGAL_K_REG2_OR_93_o ),
    .R(\U0/gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_61_o ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/EXTEND_ERR_395 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/ILLEGAL_K  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RXCHARISK_REG1_K28p5_REG1_AND_184_o ),
    .R(\U0/gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_61_o ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/ILLEGAL_K_398 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/EOP  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/I_REG_T_REG2_OR_74_o ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/EOP_401 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/SOP  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/S_WAIT_FOR_K_AND_161_o ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/SOP_402 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/EOP_REG1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/EOP_EXTEND_OR_75_o ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/EOP_REG1_400 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/FROM_RX_CX  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/C_REG1_C_REG3_OR_69_o_350 ),
    .R(\U0/gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_61_o ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/FROM_RX_CX_403 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/SYNC_STATUS_REG  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_129 ),
    .R(\U0/gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_61_o ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/SYNC_STATUS_REG_406 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_INT  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/SYNC_STATUS_C_REG1_AND_142_o_368 ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_INT_407 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/R  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/K23p7 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/R_410 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/CGBAD  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RXFIFO_ERR_RXDISPERR_OR_46_o ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/CGBAD_409 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/RXCHARISK_REG1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RXCHARISK_INT_125 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RXCHARISK_REG1_426 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/D0p0_REG  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/D0p0_352 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/D0p0_REG_433 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/K28p5_REG1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/K28p5 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_434 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/I  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/EVEN_RXCHARISK_AND_132_o_369 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/I_436 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/S  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_128_o ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/S_438 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/T  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/K29p7 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/T_437 )
  );
  LUT4 #(
    .INIT ( 16'hEA6A ))
  \U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd2-In1  (
    .I0(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd2_87 ),
    .I1(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd4_89 ),
    .I2(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd3_88 ),
    .I3(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd1_86 ),
    .O(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd2-In )
  );
  LUT4 #(
    .INIT ( 16'hEA6A ))
  \U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd2-In1  (
    .I0(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd2_80 ),
    .I1(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd4_82 ),
    .I2(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd3_81 ),
    .I3(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd1_79 ),
    .O(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd2-In )
  );
  LUT4 #(
    .INIT ( 16'hE666 ))
  \U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd3-In1  (
    .I0(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd3_81 ),
    .I1(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd4_82 ),
    .I2(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd1_79 ),
    .I3(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd2_80 ),
    .O(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd3-In )
  );
  LUT4 #(
    .INIT ( 16'hE666 ))
  \U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd3-In1  (
    .I0(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd3_88 ),
    .I1(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd4_89 ),
    .I2(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd1_86 ),
    .I3(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd2_87 ),
    .O(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd3-In )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/gpcs_pma_inst/Mmux_TXCHARDISPVAL_INT_GND_15_o_MUX_288_o11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TXCHARDISPVAL_139 ),
    .O(\U0/gpcs_pma_inst/TXCHARDISPVAL_INT_GND_15_o_MUX_288_o )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/Mmux_TXCHARDISPMODE_INT_TXEVEN_MUX_287_o11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TXCHARDISPMODE_140 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/gpcs_pma_inst/TXCHARDISPMODE_INT_TXEVEN_MUX_287_o )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/Mmux_TXCHARISK_INT_TXEVEN_MUX_286_o11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TXCHARISK_141 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/gpcs_pma_inst/TXCHARISK_INT_TXEVEN_MUX_286_o )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/gpcs_pma_inst/Mmux_TXDATA_INT[7]_GND_15_o_mux_26_OUT11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [0]),
    .O(\U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<0> )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/gpcs_pma_inst/Mmux_TXDATA_INT[7]_GND_15_o_mux_26_OUT21  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [1]),
    .O(\U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<1> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/Mmux_TXDATA_INT[7]_GND_15_o_mux_26_OUT31  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [2]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<2> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/Mmux_TXDATA_INT[7]_GND_15_o_mux_26_OUT41  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [3]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<3> )
  );
  LUT2 #(
    .INIT ( 4'hE ))
  \U0/gpcs_pma_inst/Mmux_TXDATA_INT[7]_GND_15_o_mux_26_OUT51  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [4]),
    .O(\U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<4> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/Mmux_TXDATA_INT[7]_GND_15_o_mux_26_OUT61  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [5]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<5> )
  );
  LUT3 #(
    .INIT ( 8'h4E ))
  \U0/gpcs_pma_inst/Mmux_TXDATA_INT[7]_GND_15_o_mux_26_OUT71  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [6]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<6> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/Mmux_TXDATA_INT[7]_GND_15_o_mux_26_OUT81  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [7]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/gpcs_pma_inst/TXDATA_INT[7]_GND_15_o_mux_26_OUT<7> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/Mmux_RXDATA[7]_TXDATA_INT[7]_mux_16_OUT11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(rxdata[0]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [0]),
    .O(\U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<0> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/Mmux_RXDATA[7]_TXDATA_INT[7]_mux_16_OUT21  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(rxdata[1]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [1]),
    .O(\U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<1> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/Mmux_RXDATA[7]_TXDATA_INT[7]_mux_16_OUT31  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(rxdata[2]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [2]),
    .O(\U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<2> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/Mmux_RXDATA[7]_TXDATA_INT[7]_mux_16_OUT41  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(rxdata[3]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [3]),
    .O(\U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<3> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/Mmux_RXDATA[7]_TXDATA_INT[7]_mux_16_OUT51  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(rxdata[4]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [4]),
    .O(\U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<4> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/Mmux_RXDATA[7]_TXDATA_INT[7]_mux_16_OUT61  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(rxdata[5]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [5]),
    .O(\U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<5> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/Mmux_RXDATA[7]_TXDATA_INT[7]_mux_16_OUT71  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(rxdata[6]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [6]),
    .O(\U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<6> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/Mmux_RXDATA[7]_TXDATA_INT[7]_mux_16_OUT81  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(rxdata[7]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TXDATA [7]),
    .O(\U0/gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_16_OUT<7> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/Mmux_RXCHARISCOMMA[0]_TXCHARISK_INT_MUX_280_o11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(rxchariscomma[0]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TXCHARISK_141 ),
    .O(\U0/gpcs_pma_inst/RXCHARISCOMMA[0]_TXCHARISK_INT_MUX_280_o )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/Mmux_RXCHARISK[0]_TXCHARISK_INT_MUX_279_o11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(rxcharisk[0]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TXCHARISK_141 ),
    .O(\U0/gpcs_pma_inst/RXCHARISK[0]_TXCHARISK_INT_MUX_279_o )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/gpcs_pma_inst/Mmux_RXNOTINTABLE[0]_GND_15_o_MUX_276_o11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(rxnotintable[0]),
    .O(\U0/gpcs_pma_inst/RXNOTINTABLE[0]_GND_15_o_MUX_276_o )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/gpcs_pma_inst/Mmux_RXDISPERR[0]_GND_15_o_MUX_277_o11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(rxdisperr[0]),
    .O(\U0/gpcs_pma_inst/RXDISPERR[0]_GND_15_o_MUX_277_o )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/gpcs_pma_inst/Mmux_RXCLKCORCNT[2]_GND_15_o_mux_18_OUT11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(rxclkcorcnt[0]),
    .O(\U0/gpcs_pma_inst/RXCLKCORCNT[2]_GND_15_o_mux_18_OUT<0> )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/gpcs_pma_inst/Mmux_RXCLKCORCNT[2]_GND_15_o_mux_18_OUT21  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(rxclkcorcnt[1]),
    .O(\U0/gpcs_pma_inst/RXCLKCORCNT[2]_GND_15_o_mux_18_OUT<1> )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/gpcs_pma_inst/Mmux_RXCLKCORCNT[2]_GND_15_o_mux_18_OUT31  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(rxclkcorcnt[2]),
    .O(\U0/gpcs_pma_inst/RXCLKCORCNT[2]_GND_15_o_mux_18_OUT<2> )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/gpcs_pma_inst/Mmux_RXBUFSTATUS[1]_GND_15_o_mux_17_OUT21  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I1(rxbufstatus[1]),
    .O(\U0/gpcs_pma_inst/RXBUFSTATUS[1]_GND_15_o_mux_17_OUT<1> )
  );
  LUT2 #(
    .INIT ( 4'hE ))
  \U0/gpcs_pma_inst/Mmux_SRESET_PIPE_PWR_15_o_MUX_1_o11  (
    .I0(\U0/gpcs_pma_inst/RESET_INT_134 ),
    .I1(\U0/gpcs_pma_inst/SRESET_PIPE_128 ),
    .O(\U0/gpcs_pma_inst/SRESET_PIPE_PWR_15_o_MUX_1_o )
  );
  LUT4 #(
    .INIT ( 16'hFF80 ))
  \U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd1-In1  (
    .I0(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd4_82 ),
    .I1(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd3_81 ),
    .I2(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd2_80 ),
    .I3(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd1_79 ),
    .O(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd1-In )
  );
  LUT4 #(
    .INIT ( 16'hDFFF ))
  \U0/gpcs_pma_inst/RX_RST_SM_RX_RST_SM[3]_GND_15_o_Mux_13_o1  (
    .I0(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd3_81 ),
    .I1(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd4_82 ),
    .I2(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd1_79 ),
    .I3(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd2_80 ),
    .O(\U0/gpcs_pma_inst/RX_RST_SM[3]_GND_15_o_Mux_13_o )
  );
  LUT4 #(
    .INIT ( 16'hFF80 ))
  \U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd1-In1  (
    .I0(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd4_89 ),
    .I1(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd3_88 ),
    .I2(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd2_87 ),
    .I3(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd1_86 ),
    .O(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd1-In )
  );
  LUT4 #(
    .INIT ( 16'hDFFF ))
  \U0/gpcs_pma_inst/TX_RST_SM_TX_RST_SM[3]_GND_15_o_Mux_9_o1  (
    .I0(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd3_88 ),
    .I1(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd4_89 ),
    .I2(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd1_86 ),
    .I3(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd2_87 ),
    .O(\U0/gpcs_pma_inst/TX_RST_SM[3]_GND_15_o_Mux_9_o )
  );
  LUT2 #(
    .INIT ( 4'hE ))
  \U0/gpcs_pma_inst/RESET_INT_RXBUFSTATUS_INT[1]_OR_126_o1  (
    .I0(\U0/gpcs_pma_inst/RESET_INT_134 ),
    .I1(\U0/gpcs_pma_inst/RXBUFSTATUS_INT [1]),
    .O(\U0/gpcs_pma_inst/RESET_INT_RXBUFSTATUS_INT[1]_OR_126_o )
  );
  LUT2 #(
    .INIT ( 4'hE ))
  \U0/gpcs_pma_inst/RESET_INT_TXBUFERR_INT_OR_125_o1  (
    .I0(\U0/gpcs_pma_inst/RESET_INT_134 ),
    .I1(\U0/gpcs_pma_inst/TXBUFERR_INT_110 ),
    .O(\U0/gpcs_pma_inst/RESET_INT_TXBUFERR_INT_OR_125_o )
  );
  LUT3 #(
    .INIT ( 8'hFB ))
  \U0/gpcs_pma_inst/DCM_LOCKED_SOFT_RESET_OR_2_o1  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/RESET_REG_138 ),
    .I1(dcm_locked),
    .I2(reset),
    .O(\U0/gpcs_pma_inst/DCM_LOCKED_SOFT_RESET_OR_2_o )
  );
  LUT4 #(
    .INIT ( 16'hFE54 ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT51  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_200 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT511 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1 [4]),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [2]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<4> )
  );
  LUT4 #(
    .INIT ( 16'hFE54 ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT61  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_200 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT511 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1 [5]),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [2]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<5> )
  );
  LUT4 #(
    .INIT ( 16'hFE54 ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT81  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_200 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT511 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1 [7]),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [2]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<7> )
  );
  LUT6 #(
    .INIT ( 64'hFFFFFFFFFFFEFFFF ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT5111  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/T_207 ),
    .I1(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/V_197 ),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/S_209 ),
    .I4(\U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_199 ),
    .I5(\U0/gpcs_pma_inst/TRANSMITTER/R_198 ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT511 )
  );
  LUT5 #(
    .INIT ( 32'hE881811F ))
  \U0/gpcs_pma_inst/TRANSMITTER/DISP51  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [3]),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [4]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [1]),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [2]),
    .I4(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [0]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/DISP5 )
  );
  LUT3 #(
    .INIT ( 8'h15 ))
  \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT[1]_TX_CONFIG[15]_wide_mux_4_OUT<7>1  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [1]),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/C1_OR_C2_202 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT[1]_TX_CONFIG[15]_wide_mux_4_OUT<7> )
  );
  LUT6 #(
    .INIT ( 64'hFFFF444044404440 ))
  \U0/gpcs_pma_inst/TRANSMITTER/TX_EN_TRIGGER_T_OR_27_o1  (
    .I0(gmii_tx_en),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_223 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/S_209 ),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_199 ),
    .I4(\U0/gpcs_pma_inst/TRANSMITTER/TRIGGER_T_206 ),
    .I5(\U0/gpcs_pma_inst/TRANSMITTER/V_197 ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/TX_EN_TRIGGER_T_OR_27_o )
  );
  LUT6 #(
    .INIT ( 64'hFFFFFFFF45455545 ))
  \U0/gpcs_pma_inst/TRANSMITTER/TX_EN_TRIGGER_S_OR_25_o11  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/TRIGGER_S_208 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_223 ),
    .I2(gmii_tx_en),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I4(\U0/gpcs_pma_inst/TRANSMITTER/TX_ER_REG1_222 ),
    .I5(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/TX_EN_TRIGGER_S_OR_25_o_0 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mcount_CODE_GRP_CNT_xor<1>11  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [1]),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/Result [1])
  );
  LUT2 #(
    .INIT ( 4'h1 ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mram_CODE_GRP_CNT[1]_GND_26_o_Mux_5_o1  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [1]),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/Mram_CODE_GRP_CNT[1]_GND_26_o_Mux_5_o )
  );
  LUT4 #(
    .INIT ( 16'h0040 ))
  \U0/gpcs_pma_inst/TRANSMITTER/TX_EN_EVEN_AND_25_o1  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/TX_ER_REG1_222 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I2(gmii_tx_en),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_223 ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/TX_EN_EVEN_AND_25_o )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_EVEN_AND_59_o1  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_205 ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_EVEN_AND_59_o )
  );
  LUT4 #(
    .INIT ( 16'h0001 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/Mmux_DATA_RD511  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [3]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [2]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [1]),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [0]),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/DATA_RD [6])
  );
  LUT3 #(
    .INIT ( 8'hEA ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/_n0162_inv1  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/CONFIGURATION_VALID_EN_247 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG3_250 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/GND_22_o_MDIO_WE_AND_14_o ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/_n0162_inv )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/Mmux_UNIDIRECTIONAL_ENABLE_REG_DATA_WR[5]_MUX_126_o11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/GND_22_o_MDIO_WE_AND_14_o ),
    .I1(configuration_vector[0]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [5]),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/UNIDIRECTIONAL_ENABLE_REG_DATA_WR[5]_MUX_126_o )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/Mmux_LOOPBACK_REG_DATA_WR[14]_MUX_120_o11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/GND_22_o_MDIO_WE_AND_14_o ),
    .I1(configuration_vector[1]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [14]),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_DATA_WR[14]_MUX_120_o )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/Mmux_ISOLATE_REG_DATA_WR[10]_MUX_124_o11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/GND_22_o_MDIO_WE_AND_14_o ),
    .I1(configuration_vector[3]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [10]),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG_DATA_WR[10]_MUX_124_o )
  );
  LUT6 #(
    .INIT ( 64'h0000000000000002 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/GND_22_o_MDIO_WE_AND_14_o1  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/WE_249 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDR_WR [4]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDR_WR [3]),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDR_WR [2]),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDR_WR [1]),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDR_WR [0]),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/GND_22_o_MDIO_WE_AND_14_o )
  );
  LUT6 #(
    .INIT ( 64'hA9A9A9A9FFA9A9A9 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT_xor<2>11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [2]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [1]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [0]),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1_306 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT2 )
  );
  LUT6 #(
    .INIT ( 64'h99999999F9980999 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT_xor<3>11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [3]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT_xor<3>14 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1_306 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT3 )
  );
  LUT6 #(
    .INIT ( 64'hAAAAAAAA20022000 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0141_inv1  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_248 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1_306 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In1 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0141_inv )
  );
  LUT4 #(
    .INIT ( 16'h6AAA ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2-In1  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_248 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2-In )
  );
  LUT5 #(
    .INIT ( 32'h20000000 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mmux_GND_24_o_GND_24_o_MUX_62_o11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/OPCODE [0]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/OPCODE [1]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1_306 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_307 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/GND_24_o_GND_24_o_MUX_62_o )
  );
  LUT6 #(
    .INIT ( 64'h8040201008040200 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB21  (
    .I0(phyad[3]),
    .I1(phyad[4]),
    .I2(phyad[2]),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [2]),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [3]),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [1]),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB2_271 )
  );
  LUT6 #(
    .INIT ( 64'hFFFFFFFFFFFB4051 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mmux_STATE[3]_PWR_20_o_Mux_36_o12  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In1 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1_306 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [15]),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mmux_STATE[3]_PWR_20_o_Mux_36_o11_270 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE[3]_PWR_20_o_Mux_36_o )
  );
  LUT5 #(
    .INIT ( 32'hFFFF1011 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mmux_STATE[3]_PWR_20_o_Mux_37_o11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1_306 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In1 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mmux_STATE[3]_PWR_20_o_Mux_36_o11_270 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE[3]_PWR_20_o_Mux_37_o )
  );
  LUT5 #(
    .INIT ( 32'hCAAA8AAA ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1-In1  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1_306 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_248 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1-In )
  );
  LUT5 #(
    .INIT ( 32'h00000400 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3-In11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [3]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT_xor<3>14 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3-In1 )
  );
  LUT3 #(
    .INIT ( 8'hFE ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT_xor<3>141  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [2]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [0]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [1]),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT_xor<3>14 )
  );
  LUT5 #(
    .INIT ( 32'hEAAAC000 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux1311  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [6]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_IN_REG_258 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/DATA_RD [6]),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3-In1 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux1011 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<7> )
  );
  LUT4 #(
    .INIT ( 16'hEAC0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux1411  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [7]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/DATA_RD [6]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3-In1 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux1011 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<8> )
  );
  LUT4 #(
    .INIT ( 16'hEAC0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux1211  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [5]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/DATA_RD [6]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3-In1 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux1011 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<6> )
  );
  LUT6 #(
    .INIT ( 64'hAEAAAAAA0C000000 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux11111  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [4]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/UNIDIRECTIONAL_ENABLE_REG_242 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_IN_REG_258 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/DATA_RD [6]),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3-In1 ),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux1011 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<5> )
  );
  LUT6 #(
    .INIT ( 64'hEAAAAAAAC0000000 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux811  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [1]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_IN_REG_258 ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_129 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/DATA_RD [6]),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3-In1 ),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux1011 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<2> )
  );
  LUT6 #(
    .INIT ( 64'hBAAAAAAA30000000 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux511  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [13]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_IN_REG_258 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/DATA_RD [6]),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3-In1 ),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux1011 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<14> )
  );
  LUT6 #(
    .INIT ( 64'hAEAAAAAA0C000000 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux211  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [10]),
    .I1(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/POWERDOWN_REG ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_IN_REG_258 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/DATA_RD [6]),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3-In1 ),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux1011 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<11> )
  );
  LUT6 #(
    .INIT ( 64'hBAAAAAAA30000000 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux1111  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [9]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_IN_REG_258 ),
    .I2(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/DATA_RD [6]),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3-In1 ),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux1011 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<10> )
  );
  LUT4 #(
    .INIT ( 16'hFFF7 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux10111  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In1 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux1011 )
  );
  LUT3 #(
    .INIT ( 8'h80 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG1_PWR_20_o_AND_3_o1  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG1_309 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1_306 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG1_PWR_20_o_AND_3_o )
  );
  LUT2 #(
    .INIT ( 4'hE ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_IN_LAST_DATA_SHIFT_OR_9_o1  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_248 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/LAST_DATA_SHIFT_302 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_IN_LAST_DATA_SHIFT_OR_9_o )
  );
  LUT6 #(
    .INIT ( 64'h0000577757770000 ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/Mmux_GOOD_CGS[1]_GND_27_o_mux_30_OUT21  (
    .I0(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_330 ),
    .I1(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_327 ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_328 ),
    .I3(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_329 ),
    .I4(\U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS [0]),
    .I5(\U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS [1]),
    .O(\U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_GND_27_o_mux_30_OUT<1> )
  );
  LUT5 #(
    .INIT ( 32'hA888FFFF ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/_n0103_inv1  (
    .I0(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_330 ),
    .I1(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_327 ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_329 ),
    .I3(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_328 ),
    .I4(\U0/gpcs_pma_inst/SYNCHRONISATION/CGBAD ),
    .O(\U0/gpcs_pma_inst/SYNCHRONISATION/_n0103_inv )
  );
  LUT5 #(
    .INIT ( 32'h01115555 ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/Mmux_GOOD_CGS[1]_GND_27_o_mux_30_OUT11  (
    .I0(\U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS [0]),
    .I1(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_327 ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_329 ),
    .I3(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_328 ),
    .I4(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_330 ),
    .O(\U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_GND_27_o_mux_30_OUT<0> )
  );
  LUT6 #(
    .INIT ( 64'hF2A8F2AAAA28AA2A ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1-In21  (
    .I0(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_327 ),
    .I1(\U0/gpcs_pma_inst/SYNCHRONISATION/CGBAD ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_328 ),
    .I3(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_330 ),
    .I4(\U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_PWR_23_o_equal_19_o ),
    .I5(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_329 ),
    .O(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1-In2 )
  );
  LUT3 #(
    .INIT ( 8'hAB ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In1_01  (
    .I0(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/LOOPBACK_REG_137 ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/SIGNAL_DETECT_REG_343 ),
    .O(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In1_0 )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_PWR_23_o_equal_19_o<1>1  (
    .I0(\U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS [0]),
    .I1(\U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS [1]),
    .O(\U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_PWR_23_o_equal_19_o )
  );
  LUT5 #(
    .INIT ( 32'h00000008 ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/CGBAD_GND_27_o_AND_69_o1  (
    .I0(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_330 ),
    .I1(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_328 ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_329 ),
    .I3(\U0/gpcs_pma_inst/RXCHARISK_INT_125 ),
    .I4(\U0/gpcs_pma_inst/SYNCHRONISATION/CGBAD ),
    .O(\U0/gpcs_pma_inst/SYNCHRONISATION/CGBAD_GND_27_o_AND_69_o )
  );
  LUT5 #(
    .INIT ( 32'hFFFEFEFE ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/CGBAD1  (
    .I0(\U0/gpcs_pma_inst/RXBUFSTATUS_INT [1]),
    .I1(\U0/gpcs_pma_inst/RXNOTINTABLE_INT_115 ),
    .I2(\U0/gpcs_pma_inst/RXDISPERR_INT_116 ),
    .I3(\U0/gpcs_pma_inst/RXCHARISCOMMA_INT_126 ),
    .I4(\U0/gpcs_pma_inst/SYNCHRONISATION/EVEN_130 ),
    .O(\U0/gpcs_pma_inst/SYNCHRONISATION/CGBAD )
  );
  LUT4 #(
    .INIT ( 16'h5554 ))
  \U0/gpcs_pma_inst/RECEIVER/Mmux_RXDATA_REG5[7]_GND_28_o_mux_9_OUT21  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/SOP_REG3_421 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1_420 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_405 ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5 [1]),
    .O(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<1> )
  );
  LUT4 #(
    .INIT ( 16'h5554 ))
  \U0/gpcs_pma_inst/RECEIVER/Mmux_RXDATA_REG5[7]_GND_28_o_mux_9_OUT41  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/SOP_REG3_421 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1_420 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_405 ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5 [3]),
    .O(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<3> )
  );
  LUT4 #(
    .INIT ( 16'hFFFE ))
  \U0/gpcs_pma_inst/RECEIVER/Mmux_RXDATA_REG5[7]_GND_28_o_mux_9_OUT31  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5 [2]),
    .I1(\U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1_420 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_405 ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/SOP_REG3_421 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<2> )
  );
  LUT4 #(
    .INIT ( 16'h0002 ))
  \U0/gpcs_pma_inst/RECEIVER/Mmux_RXDATA_REG5[7]_GND_28_o_mux_9_OUT61  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5 [5]),
    .I1(\U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1_420 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/SOP_REG3_421 ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_405 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<5> )
  );
  LUT4 #(
    .INIT ( 16'h0002 ))
  \U0/gpcs_pma_inst/RECEIVER/Mmux_RXDATA_REG5[7]_GND_28_o_mux_9_OUT81  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5 [7]),
    .I1(\U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1_420 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/SOP_REG3_421 ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_405 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<7> )
  );
  LUT4 #(
    .INIT ( 16'h2000 ))
  \U0/gpcs_pma_inst/RECEIVER/K29p71  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_128_o1_344 ),
    .I1(\U0/gpcs_pma_inst/RXDATA_INT [1]),
    .I2(\U0/gpcs_pma_inst/RXDATA_INT [3]),
    .I3(\U0/gpcs_pma_inst/RXDATA_INT [2]),
    .O(\U0/gpcs_pma_inst/RECEIVER/K29p7 )
  );
  LUT6 #(
    .INIT ( 64'h8000000000000000 ))
  \U0/gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_128_o11  (
    .I0(\U0/gpcs_pma_inst/RXCHARISK_INT_125 ),
    .I1(\U0/gpcs_pma_inst/RXDATA_INT [6]),
    .I2(\U0/gpcs_pma_inst/RXDATA_INT [0]),
    .I3(\U0/gpcs_pma_inst/RXDATA_INT [4]),
    .I4(\U0/gpcs_pma_inst/RXDATA_INT [5]),
    .I5(\U0/gpcs_pma_inst/RXDATA_INT [7]),
    .O(\U0/gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_128_o1_344 )
  );
  LUT4 #(
    .INIT ( 16'hFF54 ))
  \U0/gpcs_pma_inst/RECEIVER/Mmux_RXDATA_REG5[7]_GND_28_o_mux_9_OUT11  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_405 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1_420 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5 [0]),
    .I3(\U0/gpcs_pma_inst/RECEIVER/SOP_REG3_421 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<0> )
  );
  LUT5 #(
    .INIT ( 32'hFFFF4540 ))
  \U0/gpcs_pma_inst/RECEIVER/Mmux_RXDATA_REG5[7]_GND_28_o_mux_9_OUT51  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_405 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/EXTEND_ERR_395 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1_420 ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5 [4]),
    .I4(\U0/gpcs_pma_inst/RECEIVER/SOP_REG3_421 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<4> )
  );
  LUT4 #(
    .INIT ( 16'h2002 ))
  \U0/gpcs_pma_inst/RECEIVER/RXDATA[7]_RXNOTINTABLE_AND_228_o1  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/K28p51_345 ),
    .I1(\U0/gpcs_pma_inst/RXNOTINTABLE_INT_115 ),
    .I2(\U0/gpcs_pma_inst/RXDATA_INT [5]),
    .I3(\U0/gpcs_pma_inst/RXDATA_INT [6]),
    .O(\U0/gpcs_pma_inst/RECEIVER/RXDATA[7]_RXNOTINTABLE_AND_228_o )
  );
  LUT5 #(
    .INIT ( 32'h08080800 ))
  \U0/gpcs_pma_inst/RECEIVER/S_WAIT_FOR_K_AND_161_o1  (
    .I0(\U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_129 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/S_438 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/WAIT_FOR_K_381 ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/EXTEND_378 ),
    .I4(\U0/gpcs_pma_inst/RECEIVER/I_REG_429 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/S_WAIT_FOR_K_AND_161_o )
  );
  LUT4 #(
    .INIT ( 16'h2000 ))
  \U0/gpcs_pma_inst/RECEIVER/K23p71  (
    .I0(\U0/gpcs_pma_inst/RXDATA_INT [2]),
    .I1(\U0/gpcs_pma_inst/RXDATA_INT [3]),
    .I2(\U0/gpcs_pma_inst/RXDATA_INT [1]),
    .I3(\U0/gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_128_o1_344 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/K23p7 )
  );
  LUT5 #(
    .INIT ( 32'h00200000 ))
  \U0/gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_128_o1  (
    .I0(\U0/gpcs_pma_inst/RXDATA_INT [1]),
    .I1(\U0/gpcs_pma_inst/RXDATA_INT [2]),
    .I2(\U0/gpcs_pma_inst/RXDATA_INT [3]),
    .I3(\U0/gpcs_pma_inst/RECEIVER/RXFIFO_ERR_RXDISPERR_OR_46_o ),
    .I4(\U0/gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_128_o1_344 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_128_o )
  );
  LUT3 #(
    .INIT ( 8'h20 ))
  \U0/gpcs_pma_inst/RECEIVER/K28p52  (
    .I0(\U0/gpcs_pma_inst/RXDATA_INT [5]),
    .I1(\U0/gpcs_pma_inst/RXDATA_INT [6]),
    .I2(\U0/gpcs_pma_inst/RECEIVER/K28p51_345 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/K28p5 )
  );
  LUT4 #(
    .INIT ( 16'hFF10 ))
  \U0/gpcs_pma_inst/RECEIVER/Mmux_RXDATA_REG5[7]_GND_28_o_mux_9_OUT71  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_405 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1_420 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5 [6]),
    .I3(\U0/gpcs_pma_inst/RECEIVER/SOP_REG3_421 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_28_o_mux_9_OUT<6> )
  );
  LUT2 #(
    .INIT ( 4'hE ))
  \U0/gpcs_pma_inst/RECEIVER/IDLE_REG[1]_IDLE_REG[2]_OR_124_o1  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/IDLE_REG [1]),
    .I1(\U0/gpcs_pma_inst/RECEIVER/IDLE_REG [2]),
    .O(\U0/gpcs_pma_inst/RECEIVER/IDLE_REG[1]_IDLE_REG[2]_OR_124_o )
  );
  LUT4 #(
    .INIT ( 16'hFFFE ))
  \U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG[0]_RX_CONFIG_VALID_REG[3]_OR_123_o<0>1  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [0]),
    .I1(\U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [1]),
    .I2(\U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [2]),
    .I3(\U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [3]),
    .O(\U0/gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG[0]_RX_CONFIG_VALID_REG[3]_OR_123_o )
  );
  LUT3 #(
    .INIT ( 8'hF8 ))
  \U0/gpcs_pma_inst/RECEIVER/EXTEND_REG3_EXT_ILLEGAL_K_REG2_OR_93_o1  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/EXTEND_REG3_419 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/CGBAD_REG3_408 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_REG2_392 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/EXTEND_REG3_EXT_ILLEGAL_K_REG2_OR_93_o )
  );
  LUT3 #(
    .INIT ( 8'hAB ))
  \U0/gpcs_pma_inst/RECEIVER/EOP_REG1_SYNC_STATUS_OR_97_o1  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/EOP_REG1_400 ),
    .I1(\U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_129 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/RECEIVE_379 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/EOP_REG1_SYNC_STATUS_OR_97_o )
  );
  LUT3 #(
    .INIT ( 8'hF8 ))
  \U0/gpcs_pma_inst/RECEIVER/EOP_EXTEND_OR_75_o1  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/EXTEND_378 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1_420 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/EOP_401 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/EOP_EXTEND_OR_75_o )
  );
  LUT4 #(
    .INIT ( 16'h0002 ))
  \U0/gpcs_pma_inst/RECEIVER/RXCHARISK_REG1_K28p5_REG1_AND_184_o1  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/RXCHARISK_REG1_426 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_434 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/R_410 ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/T_437 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/RXCHARISK_REG1_K28p5_REG1_AND_184_o )
  );
  LUT2 #(
    .INIT ( 4'h8 ))
  \U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_EVEN_AND_144_o1  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_434 ),
    .I1(\U0/gpcs_pma_inst/SYNCHRONISATION/EVEN_130 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_EVEN_AND_144_o )
  );
  LUT3 #(
    .INIT ( 8'hFE ))
  \U0/gpcs_pma_inst/RECEIVER/RXFIFO_ERR_RXDISPERR_OR_46_o1  (
    .I0(\U0/gpcs_pma_inst/RXBUFSTATUS_INT [1]),
    .I1(\U0/gpcs_pma_inst/RXNOTINTABLE_INT_115 ),
    .I2(\U0/gpcs_pma_inst/RXDISPERR_INT_116 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/RXFIFO_ERR_RXDISPERR_OR_46_o )
  );
  LUT2 #(
    .INIT ( 4'hB ))
  \U0/gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_61_o1  (
    .I0(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .I1(\U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_129 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_61_o )
  );
  LUT4 #(
    .INIT ( 16'hFFFE ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT3_SW0  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/V_197 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1 [2]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/T_207 ),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/R_198 ),
    .O(N2)
  );
  LUT6 #(
    .INIT ( 64'hFFFFBBAB55551101 ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT3  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_200 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/S_209 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_199 ),
    .I3(N2),
    .I4(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ),
    .I5(\U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [2]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<2> )
  );
  LUT4 #(
    .INIT ( 16'hFFFE ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT4_SW0  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/V_197 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/T_207 ),
    .I2(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/S_209 ),
    .O(N6)
  );
  LUT6 #(
    .INIT ( 64'hFFFFBBAB55551101 ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT4  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_200 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/R_198 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_199 ),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1 [3]),
    .I4(N6),
    .I5(\U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [3]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<3> )
  );
  LUT4 #(
    .INIT ( 16'hFFFE ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT7_SW0  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/V_197 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/T_207 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/R_198 ),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/S_209 ),
    .O(N8)
  );
  LUT6 #(
    .INIT ( 64'hDDDDDCCC11111000 ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT7  (
    .I0(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_200 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_199 ),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1 [6]),
    .I4(N8),
    .I5(\U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [1]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<6> )
  );
  LUT6 #(
    .INIT ( 64'hFFFFFFFDFFFFFFFF ))
  \U0/gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_XMIT_DATA_INT_AND_37_o1  (
    .I0(gmii_txd[3]),
    .I1(gmii_txd[7]),
    .I2(gmii_txd[4]),
    .I3(gmii_txd[5]),
    .I4(gmii_txd[6]),
    .I5(gmii_txd[2]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_XMIT_DATA_INT_AND_37_o1_442 )
  );
  LUT6 #(
    .INIT ( 64'hA8AAAAAA20222222 ))
  \U0/gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_XMIT_DATA_INT_AND_37_o2  (
    .I0(gmii_tx_er),
    .I1(gmii_tx_en),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_XMIT_DATA_INT_AND_37_o1_442 ),
    .I3(gmii_txd[0]),
    .I4(gmii_txd[1]),
    .I5(\U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_199 ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_XMIT_DATA_INT_AND_37_o2_443 )
  );
  LUT6 #(
    .INIT ( 64'hFFFFFFFFFFFF5540 ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT21  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/T_207 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1 [1]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_199 ),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/R_198 ),
    .I4(\U0/gpcs_pma_inst/TRANSMITTER/S_209 ),
    .I5(\U0/gpcs_pma_inst/TRANSMITTER/V_197 ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT2 )
  );
  LUT4 #(
    .INIT ( 16'hAE04 ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT22  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_200 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT2 ),
    .I2(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [1]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<1> )
  );
  LUT6 #(
    .INIT ( 64'hFFFFFFFF55555540 ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT11  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/V_197 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TXD_REG1 [0]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_199 ),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/T_207 ),
    .I4(\U0/gpcs_pma_inst/TRANSMITTER/R_198 ),
    .I5(\U0/gpcs_pma_inst/TRANSMITTER/S_209 ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT1 )
  );
  LUT4 #(
    .INIT ( 16'hAE04 ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT12  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_200 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/Mmux_PWR_22_o_CONFIG_DATA[7]_mux_21_OUT1 ),
    .I2(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [0]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/PWR_22_o_CONFIG_DATA[7]_mux_21_OUT<0> )
  );
  LUT3 #(
    .INIT ( 8'h04 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT_xor<1>1_SW0  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [3]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [2]),
    .O(N10)
  );
  LUT6 #(
    .INIT ( 64'h9999999909099899 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT_xor<1>1  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [1]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [0]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I3(N10),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1_306 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT1 )
  );
  LUT3 #(
    .INIT ( 8'hFB ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0147_inv_SW0  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [1]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [2]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [0]),
    .O(N12)
  );
  LUT6 #(
    .INIT ( 64'h0000000001000000 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0147_inv  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [3]),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_248 ),
    .I5(N12),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0147_inv_283 )
  );
  LUT3 #(
    .INIT ( 8'h01 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB1  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [2]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [3]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [1]),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB1_448 )
  );
  LUT3 #(
    .INIT ( 8'h01 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB2  (
    .I0(phyad[3]),
    .I1(phyad[4]),
    .I2(phyad[2]),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB3_449 )
  );
  LUT6 #(
    .INIT ( 64'h002008FF00000000 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB3  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB3_449 ),
    .I1(phyad[1]),
    .I2(phyad[0]),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [0]),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_IN_REG_258 ),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB1_448 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB4_450 )
  );
  LUT6 #(
    .INIT ( 64'h8040201008040216 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB4  (
    .I0(phyad[3]),
    .I1(phyad[4]),
    .I2(phyad[2]),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [2]),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [3]),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [1]),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB5_451 )
  );
  LUT4 #(
    .INIT ( 16'h9810 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB5  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [0]),
    .I1(phyad[1]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB5_451 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB2_271 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB6_452 )
  );
  LUT6 #(
    .INIT ( 64'h8008000020020000 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB6  (
    .I0(phyad[1]),
    .I1(phyad[2]),
    .I2(phyad[3]),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [2]),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [0]),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [1]),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB7_453 )
  );
  LUT6 #(
    .INIT ( 64'hC3D70055C3C30000 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB7  (
    .I0(phyad[1]),
    .I1(phyad[4]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [3]),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [0]),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB7_453 ),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB2_271 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB8_454 )
  );
  LUT5 #(
    .INIT ( 32'hF9F8F1F0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB8  (
    .I0(phyad[0]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_IN_REG_258 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB4_450 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB6_452 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB8_454 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB )
  );
  LUT3 #(
    .INIT ( 8'h01 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT_xor<0>1_SW0  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [3]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [2]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [1]),
    .O(N14)
  );
  LUT6 #(
    .INIT ( 64'h55555555D145D155 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT_xor<0>1  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [0]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I4(N14),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1_306 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT )
  );
  LUT5 #(
    .INIT ( 32'hABC4AAC4 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3-In_SW0  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1_306 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_IN_REG_258 ),
    .O(N16)
  );
  LUT4 #(
    .INIT ( 16'hEEE4 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3-In  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_248 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I2(N16),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3-In1 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3-In_281 )
  );
  LUT5 #(
    .INIT ( 32'hA2A2A2F6 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In2  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1_306 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [3]),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT_xor<3>14 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In3_457 )
  );
  LUT2 #(
    .INIT ( 4'h7 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mmux_STATE[3]_PWR_20_o_Mux_36_o11_SW0  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/OPCODE [1]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_307 ),
    .O(N18)
  );
  LUT6 #(
    .INIT ( 64'hFFFFFFFFFFFFAFAD ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mmux_STATE[3]_PWR_20_o_Mux_36_o11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1_306 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/OPCODE [0]),
    .I5(N18),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mmux_STATE[3]_PWR_20_o_Mux_36_o11_270 )
  );
  LUT5 #(
    .INIT ( 32'h7F7FFEFF ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux61_SW0  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_IN_REG_258 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [1]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [0]),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/RESET_REG_138 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [2]),
    .O(N20)
  );
  LUT5 #(
    .INIT ( 32'hABAA0300 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux61  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [14]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [3]),
    .I2(N20),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3-In1 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux1011 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<15> )
  );
  LUT2 #(
    .INIT ( 4'hB ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux16_SW0  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [1]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [0]),
    .O(N22)
  );
  LUT6 #(
    .INIT ( 64'hF0F4F0F000040000 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux16  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [2]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [3]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_IN_REG_258 ),
    .I3(N22),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3-In1 ),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux1011 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<0> )
  );
  LUT6 #(
    .INIT ( 64'h7777555722220002 ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3-In32  (
    .I0(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_330 ),
    .I1(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_329 ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/CGBAD ),
    .I3(\U0/gpcs_pma_inst/RXCHARISK_INT_125 ),
    .I4(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_327 ),
    .I5(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3-In31_461 ),
    .O(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3-In3 )
  );
  LUT6 #(
    .INIT ( 64'h91C49BE4DD80DFA0 ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2-In21  (
    .I0(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_330 ),
    .I1(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_327 ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_329 ),
    .I3(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_328 ),
    .I4(\U0/gpcs_pma_inst/RXCHARISK_INT_125 ),
    .I5(\U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_PWR_23_o_equal_19_o ),
    .O(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2-In21_462 )
  );
  LUT5 #(
    .INIT ( 32'h4040FF40 ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2-In22  (
    .I0(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_330 ),
    .I1(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_328 ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_329 ),
    .I3(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2-In21_462 ),
    .I4(\U0/gpcs_pma_inst/SYNCHRONISATION/CGBAD ),
    .O(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2-In2 )
  );
  LUT2 #(
    .INIT ( 4'hE ))
  \U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1_ISOLATE_AND_199_o_SW0  (
    .I0(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ),
    .I1(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/POWERDOWN_REG ),
    .O(N28)
  );
  LUT6 #(
    .INIT ( 64'h5555555144444440 ))
  \U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1_ISOLATE_AND_199_o  (
    .I0(N28),
    .I1(\U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_129 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/RX_DATA_ERROR_399 ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_405 ),
    .I4(\U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1_420 ),
    .I5(\U0/gpcs_pma_inst/RECEIVER/RECEIVE_379 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1_ISOLATE_AND_199_o_367 )
  );
  LUT4 #(
    .INIT ( 16'hAAA8 ))
  \U0/gpcs_pma_inst/RECEIVER/EVEN_RXCHARISK_AND_132_o_SW0  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/I_REG_429 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_389 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/FALSE_K_390 ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_391 ),
    .O(N32)
  );
  LUT6 #(
    .INIT ( 64'h00000000AA088808 ))
  \U0/gpcs_pma_inst/RECEIVER/EVEN_RXCHARISK_AND_132_o  (
    .I0(\U0/gpcs_pma_inst/SYNCHRONISATION/EVEN_130 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_434 ),
    .I2(\U0/gpcs_pma_inst/RXCHARISK_INT_125 ),
    .I3(\U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_129 ),
    .I4(N32),
    .I5(\U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_D21p5_AND_133_o_norst ),
    .O(\U0/gpcs_pma_inst/RECEIVER/EVEN_RXCHARISK_AND_132_o_369 )
  );
  LUT2 #(
    .INIT ( 4'hB ))
  \U0/gpcs_pma_inst/RECEIVER/K28p51_SW0  (
    .I0(\U0/gpcs_pma_inst/RXDATA_INT [0]),
    .I1(\U0/gpcs_pma_inst/RXCHARISK_INT_125 ),
    .O(N34)
  );
  LUT6 #(
    .INIT ( 64'h0000000000800000 ))
  \U0/gpcs_pma_inst/RECEIVER/K28p51  (
    .I0(\U0/gpcs_pma_inst/RXDATA_INT [7]),
    .I1(\U0/gpcs_pma_inst/RXDATA_INT [3]),
    .I2(\U0/gpcs_pma_inst/RXDATA_INT [2]),
    .I3(\U0/gpcs_pma_inst/RXDATA_INT [1]),
    .I4(\U0/gpcs_pma_inst/RXDATA_INT [4]),
    .I5(N34),
    .O(\U0/gpcs_pma_inst/RECEIVER/K28p51_345 )
  );
  LUT6 #(
    .INIT ( 64'h4000000000000000 ))
  \U0/gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_48_o1  (
    .I0(\U0/gpcs_pma_inst/RXDATA_INT [6]),
    .I1(\U0/gpcs_pma_inst/RXDATA_INT [7]),
    .I2(\U0/gpcs_pma_inst/RXDATA_INT [0]),
    .I3(\U0/gpcs_pma_inst/RXDATA_INT [2]),
    .I4(\U0/gpcs_pma_inst/RXDATA_INT [4]),
    .I5(\U0/gpcs_pma_inst/RXDATA_INT [5]),
    .O(\U0/gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_48_o )
  );
  LUT6 #(
    .INIT ( 64'h0000000001000000 ))
  \U0/gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_48_o2  (
    .I0(\U0/gpcs_pma_inst/RXDATA_INT [7]),
    .I1(\U0/gpcs_pma_inst/RXDATA_INT [5]),
    .I2(\U0/gpcs_pma_inst/RXDATA_INT [4]),
    .I3(\U0/gpcs_pma_inst/RXDATA_INT [6]),
    .I4(\U0/gpcs_pma_inst/RXDATA_INT [1]),
    .I5(\U0/gpcs_pma_inst/RXDATA_INT [2]),
    .O(\U0/gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_48_o1_467 )
  );
  LUT6 #(
    .INIT ( 64'h0013000300110000 ))
  \U0/gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_48_o3  (
    .I0(\U0/gpcs_pma_inst/RXDATA_INT [0]),
    .I1(\U0/gpcs_pma_inst/RXDATA_INT [3]),
    .I2(\U0/gpcs_pma_inst/RXDATA_INT [1]),
    .I3(\U0/gpcs_pma_inst/RXCHARISK_INT_125 ),
    .I4(\U0/gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_48_o1_467 ),
    .I5(\U0/gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_48_o ),
    .O(\U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_D21p5_AND_133_o_norst )
  );
  LUT4 #(
    .INIT ( 16'hFFFE ))
  \U0/gpcs_pma_inst/RECEIVER/D0p0_SW0  (
    .I0(\U0/gpcs_pma_inst/RXDATA_INT [5]),
    .I1(\U0/gpcs_pma_inst/RXDATA_INT [4]),
    .I2(\U0/gpcs_pma_inst/RXCHARISK_INT_125 ),
    .I3(\U0/gpcs_pma_inst/RXDATA_INT [0]),
    .O(N36)
  );
  LUT6 #(
    .INIT ( 64'h0000000000000001 ))
  \U0/gpcs_pma_inst/RECEIVER/D0p0  (
    .I0(\U0/gpcs_pma_inst/RXDATA_INT [7]),
    .I1(\U0/gpcs_pma_inst/RXDATA_INT [3]),
    .I2(\U0/gpcs_pma_inst/RXDATA_INT [2]),
    .I3(\U0/gpcs_pma_inst/RXDATA_INT [1]),
    .I4(\U0/gpcs_pma_inst/RXDATA_INT [6]),
    .I5(N36),
    .O(\U0/gpcs_pma_inst/RECEIVER/D0p0_352 )
  );
  LUT2 #(
    .INIT ( 4'hE ))
  \U0/gpcs_pma_inst/RECEIVER/C_REG1_C_REG3_OR_69_o_SW0  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/C_REG1_428 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/C_REG2_346 ),
    .O(N38)
  );
  LUT6 #(
    .INIT ( 64'hFFFF8AAACEEE8AAA ))
  \U0/gpcs_pma_inst/RECEIVER/C_REG1_C_REG3_OR_69_o  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/C_REG3_427 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/CGBAD_409 ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/EVEN_130 ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_434 ),
    .I4(N38),
    .I5(\U0/gpcs_pma_inst/RECEIVER/RXCHARISK_REG1_426 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/C_REG1_C_REG3_OR_69_o_350 )
  );
  LUT5 #(
    .INIT ( 32'h00200000 ))
  \U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o1  (
    .I0(\U0/gpcs_pma_inst/RXDATA_INT [0]),
    .I1(\U0/gpcs_pma_inst/RXDATA_INT [7]),
    .I2(\U0/gpcs_pma_inst/RXDATA_INT [6]),
    .I3(\U0/gpcs_pma_inst/RXDATA_INT [5]),
    .I4(\U0/gpcs_pma_inst/RXDATA_INT [1]),
    .O(\U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o1_470 )
  );
  LUT4 #(
    .INIT ( 16'h1118 ))
  \U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o2  (
    .I0(\U0/gpcs_pma_inst/RXDATA_INT [1]),
    .I1(\U0/gpcs_pma_inst/RXDATA_INT [0]),
    .I2(\U0/gpcs_pma_inst/RXDATA_INT [3]),
    .I3(\U0/gpcs_pma_inst/RXDATA_INT [4]),
    .O(\U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o2_471 )
  );
  LUT4 #(
    .INIT ( 16'h2000 ))
  \U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o3  (
    .I0(\U0/gpcs_pma_inst/RXDATA_INT [5]),
    .I1(\U0/gpcs_pma_inst/RXDATA_INT [6]),
    .I2(\U0/gpcs_pma_inst/RXDATA_INT [7]),
    .I3(\U0/gpcs_pma_inst/RXDATA_INT [2]),
    .O(\U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o3_472 )
  );
  LUT6 #(
    .INIT ( 64'hFF171717FF000000 ))
  \U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o4  (
    .I0(\U0/gpcs_pma_inst/RXDATA_INT [4]),
    .I1(\U0/gpcs_pma_inst/RXDATA_INT [3]),
    .I2(\U0/gpcs_pma_inst/RXDATA_INT [2]),
    .I3(\U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o3_472 ),
    .I4(\U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o2_471 ),
    .I5(\U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o1_470 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o4_473 )
  );
  LUT3 #(
    .INIT ( 8'h10 ))
  \U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o5  (
    .I0(\U0/gpcs_pma_inst/RXNOTINTABLE_INT_115 ),
    .I1(\U0/gpcs_pma_inst/RXCHARISK_INT_125 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o4_473 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_220_o )
  );
  LUT6 #(
    .INIT ( 64'hE8FFFFFFFFFFFFFF ))
  \U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_118_o11  (
    .I0(\U0/gpcs_pma_inst/RXDATA_INT [7]),
    .I1(\U0/gpcs_pma_inst/RXDISPERR_INT_116 ),
    .I2(\U0/gpcs_pma_inst/RXDATA_INT [1]),
    .I3(\U0/gpcs_pma_inst/RXDATA_INT [6]),
    .I4(\U0/gpcs_pma_inst/RXDATA_INT [4]),
    .I5(\U0/gpcs_pma_inst/RXDATA_INT [3]),
    .O(\U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_118_o1 )
  );
  LUT5 #(
    .INIT ( 32'hFFFFFFFE ))
  \U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_118_o13  (
    .I0(\U0/gpcs_pma_inst/RXDATA_INT [4]),
    .I1(\U0/gpcs_pma_inst/RXDATA_INT [6]),
    .I2(\U0/gpcs_pma_inst/RXCHARISK_INT_125 ),
    .I3(\U0/gpcs_pma_inst/RXDATA_INT [2]),
    .I4(\U0/gpcs_pma_inst/RXDATA_INT [3]),
    .O(\U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_118_o13_476 )
  );
  LUT5 #(
    .INIT ( 32'h88888000 ))
  \U0/gpcs_pma_inst/RECEIVER/I_REG_T_REG2_OR_74_o1  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/T_REG2_431 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/R_REG1_430 ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/EVEN_130 ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_434 ),
    .I4(\U0/gpcs_pma_inst/RECEIVER/R_410 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/I_REG_T_REG2_OR_74_o1_477 )
  );
  LUT6 #(
    .INIT ( 64'hFFFFFF80FF80FF80 ))
  \U0/gpcs_pma_inst/RECEIVER/I_REG_T_REG2_OR_74_o2  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/C_REG1_428 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/D0p0_REG_433 ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/EVEN_130 ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/I_REG_T_REG2_OR_74_o1_477 ),
    .I4(\U0/gpcs_pma_inst/RECEIVER/I_REG_429 ),
    .I5(\U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_434 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/I_REG_T_REG2_OR_74_o )
  );
  LUT5 #(
    .INIT ( 32'h54545554 ))
  \U0/gpcs_pma_inst/RECEIVER/T_REG2_R_REG1_OR_89_o2  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/R_REG1_430 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/T_REG2_431 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_434 ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/R_410 ),
    .I4(\U0/gpcs_pma_inst/RECEIVER/T_REG1_432 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/T_REG2_R_REG1_OR_89_o2_478 )
  );
  LUT2 #(
    .INIT ( 4'hE ))
  \U0/gpcs_pma_inst/RECEIVER/SYNC_STATUS_C_REG1_AND_142_o_SW0  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/RXCHARISK_REG1_426 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/CGBAD_409 ),
    .O(N40)
  );
  LUT6 #(
    .INIT ( 64'h0010001000100000 ))
  \U0/gpcs_pma_inst/RECEIVER/SYNC_STATUS_C_REG1_AND_142_o  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/RXFIFO_ERR_RXDISPERR_OR_46_o ),
    .I1(\U0/gpcs_pma_inst/RXCHARISK_INT_125 ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_129 ),
    .I3(N40),
    .I4(\U0/gpcs_pma_inst/RECEIVER/C_REG1_428 ),
    .I5(\U0/gpcs_pma_inst/RECEIVER/C_HDR_REMOVED_REG_424 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/SYNC_STATUS_C_REG1_AND_142_o_368 )
  );
  FDR   \U0/gpcs_pma_inst/TRANSMITTER/V  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/V_glue_set_480 ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/V_197 )
  );
  FDR   \U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_glue_set_481 ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_199 )
  );
  FDR   \U0/gpcs_pma_inst/TRANSMITTER/R  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/R_glue_set_482 ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/R_198 )
  );
  FDS   \U0/gpcs_pma_inst/TRANSMITTER/DISPARITY  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/DISPARITY_glue_rst_483 ),
    .S(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/DISPARITY_196 )
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/POWERDOWN_REG  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/POWERDOWN_REG_glue_set_484 ),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/POWERDOWN_REG )
  );
  FDR   \U0/gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN_glue_set_485 ),
    .R(\U0/gpcs_pma_inst/SYNCHRONISATION/CGBAD_GND_27_o_AND_69_o ),
    .Q(\NlwRenamedSig_OI_U0/gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN )
  );
  FDS #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_glue_rst_486 ),
    .S(\U0/gpcs_pma_inst/SYNCHRONISATION/CGBAD_GND_27_o_AND_69_o ),
    .Q(\U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_129 )
  );
  FDR   \U0/gpcs_pma_inst/SYNCHRONISATION/EVEN  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/SYNCHRONISATION/EVEN_glue_set_487 ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/SYNCHRONISATION/EVEN_130 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/RECEIVE  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RECEIVE_glue_set_488 ),
    .R(\U0/gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_61_o ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RECEIVE_379 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/RX_INVALID  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RX_INVALID_glue_set_489 ),
    .R(\U0/gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_61_o ),
    .Q(\NlwRenamedSig_OI_U0/gpcs_pma_inst/RECEIVER/RX_INVALID )
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/RECEIVER/RX_DV  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RX_DV_glue_set_490 ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\NlwRenamedSig_OI_U0/gpcs_pma_inst/RECEIVER/RX_DV )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/EXTEND  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/EXTEND_glue_set_491 ),
    .R(\U0/gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_61_o ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/EXTEND_378 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_glue_set_492 ),
    .R(\U0/gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_61_o ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_380 )
  );
  FDR   \U0/gpcs_pma_inst/RECEIVER/WAIT_FOR_K  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/WAIT_FOR_K_glue_set_493 ),
    .R(\U0/gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_61_o ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/WAIT_FOR_K_381 )
  );
  FDR   \U0/gpcs_pma_inst/TRANSMITTER/C1_OR_C2  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/C1_OR_C2_rstpot_494 ),
    .R(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/C1_OR_C2_202 )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/WE_rstpot  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG2_308 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/WE_249 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/GND_24_o_GND_24_o_MUX_62_o ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/WE_rstpot_495 )
  );
  FDR   \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/WE  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/WE_rstpot_495 ),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/WE_249 )
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_rstpot_496 ),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_307 )
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/RESET_REG  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/RESET_REG_rstpot_497 ),
    .R(\U0/gpcs_pma_inst/SRESET_127 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/RESET_REG_138 )
  );
  FD   \U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd4  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd4_rstpot_498 ),
    .Q(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd4_82 )
  );
  FD   \U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd4  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd4_rstpot_499 ),
    .Q(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd4_89 )
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRPISK  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_rstpot_500 ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_204 )
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/TXCHARDISPVAL  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/TXCHARDISPVAL_rstpot_501 ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TXCHARDISPVAL_139 )
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/TRIGGER_T  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/TRIGGER_T_rstpot_502 ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/TRIGGER_T_206 )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/gpcs_pma_inst/TRANSMITTER/S_rstpot  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/TX_EN_TRIGGER_S_OR_25_o_0 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/XMIT_DATA_INT_201 ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/S_rstpot_503 )
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/S  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/S_rstpot_503 ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/S_209 )
  );
  FD   \U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA_0  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA_0_rstpot_504 ),
    .Q(\U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [0])
  );
  FD   \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/CONFIGURATION_VALID_EN  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/CONFIGURATION_VALID_EN_rstpot_505 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/CONFIGURATION_VALID_EN_247 )
  );
  FD   \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_rstpot_506 ),
    .Q(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_248 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/C_HDR_REMOVED_REG  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/C_HDR_REMOVED_REG_rstpot_507 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/C_HDR_REMOVED_REG_424 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/C  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/C_rstpot_508 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/C_435 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_rstpot_509 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_394 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/RX_DATA_ERROR  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/RX_DATA_ERROR_rstpot_510 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RX_DATA_ERROR_399 )
  );
  LUT6 #(
    .INIT ( 64'hFFFFFFFFA9999995 ))
  \U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_118_o12  (
    .I0(\U0/gpcs_pma_inst/RXDATA_INT [0]),
    .I1(\U0/gpcs_pma_inst/RXDATA_INT [5]),
    .I2(\U0/gpcs_pma_inst/RXDISPERR_INT_116 ),
    .I3(\U0/gpcs_pma_inst/RXDATA_INT [7]),
    .I4(\U0/gpcs_pma_inst/RXDATA_INT [1]),
    .I5(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .O(\U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_118_o12_475 )
  );
  LUT5 #(
    .INIT ( 32'hA8AAAAAA ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux1511  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [8]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In1 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<9> )
  );
  LUT5 #(
    .INIT ( 32'hA8AAAAAA ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux1012  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [3]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In1 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<4> )
  );
  LUT5 #(
    .INIT ( 32'hA8AAAAAA ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux911  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [2]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In1 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<3> )
  );
  LUT5 #(
    .INIT ( 32'hA8AAAAAA ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux711  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [0]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In1 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<1> )
  );
  LUT5 #(
    .INIT ( 32'hA8AAAAAA ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux411  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [12]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In1 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<13> )
  );
  LUT5 #(
    .INIT ( 32'hA8AAAAAA ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/mux311  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [11]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In1 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG[14]_DATA_IN[15]_mux_25_OUT<12> )
  );
  LUT4 #(
    .INIT ( 16'hFFFE ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In11  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [3]),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [2]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [0]),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [1]),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In1 )
  );
  FD   \U0/gpcs_pma_inst/RECEIVER/FALSE_NIT  (
    .C(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_rstpot_511 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_389 )
  );
  LUT6 #(
    .INIT ( 64'hAAAAAAEAAAAAAA2A ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_rstpot  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_307 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_248 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_COMB ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/ADDRESS_MATCH_rstpot_496 )
  );
  LUT6 #(
    .INIT ( 64'h0000000000200000 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0155_inv1  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_248 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/BIT_COUNT [3]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/Mcount_BIT_COUNT_xor<3>14 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/_n0155_inv )
  );
  LUT6 #(
    .INIT ( 64'hFFFFDD80AAFF8880 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/POWERDOWN_REG_glue_set  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/GND_22_o_MDIO_WE_AND_14_o ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [11]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG3_250 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/CONFIGURATION_VALID_EN_247 ),
    .I4(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/POWERDOWN_REG ),
    .I5(configuration_vector[2]),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/POWERDOWN_REG_glue_set_484 )
  );
  LUT6 #(
    .INIT ( 64'h1000FFFF10001000 ))
  \U0/gpcs_pma_inst/RECEIVER/RX_DV_glue_set  (
    .I0(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/POWERDOWN_REG ),
    .I1(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_129 ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/SOP_REG3_421 ),
    .I4(\U0/gpcs_pma_inst/RECEIVER/EOP_REG1_SYNC_STATUS_OR_97_o ),
    .I5(\NlwRenamedSig_OI_U0/gpcs_pma_inst/RECEIVER/RX_DV ),
    .O(\U0/gpcs_pma_inst/RECEIVER/RX_DV_glue_set_490 )
  );
  LUT4 #(
    .INIT ( 16'h6A2A ))
  \U0/gpcs_pma_inst/TRANSMITTER/C1_OR_C2_rstpot  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/C1_OR_C2_202 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [1]),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_200 ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/C1_OR_C2_rstpot_494 )
  );
  LUT3 #(
    .INIT ( 8'h2F ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/EVEN_glue_set  (
    .I0(\U0/gpcs_pma_inst/RXCHARISCOMMA_INT_126 ),
    .I1(\U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_129 ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/EVEN_130 ),
    .O(\U0/gpcs_pma_inst/SYNCHRONISATION/EVEN_glue_set_487 )
  );
  LUT6 #(
    .INIT ( 64'h0001010101010101 ))
  \U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd4_rstpot  (
    .I0(\U0/gpcs_pma_inst/RESET_INT_134 ),
    .I1(\U0/gpcs_pma_inst/RXBUFSTATUS_INT [1]),
    .I2(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd4_82 ),
    .I3(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd2_80 ),
    .I4(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd3_81 ),
    .I5(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd1_79 ),
    .O(\U0/gpcs_pma_inst/RX_RST_SM_FSM_FFd4_rstpot_498 )
  );
  LUT6 #(
    .INIT ( 64'h0001010101010101 ))
  \U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd4_rstpot  (
    .I0(\U0/gpcs_pma_inst/RESET_INT_134 ),
    .I1(\U0/gpcs_pma_inst/TXBUFERR_INT_110 ),
    .I2(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd4_89 ),
    .I3(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd2_87 ),
    .I4(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd3_88 ),
    .I5(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd1_86 ),
    .O(\U0/gpcs_pma_inst/TX_RST_SM_FSM_FFd4_rstpot_499 )
  );
  LUT6 #(
    .INIT ( 64'h4114055014411441 ))
  \U0/gpcs_pma_inst/TRANSMITTER/DISPARITY_glue_rst  (
    .I0(N47),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [6]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/DISP5 ),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/DISPARITY_196 ),
    .I4(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [7]),
    .I5(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [5]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/DISPARITY_glue_rst_483 )
  );
  LUT2 #(
    .INIT ( 4'hB ))
  \U0/gpcs_pma_inst/TRANSMITTER/V_glue_set_SW0  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_199 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TX_ER_REG1_222 ),
    .O(N49)
  );
  LUT4 #(
    .INIT ( 16'h0002 ))
  \U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA_0_rstpot  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I1(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/C1_OR_C2_202 ),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [1]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/CONFIG_DATA_0_rstpot_504 )
  );
  LUT4 #(
    .INIT ( 16'hFF80 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/RESET_REG_rstpot  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDC_RISING_REG3_250 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/SHIFT_REG [15]),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/GND_22_o_MDIO_WE_AND_14_o ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/RESET_REG_138 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/RESET_REG_rstpot_497 )
  );
  LUT3 #(
    .INIT ( 8'hBA ))
  \U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_glue_set  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/S_209 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/T_207 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_199 ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_glue_set_481 )
  );
  LUT3 #(
    .INIT ( 8'hBA ))
  \U0/gpcs_pma_inst/RECEIVER/RECEIVE_glue_set  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/SOP_REG2_422 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/EOP_401 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/RECEIVE_379 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/RECEIVE_glue_set_488 )
  );
  LUT3 #(
    .INIT ( 8'hBA ))
  \U0/gpcs_pma_inst/RECEIVER/RX_INVALID_glue_set  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/FROM_RX_CX_403 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_434 ),
    .I2(\NlwRenamedSig_OI_U0/gpcs_pma_inst/RECEIVER/RX_INVALID ),
    .O(\U0/gpcs_pma_inst/RECEIVER/RX_INVALID_glue_set_489 )
  );
  LUT3 #(
    .INIT ( 8'h04 ))
  \U0/gpcs_pma_inst/TRANSMITTER/TRIGGER_T_rstpot  (
    .I0(gmii_tx_en),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_223 ),
    .I2(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/TRIGGER_T_rstpot_502 )
  );
  LUT3 #(
    .INIT ( 8'h04 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/CONFIGURATION_VALID_EN_rstpot  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/CONFIGURATION_VALID_EN_REG_246 ),
    .I1(configuration_valid),
    .I2(\U0/gpcs_pma_inst/SRESET_127 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/CONFIGURATION_VALID_EN_rstpot_505 )
  );
  LUT3 #(
    .INIT ( 8'h04 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_rstpot  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_REG3_245 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_REG2 ),
    .I2(\U0/gpcs_pma_inst/SRESET_127 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_rstpot_506 )
  );
  LUT4 #(
    .INIT ( 16'h0040 ))
  \U0/gpcs_pma_inst/RECEIVER/C_HDR_REMOVED_REG_rstpot  (
    .I0(\U0/gpcs_pma_inst/RXCLKCORCNT_INT [1]),
    .I1(\U0/gpcs_pma_inst/RECEIVER/C_REG2_346 ),
    .I2(\U0/gpcs_pma_inst/RXCLKCORCNT_INT [0]),
    .I3(\U0/gpcs_pma_inst/RXCLKCORCNT_INT [2]),
    .O(\U0/gpcs_pma_inst/RECEIVER/C_HDR_REMOVED_REG_rstpot_507 )
  );
  LUT6 #(
    .INIT ( 64'h2222222200020000 ))
  \U0/gpcs_pma_inst/RECEIVER/RX_DATA_ERROR_rstpot  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/RECEIVE_379 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_61_o ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_EVEN_AND_144_o ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/R_410 ),
    .I4(\U0/gpcs_pma_inst/RECEIVER/T_REG2_431 ),
    .I5(N55),
    .O(\U0/gpcs_pma_inst/RECEIVER/RX_DATA_ERROR_rstpot_510 )
  );
  LUT2 #(
    .INIT ( 4'h8 ))
  \U0/gpcs_pma_inst/RECEIVER/EXTEND_glue_set_SW0  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/RECEIVE_379 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/R_REG1_430 ),
    .O(N57)
  );
  LUT6 #(
    .INIT ( 64'hFFFF022202220222 ))
  \U0/gpcs_pma_inst/RECEIVER/EXTEND_glue_set  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/EXTEND_378 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/S_438 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_434 ),
    .I3(\U0/gpcs_pma_inst/SYNCHRONISATION/EVEN_130 ),
    .I4(N57),
    .I5(\U0/gpcs_pma_inst/RECEIVER/R_410 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/EXTEND_glue_set_491 )
  );
  LUT2 #(
    .INIT ( 4'h8 ))
  \U0/gpcs_pma_inst/RECEIVER/C_rstpot  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_434 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_D21p5_AND_133_o_norst ),
    .O(\U0/gpcs_pma_inst/RECEIVER/C_rstpot_508 )
  );
  LUT6 #(
    .INIT ( 64'hFFFFFFFF88808085 ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN_glue_set  (
    .I0(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_327 ),
    .I1(\U0/gpcs_pma_inst/SYNCHRONISATION/CGBAD ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_328 ),
    .I3(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_329 ),
    .I4(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_330 ),
    .I5(\NlwRenamedSig_OI_U0/gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN ),
    .O(\U0/gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN_glue_set_485 )
  );
  LUT3 #(
    .INIT ( 8'h2A ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_CODE_GRPISK_GND_26_o_MUX_192_o11  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_204 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_205 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_GND_26_o_MUX_192_o )
  );
  LUT4 #(
    .INIT ( 16'hEA2A ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_CODE_GRP[7]_GND_26_o_mux_24_OUT11  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [0]),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_205 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/DISPARITY_196 ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<0> )
  );
  LUT3 #(
    .INIT ( 8'h2A ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_CODE_GRP[7]_GND_26_o_mux_24_OUT21  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [1]),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_205 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<1> )
  );
  LUT4 #(
    .INIT ( 16'hEA2A ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_CODE_GRP[7]_GND_26_o_mux_24_OUT31  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [2]),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_205 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/DISPARITY_196 ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<2> )
  );
  LUT3 #(
    .INIT ( 8'h2A ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_CODE_GRP[7]_GND_26_o_mux_24_OUT41  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [3]),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_205 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<3> )
  );
  LUT4 #(
    .INIT ( 16'h2AEA ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_CODE_GRP[7]_GND_26_o_mux_24_OUT51  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [4]),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_205 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/DISPARITY_196 ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<4> )
  );
  LUT3 #(
    .INIT ( 8'h2A ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_CODE_GRP[7]_GND_26_o_mux_24_OUT61  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [5]),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_205 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<5> )
  );
  LUT3 #(
    .INIT ( 8'hF8 ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_CODE_GRP[7]_GND_26_o_mux_24_OUT71  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_205 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [6]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<6> )
  );
  LUT4 #(
    .INIT ( 16'hEA2A ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_CODE_GRP[7]_GND_26_o_mux_24_OUT81  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP [7]),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_205 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/DISPARITY_196 ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_26_o_mux_24_OUT<7> )
  );
  LUT3 #(
    .INIT ( 8'h20 ))
  \U0/gpcs_pma_inst/TRANSMITTER/_n0235<1>1  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/C1_OR_C2_202 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [1]),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/_n0235 [1])
  );
  LUT6 #(
    .INIT ( 64'hFFF00000FFFF0020 ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3-In31  (
    .I0(\U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS [1]),
    .I1(\U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS [0]),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_327 ),
    .I3(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_328 ),
    .I4(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_329 ),
    .I5(\U0/gpcs_pma_inst/SYNCHRONISATION/CGBAD ),
    .O(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3-In31_461 )
  );
  LUT4 #(
    .INIT ( 16'hFFFB ))
  \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_rstpot_SW1  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/T_207 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_199 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/S_209 ),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/V_197 ),
    .O(N59)
  );
  LUT6 #(
    .INIT ( 64'h55545554FFFE5554 ))
  \U0/gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_rstpot  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_200 ),
    .I1(N59),
    .I2(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/R_198 ),
    .I4(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I5(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [1]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_rstpot_500 )
  );
  LUT5 #(
    .INIT ( 32'hFFFF4440 ))
  \U0/gpcs_pma_inst/TRANSMITTER/R_glue_set  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/S_209 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/R_198 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/TX_ER_REG1_222 ),
    .I4(\U0/gpcs_pma_inst/TRANSMITTER/T_207 ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/R_glue_set_482 )
  );
  LUT3 #(
    .INIT ( 8'h20 ))
  \U0/gpcs_pma_inst/TRANSMITTER/DISPARITY_glue_rst_SW0  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_205 ),
    .I1(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(N47)
  );
  LUT6 #(
    .INIT ( 64'hFFFF88A888A888A8 ))
  \U0/gpcs_pma_inst/TRANSMITTER/V_glue_set  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/XMIT_DATA_INT_201 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_XMIT_DATA_INT_AND_37_o2_443 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_223 ),
    .I3(N49),
    .I4(\U0/gpcs_pma_inst/TRANSMITTER/S_209 ),
    .I5(\U0/gpcs_pma_inst/TRANSMITTER/V_197 ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/V_glue_set_480 )
  );
  LUT5 #(
    .INIT ( 32'hFFFFFFFE ))
  \U0/gpcs_pma_inst/RECEIVER/RX_DATA_ERROR_rstpot_SW0  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/I_REG_429 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/ILLEGAL_K_REG2_396 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/C_REG1_428 ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/CGBAD_REG3_408 ),
    .I4(\U0/gpcs_pma_inst/RECEIVER/T_REG2_R_REG1_OR_89_o2_478 ),
    .O(N55)
  );
  LUT4 #(
    .INIT ( 16'hFDFF ))
  \U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_glue_set_SW1  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/I_REG_429 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_389 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/S_438 ),
    .I3(\U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_129 ),
    .O(N61)
  );
  LUT6 #(
    .INIT ( 64'h44444445CCCCCCCD ))
  \U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_glue_set  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_434 ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_380 ),
    .I2(N61),
    .I3(\U0/gpcs_pma_inst/RECEIVER/FALSE_DATA_391 ),
    .I4(\U0/gpcs_pma_inst/RECEIVER/FALSE_K_390 ),
    .I5(\U0/gpcs_pma_inst/SYNCHRONISATION/EVEN_130 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_glue_set_492 )
  );
  LUT5 #(
    .INIT ( 32'h2AFF2A2A ))
  \U0/gpcs_pma_inst/RECEIVER/WAIT_FOR_K_glue_set  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/WAIT_FOR_K_381 ),
    .I1(\U0/gpcs_pma_inst/SYNCHRONISATION/EVEN_130 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_434 ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/SYNC_STATUS_REG_406 ),
    .I4(\U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_129 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/WAIT_FOR_K_glue_set_493 )
  );
  LUT4 #(
    .INIT ( 16'h0040 ))
  \U0/gpcs_pma_inst/TRANSMITTER/TXCHARDISPVAL_rstpot  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/DISPARITY_196 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_205 ),
    .I3(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_TX_RESET_INT ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/TXCHARDISPVAL_rstpot_501 )
  );
  LUT6 #(
    .INIT ( 64'h0000000001000000 ))
  \U0/gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_rstpot  (
    .I0(\NlwRenamedSig_OI_U0/gpcs_pma_inst/MGT_RX_RESET_INT ),
    .I1(\U0/gpcs_pma_inst/RECEIVER/K28p5_REG1_EVEN_AND_144_o ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/R_410 ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1_420 ),
    .I4(\U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_129 ),
    .I5(\U0/gpcs_pma_inst/RECEIVER/S_438 ),
    .O(\U0/gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_rstpot_509 )
  );
  LUT6 #(
    .INIT ( 64'h2A2A2AAA2AAA2A88 ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_glue_rst  (
    .I0(\U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_129 ),
    .I1(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_327 ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/CGBAD ),
    .I3(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_328 ),
    .I4(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_329 ),
    .I5(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_330 ),
    .O(\U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_glue_rst_486 )
  );
  MUXF7   \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In2  (
    .I0(N63),
    .I1(N64),
    .S(\U0/gpcs_pma_inst/SYNCHRONISATION/CGBAD ),
    .O(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In2_331 )
  );
  LUT6 #(
    .INIT ( 64'hF0F4540400040404 ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In2_F  (
    .I0(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_330 ),
    .I1(\U0/gpcs_pma_inst/RXCHARISCOMMA_INT_126 ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_327 ),
    .I3(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_329 ),
    .I4(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_328 ),
    .I5(\U0/gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_PWR_23_o_equal_19_o ),
    .O(N63)
  );
  LUT5 #(
    .INIT ( 32'hDBDB8988 ))
  \U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In2_G  (
    .I0(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_329 ),
    .I1(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_328 ),
    .I2(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_330 ),
    .I3(\U0/gpcs_pma_inst/RXCHARISCOMMA_INT_126 ),
    .I4(\U0/gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_327 ),
    .O(N64)
  );
  MUXF7   \U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_rstpot  (
    .I0(N65),
    .I1(N66),
    .S(\U0/gpcs_pma_inst/RXDATA_INT [5]),
    .O(\U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_rstpot_511 )
  );
  LUT6 #(
    .INIT ( 64'h0404040004000000 ))
  \U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_rstpot_F  (
    .I0(\U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_118_o12_475 ),
    .I1(\U0/gpcs_pma_inst/RXNOTINTABLE_INT_115 ),
    .I2(\U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_118_o13_476 ),
    .I3(\U0/gpcs_pma_inst/RXDISPERR_INT_116 ),
    .I4(\U0/gpcs_pma_inst/RXDATA_INT [1]),
    .I5(\U0/gpcs_pma_inst/RXDATA_INT [7]),
    .O(N65)
  );
  LUT5 #(
    .INIT ( 32'h00200000 ))
  \U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_rstpot_G  (
    .I0(\U0/gpcs_pma_inst/RXDATA_INT [2]),
    .I1(\U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_118_o12_475 ),
    .I2(\U0/gpcs_pma_inst/RXNOTINTABLE_INT_115 ),
    .I3(\U0/gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_118_o1 ),
    .I4(\U0/gpcs_pma_inst/RXCHARISK_INT_125 ),
    .O(N66)
  );
  MUXF7   \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In3  (
    .I0(N67),
    .I1(N68),
    .S(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4_303 ),
    .O(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In )
  );
  LUT6 #(
    .INIT ( 64'hAAAAAAAA00000002 ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In3_F  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_248 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1_306 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_IN_REG_258 ),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In3_457 ),
    .O(N67)
  );
  LUT6 #(
    .INIT ( 64'h44450001FFFFFFFF ))
  \U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In3_G  (
    .I0(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd3_304 ),
    .I1(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd2_305 ),
    .I2(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/MDIO_IN_REG_258 ),
    .I3(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd1_306 ),
    .I4(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDIO_INTERFACE_1/STATE_FSM_FFd4-In1 ),
    .I5(\U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/MDC_RISING_REG1_248 ),
    .O(N68)
  );
  MUXF7   \U0/gpcs_pma_inst/TRANSMITTER/Mmux_TX_PACKET_CODE_GRP_CNT[1]_MUX_186_o11  (
    .I0(N69),
    .I1(N70),
    .S(\U0/gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_200 ),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_CODE_GRP_CNT[1]_MUX_186_o )
  );
  LUT6 #(
    .INIT ( 64'hFFFFFFFF00000001 ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_TX_PACKET_CODE_GRP_CNT[1]_MUX_186_o11_F  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/S_209 ),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/V_197 ),
    .I2(\U0/gpcs_pma_inst/TRANSMITTER/T_207 ),
    .I3(\U0/gpcs_pma_inst/TRANSMITTER/R_198 ),
    .I4(\U0/gpcs_pma_inst/TRANSMITTER/TX_PACKET_199 ),
    .I5(\NlwRenamedSig_OI_U0/gpcs_pma_inst/HAS_MANAGEMENT.MDIO/ISOLATE_REG ),
    .O(N69)
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/gpcs_pma_inst/TRANSMITTER/Mmux_TX_PACKET_CODE_GRP_CNT[1]_MUX_186_o11_G  (
    .I0(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [1]),
    .I1(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(N70)
  );
  INV   \U0/gpcs_pma_inst/TRANSMITTER/Mcount_CODE_GRP_CNT_xor<0>11_INV_0  (
    .I(\U0/gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/gpcs_pma_inst/TRANSMITTER/Result [0])
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/gpcs_pma_inst/Mshreg_STATUS_VECTOR_0  (
    .A0(NlwRenamedSig_OI_status_vector[7]),
    .A1(NlwRenamedSig_OI_status_vector[7]),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_129 ),
    .Q(\U0/gpcs_pma_inst/Mshreg_STATUS_VECTOR_0_526 ),
    .Q15(\NLW_U0/gpcs_pma_inst/Mshreg_STATUS_VECTOR_0_Q15_UNCONNECTED )
  );
  FDE   \U0/gpcs_pma_inst/STATUS_VECTOR_0  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/gpcs_pma_inst/Mshreg_STATUS_VECTOR_0_526 ),
    .Q(\NlwRenamedSignal_U0/gpcs_pma_inst/STATUS_VECTOR_0 )
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_7  (
    .A0(N0),
    .A1(N0),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/gpcs_pma_inst/RXDATA_INT [7]),
    .Q(\U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_7_527 ),
    .Q15(\NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_7_Q15_UNCONNECTED )
  );
  FDE   \U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5_7  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_7_527 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5 [7])
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_6  (
    .A0(N0),
    .A1(N0),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/gpcs_pma_inst/RXDATA_INT [6]),
    .Q(\U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_6_528 ),
    .Q15(\NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_6_Q15_UNCONNECTED )
  );
  FDE   \U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5_6  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_6_528 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5 [6])
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_5  (
    .A0(N0),
    .A1(N0),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/gpcs_pma_inst/RXDATA_INT [5]),
    .Q(\U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_5_529 ),
    .Q15(\NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_5_Q15_UNCONNECTED )
  );
  FDE   \U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5_5  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_5_529 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5 [5])
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_2  (
    .A0(N0),
    .A1(N0),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/gpcs_pma_inst/RXDATA_INT [2]),
    .Q(\U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_2_530 ),
    .Q15(\NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_2_Q15_UNCONNECTED )
  );
  FDE   \U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5_2  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_2_530 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5 [2])
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_4  (
    .A0(N0),
    .A1(N0),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/gpcs_pma_inst/RXDATA_INT [4]),
    .Q(\U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_4_531 ),
    .Q15(\NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_4_Q15_UNCONNECTED )
  );
  FDE   \U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5_4  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_4_531 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5 [4])
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_3  (
    .A0(N0),
    .A1(N0),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/gpcs_pma_inst/RXDATA_INT [3]),
    .Q(\U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_3_532 ),
    .Q15(\NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_3_Q15_UNCONNECTED )
  );
  FDE   \U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5_3  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_3_532 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5 [3])
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/gpcs_pma_inst/RECEIVER/Mshreg_EXTEND_REG3  (
    .A0(NlwRenamedSig_OI_status_vector[7]),
    .A1(NlwRenamedSig_OI_status_vector[7]),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/EXTEND_REG1_420 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/Mshreg_EXTEND_REG3_533 ),
    .Q15(\NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_EXTEND_REG3_Q15_UNCONNECTED )
  );
  FDE   \U0/gpcs_pma_inst/RECEIVER/EXTEND_REG3  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/gpcs_pma_inst/RECEIVER/Mshreg_EXTEND_REG3_533 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/EXTEND_REG3_419 )
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_1  (
    .A0(N0),
    .A1(N0),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/gpcs_pma_inst/RXDATA_INT [1]),
    .Q(\U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_1_534 ),
    .Q15(\NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_1_Q15_UNCONNECTED )
  );
  FDE   \U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5_1  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_1_534 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5 [1])
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_0  (
    .A0(N0),
    .A1(N0),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/gpcs_pma_inst/RXDATA_INT [0]),
    .Q(\U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_0_535 ),
    .Q15(\NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_0_Q15_UNCONNECTED )
  );
  FDE   \U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5_0  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_0_535 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/RXDATA_REG5 [0])
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/gpcs_pma_inst/RECEIVER/Mshreg_CGBAD_REG2  (
    .A0(N0),
    .A1(NlwRenamedSig_OI_status_vector[7]),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/CGBAD_409 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/CGBAD_REG2 ),
    .Q15(\NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_CGBAD_REG2_Q15_UNCONNECTED )
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/gpcs_pma_inst/RECEIVER/Mshreg_SOP_REG2  (
    .A0(NlwRenamedSig_OI_status_vector[7]),
    .A1(NlwRenamedSig_OI_status_vector[7]),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/SOP_402 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/Mshreg_SOP_REG2_536 ),
    .Q15(\NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_SOP_REG2_Q15_UNCONNECTED )
  );
  FDE   \U0/gpcs_pma_inst/RECEIVER/SOP_REG2  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/gpcs_pma_inst/RECEIVER/Mshreg_SOP_REG2_536 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/SOP_REG2_422 )
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/gpcs_pma_inst/RECEIVER/Mshreg_FALSE_CARRIER_REG2  (
    .A0(N0),
    .A1(NlwRenamedSig_OI_status_vector[7]),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_380 ),
    .Q(\U0/gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG2 ),
    .Q15(\NLW_U0/gpcs_pma_inst/RECEIVER/Mshreg_FALSE_CARRIER_REG2_Q15_UNCONNECTED )
  );

// synthesis translate_on

endmodule

// synthesis translate_off

`ifndef GLBL
`define GLBL

`timescale  1 ps / 1 ps

module glbl ();

    parameter ROC_WIDTH = 100000;
    parameter TOC_WIDTH = 0;

//--------   STARTUP Globals --------------
    wire GSR;
    wire GTS;
    wire GWE;
    wire PRLD;
    tri1 p_up_tmp;
    tri (weak1, strong0) PLL_LOCKG = p_up_tmp;

    wire PROGB_GLBL;
    wire CCLKO_GLBL;

    reg GSR_int;
    reg GTS_int;
    reg PRLD_int;

//--------   JTAG Globals --------------
    wire JTAG_TDO_GLBL;
    wire JTAG_TCK_GLBL;
    wire JTAG_TDI_GLBL;
    wire JTAG_TMS_GLBL;
    wire JTAG_TRST_GLBL;

    reg JTAG_CAPTURE_GLBL;
    reg JTAG_RESET_GLBL;
    reg JTAG_SHIFT_GLBL;
    reg JTAG_UPDATE_GLBL;
    reg JTAG_RUNTEST_GLBL;

    reg JTAG_SEL1_GLBL = 0;
    reg JTAG_SEL2_GLBL = 0 ;
    reg JTAG_SEL3_GLBL = 0;
    reg JTAG_SEL4_GLBL = 0;

    reg JTAG_USER_TDO1_GLBL = 1'bz;
    reg JTAG_USER_TDO2_GLBL = 1'bz;
    reg JTAG_USER_TDO3_GLBL = 1'bz;
    reg JTAG_USER_TDO4_GLBL = 1'bz;

    assign (weak1, weak0) GSR = GSR_int;
    assign (weak1, weak0) GTS = GTS_int;
    assign (weak1, weak0) PRLD = PRLD_int;

    initial begin
	GSR_int = 1'b1;
	PRLD_int = 1'b1;
	#(ROC_WIDTH)
	GSR_int = 1'b0;
	PRLD_int = 1'b0;
    end

    initial begin
	GTS_int = 1'b1;
	#(TOC_WIDTH)
	GTS_int = 1'b0;
    end

endmodule

`endif

// synthesis translate_on
