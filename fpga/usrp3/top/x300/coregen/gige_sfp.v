////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1995-2012 Xilinx, Inc.  All rights reserved.
////////////////////////////////////////////////////////////////////////////////
//   ____  ____
//  /   /\/   /
// /___/  \  /    Vendor: Xilinx
// \   \   \/     Version: P.40xd
//  \   \         Application: netgen
//  /   /         Filename: gige_sfp.v
// /___/   /\     Timestamp: Fri Dec 21 13:53:12 2012
// \   \  /  \ 
//  \___\/\___\
//             
// Command	: -w -sim -ofmt verilog /home/matt/sourcerepo/fpga/usrp3/top/x300/coregen/tmp/_cg/gige_sfp.ngc /home/matt/sourcerepo/fpga/usrp3/top/x300/coregen/tmp/_cg/gige_sfp.v 
// Device	: 7k410tffg900-2
// Input file	: /home/matt/sourcerepo/fpga/usrp3/top/x300/coregen/tmp/_cg/gige_sfp.ngc
// Output file	: /home/matt/sourcerepo/fpga/usrp3/top/x300/coregen/tmp/_cg/gige_sfp.v
// # of Modules	: 1
// Design Name	: gige_sfp
// Xilinx        : /opt/Xilinx/14.3/ISE_DS/ISE/
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

