////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1995-2010 Xilinx, Inc.  All rights reserved.
////////////////////////////////////////////////////////////////////////////////
//   ____  ____
//  /   /\/   /
// /___/  \  /    Vendor: Xilinx
// \   \   \/     Version: M.53d
//  \   \         Application: netgen
//  /   /         Filename: icon.v
// /___/   /\     Timestamp: Tue Jul 20 20:31:15 2010
// \   \  /  \ 
//  \___\/\___\
//             
// Command	: -w -sim -ofmt verilog /home/ianb/ettus/sram_fifo/fpgapriv/usrp2/extramfifo/tmp/_cg/icon.ngc /home/ianb/ettus/sram_fifo/fpgapriv/usrp2/extramfifo/tmp/_cg/icon.v 
// Device	: xc3s2000-fg456-5
// Input file	: /home/ianb/ettus/sram_fifo/fpgapriv/usrp2/extramfifo/tmp/_cg/icon.ngc
// Output file	: /home/ianb/ettus/sram_fifo/fpgapriv/usrp2/extramfifo/tmp/_cg/icon.v
// # of Modules	: 1
// Design Name	: icon
// Xilinx        : /opt/Xilinx/12.1/ISE_DS/ISE
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

module icon (
CONTROL0
)/* synthesis syn_black_box syn_noprune=1 */;
  inout [35 : 0] CONTROL0;
  
  // synthesis translate_off
  
  wire N1;
  wire \U0/U_ICON/I_YES_BSCAN.U_BS/DRCK1 ;
  wire \U0/U_ICON/U_CMD/iSEL_n ;
  wire \U0/U_ICON/U_CMD/iTARGET_CE ;
  wire \U0/U_ICON/U_CTRL_OUT/iDATA_VALID ;
  wire \U0/U_ICON/U_STAT/iCMD_GRP0_SEL ;
  wire \U0/U_ICON/U_STAT/iDATA_VALID ;
  wire \U0/U_ICON/U_STAT/iSTATCMD_CE ;
  wire \U0/U_ICON/U_STAT/iSTATCMD_CE_n ;
  wire \U0/U_ICON/U_STAT/iSTAT_HIGH ;
  wire \U0/U_ICON/U_STAT/iSTAT_LOW ;
  wire \U0/U_ICON/U_STAT/iTDO_next ;
  wire \U0/U_ICON/U_SYNC/iDATA_CMD_n ;
  wire \U0/U_ICON/U_SYNC/iGOT_SYNC ;
  wire \U0/U_ICON/U_SYNC/iGOT_SYNC_HIGH ;
  wire \U0/U_ICON/U_SYNC/iGOT_SYNC_LOW ;
  wire \U0/U_ICON/U_TDO_MUX/U_CS_MUX/I4.U_MUX16/Mmux_O_3_91 ;
  wire \U0/U_ICON/U_TDO_MUX/U_CS_MUX/I4.U_MUX16/Mmux_O_4_92 ;
  wire \U0/U_ICON/iCORE_ID_SEL[0] ;
  wire \U0/U_ICON/iCORE_ID_SEL[15] ;
  wire \U0/U_ICON/iDATA_CMD ;
  wire \U0/U_ICON/iDATA_CMD_n ;
  wire \U0/U_ICON/iSEL ;
  wire \U0/U_ICON/iSEL_n ;
  wire \U0/U_ICON/iSYNC ;
  wire \U0/U_ICON/iTDI ;
  wire \U0/U_ICON/iTDO ;
  wire \U0/U_ICON/iTDO_next ;
  wire \U0/iSHIFT_OUT ;
  wire \U0/iUPDATE_OUT ;
  wire \NLW_U0/U_ICON/I_YES_BSCAN.U_BS/I_SPARTAN3.ISYN.I_USE_SOFTBSCAN_EQ0.I_3.U_BS_DRCK2_UNCONNECTED ;
  wire \NLW_U0/U_ICON/I_YES_BSCAN.U_BS/I_SPARTAN3.ISYN.I_USE_SOFTBSCAN_EQ0.I_3.U_BS_RESET_UNCONNECTED ;
  wire \NLW_U0/U_ICON/I_YES_BSCAN.U_BS/I_SPARTAN3.ISYN.I_USE_SOFTBSCAN_EQ0.I_3.U_BS_CAPTURE_UNCONNECTED ;
  wire \NLW_U0/U_ICON/I_YES_BSCAN.U_BS/I_SPARTAN3.ISYN.I_USE_SOFTBSCAN_EQ0.I_3.U_BS_SEL2_UNCONNECTED ;
  wire \NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[1].U_LUT_O_UNCONNECTED ;
  wire \NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[2].U_LUT_O_UNCONNECTED ;
  wire \NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[3].U_LUT_O_UNCONNECTED ;
  wire \NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[4].U_LUT_O_UNCONNECTED ;
  wire \NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[5].U_LUT_O_UNCONNECTED ;
  wire \NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[6].U_LUT_O_UNCONNECTED ;
  wire \NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[7].U_LUT_O_UNCONNECTED ;
  wire \NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[8].U_LUT_O_UNCONNECTED ;
  wire \NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[9].U_LUT_O_UNCONNECTED ;
  wire \NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[10].U_LUT_O_UNCONNECTED ;
  wire \NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[11].U_LUT_O_UNCONNECTED ;
  wire \NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[12].U_LUT_O_UNCONNECTED ;
  wire \NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[13].U_LUT_O_UNCONNECTED ;
  wire \NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[14].U_LUT_O_UNCONNECTED ;
  wire [11 : 8] \U0/U_ICON/U_CMD/iTARGET ;
  wire [1 : 0] \U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL ;
  wire [5 : 1] \U0/U_ICON/U_STAT/U_STAT_CNT/CI ;
  wire [5 : 0] \U0/U_ICON/U_STAT/U_STAT_CNT/D ;
  wire [5 : 0] \U0/U_ICON/U_STAT/U_STAT_CNT/S ;
  wire [3 : 0] \U0/U_ICON/U_STAT/iSTAT ;
  wire [5 : 0] \U0/U_ICON/U_STAT/iSTAT_CNT ;
  wire [6 : 0] \U0/U_ICON/U_SYNC/iSYNC_WORD ;
  wire [1 : 0] \U0/U_ICON/iCOMMAND_GRP ;
  wire [15 : 0] \U0/U_ICON/iCOMMAND_SEL ;
  wire [3 : 0] \U0/U_ICON/iCORE_ID ;
  wire [15 : 15] \U0/U_ICON/iTDO_VEC ;
  GND   XST_GND (
    .G(CONTROL0[2])
  );
  VCC   XST_VCC (
    .P(N1)
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_TDI_reg  (
    .C(CONTROL0[0]),
    .CE(N1),
    .D(\U0/U_ICON/iTDI ),
    .Q(CONTROL0[1])
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_TDO_reg  (
    .C(CONTROL0[0]),
    .CE(N1),
    .D(\U0/U_ICON/iTDO_next ),
    .Q(\U0/U_ICON/iTDO )
  );
  FDC #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_iDATA_CMD  (
    .C(\U0/iUPDATE_OUT ),
    .CLR(\U0/U_ICON/iSEL_n ),
    .D(\U0/U_ICON/iDATA_CMD_n ),
    .Q(\U0/U_ICON/iDATA_CMD )
  );
  MUXF5   \U0/U_ICON/U_TDO_MUX/U_CS_MUX/I4.U_MUX16/Mmux_O_2_f5  (
    .I0(\U0/U_ICON/U_TDO_MUX/U_CS_MUX/I4.U_MUX16/Mmux_O_4_92 ),
    .I1(\U0/U_ICON/U_TDO_MUX/U_CS_MUX/I4.U_MUX16/Mmux_O_3_91 ),
    .S(\U0/U_ICON/iCORE_ID [3]),
    .O(\U0/U_ICON/iTDO_next )
  );
  LUT4 #(
    .INIT ( 16'h0002 ))
  \U0/U_ICON/U_TDO_MUX/U_CS_MUX/I4.U_MUX16/Mmux_O_4  (
    .I0(CONTROL0[3]),
    .I1(\U0/U_ICON/iCORE_ID [0]),
    .I2(\U0/U_ICON/iCORE_ID [1]),
    .I3(\U0/U_ICON/iCORE_ID [2]),
    .O(\U0/U_ICON/U_TDO_MUX/U_CS_MUX/I4.U_MUX16/Mmux_O_4_92 )
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_TDO_MUX/U_CS_MUX/I4.U_MUX16/Mmux_O_3  (
    .I0(\U0/U_ICON/iTDO_VEC [15]),
    .I1(\U0/U_ICON/iCORE_ID [0]),
    .I2(\U0/U_ICON/iCORE_ID [1]),
    .I3(\U0/U_ICON/iCORE_ID [2]),
    .O(\U0/U_ICON/U_TDO_MUX/U_CS_MUX/I4.U_MUX16/Mmux_O_3_91 )
  );
  INV   \U0/U_ICON/U_iSEL_n  (
    .I(\U0/U_ICON/iSEL ),
    .O(\U0/U_ICON/iSEL_n )
  );
  INV   \U0/U_ICON/U_iDATA_CMD_n  (
    .I(\U0/U_ICON/iDATA_CMD ),
    .O(\U0/U_ICON/iDATA_CMD_n )
  );
  BSCAN_SPARTAN3   \U0/U_ICON/I_YES_BSCAN.U_BS/I_SPARTAN3.ISYN.I_USE_SOFTBSCAN_EQ0.I_3.U_BS  (
    .TDI(\U0/U_ICON/iTDI ),
    .SHIFT(\U0/iSHIFT_OUT ),
    .DRCK1(\U0/U_ICON/I_YES_BSCAN.U_BS/DRCK1 ),
    .DRCK2(\NLW_U0/U_ICON/I_YES_BSCAN.U_BS/I_SPARTAN3.ISYN.I_USE_SOFTBSCAN_EQ0.I_3.U_BS_DRCK2_UNCONNECTED ),
    .RESET(\NLW_U0/U_ICON/I_YES_BSCAN.U_BS/I_SPARTAN3.ISYN.I_USE_SOFTBSCAN_EQ0.I_3.U_BS_RESET_UNCONNECTED ),
    .UPDATE(\U0/iUPDATE_OUT ),
    .TDO1(\U0/U_ICON/iTDO ),
    .TDO2(CONTROL0[2]),
    .CAPTURE(\NLW_U0/U_ICON/I_YES_BSCAN.U_BS/I_SPARTAN3.ISYN.I_USE_SOFTBSCAN_EQ0.I_3.U_BS_CAPTURE_UNCONNECTED ),
    .SEL1(\U0/U_ICON/iSEL ),
    .SEL2(\NLW_U0/U_ICON/I_YES_BSCAN.U_BS/I_SPARTAN3.ISYN.I_USE_SOFTBSCAN_EQ0.I_3.U_BS_SEL2_UNCONNECTED )
  );
  icon_bscan_bufg   \U0/U_ICON/I_YES_BSCAN.U_BS/I_USE_SOFTBSCAN_EQ0.I_USE_XST_TCK_WORKAROUND_EQ1.U_ICON_BSCAN_BUFG  (
    .DRCK_LOCAL_I(\U0/U_ICON/I_YES_BSCAN.U_BS/DRCK1 ),
    .DRCK_LOCAL_O(CONTROL0[0])
  );
  LUT2 #(
    .INIT ( 4'h8 ))
  \U0/U_ICON/U_SYNC/U_GOT_SYNC  (
    .I0(\U0/U_ICON/U_SYNC/iGOT_SYNC_LOW ),
    .I1(\U0/U_ICON/U_SYNC/iGOT_SYNC_HIGH ),
    .O(\U0/U_ICON/U_SYNC/iGOT_SYNC )
  );
  LUT4 #(
    .INIT ( 16'h0200 ))
  \U0/U_ICON/U_SYNC/U_GOT_SYNC_L  (
    .I0(\U0/U_ICON/U_SYNC/iSYNC_WORD [0]),
    .I1(\U0/U_ICON/U_SYNC/iSYNC_WORD [1]),
    .I2(\U0/U_ICON/U_SYNC/iSYNC_WORD [2]),
    .I3(\U0/U_ICON/U_SYNC/iSYNC_WORD [3]),
    .O(\U0/U_ICON/U_SYNC/iGOT_SYNC_LOW )
  );
  LUT4 #(
    .INIT ( 16'h0400 ))
  \U0/U_ICON/U_SYNC/U_GOT_SYNC_H  (
    .I0(\U0/U_ICON/U_SYNC/iSYNC_WORD [4]),
    .I1(\U0/U_ICON/U_SYNC/iSYNC_WORD [5]),
    .I2(\U0/U_ICON/U_SYNC/iSYNC_WORD [6]),
    .I3(CONTROL0[1]),
    .O(\U0/U_ICON/U_SYNC/iGOT_SYNC_HIGH )
  );
  INV   \U0/U_ICON/U_SYNC/U_iDATA_CMD_n  (
    .I(\U0/U_ICON/iDATA_CMD ),
    .O(\U0/U_ICON/U_SYNC/iDATA_CMD_n )
  );
  FDRE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_SYNC/U_SYNC  (
    .C(CONTROL0[0]),
    .CE(\U0/U_ICON/U_SYNC/iGOT_SYNC ),
    .D(N1),
    .R(\U0/U_ICON/U_SYNC/iDATA_CMD_n ),
    .Q(\U0/U_ICON/iSYNC )
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_SYNC/G_SYNC_WORD[0].I_NE0.U_FDR  (
    .C(CONTROL0[0]),
    .D(\U0/U_ICON/U_SYNC/iSYNC_WORD [1]),
    .R(\U0/U_ICON/U_SYNC/iDATA_CMD_n ),
    .Q(\U0/U_ICON/U_SYNC/iSYNC_WORD [0])
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_SYNC/G_SYNC_WORD[1].I_NE0.U_FDR  (
    .C(CONTROL0[0]),
    .D(\U0/U_ICON/U_SYNC/iSYNC_WORD [2]),
    .R(\U0/U_ICON/U_SYNC/iDATA_CMD_n ),
    .Q(\U0/U_ICON/U_SYNC/iSYNC_WORD [1])
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_SYNC/G_SYNC_WORD[2].I_NE0.U_FDR  (
    .C(CONTROL0[0]),
    .D(\U0/U_ICON/U_SYNC/iSYNC_WORD [3]),
    .R(\U0/U_ICON/U_SYNC/iDATA_CMD_n ),
    .Q(\U0/U_ICON/U_SYNC/iSYNC_WORD [2])
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_SYNC/G_SYNC_WORD[3].I_NE0.U_FDR  (
    .C(CONTROL0[0]),
    .D(\U0/U_ICON/U_SYNC/iSYNC_WORD [4]),
    .R(\U0/U_ICON/U_SYNC/iDATA_CMD_n ),
    .Q(\U0/U_ICON/U_SYNC/iSYNC_WORD [3])
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_SYNC/G_SYNC_WORD[4].I_NE0.U_FDR  (
    .C(CONTROL0[0]),
    .D(\U0/U_ICON/U_SYNC/iSYNC_WORD [5]),
    .R(\U0/U_ICON/U_SYNC/iDATA_CMD_n ),
    .Q(\U0/U_ICON/U_SYNC/iSYNC_WORD [4])
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_SYNC/G_SYNC_WORD[5].I_NE0.U_FDR  (
    .C(CONTROL0[0]),
    .D(\U0/U_ICON/U_SYNC/iSYNC_WORD [6]),
    .R(\U0/U_ICON/U_SYNC/iDATA_CMD_n ),
    .Q(\U0/U_ICON/U_SYNC/iSYNC_WORD [5])
  );
  FDR #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_SYNC/G_SYNC_WORD[6].I_EQ0.U_FDR  (
    .C(CONTROL0[0]),
    .D(CONTROL0[1]),
    .R(\U0/U_ICON/U_SYNC/iDATA_CMD_n ),
    .Q(\U0/U_ICON/U_SYNC/iSYNC_WORD [6])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[0].U_HCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [0]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [1]),
    .O(CONTROL0[20])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[0].U_LCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [0]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [0]),
    .O(CONTROL0[4])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[1].U_HCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [1]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [1]),
    .O(CONTROL0[21])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[1].U_LCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [1]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [0]),
    .O(CONTROL0[5])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[2].U_HCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [2]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [1]),
    .O(CONTROL0[22])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[2].U_LCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [2]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [0]),
    .O(CONTROL0[6])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[3].U_HCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [3]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [1]),
    .O(CONTROL0[23])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[3].U_LCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [3]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [0]),
    .O(CONTROL0[7])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[4].U_HCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [4]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [1]),
    .O(CONTROL0[24])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[4].U_LCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [4]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [0]),
    .O(CONTROL0[8])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[5].U_HCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [5]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [1]),
    .O(CONTROL0[25])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[5].U_LCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [5]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [0]),
    .O(CONTROL0[9])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[6].U_HCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [6]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [1]),
    .O(CONTROL0[26])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[6].U_LCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [6]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [0]),
    .O(CONTROL0[10])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[7].U_HCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [7]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [1]),
    .O(CONTROL0[27])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[7].U_LCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [7]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [0]),
    .O(CONTROL0[11])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[8].U_HCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [8]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [1]),
    .O(CONTROL0[28])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[8].U_LCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [8]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [0]),
    .O(CONTROL0[12])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[9].U_HCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [9]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [1]),
    .O(CONTROL0[29])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[9].U_LCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [9]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [0]),
    .O(CONTROL0[13])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[10].U_HCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [10]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [1]),
    .O(CONTROL0[30])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[10].U_LCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [10]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [0]),
    .O(CONTROL0[14])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[11].U_HCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [11]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [1]),
    .O(CONTROL0[31])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[11].U_LCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [11]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [0]),
    .O(CONTROL0[15])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[12].U_HCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [12]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [1]),
    .O(CONTROL0[32])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[12].U_LCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [12]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [0]),
    .O(CONTROL0[16])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[13].U_HCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [13]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [1]),
    .O(CONTROL0[33])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[13].U_LCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [13]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [0]),
    .O(CONTROL0[17])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[14].U_HCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [14]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [1]),
    .O(CONTROL0[34])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[14].U_LCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [14]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [0]),
    .O(CONTROL0[18])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[15].U_HCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [15]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [1]),
    .O(CONTROL0[35])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CTRL_OUT/F_NCP[0].F_CMD[15].U_LCE  (
    .I0(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [15]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[0] ),
    .I3(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [0]),
    .O(CONTROL0[19])
  );
  LUT2 #(
    .INIT ( 4'h2 ))
  \U0/U_ICON/U_CTRL_OUT/U_CMDGRP1  (
    .I0(\U0/U_ICON/iCOMMAND_GRP [0]),
    .I1(\U0/U_ICON/iCOMMAND_GRP [1]),
    .O(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [1])
  );
  LUT2 #(
    .INIT ( 4'h1 ))
  \U0/U_ICON/U_CTRL_OUT/U_CMDGRP0  (
    .I0(\U0/U_ICON/iCOMMAND_GRP [0]),
    .I1(\U0/U_ICON/iCOMMAND_GRP [1]),
    .O(\U0/U_ICON/U_CTRL_OUT/iCOMMAND_GRP_SEL [0])
  );
  LUT2 #(
    .INIT ( 4'h8 ))
  \U0/U_ICON/U_CTRL_OUT/U_DATA_VALID  (
    .I0(\U0/U_ICON/iSYNC ),
    .I1(\U0/iSHIFT_OUT ),
    .O(\U0/U_ICON/U_CTRL_OUT/iDATA_VALID )
  );
  LUT4 #(
    .INIT ( 16'h0001 ))
  \U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[0].U_LUT  (
    .I0(\U0/U_ICON/iCORE_ID [0]),
    .I1(\U0/U_ICON/iCORE_ID [1]),
    .I2(\U0/U_ICON/iCORE_ID [2]),
    .I3(\U0/U_ICON/iCORE_ID [3]),
    .O(\U0/U_ICON/iCORE_ID_SEL[0] )
  );
  LUT4 #(
    .INIT ( 16'h0002 ))
  \U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[1].U_LUT  (
    .I0(\U0/U_ICON/iCORE_ID [0]),
    .I1(\U0/U_ICON/iCORE_ID [1]),
    .I2(\U0/U_ICON/iCORE_ID [2]),
    .I3(\U0/U_ICON/iCORE_ID [3]),
    .O(\NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[1].U_LUT_O_UNCONNECTED )
  );
  LUT4 #(
    .INIT ( 16'h0004 ))
  \U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[2].U_LUT  (
    .I0(\U0/U_ICON/iCORE_ID [0]),
    .I1(\U0/U_ICON/iCORE_ID [1]),
    .I2(\U0/U_ICON/iCORE_ID [2]),
    .I3(\U0/U_ICON/iCORE_ID [3]),
    .O(\NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[2].U_LUT_O_UNCONNECTED )
  );
  LUT4 #(
    .INIT ( 16'h0008 ))
  \U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[3].U_LUT  (
    .I0(\U0/U_ICON/iCORE_ID [0]),
    .I1(\U0/U_ICON/iCORE_ID [1]),
    .I2(\U0/U_ICON/iCORE_ID [2]),
    .I3(\U0/U_ICON/iCORE_ID [3]),
    .O(\NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[3].U_LUT_O_UNCONNECTED )
  );
  LUT4 #(
    .INIT ( 16'h0010 ))
  \U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[4].U_LUT  (
    .I0(\U0/U_ICON/iCORE_ID [0]),
    .I1(\U0/U_ICON/iCORE_ID [1]),
    .I2(\U0/U_ICON/iCORE_ID [2]),
    .I3(\U0/U_ICON/iCORE_ID [3]),
    .O(\NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[4].U_LUT_O_UNCONNECTED )
  );
  LUT4 #(
    .INIT ( 16'h0020 ))
  \U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[5].U_LUT  (
    .I0(\U0/U_ICON/iCORE_ID [0]),
    .I1(\U0/U_ICON/iCORE_ID [1]),
    .I2(\U0/U_ICON/iCORE_ID [2]),
    .I3(\U0/U_ICON/iCORE_ID [3]),
    .O(\NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[5].U_LUT_O_UNCONNECTED )
  );
  LUT4 #(
    .INIT ( 16'h0040 ))
  \U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[6].U_LUT  (
    .I0(\U0/U_ICON/iCORE_ID [0]),
    .I1(\U0/U_ICON/iCORE_ID [1]),
    .I2(\U0/U_ICON/iCORE_ID [2]),
    .I3(\U0/U_ICON/iCORE_ID [3]),
    .O(\NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[6].U_LUT_O_UNCONNECTED )
  );
  LUT4 #(
    .INIT ( 16'h0080 ))
  \U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[7].U_LUT  (
    .I0(\U0/U_ICON/iCORE_ID [0]),
    .I1(\U0/U_ICON/iCORE_ID [1]),
    .I2(\U0/U_ICON/iCORE_ID [2]),
    .I3(\U0/U_ICON/iCORE_ID [3]),
    .O(\NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[7].U_LUT_O_UNCONNECTED )
  );
  LUT4 #(
    .INIT ( 16'h0100 ))
  \U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[8].U_LUT  (
    .I0(\U0/U_ICON/iCORE_ID [0]),
    .I1(\U0/U_ICON/iCORE_ID [1]),
    .I2(\U0/U_ICON/iCORE_ID [2]),
    .I3(\U0/U_ICON/iCORE_ID [3]),
    .O(\NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[8].U_LUT_O_UNCONNECTED )
  );
  LUT4 #(
    .INIT ( 16'h0200 ))
  \U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[9].U_LUT  (
    .I0(\U0/U_ICON/iCORE_ID [0]),
    .I1(\U0/U_ICON/iCORE_ID [1]),
    .I2(\U0/U_ICON/iCORE_ID [2]),
    .I3(\U0/U_ICON/iCORE_ID [3]),
    .O(\NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[9].U_LUT_O_UNCONNECTED )
  );
  LUT4 #(
    .INIT ( 16'h0400 ))
  \U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[10].U_LUT  (
    .I0(\U0/U_ICON/iCORE_ID [0]),
    .I1(\U0/U_ICON/iCORE_ID [1]),
    .I2(\U0/U_ICON/iCORE_ID [2]),
    .I3(\U0/U_ICON/iCORE_ID [3]),
    .O(\NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[10].U_LUT_O_UNCONNECTED )
  );
  LUT4 #(
    .INIT ( 16'h0800 ))
  \U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[11].U_LUT  (
    .I0(\U0/U_ICON/iCORE_ID [0]),
    .I1(\U0/U_ICON/iCORE_ID [1]),
    .I2(\U0/U_ICON/iCORE_ID [2]),
    .I3(\U0/U_ICON/iCORE_ID [3]),
    .O(\NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[11].U_LUT_O_UNCONNECTED )
  );
  LUT4 #(
    .INIT ( 16'h1000 ))
  \U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[12].U_LUT  (
    .I0(\U0/U_ICON/iCORE_ID [0]),
    .I1(\U0/U_ICON/iCORE_ID [1]),
    .I2(\U0/U_ICON/iCORE_ID [2]),
    .I3(\U0/U_ICON/iCORE_ID [3]),
    .O(\NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[12].U_LUT_O_UNCONNECTED )
  );
  LUT4 #(
    .INIT ( 16'h2000 ))
  \U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[13].U_LUT  (
    .I0(\U0/U_ICON/iCORE_ID [0]),
    .I1(\U0/U_ICON/iCORE_ID [1]),
    .I2(\U0/U_ICON/iCORE_ID [2]),
    .I3(\U0/U_ICON/iCORE_ID [3]),
    .O(\NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[13].U_LUT_O_UNCONNECTED )
  );
  LUT4 #(
    .INIT ( 16'h4000 ))
  \U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[14].U_LUT  (
    .I0(\U0/U_ICON/iCORE_ID [0]),
    .I1(\U0/U_ICON/iCORE_ID [1]),
    .I2(\U0/U_ICON/iCORE_ID [2]),
    .I3(\U0/U_ICON/iCORE_ID [3]),
    .O(\NLW_U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[14].U_LUT_O_UNCONNECTED )
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CMD/U_CORE_ID_SEL/I4.FI[15].U_LUT  (
    .I0(\U0/U_ICON/iCORE_ID [0]),
    .I1(\U0/U_ICON/iCORE_ID [1]),
    .I2(\U0/U_ICON/iCORE_ID [2]),
    .I3(\U0/U_ICON/iCORE_ID [3]),
    .O(\U0/U_ICON/iCORE_ID_SEL[15] )
  );
  LUT4 #(
    .INIT ( 16'h0001 ))
  \U0/U_ICON/U_CMD/U_COMMAND_SEL/I4.FI[0].U_LUT  (
    .I0(\U0/U_ICON/U_CMD/iTARGET [8]),
    .I1(\U0/U_ICON/U_CMD/iTARGET [9]),
    .I2(\U0/U_ICON/U_CMD/iTARGET [10]),
    .I3(\U0/U_ICON/U_CMD/iTARGET [11]),
    .O(\U0/U_ICON/iCOMMAND_SEL [0])
  );
  LUT4 #(
    .INIT ( 16'h0002 ))
  \U0/U_ICON/U_CMD/U_COMMAND_SEL/I4.FI[1].U_LUT  (
    .I0(\U0/U_ICON/U_CMD/iTARGET [8]),
    .I1(\U0/U_ICON/U_CMD/iTARGET [9]),
    .I2(\U0/U_ICON/U_CMD/iTARGET [10]),
    .I3(\U0/U_ICON/U_CMD/iTARGET [11]),
    .O(\U0/U_ICON/iCOMMAND_SEL [1])
  );
  LUT4 #(
    .INIT ( 16'h0004 ))
  \U0/U_ICON/U_CMD/U_COMMAND_SEL/I4.FI[2].U_LUT  (
    .I0(\U0/U_ICON/U_CMD/iTARGET [8]),
    .I1(\U0/U_ICON/U_CMD/iTARGET [9]),
    .I2(\U0/U_ICON/U_CMD/iTARGET [10]),
    .I3(\U0/U_ICON/U_CMD/iTARGET [11]),
    .O(\U0/U_ICON/iCOMMAND_SEL [2])
  );
  LUT4 #(
    .INIT ( 16'h0008 ))
  \U0/U_ICON/U_CMD/U_COMMAND_SEL/I4.FI[3].U_LUT  (
    .I0(\U0/U_ICON/U_CMD/iTARGET [8]),
    .I1(\U0/U_ICON/U_CMD/iTARGET [9]),
    .I2(\U0/U_ICON/U_CMD/iTARGET [10]),
    .I3(\U0/U_ICON/U_CMD/iTARGET [11]),
    .O(\U0/U_ICON/iCOMMAND_SEL [3])
  );
  LUT4 #(
    .INIT ( 16'h0010 ))
  \U0/U_ICON/U_CMD/U_COMMAND_SEL/I4.FI[4].U_LUT  (
    .I0(\U0/U_ICON/U_CMD/iTARGET [8]),
    .I1(\U0/U_ICON/U_CMD/iTARGET [9]),
    .I2(\U0/U_ICON/U_CMD/iTARGET [10]),
    .I3(\U0/U_ICON/U_CMD/iTARGET [11]),
    .O(\U0/U_ICON/iCOMMAND_SEL [4])
  );
  LUT4 #(
    .INIT ( 16'h0020 ))
  \U0/U_ICON/U_CMD/U_COMMAND_SEL/I4.FI[5].U_LUT  (
    .I0(\U0/U_ICON/U_CMD/iTARGET [8]),
    .I1(\U0/U_ICON/U_CMD/iTARGET [9]),
    .I2(\U0/U_ICON/U_CMD/iTARGET [10]),
    .I3(\U0/U_ICON/U_CMD/iTARGET [11]),
    .O(\U0/U_ICON/iCOMMAND_SEL [5])
  );
  LUT4 #(
    .INIT ( 16'h0040 ))
  \U0/U_ICON/U_CMD/U_COMMAND_SEL/I4.FI[6].U_LUT  (
    .I0(\U0/U_ICON/U_CMD/iTARGET [8]),
    .I1(\U0/U_ICON/U_CMD/iTARGET [9]),
    .I2(\U0/U_ICON/U_CMD/iTARGET [10]),
    .I3(\U0/U_ICON/U_CMD/iTARGET [11]),
    .O(\U0/U_ICON/iCOMMAND_SEL [6])
  );
  LUT4 #(
    .INIT ( 16'h0080 ))
  \U0/U_ICON/U_CMD/U_COMMAND_SEL/I4.FI[7].U_LUT  (
    .I0(\U0/U_ICON/U_CMD/iTARGET [8]),
    .I1(\U0/U_ICON/U_CMD/iTARGET [9]),
    .I2(\U0/U_ICON/U_CMD/iTARGET [10]),
    .I3(\U0/U_ICON/U_CMD/iTARGET [11]),
    .O(\U0/U_ICON/iCOMMAND_SEL [7])
  );
  LUT4 #(
    .INIT ( 16'h0100 ))
  \U0/U_ICON/U_CMD/U_COMMAND_SEL/I4.FI[8].U_LUT  (
    .I0(\U0/U_ICON/U_CMD/iTARGET [8]),
    .I1(\U0/U_ICON/U_CMD/iTARGET [9]),
    .I2(\U0/U_ICON/U_CMD/iTARGET [10]),
    .I3(\U0/U_ICON/U_CMD/iTARGET [11]),
    .O(\U0/U_ICON/iCOMMAND_SEL [8])
  );
  LUT4 #(
    .INIT ( 16'h0200 ))
  \U0/U_ICON/U_CMD/U_COMMAND_SEL/I4.FI[9].U_LUT  (
    .I0(\U0/U_ICON/U_CMD/iTARGET [8]),
    .I1(\U0/U_ICON/U_CMD/iTARGET [9]),
    .I2(\U0/U_ICON/U_CMD/iTARGET [10]),
    .I3(\U0/U_ICON/U_CMD/iTARGET [11]),
    .O(\U0/U_ICON/iCOMMAND_SEL [9])
  );
  LUT4 #(
    .INIT ( 16'h0400 ))
  \U0/U_ICON/U_CMD/U_COMMAND_SEL/I4.FI[10].U_LUT  (
    .I0(\U0/U_ICON/U_CMD/iTARGET [8]),
    .I1(\U0/U_ICON/U_CMD/iTARGET [9]),
    .I2(\U0/U_ICON/U_CMD/iTARGET [10]),
    .I3(\U0/U_ICON/U_CMD/iTARGET [11]),
    .O(\U0/U_ICON/iCOMMAND_SEL [10])
  );
  LUT4 #(
    .INIT ( 16'h0800 ))
  \U0/U_ICON/U_CMD/U_COMMAND_SEL/I4.FI[11].U_LUT  (
    .I0(\U0/U_ICON/U_CMD/iTARGET [8]),
    .I1(\U0/U_ICON/U_CMD/iTARGET [9]),
    .I2(\U0/U_ICON/U_CMD/iTARGET [10]),
    .I3(\U0/U_ICON/U_CMD/iTARGET [11]),
    .O(\U0/U_ICON/iCOMMAND_SEL [11])
  );
  LUT4 #(
    .INIT ( 16'h1000 ))
  \U0/U_ICON/U_CMD/U_COMMAND_SEL/I4.FI[12].U_LUT  (
    .I0(\U0/U_ICON/U_CMD/iTARGET [8]),
    .I1(\U0/U_ICON/U_CMD/iTARGET [9]),
    .I2(\U0/U_ICON/U_CMD/iTARGET [10]),
    .I3(\U0/U_ICON/U_CMD/iTARGET [11]),
    .O(\U0/U_ICON/iCOMMAND_SEL [12])
  );
  LUT4 #(
    .INIT ( 16'h2000 ))
  \U0/U_ICON/U_CMD/U_COMMAND_SEL/I4.FI[13].U_LUT  (
    .I0(\U0/U_ICON/U_CMD/iTARGET [8]),
    .I1(\U0/U_ICON/U_CMD/iTARGET [9]),
    .I2(\U0/U_ICON/U_CMD/iTARGET [10]),
    .I3(\U0/U_ICON/U_CMD/iTARGET [11]),
    .O(\U0/U_ICON/iCOMMAND_SEL [13])
  );
  LUT4 #(
    .INIT ( 16'h4000 ))
  \U0/U_ICON/U_CMD/U_COMMAND_SEL/I4.FI[14].U_LUT  (
    .I0(\U0/U_ICON/U_CMD/iTARGET [8]),
    .I1(\U0/U_ICON/U_CMD/iTARGET [9]),
    .I2(\U0/U_ICON/U_CMD/iTARGET [10]),
    .I3(\U0/U_ICON/U_CMD/iTARGET [11]),
    .O(\U0/U_ICON/iCOMMAND_SEL [14])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_CMD/U_COMMAND_SEL/I4.FI[15].U_LUT  (
    .I0(\U0/U_ICON/U_CMD/iTARGET [8]),
    .I1(\U0/U_ICON/U_CMD/iTARGET [9]),
    .I2(\U0/U_ICON/U_CMD/iTARGET [10]),
    .I3(\U0/U_ICON/U_CMD/iTARGET [11]),
    .O(\U0/U_ICON/iCOMMAND_SEL [15])
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \U0/U_ICON/U_CMD/U_TARGET_CE  (
    .I0(\U0/U_ICON/iDATA_CMD ),
    .I1(\U0/iSHIFT_OUT ),
    .O(\U0/U_ICON/U_CMD/iTARGET_CE )
  );
  INV   \U0/U_ICON/U_CMD/U_SEL_n  (
    .I(\U0/U_ICON/iSEL ),
    .O(\U0/U_ICON/U_CMD/iSEL_n )
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_CMD/G_TARGET[6].I_NE0.U_TARGET  (
    .C(CONTROL0[0]),
    .CE(\U0/U_ICON/U_CMD/iTARGET_CE ),
    .CLR(\U0/U_ICON/U_CMD/iSEL_n ),
    .D(\U0/U_ICON/iCOMMAND_GRP [1]),
    .Q(\U0/U_ICON/iCOMMAND_GRP [0])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_CMD/G_TARGET[7].I_NE0.U_TARGET  (
    .C(CONTROL0[0]),
    .CE(\U0/U_ICON/U_CMD/iTARGET_CE ),
    .CLR(\U0/U_ICON/U_CMD/iSEL_n ),
    .D(\U0/U_ICON/U_CMD/iTARGET [8]),
    .Q(\U0/U_ICON/iCOMMAND_GRP [1])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_CMD/G_TARGET[8].I_NE0.U_TARGET  (
    .C(CONTROL0[0]),
    .CE(\U0/U_ICON/U_CMD/iTARGET_CE ),
    .CLR(\U0/U_ICON/U_CMD/iSEL_n ),
    .D(\U0/U_ICON/U_CMD/iTARGET [9]),
    .Q(\U0/U_ICON/U_CMD/iTARGET [8])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_CMD/G_TARGET[9].I_NE0.U_TARGET  (
    .C(CONTROL0[0]),
    .CE(\U0/U_ICON/U_CMD/iTARGET_CE ),
    .CLR(\U0/U_ICON/U_CMD/iSEL_n ),
    .D(\U0/U_ICON/U_CMD/iTARGET [10]),
    .Q(\U0/U_ICON/U_CMD/iTARGET [9])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_CMD/G_TARGET[10].I_NE0.U_TARGET  (
    .C(CONTROL0[0]),
    .CE(\U0/U_ICON/U_CMD/iTARGET_CE ),
    .CLR(\U0/U_ICON/U_CMD/iSEL_n ),
    .D(\U0/U_ICON/U_CMD/iTARGET [11]),
    .Q(\U0/U_ICON/U_CMD/iTARGET [10])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_CMD/G_TARGET[11].I_NE0.U_TARGET  (
    .C(CONTROL0[0]),
    .CE(\U0/U_ICON/U_CMD/iTARGET_CE ),
    .CLR(\U0/U_ICON/U_CMD/iSEL_n ),
    .D(\U0/U_ICON/iCORE_ID [0]),
    .Q(\U0/U_ICON/U_CMD/iTARGET [11])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_CMD/G_TARGET[12].I_NE0.U_TARGET  (
    .C(CONTROL0[0]),
    .CE(\U0/U_ICON/U_CMD/iTARGET_CE ),
    .CLR(\U0/U_ICON/U_CMD/iSEL_n ),
    .D(\U0/U_ICON/iCORE_ID [1]),
    .Q(\U0/U_ICON/iCORE_ID [0])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_CMD/G_TARGET[13].I_NE0.U_TARGET  (
    .C(CONTROL0[0]),
    .CE(\U0/U_ICON/U_CMD/iTARGET_CE ),
    .CLR(\U0/U_ICON/U_CMD/iSEL_n ),
    .D(\U0/U_ICON/iCORE_ID [2]),
    .Q(\U0/U_ICON/iCORE_ID [1])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_CMD/G_TARGET[14].I_NE0.U_TARGET  (
    .C(CONTROL0[0]),
    .CE(\U0/U_ICON/U_CMD/iTARGET_CE ),
    .CLR(\U0/U_ICON/U_CMD/iSEL_n ),
    .D(\U0/U_ICON/iCORE_ID [3]),
    .Q(\U0/U_ICON/iCORE_ID [2])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_CMD/G_TARGET[15].I_EQ0.U_TARGET  (
    .C(CONTROL0[0]),
    .CE(\U0/U_ICON/U_CMD/iTARGET_CE ),
    .CLR(\U0/U_ICON/U_CMD/iSEL_n ),
    .D(CONTROL0[1]),
    .Q(\U0/U_ICON/iCORE_ID [3])
  );
  FDRE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_STAT/U_STAT_CNT/G[5].U_FDRE  (
    .C(CONTROL0[0]),
    .CE(N1),
    .D(\U0/U_ICON/U_STAT/U_STAT_CNT/D [5]),
    .R(\U0/U_ICON/U_STAT/iSTATCMD_CE_n ),
    .Q(\U0/U_ICON/U_STAT/iSTAT_CNT [5])
  );
  FDRE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_STAT/U_STAT_CNT/G[4].U_FDRE  (
    .C(CONTROL0[0]),
    .CE(N1),
    .D(\U0/U_ICON/U_STAT/U_STAT_CNT/D [4]),
    .R(\U0/U_ICON/U_STAT/iSTATCMD_CE_n ),
    .Q(\U0/U_ICON/U_STAT/iSTAT_CNT [4])
  );
  FDRE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_STAT/U_STAT_CNT/G[3].U_FDRE  (
    .C(CONTROL0[0]),
    .CE(N1),
    .D(\U0/U_ICON/U_STAT/U_STAT_CNT/D [3]),
    .R(\U0/U_ICON/U_STAT/iSTATCMD_CE_n ),
    .Q(\U0/U_ICON/U_STAT/iSTAT_CNT [3])
  );
  FDRE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_STAT/U_STAT_CNT/G[2].U_FDRE  (
    .C(CONTROL0[0]),
    .CE(N1),
    .D(\U0/U_ICON/U_STAT/U_STAT_CNT/D [2]),
    .R(\U0/U_ICON/U_STAT/iSTATCMD_CE_n ),
    .Q(\U0/U_ICON/U_STAT/iSTAT_CNT [2])
  );
  FDRE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_STAT/U_STAT_CNT/G[1].U_FDRE  (
    .C(CONTROL0[0]),
    .CE(N1),
    .D(\U0/U_ICON/U_STAT/U_STAT_CNT/D [1]),
    .R(\U0/U_ICON/U_STAT/iSTATCMD_CE_n ),
    .Q(\U0/U_ICON/U_STAT/iSTAT_CNT [1])
  );
  FDRE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_STAT/U_STAT_CNT/G[0].U_FDRE  (
    .C(CONTROL0[0]),
    .CE(N1),
    .D(\U0/U_ICON/U_STAT/U_STAT_CNT/D [0]),
    .R(\U0/U_ICON/U_STAT/iSTATCMD_CE_n ),
    .Q(\U0/U_ICON/U_STAT/iSTAT_CNT [0])
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \U0/U_ICON/U_STAT/U_STAT_CNT/G[5].U_LUT  (
    .I0(\U0/U_ICON/U_STAT/iSTAT_CNT [5]),
    .O(\U0/U_ICON/U_STAT/U_STAT_CNT/S [5])
  );
  XORCY   \U0/U_ICON/U_STAT/U_STAT_CNT/G[5].U_XORCY  (
    .CI(\U0/U_ICON/U_STAT/U_STAT_CNT/CI [5]),
    .LI(\U0/U_ICON/U_STAT/U_STAT_CNT/S [5]),
    .O(\U0/U_ICON/U_STAT/U_STAT_CNT/D [5])
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \U0/U_ICON/U_STAT/U_STAT_CNT/G[4].U_LUT  (
    .I0(\U0/U_ICON/U_STAT/iSTAT_CNT [4]),
    .O(\U0/U_ICON/U_STAT/U_STAT_CNT/S [4])
  );
  MUXCY_L   \U0/U_ICON/U_STAT/U_STAT_CNT/G[4].GnH.U_MUXCY  (
    .CI(\U0/U_ICON/U_STAT/U_STAT_CNT/CI [4]),
    .DI(CONTROL0[2]),
    .S(\U0/U_ICON/U_STAT/U_STAT_CNT/S [4]),
    .LO(\U0/U_ICON/U_STAT/U_STAT_CNT/CI [5])
  );
  XORCY   \U0/U_ICON/U_STAT/U_STAT_CNT/G[4].U_XORCY  (
    .CI(\U0/U_ICON/U_STAT/U_STAT_CNT/CI [4]),
    .LI(\U0/U_ICON/U_STAT/U_STAT_CNT/S [4]),
    .O(\U0/U_ICON/U_STAT/U_STAT_CNT/D [4])
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \U0/U_ICON/U_STAT/U_STAT_CNT/G[3].U_LUT  (
    .I0(\U0/U_ICON/U_STAT/iSTAT_CNT [3]),
    .O(\U0/U_ICON/U_STAT/U_STAT_CNT/S [3])
  );
  MUXCY_L   \U0/U_ICON/U_STAT/U_STAT_CNT/G[3].GnH.U_MUXCY  (
    .CI(\U0/U_ICON/U_STAT/U_STAT_CNT/CI [3]),
    .DI(CONTROL0[2]),
    .S(\U0/U_ICON/U_STAT/U_STAT_CNT/S [3]),
    .LO(\U0/U_ICON/U_STAT/U_STAT_CNT/CI [4])
  );
  XORCY   \U0/U_ICON/U_STAT/U_STAT_CNT/G[3].U_XORCY  (
    .CI(\U0/U_ICON/U_STAT/U_STAT_CNT/CI [3]),
    .LI(\U0/U_ICON/U_STAT/U_STAT_CNT/S [3]),
    .O(\U0/U_ICON/U_STAT/U_STAT_CNT/D [3])
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \U0/U_ICON/U_STAT/U_STAT_CNT/G[2].U_LUT  (
    .I0(\U0/U_ICON/U_STAT/iSTAT_CNT [2]),
    .O(\U0/U_ICON/U_STAT/U_STAT_CNT/S [2])
  );
  MUXCY_L   \U0/U_ICON/U_STAT/U_STAT_CNT/G[2].GnH.U_MUXCY  (
    .CI(\U0/U_ICON/U_STAT/U_STAT_CNT/CI [2]),
    .DI(CONTROL0[2]),
    .S(\U0/U_ICON/U_STAT/U_STAT_CNT/S [2]),
    .LO(\U0/U_ICON/U_STAT/U_STAT_CNT/CI [3])
  );
  XORCY   \U0/U_ICON/U_STAT/U_STAT_CNT/G[2].U_XORCY  (
    .CI(\U0/U_ICON/U_STAT/U_STAT_CNT/CI [2]),
    .LI(\U0/U_ICON/U_STAT/U_STAT_CNT/S [2]),
    .O(\U0/U_ICON/U_STAT/U_STAT_CNT/D [2])
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \U0/U_ICON/U_STAT/U_STAT_CNT/G[1].U_LUT  (
    .I0(\U0/U_ICON/U_STAT/iSTAT_CNT [1]),
    .O(\U0/U_ICON/U_STAT/U_STAT_CNT/S [1])
  );
  MUXCY_L   \U0/U_ICON/U_STAT/U_STAT_CNT/G[1].GnH.U_MUXCY  (
    .CI(\U0/U_ICON/U_STAT/U_STAT_CNT/CI [1]),
    .DI(CONTROL0[2]),
    .S(\U0/U_ICON/U_STAT/U_STAT_CNT/S [1]),
    .LO(\U0/U_ICON/U_STAT/U_STAT_CNT/CI [2])
  );
  XORCY   \U0/U_ICON/U_STAT/U_STAT_CNT/G[1].U_XORCY  (
    .CI(\U0/U_ICON/U_STAT/U_STAT_CNT/CI [1]),
    .LI(\U0/U_ICON/U_STAT/U_STAT_CNT/S [1]),
    .O(\U0/U_ICON/U_STAT/U_STAT_CNT/D [1])
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \U0/U_ICON/U_STAT/U_STAT_CNT/G[0].U_LUT  (
    .I0(\U0/U_ICON/U_STAT/iSTAT_CNT [0]),
    .O(\U0/U_ICON/U_STAT/U_STAT_CNT/S [0])
  );
  MUXCY_L   \U0/U_ICON/U_STAT/U_STAT_CNT/G[0].GnH.U_MUXCY  (
    .CI(N1),
    .DI(CONTROL0[2]),
    .S(\U0/U_ICON/U_STAT/U_STAT_CNT/S [0]),
    .LO(\U0/U_ICON/U_STAT/U_STAT_CNT/CI [1])
  );
  XORCY   \U0/U_ICON/U_STAT/U_STAT_CNT/G[0].U_XORCY  (
    .CI(N1),
    .LI(\U0/U_ICON/U_STAT/U_STAT_CNT/S [0]),
    .O(\U0/U_ICON/U_STAT/U_STAT_CNT/D [0])
  );
  MUXF6   \U0/U_ICON/U_STAT/U_TDO_next  (
    .I0(\U0/U_ICON/U_STAT/iSTAT_LOW ),
    .I1(\U0/U_ICON/U_STAT/iSTAT_HIGH ),
    .S(\U0/U_ICON/U_STAT/iSTAT_CNT [5]),
    .O(\U0/U_ICON/U_STAT/iTDO_next )
  );
  MUXF5   \U0/U_ICON/U_STAT/U_STAT_LOW  (
    .I0(\U0/U_ICON/U_STAT/iSTAT [0]),
    .I1(\U0/U_ICON/U_STAT/iSTAT [1]),
    .S(\U0/U_ICON/U_STAT/iSTAT_CNT [4]),
    .O(\U0/U_ICON/U_STAT/iSTAT_LOW )
  );
  MUXF5   \U0/U_ICON/U_STAT/U_STAT_HIGH  (
    .I0(\U0/U_ICON/U_STAT/iSTAT [2]),
    .I1(\U0/U_ICON/U_STAT/iSTAT [3]),
    .S(\U0/U_ICON/U_STAT/iSTAT_CNT [4]),
    .O(\U0/U_ICON/U_STAT/iSTAT_HIGH )
  );
  LUT4 #(
    .INIT ( 16'h0101 ))
  \U0/U_ICON/U_STAT/F_STAT[0].U_STAT  (
    .I0(\U0/U_ICON/U_STAT/iSTAT_CNT [0]),
    .I1(\U0/U_ICON/U_STAT/iSTAT_CNT [1]),
    .I2(\U0/U_ICON/U_STAT/iSTAT_CNT [2]),
    .I3(\U0/U_ICON/U_STAT/iSTAT_CNT [3]),
    .O(\U0/U_ICON/U_STAT/iSTAT [0])
  );
  LUT4 #(
    .INIT ( 16'hC101 ))
  \U0/U_ICON/U_STAT/F_STAT[1].U_STAT  (
    .I0(\U0/U_ICON/U_STAT/iSTAT_CNT [0]),
    .I1(\U0/U_ICON/U_STAT/iSTAT_CNT [1]),
    .I2(\U0/U_ICON/U_STAT/iSTAT_CNT [2]),
    .I3(\U0/U_ICON/U_STAT/iSTAT_CNT [3]),
    .O(\U0/U_ICON/U_STAT/iSTAT [1])
  );
  LUT4 #(
    .INIT ( 16'h2100 ))
  \U0/U_ICON/U_STAT/F_STAT[2].U_STAT  (
    .I0(\U0/U_ICON/U_STAT/iSTAT_CNT [0]),
    .I1(\U0/U_ICON/U_STAT/iSTAT_CNT [1]),
    .I2(\U0/U_ICON/U_STAT/iSTAT_CNT [2]),
    .I3(\U0/U_ICON/U_STAT/iSTAT_CNT [3]),
    .O(\U0/U_ICON/U_STAT/iSTAT [2])
  );
  LUT4 #(
    .INIT ( 16'h1610 ))
  \U0/U_ICON/U_STAT/F_STAT[3].U_STAT  (
    .I0(\U0/U_ICON/U_STAT/iSTAT_CNT [0]),
    .I1(\U0/U_ICON/U_STAT/iSTAT_CNT [1]),
    .I2(\U0/U_ICON/U_STAT/iSTAT_CNT [2]),
    .I3(\U0/U_ICON/U_STAT/iSTAT_CNT [3]),
    .O(\U0/U_ICON/U_STAT/iSTAT [3])
  );
  INV   \U0/U_ICON/U_STAT/U_STATCMD_n  (
    .I(\U0/U_ICON/U_STAT/iSTATCMD_CE ),
    .O(\U0/U_ICON/U_STAT/iSTATCMD_CE_n )
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \U0/U_ICON/U_STAT/U_STATCMD  (
    .I0(\U0/U_ICON/U_STAT/iDATA_VALID ),
    .I1(\U0/U_ICON/iCOMMAND_SEL [0]),
    .I2(\U0/U_ICON/iCORE_ID_SEL[15] ),
    .I3(\U0/U_ICON/U_STAT/iCMD_GRP0_SEL ),
    .O(\U0/U_ICON/U_STAT/iSTATCMD_CE )
  );
  LUT2 #(
    .INIT ( 4'h1 ))
  \U0/U_ICON/U_STAT/U_CMDGRP0  (
    .I0(\U0/U_ICON/iCOMMAND_GRP [0]),
    .I1(\U0/U_ICON/iCOMMAND_GRP [1]),
    .O(\U0/U_ICON/U_STAT/iCMD_GRP0_SEL )
  );
  LUT2 #(
    .INIT ( 4'h8 ))
  \U0/U_ICON/U_STAT/U_DATA_VALID  (
    .I0(\U0/U_ICON/iSYNC ),
    .I1(\U0/iSHIFT_OUT ),
    .O(\U0/U_ICON/U_STAT/iDATA_VALID )
  );
  FDE #(
    .INIT ( 1'b0 ))
  \U0/U_ICON/U_STAT/U_TDO  (
    .C(CONTROL0[0]),
    .CE(N1),
    .D(\U0/U_ICON/U_STAT/iTDO_next ),
    .Q(\U0/U_ICON/iTDO_VEC [15])
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

    wire GSR;
    wire GTS;
    wire GWE;
    wire PRLD;
    tri1 p_up_tmp;
    tri (weak1, strong0) PLL_LOCKG = p_up_tmp;

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
