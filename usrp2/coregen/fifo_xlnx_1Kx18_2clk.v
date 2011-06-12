////////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1995-2008 Xilinx, Inc.  All rights reserved.
////////////////////////////////////////////////////////////////////////////////
//   ____  ____
//  /   /\/   /
// /___/  \  /    Vendor: Xilinx
// \   \   \/     Version: K.39
//  \   \         Application: netgen
//  /   /         Filename: fifo_xlnx_1Kx18_2clk.v
// /___/   /\     Timestamp: Fri Jun 10 16:12:12 2011
// \   \  /  \ 
//  \___\/\___\
//             
// Command	: -intstyle ise -w -sim -ofmt verilog /tmp/_cg/fifo_xlnx_1Kx18_2clk.ngc /tmp/_cg/fifo_xlnx_1Kx18_2clk.v 
// Device	: 3s2000fg456-5
// Input file	: /tmp/_cg/fifo_xlnx_1Kx18_2clk.ngc
// Output file	: /tmp/_cg/fifo_xlnx_1Kx18_2clk.v
// # of Modules	: 1
// Design Name	: fifo_xlnx_1Kx18_2clk
// Xilinx        : /opt/Xilinx/10.1/ISE
//             
// Purpose:    
//     This verilog netlist is a verification model and uses simulation 
//     primitives which may not represent the true implementation of the 
//     device, however the netlist is functionally correct and should not 
//     be modified. This file cannot be synthesized and should only be used 
//     with supported simulation tools.
//             
// Reference:  
//     Development System Reference Guide, Chapter 23 and Synthesis and Simulation Design Guide, Chapter 6
//             
////////////////////////////////////////////////////////////////////////////////