module gige_sfp (
  reset, signal_detect, userclk, userclk2, dcm_locked, rxchariscomma, rxcharisk, rxdisperr, rxnotintable, rxrundisp, txbuferr, gmii_tx_en, gmii_tx_er
, mgt_rx_reset, mgt_tx_reset, powerdown, txchardispmode, txchardispval, txcharisk, enablealign, gmii_rx_dv, gmii_rx_er, gmii_isolate, rxbufstatus, 
rxclkcorcnt, rxdata, gmii_txd, configuration_vector, txdata, gmii_rxd, status_vector
)/* synthesis syn_black_box syn_noprune=1 */;
  input reset;
  input signal_detect;
  input userclk;
  input userclk2;
  input dcm_locked;
  input rxchariscomma;
  input rxcharisk;
  input rxdisperr;
  input rxnotintable;
  input rxrundisp;
  input txbuferr;
  input gmii_tx_en;
  input gmii_tx_er;
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
  input [1 : 0] rxbufstatus;
  input [2 : 0] rxclkcorcnt;
  input [7 : 0] rxdata;
  input [7 : 0] gmii_txd;
  input [4 : 0] configuration_vector;
  output [7 : 0] txdata;
  output [7 : 0] gmii_rxd;
  output [15 : 0] status_vector;
  
  // synthesis translate_off
  
  wire \U0/xst_options.gpcs_pma_inst/SYNC_SIGNAL_DETECT/data_in ;
  wire \U0/xst_options.gpcs_pma_inst/RXNOTINTABLE_REG_51 ;
  wire \U0/xst_options.gpcs_pma_inst/RXDISPERR_REG_52 ;
  wire \NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/RECEIVER/RX_INVALID ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RUDI_I_54 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RUDI_C_55 ;
  wire \NlwRenamedSignal_U0/xst_options.gpcs_pma_inst/STATUS_VECTOR_0 ;
  wire \NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ;
  wire \NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ;
  wire \U0/xst_options.gpcs_pma_inst/TXCHARDISPMODE_60 ;
  wire \U0/xst_options.gpcs_pma_inst/TXCHARDISPVAL_61 ;
  wire \U0/xst_options.gpcs_pma_inst/TXCHARISK_62 ;
  wire \NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN ;
  wire \NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/RECEIVER/RX_DV ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_ER_65 ;
  wire N0;
  wire \U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd1_68 ;
  wire \U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd2_69 ;
  wire \U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd3_70 ;
  wire \U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd4_71 ;
  wire \U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd1-In ;
  wire \U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd2-In ;
  wire \U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd3-In ;
  wire \U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd1_75 ;
  wire \U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd2_76 ;
  wire \U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd3_77 ;
  wire \U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd4_78 ;
  wire \U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd1-In ;
  wire \U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd2-In ;
  wire \U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd3-In ;
  wire \U0/xst_options.gpcs_pma_inst/SYNC_SIGNAL_DETECT/data_sync1 ;
  wire \U0/xst_options.gpcs_pma_inst/SRESET_PIPE_PWR_14_o_MUX_1_o ;
  wire \U0/xst_options.gpcs_pma_inst/RESET_INT_RXBUFSTATUS_INT[1]_OR_116_o ;
  wire \U0/xst_options.gpcs_pma_inst/RESET_INT_TXBUFERR_INT_OR_115_o ;
  wire \U0/xst_options.gpcs_pma_inst/TXCHARDISPVAL_INT_GND_14_o_MUX_194_o ;
  wire \U0/xst_options.gpcs_pma_inst/TXCHARDISPMODE_INT_TXEVEN_MUX_193_o ;
  wire \U0/xst_options.gpcs_pma_inst/TXCHARISK_INT_TXEVEN_MUX_192_o ;
  wire \U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<0> ;
  wire \U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<1> ;
  wire \U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<2> ;
  wire \U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<3> ;
  wire \U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<4> ;
  wire \U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<5> ;
  wire \U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<6> ;
  wire \U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<7> ;
  wire \U0/xst_options.gpcs_pma_inst/RXNOTINTABLE_GND_14_o_MUX_182_o ;
  wire \U0/xst_options.gpcs_pma_inst/RXDISPERR_GND_14_o_MUX_183_o ;
  wire \U0/xst_options.gpcs_pma_inst/RXCHARISK_TXCHARISK_INT_MUX_185_o ;
  wire \U0/xst_options.gpcs_pma_inst/RXCHARISCOMMA_TXCHARISK_INT_MUX_186_o ;
  wire \U0/xst_options.gpcs_pma_inst/RX_RST_SM[3]_GND_14_o_Mux_17_o ;
  wire \U0/xst_options.gpcs_pma_inst/TX_RST_SM[3]_GND_14_o_Mux_13_o ;
  wire \U0/xst_options.gpcs_pma_inst/TXBUFERR_INT_103 ;
  wire \U0/xst_options.gpcs_pma_inst/RXNOTINTABLE_INT_108 ;
  wire \U0/xst_options.gpcs_pma_inst/RXDISPERR_INT_109 ;
  wire \U0/xst_options.gpcs_pma_inst/RXCHARISK_INT_118 ;
  wire \U0/xst_options.gpcs_pma_inst/RXCHARISCOMMA_INT_119 ;
  wire \U0/xst_options.gpcs_pma_inst/SRESET_121 ;
  wire \U0/xst_options.gpcs_pma_inst/SRESET_PIPE_122 ;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_123 ;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/EVEN_124 ;
  wire \U0/xst_options.gpcs_pma_inst/RXNOTINTABLE_SRL ;
  wire \U0/xst_options.gpcs_pma_inst/RXDISPERR_SRL ;
  wire \U0/xst_options.gpcs_pma_inst/RESET_INT_PIPE_127 ;
  wire \U0/xst_options.gpcs_pma_inst/RESET_INT_128 ;
  wire \U0/xst_options.gpcs_pma_inst/SIGNAL_DETECT_REG ;
  wire \U0/xst_options.gpcs_pma_inst/DCM_LOCKED_SOFT_RESET_OR_2_o ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXCHARDISPVAL_131 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXCHARDISPMODE_132 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXCHARISK_133 ;
  wire \U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<0> ;
  wire \U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<1> ;
  wire \U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<2> ;
  wire \U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<3> ;
  wire \U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<4> ;
  wire \U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<5> ;
  wire \U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<6> ;
  wire \U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<7> ;
  wire \U0/xst_options.gpcs_pma_inst/RXCLKCORCNT[2]_GND_14_o_mux_22_OUT<0> ;
  wire \U0/xst_options.gpcs_pma_inst/RXCLKCORCNT[2]_GND_14_o_mux_22_OUT<1> ;
  wire \U0/xst_options.gpcs_pma_inst/RXCLKCORCNT[2]_GND_14_o_mux_22_OUT<2> ;
  wire \U0/xst_options.gpcs_pma_inst/RXBUFSTATUS[1]_GND_14_o_mux_21_OUT<1> ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT511 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mram_CODE_GRP_CNT[1]_GND_21_o_Mux_5_o ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT[1]_TX_CONFIG[15]_wide_mux_4_OUT<7> ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/DISP5 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_TRIGGER_T_OR_14_o ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_EVEN_AND_8_o ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_CODE_GRP_CNT[1]_MUX_73_o ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_GND_21_o_MUX_79_o ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<0> ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<1> ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<2> ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<3> ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<4> ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<5> ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<6> ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<7> ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_EVEN_AND_42_o ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<0> ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<1> ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<2> ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<3> ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<4> ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<5> ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<6> ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<7> ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/DISPARITY_183 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/V_184 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/R_185 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_186 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_187 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/C1_OR_C2_188 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_190 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_191 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TRIGGER_T_192 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/T_193 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TRIGGER_S_194 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_195 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_ER_REG1_208 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_209 ;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In1_0 ;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_219 ;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_220 ;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_221 ;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_222 ;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In2_223 ;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1-In2 ;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2-In2 ;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3-In3 ;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/_n0103_inv ;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_PWR_19_o_equal_19_o ;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_GND_23_o_mux_30_OUT<0> ;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_GND_23_o_mux_30_OUT<1> ;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/CGBAD ;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SIGNAL_DETECT_REG_234 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_111_o1_235 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG2_236 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_D21p5_AND_116_o_norst ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/IDLE_REG[1]_IDLE_REG[2]_OR_114_o ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG[0]_RX_CONFIG_VALID_REG[3]_OR_113_o ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG1_C_REG3_OR_59_o_240 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/I_REG_T_REG2_OR_64_o ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/D0p0_242 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_203_o ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1_ISOLATE_AND_182_o_244 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG3_EXT_ILLEGAL_K_REG2_OR_83_o ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/EOP_EXTEND_OR_65_o ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<0> ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<1> ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<2> ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<3> ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<4> ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<5> ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<6> ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<7> ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RXCHARISK_REG1_K28p5_REG1_AND_167_o ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/S_WAIT_FOR_K_AND_144_o ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/SYNC_STATUS_C_REG1_AND_125_o_257 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/EVEN_RXCHARISK_AND_115_o_258 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_EVEN_AND_127_o ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA[7]_RXNOTINTABLE_AND_211_o ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/K23p7 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_111_o ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/K29p7 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RXFIFO_ERR_RXDISPERR_OR_36_o ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_51_o ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_267 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RECEIVE_268 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_269 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/WAIT_FOR_K_270 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_278 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_K_279 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_DATA_280 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_REG2_281 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_REG1_282 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_283 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_ERR_284 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/ILLEGAL_K_REG2_285 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/ILLEGAL_K_REG1_286 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/ILLEGAL_K_287 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_DATA_ERROR_288 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/EOP_REG1_289 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/EOP_290 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_291 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/FROM_RX_CX_292 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_294 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/SYNC_STATUS_REG_295 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_INT_296 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/CGBAD_REG3_297 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/CGBAD_298 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/R_299 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG3_308 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1_309 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_REG3_310 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_REG2_311 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG2 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/C_HDR_REMOVED_REG_313 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/CGBAD_REG2 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RXCHARISK_REG1_315 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG3_316 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG1_317 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/I_REG_318 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/R_REG1_319 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/T_REG2_320 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/T_REG1_321 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/D0p0_REG_322 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_323 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/C_324 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/I_325 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/T_326 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/S_327 ;
  wire N2;
  wire N6;
  wire N8;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_XMIT_DATA_INT_AND_20_o1_331 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_XMIT_DATA_INT_AND_20_o2_332 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT2 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT1 ;
  wire N14;
  wire N18;
  wire N20;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_38_o ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_38_o1_339 ;
  wire N22;
  wire N24;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_108_o1 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_108_o12_343 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_108_o13_344 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_108_o14_345 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/I_REG_T_REG2_OR_64_o1_346 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/T_REG2_R_REG1_OR_79_o1_347 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/T_REG2_R_REG1_OR_79_o2_348 ;
  wire N26;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/V_glue_set_350 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_glue_set_351 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/R_glue_set_352 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/DISPARITY_glue_rst_353 ;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/EVEN_glue_set_354 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RECEIVE_glue_set_355 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_INVALID_glue_set_356 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_DV_glue_set_357 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_glue_set_358 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_glue_set_359 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/WAIT_FOR_K_glue_set_360 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/C1_OR_C2_rstpot_361 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_rstpot_362 ;
  wire \U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd4_rstpot_363 ;
  wire \U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd4_rstpot_364 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_rstpot_365 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXCHARDISPVAL_rstpot_366 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TRIGGER_T_rstpot_367 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_rstpot_368 ;
  wire \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA_0_rstpot_369 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/C_HDR_REMOVED_REG_rstpot_370 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/C_rstpot_371 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_rstpot_372 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_DATA_ERROR_rstpot_373 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_rstpot_374 ;
  wire N28;
  wire N34;
  wire N38;
  wire N39;
  wire N43;
  wire N45;
  wire N57;
  wire N59;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN_rstpot_383 ;
  wire \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_rstpot_384 ;
  wire N71;
  wire N73;
  wire N75;
  wire N77;
  wire N79;
  wire N81;
  wire N83;
  wire N84;
  wire N85;
  wire N86;
  wire N87;
  wire N88;
  wire N89;
  wire N90;
  wire N91;
  wire N92;
  wire N93;
  wire N94;
  wire N95;
  wire N96;
  wire \U0/xst_options.gpcs_pma_inst/Mshreg_STATUS_VECTOR_0_405 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_7_406 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_6_407 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_5_408 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_2_409 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_4_410 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_3_411 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_EXTEND_REG3_412 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_1_413 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_0_414 ;
  wire \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_SOP_REG2_415 ;
  wire \NLW_U0/xst_options.gpcs_pma_inst/Mshreg_STATUS_VECTOR_0_Q15_UNCONNECTED ;
  wire \NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_7_Q15_UNCONNECTED ;
  wire \NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_6_Q15_UNCONNECTED ;
  wire \NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_5_Q15_UNCONNECTED ;
  wire \NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_2_Q15_UNCONNECTED ;
  wire \NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_4_Q15_UNCONNECTED ;
  wire \NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_3_Q15_UNCONNECTED ;
  wire \NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_EXTEND_REG3_Q15_UNCONNECTED ;
  wire \NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_1_Q15_UNCONNECTED ;
  wire \NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_0_Q15_UNCONNECTED ;
  wire \NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_CGBAD_REG2_Q15_UNCONNECTED ;
  wire \NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_SOP_REG2_Q15_UNCONNECTED ;
  wire \NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_FALSE_CARRIER_REG2_Q15_UNCONNECTED ;
  wire [7 : 0] \U0/xst_options.gpcs_pma_inst/TXDATA ;
  wire [7 : 0] \U0/xst_options.gpcs_pma_inst/RECEIVER/RXD ;
  wire [3 : 2] \NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG ;
  wire [2 : 0] \U0/xst_options.gpcs_pma_inst/RXCLKCORCNT_INT ;
  wire [1 : 1] \U0/xst_options.gpcs_pma_inst/RXBUFSTATUS_INT ;
  wire [7 : 0] \U0/xst_options.gpcs_pma_inst/RXDATA_INT ;
  wire [1 : 1] \U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG ;
  wire [1 : 0] \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT ;
  wire [7 : 0] \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA ;
  wire [1 : 0] \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Result ;
  wire [1 : 1] \U0/xst_options.gpcs_pma_inst/TRANSMITTER/_n0234 ;
  wire [3 : 0] \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA ;
  wire [7 : 0] \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP ;
  wire [7 : 0] \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1 ;
  wire [1 : 0] \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS ;
  wire [2 : 0] \U0/xst_options.gpcs_pma_inst/RECEIVER/IDLE_REG ;
  wire [3 : 0] \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG ;
  wire [7 : 7] NlwRenamedSig_OI_status_vector;
  wire [7 : 0] \U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5 ;
  assign
    \U0/xst_options.gpcs_pma_inst/SYNC_SIGNAL_DETECT/data_in  = signal_detect,
    txdata[7] = \U0/xst_options.gpcs_pma_inst/TXDATA [7],
    txdata[6] = \U0/xst_options.gpcs_pma_inst/TXDATA [6],
    txdata[5] = \U0/xst_options.gpcs_pma_inst/TXDATA [5],
    txdata[4] = \U0/xst_options.gpcs_pma_inst/TXDATA [4],
    txdata[3] = \U0/xst_options.gpcs_pma_inst/TXDATA [3],
    txdata[2] = \U0/xst_options.gpcs_pma_inst/TXDATA [2],
    txdata[1] = \U0/xst_options.gpcs_pma_inst/TXDATA [1],
    txdata[0] = \U0/xst_options.gpcs_pma_inst/TXDATA [0],
    gmii_rxd[7] = \U0/xst_options.gpcs_pma_inst/RECEIVER/RXD [7],
    gmii_rxd[6] = \U0/xst_options.gpcs_pma_inst/RECEIVER/RXD [6],
    gmii_rxd[5] = \U0/xst_options.gpcs_pma_inst/RECEIVER/RXD [5],
    gmii_rxd[4] = \U0/xst_options.gpcs_pma_inst/RECEIVER/RXD [4],
    gmii_rxd[3] = \U0/xst_options.gpcs_pma_inst/RECEIVER/RXD [3],
    gmii_rxd[2] = \U0/xst_options.gpcs_pma_inst/RECEIVER/RXD [2],
    gmii_rxd[1] = \U0/xst_options.gpcs_pma_inst/RECEIVER/RXD [1],
    gmii_rxd[0] = \U0/xst_options.gpcs_pma_inst/RECEIVER/RXD [0],
    status_vector[15] = NlwRenamedSig_OI_status_vector[7],
    status_vector[14] = NlwRenamedSig_OI_status_vector[7],
    status_vector[13] = NlwRenamedSig_OI_status_vector[7],
    status_vector[12] = NlwRenamedSig_OI_status_vector[7],
    status_vector[11] = NlwRenamedSig_OI_status_vector[7],
    status_vector[10] = NlwRenamedSig_OI_status_vector[7],
    status_vector[9] = NlwRenamedSig_OI_status_vector[7],
    status_vector[8] = NlwRenamedSig_OI_status_vector[7],
    status_vector[7] = NlwRenamedSig_OI_status_vector[7],
    status_vector[6] = \U0/xst_options.gpcs_pma_inst/RXNOTINTABLE_REG_51 ,
    status_vector[5] = \U0/xst_options.gpcs_pma_inst/RXDISPERR_REG_52 ,
    status_vector[4] = \NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/RECEIVER/RX_INVALID ,
    status_vector[3] = \U0/xst_options.gpcs_pma_inst/RECEIVER/RUDI_I_54 ,
    status_vector[2] = \U0/xst_options.gpcs_pma_inst/RECEIVER/RUDI_C_55 ,
    status_vector[1] = \NlwRenamedSignal_U0/xst_options.gpcs_pma_inst/STATUS_VECTOR_0 ,
    status_vector[0] = \NlwRenamedSignal_U0/xst_options.gpcs_pma_inst/STATUS_VECTOR_0 ,
    mgt_rx_reset = \NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ,
    mgt_tx_reset = \NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ,
    powerdown = \NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [2],
    txchardispmode = \U0/xst_options.gpcs_pma_inst/TXCHARDISPMODE_60 ,
    txchardispval = \U0/xst_options.gpcs_pma_inst/TXCHARDISPVAL_61 ,
    txcharisk = \U0/xst_options.gpcs_pma_inst/TXCHARISK_62 ,
    enablealign = \NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN ,
    gmii_rx_dv = \NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/RECEIVER/RX_DV ,
    gmii_rx_er = \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_ER_65 ,
    gmii_isolate = \NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3];
  VCC   XST_VCC (
    .P(N0)
  );
  GND   XST_GND (
    .G(NlwRenamedSig_OI_status_vector[7])
  );
  SRL16 #(
    .INIT ( 16'h0000 ))
  \U0/xst_options.gpcs_pma_inst/DELAY_RXNOTINTABLE  (
    .A0(NlwRenamedSig_OI_status_vector[7]),
    .A1(NlwRenamedSig_OI_status_vector[7]),
    .A2(N0),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CLK(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXNOTINTABLE_INT_108 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXNOTINTABLE_SRL )
  );
  SRL16 #(
    .INIT ( 16'h0000 ))
  \U0/xst_options.gpcs_pma_inst/DELAY_RXDISPERR  (
    .A0(NlwRenamedSig_OI_status_vector[7]),
    .A1(NlwRenamedSig_OI_status_vector[7]),
    .A2(N0),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CLK(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXDISPERR_INT_109 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXDISPERR_SRL )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd2  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd2-In ),
    .R(\U0/xst_options.gpcs_pma_inst/RESET_INT_RXBUFSTATUS_INT[1]_OR_116_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd2_69 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd3  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd3-In ),
    .R(\U0/xst_options.gpcs_pma_inst/RESET_INT_RXBUFSTATUS_INT[1]_OR_116_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd3_70 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd1-In ),
    .R(\U0/xst_options.gpcs_pma_inst/RESET_INT_RXBUFSTATUS_INT[1]_OR_116_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd1_68 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd1-In ),
    .R(\U0/xst_options.gpcs_pma_inst/RESET_INT_TXBUFERR_INT_OR_115_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd1_75 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd2  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd2-In ),
    .R(\U0/xst_options.gpcs_pma_inst/RESET_INT_TXBUFERR_INT_OR_115_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd2_76 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd3  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd3-In ),
    .R(\U0/xst_options.gpcs_pma_inst/RESET_INT_TXBUFERR_INT_OR_115_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd3_77 )
  );
  FD #(
    .INIT ( 1'b0 ))
  \U0/xst_options.gpcs_pma_inst/SYNC_SIGNAL_DETECT/data_sync  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/SYNC_SIGNAL_DETECT/data_in ),
    .Q(\U0/xst_options.gpcs_pma_inst/SYNC_SIGNAL_DETECT/data_sync1 )
  );
  FD #(
    .INIT ( 1'b0 ))
  \U0/xst_options.gpcs_pma_inst/SYNC_SIGNAL_DETECT/data_sync_reg  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/SYNC_SIGNAL_DETECT/data_sync1 ),
    .Q(\U0/xst_options.gpcs_pma_inst/SIGNAL_DETECT_REG )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RXNOTINTABLE_REG  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXNOTINTABLE_SRL ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXNOTINTABLE_REG_51 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RXDISPERR_REG  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXDISPERR_SRL ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXDISPERR_REG_52 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TXCHARDISPVAL  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TXCHARDISPVAL_INT_GND_14_o_MUX_194_o ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TXCHARDISPVAL_61 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TXCHARISK  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TXCHARISK_INT_TXEVEN_MUX_192_o ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TXCHARISK_62 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TXDATA_7  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<7> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TXDATA [7])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TXDATA_6  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<6> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TXDATA [6])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TXDATA_5  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<5> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TXDATA [5])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TXDATA_4  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<4> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TXDATA [4])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TXDATA_3  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<3> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TXDATA [3])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TXDATA_2  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<2> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TXDATA [2])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TXDATA_1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<1> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TXDATA [1])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TXDATA_0  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<0> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TXDATA [0])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TXCHARDISPMODE  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TXCHARDISPMODE_INT_TXEVEN_MUX_193_o ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TXCHARDISPMODE_60 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RXCHARISK_INT  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXCHARISK_TXCHARISK_INT_MUX_185_o ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXCHARISK_INT_118 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RXCHARISCOMMA_INT  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXCHARISCOMMA_TXCHARISK_INT_MUX_186_o ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXCHARISCOMMA_INT_119 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RXDATA_INT_7  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<7> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [7])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RXDATA_INT_6  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<6> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [6])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RXDATA_INT_5  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<5> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [5])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RXDATA_INT_4  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<4> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [4])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RXDATA_INT_3  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<3> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [3])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RXDATA_INT_2  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<2> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [2])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RXDATA_INT_1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<1> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [1])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RXDATA_INT_0  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<0> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [0])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RXBUFSTATUS_INT_1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXBUFSTATUS[1]_GND_14_o_mux_21_OUT<1> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXBUFSTATUS_INT [1])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RXCLKCORCNT_INT_2  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXCLKCORCNT[2]_GND_14_o_mux_22_OUT<2> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXCLKCORCNT_INT [2])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RXCLKCORCNT_INT_1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXCLKCORCNT[2]_GND_14_o_mux_22_OUT<1> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXCLKCORCNT_INT [1])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RXCLKCORCNT_INT_0  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXCLKCORCNT[2]_GND_14_o_mux_22_OUT<0> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXCLKCORCNT_INT [0])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RXNOTINTABLE_INT  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXNOTINTABLE_GND_14_o_MUX_182_o ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXNOTINTABLE_INT_108 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RXDISPERR_INT  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXDISPERR_GND_14_o_MUX_183_o ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RXDISPERR_INT_109 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG_3  (
    .C(userclk2),
    .D(configuration_vector[3]),
    .R(\U0/xst_options.gpcs_pma_inst/SRESET_121 ),
    .Q(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG_2  (
    .C(userclk2),
    .D(configuration_vector[2]),
    .R(\U0/xst_options.gpcs_pma_inst/SRESET_121 ),
    .Q(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [2])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG_1  (
    .C(userclk2),
    .D(configuration_vector[1]),
    .R(\U0/xst_options.gpcs_pma_inst/SRESET_121 ),
    .Q(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1])
  );
  FD   \U0/xst_options.gpcs_pma_inst/SRESET  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/SRESET_PIPE_PWR_14_o_MUX_1_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/SRESET_121 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TXBUFERR_INT  (
    .C(userclk2),
    .D(txbuferr),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TXBUFERR_INT_103 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/SRESET_PIPE  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RESET_INT_128 ),
    .Q(\U0/xst_options.gpcs_pma_inst/SRESET_PIPE_122 )
  );
  FDS   \U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RX_RST_SM[3]_GND_14_o_Mux_17_o ),
    .S(\U0/xst_options.gpcs_pma_inst/RESET_INT_RXBUFSTATUS_INT[1]_OR_116_o ),
    .Q(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT )
  );
  FDS   \U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TX_RST_SM[3]_GND_14_o_Mux_13_o ),
    .S(\U0/xst_options.gpcs_pma_inst/RESET_INT_TXBUFERR_INT_OR_115_o ),
    .Q(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT )
  );
  FDP   \U0/xst_options.gpcs_pma_inst/RESET_INT  (
    .C(userclk),
    .D(\U0/xst_options.gpcs_pma_inst/RESET_INT_PIPE_127 ),
    .PRE(\U0/xst_options.gpcs_pma_inst/DCM_LOCKED_SOFT_RESET_OR_2_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/RESET_INT_128 )
  );
  FDP   \U0/xst_options.gpcs_pma_inst/RESET_INT_PIPE  (
    .C(userclk),
    .D(NlwRenamedSig_OI_status_vector[7]),
    .PRE(\U0/xst_options.gpcs_pma_inst/DCM_LOCKED_SOFT_RESET_OR_2_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/RESET_INT_PIPE_127 )
  );
  FDS   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT_1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/Result [1]),
    .S(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [1])
  );
  FDS   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT_0  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/Result [0]),
    .S(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA_7  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<7> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [7])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA_6  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<6> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [6])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA_5  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<5> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [5])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA_4  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<4> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [4])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA_3  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<3> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [3])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA_2  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<2> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [2])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA_1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<1> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [1])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA_0  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<0> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [0])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXCHARISK  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_GND_21_o_MUX_79_o ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXCHARISK_133 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_7  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<7> ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [7])
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_6  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<6> ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [6])
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_5  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<5> ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [5])
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_4  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<4> ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [4])
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_3  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<3> ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [3])
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_2  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<2> ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [2])
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<1> ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [1])
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_0  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<0> ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [0])
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_CODE_GRP_CNT[1]_MUX_73_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_191 )
  );
  FDS   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXCHARDISPMODE  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_EVEN_AND_42_o ),
    .S(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXCHARDISPMODE_132 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TRIGGER_S  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_EVEN_AND_8_o ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TRIGGER_S_194 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/T  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_TRIGGER_T_OR_14_o ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/T_193 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA_3  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mram_CODE_GRP_CNT[1]_GND_21_o_Mux_5_o ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [3])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA_2  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT[1]_TX_CONFIG[15]_wide_mux_4_OUT<7> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [2])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA_1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/_n0234 [1]),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [1])
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_ER_REG1  (
    .C(userclk2),
    .D(gmii_tx_er),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_ER_REG1_208 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_REG1  (
    .C(userclk2),
    .D(gmii_tx_en),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_209 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1_7  (
    .C(userclk2),
    .D(gmii_txd[7]),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1 [7])
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1_6  (
    .C(userclk2),
    .D(gmii_txd[6]),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1 [6])
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1_5  (
    .C(userclk2),
    .D(gmii_txd[5]),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1 [5])
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1_4  (
    .C(userclk2),
    .D(gmii_txd[4]),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1 [4])
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1_3  (
    .C(userclk2),
    .D(gmii_txd[3]),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1 [3])
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1_2  (
    .C(userclk2),
    .D(gmii_txd[2]),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1 [2])
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1_1  (
    .C(userclk2),
    .D(gmii_txd[1]),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1 [1])
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1_0  (
    .C(userclk2),
    .D(gmii_txd[0]),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1 [0])
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1-In2 ),
    .R(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In1_0 ),
    .Q(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_219 )
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2-In2 ),
    .R(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In1_0 ),
    .Q(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_220 )
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In2_223 ),
    .R(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In1_0 ),
    .Q(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_222 )
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3-In3 ),
    .R(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In1_0 ),
    .Q(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_221 )
  );
  FDRE   \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS_1  (
    .C(userclk2),
    .CE(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/_n0103_inv ),
    .D(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_GND_23_o_mux_30_OUT<1> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS [1])
  );
  FDRE   \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS_0  (
    .C(userclk2),
    .CE(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/_n0103_inv ),
    .D(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_GND_23_o_mux_30_OUT<0> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS [0])
  );
  FD   \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SIGNAL_DETECT_REG  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/SIGNAL_DETECT_REG ),
    .Q(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SIGNAL_DETECT_REG_234 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/RXD_7  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<7> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3]),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXD [7])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/RXD_6  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<6> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3]),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXD [6])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/RXD_5  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<5> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3]),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXD [5])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/RXD_4  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<4> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3]),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXD [4])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/RXD_3  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<3> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3]),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXD [3])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/RXD_2  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<2> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3]),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXD [2])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/RXD_1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<1> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3]),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXD [1])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/RXD_0  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<0> ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3]),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXD [0])
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG3  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG2_236 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG3_316 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG2 ),
    .R(\U0/xst_options.gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_51_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_294 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/CGBAD_REG3  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/CGBAD_REG2 ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/CGBAD_REG3_297 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_REG3  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_REG2_311 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_REG3_310 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG2  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG1_317 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG2_236 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/IDLE_REG_2  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/IDLE_REG [1]),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/IDLE_REG [2])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/IDLE_REG_1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/IDLE_REG [0]),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/IDLE_REG [1])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/IDLE_REG_0  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/I_REG_318 ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/IDLE_REG [0])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_REG2  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_REG1_282 ),
    .R(\U0/xst_options.gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_51_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_REG2_281 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/ILLEGAL_K_REG2  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/ILLEGAL_K_REG1_286 ),
    .R(\U0/xst_options.gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_51_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/ILLEGAL_K_REG2_285 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_324 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG1_317 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/T_REG2  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/T_REG1_321 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/T_REG2_320 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG_3  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [2]),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [3])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG_2  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [1]),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [2])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG_1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [0]),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [1])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG_0  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_INT_296 ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [0])
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_REG1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_283 ),
    .R(\U0/xst_options.gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_51_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_REG1_282 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/ILLEGAL_K_REG1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/ILLEGAL_K_287 ),
    .R(\U0/xst_options.gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_51_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/ILLEGAL_K_REG1_286 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_267 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1_309 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/I_REG  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/I_325 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/I_REG_318 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/R_REG1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/R_299 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/R_REG1_319 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/T_REG1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/T_326 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/T_REG1_321 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/RUDI_I  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/IDLE_REG[1]_IDLE_REG[2]_OR_114_o ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RUDI_I_54 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/RUDI_C  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG[0]_RX_CONFIG_VALID_REG[3]_OR_113_o ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RUDI_C_55 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_K  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA[7]_RXNOTINTABLE_AND_211_o ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_K_279 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_DATA  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_203_o ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_DATA_280 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_ER  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1_ISOLATE_AND_182_o_244 ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_ER_65 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_ERR  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG3_EXT_ILLEGAL_K_REG2_OR_83_o ),
    .R(\U0/xst_options.gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_51_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_ERR_284 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/ILLEGAL_K  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXCHARISK_REG1_K28p5_REG1_AND_167_o ),
    .R(\U0/xst_options.gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_51_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/ILLEGAL_K_287 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/EOP  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/I_REG_T_REG2_OR_64_o ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/EOP_290 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/SOP  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/S_WAIT_FOR_K_AND_144_o ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_291 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/EOP_REG1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/EOP_EXTEND_OR_65_o ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/EOP_REG1_289 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/FROM_RX_CX  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG1_C_REG3_OR_59_o_240 ),
    .R(\U0/xst_options.gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_51_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/FROM_RX_CX_292 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/SYNC_STATUS_REG  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_123 ),
    .R(\U0/xst_options.gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_51_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/SYNC_STATUS_REG_295 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_INT  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/SYNC_STATUS_C_REG1_AND_125_o_257 ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_INT_296 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/R  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/K23p7 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/R_299 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/CGBAD  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXFIFO_ERR_RXDISPERR_OR_36_o ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/CGBAD_298 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/RXCHARISK_REG1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXCHARISK_INT_118 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXCHARISK_REG1_315 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/D0p0_REG  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/D0p0_242 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/D0p0_REG_322 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_323 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/I  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/EVEN_RXCHARISK_AND_115_o_258 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/I_325 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/S  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_111_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/S_327 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/T  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/K29p7 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/T_326 )
  );
  LUT4 #(
    .INIT ( 16'hEA6A ))
  \U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd2-In1  (
    .I0(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd2_76 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd4_78 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd3_77 ),
    .I3(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd1_75 ),
    .O(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd2-In )
  );
  LUT4 #(
    .INIT ( 16'hEA6A ))
  \U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd2-In1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd2_69 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd4_71 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd3_70 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd1_68 ),
    .O(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd2-In )
  );
  LUT4 #(
    .INIT ( 16'hE666 ))
  \U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd3-In1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd3_70 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd4_71 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd1_68 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd2_69 ),
    .O(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd3-In )
  );
  LUT4 #(
    .INIT ( 16'hE666 ))
  \U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd3-In1  (
    .I0(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd3_77 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd4_78 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd1_75 ),
    .I3(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd2_76 ),
    .O(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd3-In )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_TXCHARDISPVAL_INT_GND_14_o_MUX_194_o11  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXCHARDISPVAL_131 ),
    .O(\U0/xst_options.gpcs_pma_inst/TXCHARDISPVAL_INT_GND_14_o_MUX_194_o )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_TXCHARDISPMODE_INT_TXEVEN_MUX_193_o11  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXCHARDISPMODE_132 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/xst_options.gpcs_pma_inst/TXCHARDISPMODE_INT_TXEVEN_MUX_193_o )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_TXCHARISK_INT_TXEVEN_MUX_192_o11  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXCHARISK_133 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/xst_options.gpcs_pma_inst/TXCHARISK_INT_TXEVEN_MUX_192_o )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_TXDATA_INT[7]_GND_14_o_mux_30_OUT11  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [0]),
    .O(\U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<0> )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_TXDATA_INT[7]_GND_14_o_mux_30_OUT21  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [1]),
    .O(\U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<1> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_TXDATA_INT[7]_GND_14_o_mux_30_OUT31  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [2]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<2> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_TXDATA_INT[7]_GND_14_o_mux_30_OUT41  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [3]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<3> )
  );
  LUT2 #(
    .INIT ( 4'hE ))
  \U0/xst_options.gpcs_pma_inst/Mmux_TXDATA_INT[7]_GND_14_o_mux_30_OUT51  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [4]),
    .O(\U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<4> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_TXDATA_INT[7]_GND_14_o_mux_30_OUT61  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [5]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<5> )
  );
  LUT3 #(
    .INIT ( 8'h4E ))
  \U0/xst_options.gpcs_pma_inst/Mmux_TXDATA_INT[7]_GND_14_o_mux_30_OUT71  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [6]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<6> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_TXDATA_INT[7]_GND_14_o_mux_30_OUT81  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [7]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/xst_options.gpcs_pma_inst/TXDATA_INT[7]_GND_14_o_mux_30_OUT<7> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_RXCHARISK_TXCHARISK_INT_MUX_185_o11  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(rxcharisk),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXCHARISK_133 ),
    .O(\U0/xst_options.gpcs_pma_inst/RXCHARISK_TXCHARISK_INT_MUX_185_o )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_RXCHARISCOMMA_TXCHARISK_INT_MUX_186_o11  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(rxchariscomma),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXCHARISK_133 ),
    .O(\U0/xst_options.gpcs_pma_inst/RXCHARISCOMMA_TXCHARISK_INT_MUX_186_o )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_RXDATA[7]_TXDATA_INT[7]_mux_20_OUT11  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(rxdata[0]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [0]),
    .O(\U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<0> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_RXDATA[7]_TXDATA_INT[7]_mux_20_OUT21  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(rxdata[1]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [1]),
    .O(\U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<1> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_RXDATA[7]_TXDATA_INT[7]_mux_20_OUT31  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(rxdata[2]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [2]),
    .O(\U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<2> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_RXDATA[7]_TXDATA_INT[7]_mux_20_OUT41  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(rxdata[3]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [3]),
    .O(\U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<3> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_RXDATA[7]_TXDATA_INT[7]_mux_20_OUT51  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(rxdata[4]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [4]),
    .O(\U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<4> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_RXDATA[7]_TXDATA_INT[7]_mux_20_OUT61  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(rxdata[5]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [5]),
    .O(\U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<5> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_RXDATA[7]_TXDATA_INT[7]_mux_20_OUT71  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(rxdata[6]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [6]),
    .O(\U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<6> )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_RXDATA[7]_TXDATA_INT[7]_mux_20_OUT81  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(rxdata[7]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXDATA [7]),
    .O(\U0/xst_options.gpcs_pma_inst/RXDATA[7]_TXDATA_INT[7]_mux_20_OUT<7> )
  );
  LUT2 #(
    .INIT ( 4'hE ))
  \U0/xst_options.gpcs_pma_inst/Mmux_SRESET_PIPE_PWR_14_o_MUX_1_o11  (
    .I0(\U0/xst_options.gpcs_pma_inst/RESET_INT_128 ),
    .I1(\U0/xst_options.gpcs_pma_inst/SRESET_PIPE_122 ),
    .O(\U0/xst_options.gpcs_pma_inst/SRESET_PIPE_PWR_14_o_MUX_1_o )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_RXDISPERR_GND_14_o_MUX_183_o11  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(rxdisperr),
    .O(\U0/xst_options.gpcs_pma_inst/RXDISPERR_GND_14_o_MUX_183_o )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_RXNOTINTABLE_GND_14_o_MUX_182_o11  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(rxnotintable),
    .O(\U0/xst_options.gpcs_pma_inst/RXNOTINTABLE_GND_14_o_MUX_182_o )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_RXCLKCORCNT[2]_GND_14_o_mux_22_OUT11  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(rxclkcorcnt[0]),
    .O(\U0/xst_options.gpcs_pma_inst/RXCLKCORCNT[2]_GND_14_o_mux_22_OUT<0> )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_RXCLKCORCNT[2]_GND_14_o_mux_22_OUT21  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(rxclkcorcnt[1]),
    .O(\U0/xst_options.gpcs_pma_inst/RXCLKCORCNT[2]_GND_14_o_mux_22_OUT<1> )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_RXCLKCORCNT[2]_GND_14_o_mux_22_OUT31  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(rxclkcorcnt[2]),
    .O(\U0/xst_options.gpcs_pma_inst/RXCLKCORCNT[2]_GND_14_o_mux_22_OUT<2> )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/xst_options.gpcs_pma_inst/Mmux_RXBUFSTATUS[1]_GND_14_o_mux_21_OUT21  (
    .I0(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I1(rxbufstatus[1]),
    .O(\U0/xst_options.gpcs_pma_inst/RXBUFSTATUS[1]_GND_14_o_mux_21_OUT<1> )
  );
  LUT4 #(
    .INIT ( 16'hFF80 ))
  \U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd1-In1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd4_71 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd3_70 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd2_69 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd1_68 ),
    .O(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd1-In )
  );
  LUT4 #(
    .INIT ( 16'hDFFF ))
  \U0/xst_options.gpcs_pma_inst/RX_RST_SM_RX_RST_SM[3]_GND_14_o_Mux_17_o1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd3_70 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd4_71 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd1_68 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd2_69 ),
    .O(\U0/xst_options.gpcs_pma_inst/RX_RST_SM[3]_GND_14_o_Mux_17_o )
  );
  LUT4 #(
    .INIT ( 16'hFF80 ))
  \U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd1-In1  (
    .I0(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd4_78 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd3_77 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd2_76 ),
    .I3(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd1_75 ),
    .O(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd1-In )
  );
  LUT4 #(
    .INIT ( 16'hDFFF ))
  \U0/xst_options.gpcs_pma_inst/TX_RST_SM_TX_RST_SM[3]_GND_14_o_Mux_13_o1  (
    .I0(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd3_77 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd4_78 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd1_75 ),
    .I3(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd2_76 ),
    .O(\U0/xst_options.gpcs_pma_inst/TX_RST_SM[3]_GND_14_o_Mux_13_o )
  );
  LUT2 #(
    .INIT ( 4'hE ))
  \U0/xst_options.gpcs_pma_inst/RESET_INT_RXBUFSTATUS_INT[1]_OR_116_o1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RESET_INT_128 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RXBUFSTATUS_INT [1]),
    .O(\U0/xst_options.gpcs_pma_inst/RESET_INT_RXBUFSTATUS_INT[1]_OR_116_o )
  );
  LUT2 #(
    .INIT ( 4'hE ))
  \U0/xst_options.gpcs_pma_inst/RESET_INT_TXBUFERR_INT_OR_115_o1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RESET_INT_128 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TXBUFERR_INT_103 ),
    .O(\U0/xst_options.gpcs_pma_inst/RESET_INT_TXBUFERR_INT_OR_115_o )
  );
  LUT2 #(
    .INIT ( 4'hB ))
  \U0/xst_options.gpcs_pma_inst/DCM_LOCKED_SOFT_RESET_OR_2_o1  (
    .I0(reset),
    .I1(dcm_locked),
    .O(\U0/xst_options.gpcs_pma_inst/DCM_LOCKED_SOFT_RESET_OR_2_o )
  );
  LUT4 #(
    .INIT ( 16'hCFCA ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT51  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1 [4]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [2]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_187 ),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT511 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<4> )
  );
  LUT4 #(
    .INIT ( 16'hCFCA ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT61  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1 [5]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [2]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_187 ),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT511 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<5> )
  );
  LUT4 #(
    .INIT ( 16'hCFCA ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT81  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1 [7]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [2]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_187 ),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT511 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<7> )
  );
  LUT6 #(
    .INIT ( 64'hFFFFFFFFFFFFFEFF ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT5111  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/R_185 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/V_184 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/T_193 ),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_186 ),
    .I4(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_195 ),
    .I5(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3]),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT511 )
  );
  LUT5 #(
    .INIT ( 32'hE881811F ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/DISP51  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [3]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [4]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [1]),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [2]),
    .I4(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [0]),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/DISP5 )
  );
  LUT3 #(
    .INIT ( 8'h15 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT[1]_TX_CONFIG[15]_wide_mux_4_OUT<7>1  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/C1_OR_C2_188 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT[1]_TX_CONFIG[15]_wide_mux_4_OUT<7> )
  );
  LUT6 #(
    .INIT ( 64'hFFFF444044404440 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_TRIGGER_T_OR_14_o1  (
    .I0(gmii_tx_en),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_209 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_195 ),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_186 ),
    .I4(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TRIGGER_T_192 ),
    .I5(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/V_184 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_TRIGGER_T_OR_14_o )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mcount_CODE_GRP_CNT_xor<1>11  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/Result [1])
  );
  LUT2 #(
    .INIT ( 4'h1 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mram_CODE_GRP_CNT[1]_GND_21_o_Mux_5_o1  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mram_CODE_GRP_CNT[1]_GND_21_o_Mux_5_o )
  );
  LUT4 #(
    .INIT ( 16'h0040 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_EVEN_AND_8_o1  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_ER_REG1_208 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I2(gmii_tx_en),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_209 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_EVEN_AND_8_o )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_EVEN_AND_42_o1  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_191 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_EVEN_AND_42_o )
  );
  LUT6 #(
    .INIT ( 64'h0000577757770000 ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/Mmux_GOOD_CGS[1]_GND_23_o_mux_30_OUT21  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_222 ),
    .I1(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_219 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_220 ),
    .I3(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_221 ),
    .I4(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS [0]),
    .I5(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS [1]),
    .O(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_GND_23_o_mux_30_OUT<1> )
  );
  LUT5 #(
    .INIT ( 32'hA888FFFF ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/_n0103_inv1  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_222 ),
    .I1(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_219 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_221 ),
    .I3(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_220 ),
    .I4(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/CGBAD ),
    .O(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/_n0103_inv )
  );
  LUT5 #(
    .INIT ( 32'h01115555 ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/Mmux_GOOD_CGS[1]_GND_23_o_mux_30_OUT11  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS [0]),
    .I1(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_219 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_221 ),
    .I3(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_220 ),
    .I4(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_222 ),
    .O(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_GND_23_o_mux_30_OUT<0> )
  );
  LUT6 #(
    .INIT ( 64'hD8B0D8B0E8E0F8F0 ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1-In21  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_222 ),
    .I1(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_220 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_219 ),
    .I3(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_221 ),
    .I4(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_PWR_19_o_equal_19_o ),
    .I5(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/CGBAD ),
    .O(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1-In2 )
  );
  LUT3 #(
    .INIT ( 8'hF1 ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In1_01  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SIGNAL_DETECT_REG_234 ),
    .I1(\U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [1]),
    .I2(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .O(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In1_0 )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_PWR_19_o_equal_19_o<1>1  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS [0]),
    .I1(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS [1]),
    .O(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_PWR_19_o_equal_19_o )
  );
  LUT5 #(
    .INIT ( 32'hFFFEFEFE ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/CGBAD1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXBUFSTATUS_INT [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXNOTINTABLE_INT_108 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDISPERR_INT_109 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RXCHARISCOMMA_INT_119 ),
    .I4(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/EVEN_124 ),
    .O(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/CGBAD )
  );
  LUT4 #(
    .INIT ( 16'h5554 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mmux_RXDATA_REG5[7]_GND_24_o_mux_9_OUT21  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_REG3_310 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1_309 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_294 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5 [1]),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<1> )
  );
  LUT4 #(
    .INIT ( 16'h5554 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mmux_RXDATA_REG5[7]_GND_24_o_mux_9_OUT41  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_REG3_310 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1_309 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_294 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5 [3]),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<3> )
  );
  LUT4 #(
    .INIT ( 16'hFFFE ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mmux_RXDATA_REG5[7]_GND_24_o_mux_9_OUT31  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5 [2]),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1_309 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_294 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_REG3_310 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<2> )
  );
  LUT4 #(
    .INIT ( 16'h0002 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mmux_RXDATA_REG5[7]_GND_24_o_mux_9_OUT61  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5 [5]),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1_309 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_REG3_310 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_294 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<5> )
  );
  LUT4 #(
    .INIT ( 16'h0002 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mmux_RXDATA_REG5[7]_GND_24_o_mux_9_OUT81  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5 [7]),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1_309 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_REG3_310 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_294 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<7> )
  );
  LUT4 #(
    .INIT ( 16'h0800 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/K29p71  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [2]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [3]),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [1]),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_111_o1_235 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/K29p7 )
  );
  LUT6 #(
    .INIT ( 64'h8000000000000000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_111_o11  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [5]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [0]),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [6]),
    .I3(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [7]),
    .I4(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [4]),
    .I5(\U0/xst_options.gpcs_pma_inst/RXCHARISK_INT_118 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_111_o1_235 )
  );
  LUT4 #(
    .INIT ( 16'hFF54 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mmux_RXDATA_REG5[7]_GND_24_o_mux_9_OUT11  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_294 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1_309 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5 [0]),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_REG3_310 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<0> )
  );
  LUT5 #(
    .INIT ( 32'hFFFF4540 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mmux_RXDATA_REG5[7]_GND_24_o_mux_9_OUT51  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_294 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_ERR_284 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1_309 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5 [4]),
    .I4(\U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_REG3_310 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<4> )
  );
  LUT5 #(
    .INIT ( 32'h08080800 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/S_WAIT_FOR_K_AND_144_o1  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_123 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/S_327 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/WAIT_FOR_K_270 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_267 ),
    .I4(\U0/xst_options.gpcs_pma_inst/RECEIVER/I_REG_318 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/S_WAIT_FOR_K_AND_144_o )
  );
  LUT4 #(
    .INIT ( 16'h2000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/K23p71  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [2]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [3]),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [1]),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_111_o1_235 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/K23p7 )
  );
  LUT5 #(
    .INIT ( 32'h00200000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_111_o1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [2]),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [3]),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXFIFO_ERR_RXDISPERR_OR_36_o ),
    .I4(\U0/xst_options.gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_111_o1_235 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/K27p7_RXFIFO_ERR_AND_111_o )
  );
  LUT4 #(
    .INIT ( 16'hFF10 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mmux_RXDATA_REG5[7]_GND_24_o_mux_9_OUT71  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_294 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1_309 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5 [6]),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_REG3_310 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5[7]_GND_24_o_mux_9_OUT<6> )
  );
  LUT2 #(
    .INIT ( 4'hE ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/IDLE_REG[1]_IDLE_REG[2]_OR_114_o1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/IDLE_REG [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/IDLE_REG [2]),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/IDLE_REG[1]_IDLE_REG[2]_OR_114_o )
  );
  LUT4 #(
    .INIT ( 16'hFFFE ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG[0]_RX_CONFIG_VALID_REG[3]_OR_113_o<0>1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [0]),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [1]),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [2]),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG [3]),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_CONFIG_VALID_REG[0]_RX_CONFIG_VALID_REG[3]_OR_113_o )
  );
  LUT3 #(
    .INIT ( 8'hEA ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG3_EXT_ILLEGAL_K_REG2_OR_83_o1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_REG2_281 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG3_308 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/CGBAD_REG3_297 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG3_EXT_ILLEGAL_K_REG2_OR_83_o )
  );
  LUT3 #(
    .INIT ( 8'hEA ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/EOP_EXTEND_OR_65_o1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/EOP_290 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_267 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1_309 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/EOP_EXTEND_OR_65_o )
  );
  LUT4 #(
    .INIT ( 16'h0002 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/RXCHARISK_REG1_K28p5_REG1_AND_167_o1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXCHARISK_REG1_315 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_323 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/R_299 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/T_326 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXCHARISK_REG1_K28p5_REG1_AND_167_o )
  );
  LUT2 #(
    .INIT ( 4'h8 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_EVEN_AND_127_o1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_323 ),
    .I1(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/EVEN_124 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_EVEN_AND_127_o )
  );
  LUT3 #(
    .INIT ( 8'hFE ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/RXFIFO_ERR_RXDISPERR_OR_36_o1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXBUFSTATUS_INT [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXNOTINTABLE_INT_108 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDISPERR_INT_109 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXFIFO_ERR_RXDISPERR_OR_36_o )
  );
  LUT2 #(
    .INIT ( 4'hD ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_51_o1  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_123 ),
    .I1(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_51_o )
  );
  LUT4 #(
    .INIT ( 16'hFFFE ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT3_SW0  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/V_184 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1 [2]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/T_193 ),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/R_185 ),
    .O(N2)
  );
  LUT6 #(
    .INIT ( 64'hFFFFBBAB55551101 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT3  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_187 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_195 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_186 ),
    .I3(N2),
    .I4(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3]),
    .I5(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [2]),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<2> )
  );
  LUT4 #(
    .INIT ( 16'hFFFE ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT4_SW0  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/V_184 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/T_193 ),
    .I2(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3]),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_195 ),
    .O(N6)
  );
  LUT6 #(
    .INIT ( 64'hFFFFBBAB55551101 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT4  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_187 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/R_185 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_186 ),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1 [3]),
    .I4(N6),
    .I5(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [3]),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<3> )
  );
  LUT4 #(
    .INIT ( 16'hFFFE ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT7_SW0  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/V_184 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/T_193 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/R_185 ),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_195 ),
    .O(N8)
  );
  LUT6 #(
    .INIT ( 64'hDDDDDCCC11111000 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT7  (
    .I0(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_187 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_186 ),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1 [6]),
    .I4(N8),
    .I5(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [1]),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<6> )
  );
  LUT6 #(
    .INIT ( 64'hFFFFFFFDFFFFFFFF ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_XMIT_DATA_INT_AND_20_o1  (
    .I0(gmii_txd[3]),
    .I1(gmii_txd[7]),
    .I2(gmii_txd[4]),
    .I3(gmii_txd[5]),
    .I4(gmii_txd[6]),
    .I5(gmii_txd[2]),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_XMIT_DATA_INT_AND_20_o1_331 )
  );
  LUT6 #(
    .INIT ( 64'hA8AAAAAA20222222 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_XMIT_DATA_INT_AND_20_o2  (
    .I0(gmii_tx_er),
    .I1(gmii_tx_en),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_XMIT_DATA_INT_AND_20_o1_331 ),
    .I3(gmii_txd[0]),
    .I4(gmii_txd[1]),
    .I5(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_186 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_XMIT_DATA_INT_AND_20_o2_332 )
  );
  LUT6 #(
    .INIT ( 64'hFFFFFFFFFFFF5540 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT21  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/T_193 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1 [1]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_186 ),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/R_185 ),
    .I4(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_195 ),
    .I5(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/V_184 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT2 )
  );
  LUT4 #(
    .INIT ( 16'h8B88 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT22  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_187 ),
    .I2(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3]),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT2 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<1> )
  );
  LUT6 #(
    .INIT ( 64'hFFFFFFFF55555540 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT11  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/V_184 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXD_REG1 [0]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_186 ),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/T_193 ),
    .I4(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/R_185 ),
    .I5(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_195 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT1 )
  );
  LUT4 #(
    .INIT ( 16'h8B88 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT12  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [0]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_187 ),
    .I2(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3]),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_PWR_17_o_CONFIG_DATA[7]_mux_21_OUT1 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/PWR_17_o_CONFIG_DATA[7]_mux_21_OUT<0> )
  );
  LUT2 #(
    .INIT ( 4'hE ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1_ISOLATE_AND_182_o_SW0  (
    .I0(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3]),
    .I1(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [2]),
    .O(N14)
  );
  LUT6 #(
    .INIT ( 64'h5555555144444440 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1_ISOLATE_AND_182_o  (
    .I0(N14),
    .I1(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_123 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_DATA_ERROR_288 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG3_294 ),
    .I4(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1_309 ),
    .I5(\U0/xst_options.gpcs_pma_inst/RECEIVER/RECEIVE_268 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1_ISOLATE_AND_182_o_244 )
  );
  LUT4 #(
    .INIT ( 16'hAAA8 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/EVEN_RXCHARISK_AND_115_o_SW0  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/I_REG_318 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_278 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_K_279 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_DATA_280 ),
    .O(N18)
  );
  LUT6 #(
    .INIT ( 64'h00000000A0A88088 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/EVEN_RXCHARISK_AND_115_o  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/EVEN_124 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_323 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_123 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RXCHARISK_INT_118 ),
    .I4(N18),
    .I5(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_D21p5_AND_116_o_norst ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/EVEN_RXCHARISK_AND_115_o_258 )
  );
  LUT2 #(
    .INIT ( 4'hB ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/K28p51_SW0  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [0]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXCHARISK_INT_118 ),
    .O(N20)
  );
  LUT6 #(
    .INIT ( 64'h4000000000000000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_38_o1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [6]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [7]),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [0]),
    .I3(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [2]),
    .I4(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [4]),
    .I5(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [5]),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_38_o )
  );
  LUT6 #(
    .INIT ( 64'h0000000400000000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_38_o2  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [5]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [6]),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [4]),
    .I3(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [7]),
    .I4(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [2]),
    .I5(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [1]),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_38_o1_339 )
  );
  LUT6 #(
    .INIT ( 64'h0013001100030000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_38_o3  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [0]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [3]),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [1]),
    .I3(\U0/xst_options.gpcs_pma_inst/RXCHARISK_INT_118 ),
    .I4(\U0/xst_options.gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_38_o ),
    .I5(\U0/xst_options.gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_38_o1_339 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_D21p5_AND_116_o_norst )
  );
  LUT4 #(
    .INIT ( 16'hFFFE ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/D0p0_SW0  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [5]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [4]),
    .I2(\U0/xst_options.gpcs_pma_inst/RXCHARISK_INT_118 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [0]),
    .O(N22)
  );
  LUT6 #(
    .INIT ( 64'h0000000000000001 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/D0p0  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [7]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [3]),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [2]),
    .I3(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [1]),
    .I4(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [6]),
    .I5(N22),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/D0p0_242 )
  );
  LUT2 #(
    .INIT ( 4'hE ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG1_C_REG3_OR_59_o_SW0  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG1_317 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG2_236 ),
    .O(N24)
  );
  LUT6 #(
    .INIT ( 64'hFFFF8AAACEEE8AAA ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG1_C_REG3_OR_59_o  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG3_316 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/CGBAD_298 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/EVEN_124 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_323 ),
    .I4(N24),
    .I5(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXCHARISK_REG1_315 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG1_C_REG3_OR_59_o_240 )
  );
  LUT6 #(
    .INIT ( 64'hE8FFFFFFFFFFFFFF ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_108_o11  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [7]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDISPERR_INT_109 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [1]),
    .I3(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [6]),
    .I4(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [4]),
    .I5(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [3]),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_108_o1 )
  );
  LUT5 #(
    .INIT ( 32'hFFFFFFFE ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_108_o13  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [4]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [6]),
    .I2(\U0/xst_options.gpcs_pma_inst/RXCHARISK_INT_118 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [2]),
    .I4(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [3]),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_108_o13_344 )
  );
  LUT4 #(
    .INIT ( 16'hFF17 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_108_o14  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [7]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDISPERR_INT_109 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [1]),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_108_o13_344 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_108_o14_345 )
  );
  LUT5 #(
    .INIT ( 32'h88888000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/I_REG_T_REG2_OR_64_o1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/T_REG2_320 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/R_REG1_319 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/EVEN_124 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_323 ),
    .I4(\U0/xst_options.gpcs_pma_inst/RECEIVER/R_299 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/I_REG_T_REG2_OR_64_o1_346 )
  );
  LUT6 #(
    .INIT ( 64'hFFFFFF80FF80FF80 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/I_REG_T_REG2_OR_64_o2  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG1_317 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/D0p0_REG_322 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/EVEN_124 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/I_REG_T_REG2_OR_64_o1_346 ),
    .I4(\U0/xst_options.gpcs_pma_inst/RECEIVER/I_REG_318 ),
    .I5(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_323 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/I_REG_T_REG2_OR_64_o )
  );
  LUT4 #(
    .INIT ( 16'hFFFE ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/T_REG2_R_REG1_OR_79_o1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/I_REG_318 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/ILLEGAL_K_REG2_285 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG1_317 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/CGBAD_REG3_297 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/T_REG2_R_REG1_OR_79_o1_347 )
  );
  LUT5 #(
    .INIT ( 32'h54545554 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/T_REG2_R_REG1_OR_79_o2  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/R_REG1_319 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/T_REG2_320 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_323 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/R_299 ),
    .I4(\U0/xst_options.gpcs_pma_inst/RECEIVER/T_REG1_321 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/T_REG2_R_REG1_OR_79_o2_348 )
  );
  LUT2 #(
    .INIT ( 4'hE ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/SYNC_STATUS_C_REG1_AND_125_o_SW0  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXCHARISK_REG1_315 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/CGBAD_298 ),
    .O(N26)
  );
  LUT6 #(
    .INIT ( 64'h0010001000100000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/SYNC_STATUS_C_REG1_AND_125_o  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXFIFO_ERR_RXDISPERR_OR_36_o ),
    .I1(\U0/xst_options.gpcs_pma_inst/RXCHARISK_INT_118 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_123 ),
    .I3(N26),
    .I4(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG1_317 ),
    .I5(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_HDR_REMOVED_REG_313 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/SYNC_STATUS_C_REG1_AND_125_o_257 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/V  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/V_glue_set_350 ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/V_184 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_glue_set_351 ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_186 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/R  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/R_glue_set_352 ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/R_185 )
  );
  FDS   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/DISPARITY  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/DISPARITY_glue_rst_353 ),
    .S(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/DISPARITY_183 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/EVEN  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/EVEN_glue_set_354 ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/EVEN_124 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/RECEIVE  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RECEIVE_glue_set_355 ),
    .R(\U0/xst_options.gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_51_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RECEIVE_268 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_INVALID  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_INVALID_glue_set_356 ),
    .R(\U0/xst_options.gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_51_o ),
    .Q(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/RECEIVER/RX_INVALID )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_DV  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_DV_glue_set_357 ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .Q(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/RECEIVER/RX_DV )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_glue_set_358 ),
    .R(\U0/xst_options.gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_51_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_267 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_glue_set_359 ),
    .R(\U0/xst_options.gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_51_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_269 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/RECEIVER/WAIT_FOR_K  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/WAIT_FOR_K_glue_set_360 ),
    .R(\U0/xst_options.gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_51_o ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/WAIT_FOR_K_270 )
  );
  FDR   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/C1_OR_C2  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/C1_OR_C2_rstpot_361 ),
    .R(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/C1_OR_C2_188 )
  );
  FDS   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_rstpot_362 ),
    .S(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_187 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd4  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd4_rstpot_363 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd4_71 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd4  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd4_rstpot_364 ),
    .Q(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd4_78 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRPISK  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_rstpot_365 ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_190 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXCHARDISPVAL  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXCHARDISPVAL_rstpot_366 ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXCHARDISPVAL_131 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TRIGGER_T  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TRIGGER_T_rstpot_367 ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TRIGGER_T_192 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/S  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_rstpot_368 ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_195 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA_0  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA_0_rstpot_369 ),
    .Q(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA [0])
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/C_HDR_REMOVED_REG  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_HDR_REMOVED_REG_rstpot_370 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_HDR_REMOVED_REG_313 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/C  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_rstpot_371 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_324 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_rstpot_372 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_283 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_DATA_ERROR  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_DATA_ERROR_rstpot_373 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_DATA_ERROR_288 )
  );
  LUT6 #(
    .INIT ( 64'hFFFFFFFFA9999995 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_108_o12  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [0]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [5]),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDISPERR_INT_109 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [7]),
    .I4(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [1]),
    .I5(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_108_o12_343 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_rstpot_374 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_278 )
  );
  LUT2 #(
    .INIT ( 4'h8 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_108_o15_SW0  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [2]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXCHARISK_INT_118 ),
    .O(N28)
  );
  LUT6 #(
    .INIT ( 64'h0400000004440044 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_rstpot  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_108_o12_343 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RXNOTINTABLE_INT_108 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_108_o1 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [5]),
    .I4(N28),
    .I5(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_POS_FALSE_NIT_NEG_OR_108_o14_345 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_rstpot_374 )
  );
  LUT2 #(
    .INIT ( 4'hB ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_DATA_ERROR_rstpot_SW0  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/R_299 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/T_REG2_320 ),
    .O(N34)
  );
  LUT6 #(
    .INIT ( 64'h00000000AAAAAA02 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_DATA_ERROR_rstpot  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/RECEIVE_268 ),
    .I1(N34),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_EVEN_AND_127_o ),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/T_REG2_R_REG1_OR_79_o1_347 ),
    .I4(\U0/xst_options.gpcs_pma_inst/RECEIVER/T_REG2_R_REG1_OR_79_o2_348 ),
    .I5(\U0/xst_options.gpcs_pma_inst/RECEIVER/RESET_SYNC_STATUS_OR_51_o ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_DATA_ERROR_rstpot_373 )
  );
  LUT5 #(
    .INIT ( 32'hE8FFFFFF ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_203_o4_SW0  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [4]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [3]),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [2]),
    .I3(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [1]),
    .I4(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [0]),
    .O(N38)
  );
  LUT5 #(
    .INIT ( 32'hFCBDFFFF ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_203_o4_SW1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [3]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [0]),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [1]),
    .I3(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [4]),
    .I4(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [2]),
    .O(N39)
  );
  LUT4 #(
    .INIT ( 16'hDFFF ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/K28p51_SW1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [4]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [1]),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [3]),
    .I3(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [2]),
    .O(N43)
  );
  LUT6 #(
    .INIT ( 64'h0010000000000010 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA[7]_RXNOTINTABLE_AND_211_o1  (
    .I0(N20),
    .I1(\U0/xst_options.gpcs_pma_inst/RXNOTINTABLE_INT_108 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [7]),
    .I3(N43),
    .I4(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [5]),
    .I5(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [6]),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA[7]_RXNOTINTABLE_AND_211_o )
  );
  LUT3 #(
    .INIT ( 8'hBF ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/K28p51_SW2  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [3]),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [2]),
    .O(N45)
  );
  LUT6 #(
    .INIT ( 64'h0000000004000000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/K28p52  (
    .I0(N20),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [7]),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [6]),
    .I3(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [5]),
    .I4(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [4]),
    .I5(N45),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5 )
  );
  LUT2 #(
    .INIT ( 4'hE ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_203_o4_SW2  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXCHARISK_INT_118 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RXNOTINTABLE_INT_108 ),
    .O(N57)
  );
  LUT6 #(
    .INIT ( 64'h0000000400200024 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_203_o5  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [5]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [6]),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [7]),
    .I3(N57),
    .I4(N38),
    .I5(N39),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_DATA_POS_RXNOTINTABLE_AND_203_o )
  );
  LUT2 #(
    .INIT ( 4'hB ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_38_o3_SW0  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXCHARISK_INT_118 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_323 ),
    .O(N59)
  );
  LUT6 #(
    .INIT ( 64'h0013000300110000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/C_rstpot  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [0]),
    .I1(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [3]),
    .I2(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [1]),
    .I3(N59),
    .I4(\U0/xst_options.gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_38_o1_339 ),
    .I5(\U0/xst_options.gpcs_pma_inst/RECEIVER/D21p5_D2p2_OR_38_o ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_rstpot_371 )
  );
  FD   \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN_rstpot_383 ),
    .Q(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN )
  );
  FD #(
    .INIT ( 1'b0 ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS  (
    .C(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_rstpot_384 ),
    .Q(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_123 )
  );
  LUT4 #(
    .INIT ( 16'h6A2A ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/C1_OR_C2_rstpot  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/C1_OR_C2_188 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [1]),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_187 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/C1_OR_C2_rstpot_361 )
  );
  LUT3 #(
    .INIT ( 8'h2F ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/EVEN_glue_set  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXCHARISCOMMA_INT_119 ),
    .I1(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_123 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/EVEN_124 ),
    .O(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/EVEN_glue_set_354 )
  );
  LUT6 #(
    .INIT ( 64'h0001010101010101 ))
  \U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd4_rstpot  (
    .I0(\U0/xst_options.gpcs_pma_inst/RESET_INT_128 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RXBUFSTATUS_INT [1]),
    .I2(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd4_71 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd2_69 ),
    .I4(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd3_70 ),
    .I5(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd1_68 ),
    .O(\U0/xst_options.gpcs_pma_inst/RX_RST_SM_FSM_FFd4_rstpot_363 )
  );
  LUT6 #(
    .INIT ( 64'h0001010101010101 ))
  \U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd4_rstpot  (
    .I0(\U0/xst_options.gpcs_pma_inst/RESET_INT_128 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TXBUFERR_INT_103 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd4_78 ),
    .I3(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd2_76 ),
    .I4(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd3_77 ),
    .I5(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd1_75 ),
    .O(\U0/xst_options.gpcs_pma_inst/TX_RST_SM_FSM_FFd4_rstpot_364 )
  );
  LUT4 #(
    .INIT ( 16'h0002 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA_0_rstpot  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I1(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/C1_OR_C2_188 ),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [1]),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CONFIG_DATA_0_rstpot_369 )
  );
  LUT5 #(
    .INIT ( 32'h2AFF2A2A ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/WAIT_FOR_K_glue_set  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/WAIT_FOR_K_270 ),
    .I1(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/EVEN_124 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_323 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/SYNC_STATUS_REG_295 ),
    .I4(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_123 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/WAIT_FOR_K_glue_set_360 )
  );
  LUT3 #(
    .INIT ( 8'hBA ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_glue_set  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_195 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/T_193 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_186 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_glue_set_351 )
  );
  LUT3 #(
    .INIT ( 8'hBA ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/RECEIVE_glue_set  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_REG2_311 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/EOP_290 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/RECEIVE_268 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/RECEIVE_glue_set_355 )
  );
  LUT3 #(
    .INIT ( 8'hBA ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_INVALID_glue_set  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/FROM_RX_CX_292 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_323 ),
    .I2(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/RECEIVER/RX_INVALID ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_INVALID_glue_set_356 )
  );
  LUT3 #(
    .INIT ( 8'h02 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TRIGGER_T_rstpot  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_209 ),
    .I1(gmii_tx_en),
    .I2(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TRIGGER_T_rstpot_367 )
  );
  LUT4 #(
    .INIT ( 16'h0040 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/C_HDR_REMOVED_REG_rstpot  (
    .I0(\U0/xst_options.gpcs_pma_inst/RXCLKCORCNT_INT [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_REG2_236 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RXCLKCORCNT_INT [0]),
    .I3(\U0/xst_options.gpcs_pma_inst/RXCLKCORCNT_INT [2]),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/C_HDR_REMOVED_REG_rstpot_370 )
  );
  LUT5 #(
    .INIT ( 32'hFFFF4440 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/R_glue_set  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_195 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/R_185 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_ER_REG1_208 ),
    .I4(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/T_193 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/R_glue_set_352 )
  );
  LUT2 #(
    .INIT ( 4'hB ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_DV_glue_set_SW0  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/EOP_REG1_289 ),
    .I1(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/RECEIVER/RX_DV ),
    .O(N71)
  );
  LUT6 #(
    .INIT ( 64'h0200FFFF0200AAAA ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/RX_DV_glue_set  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_123 ),
    .I1(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3]),
    .I2(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [2]),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_REG3_310 ),
    .I4(N71),
    .I5(\U0/xst_options.gpcs_pma_inst/RECEIVER/RECEIVE_268 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/RX_DV_glue_set_357 )
  );
  LUT4 #(
    .INIT ( 16'hFFDF ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_glue_set_SW1  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/I_REG_318 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/S_327 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_123 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_NIT_278 ),
    .O(N73)
  );
  LUT6 #(
    .INIT ( 64'h44444445CCCCCCCD ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_glue_set  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_323 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_269 ),
    .I2(N73),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_DATA_280 ),
    .I4(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_K_279 ),
    .I5(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/EVEN_124 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_glue_set_359 )
  );
  LUT3 #(
    .INIT ( 8'h08 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/DISPARITY_glue_rst_SW1  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_191 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I2(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .O(N75)
  );
  LUT6 #(
    .INIT ( 64'h00000000857A7A85 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/DISPARITY_glue_rst  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [5]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [7]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [6]),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/DISPARITY_183 ),
    .I4(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/DISP5 ),
    .I5(N75),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/DISPARITY_glue_rst_353 )
  );
  LUT2 #(
    .INIT ( 4'h8 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_glue_set_SW0  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/RECEIVE_268 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/R_REG1_319 ),
    .O(N77)
  );
  LUT6 #(
    .INIT ( 64'hFFFF022202220222 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_glue_set  (
    .I0(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_267 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/S_327 ),
    .I2(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_323 ),
    .I3(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/EVEN_124 ),
    .I4(N77),
    .I5(\U0/xst_options.gpcs_pma_inst/RECEIVER/R_299 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_glue_set_358 )
  );
  LUT2 #(
    .INIT ( 4'hB ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/V_glue_set_SW1  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_186 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_ER_REG1_208 ),
    .O(N79)
  );
  LUT6 #(
    .INIT ( 64'hFFFF445444544454 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/V_glue_set  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_187 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_XMIT_DATA_INT_AND_20_o2_332 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_209 ),
    .I3(N79),
    .I4(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_195 ),
    .I5(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/V_184 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/V_glue_set_350 )
  );
  LUT4 #(
    .INIT ( 16'hFFFB ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_rstpot_SW0  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/T_193 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_186 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_195 ),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/V_184 ),
    .O(N81)
  );
  LUT6 #(
    .INIT ( 64'h5F5F0F0F5F5C0F0C ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_rstpot  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/R_185 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_187 ),
    .I3(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3]),
    .I4(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I5(N81),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_rstpot_365 )
  );
  LUT3 #(
    .INIT ( 8'hA8 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_rstpot  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_187 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [1]),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_rstpot_362 )
  );
  LUT3 #(
    .INIT ( 8'h2A ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_CODE_GRPISK_GND_21_o_MUX_79_o11  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_190 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_191 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRPISK_GND_21_o_MUX_79_o )
  );
  LUT4 #(
    .INIT ( 16'hEA2A ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_CODE_GRP[7]_GND_21_o_mux_24_OUT11  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [0]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_191 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/DISPARITY_183 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<0> )
  );
  LUT3 #(
    .INIT ( 8'h2A ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_CODE_GRP[7]_GND_21_o_mux_24_OUT21  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_191 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<1> )
  );
  LUT4 #(
    .INIT ( 16'hEA2A ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_CODE_GRP[7]_GND_21_o_mux_24_OUT31  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [2]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_191 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/DISPARITY_183 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<2> )
  );
  LUT3 #(
    .INIT ( 8'h2A ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_CODE_GRP[7]_GND_21_o_mux_24_OUT41  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [3]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_191 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<3> )
  );
  LUT4 #(
    .INIT ( 16'h2AEA ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_CODE_GRP[7]_GND_21_o_mux_24_OUT51  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [4]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_191 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/DISPARITY_183 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<4> )
  );
  LUT3 #(
    .INIT ( 8'h2A ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_CODE_GRP[7]_GND_21_o_mux_24_OUT61  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [5]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_191 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<5> )
  );
  LUT3 #(
    .INIT ( 8'hEA ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_CODE_GRP[7]_GND_21_o_mux_24_OUT71  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [6]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_191 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<6> )
  );
  LUT4 #(
    .INIT ( 16'hEA2A ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_CODE_GRP[7]_GND_21_o_mux_24_OUT81  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP [7]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_191 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/DISPARITY_183 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP[7]_GND_21_o_mux_24_OUT<7> )
  );
  LUT3 #(
    .INIT ( 8'h20 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/_n0234<1>1  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/C1_OR_C2_188 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [1]),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/_n0234 [1])
  );
  LUT4 #(
    .INIT ( 16'h0040 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXCHARDISPVAL_rstpot  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/DISPARITY_183 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/SYNC_DISPARITY_191 ),
    .I3(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TXCHARDISPVAL_rstpot_366 )
  );
  LUT6 #(
    .INIT ( 64'h0000000000000008 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_rstpot  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_123 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1_309 ),
    .I2(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_RX_RESET_INT ),
    .I3(\U0/xst_options.gpcs_pma_inst/RECEIVER/K28p5_REG1_EVEN_AND_127_o ),
    .I4(\U0/xst_options.gpcs_pma_inst/RECEIVER/R_299 ),
    .I5(\U0/xst_options.gpcs_pma_inst/RECEIVER/S_327 ),
    .O(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXT_ILLEGAL_K_rstpot_372 )
  );
  MUXF7   \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In2  (
    .I0(N83),
    .I1(N84),
    .S(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/CGBAD ),
    .O(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In2_223 )
  );
  LUT6 #(
    .INIT ( 64'hF0F4540400040404 ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In2_F  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_222 ),
    .I1(\U0/xst_options.gpcs_pma_inst/RXCHARISCOMMA_INT_119 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_219 ),
    .I3(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_221 ),
    .I4(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_220 ),
    .I5(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_PWR_19_o_equal_19_o ),
    .O(N83)
  );
  LUT5 #(
    .INIT ( 32'hDBDB8988 ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4-In2_G  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_221 ),
    .I1(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_220 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_222 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RXCHARISCOMMA_INT_119 ),
    .I4(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_219 ),
    .O(N84)
  );
  MUXF7   \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN_rstpot  (
    .I0(N85),
    .I1(N86),
    .S(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/CGBAD ),
    .O(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN_rstpot_383 )
  );
  LUT6 #(
    .INIT ( 64'hAAAAAA2AAAAAAB2B ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN_rstpot_F  (
    .I0(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN ),
    .I1(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_220 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_222 ),
    .I3(\U0/xst_options.gpcs_pma_inst/RXCHARISK_INT_118 ),
    .I4(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_221 ),
    .I5(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_219 ),
    .O(N85)
  );
  LUT5 #(
    .INIT ( 32'hFFFFA889 ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN_rstpot_G  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_219 ),
    .I1(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_220 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_221 ),
    .I3(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_222 ),
    .I4(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/ENCOMMAALIGN ),
    .O(N86)
  );
  MUXF7   \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_rstpot  (
    .I0(N87),
    .I1(N88),
    .S(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/CGBAD ),
    .O(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_rstpot_384 )
  );
  LUT6 #(
    .INIT ( 64'hAAAAAAA8AEAAAEA8 ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_rstpot_F  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_123 ),
    .I1(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_220 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_221 ),
    .I3(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_222 ),
    .I4(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_219 ),
    .I5(\U0/xst_options.gpcs_pma_inst/RXCHARISK_INT_118 ),
    .O(N87)
  );
  LUT5 #(
    .INIT ( 32'h222A22A8 ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_rstpot_G  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_123 ),
    .I1(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_219 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_222 ),
    .I3(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_220 ),
    .I4(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_221 ),
    .O(N88)
  );
  MUXF7   \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2-In22  (
    .I0(N89),
    .I1(N90),
    .S(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/CGBAD ),
    .O(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2-In2 )
  );
  LUT6 #(
    .INIT ( 64'hF212E2A2F69AE6AA ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2-In22_F  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_220 ),
    .I1(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_222 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_219 ),
    .I3(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_221 ),
    .I4(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS[1]_PWR_19_o_equal_19_o ),
    .I5(\U0/xst_options.gpcs_pma_inst/RXCHARISK_INT_118 ),
    .O(N89)
  );
  LUT3 #(
    .INIT ( 8'h20 ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2-In22_G  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_221 ),
    .I1(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_222 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_220 ),
    .O(N90)
  );
  MUXF7   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_TX_PACKET_CODE_GRP_CNT[1]_MUX_73_o11  (
    .I0(N91),
    .I1(N92),
    .S(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_187 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_CODE_GRP_CNT[1]_MUX_73_o )
  );
  LUT6 #(
    .INIT ( 64'hFFFFFFFF00000001 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_TX_PACKET_CODE_GRP_CNT[1]_MUX_73_o11_F  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_195 ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/V_184 ),
    .I2(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/T_193 ),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/R_185 ),
    .I4(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_PACKET_186 ),
    .I5(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/CONFIGURATION_VECTOR_REG [3]),
    .O(N91)
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mmux_TX_PACKET_CODE_GRP_CNT[1]_MUX_73_o11_G  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [1]),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(N92)
  );
  MUXF7   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_rstpot  (
    .I0(N93),
    .I1(N94),
    .S(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TRIGGER_S_194 ),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_rstpot_368 )
  );
  LUT6 #(
    .INIT ( 64'h0010000000100010 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_rstpot_F  (
    .I0(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_EN_REG1_209 ),
    .I1(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .I2(gmii_tx_en),
    .I3(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_187 ),
    .I4(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/TX_ER_REG1_208 ),
    .I5(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(N93)
  );
  LUT2 #(
    .INIT ( 4'h1 ))
  \U0/xst_options.gpcs_pma_inst/TRANSMITTER/S_rstpot_G  (
    .I0(\NlwRenamedSig_OI_U0/xst_options.gpcs_pma_inst/MGT_TX_RESET_INT ),
    .I1(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/XMIT_CONFIG_INT_187 ),
    .O(N94)
  );
  MUXF7   \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3-In32  (
    .I0(N95),
    .I1(N96),
    .S(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd4_222 ),
    .O(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3-In3 )
  );
  LUT6 #(
    .INIT ( 64'hAA8AAB8AAA8AAA8A ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3-In32_F  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_221 ),
    .I1(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd2_220 ),
    .I2(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/CGBAD ),
    .I3(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_219 ),
    .I4(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS [0]),
    .I5(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/GOOD_CGS [1]),
    .O(N95)
  );
  LUT4 #(
    .INIT ( 16'h5501 ))
  \U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3-In32_G  (
    .I0(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd3_221 ),
    .I1(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/CGBAD ),
    .I2(\U0/xst_options.gpcs_pma_inst/RXCHARISK_INT_118 ),
    .I3(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/STATE_FSM_FFd1_219 ),
    .O(N96)
  );
  INV   \U0/xst_options.gpcs_pma_inst/TRANSMITTER/Mcount_CODE_GRP_CNT_xor<0>11_INV_0  (
    .I(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/CODE_GRP_CNT [0]),
    .O(\U0/xst_options.gpcs_pma_inst/TRANSMITTER/Result [0])
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/xst_options.gpcs_pma_inst/Mshreg_STATUS_VECTOR_0  (
    .A0(NlwRenamedSig_OI_status_vector[7]),
    .A1(NlwRenamedSig_OI_status_vector[7]),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/SYNCHRONISATION/SYNC_STATUS_123 ),
    .Q(\U0/xst_options.gpcs_pma_inst/Mshreg_STATUS_VECTOR_0_405 ),
    .Q15(\NLW_U0/xst_options.gpcs_pma_inst/Mshreg_STATUS_VECTOR_0_Q15_UNCONNECTED )
  );
  FDE   \U0/xst_options.gpcs_pma_inst/STATUS_VECTOR_0  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/xst_options.gpcs_pma_inst/Mshreg_STATUS_VECTOR_0_405 ),
    .Q(\NlwRenamedSignal_U0/xst_options.gpcs_pma_inst/STATUS_VECTOR_0 )
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_7  (
    .A0(N0),
    .A1(N0),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [7]),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_7_406 ),
    .Q15(\NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_7_Q15_UNCONNECTED )
  );
  FDE   \U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5_7  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_7_406 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5 [7])
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_6  (
    .A0(N0),
    .A1(N0),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [6]),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_6_407 ),
    .Q15(\NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_6_Q15_UNCONNECTED )
  );
  FDE   \U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5_6  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_6_407 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5 [6])
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_5  (
    .A0(N0),
    .A1(N0),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [5]),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_5_408 ),
    .Q15(\NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_5_Q15_UNCONNECTED )
  );
  FDE   \U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5_5  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_5_408 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5 [5])
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_2  (
    .A0(N0),
    .A1(N0),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [2]),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_2_409 ),
    .Q15(\NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_2_Q15_UNCONNECTED )
  );
  FDE   \U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5_2  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_2_409 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5 [2])
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_4  (
    .A0(N0),
    .A1(N0),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [4]),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_4_410 ),
    .Q15(\NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_4_Q15_UNCONNECTED )
  );
  FDE   \U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5_4  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_4_410 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5 [4])
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_3  (
    .A0(N0),
    .A1(N0),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [3]),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_3_411 ),
    .Q15(\NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_3_Q15_UNCONNECTED )
  );
  FDE   \U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5_3  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_3_411 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5 [3])
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_EXTEND_REG3  (
    .A0(NlwRenamedSig_OI_status_vector[7]),
    .A1(NlwRenamedSig_OI_status_vector[7]),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG1_309 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_EXTEND_REG3_412 ),
    .Q15(\NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_EXTEND_REG3_Q15_UNCONNECTED )
  );
  FDE   \U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG3  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_EXTEND_REG3_412 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/EXTEND_REG3_308 )
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_1  (
    .A0(N0),
    .A1(N0),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [1]),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_1_413 ),
    .Q15(\NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_1_Q15_UNCONNECTED )
  );
  FDE   \U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5_1  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_1_413 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5 [1])
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_0  (
    .A0(N0),
    .A1(N0),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RXDATA_INT [0]),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_0_414 ),
    .Q15(\NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_0_Q15_UNCONNECTED )
  );
  FDE   \U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5_0  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_RXDATA_REG5_0_414 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/RXDATA_REG5 [0])
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_CGBAD_REG2  (
    .A0(N0),
    .A1(NlwRenamedSig_OI_status_vector[7]),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/CGBAD_298 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/CGBAD_REG2 ),
    .Q15(\NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_CGBAD_REG2_Q15_UNCONNECTED )
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_SOP_REG2  (
    .A0(NlwRenamedSig_OI_status_vector[7]),
    .A1(NlwRenamedSig_OI_status_vector[7]),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_291 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_SOP_REG2_415 ),
    .Q15(\NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_SOP_REG2_Q15_UNCONNECTED )
  );
  FDE   \U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_REG2  (
    .C(userclk2),
    .CE(N0),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_SOP_REG2_415 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/SOP_REG2_311 )
  );
  SRLC16E #(
    .INIT ( 16'h0000 ))
  \U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_FALSE_CARRIER_REG2  (
    .A0(N0),
    .A1(NlwRenamedSig_OI_status_vector[7]),
    .A2(NlwRenamedSig_OI_status_vector[7]),
    .A3(NlwRenamedSig_OI_status_vector[7]),
    .CE(N0),
    .CLK(userclk2),
    .D(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_269 ),
    .Q(\U0/xst_options.gpcs_pma_inst/RECEIVER/FALSE_CARRIER_REG2 ),
    .Q15(\NLW_U0/xst_options.gpcs_pma_inst/RECEIVER/Mshreg_FALSE_CARRIER_REG2_Q15_UNCONNECTED )
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
