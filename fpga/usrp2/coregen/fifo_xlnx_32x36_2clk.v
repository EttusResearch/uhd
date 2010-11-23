////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1995-2010 Xilinx, Inc.  All rights reserved.
////////////////////////////////////////////////////////////////////////////////
//   ____  ____
//  /   /\/   /
// /___/  \  /    Vendor: Xilinx
// \   \   \/     Version: M.63c
//  \   \         Application: netgen
//  /   /         Filename: fifo_xlnx_32x36_2clk.v
// /___/   /\     Timestamp: Fri Oct 15 00:50:08 2010
// \   \  /  \ 
//  \___\/\___\
//             
// Command	: -intstyle ise -w -sim -ofmt verilog /tmp/_cg/fifo_xlnx_32x36_2clk.ngc /tmp/_cg/fifo_xlnx_32x36_2clk.v 
// Device	: 3s2000fg456-5
// Input file	: /tmp/_cg/fifo_xlnx_32x36_2clk.ngc
// Output file	: /tmp/_cg/fifo_xlnx_32x36_2clk.v
// # of Modules	: 1
// Design Name	: fifo_xlnx_32x36_2clk
// Xilinx        : /opt/Xilinx/12.2/ISE_DS/ISE/
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

module fifo_xlnx_32x36_2clk (
  rd_en, almost_full, prog_full, wr_en, full, empty, wr_clk, rst, rd_clk, dout, din
)/* synthesis syn_black_box syn_noprune=1 */;
  input rd_en;
  output almost_full;
  output prog_full;
  input wr_en;
  output full;
  output empty;
  input wr_clk;
  input rst;
  input rd_clk;
  output [35 : 0] dout;
  input [35 : 0] din;
  
  // synthesis translate_off
  
  wire \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/dout_i62_392 ;
  wire \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or0000156_391 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_mux000079_390 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_mux000063_389 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_mux000027_388 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/dout_i26_387 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_almost_full_i_mux000069_386 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwas.wsts/comp2 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_almost_full_i_mux000067_384 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_almost_full_i_mux000026_383 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/count_not0001 ;
  wire \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or000063_381 ;
  wire \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or0000158_380 ;
  wire \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or0000115_379 ;
  wire \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or000062_378 ;
  wire \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or000026_377 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/count_not0001 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/N11 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/N11 ;
  wire \BU2/N16 ;
  wire \BU2/N14 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_almost_full_i_mux0000 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_fb_i_370 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_mux0000 ;
  wire \BU2/U0/grf.rf/mem/dout_i_not0001 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N147 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N145 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N143 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N141 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N137 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N135 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N139 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N133 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N131 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N129 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N127 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N123 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N121 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N125 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N119 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N117 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N115 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N113 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N109 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N107 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N111 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N103 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N101 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N105 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N97 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N95 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N99 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N93 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N91 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N89 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N87 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N83 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N81 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N85 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N79 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N77 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N75 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N73 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N69 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N67 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N71 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N65 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N63 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N61 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N59 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N55 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N53 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N57 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N51 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N49 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N47 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N45 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N41 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N39 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N43 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N37 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N35 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N33 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N31 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N27 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N25 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N29 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N23 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N21 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N19 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N17 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N13 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N11 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N15 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N9 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N7 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/N5 ;
  wire \BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count2 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count1 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count3 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count4 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/prog_full_i_mux0003 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/prog_full_i_not0001 ;
  wire \BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ;
  wire \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_fb_176 ;
  wire \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_i_mux0000 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count2 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count1 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count3 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count4 ;
  wire \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0003 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0002 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0001 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0000 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0003 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0002 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0001 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0000 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0003_120 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0002 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0001 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0000 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0003_110 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0002 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0001 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0000 ;
  wire \BU2/U0/grf.rf/rstblk/wr_rst_comb ;
  wire \BU2/U0/grf.rf/rstblk/rd_rst_comb ;
  wire \BU2/U0/grf.rf/rstblk/wr_rst_asreg_95 ;
  wire \BU2/U0/grf.rf/rstblk/rd_rst_asreg_94 ;
  wire \BU2/U0/grf.rf/rstblk/rst_d1_93 ;
  wire \BU2/U0/grf.rf/rstblk/wr_rst_asreg_d2_92 ;
  wire \BU2/U0/grf.rf/rstblk/wr_rst_asreg_d1_91 ;
  wire \BU2/U0/grf.rf/rstblk/rd_rst_asreg_d2_90 ;
  wire \BU2/U0/grf.rf/rstblk/rd_rst_asreg_d1_89 ;
  wire \BU2/U0/grf.rf/rstblk/rst_d2_88 ;
  wire \BU2/U0/grf.rf/rstblk/RST_FULL_GEN_87 ;
  wire \BU2/U0/grf.rf/rstblk/rst_d3_86 ;
  wire \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_85 ;
  wire \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or0000 ;
  wire \BU2/N1 ;
  wire NLW_VCC_P_UNCONNECTED;
  wire NLW_GND_G_UNCONNECTED;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM72_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM71_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM70_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM69_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM67_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM66_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM68_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM65_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM64_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM63_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM62_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM60_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM59_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM61_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM58_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM57_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM56_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM55_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM53_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM52_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM54_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM50_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM49_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM51_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM47_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM46_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM48_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM45_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM44_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM43_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM42_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM40_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM39_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM41_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM38_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM37_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM36_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM35_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM33_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM32_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM34_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM31_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM30_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM29_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM28_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM26_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM25_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM27_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM24_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM23_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM22_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM21_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM19_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM18_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM20_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM17_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM16_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM15_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM14_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM12_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM11_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM13_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM10_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM9_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM8_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM7_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM5_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM4_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM6_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM3_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM2_SPO_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM1_SPO_UNCONNECTED ;
  wire [35 : 0] din_2;
  wire [35 : 0] dout_3;
  wire [35 : 0] \BU2/U0/grf.rf/mem/gdm.dm/dout_i ;
  wire [35 : 0] \BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 ;
  wire [4 : 0] \BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 ;
  wire [4 : 0] \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 ;
  wire [4 : 0] \BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 ;
  wire [4 : 0] \BU2/U0/grf.rf/gl0.wr/wpntr/count ;
  wire [5 : 4] \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/diff_pntr_pad ;
  wire [5 : 1] \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_lut ;
  wire [4 : 0] \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_cy ;
  wire [5 : 4] \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/diff_pntr_pad_add0000 ;
  wire [1 : 0] \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state ;
  wire [1 : 0] \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_mux0001 ;
  wire [4 : 0] \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 ;
  wire [4 : 0] \BU2/U0/grf.rf/gl0.rd/rpntr/count ;
  wire [4 : 0] \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc ;
  wire [4 : 0] \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc ;
  wire [4 : 0] \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 ;
  wire [4 : 0] \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg ;
  wire [4 : 0] \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 ;
  wire [4 : 0] \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg ;
  wire [4 : 0] \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin ;
  wire [4 : 0] \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin ;
  wire [1 : 0] \BU2/U0/grf.rf/rstblk/wr_rst_reg ;
  wire [2 : 0] \BU2/U0/grf.rf/rstblk/rd_rst_reg ;
  wire [0 : 0] \BU2/rd_data_count ;
  assign
    dout[35] = dout_3[35],
    dout[34] = dout_3[34],
    dout[33] = dout_3[33],
    dout[32] = dout_3[32],
    dout[31] = dout_3[31],
    dout[30] = dout_3[30],
    dout[29] = dout_3[29],
    dout[28] = dout_3[28],
    dout[27] = dout_3[27],
    dout[26] = dout_3[26],
    dout[25] = dout_3[25],
    dout[24] = dout_3[24],
    dout[23] = dout_3[23],
    dout[22] = dout_3[22],
    dout[21] = dout_3[21],
    dout[20] = dout_3[20],
    dout[19] = dout_3[19],
    dout[18] = dout_3[18],
    dout[17] = dout_3[17],
    dout[16] = dout_3[16],
    dout[15] = dout_3[15],
    dout[14] = dout_3[14],
    dout[13] = dout_3[13],
    dout[12] = dout_3[12],
    dout[11] = dout_3[11],
    dout[10] = dout_3[10],
    dout[9] = dout_3[9],
    dout[8] = dout_3[8],
    dout[7] = dout_3[7],
    dout[6] = dout_3[6],
    dout[5] = dout_3[5],
    dout[4] = dout_3[4],
    dout[3] = dout_3[3],
    dout[2] = dout_3[2],
    dout[1] = dout_3[1],
    dout[0] = dout_3[0],
    din_2[35] = din[35],
    din_2[34] = din[34],
    din_2[33] = din[33],
    din_2[32] = din[32],
    din_2[31] = din[31],
    din_2[30] = din[30],
    din_2[29] = din[29],
    din_2[28] = din[28],
    din_2[27] = din[27],
    din_2[26] = din[26],
    din_2[25] = din[25],
    din_2[24] = din[24],
    din_2[23] = din[23],
    din_2[22] = din[22],
    din_2[21] = din[21],
    din_2[20] = din[20],
    din_2[19] = din[19],
    din_2[18] = din[18],
    din_2[17] = din[17],
    din_2[16] = din[16],
    din_2[15] = din[15],
    din_2[14] = din[14],
    din_2[13] = din[13],
    din_2[12] = din[12],
    din_2[11] = din[11],
    din_2[10] = din[10],
    din_2[9] = din[9],
    din_2[8] = din[8],
    din_2[7] = din[7],
    din_2[6] = din[6],
    din_2[5] = din[5],
    din_2[4] = din[4],
    din_2[3] = din[3],
    din_2[2] = din[2],
    din_2[1] = din[1],
    din_2[0] = din[0];
  VCC   VCC_0 (
    .P(NLW_VCC_P_UNCONNECTED)
  );
  GND   GND_1 (
    .G(NLW_GND_G_UNCONNECTED)
  );
  LUT3_L #(
    .INIT ( 8'h90 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or000063  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [0]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .I2(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or000062_378 ),
    .LO(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or000063_381 )
  );
  LUT4_L #(
    .INIT ( 16'h9000 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_mux000079  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [0]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [0]),
    .I2(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_mux000063_389 ),
    .I3(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_mux000027_388 ),
    .LO(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_mux000079_390 )
  );
  LUT4_L #(
    .INIT ( 16'h9000 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_almost_full_i_mux000069  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [0]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count [0]),
    .I2(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_almost_full_i_mux000067_384 ),
    .I3(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .LO(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_almost_full_i_mux000069_386 )
  );
  LUT4_L #(
    .INIT ( 16'h8421 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/dout_i62  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [2]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [3]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [2]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [3]),
    .LO(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/dout_i62_392 )
  );
  LUT4_L #(
    .INIT ( 16'h8241 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or0000156  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [1]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [2]),
    .I2(\BU2/U0/grf.rf/gl0.rd/rpntr/count [2]),
    .I3(\BU2/U0/grf.rf/gl0.rd/rpntr/count [1]),
    .LO(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or0000156_391 )
  );
  LUT3_L #(
    .INIT ( 8'h7F ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<3>111  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [0]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count [1]),
    .I2(\BU2/U0/grf.rf/gl0.rd/rpntr/count [2]),
    .LO(\BU2/U0/grf.rf/gl0.rd/rpntr/N11 )
  );
  LUT3_L #(
    .INIT ( 8'h7F ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<3>111  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [0]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count [1]),
    .I2(\BU2/U0/grf.rf/gl0.wr/wpntr/count [2]),
    .LO(\BU2/U0/grf.rf/gl0.wr/wpntr/N11 )
  );
  LUT2_L #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0003_SW0  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [3]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [2]),
    .LO(\BU2/N16 )
  );
  LUT2_L #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0003_SW0  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [3]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [2]),
    .LO(\BU2/N14 )
  );
  INV   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<0>11_INV_0  (
    .I(\BU2/U0/grf.rf/gl0.rd/rpntr/count [0]),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count )
  );
  INV   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<0>11_INV_0  (
    .I(\BU2/U0/grf.rf/gl0.wr/wpntr/count [0]),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count )
  );
  LUT2 #(
    .INIT ( 4'h2 ))
  \BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_1  (
    .I0(wr_en),
    .I1(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_fb_i_370 ),
    .O(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 )
  );
  LUT4 #(
    .INIT ( 16'h2333 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_1  (
    .I0(rd_en),
    .I1(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_85 ),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state [1]),
    .I3(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state [0]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 )
  );
  LUT4 #(
    .INIT ( 16'h6AAA ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<3>12  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [3]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count [0]),
    .I2(\BU2/U0/grf.rf/gl0.rd/rpntr/count [1]),
    .I3(\BU2/U0/grf.rf/gl0.rd/rpntr/count [2]),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count3 )
  );
  LUT4 #(
    .INIT ( 16'h6AAA ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<3>12  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [3]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count [0]),
    .I2(\BU2/U0/grf.rf/gl0.wr/wpntr/count [1]),
    .I3(\BU2/U0/grf.rf/gl0.wr/wpntr/count [2]),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count3 )
  );
  LUT3 #(
    .INIT ( 8'h08 ))
  \BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1  (
    .I0(wr_en),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [4]),
    .I2(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_fb_i_370 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 )
  );
  LUT3 #(
    .INIT ( 8'h10 ))
  \BU2/U0/grf.rf/mem/gdm.dm/write_ctrl  (
    .I0(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_fb_i_370 ),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [4]),
    .I2(wr_en),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 )
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_lut<5>  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [4]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [4]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_lut [5])
  );
  LUT4 #(
    .INIT ( 16'h9000 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/dout_i78  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [0]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [0]),
    .I2(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/dout_i62_392 ),
    .I3(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/dout_i26_387 ),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/comp2 )
  );
  LUT4 #(
    .INIT ( 16'h9000 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or0000158  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [0]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count [0]),
    .I2(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or0000156_391 ),
    .I3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_not0001 ),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or0000158_380 )
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_lut<4>  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [3]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [3]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_lut [4])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_lut<3>  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [2]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [2]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_lut [3])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_lut<2>  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [1]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [1]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_lut [2])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_lut<1>  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [0]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [0]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_lut [1])
  );
  LUT4 #(
    .INIT ( 16'h5450 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_mux0000105  (
    .I0(\BU2/U0/grf.rf/rstblk/RST_FULL_GEN_87 ),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_not0001 ),
    .I2(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_mux000079_390 ),
    .I3(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/comp2 ),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_mux0000 )
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_mux000063  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [3]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [3]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [2]),
    .I3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [2]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_mux000063_389 )
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_mux000027  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [4]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [4]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [1]),
    .I3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [1]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_mux000027_388 )
  );
  LUT4 #(
    .INIT ( 16'h8421 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/dout_i26  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [1]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [4]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [1]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [4]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/dout_i26_387 )
  );
  LUT4 #(
    .INIT ( 16'h5450 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_almost_full_i_mux0000107  (
    .I0(\BU2/U0/grf.rf/rstblk/RST_FULL_GEN_87 ),
    .I1(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_almost_full_i_mux000026_383 ),
    .I2(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/comp2 ),
    .I3(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_almost_full_i_mux000069_386 ),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_almost_full_i_mux0000 )
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_almost_full_i_mux000067  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [2]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count [2]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [1]),
    .I3(\BU2/U0/grf.rf/gl0.wr/wpntr/count [1]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_almost_full_i_mux000067_384 )
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_almost_full_i_mux000026  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [4]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count [4]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [3]),
    .I3(\BU2/U0/grf.rf/gl0.wr/wpntr/count [3]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_almost_full_i_mux000026_383 )
  );
  LUT2 #(
    .INIT ( 4'h2 ))
  \BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1  (
    .I0(wr_en),
    .I1(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_fb_i_370 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/count_not0001 )
  );
  LUT4 #(
    .INIT ( 16'hECA0 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or0000183  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or0000115_379 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or000026_377 ),
    .I2(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or0000158_380 ),
    .I3(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or000063_381 ),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or0000 )
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or0000115  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [4]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [4]),
    .I2(\BU2/U0/grf.rf/gl0.rd/rpntr/count [3]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [3]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or0000115_379 )
  );
  LUT4 #(
    .INIT ( 16'h8421 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or000062  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [2]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [3]),
    .I2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .I3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or000062_378 )
  );
  LUT4 #(
    .INIT ( 16'h8241 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or000026  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [1]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [4]),
    .I2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or000026_377 )
  );
  LUT4 #(
    .INIT ( 16'h2333 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21  (
    .I0(rd_en),
    .I1(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_85 ),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state [1]),
    .I3(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state [0]),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/count_not0001 )
  );
  LUT3 #(
    .INIT ( 8'hA6 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<4>11  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [4]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count [3]),
    .I2(\BU2/U0/grf.rf/gl0.rd/rpntr/N11 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count4 )
  );
  LUT3 #(
    .INIT ( 8'hA6 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<4>11  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [4]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count [3]),
    .I2(\BU2/U0/grf.rf/gl0.wr/wpntr/N11 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count4 )
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX11  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N5 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N7 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [0])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX1011  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N45 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N47 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [10])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX1111  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N9 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N11 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [1])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX11111  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N49 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N51 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [11])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX1211  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N53 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N55 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [12])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX1311  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N57 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N59 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [13])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX1411  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N61 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N63 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [14])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX1511  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N65 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N67 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [15])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX1611  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N69 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N71 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [16])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX1711  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N73 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N75 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [17])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX1811  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N77 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N79 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [18])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX1911  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N81 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N83 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [19])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX2011  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N85 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N87 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [20])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX2111  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N13 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N15 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [2])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX21111  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N89 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N91 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [21])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX2211  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N93 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N95 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [22])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX2311  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N97 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N99 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [23])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX2411  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N101 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N103 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [24])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX2511  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N105 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N107 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [25])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX2611  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N109 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N111 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [26])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX2711  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N113 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N115 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [27])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX2811  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N117 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N119 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [28])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX2911  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N121 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N123 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [29])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX3011  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N125 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N127 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [30])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX3111  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N17 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N19 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [3])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX31111  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N129 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N131 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [31])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX3211  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N133 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N135 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [32])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX3311  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N137 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N139 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [33])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX3411  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N141 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N143 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [34])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX3511  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N145 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N147 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [35])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX411  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N21 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N23 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [4])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX511  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N25 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N27 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [5])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX611  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N29 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N31 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [6])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX711  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N33 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N35 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [7])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX811  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N37 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N39 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [8])
  );
  LUT3 #(
    .INIT ( 8'hE4 ))
  \BU2/U0/grf.rf/mem/gdm.dm/inst_LPM_MUX911  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/mem/gdm.dm/N41 ),
    .I2(\BU2/U0/grf.rf/mem/gdm.dm/N43 ),
    .O(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [9])
  );
  LUT4 #(
    .INIT ( 16'h6996 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0003  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [1]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [0]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [4]),
    .I3(\BU2/N16 ),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0003_110 )
  );
  LUT4 #(
    .INIT ( 16'h6996 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0003  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [1]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [0]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [4]),
    .I3(\BU2/N14 ),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0003_120 )
  );
  LUT3 #(
    .INIT ( 8'hA2 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_REGOUT_EN11  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state [1]),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state [0]),
    .I2(rd_en),
    .O(\BU2/U0/grf.rf/mem/dout_i_not0001 )
  );
  LUT2 #(
    .INIT ( 4'hD ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_almost_full_i_not00011  (
    .I0(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_fb_i_370 ),
    .I1(\BU2/U0/grf.rf/rstblk/RST_FULL_GEN_87 ),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/prog_full_i_not0001 )
  );
  LUT4 #(
    .INIT ( 16'h8E8A ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_i_mux00001  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_fb_176 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state [0]),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state [1]),
    .I3(rd_en),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_i_mux0000 )
  );
  LUT4 #(
    .INIT ( 16'h6996 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor00021  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [2]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [1]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [4]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [3]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0002 )
  );
  LUT4 #(
    .INIT ( 16'h6996 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor00021  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [2]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [1]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [4]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [3]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0002 )
  );
  LUT4 #(
    .INIT ( 16'h40FF ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_mux0001<0>1  (
    .I0(rd_en),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state [0]),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state [1]),
    .I3(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_85 ),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_mux0001 [0])
  );
  LUT3 #(
    .INIT ( 8'h6A ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<2>11  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [2]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count [0]),
    .I2(\BU2/U0/grf.rf/gl0.rd/rpntr/count [1]),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count2 )
  );
  LUT3 #(
    .INIT ( 8'h6A ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<2>11  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [2]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count [0]),
    .I2(\BU2/U0/grf.rf/gl0.wr/wpntr/count [1]),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count2 )
  );
  LUT3 #(
    .INIT ( 8'h96 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor00011  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [4]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [3]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [2]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0001 )
  );
  LUT3 #(
    .INIT ( 8'h96 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor00011  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [4]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [3]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [2]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0001 )
  );
  LUT3 #(
    .INIT ( 8'hF2 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_mux0001<1>1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state [0]),
    .I1(rd_en),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state [1]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_mux0001 [1])
  );
  LUT3 #(
    .INIT ( 8'h08 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/prog_full_i_mux00031  (
    .I0(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/diff_pntr_pad [4]),
    .I1(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/diff_pntr_pad [5]),
    .I2(\BU2/U0/grf.rf/rstblk/RST_FULL_GEN_87 ),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/prog_full_i_mux0003 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_rd_pntr_gc_xor0000_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0000 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_rd_pntr_gc_xor0001_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0001 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_rd_pntr_gc_xor0002_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0002 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_rd_pntr_gc_xor0003_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0003 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_wr_pntr_gc_xor0000_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [4]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0000 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_wr_pntr_gc_xor0001_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0001 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_wr_pntr_gc_xor0002_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0002 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_wr_pntr_gc_xor0003_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0003 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<1>11  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [0]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count [1]),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count1 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<1>11  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [1]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count [0]),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count1 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor00001  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [3]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [4]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0000 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor00001  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [3]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [4]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0000 )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \BU2/U0/grf.rf/rstblk/rd_rst_comb1  (
    .I0(\BU2/U0/grf.rf/rstblk/rd_rst_asreg_d2_90 ),
    .I1(\BU2/U0/grf.rf/rstblk/rd_rst_asreg_94 ),
    .O(\BU2/U0/grf.rf/rstblk/rd_rst_comb )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \BU2/U0/grf.rf/rstblk/wr_rst_comb1  (
    .I0(\BU2/U0/grf.rf/rstblk/wr_rst_asreg_d2_92 ),
    .I1(\BU2/U0/grf.rf/rstblk/wr_rst_asreg_95 ),
    .O(\BU2/U0/grf.rf/rstblk/wr_rst_comb )
  );
  FDPE #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_almost_full_i  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/prog_full_i_not0001 ),
    .D(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_almost_full_i_mux0000 ),
    .PRE(\BU2/U0/grf.rf/rstblk/rst_d2_88 ),
    .Q(almost_full)
  );
  FDP #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i  (
    .C(wr_clk),
    .D(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_mux0000 ),
    .PRE(\BU2/U0/grf.rf/rstblk/rst_d2_88 ),
    .Q(full)
  );
  FDP #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_fb_i  (
    .C(wr_clk),
    .D(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_mux0000 ),
    .PRE(\BU2/U0/grf.rf/rstblk/rst_d2_88 ),
    .Q(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_fb_i_370 )
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_0  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [0]),
    .Q(dout_3[0])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_1  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [1]),
    .Q(dout_3[1])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_2  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [2]),
    .Q(dout_3[2])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_3  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [3]),
    .Q(dout_3[3])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_4  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [4]),
    .Q(dout_3[4])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_5  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [5]),
    .Q(dout_3[5])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_6  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [6]),
    .Q(dout_3[6])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_7  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [7]),
    .Q(dout_3[7])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_8  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [8]),
    .Q(dout_3[8])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_9  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [9]),
    .Q(dout_3[9])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_10  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [10]),
    .Q(dout_3[10])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_11  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [11]),
    .Q(dout_3[11])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_12  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [12]),
    .Q(dout_3[12])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_13  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [13]),
    .Q(dout_3[13])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_14  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [14]),
    .Q(dout_3[14])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_15  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [15]),
    .Q(dout_3[15])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_16  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [16]),
    .Q(dout_3[16])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_17  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [17]),
    .Q(dout_3[17])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_18  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [18]),
    .Q(dout_3[18])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_19  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [19]),
    .Q(dout_3[19])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_20  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [20]),
    .Q(dout_3[20])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_21  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [21]),
    .Q(dout_3[21])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_22  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [22]),
    .Q(dout_3[22])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_23  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [23]),
    .Q(dout_3[23])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_24  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [24]),
    .Q(dout_3[24])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_25  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [25]),
    .Q(dout_3[25])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_26  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [26]),
    .Q(dout_3[26])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_27  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [27]),
    .Q(dout_3[27])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_28  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [28]),
    .Q(dout_3[28])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_29  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [29]),
    .Q(dout_3[29])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_30  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [30]),
    .Q(dout_3[30])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_31  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [31]),
    .Q(dout_3[31])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_32  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [32]),
    .Q(dout_3[32])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_33  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [33]),
    .Q(dout_3[33])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_34  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [34]),
    .Q(dout_3[34])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_35  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/mem/dout_i_not0001 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [35]),
    .Q(dout_3[35])
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM72  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[35]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM72_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N147 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM71  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[35]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM71_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N145 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM70  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[34]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM70_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N143 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM69  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[34]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM69_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N141 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM67  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[33]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM67_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N137 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM66  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[32]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM66_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N135 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM68  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[33]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM68_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N139 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM65  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[32]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM65_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N133 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM64  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[31]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM64_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N131 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM63  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[31]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM63_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N129 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM62  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[30]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM62_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N127 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM60  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[29]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM60_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N123 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM59  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[29]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM59_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N121 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM61  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[30]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM61_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N125 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM58  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[28]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM58_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N119 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM57  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[28]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM57_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N117 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM56  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[27]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM56_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N115 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM55  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[27]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM55_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N113 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM53  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[26]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM53_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N109 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM52  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[25]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM52_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N107 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM54  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[26]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM54_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N111 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM50  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[24]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM50_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N103 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM49  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[24]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM49_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N101 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM51  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[25]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM51_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N105 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM47  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[23]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM47_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N97 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM46  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[22]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM46_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N95 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM48  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[23]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM48_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N99 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM45  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[22]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM45_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N93 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM44  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[21]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM44_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N91 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM43  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[21]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM43_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N89 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM42  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[20]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM42_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N87 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM40  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[19]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM40_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N83 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM39  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[19]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM39_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N81 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM41  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[20]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM41_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N85 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM38  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[18]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM38_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N79 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM37  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[18]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM37_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N77 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM36  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[17]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM36_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N75 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM35  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[17]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM35_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N73 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM33  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[16]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM33_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N69 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM32  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[15]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM32_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N67 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM34  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[16]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM34_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N71 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM31  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[15]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM31_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N65 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM30  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[14]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM30_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N63 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM29  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[14]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM29_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N61 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM28  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[13]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM28_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N59 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM26  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[12]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM26_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N55 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM25  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[12]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM25_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N53 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM27  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[13]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM27_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N57 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM24  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[11]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM24_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N51 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM23  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[11]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM23_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N49 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM22  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[10]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM22_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N47 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM21  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[10]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM21_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N45 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM19  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[9]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM19_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N41 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM18  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[8]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM18_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N39 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM20  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[9]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM20_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N43 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM17  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[8]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM17_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N37 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM16  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[7]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM16_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N35 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM15  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[7]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM15_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N33 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM14  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[6]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM14_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N31 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM12  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[5]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM12_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N27 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM11  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[5]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM11_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N25 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM13  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[6]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM13_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N29 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM10  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[4]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM10_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N23 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM9  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[4]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM9_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N21 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM8  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[3]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM8_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N19 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM7  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[3]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM7_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N17 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM5  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[2]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM5_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N13 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM4  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[1]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM4_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N11 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM6  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[2]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM6_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N15 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM3  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[1]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM3_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N9 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM2  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[0]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl1_296 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM2_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N7 )
  );
  RAM16X1D   \BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM1  (
    .A0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0]),
    .A1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1]),
    .A2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2]),
    .A3(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3]),
    .D(din_2[0]),
    .DPRA0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .DPRA1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .DPRA2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .DPRA3(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .WCLK(wr_clk),
    .WE(\BU2/U0/grf.rf/mem/gdm.dm/write_ctrl_294 ),
    .SPO(\NLW_BU2/U0/grf.rf/mem/gdm.dm/Mram_RAM1_SPO_UNCONNECTED ),
    .DPO(\BU2/U0/grf.rf/mem/gdm.dm/N5 )
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_35  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [35]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [35])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_34  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [34]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [34])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_33  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [33]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [33])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_32  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [32]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [32])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_31  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [31]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [31])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_30  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [30]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [30])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_29  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [29]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [29])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_28  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [28]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [28])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_27  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [27]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [27])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_26  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [26]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [26])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_25  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [25]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [25])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_24  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [24]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [24])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_23  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [23]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [23])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_22  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [22]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [22])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_21  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [21]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [21])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_20  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [20]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [20])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_19  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [19]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [19])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_18  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [18]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [18])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_17  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [17]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [17])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_16  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [16]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [16])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_15  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [15]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [15])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_14  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [14]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [14])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_13  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [13]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [13])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_12  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [12]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [12])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_11  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [11]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [11])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_10  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [10]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [10])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_9  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [9]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [9])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_8  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [8]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [8])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_7  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [7]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [7])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_6  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [6]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [6])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_5  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [5]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [5])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_4  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [4]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [4])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_3  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [3]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [3])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_2  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [2]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [2])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_1  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [1]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [1])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/gdm.dm/dout_i_0  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/gdm.dm/_varindex0000 [0]),
    .Q(\BU2/U0/grf.rf/mem/gdm.dm/dout_i [0])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d3_0  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [0]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [0])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d3_1  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [1]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [1])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d3_2  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [2]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [2])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d3_3  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [3]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [3])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d3_4  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [4]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [4])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2_4  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [4]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [4])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2_3  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [3]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [3])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2_1  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [1]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [1])
  );
  FDPE #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2_0  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [0]),
    .PRE(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [0])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2_2  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [2]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [2])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d1_4  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count [4]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [4])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d1_3  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count [3]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [3])
  );
  FDPE #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d1_1  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count [1]),
    .PRE(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [1])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d1_0  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count [0]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [0])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d1_2  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count [2]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [2])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_2  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count2 ),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count [2])
  );
  FDPE #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_0  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count ),
    .PRE(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count [0])
  );
  FDPE #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_1  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count1 ),
    .PRE(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count [1])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_3  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count3 ),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count [3])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_4  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count4 ),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count [4])
  );
  FDPE #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/prog_full_i  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/prog_full_i_not0001 ),
    .D(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/prog_full_i_mux0003 ),
    .PRE(\BU2/U0/grf.rf/rstblk/rst_d2_88 ),
    .Q(prog_full)
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/diff_pntr_pad_4  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/diff_pntr_pad_add0000 [4]),
    .Q(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/diff_pntr_pad [4])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/diff_pntr_pad_5  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/diff_pntr_pad_add0000 [5]),
    .Q(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/diff_pntr_pad [5])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_cy<0>  (
    .CI(\BU2/N1 ),
    .DI(\BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1_197 ),
    .S(\BU2/rd_data_count [0]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_cy [0])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_cy<1>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_cy [0]),
    .DI(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_lut [1]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_cy [1])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_cy<2>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_cy [1]),
    .DI(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [1]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_lut [2]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_cy [2])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_cy<3>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_cy [2]),
    .DI(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [2]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_lut [3]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_cy [3])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_cy<4>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_cy [3]),
    .DI(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [3]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_lut [4]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_cy [4])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_xor<4>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_cy [3]),
    .LI(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_lut [4]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/diff_pntr_pad_add0000 [4])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_xor<5>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_cy [4]),
    .LI(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/Madd_diff_pntr_pad_add0000_lut [5]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.gpf.wrpf/diff_pntr_pad_add0000 [5])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_0  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_mux0001 [1]),
    .Q(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state [0])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_1  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_mux0001 [0]),
    .Q(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state [1])
  );
  FDP #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_i  (
    .C(rd_clk),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_i_mux0000 ),
    .PRE(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .Q(empty)
  );
  FDP #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_fb  (
    .C(rd_clk),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_i_mux0000 ),
    .PRE(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .Q(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_fb_176 )
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1_0  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/count [0]),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1_1  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/count [1]),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1_2  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/count [2]),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1_3  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/count [3]),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1_4  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/count [4]),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_2  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count2 ),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count [2])
  );
  FDPE #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_0  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count ),
    .PRE(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count [0])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_1  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count1 ),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count [1])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_3  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count3 ),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count [3])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_4  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/Mmux_RAM_RD_EN_FWFT21_160 ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count4 ),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count [4])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_0  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0003 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [0])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_1  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0002 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [1])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_2  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0001 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [2])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_3  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0000 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [3])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_4  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d3 [4]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [4])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_0  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0003 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [0])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_1  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0002 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [1])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_2  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0001 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [2])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_3  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0000 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [3])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_4  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [4])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_0  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [0]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [0])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_1  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [1]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [1])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_2  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [2]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [2])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_3  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [3]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [3])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_4  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [4]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [4])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_0  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [0]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [0])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_1  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [1]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [1])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_2  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [2]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [2])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_3  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [3]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [3])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_4  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [4]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [4])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1_0  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [0]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [0])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1_1  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [1]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [1])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1_2  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [2]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [2])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1_3  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [3]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [3])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1_4  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [4]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [4])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1_0  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [0]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [0])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1_1  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [1]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [1])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1_2  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [2]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [2])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1_3  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [3]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [3])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1_4  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [4]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [4])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_0  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0003_120 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [0])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_1  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0002 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [1])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_2  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0001 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [2])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_3  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0000 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [3])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_4  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [4]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [4])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_0  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0003_110 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [0])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_1  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0002 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [1])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_2  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0001 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [2])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_3  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0000 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [3])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_4  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [4]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [4])
  );
  FDP #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/rstblk/wr_rst_reg_0  (
    .C(wr_clk),
    .D(\BU2/rd_data_count [0]),
    .PRE(\BU2/U0/grf.rf/rstblk/wr_rst_comb ),
    .Q(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0])
  );
  FDP #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/rstblk/wr_rst_reg_1  (
    .C(wr_clk),
    .D(\BU2/rd_data_count [0]),
    .PRE(\BU2/U0/grf.rf/rstblk/wr_rst_comb ),
    .Q(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1])
  );
  FDP #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/rstblk/rd_rst_reg_0  (
    .C(rd_clk),
    .D(\BU2/rd_data_count [0]),
    .PRE(\BU2/U0/grf.rf/rstblk/rd_rst_comb ),
    .Q(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0])
  );
  FDP #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/rstblk/rd_rst_reg_1  (
    .C(rd_clk),
    .D(\BU2/rd_data_count [0]),
    .PRE(\BU2/U0/grf.rf/rstblk/rd_rst_comb ),
    .Q(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1])
  );
  FDP #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/rstblk/rd_rst_reg_2  (
    .C(rd_clk),
    .D(\BU2/rd_data_count [0]),
    .PRE(\BU2/U0/grf.rf/rstblk/rd_rst_comb ),
    .Q(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2])
  );
  FDP #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/rstblk/rst_d1  (
    .C(wr_clk),
    .D(\BU2/rd_data_count [0]),
    .PRE(rst),
    .Q(\BU2/U0/grf.rf/rstblk/rst_d1_93 )
  );
  FDPE   \BU2/U0/grf.rf/rstblk/rd_rst_asreg  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/rstblk/rd_rst_asreg_d1_89 ),
    .D(\BU2/rd_data_count [0]),
    .PRE(rst),
    .Q(\BU2/U0/grf.rf/rstblk/rd_rst_asreg_94 )
  );
  FD #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/rstblk/wr_rst_asreg_d1  (
    .C(wr_clk),
    .D(\BU2/U0/grf.rf/rstblk/wr_rst_asreg_95 ),
    .Q(\BU2/U0/grf.rf/rstblk/wr_rst_asreg_d1_91 )
  );
  FDPE   \BU2/U0/grf.rf/rstblk/wr_rst_asreg  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/rstblk/wr_rst_asreg_d1_91 ),
    .D(\BU2/rd_data_count [0]),
    .PRE(rst),
    .Q(\BU2/U0/grf.rf/rstblk/wr_rst_asreg_95 )
  );
  FD #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/rstblk/rd_rst_asreg_d1  (
    .C(rd_clk),
    .D(\BU2/U0/grf.rf/rstblk/rd_rst_asreg_94 ),
    .Q(\BU2/U0/grf.rf/rstblk/rd_rst_asreg_d1_89 )
  );
  FDP #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/rstblk/rst_d2  (
    .C(wr_clk),
    .D(\BU2/U0/grf.rf/rstblk/rst_d1_93 ),
    .PRE(rst),
    .Q(\BU2/U0/grf.rf/rstblk/rst_d2_88 )
  );
  FD #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/rstblk/wr_rst_asreg_d2  (
    .C(wr_clk),
    .D(\BU2/U0/grf.rf/rstblk/wr_rst_asreg_d1_91 ),
    .Q(\BU2/U0/grf.rf/rstblk/wr_rst_asreg_d2_92 )
  );
  FD #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/rstblk/rd_rst_asreg_d2  (
    .C(rd_clk),
    .D(\BU2/U0/grf.rf/rstblk/rd_rst_asreg_d1_89 ),
    .Q(\BU2/U0/grf.rf/rstblk/rd_rst_asreg_d2_90 )
  );
  FDP #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/rstblk/rst_d3  (
    .C(wr_clk),
    .D(\BU2/U0/grf.rf/rstblk/rst_d2_88 ),
    .PRE(rst),
    .Q(\BU2/U0/grf.rf/rstblk/rst_d3_86 )
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/rstblk/RST_FULL_GEN  (
    .C(wr_clk),
    .CLR(rst),
    .D(\BU2/U0/grf.rf/rstblk/rst_d3_86 ),
    .Q(\BU2/U0/grf.rf/rstblk/RST_FULL_GEN_87 )
  );
  FDP #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i  (
    .C(rd_clk),
    .D(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or0000 ),
    .PRE(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .Q(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_85 )
  );
  VCC   \BU2/XST_VCC  (
    .P(\BU2/N1 )
  );
  GND   \BU2/XST_GND  (
    .G(\BU2/rd_data_count [0])
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