`timescale 1 ns/1 ps

module fifo_xlnx_1Kx18_2clk (
  rd_en, wr_en, full, empty, wr_clk, rst, rd_clk, wr_data_count, rd_data_count, dout, din
);
  input rd_en;
  input wr_en;
  output full;
  output empty;
  input wr_clk;
  input rst;
  input rd_clk;
  output [10 : 0] wr_data_count;
  output [10 : 0] rd_data_count;
  output [17 : 0] dout;
  input [17 : 0] din;
  
  // synthesis translate_off
  
  wire \BU2/U0/grf.rf/gl0.rd/ram_valid_fwft ;
  wire \BU2/N39 ;
  wire \BU2/N31 ;
  wire \BU2/N37 ;
  wire \BU2/N27 ;
  wire \BU2/N25 ;
  wire \BU2/N21 ;
  wire \BU2/N19 ;
  wire \BU2/N17 ;
  wire \BU2/N11 ;
  wire \BU2/N13 ;
  wire \BU2/N36 ;
  wire \BU2/N381 ;
  wire \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<9>4_470 ;
  wire \BU2/N6 ;
  wire \BU2/N7 ;
  wire \BU2/N35 ;
  wire \BU2/N41 ;
  wire \BU2/N2 ;
  wire \BU2/N40 ;
  wire \BU2/N30 ;
  wire \BU2/N4 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy<9>_bdd4 ;
  wire \BU2/N38 ;
  wire \BU2/N33 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0006_bdd0 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0006_bdd0 ;
  wire \BU2/N351 ;
  wire \BU2/U0/grf.rf/mem/tmp_ram_rd_en ;
  wire \BU2/U0/grf.rf/ram_regout_en ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<0>_rt_425 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<1>_rt_424 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<2>_rt_422 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<3>_rt_420 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<4>_rt_418 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<5>_rt_416 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<6>_rt_414 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<7>_rt_412 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<8>_rt_410 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<9>_rt_408 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count2 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count1 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count3 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count4 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count7 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count5 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count6 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count8 ;
  wire \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count9 ;
  wire \BU2/U0/grf.rf/ram_wr_en ;
  wire \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_fb_385 ;
  wire \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_i_or0000 ;
  wire \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/user_valid_383 ;
  wire \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd1_382 ;
  wire \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd1-In ;
  wire \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd2_380 ;
  wire \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd2-In ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<0>_rt_378 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<1>_rt_377 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<2>_rt_375 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<3>_rt_373 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<4>_rt_371 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<5>_rt_369 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<6>_rt_367 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<7>_rt_365 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<8>_rt_363 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<9>_rt_361 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count2 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count1 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count3 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count4 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count7 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count5 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count6 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count8 ;
  wire \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count9 ;
  wire \BU2/U0/grf.rf/ram_rd_en ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0008 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0007 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0006 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0005 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0004 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0003 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0002 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0001 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0000 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0008 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0007 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0006 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0005 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0004 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0003 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0002 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0001 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0000 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0008 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0007 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0006 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0005 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0004 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0003 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0002 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0001 ;
  wire \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0000 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0008 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0007 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0006 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0005 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0004 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0003 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0002 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0001 ;
  wire \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0000 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwas.wsts/comp2 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwas.wsts/comp1 ;
  wire \BU2/U0/grf.rf/gl0.rd/gras.rsts/comp1 ;
  wire \BU2/U0/grf.rf/gl0.rd/gras.rsts/comp0 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000016 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000014 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000012 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000010 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00008 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00006 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00004 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00002 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy[1] ;
  wire \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000016 ;
  wire \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000014 ;
  wire \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000012 ;
  wire \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000010 ;
  wire \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00008 ;
  wire \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00006 ;
  wire \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00004 ;
  wire \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00002 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy[9] ;
  wire \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_fb_i_94 ;
  wire \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_or0000 ;
  wire \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_81 ;
  wire \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or0000 ;
  wire \BU2/U0/grf.rf/rstblk/wr_rst_comb ;
  wire \BU2/U0/grf.rf/rstblk/rd_rst_comb ;
  wire \BU2/U0/grf.rf/rstblk/wr_rst_asreg_72 ;
  wire \BU2/U0/grf.rf/rstblk/rd_rst_asreg_71 ;
  wire \BU2/U0/grf.rf/rstblk/wr_rst_asreg_d2_70 ;
  wire \BU2/U0/grf.rf/rstblk/wr_rst_asreg_d1_69 ;
  wire \BU2/U0/grf.rf/rstblk/rd_rst_asreg_d2_68 ;
  wire \BU2/U0/grf.rf/rstblk/rd_rst_asreg_d1_67 ;
  wire \BU2/N1 ;
  wire NLW_VCC_P_UNCONNECTED;
  wire NLW_GND_G_UNCONNECTED;
  wire \NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<15>_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<14>_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<13>_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<12>_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<11>_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<10>_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<9>_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<8>_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<7>_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<6>_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<5>_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<4>_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<3>_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<2>_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<1>_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<0>_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOPA<1>_UNCONNECTED ;
  wire \NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOPA<0>_UNCONNECTED ;
  wire [17 : 0] din_2;
  wire [17 : 0] dout_3;
  wire [10 : 0] rd_data_count_4;
  wire [10 : 0] wr_data_count_5;
  wire [17 : 0] \BU2/U0/grf.rf/mem/dout_mem ;
  wire [9 : 0] \BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 ;
  wire [8 : 0] \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy ;
  wire [9 : 0] \BU2/U0/grf.rf/gl0.wr/wpntr/count ;
  wire [8 : 0] \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy ;
  wire [9 : 0] \BU2/U0/grf.rf/gl0.rd/rpntr/count ;
  wire [9 : 0] \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc ;
  wire [9 : 0] \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc ;
  wire [9 : 0] \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 ;
  wire [9 : 0] \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg ;
  wire [9 : 0] \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 ;
  wire [9 : 0] \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg ;
  wire [4 : 0] \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/v1 ;
  wire [3 : 0] \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/carrynet ;
  wire [4 : 0] \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/v1 ;
  wire [3 : 0] \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/carrynet ;
  wire [4 : 0] \BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/v1 ;
  wire [3 : 0] \BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/carrynet ;
  wire [4 : 0] \BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/v1 ;
  wire [3 : 0] \BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/carrynet ;
  wire [9 : 0] \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut ;
  wire [9 : 0] \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin ;
  wire [9 : 0] \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 ;
  wire [8 : 0] \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy ;
  wire [9 : 0] \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut ;
  wire [9 : 0] \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 ;
  wire [9 : 0] \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin ;
  wire [8 : 0] \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy ;
  wire [1 : 1] \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub0000_cy ;
  wire [0 : 0] \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/diff_wr_rd_tmp ;
  wire [9 : 1] \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000 ;
  wire [0 : 0] \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/diff_wr_rd_tmp ;
  wire [10 : 0] \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 ;
  wire [1 : 0] \BU2/U0/grf.rf/rstblk/wr_rst_reg ;
  wire [2 : 0] \BU2/U0/grf.rf/rstblk/rd_rst_reg ;
  wire [0 : 0] \BU2/data_count ;
  assign
    wr_data_count[10] = wr_data_count_5[10],
    wr_data_count[9] = wr_data_count_5[9],
    wr_data_count[8] = wr_data_count_5[8],
    wr_data_count[7] = wr_data_count_5[7],
    wr_data_count[6] = wr_data_count_5[6],
    wr_data_count[5] = wr_data_count_5[5],
    wr_data_count[4] = wr_data_count_5[4],
    wr_data_count[3] = wr_data_count_5[3],
    wr_data_count[2] = wr_data_count_5[2],
    wr_data_count[1] = wr_data_count_5[1],
    wr_data_count[0] = wr_data_count_5[0],
    rd_data_count[10] = rd_data_count_4[10],
    rd_data_count[9] = rd_data_count_4[9],
    rd_data_count[8] = rd_data_count_4[8],
    rd_data_count[7] = rd_data_count_4[7],
    rd_data_count[6] = rd_data_count_4[6],
    rd_data_count[5] = rd_data_count_4[5],
    rd_data_count[4] = rd_data_count_4[4],
    rd_data_count[3] = rd_data_count_4[3],
    rd_data_count[2] = rd_data_count_4[2],
    rd_data_count[1] = rd_data_count_4[1],
    rd_data_count[0] = rd_data_count_4[0],
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
  LUT4_D #(
    .INIT ( 16'h0440 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<9>31  (
    .I0(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/user_valid_383 ),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd1_382 ),
    .I3(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd2_380 ),
    .LO(\BU2/N40 ),
    .O(\BU2/N33 )
  );
  LUT4_L #(
    .INIT ( 16'h8000 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<8>_SW1_SW0  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub0000_cy [1]),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00002 ),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00004 ),
    .I3(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00006 ),
    .LO(\BU2/N31 )
  );
  LUT3_D #(
    .INIT ( 8'h80 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000<9>_SW0_SW0  (
    .I0(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00008 ),
    .I1(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000010 ),
    .I2(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000012 ),
    .LO(\BU2/N39 ),
    .O(\BU2/N27 )
  );
  LUT4_L #(
    .INIT ( 16'h8000 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<9>21_SW1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00002 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00004 ),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00006 ),
    .I3(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00008 ),
    .LO(\BU2/N19 )
  );
  LUT3_L #(
    .INIT ( 8'h80 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<9>21_SW0  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00002 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00004 ),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00006 ),
    .LO(\BU2/N17 )
  );
  LUT4_D #(
    .INIT ( 16'h2000 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<4>111_SW0  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/user_valid_383 ),
    .I1(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .I2(\BU2/U0/grf.rf/gl0.rd/ram_valid_fwft ),
    .I3(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub0000_cy [1]),
    .LO(\BU2/N381 ),
    .O(\BU2/N11 )
  );
  LUT3_D #(
    .INIT ( 8'h80 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<9>41_SW0  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000012 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000010 ),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00008 ),
    .LO(\BU2/N37 ),
    .O(\BU2/N13 )
  );
  LUT3_D #(
    .INIT ( 8'h7F ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<4>111  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub0000_cy [1]),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00002 ),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00004 ),
    .LO(\BU2/N36 ),
    .O(\BU2/N38 )
  );
  LUT2_D #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/RAM_VALID_FWFT11  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd1_382 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd2_380 ),
    .LO(\BU2/N351 ),
    .O(\BU2/U0/grf.rf/gl0.rd/ram_valid_fwft )
  );
  RAMB16_S18_S18 #(
    .INIT_3E ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_3F ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INITP_00 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INITP_01 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INITP_02 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INITP_03 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INITP_04 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INITP_05 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .SRVAL_A ( 18'h00000 ),
    .SRVAL_B ( 18'h00000 ),
    .INIT_00 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_01 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_02 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_03 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_04 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_05 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_06 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_07 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_08 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_09 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_0A ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_0B ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_0C ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_0D ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_0E ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_0F ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_10 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_11 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_12 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_13 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_14 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_15 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_16 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_17 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_18 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_19 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_1A ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_1B ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_1C ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_1D ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_1E ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_1F ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_20 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_21 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_22 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_23 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_24 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_25 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_26 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_27 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_28 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_29 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_2A ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_2B ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_2C ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_2D ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_2E ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_2F ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_30 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_31 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_32 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_33 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_34 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_35 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_36 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_37 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_38 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_39 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_3A ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_3B ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_3C ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .INIT_3D ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .WRITE_MODE_B ( "WRITE_FIRST" ),
    .INITP_06 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ),
    .SIM_COLLISION_CHECK ( "NONE" ),
    .INIT_A ( 18'h00000 ),
    .INIT_B ( 18'h00000 ),
    .WRITE_MODE_A ( "WRITE_FIRST" ),
    .INITP_07 ( 256'h0000000000000000000000000000000000000000000000000000000000000000 ))
  \BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram  (
    .CLKA(wr_clk),
    .CLKB(rd_clk),
    .ENA(\BU2/N1 ),
    .ENB(\BU2/U0/grf.rf/mem/tmp_ram_rd_en ),
    .SSRA(\BU2/data_count [0]),
    .SSRB(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .WEA(\BU2/U0/grf.rf/ram_wr_en ),
    .WEB(\BU2/data_count [0]),
    .ADDRA({\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [9], \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [8], \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [7], 
\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [6], \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [5], \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [4], 
\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [3], \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [2], \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [1], 
\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [0]}),
    .ADDRB({\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [9], \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [8], \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [7], 
\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [6], \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [5], \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4], 
\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3], \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2], \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1], 
\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]}),
    .DIA({din_2[16], din_2[15], din_2[14], din_2[13], din_2[12], din_2[11], din_2[10], din_2[9], din_2[7], din_2[6], din_2[5], din_2[4], din_2[3], 
din_2[2], din_2[1], din_2[0]}),
    .DIB({\BU2/data_count [0], \BU2/data_count [0], \BU2/data_count [0], \BU2/data_count [0], \BU2/data_count [0], \BU2/data_count [0], 
\BU2/data_count [0], \BU2/data_count [0], \BU2/data_count [0], \BU2/data_count [0], \BU2/data_count [0], \BU2/data_count [0], \BU2/data_count [0], 
\BU2/data_count [0], \BU2/data_count [0], \BU2/data_count [0]}),
    .DIPA({din_2[17], din_2[8]}),
    .DIPB({\BU2/data_count [0], \BU2/data_count [0]}),
    .DOA({\NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<15>_UNCONNECTED 
, \NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<14>_UNCONNECTED , 
\NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<13>_UNCONNECTED , 
\NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<12>_UNCONNECTED , 
\NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<11>_UNCONNECTED , 
\NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<10>_UNCONNECTED , 
\NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<9>_UNCONNECTED , 
\NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<8>_UNCONNECTED , 
\NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<7>_UNCONNECTED , 
\NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<6>_UNCONNECTED , 
\NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<5>_UNCONNECTED , 
\NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<4>_UNCONNECTED , 
\NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<3>_UNCONNECTED , 
\NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<2>_UNCONNECTED , 
\NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<1>_UNCONNECTED , 
\NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOA<0>_UNCONNECTED }),
    .DOPA({
\NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOPA<1>_UNCONNECTED , 
\NLW_BU2/U0/grf.rf/mem/gbm.gbmg.gbmga.ngecc.bmg/blk_mem_generator/valid.cstr/ramloop[0].ram.r/v2_noinit.ram/dp18x18.ram_DOPA<0>_UNCONNECTED }),
    .DOB({\BU2/U0/grf.rf/mem/dout_mem [16], \BU2/U0/grf.rf/mem/dout_mem [15], \BU2/U0/grf.rf/mem/dout_mem [14], \BU2/U0/grf.rf/mem/dout_mem [13], 
\BU2/U0/grf.rf/mem/dout_mem [12], \BU2/U0/grf.rf/mem/dout_mem [11], \BU2/U0/grf.rf/mem/dout_mem [10], \BU2/U0/grf.rf/mem/dout_mem [9], 
\BU2/U0/grf.rf/mem/dout_mem [7], \BU2/U0/grf.rf/mem/dout_mem [6], \BU2/U0/grf.rf/mem/dout_mem [5], \BU2/U0/grf.rf/mem/dout_mem [4], 
\BU2/U0/grf.rf/mem/dout_mem [3], \BU2/U0/grf.rf/mem/dout_mem [2], \BU2/U0/grf.rf/mem/dout_mem [1], \BU2/U0/grf.rf/mem/dout_mem [0]}),
    .DOPB({\BU2/U0/grf.rf/mem/dout_mem [17], \BU2/U0/grf.rf/mem/dout_mem [8]})
  );
  INV   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_xor<1>11_INV_0  (
    .I(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy[1] ),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000 [1])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy<9>  (
    .I0(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000014 ),
    .I1(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000016 ),
    .I2(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy<9>_bdd4 ),
    .I3(\BU2/N39 ),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy[9] )
  );
  LUT4 #(
    .INIT ( 16'h7FFF ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<8>_SW1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000012 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000010 ),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00008 ),
    .I3(\BU2/N31 ),
    .O(\BU2/N7 )
  );
  LUT4 #(
    .INIT ( 16'hA2AA ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<9>30_SW0  (
    .I0(\BU2/N33 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000014 ),
    .I2(\BU2/N4 ),
    .I3(\BU2/N37 ),
    .O(\BU2/N21 )
  );
  LUT4 #(
    .INIT ( 16'h6CCC ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000<9>  (
    .I0(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000014 ),
    .I1(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000016 ),
    .I2(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy<9>_bdd4 ),
    .I3(\BU2/N27 ),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000 [9])
  );
  LUT4 #(
    .INIT ( 16'h6CCC ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000<8>1  (
    .I0(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000012 ),
    .I1(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000014 ),
    .I2(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy<9>_bdd4 ),
    .I3(\BU2/N25 ),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000 [8])
  );
  LUT2 #(
    .INIT ( 4'h8 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy<9>31_SW1  (
    .I0(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00008 ),
    .I1(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000010 ),
    .O(\BU2/N25 )
  );
  LUT4 #(
    .INIT ( 16'hEAC0 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<9>38  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000016 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<9>4_470 ),
    .I2(\BU2/N35 ),
    .I3(\BU2/N21 ),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [9])
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<8>_SW0  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000012 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000010 ),
    .I2(\BU2/N11 ),
    .I3(\BU2/N19 ),
    .O(\BU2/N6 )
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<9>41  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000010 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00008 ),
    .I2(\BU2/N11 ),
    .I3(\BU2/N17 ),
    .O(\BU2/N35 )
  );
  LUT4 #(
    .INIT ( 16'h7FFF ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<5>11  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub0000_cy [1]),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00002 ),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00004 ),
    .I3(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00006 ),
    .O(\BU2/N4 )
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<10>1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000014 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000016 ),
    .I2(\BU2/N13 ),
    .I3(\BU2/N30 ),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [10])
  );
  LUT4 #(
    .INIT ( 16'hFF7F ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<7>_SW0  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000010 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00008 ),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00006 ),
    .I3(\BU2/N36 ),
    .O(\BU2/N41 )
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<9>21  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00006 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00004 ),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00002 ),
    .I3(\BU2/N381 ),
    .O(\BU2/N30 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<9>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [9]),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<9>_rt_408 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<9>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [9]),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<9>_rt_361 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<0>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [0]),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<0>_rt_425 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<1>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [1]),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<1>_rt_424 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<2>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [2]),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<2>_rt_422 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<3>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [3]),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<3>_rt_420 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<4>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [4]),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<4>_rt_418 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<5>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [5]),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<5>_rt_416 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<6>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [6]),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<6>_rt_414 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<7>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [7]),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<7>_rt_412 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<8>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [8]),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<8>_rt_410 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<0>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [0]),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<0>_rt_378 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<1>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [1]),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<1>_rt_377 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<2>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [2]),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<2>_rt_375 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<3>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [3]),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<3>_rt_373 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<4>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [4]),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<4>_rt_371 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<5>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [5]),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<5>_rt_369 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<6>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [6]),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<6>_rt_367 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<7>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [7]),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<7>_rt_365 )
  );
  LUT1 #(
    .INIT ( 2'h2 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<8>_rt  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [8]),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<8>_rt_363 )
  );
  LUT4 #(
    .INIT ( 16'h8000 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy<9>41  (
    .I0(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy[1] ),
    .I1(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00002 ),
    .I2(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00004 ),
    .I3(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00006 ),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy<9>_bdd4 )
  );
  LUT3 #(
    .INIT ( 8'h08 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<9>4  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000012 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000014 ),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000016 ),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<9>4_470 )
  );
  LUT4 #(
    .INIT ( 16'hD580 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<8>  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000014 ),
    .I1(\BU2/N33 ),
    .I2(\BU2/N7 ),
    .I3(\BU2/N6 ),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [8])
  );
  LUT4 #(
    .INIT ( 16'hD580 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<7>  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000012 ),
    .I1(\BU2/N33 ),
    .I2(\BU2/N41 ),
    .I3(\BU2/N35 ),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [7])
  );
  LUT4 #(
    .INIT ( 16'hEA40 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<6>  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000010 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00008 ),
    .I2(\BU2/N30 ),
    .I3(\BU2/N2 ),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [6])
  );
  LUT4 #(
    .INIT ( 16'hAA2A ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<6>_SW0  (
    .I0(\BU2/N40 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00008 ),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00006 ),
    .I3(\BU2/N38 ),
    .O(\BU2/N2 )
  );
  LUT4 #(
    .INIT ( 16'hD580 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<5>2  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00008 ),
    .I1(\BU2/N4 ),
    .I2(\BU2/N33 ),
    .I3(\BU2/N30 ),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [5])
  );
  LUT4 #(
    .INIT ( 16'h6CCC ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000<7>1  (
    .I0(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00008 ),
    .I1(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000012 ),
    .I2(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000010 ),
    .I3(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy<9>_bdd4 ),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000 [7])
  );
  LUT3 #(
    .INIT ( 8'h6C ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000<6>1  (
    .I0(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00008 ),
    .I1(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000010 ),
    .I2(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy<9>_bdd4 ),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000 [6])
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000<5>1  (
    .I0(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00008 ),
    .I1(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy<9>_bdd4 ),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000 [5])
  );
  LUT3 #(
    .INIT ( 8'h82 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<4>1  (
    .I0(\BU2/N33 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00006 ),
    .I2(\BU2/N38 ),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [4])
  );
  LUT4 #(
    .INIT ( 16'h6CCC ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_xor<4>11  (
    .I0(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00002 ),
    .I1(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00006 ),
    .I2(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00004 ),
    .I3(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy[1] ),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000 [4])
  );
  LUT4 #(
    .INIT ( 16'h60C0 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<3>1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00002 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00004 ),
    .I2(\BU2/N33 ),
    .I3(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub0000_cy [1]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [3])
  );
  LUT3 #(
    .INIT ( 8'h6C ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_xor<3>11  (
    .I0(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00002 ),
    .I1(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00004 ),
    .I2(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy[1] ),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000 [3])
  );
  LUT3 #(
    .INIT ( 8'h28 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<2>1  (
    .I0(\BU2/N33 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00002 ),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub0000_cy [1]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [2])
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_xor<2>11  (
    .I0(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00002 ),
    .I1(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy[1] ),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000 [2])
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<1>1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub0000_cy [1]),
    .I1(\BU2/N33 ),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [1])
  );
  LUT4 #(
    .INIT ( 16'h9669 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor000611  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [3]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [4]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [5]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0002 ),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0006_bdd0 )
  );
  LUT4 #(
    .INIT ( 16'h9669 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor00081  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [0]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [1]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [2]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0006_bdd0 ),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0008 )
  );
  LUT4 #(
    .INIT ( 16'h9669 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor000611  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [3]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [4]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [5]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0002 ),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0006_bdd0 )
  );
  LUT4 #(
    .INIT ( 16'h9669 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor00081  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [0]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [1]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [2]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0006_bdd0 ),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0008 )
  );
  LUT3 #(
    .INIT ( 8'h69 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor00071  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [1]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [2]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0006_bdd0 ),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0007 )
  );
  LUT3 #(
    .INIT ( 8'h69 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor00071  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [1]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [2]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0006_bdd0 ),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0007 )
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor00062  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [2]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0006_bdd0 ),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0006 )
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor00062  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [2]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0006_bdd0 ),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0006 )
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/v1_0_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [1]),
    .I2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [0]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/v1 [0])
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/v1_0_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [1]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [1]),
    .I2(\BU2/U0/grf.rf/gl0.rd/rpntr/count [0]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [0]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/v1 [0])
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/v1_0_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [1]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [1]),
    .I2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [0]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [0]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/v1 [0])
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/v1_0_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [1]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [1]),
    .I2(\BU2/U0/grf.rf/gl0.wr/wpntr/count [0]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [0]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/v1 [0])
  );
  LUT4 #(
    .INIT ( 16'hBAAA ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_or00001  (
    .I0(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/comp1 ),
    .I1(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_fb_i_94 ),
    .I2(wr_en),
    .I3(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/comp2 ),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_or0000 )
  );
  LUT3 #(
    .INIT ( 8'hF8 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or00001  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gras.rsts/comp1 ),
    .I1(\BU2/U0/grf.rf/ram_rd_en ),
    .I2(\BU2/U0/grf.rf/gl0.rd/gras.rsts/comp0 ),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or0000 )
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/v1_1_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [3]),
    .I2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [2]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/v1 [1])
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/v1_1_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [3]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [3]),
    .I2(\BU2/U0/grf.rf/gl0.rd/rpntr/count [2]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [2]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/v1 [1])
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/v1_1_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [3]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [3]),
    .I2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [2]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [2]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/v1 [1])
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/v1_1_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [3]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [3]),
    .I2(\BU2/U0/grf.rf/gl0.wr/wpntr/count [2]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [2]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/v1 [1])
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/v1_2_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [5]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [5]),
    .I2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [4]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/v1 [2])
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/v1_2_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [5]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [5]),
    .I2(\BU2/U0/grf.rf/gl0.rd/rpntr/count [4]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [4]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/v1 [2])
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/v1_2_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [5]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [5]),
    .I2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [4]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [4]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/v1 [2])
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/v1_2_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [5]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [5]),
    .I2(\BU2/U0/grf.rf/gl0.wr/wpntr/count [4]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [4]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/v1 [2])
  );
  LUT4 #(
    .INIT ( 16'h3010 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002<0>1  (
    .I0(\BU2/N351 ),
    .I1(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/user_valid_383 ),
    .I3(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/diff_wr_rd_tmp [0]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [0])
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/v1_3_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [7]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [7]),
    .I2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [6]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [6]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/v1 [3])
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/v1_3_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [7]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [7]),
    .I2(\BU2/U0/grf.rf/gl0.rd/rpntr/count [6]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [6]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/v1 [3])
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/v1_3_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [7]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [7]),
    .I2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [6]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [6]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/v1 [3])
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/v1_3_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [7]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [7]),
    .I2(\BU2/U0/grf.rf/gl0.wr/wpntr/count [6]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [6]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/v1 [3])
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/v1_4_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [9]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [9]),
    .I2(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [8]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [8]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/v1 [4])
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/v1_4_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count [9]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [9]),
    .I2(\BU2/U0/grf.rf/gl0.rd/rpntr/count [8]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [8]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/v1 [4])
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/v1_4_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [9]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [9]),
    .I2(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [8]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [8]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/v1 [4])
  );
  LUT4 #(
    .INIT ( 16'h9009 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/v1_4_and00001  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count [9]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [9]),
    .I2(\BU2/U0/grf.rf/gl0.wr/wpntr/count [8]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [8]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/v1 [4])
  );
  LUT2 #(
    .INIT ( 4'hE ))
  \BU2/U0/grf.rf/mem/tmp_ram_rd_en1  (
    .I0(\BU2/U0/grf.rf/ram_rd_en ),
    .I1(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .O(\BU2/U0/grf.rf/mem/tmp_ram_rd_en )
  );
  LUT4 #(
    .INIT ( 16'h5455 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/RAM_RD_EN_FWFT1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_81 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd2_380 ),
    .I2(rd_en),
    .I3(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd1_382 ),
    .O(\BU2/U0/grf.rf/ram_rd_en )
  );
  LUT4 #(
    .INIT ( 16'h6996 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor00051  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [4]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [3]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0002 ),
    .I3(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [5]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0005 )
  );
  LUT4 #(
    .INIT ( 16'h6996 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor00051  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [4]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [3]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0002 ),
    .I3(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [5]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0005 )
  );
  LUT3 #(
    .INIT ( 8'h96 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor00041  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0002 ),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [5]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [4]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0004 )
  );
  LUT3 #(
    .INIT ( 8'h96 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor00041  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0002 ),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [5]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [4]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0004 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor00032  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [5]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0002 ),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0003 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor00032  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [5]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0002 ),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0003 )
  );
  LUT3 #(
    .INIT ( 8'h62 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/RAM_REGOUT_EN1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd2_380 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd1_382 ),
    .I2(rd_en),
    .O(\BU2/U0/grf.rf/ram_regout_en )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \BU2/U0/grf.rf/gl0.wr/ram_wr_en_i1  (
    .I0(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_fb_i_94 ),
    .I1(wr_en),
    .O(\BU2/U0/grf.rf/ram_wr_en )
  );
  LUT4 #(
    .INIT ( 16'h69A1 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd2-In11  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_81 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd1_382 ),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd2_380 ),
    .I3(rd_en),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd2-In )
  );
  LUT4 #(
    .INIT ( 16'hCA8A ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_i_or00001  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_fb_385 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd1_382 ),
    .I2(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd2_380 ),
    .I3(rd_en),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_i_or0000 )
  );
  LUT4 #(
    .INIT ( 16'h6996 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor00021  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [7]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [6]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [9]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [8]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0002 )
  );
  LUT4 #(
    .INIT ( 16'h6996 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor00021  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [7]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [6]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [9]),
    .I3(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [8]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0002 )
  );
  LUT3 #(
    .INIT ( 8'h6E ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd1-In11  (
    .I0(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd1_382 ),
    .I1(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd2_380 ),
    .I2(rd_en),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd1-In )
  );
  LUT3 #(
    .INIT ( 8'h96 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor00011  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [9]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [8]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [7]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0001 )
  );
  LUT3 #(
    .INIT ( 8'h96 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor00011  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [9]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [8]),
    .I2(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [7]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0001 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_rd_pntr_gc_xor0000_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [9]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [8]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0000 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_rd_pntr_gc_xor0001_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [8]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [7]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0001 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_rd_pntr_gc_xor0002_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [7]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [6]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0002 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_rd_pntr_gc_xor0003_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [6]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [5]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0003 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_rd_pntr_gc_xor0004_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [5]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0004 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_rd_pntr_gc_xor0005_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0005 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_rd_pntr_gc_xor0006_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0006 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_rd_pntr_gc_xor0007_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0007 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_rd_pntr_gc_xor0008_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0008 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_wr_pntr_gc_xor0000_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [9]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [8]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0000 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_wr_pntr_gc_xor0001_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [8]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [7]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0001 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_wr_pntr_gc_xor0002_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [7]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [6]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0002 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_wr_pntr_gc_xor0003_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [6]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [5]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0003 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_wr_pntr_gc_xor0004_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [5]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [4]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0004 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_wr_pntr_gc_xor0005_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [4]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [3]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0005 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_wr_pntr_gc_xor0006_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [3]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [2]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0006 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_wr_pntr_gc_xor0007_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [2]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [1]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0007 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/Mxor_wr_pntr_gc_xor0008_Result1  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [1]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [0]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0008 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor00001  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [8]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [9]),
    .O(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0000 )
  );
  LUT2 #(
    .INIT ( 4'h6 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor00001  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [8]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [9]),
    .O(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0000 )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \BU2/U0/grf.rf/rstblk/rd_rst_comb1  (
    .I0(\BU2/U0/grf.rf/rstblk/rd_rst_asreg_d2_68 ),
    .I1(\BU2/U0/grf.rf/rstblk/rd_rst_asreg_71 ),
    .O(\BU2/U0/grf.rf/rstblk/rd_rst_comb )
  );
  LUT2 #(
    .INIT ( 4'h4 ))
  \BU2/U0/grf.rf/rstblk/wr_rst_comb1  (
    .I0(\BU2/U0/grf.rf/rstblk/wr_rst_asreg_d2_70 ),
    .I1(\BU2/U0/grf.rf/rstblk/wr_rst_asreg_72 ),
    .O(\BU2/U0/grf.rf/rstblk/wr_rst_comb )
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_0  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_regout_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/dout_mem [0]),
    .Q(dout_3[0])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_1  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_regout_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/dout_mem [1]),
    .Q(dout_3[1])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_2  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_regout_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/dout_mem [2]),
    .Q(dout_3[2])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_3  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_regout_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/dout_mem [3]),
    .Q(dout_3[3])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_4  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_regout_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/dout_mem [4]),
    .Q(dout_3[4])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_5  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_regout_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/dout_mem [5]),
    .Q(dout_3[5])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_6  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_regout_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/dout_mem [6]),
    .Q(dout_3[6])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_7  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_regout_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/dout_mem [7]),
    .Q(dout_3[7])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_8  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_regout_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/dout_mem [8]),
    .Q(dout_3[8])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_9  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_regout_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/dout_mem [9]),
    .Q(dout_3[9])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_10  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_regout_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/dout_mem [10]),
    .Q(dout_3[10])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_11  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_regout_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/dout_mem [11]),
    .Q(dout_3[11])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_12  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_regout_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/dout_mem [12]),
    .Q(dout_3[12])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_13  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_regout_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/dout_mem [13]),
    .Q(dout_3[13])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_14  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_regout_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/dout_mem [14]),
    .Q(dout_3[14])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_15  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_regout_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/dout_mem [15]),
    .Q(dout_3[15])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_16  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_regout_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/dout_mem [16]),
    .Q(dout_3[16])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/mem/dout_i_17  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_regout_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0]),
    .D(\BU2/U0/grf.rf/mem/dout_mem [17]),
    .Q(dout_3[17])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2_0  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [0]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [0])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2_1  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [1]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [1])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2_2  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [2]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [2])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2_3  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [3]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [3])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2_4  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [4]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [4])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2_5  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [5]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [5])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2_6  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [6]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [6])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2_7  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [7]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [7])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2_8  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [8]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [8])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d2_9  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [9]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [9])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d1_9  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count [9]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [9])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d1_8  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count [8]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [8])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d1_6  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count [6]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [6])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d1_5  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count [5]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [5])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d1_7  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count [7]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [7])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d1_4  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count [4]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [4])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d1_3  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count [3]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [3])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d1_1  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count [1]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [1])
  );
  FDPE #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d1_0  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count [0]),
    .PRE(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [0])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_d1_2  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count [2]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d1 [2])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<0>  (
    .CI(\BU2/N1 ),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<0>_rt_425 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [0])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<0>  (
    .CI(\BU2/N1 ),
    .LI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<0>_rt_425 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<1>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [0]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<1>_rt_424 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [1])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<1>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [0]),
    .LI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<1>_rt_424 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count1 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<2>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [1]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<2>_rt_422 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [2])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<2>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [1]),
    .LI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<2>_rt_422 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count2 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<3>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [2]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<3>_rt_420 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [3])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<3>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [2]),
    .LI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<3>_rt_420 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count3 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<4>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [3]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<4>_rt_418 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [4])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<4>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [3]),
    .LI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<4>_rt_418 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count4 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<5>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [4]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<5>_rt_416 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [5])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<5>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [4]),
    .LI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<5>_rt_416 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count5 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<6>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [5]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<6>_rt_414 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [6])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<6>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [5]),
    .LI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<6>_rt_414 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count6 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<7>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [6]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<7>_rt_412 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [7])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<7>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [6]),
    .LI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<7>_rt_412 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count7 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<8>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [7]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<8>_rt_410 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [8])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<8>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [7]),
    .LI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy<8>_rt_410 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count8 )
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<9>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_cy [8]),
    .LI(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count_xor<9>_rt_408 ),
    .O(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count9 )
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_2  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count2 ),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count [2])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_0  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count ),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count [0])
  );
  FDPE #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_1  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count1 ),
    .PRE(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count [1])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_3  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count3 ),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count [3])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_4  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count4 ),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count [4])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_7  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count7 ),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count [7])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_5  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count5 ),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count [5])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_6  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count6 ),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count [6])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_8  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count8 ),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count [8])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/wpntr/count_9  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/ram_wr_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/Mcount_count9 ),
    .Q(\BU2/U0/grf.rf/gl0.wr/wpntr/count [9])
  );
  FDP #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_fb  (
    .C(rd_clk),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_i_or0000 ),
    .PRE(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .Q(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_fb_385 )
  );
  FDP #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_i  (
    .C(rd_clk),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/empty_fwft_i_or0000 ),
    .PRE(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .Q(empty)
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/user_valid  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd1-In ),
    .Q(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/user_valid_383 )
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd1  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd1-In ),
    .Q(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd1_382 )
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd2  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd2-In ),
    .Q(\BU2/U0/grf.rf/gl0.rd/gr1.rfwft/curr_fwft_state_FSM_FFd2_380 )
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1_0  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/count [0]),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1_1  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/count [1]),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1_2  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/count [2]),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1_3  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/count [3]),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1_4  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/count [4]),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1_5  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/count [5]),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [5])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1_6  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/count [6]),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [6])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1_7  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/count [7]),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [7])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1_8  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/count [8]),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [8])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_d1_9  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/count [9]),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [9])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<0>  (
    .CI(\BU2/N1 ),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<0>_rt_378 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [0])
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<0>  (
    .CI(\BU2/N1 ),
    .LI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<0>_rt_378 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<1>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [0]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<1>_rt_377 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [1])
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<1>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [0]),
    .LI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<1>_rt_377 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count1 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<2>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [1]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<2>_rt_375 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [2])
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<2>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [1]),
    .LI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<2>_rt_375 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count2 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<3>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [2]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<3>_rt_373 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [3])
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<3>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [2]),
    .LI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<3>_rt_373 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count3 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<4>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [3]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<4>_rt_371 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [4])
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<4>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [3]),
    .LI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<4>_rt_371 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count4 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<5>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [4]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<5>_rt_369 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [5])
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<5>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [4]),
    .LI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<5>_rt_369 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count5 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<6>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [5]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<6>_rt_367 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [6])
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<6>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [5]),
    .LI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<6>_rt_367 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count6 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<7>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [6]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<7>_rt_365 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [7])
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<7>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [6]),
    .LI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<7>_rt_365 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count7 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<8>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [7]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<8>_rt_363 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [8])
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<8>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [7]),
    .LI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy<8>_rt_363 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count8 )
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<9>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_cy [8]),
    .LI(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count_xor<9>_rt_361 ),
    .O(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count9 )
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_2  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count2 ),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count [2])
  );
  FDPE #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_0  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count ),
    .PRE(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count [0])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_1  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count1 ),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count [1])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_3  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count3 ),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count [3])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_4  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count4 ),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count [4])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_7  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count7 ),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count [7])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_5  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count5 ),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count [5])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_6  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count6 ),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count [6])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_8  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count8 ),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count [8])
  );
  FDCE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/rpntr/count_9  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/ram_rd_en ),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/Mcount_count9 ),
    .Q(\BU2/U0/grf.rf/gl0.rd/rpntr/count [9])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_0  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0008 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [0])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_1  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0007 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [1])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_2  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0006 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [2])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_3  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0005 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [3])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_4  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0004 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [4])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_5  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0003 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [5])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_6  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0002 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [6])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_7  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0001 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [7])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_8  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_xor0000 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [8])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_9  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [9]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [9])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_0  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0008 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [0])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_1  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0007 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [1])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_2  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0006 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [2])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_3  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0005 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [3])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_4  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0004 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [4])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_5  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0003 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [5])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_6  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0002 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [6])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_7  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0001 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [7])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_8  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_xor0000 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [8])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_9  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [9]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [9])
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
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_5  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [5]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [5])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_6  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [6]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [6])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_7  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [7]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [7])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_8  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [8]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [8])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_9  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc [9]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [9])
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
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_5  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [5]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [5])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_6  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [6]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [6])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_7  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [7]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [7])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_8  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [8]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [8])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_9  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc [9]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [9])
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
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1_5  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [5]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [5])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1_6  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [6]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [6])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1_7  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [7]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [7])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1_8  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [8]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [8])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1_9  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg [9]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [9])
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
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1_5  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [5]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [5])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1_6  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [6]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [6])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1_7  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [7]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [7])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1_8  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [8]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [8])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1_9  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg [9]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [9])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_0  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0008 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [0])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_1  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0007 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [1])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_2  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0006 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [2])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_3  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0005 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [3])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_4  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0004 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [4])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_5  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0003 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [5])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_6  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0002 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [6])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_7  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0001 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [7])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_8  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_xor0000 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [8])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin_9  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_gc_asreg_d1 [9]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [9])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_0  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0008 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [0])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_1  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0007 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [1])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_2  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0006 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [2])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_3  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0005 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [3])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_4  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0004 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [4])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_5  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0003 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [5])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_6  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0002 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [6])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_7  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0001 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [7])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_8  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_xor0000 ),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [8])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin_9  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0]),
    .D(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_gc_asreg_d1 [9]),
    .Q(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [9])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/gmux.gm[0].gm1.m1  (
    .CI(\BU2/N1 ),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/v1 [0]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/carrynet [0])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/gmux.gm[1].gms.ms  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/carrynet [0]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/v1 [1]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/carrynet [1])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/gmux.gm[2].gms.ms  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/carrynet [1]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/v1 [2]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/carrynet [2])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/gmux.gm[3].gms.ms  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/carrynet [2]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/v1 [3]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/carrynet [3])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/gmux.gm[4].gms.ms  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/carrynet [3]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c2/v1 [4]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/comp2 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/gmux.gm[0].gm1.m1  (
    .CI(\BU2/N1 ),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/v1 [0]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/carrynet [0])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/gmux.gm[1].gms.ms  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/carrynet [0]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/v1 [1]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/carrynet [1])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/gmux.gm[2].gms.ms  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/carrynet [1]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/v1 [2]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/carrynet [2])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/gmux.gm[3].gms.ms  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/carrynet [2]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/v1 [3]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/carrynet [3])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/gmux.gm[4].gms.ms  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/carrynet [3]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/c1/v1 [4]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/comp1 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/gmux.gm[0].gm1.m1  (
    .CI(\BU2/N1 ),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/v1 [0]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/carrynet [0])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/gmux.gm[1].gms.ms  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/carrynet [0]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/v1 [1]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/carrynet [1])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/gmux.gm[2].gms.ms  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/carrynet [1]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/v1 [2]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/carrynet [2])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/gmux.gm[3].gms.ms  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/carrynet [2]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/v1 [3]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/carrynet [3])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/gmux.gm[4].gms.ms  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/carrynet [3]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c1/v1 [4]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/comp1 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/gmux.gm[0].gm1.m1  (
    .CI(\BU2/N1 ),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/v1 [0]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/carrynet [0])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/gmux.gm[1].gms.ms  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/carrynet [0]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/v1 [1]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/carrynet [1])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/gmux.gm[2].gms.ms  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/carrynet [1]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/v1 [2]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/carrynet [2])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/gmux.gm[3].gms.ms  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/carrynet [2]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/v1 [3]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/carrynet [3])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/gmux.gm[4].gms.ms  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/carrynet [3]),
    .DI(\BU2/data_count [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/gras.rsts/c0/v1 [4]),
    .O(\BU2/U0/grf.rf/gl0.rd/gras.rsts/comp0 )
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_xor<9>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [8]),
    .LI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [9]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000016 )
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut<9>  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [9]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [9]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [9])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_xor<8>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [7]),
    .LI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [8]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000014 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy<8>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [7]),
    .DI(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [8]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [8]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [8])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut<8>  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [8]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [8]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [8])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_xor<7>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [6]),
    .LI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [7]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000012 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy<7>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [6]),
    .DI(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [7]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [7]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [7])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut<7>  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [7]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [7]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [7])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_xor<6>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [5]),
    .LI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [6]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add000010 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy<6>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [5]),
    .DI(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [6]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [6]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [6])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut<6>  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [6]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [6]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [6])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_xor<5>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [4]),
    .LI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [5]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00008 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy<5>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [4]),
    .DI(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [5]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [5]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [5])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut<5>  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [5]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [5]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [5])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_xor<4>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [3]),
    .LI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [4]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00006 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy<4>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [3]),
    .DI(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [4]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [4]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [4])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut<4>  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [4]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [4]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [4])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_xor<3>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [2]),
    .LI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [3]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00004 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy<3>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [2]),
    .DI(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [3]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [3]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [3])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut<3>  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [3]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [3]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [3])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_xor<2>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [1]),
    .LI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [2]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add00002 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy<2>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [1]),
    .DI(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [2]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [2]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [2])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut<2>  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [2]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [2]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [2])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_xor<1>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [0]),
    .LI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [1]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy[1] )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy<1>  (
    .CI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [0]),
    .DI(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [1]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [1]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [1])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut<1>  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [1]),
    .I1(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [1]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [1])
  );
  XORCY   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_xor<0>  (
    .CI(\BU2/N1 ),
    .LI(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [0]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/diff_wr_rd_tmp [0])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy<0>  (
    .CI(\BU2/N1 ),
    .DI(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [0]),
    .S(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [0]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_cy [0])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut<0>  (
    .I0(\BU2/U0/grf.rf/gl0.wr/wpntr/count_d2 [0]),
    .I1(\BU2/U0/grf.rf/gcx.clkx/rd_pntr_bin [0]),
    .O(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Msub_diff_wr_rd_tmp_lut [0])
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_xor<9>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [8]),
    .LI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [9]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000016 )
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut<9>  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [9]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [9]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [9])
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_xor<8>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [7]),
    .LI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [8]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000014 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy<8>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [7]),
    .DI(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [8]),
    .S(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [8]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [8])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut<8>  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [8]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [8]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [8])
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_xor<7>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [6]),
    .LI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [7]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000012 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy<7>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [6]),
    .DI(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [7]),
    .S(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [7]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [7])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut<7>  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [7]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [7]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [7])
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_xor<6>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [5]),
    .LI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [6]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub000010 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy<6>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [5]),
    .DI(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [6]),
    .S(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [6]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [6])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut<6>  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [6]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [6]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [6])
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_xor<5>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [4]),
    .LI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [5]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00008 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy<5>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [4]),
    .DI(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [5]),
    .S(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [5]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [5])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut<5>  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [5]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [5]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [5])
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_xor<4>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [3]),
    .LI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [4]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00006 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy<4>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [3]),
    .DI(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [4]),
    .S(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [4]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [4])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut<4>  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [4]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [4]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [4])
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_xor<3>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [2]),
    .LI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [3]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00004 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy<3>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [2]),
    .DI(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [3]),
    .S(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [3]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [3])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut<3>  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [3]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [3]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [3])
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_xor<2>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [1]),
    .LI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [2]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub00002 )
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy<2>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [1]),
    .DI(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [2]),
    .S(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [2]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [2])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut<2>  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [2]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [2]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [2])
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_xor<1>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [0]),
    .LI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [1]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Madd_rd_dc_i_addsub0000_cy [1])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy<1>  (
    .CI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [0]),
    .DI(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [1]),
    .S(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [1]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [1])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut<1>  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [1]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [1]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [1])
  );
  XORCY   \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_xor<0>  (
    .CI(\BU2/N1 ),
    .LI(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [0]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/diff_wr_rd_tmp [0])
  );
  MUXCY   \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy<0>  (
    .CI(\BU2/N1 ),
    .DI(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [0]),
    .S(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [0]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_cy [0])
  );
  LUT2 #(
    .INIT ( 4'h9 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut<0>  (
    .I0(\BU2/U0/grf.rf/gcx.clkx/wr_pntr_bin [0]),
    .I1(\BU2/U0/grf.rf/gl0.rd/rpntr/count_d1 [0]),
    .O(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/Msub_diff_wr_rd_tmp_lut [0])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_10  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/Madd_wr_data_count_i_add0000_cy[9] ),
    .Q(wr_data_count_5[10])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_9  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000 [9]),
    .Q(wr_data_count_5[9])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_8  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000 [8]),
    .Q(wr_data_count_5[8])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_7  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000 [7]),
    .Q(wr_data_count_5[7])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_6  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000 [6]),
    .Q(wr_data_count_5[6])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_5  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000 [5]),
    .Q(wr_data_count_5[5])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_4  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000 [4]),
    .Q(wr_data_count_5[4])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_3  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000 [3]),
    .Q(wr_data_count_5[3])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_2  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000 [2]),
    .Q(wr_data_count_5[2])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_1  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_add0000 [1]),
    .Q(wr_data_count_5[1])
  );
  FDC #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/wr_data_count_i_0  (
    .C(wr_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .D(\BU2/U0/grf.rf/gl0.wr/gwdc1.wdcext/diff_wr_rd_tmp [0]),
    .Q(wr_data_count_5[0])
  );
  FDP #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_fb_i  (
    .C(wr_clk),
    .D(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_or0000 ),
    .PRE(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .Q(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_fb_i_94 )
  );
  FDP #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i  (
    .C(wr_clk),
    .D(\BU2/U0/grf.rf/gl0.wr/gwas.wsts/ram_full_i_or0000 ),
    .PRE(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1]),
    .Q(full)
  );
  FDCP #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_10  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [10]),
    .PRE(\BU2/data_count [0]),
    .Q(rd_data_count_4[10])
  );
  FDCP #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_9  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [9]),
    .PRE(\BU2/data_count [0]),
    .Q(rd_data_count_4[9])
  );
  FDCP #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_8  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [8]),
    .PRE(\BU2/data_count [0]),
    .Q(rd_data_count_4[8])
  );
  FDCP #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_7  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [7]),
    .PRE(\BU2/data_count [0]),
    .Q(rd_data_count_4[7])
  );
  FDCP #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_6  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [6]),
    .PRE(\BU2/data_count [0]),
    .Q(rd_data_count_4[6])
  );
  FDCP #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_5  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [5]),
    .PRE(\BU2/data_count [0]),
    .Q(rd_data_count_4[5])
  );
  FDCP #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_4  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [4]),
    .PRE(\BU2/data_count [0]),
    .Q(rd_data_count_4[4])
  );
  FDCP #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_3  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [3]),
    .PRE(\BU2/data_count [0]),
    .Q(rd_data_count_4[3])
  );
  FDCP #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_2  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [2]),
    .PRE(\BU2/data_count [0]),
    .Q(rd_data_count_4[2])
  );
  FDCP #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_1  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [1]),
    .PRE(\BU2/data_count [0]),
    .Q(rd_data_count_4[1])
  );
  FDCP #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_0  (
    .C(rd_clk),
    .CLR(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .D(\BU2/U0/grf.rf/gl0.rd/gr1.grdc2.rdc/rd_dc_i_mux0002 [0]),
    .PRE(\BU2/data_count [0]),
    .Q(rd_data_count_4[0])
  );
  FDP #(
    .INIT ( 1'b1 ))
  \BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i  (
    .C(rd_clk),
    .D(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_or0000 ),
    .PRE(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2]),
    .Q(\BU2/U0/grf.rf/gl0.rd/gras.rsts/ram_empty_fb_i_81 )
  );
  FDP #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/rstblk/wr_rst_reg_0  (
    .C(wr_clk),
    .D(\BU2/data_count [0]),
    .PRE(\BU2/U0/grf.rf/rstblk/wr_rst_comb ),
    .Q(\BU2/U0/grf.rf/rstblk/wr_rst_reg [0])
  );
  FDP #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/rstblk/wr_rst_reg_1  (
    .C(wr_clk),
    .D(\BU2/data_count [0]),
    .PRE(\BU2/U0/grf.rf/rstblk/wr_rst_comb ),
    .Q(\BU2/U0/grf.rf/rstblk/wr_rst_reg [1])
  );
  FDP #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/rstblk/rd_rst_reg_0  (
    .C(rd_clk),
    .D(\BU2/data_count [0]),
    .PRE(\BU2/U0/grf.rf/rstblk/rd_rst_comb ),
    .Q(\BU2/U0/grf.rf/rstblk/rd_rst_reg [0])
  );
  FDP #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/rstblk/rd_rst_reg_1  (
    .C(rd_clk),
    .D(\BU2/data_count [0]),
    .PRE(\BU2/U0/grf.rf/rstblk/rd_rst_comb ),
    .Q(\BU2/U0/grf.rf/rstblk/rd_rst_reg [1])
  );
  FDP #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/rstblk/rd_rst_reg_2  (
    .C(rd_clk),
    .D(\BU2/data_count [0]),
    .PRE(\BU2/U0/grf.rf/rstblk/rd_rst_comb ),
    .Q(\BU2/U0/grf.rf/rstblk/rd_rst_reg [2])
  );
  FDPE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/rstblk/rd_rst_asreg  (
    .C(rd_clk),
    .CE(\BU2/U0/grf.rf/rstblk/rd_rst_asreg_d1_67 ),
    .D(\BU2/data_count [0]),
    .PRE(rst),
    .Q(\BU2/U0/grf.rf/rstblk/rd_rst_asreg_71 )
  );
  FD #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/rstblk/wr_rst_asreg_d1  (
    .C(wr_clk),
    .D(\BU2/U0/grf.rf/rstblk/wr_rst_asreg_72 ),
    .Q(\BU2/U0/grf.rf/rstblk/wr_rst_asreg_d1_69 )
  );
  FDPE #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/rstblk/wr_rst_asreg  (
    .C(wr_clk),
    .CE(\BU2/U0/grf.rf/rstblk/wr_rst_asreg_d1_69 ),
    .D(\BU2/data_count [0]),
    .PRE(rst),
    .Q(\BU2/U0/grf.rf/rstblk/wr_rst_asreg_72 )
  );
  FD #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/rstblk/rd_rst_asreg_d1  (
    .C(rd_clk),
    .D(\BU2/U0/grf.rf/rstblk/rd_rst_asreg_71 ),
    .Q(\BU2/U0/grf.rf/rstblk/rd_rst_asreg_d1_67 )
  );
  FD #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/rstblk/wr_rst_asreg_d2  (
    .C(wr_clk),
    .D(\BU2/U0/grf.rf/rstblk/wr_rst_asreg_d1_69 ),
    .Q(\BU2/U0/grf.rf/rstblk/wr_rst_asreg_d2_70 )
  );
  FD #(
    .INIT ( 1'b0 ))
  \BU2/U0/grf.rf/rstblk/rd_rst_asreg_d2  (
    .C(rd_clk),
    .D(\BU2/U0/grf.rf/rstblk/rd_rst_asreg_d1_67 ),
    .Q(\BU2/U0/grf.rf/rstblk/rd_rst_asreg_d2_68 )
  );
  VCC   \BU2/XST_VCC  (
    .P(\BU2/N1 )
  );
  GND   \BU2/XST_GND  (
    .G(\BU2/data_count [0])
  );

// synthesis translate_on

endmodule

// synthesis translate_off

`timescale  1 ps / 1 ps

module glbl ();

    parameter ROC_WIDTH = 100000;
    parameter TOC_WIDTH = 0;

    wire GSR;
    wire GTS;
    wire PRLD;

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

// synthesis translate_on
