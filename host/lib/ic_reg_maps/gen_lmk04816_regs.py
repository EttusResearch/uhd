#Copyright 2010,2015 Ettus Research LLC
#Copyright 2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0-or-later
#

########################################################################
# Template for raw text data describing registers
# name addr[bit range inclusive] default optional enums
########################################################################
REGS_TMPL="""\
########################################################################
## address 0
########################################################################
address0		 0[0:4]     0          
CLKout0_1_DIV	         0[5:15]    25          
CLKout0_1_HS             0[16]      0          
RESET                    0[17]      0          no_reset, reset
CLKout0_1_DDLY	         0[18:27]   0          
CLKout0_ADLY_SEL         0[28]      0          d_pd, d_ev_x, d_odd_y, d_both
CLKout1_ADLY_SEL	 0[29]      0          d_pd, d_ev_x, d_odd_y, d_both
Required_0		 0[30]      0         
CLKout0_1_PD             0[31] 	    1	       power_up, power_down 
########################################################################
## address 1
########################################################################
address1		 1[0:4]     1          
CLKout2_3_DIV	         1[5:15]    25         
CLKout2_3_HS             1[16]      0          
Powerdown                1[17]      0          normal, disabled
CLKout2_3_DDLY	         1[18:27]   0          
CLKout2_ADLY_SEL         1[28]      0          d_pd, d_ev_x, d_odd_y, d_both
CLKout3_ADLY_SEL	 1[29]      0          d_pd, d_ev_x, d_odd_y, d_both
Required_1		 1[30]      0         
CLKout2_3_PD             1[31] 	    1	       power_up, power_down
########################################################################
## address 2
########################################################################
address2		 2[0:4]     2          
CLKout4_5_DIV	         2[5:15]    25         
CLKout4_5_HS             2[16]      0          
Required_2_17            2[17]      0          
CLKout4_5_DDLY	         2[18:27]   0          
CLKout4_ADLY_SEL         2[28]      0          d_pd, d_ev_x, d_odd_y, d_both
CLKout5_ADLY_SEL	 2[29]      0          d_pd, d_ev_x, d_odd_y, d_both
Required_2_30		 2[30]      0         
CLKout4_5_PD             2[31] 	    1	       power_up, power_down
########################################################################
## address 3
########################################################################
address3		 3[0:4]     3          
CLKout6_7_DIV	         3[5:15]    1          
CLKout6_7_HS             3[16]      0         
Required_3_17            3[17]      0         
CLKout6_7_DDLY	         3[18:27]   0          
CLKout6_ADLY_SEL         3[28]      0          d_pd, d_ev_x, d_odd_y, d_both
CLKout7_ADLY_SEL	 3[29]      0          d_pd, d_ev_x, d_odd_y, d_both
CLKout6_7_OSCin_Sel      3[30]      1          VCO,OSCin
CLKout6_7_PD             3[31] 	    0	       power_up, power_down
########################################################################
## address 4
########################################################################
address4		 4[0:4]     4          
CLKout8_9_DIV	         4[5:15]    25         
CLKout8_9_HS             4[16]      0          
Required_4_17            4[17]      0          
CLKout8_9_DDLY	         4[18:27]   0          
CLKout8_ADLY_SEL         4[28]      0          d_pd, d_ev_x, d_odd_y, d_both
CLKout9_ADLY_SEL	 4[29]      0          d_pd, d_ev_x, d_odd_y, d_both
CLKout8_9_OSCin_Sel      4[30]      0	       VCO, OSCin         
CLKout8_9_PD             4[31] 	    0	       power_up, power_down
########################################################################
## address 5
########################################################################
address5		 5[0:4]     5          
CLKout10_11_DIV	         5[5:15]    25
CLKout10_11_HS           5[16]      0          
Required_5_17            5[17]      0          
CLKout10_11_DDLY	 5[18:27]   0          
CLKout10_ADLY_SEL        5[28]      0          d_pd, d_ev_x, d_odd_y, d_both
CLKout11_ADLY_SEL	 5[29]      0          d_pd, d_ev_x, d_odd_y, d_both
Required_5_30		 5[30]      0         
CLKout10_11_PD           5[31] 	    1	       normal, power_down
########################################################################
## address 6
########################################################################
<% CLKoutX_TYPE_ENUMS = "p_down=0, LVDS=1, LVPECL_700mVpp=2, LVPECL_1200mVpp=3, LVPECL_1600mVpp=4, LVPECL_200mVpp=5, LVCMOS=6, LVCMOS_IN=7, LVCMOS_NN=8, LVCMOS_II=9, LVCMOS_LN=10, LVCMOS_LI=11, LVCMOS_NL=12, LVCMOS_IL=13, LVCMOS_LL=1" %>\
address6		 6[0:4]     6          
CLKout0_1_ADLY	         6[5:9]     0          
Required_6_10	         6[10]      0          
CLKout2_3_ADLY           6[11:15]   0         
CLKout0_TYPE	         6[16:19]   0       ${CLKoutX_TYPE_ENUMS}
CLKout1_TYPE             6[20:23]   0       ${CLKoutX_TYPE_ENUMS}
CLKout2_TYPE	         6[24:27]   0       ${CLKoutX_TYPE_ENUMS}
CLKout3_TYPE	         6[28:31]   0       ${CLKoutX_TYPE_ENUMS}
########################################################################
## address 7
########################################################################
address7	                 7[0:4]     7          
CLKout4_5_ADLY	         7[5:9]     0          
Required_7_10	         7[10]      0          
CLKout6_7_ADLY           7[11:15]   0         
CLKout4_TYPE	         7[16:19]   0          ${CLKoutX_TYPE_ENUMS}
CLKout5_TYPE	         7[20:23]   0          ${CLKoutX_TYPE_ENUMS}
CLKout6_TYPE	         7[24:27]   0          ${CLKoutX_TYPE_ENUMS}
CLKout7_TYPE	         7[28:31]   0          ${CLKoutX_TYPE_ENUMS}
########################################################################
## address 8
########################################################################
address8		 8[0:4]     8          
CLKout8_9_ADLY	         8[5:9]     0          
Required_8_10	         8[10]      0          
CLKout10_11_ADLY         8[11:15]   0          ${CLKoutX_TYPE_ENUMS}
CLKout8_TYPE	         8[16:19]   0          ${CLKoutX_TYPE_ENUMS}
CLKout9_TYPE	         8[20:23]   0          ${CLKoutX_TYPE_ENUMS}
CLKout10_TYPE	         8[24:27]   0          ${CLKoutX_TYPE_ENUMS}
CLKout11_TYPE	         8[28:31]   0          ${CLKoutX_TYPE_ENUMS}
########################################################################
## address 9
########################################################################
address9		 9[0:4]     9          
Required_9_5	         9[5]       0          
Required_9_6	         9[6]       1          
Required_9_7             9[7]       0         
Required_9_8	         9[8]       1          
Required_9_9             9[9]       0          
Required_9_10	         9[10]      1          
Required_9_11		 9[11]      0
Required_9_12		 9[12]      1
Required_9_13		 9[13]      0
Required_9_14		 9[14]      1
Required_9_15            9[15]      0
Required_9_16            9[16]      1
Required_9_17            9[17]      0
Required_9_18            9[18]      1          
Required_9_19            9[19]      0          
Required_9_20            9[20]      1         
Required_9_21            9[21]      0          
Required_9_22            9[22]      1          
Required_9_23            9[23]      0          
Required_9_24            9[24]      1
Required_9_25            9[25]      0
Required_9_26            9[26]      1
Required_9_27            9[27]      0
Required_9_28            9[28]      1
Required_9_29            9[29]      0
Required_9_30            9[30]      1
Required_9_31            9[31]      0
########################################################################
## address 10
########################################################################
address10                10[0:4]    10          
FEEDBACK_MUX             10[5:7]    0          
VCO_DIV                  10[8:10]   2           
EN_FEEDBACK_MUX          10[11]     0           powered_down, enabled
VCO_MUX                  10[12]     0           just_vco, vco_divider
Required_10_13           10[13]     0          
Required_10_14           10[14]     1          
Required_10_15           10[15]     0
OSCout_DIV               10[16:18]  0		 
PD_OSCin                 10[19]     0		 normal, power_down
OSCout10_MUX             10[20]     0		 bypass_div, divided
Required_10_21           10[21]     0		 
EN_OSCout0               10[22]     1		 disabled, enabled
Required_10_23           10[23]     0
OSCout0_TYPE             10[24:27]  0
Required_10_28           10[28]     1
Required_10_range        10[29:31]  0
########################################################################
## address 11
########################################################################
address11		 11[0:4]    11
EN_PLL2_XTAL 		 11[5]      0		osc_disabled=0,osc_enabled=1
Required_11		 11[6:11]   0           
SYNC_TYPE                11[12:14]  1		input=0,in_pull_up=1,in_pull_down=2,out_push_pull=3,out_inverted=4,out_open_sources=5,out_open_drains=6
SYNC_EN_AUTO             11[15]     0		man_sync=0,sync_int_gen=1
SYNC_POL_INV             11[16]     1           sync_high=0,sync_low=1
SYNC_QUAL                11[17]     0           not_qual=0,fb_mux=1
SYNC_CLKin2_MUX          11[18:19]  0           log_low=0,CLKin2_LOS=1,CLKin2_Selected=2,uWire_RB=3
NO_SYNC_CLKout0_1        11[20]     0           clock_xy_sync=0,clock_xy_nosync=1
NO_SYNC_CLKout2_3        11[21]     0           clock_xy_sync=0,clock_xy_nosync=1
NO_SYNC_CLKout4_5        11[22]     1           clock_xy_sync=0,clock_xy_nosync=1
NO_SYNC_CLKout6_7        11[23]     1           clock_xy_sync=0,clock_xy_nosync=1
NO_SYNC_CLKout8_9        11[24]     0		clock_xy_sync=0,clock_xy_nosync=1
NO_SYNC_CLKout10_11      11[25]     0		clock_xy_sync=0,clock_xy_nosync=1
EN_SYNC                  11[26]     1           disable=0,enable=1
MODE                     11[27:31]  0		dual_int=0, dual_int_zer_delay=2,dual_ext=3,dual_ext_zer_delay=5,pll_two_int=6,pll_two_int_zer_delay=8,pll_two_ext=11,clock_dist=16
########################################################################
## address 12
########################################################################
address12		 12[0:4]    12
HOLDOVER_MODE		 12[6:7]    2		disabled=1,enabled=2
EN_TRACK		 12[8]      1		disabled=0,enabled=1
Required_12_range917     12[9:17]   0
Required_12_range1819	 12[18:19]  1
Required_12_20           12[20]     0
Required_LE_12           12[21]     0		
SYNC_PLL1_DLD            12[22]     0		sync_mode_noforce=0,sync_mode_force=1
SYNC_PLL2_DLD            12[23]     0		sync_mode_noforce=0,sync_mode_force=1
LD_TYPE                  12[24:26]  3           out_push_pull=3,out_inverted=4,out_open_sources=5,out_open_drains=6
LD_MUX                   12[27:31]  3		log_low=0,pll1_dld=1,pll2_dld=2,both=3,holdover=4,locked_dac=5,uWire_RB=7,rail=8,low=9,high=10,pll1_N=11,pll1_Nhalf=12,pll2_N=13,pll2_Nhalf=14,pll1_R=15,pll1_Rhalf=16,pll2_R=17,pll2_Rhalf=18
########################################################################
## address 13
########################################################################
address13		 13[0:4]    13
EN_CLKin0		 13[5]      1		no_valid_use=0,valid_use=1 
EN_CLKin1                13[6]      1		no_valid_use=0,valid_use=1 
EN_CLKin2                13[7]      1		no_valid_use=0,valid_use=1		
CLKin_Sel_INV            13[8]      0		active_high=0,active_low=1
CLKin_Select_MODE        13[9:11]   3		CLKin0_man=0,CLKin1_man=1,CLKin2_man=2,pin_sel_mode=3,auto_mode=4,auto_mode_w_next_block_sel=6
Status_CLKin0_MUX        13[14:12]  0		log_low=0,CLKin0_LOS=1,CLKin0_Selected=2,dac_lock=3,dac_low=4,dac_high=5,uWire_RB=6
DISABLE_DLD1_DET         13[15]     0		pll1_dld_cause_event=0,pll1_dld_not_cause=1
Status_CLKin0_TYPE       13[16:18]  2		input=0,in_pull_up=1,in_pull_down=2,out_push_pull=3,out_inverted=4,out_open_sources=5,out_open_drains=6
Required_13_19           13[19]     0
Status_CLKin1_MUX        13[20:22]  0		log_low=0,CLKin1_LOS=1,CLKin1_Selected=2,DAC_lock=3,DAC_low=4,DAC_high=5,uWire_RB=6
Required_19_23           13[23]     0
HOLDOVER_TYPE            13[24:26]  3		out_push_pull=3,out_inverted=4,out_open_sources=5,out_open_drains=6   
HOLDOVER_MUX             13[27:31]  7		log_low=0,pll1_dld=1,pll2_dld=2,both=3,holdover=4,locked_dac=5,uWire_RB=7,rail=8,low=9,high=10,pll1_N=11,pll1_Nhalf=12,pll2_N=13,pll2_Nhalf=14,pll1_R=15,pll1_Rhalf=16,pll2_R=17,pll2_Rhalf=18
########################################################################
## address 14
########################################################################
address14		 14[0:4]    14
EN_VTUNE_RAIL_DET	 14[5]      0		disabled=0,enabled=1
DAC_LOW_TRIP             14[6:11]   0
Required_14_1213         14[12:13]  0
DAC_HIGH_TRIP            14[14:19]  0				
CLKin0_BUF_TYPE          14[20]     0		bipolar=0,cmos=1
CLKin1_BUF_TYPE          14[21]     0		bipolar=0,cmos=1
CLKin2_BUF_TYPE          14[22]     0		bipolar=0,cmos=1
Required_14_23           14[23]     0		
Status_CLKin1_TYPE       14[24:26]  2		input=0,in_pull_up=1,in_pull_down=2,out_push_pull=3,out_inverted=4,out_open_sources=5,out_open_drains=6
Required_14_27           14[27]     0		
EN_LOS                   14[28]     1		disable_timeout=0,enable_timeout=1
Required_14_29           14[29]     0
LOS_TIMEOUT              14[30:31]  0		1200ns_at_4p2KHz=0,206ns_at_2p5MHz=1,52p9at10MHz=2,23p7_at_22MHz=3
########################################################################
## address 15
########################################################################
address15 		 15[0:4]    15
FORCE_HOLDOVER           15[5]      0		disabled=0,enabled=1
HOLDOVER_DLD_CNT         15[6:19]   512		
EN_MAN_DAC               15[20]     0		disabled=0,enabled=1
Required_15              15[21]     0
MAN_DAC                  15[22:31]  512
########################################################################
## address 16
########################################################################
address16                16[0:4]     16
Required_16_5            16[5]       0          
Required_16_6            16[6]       0          
Required_16_7            16[7]       0         
Required_16_8            16[8]       0          
Required_16_9            16[9]       0          
Required_16_10           16[10]      1          
Required_16_11           16[11]      0
Required_16_12           16[12]      0
Required_16_13           16[13]      0
Required_16_14           16[14]      0
Required_16_15           16[15]      0
Required_16_16           16[16]      1
Required_16_17           16[17]      0
Required_16_18           16[18]      1          
Required_16_19           16[19]      0          
Required_16_20           16[20]      1         
Required_16_21           16[21]      0          
Required_16_22           16[22]      1          
Required_16_23           16[23]      0  
Required_16_24           16[24]      1
Required_16_25           16[25]      0
Required_16_26           16[26]      0
Required_16_27           16[27]      0
Required_16_28           16[28]      0
Required_16_29           16[29]      0
XTAL_LVL		 16[30:31]   0		1p65VPP=0, 1p75VPP=1, 1p90VPP=2, 2p05VPP=3
########################################################################
## address 24
########################################################################
address24		 24[0:4]     24
PLL1_WND_SIZE		 24[6:7]     3		5p5ns=0, 10ns=1, 18p6=2, 40ns=3
PLL1_R_DLY               24[8:10]    0		0ps=0, 205ps=1, 410ps=2, 615ps=3, 820ps=4, 1025ps=5, 1230ps=6, 1345ps=7	
Required_24_11           24[11]      0
PLL2_N_DLY		 24[12:14]   0		0ps=0, 205ps=1, 410ps=2, 615ps=3, 820ps=4, 1025ps=5, 1230ps=6, 1345ps=7
Required_24_15           24[15]      0
PLL2_R3_LF               24[16:18]   0		200ohm=0, 1kilo_ohm=1, 2kilo_ohm=2, 4kilo_ohm=3, 16kilo_ohm=4
Required_24_19           24[19]      0
PLL2_R4_LF               24[20:22]   0		200ohm=0, 1kilo_ohm=1, 2kilo_ohm=2, 4kilo_ohm=3, 16kilo_ohm=4
Required_24_23           24[23]      0		
PLL2_C3_LF               24[24:27]   0		10pF=0, 11pF=1, 15pF=2, 16pF=3, 19pF=4, 20pF=5, 24pF=6, 25pF=7, 29pF=8, 30pF=9, 33pF=10, 34pF=11, 38pF=12, 39pF=13
PLL2_C4_LF               24[28:31]   0		10pF=0, 15pF=1, 29pF=2, 34pF=3, 47pF=4, 52pF=5, 66pF=6, 71pF=7, 103pF=8, 108pF=9, 122pF=10, 126pF=11, 141pF=12, 146pF=13
#########################################################################
## address 25
#########################################################################
addres25		 25[0:4]     25
PLL2_CP_TRI_25		 25[5]       0
PLL1_DLD_CNT_25          25[6:19]    1024
Required_25              25[20:21]   0
DAC_CLK_DIV		 25[22:31]   4
#########################################################################
## address 26
#########################################################################
address26		 26[0:4]     26
PLL2_CP_TRI_26           26[5]       0		PLL2_cpout2_active=0,PLL2cpout2T=1
PLL2_DLD_CNT_26		 26[6:19]    81
Required_26_20           26[20]      0
Required_26_21           26[21]      1
Required_26_22           26[22]      0
Required_26_23           26[23]      1
Required_26_24           26[24]      1
Required_26_25           26[25]       1
PLL2_CP_GAIN_26          26[26:27]   3		100uA=0, 400uA=1, 1600uA=2, 3200uA=3
PLL2_CP_POL_26           26[28]      0		neg_slope=0, pos_slope=1
EN_PLL2_REF_2X           26[29]      0		normal_freq_ref=0, doubled_freq_ref=1
PLL2_WND_SIZE            26[30:31]   2		3p7ns=2
#########################################################################
## address 27
#########################################################################
address27		 27[0:4]     27
PLL1_CP_TRI_27           27[5]       0	       PLL2_cpout2_active=0,PLL2_cpout2TRI=1
PLL1_R_27                27[6:19]    96
CLKin0_PreR_DIV		 27[20:21]   0         div1=0,div2=1,div4=2,div8=3
CLKin1_PreR_DIV          27[22:23]   0         div1=0,div2=1,div4=2,div8=3
CLKin2_PreR_DIV          27[24:25]   0         div1=0,div2=1,div4=2,div8=3
PLL1_CP_GAIN_27          27[26:27]   0	       100uA=0, 200uA=1, 400uA=2, 1600uA=3
PLL1_CP_POL_27           27[28]      1	       neg_slove=0, pos_slope=1
Reqiured_27              27[29:31]   0
#########################################################################
## address 28
#########################################################################
address28                28[0:4]     28
Required_28              28[5]       0
PLL1_N_28                28[6:19]    192
PLL2_R_28                28[20:31]   4
#########################################################################
## address 29
#########################################################################
address29                29[0:4]     29
PLL2_N_CAL_29            29[5:22]    48
PLL2_FAST_PDF_29         29[23]      1		100MHz_less=0, 100MHz_more=1
OSCin_FREQ_29            29[24:26]   1		zero_to_63MHz=0, 63_to_127MHz=1, 127_to_255MHz=2, 255_to_400MHz=4
Required29               29[27:31]   0
#########################################################################
## address 30
#########################################################################
address30		 30[0:4]     30
PLL2_N_30                30[5:22]    48
Required_30_522          30[23]      0		
PLL2_P_30                30[24:26]   2		div_8=0, div_2=1, div_2a=2, div_3=3, div_4=4, div_5=5, div_6=6, div_7=7
Required_2731            30[27:31]   0
#########################################################################
## address 31
#########################################################################
address31                31[0:4]     31
uWire_LOCK               31[5]       0		reg_locked=0, reg_unlocked=1
Required_615             31[6:15]    0
READBACK_ADDR            31[16:20]   31		R0=0, R1=1, R2=2, R3=3, R4=4, R5=5, R6=6, R7=7, R8=8, R10=10, R11=11, R12=12, R13=13, R14=14, R15=15, R24=24, R25=25, R26=26, R27=27, R28=28, R29=29, R30=30, R31=31
READBACK_LE              31[21]      0		LE_low=0, LE_high=1
Required_2231            31[22:31]   0
""" 
########################################################################
# Template for methods in the body of the struct
########################################################################

BODY_TMPL = """\




uint32_t get_reg(int addr){
    uint32_t reg = 0;
    switch(addr){
    % for addr in sorted(set(map(lambda r: r.get_addr(), regs))):
    case ${addr}:
        % for reg in filter(lambda r: r.get_addr() == addr, regs):
        reg |= (uint32_t(${reg.get_name()}) & ${reg.get_mask()}) << ${reg.get_shift()};
        % endfor
        break;
    % endfor
    }
    return reg;
}
"""

if __name__ == '__main__':
    import common; common.generate(
        name='lmk04816_regs',
        regs_tmpl=REGS_TMPL,
        body_tmpl=BODY_TMPL,
        file=__file__,
    )

