// $Header: /devl/xcs/repo/env/Databases/CAEInterfaces/verunilibs/data/rainier/PLL_ADV.v,v 1.43.4.1 2007/12/07 01:25:16 yanx Exp $
///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1995/2004 Xilinx, Inc.
// All Right Reserved.
///////////////////////////////////////////////////////////////////////////////
//   ____  ____
//  /   /\/   /
// /___/  \  /    Vendor : Xilinx
// \   \   \/     Version : 10.1
//  \   \         Description : Xilinx Function Simulation Library Component
//  /   /                  Phase Lock Loop Clock
// /___/   /\     Filename : PLL_ADV.v
// \   \  /  \    Timestamp : Thu Mar 25 16:44:07 PST 2004
//  \___\/\___\
//
// Revision:
//    03/15/05 - Initial version.
//    10/14/05 - Add REL pin.
//    11/07/05 - Add PMCD.
//    12/02/05 - Change parameter default values. Add DRP read/write.
//    12/22/05 - CR 222805 222809 fix.
//    01/03/06 - Change RST_DEASSER_CLK value to CLKIN1 and CLKFB (BT#735).
//    01/11/06 - Remove GSR from reset logic of PLL (CR 223099).
//    01/26/06 - Add reset to locked logic (CR224502).
//    02/16/06 - Support -360 6o +360 phase shifting (CR 225765)
//    03/10/06 - Add parameter type declaration (CR 226003)
//    03/17/06 - Using assign/deassign to reset pll_locked_tmp2  and reduce
//               lock time by MD (CR 224502).
//    04/19/06 - Change i to i1 and i2 in clkvco_lk. (CR230260).
//    07/17/06 - Remove i2 and first 4 clkvco_lk cycle generation (CR234931).
//    08/23/06 - Use clkout_en_tmp to generate clkout_en0; Use block statement to 
//               reset clock stop counter and flag. (CR422250)
//    09/19/06 - md_product update (CR 424286).
//    09/27/06 - Add error check for RESET_ON_LOSS_OF_LOCK (CR 425255).
//    11/10/06 - Keep 3 digits for real in duty cycle check function. (CR 428703).
//    01/12/07 - Add CLKOUT_DESKEW_ADJUST parameters (CR 432189).
//    03/30/07 - Fix error message for CLKSEL change in RST=0 (CR 436927).
//    04/09/07 - Enhance error message for RESET_ON_LOSS_OF_LOCK (CR 437405).
//    04/10/07 - Using assign/deassign to reset signals with # delay (CR 437660).
//    05/22/07 - Add setup check for REL (438781).
//    06/04/07 - Add wire declaration to internal signal.
//    06/8/07  - Generate clkfb_tst when GSR=0; 
//             - Chang VCOCLK_FREQ_MAX and VCOCLK_FREQ_MIN to parameter for simprim (BT1485).
//    06/18/07 - Improve error message for VCO (CR ). Initialize DRP mem (CR ).
//             - Add CLKFBIN pulse width check (BT1476).
//    06/28/07 - Initial DRP memory (CR 434042), Error message improve (CR 438250).
//    07/11/07 - change daddr_in to 5 bits (CR 443757).
//    08/02/07 - Remove numbers from CLKOUT DESKEW_ADJUST check (CR443161).
//    08/21/07 - Not check CLKIN period when PMCD mode set (445101).
//               Fix DUTY_CYCLE_MAX formula in case of divider larger than O_MAX_HT_LT (CR445945).
//               Add warning if phase shift over pll ability (63 vco) (CR446037).
//    09/20/07 - Seperate fb_delay and delay_edge to handle 0 fb_delay (CR448938)
//    10/23/07 - Add warnings to initial phase shift calculation (CR448965)
//    11/01/07 - Remove zero check for CLKOUTx dly register bit15-8 (CR434042)
//    12/06/07 - Add I/O buf to simprim (CR456124)
// End Revision


`timescale  1 ps / 1 ps
`define PLL_LOCK_TIME 7


module PLL_ADV (
        CLKFBDCM,
        CLKFBOUT,
        CLKOUT0,
        CLKOUT1,
        CLKOUT2,
        CLKOUT3,
        CLKOUT4,
        CLKOUT5,
        CLKOUTDCM0,
        CLKOUTDCM1,
        CLKOUTDCM2,
        CLKOUTDCM3,
        CLKOUTDCM4,
        CLKOUTDCM5,
        DO,
        DRDY,
        LOCKED,
        CLKFBIN,
        CLKIN1,
        CLKIN2,
        CLKINSEL,
        DADDR,
        DCLK,
        DEN,
        DI,
        DWE,
        REL,
        RST
);

parameter BANDWIDTH = "OPTIMIZED";
parameter CLKFBOUT_DESKEW_ADJUST = "NONE";
parameter CLKOUT0_DESKEW_ADJUST = "NONE";
parameter CLKOUT1_DESKEW_ADJUST = "NONE";
parameter CLKOUT2_DESKEW_ADJUST = "NONE";
parameter CLKOUT3_DESKEW_ADJUST = "NONE";
parameter CLKOUT4_DESKEW_ADJUST = "NONE";
parameter CLKOUT5_DESKEW_ADJUST = "NONE";
parameter integer CLKFBOUT_MULT = 1;
parameter real CLKFBOUT_PHASE = 0.0;
parameter real CLKIN1_PERIOD = 0.000;
parameter real CLKIN2_PERIOD = 0.000;
parameter integer CLKOUT0_DIVIDE = 1;
parameter real CLKOUT0_DUTY_CYCLE = 0.5;
parameter real CLKOUT0_PHASE = 0.0;
parameter integer CLKOUT1_DIVIDE = 1;
parameter real CLKOUT1_DUTY_CYCLE = 0.5;
parameter real CLKOUT1_PHASE = 0.0;
parameter integer CLKOUT2_DIVIDE = 1;
parameter real CLKOUT2_DUTY_CYCLE = 0.5;
parameter real CLKOUT2_PHASE = 0.0;
parameter integer CLKOUT3_DIVIDE = 1;
parameter real CLKOUT3_DUTY_CYCLE = 0.5;
parameter real CLKOUT3_PHASE = 0.0;
parameter integer CLKOUT4_DIVIDE = 1;
parameter real CLKOUT4_DUTY_CYCLE = 0.5;
parameter real CLKOUT4_PHASE = 0.0;
parameter integer CLKOUT5_DIVIDE = 1;
parameter real CLKOUT5_DUTY_CYCLE = 0.5;
parameter real CLKOUT5_PHASE = 0.0;
parameter COMPENSATION = "SYSTEM_SYNCHRONOUS";
parameter integer DIVCLK_DIVIDE = 1;
parameter EN_REL = "FALSE";
parameter PLL_PMCD_MODE = "FALSE";
parameter real REF_JITTER = 0.100;
parameter RESET_ON_LOSS_OF_LOCK = "FALSE";
parameter RST_DEASSERT_CLK = "CLKIN1";

localparam VCOCLK_FREQ_MAX = 1100;
localparam VCOCLK_FREQ_MIN = 400;

output CLKFBDCM;
output CLKFBOUT;
output CLKOUT0;
output CLKOUT1;
output CLKOUT2;
output CLKOUT3;
output CLKOUT4;
output CLKOUT5;
output CLKOUTDCM0;
output CLKOUTDCM1;
output CLKOUTDCM2;
output CLKOUTDCM3;
output CLKOUTDCM4;
output CLKOUTDCM5;
output DRDY;
output LOCKED;
output [15:0] DO;

input CLKFBIN;
input CLKIN1;
input CLKIN2;
input CLKINSEL;
input DCLK;
input DEN;
input DWE;
input REL;
input RST;
input [15:0] DI;
input [4:0] DADDR;

localparam VCOCLK_FREQ_TARGET = 800;
localparam CLKIN_FREQ_MAX = 1000;
localparam CLKIN_FREQ_MIN = 1;    //need check speed file, current is TBD
localparam CLKPFD_FREQ_MAX = 550;
localparam CLKPFD_FREQ_MIN = 1;   //need check speed file, current is TBD
localparam M_MIN = 1;
localparam M_MAX = 74;
localparam D_MIN = 1;
localparam D_MAX = 52;
localparam O_MIN = 1;
localparam O_MAX = 128;
localparam O_MAX_HT_LT = 64;
localparam REF_CLK_JITTER_MAX = 350;
localparam REF_CLK_JITTER_SCALE = 0.1;
localparam MAX_FEEDBACK_DELAY = 10.0;
localparam MAX_FEEDBACK_DELAY_SCALE = 1.0;

tri0 GSR = glbl.GSR;

reg [4:0] daddr_lat;
reg valid_daddr;
reg drdy_out;
reg drp_lock, drp_lock1;
reg [15:0] dr_sram [31:0];
reg [160:0] tmp_string;

wire CLKFBIN, CLKIN1, CLKIN2, CLKINSEL ;
wire rst_in, RST, orig_rst_in ;
wire locked_out;
wire clkvco_lk_rst;

reg clk0_out, clk1_out, clk2_out, clk3_out, clk4_out, clk5_out;
reg clkfb_out, clkfbm1_out;
reg clkout_en, clkout_en1, clkout_en0, clkout_en0_tmp;
integer clkout_cnt, clkin_cnt, clkin_lock_cnt;
integer clkout_en_time, locked_en_time, lock_cnt_max;
reg clkvco_lk, clkvco_free, clkvco;
reg fbclk_tmp;

reg rst_in1, rst_unlock, rst_on_loss;
time rst_edge, rst_ht;

reg fb_delay_found, fb_delay_found_tmp;
reg clkfb_tst;
real fb_delay_max;
time fb_delay, clkvco_delay, val_tmp, dly_tmp,  fbm1_comp_delay;
time clkin_edge, delay_edge;

real     period_clkin;
integer  clkin_period [4:0];
integer  period_vco, period_vco_half, period_vco_max, period_vco_min;
integer  period_vco1, period_vco2, period_vco3, period_vco4;
integer  period_vco5, period_vco6, period_vco7;
integer  period_vco_target, period_vco_target_half;
integer  period_fb, period_avg;

real    clkvco_freq_init_chk, clkfbm1pm_rl;
real    tmp_real;
integer i, j, i1, i2;
integer md_product, md_product_dbl, clkin_stop_max, clkfb_stop_max;

time pll_locked_delay, clkin_dly_t, clkfb_dly_t;
reg clkpll_dly, clkfbin_dly;
wire pll_unlock;
reg pll_locked_tmp1, pll_locked_tmp2;
reg lock_period;
reg pll_locked_tm, unlock_recover;
reg clkin_stopped_p, clkin_stopped_n;
reg clkfb_stopped_p, clkfb_stopped_n;
wire clkin_stopped, clkfb_stopped;
reg clkpll_jitter_unlock;
integer clkstop_cnt_p, clkstop_cnt_n, clkfbstop_cnt_p, clkfbstop_cnt_n;
integer  clkin_jit, REF_CLK_JITTER_MAX_tmp;

wire REL, DWE, DEN, DCLK, rel_o_mux_clk_tmp, clka1_in, clkb1_in;
wire init_trig, clkpll_tmp, clkpll, clk0in, clk1in, clk2in, clk3in, clk4in, clk5in;
wire clkfbm1in, clkfbm1ps_en;


reg clkout0_out;
reg clkout1_out;
reg clkout2_out;
reg clkout3_out;
reg clkout4_out;
reg clkout5_out;

reg clka1_out, clkb1_out, clka1d2_out, clka1d4_out, clka1d8_out;
reg clkdiv_rel_rst, qrel_o_reg1, qrel_o_reg2, qrel_o_reg3, rel_o_mux_sel;
reg pmcd_mode;
reg chk_ok;

wire rel_rst_o, rel_o_mux_clk;
wire clk0ps_en, clk1ps_en, clk2ps_en, clk3ps_en, clk4ps_en, clk5ps_en;

reg [7:0] clkout_mux;
reg [2:0] clk0pm_sel, clk1pm_sel, clk2pm_sel, clk3pm_sel, clk4pm_sel, clk5pm_sel;
reg [2:0] clkfbm1pm_sel;
reg clk0_edge, clk1_edge, clk2_edge, clk3_edge, clk4_edge, clk5_edge;
reg clkfbm1_edge, clkind_edge;
reg clk0_nocnt, clk1_nocnt, clk2_nocnt, clk3_nocnt, clk4_nocnt, clk5_nocnt;
reg clkfbm1_nocnt, clkind_nocnt;
reg clkind_edget, clkind_nocntt; 
reg [5:0] clk0_dly_cnt, clkout0_dly;
reg [5:0] clk1_dly_cnt, clkout1_dly;
reg [5:0] clk2_dly_cnt, clkout2_dly;
reg [5:0] clk3_dly_cnt, clkout3_dly;
reg [5:0] clk4_dly_cnt, clkout4_dly;
reg [5:0] clk5_dly_cnt, clkout5_dly;
reg [6:0] clk0_ht, clk0_lt;
reg [6:0] clk1_ht, clk1_lt;
reg [6:0] clk2_ht, clk2_lt;
reg [6:0] clk3_ht, clk3_lt;
reg [6:0] clk4_ht, clk4_lt;
reg [6:0] clk5_ht, clk5_lt;
reg [5:0] clkfbm1_dly_cnt, clkfbm1_dly;
reg [6:0] clkfbm1_ht, clkfbm1_lt;
reg [7:0] clkind_ht, clkind_lt;
reg [7:0] clkind_htt, clkind_ltt;
reg [7:0] clk0_ht1, clk0_cnt, clk0_div, clk0_div1;
reg [7:0] clk1_ht1, clk1_cnt, clk1_div, clk1_div1;
reg [7:0] clk2_ht1, clk2_cnt, clk2_div, clk2_div1;
reg [7:0] clk3_ht1, clk3_cnt, clk3_div, clk3_div1;
reg [7:0] clk4_ht1, clk4_cnt, clk4_div, clk4_div1;
reg [7:0] clk5_ht1, clk5_cnt, clk5_div, clk5_div1;
reg [7:0] clkfbm1_ht1, clkfbm1_cnt, clkfbm1_div, clkfbm1_div1;
reg [7:0]  clkind_div;
reg [3:0] pll_cp, pll_res;
reg [1:0] pll_lfhf;
reg [1:0] pll_cpres = 2'b01;

reg notifier;
wire [15:0] do_out, di_in;
wire clkin1_in, clkin2_in, clkfb_in, clkinsel_in, dwe_in, den_in, dclk_in;
wire [4:0] daddr_in;
wire rel_in, gsr_in, rst_input;

      assign #100 LOCKED = locked_out;
      assign #100 DRDY = drdy_out;
      assign #100 DO = do_out;
      assign clkin1_in = CLKIN1;
      assign clkin2_in = CLKIN2;
      assign clkfb_in = CLKFBIN;
      assign clkinsel_in = CLKINSEL;
      assign rst_input = RST;
      assign daddr_in = DADDR;
      assign di_in = DI;
      assign dwe_in = DWE;
      assign den_in = DEN;
      assign dclk_in = DCLK;
      assign rel_in = REL;



initial begin
    #1;
    if ($realtime == 0) begin
	$display ("Simulator Resolution Error : Simulator resolution is set to a value greater than 1 ps.");
	$display ("In order to simulate the PLL_ADV, the simulator resolution must be set to 1ps or smaller.");
	$finish;
    end
end

initial begin

        case (COMPENSATION)
                "SYSTEM_SYNCHRONOUS" : ;
                "SOURCE_SYNCHRONOUS" : ;
                "INTERNAL" : ;
                "EXTERNAL" : ;
                "DCM2PLL" : ;
                "PLL2DCM" : ;
                default : begin
                        $display("Attribute Syntax Error : The Attribute COMPENSATION on PLL_ADV instance %m is set to %s.  Legal values for this attribute are SYSTEM_SYNCHRONOUS, SOURCE_SYNCHRONOUS, INTERNAL, EXTERNAL, DCM2PLL or PLL2DCM.", COMPENSATION);
                        $finish;
                end
        endcase

        case (BANDWIDTH)
                "HIGH" : ;
                "LOW" : ;
                "OPTIMIZED" : ;
                default : begin
                        $display("Attribute Syntax Error : The Attribute BANDWIDTH on PLL_ADV instance %m is set to %s.  Legal values for this attribute are HIGH, LOW or OPTIMIZED.", BANDWIDTH);
                        $finish;
                end
        endcase

        case (CLKOUT0_DESKEW_ADJUST)
                "NONE" : ; 
                "PPC" : ;
                default : begin
                        $display("Attribute Syntax Error : The Attribute CLKOUT0_DESKEW_ADJUST on PLL_ADV instance %m is set to %s.  Legal values for this attribute are NONE or PPC.", CLKOUT0_DESKEW_ADJUST);
                        $finish;
                end
        endcase

        case (CLKOUT1_DESKEW_ADJUST)
                "NONE" : ; 
                "PPC" : ;
                 default : begin
                         $display("Attribute Syntax Error : The Attribute CLKOUT1_DESKEW_ADJUST on PLL_ADV instance %m is set to %s.  Legal values for this attribute are NONE or PPC .", CLKOUT1_DESKEW_ADJUST);
                        $finish;
                end
        endcase

        case (CLKOUT2_DESKEW_ADJUST)
                "NONE" : ; 
                "PPC" : ;
                 default : begin
                         $display("Attribute Syntax Error : The Attribute CLKOUT2_DESKEW_ADJUST on PLL_ADV instance %m is set to %s.  Legal values for this attribute are NONE or PPC.", CLKOUT2_DESKEW_ADJUST);
                        $finish;
                end
        endcase

        case (CLKOUT3_DESKEW_ADJUST)
                "NONE" : ; 
                "PPC" : ;
                 default : begin
                         $display("Attribute Syntax Error : The Attribute CLKOUT3_DESKEW_ADJUST on PLL_ADV instance %m is set to %s.  Legal values for this attribute are NONE or PPC.", CLKOUT3_DESKEW_ADJUST);
                        $finish;
                end
        endcase

        case (CLKOUT4_DESKEW_ADJUST)
                "NONE" : ; 
                "PPC" : ;
                 default : begin
                         $display("Attribute Syntax Error : The Attribute CLKOUT4_DESKEW_ADJUST on PLL_ADV instance %m is set to %s.  Legal values for this attribute are NONE or PPC.", CLKOUT4_DESKEW_ADJUST);
                        $finish;
                end
        endcase

        case (CLKOUT5_DESKEW_ADJUST)
                "NONE" : ; 
                "PPC" : ;
                 default : begin
                         $display("Attribute Syntax Error : The Attribute CLKOUT5_DESKEW_ADJUST on PLL_ADV instance %m is set to %s.  Legal values for this attribute are NONE or PPC.", CLKOUT5_DESKEW_ADJUST);
                        $finish;
                end
        endcase

        case (CLKFBOUT_DESKEW_ADJUST)
                "NONE" : ; 
                "PPC" : ;
                 default : begin
                         $display("Attribute Syntax Error : The Attribute CLKFBOUT_DESKEW_ADJUST on PLL_ADV instance %m is set to %s.  Legal values for this attribute are NONE or PPC.", CLKFBOUT_DESKEW_ADJUST);
                        $finish;
                end
        endcase


        case (PLL_PMCD_MODE)
             "TRUE" : pmcd_mode = 1'b1;
             "FALSE" : pmcd_mode = 1'b0;
                default : begin
                        $display("Attribute Syntax Error : The Attribute PLL_PMCD_MODE on PLL_ADV instance %m is set to %s.  Legal values for this attribute are FALSE or TRUE.", PLL_PMCD_MODE);
                        $finish;
                end
        endcase

        tmp_string = "CLKOUT0_DIVIDE"; 
        chk_ok = para_int_pmcd_chk(CLKOUT0_DIVIDE, tmp_string, 1, 128, pmcd_mode, 8);
        tmp_string = "CLKOUT0_PHASE";
        chk_ok = para_real_pmcd_chk(CLKOUT0_PHASE, tmp_string, -360.0, 360.0, pmcd_mode, 0.0);
        tmp_string = "CLKOUT0_DUTY_CYCLE";
        chk_ok = para_real_pmcd_chk(CLKOUT0_DUTY_CYCLE, tmp_string, 0.0, 1.0, pmcd_mode, 0.5);

        tmp_string = "CLKOUT1_DIVIDE";
        chk_ok = para_int_pmcd_chk(CLKOUT1_DIVIDE, tmp_string, 1, 128, pmcd_mode, 4);
        tmp_string = "CLKOUT1_PHASE";
        chk_ok = para_real_pmcd_chk(CLKOUT1_PHASE, tmp_string, -360.0, 360.0, pmcd_mode, 0.0);
        tmp_string = "CLKOUT1_DUTY_CYCLE";
        chk_ok = para_real_pmcd_chk(CLKOUT1_DUTY_CYCLE, tmp_string, 0.0, 1.0, pmcd_mode, 0.5);

        tmp_string = "CLKOUT2_DIVIDE";
        chk_ok = para_int_pmcd_chk(CLKOUT2_DIVIDE, tmp_string, 1, 128, pmcd_mode, 2);
        tmp_string = "CLKOUT2_PHASE";
        chk_ok = para_real_pmcd_chk(CLKOUT2_PHASE, tmp_string, -360.0, 360.0, pmcd_mode, 0.0);
        tmp_string = "CLKOUT2_DUTY_CYCLE";
        chk_ok = para_real_pmcd_chk(CLKOUT2_DUTY_CYCLE, tmp_string, 0.0, 1.0, pmcd_mode, 0.5);

        tmp_string = "CLKOUT3_DIVIDE";
        chk_ok = para_int_pmcd_chk(CLKOUT3_DIVIDE, tmp_string, 1, 128, pmcd_mode, 1);
        tmp_string = "CLKOUT3_PHASE";
        chk_ok = para_real_pmcd_chk(CLKOUT3_PHASE, tmp_string, -360.0, 360.0, pmcd_mode, 0.0);
        tmp_string = "CLKOUT3_DUTY_CYCLE";
        chk_ok = para_real_pmcd_chk(CLKOUT3_DUTY_CYCLE, tmp_string, 0.0, 1.0, pmcd_mode, 0.5);

        tmp_string = "CLKOUT4_DIVIDE";
        chk_ok = para_int_range_chk(CLKOUT4_DIVIDE, tmp_string,  1, 128);
        tmp_string = "CLKOUT4_PHASE";
        chk_ok = para_real_range_chk(CLKOUT4_PHASE, tmp_string,  -360.0, 360.0);
        tmp_string = "CLKOUT4_DUTY_CYCLE";
        chk_ok = para_real_range_chk(CLKOUT4_DUTY_CYCLE, tmp_string,  0.0, 1.0);

        tmp_string = "CLKOUT5_DIVIDE";
        chk_ok = para_int_range_chk (CLKOUT5_DIVIDE, tmp_string, 1, 128);
        tmp_string = "CLKOUT5_PHASE";
        chk_ok = para_real_range_chk(CLKOUT5_PHASE, tmp_string, -360.0, 360.0);
        tmp_string = "CLKOUT5_DUTY_CYCLE";
        chk_ok = para_real_range_chk (CLKOUT5_DUTY_CYCLE, tmp_string,  0.0, 1.0);

        tmp_string = "CLKFBOUT_MULT";
        chk_ok = para_int_pmcd_chk(CLKFBOUT_MULT, tmp_string, 1, 74, pmcd_mode, 1);
        tmp_string = "CLKFBOUT_PHASE";
        chk_ok = para_real_pmcd_chk(CLKFBOUT_PHASE, tmp_string, -360.0, 360.0, pmcd_mode, 0.0);
        tmp_string = "DIVCLK_DIVIDE";
        chk_ok = para_int_range_chk (DIVCLK_DIVIDE, tmp_string, 1, 52);

        tmp_string = "REF_JITTER";
        chk_ok = para_real_range_chk (REF_JITTER, tmp_string, 0.0, 0.999);
        if (((CLKIN1_PERIOD < 1.0) || (CLKIN1_PERIOD > 52.630)) && (pmcd_mode == 0)) begin
            $display("Attribute Syntax Error : CLKIN1_PERIOD is not in range 1.0 ... 52.630.");
        end 

          if (((CLKIN2_PERIOD < 1.0) || (CLKIN2_PERIOD > 52.630)) && (pmcd_mode == 0)) begin
            $display("Attribute Syntax Error : CLKIN1_PERIOD is not in range 1.0 ... 52.630.");
         end 


        case (RESET_ON_LOSS_OF_LOCK)
                "FALSE" : rst_on_loss = 1'b0;
//                "TRUE" : if (pmcd_mode) rst_on_loss = 1'b0; else rst_on_loss = 1'b1;
                default : begin
                        $display("Attribute Syntax Error : The Attribute RESET_ON_LOSS_OF_LOCK on PLL_ADV instance %m is set to %s.  This attribute must always be set to FALSE for X_PLL_ADV to function correctly. Please correct the setting for the attribute and re-run the simulation.", RESET_ON_LOSS_OF_LOCK);
                        $finish;
                end
        endcase

  case (CLKFBOUT_MULT)
 1  : if (BANDWIDTH === "LOW") begin pll_cp = 4'b0001; pll_res = 4'b1101; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "HIGH") begin pll_cp = 4'b0101; pll_res = 4'b1111; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "OPTIMIZED") begin pll_cp = 4'b0101; pll_res = 4'b1111; pll_lfhf = 2'b11; end
 2  : if (BANDWIDTH === "LOW") begin pll_cp = 4'b0001; pll_res = 4'b1110; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "HIGH") begin pll_cp = 4'b1110; pll_res = 4'b1111; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "OPTIMIZED") begin pll_cp = 4'b1110; pll_res = 4'b1111; pll_lfhf = 2'b11; end
 3  : if (BANDWIDTH === "LOW") begin pll_cp = 4'b0001; pll_res = 4'b0110; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "HIGH") begin pll_cp = 4'b1111; pll_res = 4'b0111; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "OPTIMIZED") begin pll_cp = 4'b0110; pll_res = 4'b0101; pll_lfhf = 2'b11; end
 4  : if (BANDWIDTH === "LOW") begin pll_cp = 4'b0001; pll_res = 4'b1010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "HIGH") begin pll_cp = 4'b1111; pll_res = 4'b1101; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "OPTIMIZED") begin pll_cp = 4'b0111; pll_res = 4'b1001; pll_lfhf = 2'b11; end
 5  : if (BANDWIDTH === "LOW") begin pll_cp = 4'b0001; pll_res = 4'b1100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "HIGH") begin pll_cp = 4'b1110; pll_res = 4'b0101; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "OPTIMIZED") begin pll_cp = 4'b1101; pll_res = 4'b1001; pll_lfhf = 2'b11; end
 6  : if (BANDWIDTH === "LOW") begin pll_cp = 4'b0001; pll_res = 4'b1100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "HIGH") begin pll_cp = 4'b1111; pll_res = 4'b0101; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "OPTIMIZED") begin pll_cp = 4'b0111; pll_res = 4'b0001; pll_lfhf = 2'b11; end
 7  : if (BANDWIDTH === "LOW") begin pll_cp = 4'b0001; pll_res = 4'b1100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "HIGH") begin pll_cp = 4'b1111; pll_res = 4'b1001; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "OPTIMIZED") begin pll_cp = 4'b1100; pll_res = 4'b0001; pll_lfhf = 2'b11; end
 8  : if (BANDWIDTH === "LOW") begin pll_cp = 4'b0001; pll_res = 4'b0010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "HIGH") begin pll_cp = 4'b1111; pll_res = 4'b1110; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "OPTIMIZED") begin pll_cp = 4'b1111; pll_res = 4'b1110; pll_lfhf = 2'b11; end
 9  : if (BANDWIDTH === "LOW") begin pll_cp = 4'b0001; pll_res = 4'b0010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "HIGH") begin pll_cp = 4'b1111; pll_res = 4'b1110; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "OPTIMIZED") begin pll_cp = 4'b1110; pll_res = 4'b0001; pll_lfhf = 2'b11; end
 10 : if (BANDWIDTH === "LOW") begin pll_cp = 4'b0001; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "HIGH") begin pll_cp = 4'b1111; pll_res = 4'b0001; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "OPTIMIZED") begin pll_cp = 4'b1111; pll_res = 4'b0001; pll_lfhf = 2'b11; end
 11 : if (BANDWIDTH === "LOW") begin pll_cp = 4'b0001; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "HIGH") begin pll_cp = 4'b1111; pll_res = 4'b0001; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "OPTIMIZED") begin pll_cp = 4'b1101; pll_res = 4'b0110; pll_lfhf = 2'b11; end
 12 : if (BANDWIDTH === "LOW") begin pll_cp = 4'b0001; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "HIGH") begin pll_cp = 4'b1110; pll_res = 4'b0110; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "OPTIMIZED") begin pll_cp = 4'b1110; pll_res = 4'b0110; pll_lfhf = 2'b11; end
 13 : if (BANDWIDTH === "LOW") begin pll_cp = 4'b0001; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "HIGH") begin pll_cp = 4'b1110; pll_res = 4'b0110; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "OPTIMIZED") begin pll_cp = 4'b1110; pll_res = 4'b0110; pll_lfhf = 2'b11; end
 14 : if (BANDWIDTH === "LOW") begin pll_cp = 4'b0001; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "HIGH") begin pll_cp = 4'b1111; pll_res = 4'b0110; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "OPTIMIZED") begin pll_cp = 4'b1111; pll_res = 4'b0110; pll_lfhf = 2'b11; end
 15 : if (BANDWIDTH === "LOW") begin pll_cp = 4'b0001; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "HIGH") begin pll_cp = 4'b1110; pll_res = 4'b1010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH === "OPTIMIZED") begin pll_cp = 4'b1110; pll_res = 4'b1010; pll_lfhf = 2'b11; end
 16 : if (BANDWIDTH === "LOW") begin pll_cp = 4'b0001; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b1110; pll_res = 4'b1010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b1110; pll_res = 4'b1010; pll_lfhf = 2'b11; end
 17 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0001; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b1111; pll_res = 4'b1010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b1111; pll_res = 4'b1010; pll_lfhf = 2'b11; end
 18 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0001; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b1111; pll_res = 4'b1010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b1111; pll_res = 4'b1010; pll_lfhf = 2'b11; end
 19 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0001; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b1111; pll_res = 4'b1010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b1111; pll_res = 4'b1010; pll_lfhf = 2'b11; end
 20 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0001; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b1111; pll_res = 4'b1010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b1100; pll_res = 4'b1100; pll_lfhf = 2'b11; end
 21 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0001; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b1111; pll_res = 4'b1010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b1100; pll_res = 4'b1100; pll_lfhf = 2'b11; end
 22 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0001; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b1101; pll_res = 4'b1100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b1101; pll_res = 4'b1100; pll_lfhf = 2'b11; end
 23 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0001; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b1101; pll_res = 4'b1100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b1101; pll_res = 4'b1100; pll_lfhf = 2'b11; end
 24 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0001; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b1101; pll_res = 4'b1100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0111; pll_res = 4'b0010; pll_lfhf = 2'b11; end
 25 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0001; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b1110; pll_res = 4'b1100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b1110; pll_res = 4'b1100; pll_lfhf = 2'b11; end
 26 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0001; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b1110; pll_res = 4'b1100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b1110; pll_res = 4'b1100; pll_lfhf = 2'b11; end
 27 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0001; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b1111; pll_res = 4'b1100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b1111; pll_res = 4'b1100; pll_lfhf = 2'b11; end
 28 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0001; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b1110; pll_res = 4'b1100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b1100; pll_res = 4'b0010; pll_lfhf = 2'b11; end
 29 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0001; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b1110; pll_res = 4'b1100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b1100; pll_res = 4'b0010; pll_lfhf = 2'b11; end
 30 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0001; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b1110; pll_res = 4'b1100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b1100; pll_res = 4'b0010; pll_lfhf = 2'b11; end
 31 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b1100; pll_res = 4'b0010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b1100; pll_res = 4'b0010; pll_lfhf = 2'b11; end
 32 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b1100; pll_res = 4'b0010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b1100; pll_res = 4'b0010; pll_lfhf = 2'b11; end
 33 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b1111; pll_res = 4'b1010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0100; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 34 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0111; pll_res = 4'b0010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0100; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 35 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0111; pll_res = 4'b0010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0100; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 36 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0111; pll_res = 4'b0010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0100; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 37 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0110; pll_res = 4'b0010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0100; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 38 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0110; pll_res = 4'b0010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0100; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 39 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0100; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0100; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 40 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0100; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0011; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 41 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0100; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0011; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 42 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0100; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0011; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 43 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0100; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0011; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 44 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0100; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0011; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 45 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0011; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0011; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 46 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0011; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0011; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 47 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0101; pll_res = 4'b0010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 48 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0101; pll_res = 4'b0010; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 49 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0011; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 50 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0011; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 51 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0011; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 52 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0011; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 53 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0011; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 54 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0011; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 55 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0011; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0011; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 56 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0011; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0011; pll_res = 4'b0100; pll_lfhf = 2'b11; end
 57 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 58 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 59 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 60 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 61 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 62 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 63 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 64 : if (BANDWIDTH == "LOW") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "HIGH") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
 else if (BANDWIDTH == "OPTIMIZED") begin pll_cp = 4'b0010; pll_res = 4'b1000; pll_lfhf = 2'b11; end
  endcase


   tmp_string = "DIVCLK_DIVIDE";
   chk_ok = para_int_range_chk (DIVCLK_DIVIDE, tmp_string, D_MIN, D_MAX);

   tmp_string = "CLKFBOUT_MULT";
   chk_ok = para_int_range_chk (CLKFBOUT_MULT, tmp_string, M_MIN, M_MAX);

   tmp_string = "CLKOUT0_DUTY_CYCLE";
   chk_ok = clkout_duty_chk (CLKOUT0_DIVIDE, CLKOUT0_DUTY_CYCLE, tmp_string);
   tmp_string = "CLKOUT1_DUTY_CYCLE";
   chk_ok = clkout_duty_chk (CLKOUT1_DIVIDE, CLKOUT1_DUTY_CYCLE, tmp_string);
   tmp_string = "CLKOUT2_DUTY_CYCLE";
   chk_ok = clkout_duty_chk (CLKOUT2_DIVIDE, CLKOUT2_DUTY_CYCLE, tmp_string);
   tmp_string = "CLKOUT3_DUTY_CYCLE";
   chk_ok = clkout_duty_chk (CLKOUT3_DIVIDE, CLKOUT3_DUTY_CYCLE, tmp_string);
   tmp_string = "CLKOUT4_DUTY_CYCLE";
   chk_ok = clkout_duty_chk (CLKOUT4_DIVIDE, CLKOUT4_DUTY_CYCLE, tmp_string);
   tmp_string = "CLKOUT5_DUTY_CYCLE";
   chk_ok = clkout_duty_chk (CLKOUT5_DIVIDE, CLKOUT5_DUTY_CYCLE, tmp_string);

   period_vco_max = 1000000 / VCOCLK_FREQ_MIN;
   period_vco_min = 1000000 / VCOCLK_FREQ_MAX;
   period_vco_target = 1000000 / VCOCLK_FREQ_TARGET;
   period_vco_target_half = period_vco_target / 2;
   fb_delay_max = MAX_FEEDBACK_DELAY * MAX_FEEDBACK_DELAY_SCALE;
   md_product = CLKFBOUT_MULT * DIVCLK_DIVIDE;
   md_product_dbl = md_product * 2;
   clkout_en_time = `PLL_LOCK_TIME + 2; 
//   locked_en_time = md_product_dbl +  clkout_en_time +2;  // for DCM 3 cycle reset requirement
   locked_en_time = md_product +  clkout_en_time + 2;  // for DCM 3 cycle reset requirement
   lock_cnt_max = locked_en_time + 6;
   clkfb_stop_max = 3;
   clkin_stop_max = DIVCLK_DIVIDE + 1;
   REF_CLK_JITTER_MAX_tmp = REF_CLK_JITTER_MAX;

   clk_out_para_cal (clk0_ht, clk0_lt, clk0_nocnt, clk0_edge, CLKOUT0_DIVIDE, CLKOUT0_DUTY_CYCLE);
   clk_out_para_cal (clk1_ht, clk1_lt, clk1_nocnt, clk1_edge, CLKOUT1_DIVIDE, CLKOUT1_DUTY_CYCLE);
   clk_out_para_cal (clk2_ht, clk2_lt, clk2_nocnt, clk2_edge, CLKOUT2_DIVIDE, CLKOUT2_DUTY_CYCLE);
   clk_out_para_cal (clk3_ht, clk3_lt, clk3_nocnt, clk3_edge, CLKOUT3_DIVIDE, CLKOUT3_DUTY_CYCLE);
   clk_out_para_cal (clk4_ht, clk4_lt, clk4_nocnt, clk4_edge, CLKOUT4_DIVIDE, CLKOUT4_DUTY_CYCLE);
   clk_out_para_cal (clk5_ht, clk5_lt, clk5_nocnt, clk5_edge, CLKOUT5_DIVIDE, CLKOUT5_DUTY_CYCLE);
   clk_out_para_cal (clkfbm1_ht, clkfbm1_lt, clkfbm1_nocnt, clkfbm1_edge, CLKFBOUT_MULT, 0.50);
   clk_out_para_cal (clkind_ht, clkind_lt, clkind_nocnt, clkind_edge, DIVCLK_DIVIDE, 0.50);
   tmp_string = "CLKOUT0_PHASE";
   clkout_dly_cal (clkout0_dly, clk0pm_sel, CLKOUT0_DIVIDE, CLKOUT0_PHASE, tmp_string);
   tmp_string = "CLKOUT1_PHASE";
   clkout_dly_cal (clkout1_dly, clk1pm_sel, CLKOUT1_DIVIDE, CLKOUT1_PHASE, tmp_string);
   tmp_string = "CLKOUT2_PHASE";
   clkout_dly_cal (clkout2_dly, clk2pm_sel, CLKOUT2_DIVIDE, CLKOUT2_PHASE, tmp_string);
   tmp_string = "CLKOUT3_PHASE";
   clkout_dly_cal (clkout3_dly, clk3pm_sel, CLKOUT3_DIVIDE, CLKOUT3_PHASE, tmp_string);
   tmp_string = "CLKOUT4_PHASE";
   clkout_dly_cal (clkout4_dly, clk4pm_sel, CLKOUT4_DIVIDE, CLKOUT4_PHASE, tmp_string);
   tmp_string = "CLKOUT5_PHASE";
   clkout_dly_cal (clkout5_dly, clk5pm_sel, CLKOUT5_DIVIDE, CLKOUT5_PHASE, tmp_string);
   tmp_string = "CLKFBOUT_PHASE";
   clkout_dly_cal (clkfbm1_dly, clkfbm1pm_sel, CLKFBOUT_MULT, CLKFBOUT_PHASE, tmp_string);
        
   clkind_div = DIVCLK_DIVIDE;

   dr_sram[5'b11100] = {8'bx, clk0_edge, clk0_nocnt, clkout0_dly[5:0]};
   dr_sram[5'b11011] = {clk0pm_sel[2:0], 1'b1, clk0_ht[5:0], clk0_lt[5:0]};
   dr_sram[5'b11010] = {8'bx, clk1_edge, clk1_nocnt, clkout1_dly[5:0]};
   dr_sram[5'b11001] = {clk1pm_sel[2:0], 1'b1, clk1_ht[5:0], clk1_lt[5:0]};
   dr_sram[5'b10111] = {8'bx, clk2_edge, clk2_nocnt, clkout2_dly[5:0]};
   dr_sram[5'b10110] = {clk2pm_sel[2:0], 1'b1, clk2_ht[5:0], clk2_lt[5:0]};
   dr_sram[5'b10101] = {8'bx, clk3_edge, clk3_nocnt, clkout3_dly[5:0]};
   dr_sram[5'b10100] = {clk3pm_sel[2:0], 1'b1, clk3_ht[5:0], clk3_lt[5:0]};
   dr_sram[5'b10011] = {8'bx, clk4_edge, clk4_nocnt, clkout4_dly[5:0]};
   dr_sram[5'b10010] = {clk4pm_sel[2:0], 1'b1, clk4_ht[5:0], clk4_lt[5:0]};
   dr_sram[5'b01111] = {8'bx, clk5_edge, clk5_nocnt, clkout5_dly[5:0]};
   dr_sram[5'b01110] = {clk5pm_sel[2:0], 1'b1, clk5_ht[5:0], clk5_lt[5:0]};
   dr_sram[5'b01101] = {8'bx, clkfbm1_edge, clkfbm1_nocnt, clkfbm1_dly[5:0]};
   dr_sram[5'b01100] = {clkfbm1pm_sel[2:0], 1'b1, clkfbm1_ht[5:0], clkfbm1_lt[5:0]};
   dr_sram[5'b00110] = {2'bx, clkind_edge, clkind_nocnt, clkind_ht[5:0], clkind_lt[5:0]};
   dr_sram[5'b00001] = {8'bx, pll_lfhf, pll_cpres, pll_cp};
   dr_sram[5'b00000] = {6'bx, pll_res, 6'bx};


// **** PMCD *******

//*** Clocks MUX

        case (RST_DEASSERT_CLK)
             "CLKIN1" : rel_o_mux_sel = 1'b1;
             "CLKFBIN" : rel_o_mux_sel = 1'b0;
            default : begin
                          $display("Attribute Syntax Error : The attribute RST_DEASSERT_CLK on PLL_ADV instance %m is set to %s.  Legal values for this attribute are CLKIN1 and CLKFBIN.", RST_DEASSERT_CLK);
                          $finish;
                      end
        endcase

//*** CLKDIV_RST
        case (EN_REL)
              "FALSE" : clkdiv_rel_rst = 1'b0;
              "TRUE" : clkdiv_rel_rst = 1'b1;
            default : begin
                          $display("Attribute Syntax Error : The attribute EN_REL on PLL_ADV instance %m is set to %s.  Legal values for this attribute are TRUE or FALSE.", EN_REL);
                          $finish;
                      end
        endcase


end

initial begin
  rst_in1 = 0;
  rst_unlock = 0;
  clkin_period[0] = 0;
  clkin_period[1] = 0;
  clkin_period[2] = 0;
  clkin_period[3] = 0;
  clkin_period[4] = 0;
  period_avg = 0;
  period_fb = 0;
  fb_delay = 0;
  clkfbm1_div = 1;
  clkfbm1_div1 = 0;
  clkvco_delay = 0;
  fbm1_comp_delay = 0;
  clkfbm1pm_rl = 0;
  period_vco = 0;
  period_vco1 = 0;
  period_vco2 = 0;
  period_vco3 = 0;
  period_vco4 = 0;
  period_vco5 = 0;
  period_vco6 = 0;
  period_vco7 = 0;
  period_vco_half = 0;
  fb_delay_found = 0;
  fb_delay_found_tmp = 0;
  clkin_edge = 0;
  delay_edge = 0;
  clkvco_free = 0;
  clkvco_lk = 0;
  fbclk_tmp = 0;
  clkfb_tst = 0;
  clkout_cnt = 0;
  clkout_en = 0;
  clkout_en0 = 0;
  clkout_en0_tmp = 0;
  clkout_en1 = 0;
  pll_locked_tmp1  = 0;
  pll_locked_tmp2  = 0;
  pll_locked_tm = 0;
  pll_locked_delay = 0;
  clkout_mux = 3'b0;
  unlock_recover = 0;
  clkstop_cnt_p = 0;
  clkstop_cnt_n = 0;
  clkpll_jitter_unlock = 0;
  clkin_jit = 0;
  clkin_cnt = 0;
  clkin_lock_cnt = 0;
  clkin_stopped_p = 0;
  clkin_stopped_n = 0;
  clkfb_stopped_p = 0;
  clkfb_stopped_n = 0;
  clkpll_dly = 0;
  clkfbin_dly = 0;
  clkfbstop_cnt_p = 0;
  clkfbstop_cnt_n = 0;
  lock_period = 0;
  rst_edge = 0;
  rst_ht = 0;
  drdy_out = 0;
  drp_lock = 0;
  clkout0_out = 0;
  clkout1_out = 0;
  clkout2_out = 0;
  clkout3_out = 0;
  clkout4_out = 0;
  clkout5_out = 0;
  clka1_out = 1'b0;
  clkb1_out = 1'b0;         
  clka1d2_out = 1'b0;
  clka1d4_out = 1'b0;
  clka1d8_out = 1'b0;
  qrel_o_reg1 = 1'b0;
  qrel_o_reg2 = 1'b0;
  qrel_o_reg3 = 1'b0;
  clk0_dly_cnt = 6'b0;
  clk1_dly_cnt = 6'b0;
  clk2_dly_cnt = 6'b0;
  clk3_dly_cnt = 6'b0;
  clk4_dly_cnt = 6'b0;
  clk5_dly_cnt = 6'b0;
  clkfbm1_dly_cnt = 6'b0;
  clk0_cnt = 8'b0;
  clk1_cnt = 8'b0;
  clk2_cnt = 8'b0;
  clk3_cnt = 8'b0;
  clk4_cnt = 8'b0;
  clk5_cnt = 8'b0;
  clkfbm1_cnt = 8'b0;
  clk0_out = 0;
  clk1_out = 0;
  clk2_out = 0;
  clk3_out = 0;
  clk4_out = 0;
  clk5_out = 0;
  clkfb_out = 0;
  clkfbm1_out = 0;
end

// PMCD function

//*** asyn RST
    always @(orig_rst_in) 
        if (orig_rst_in == 1'b1) begin
            assign qrel_o_reg1 = 1'b1;
            assign qrel_o_reg2 = 1'b1;
            assign qrel_o_reg3 = 1'b1;
        end
        else if (orig_rst_in == 1'b0) begin
            deassign qrel_o_reg1;
            deassign qrel_o_reg2;
            deassign qrel_o_reg3;
        end

//*** Clocks MUX

    assign rel_o_mux_clk_tmp = rel_o_mux_sel ? clkin1_in : clkfb_in;
    assign rel_o_mux_clk = (pmcd_mode) ? rel_o_mux_clk_tmp : 0;
    assign clka1_in = (pmcd_mode) ? clkin1_in : 0;
    assign clkb1_in = (pmcd_mode) ? clkfb_in : 0;


//*** Rel and Rst
    always @(posedge rel_o_mux_clk) 
        qrel_o_reg1 <= 1'b0;

    always @(negedge rel_o_mux_clk) 
        qrel_o_reg2 <= qrel_o_reg1;

    always @(posedge rel_in) 
        qrel_o_reg3 <= 1'b0;

    assign rel_rst_o = clkdiv_rel_rst ? (qrel_o_reg3 || qrel_o_reg1) : qrel_o_reg1;

//*** CLKA
    always @(clka1_in or qrel_o_reg2)
        if (qrel_o_reg2 == 1'b1)
            clka1_out <= 1'b0;
        else if (qrel_o_reg2 == 1'b0)
            clka1_out <= clka1_in;

//*** CLKB   
    always @(clkb1_in or qrel_o_reg2)
        if (qrel_o_reg2 == 1'b1)
            clkb1_out <= 1'b0;
        else if (qrel_o_reg2 == 1'b0)
            clkb1_out <= clkb1_in;


//*** Clock divider
    always @(posedge clka1_in or posedge rel_rst_o)
        if (rel_rst_o == 1'b1)
            clka1d2_out <= 1'b0;
        else if (rel_rst_o == 1'b0)
            clka1d2_out <= ~clka1d2_out;

    always @(posedge clka1d2_out or posedge rel_rst_o)
        if (rel_rst_o == 1'b1)
            clka1d4_out <= 1'b0;
        else if (rel_rst_o == 1'b0)
            clka1d4_out <= ~clka1d4_out;

    always @(posedge clka1d4_out or posedge rel_rst_o)
        if (rel_rst_o == 1'b1)
            clka1d8_out <= 1'b0;
        else if (rel_rst_o == 1'b0)
            clka1d8_out <= ~clka1d8_out;

   assign CLKOUT5 = (pmcd_mode) ? 0 : clkout5_out;
   assign CLKOUT4 = (pmcd_mode) ? 0 : clkout4_out;
   assign CLKOUT3 = (pmcd_mode) ? clka1_out : clkout3_out;
   assign CLKOUT2 = (pmcd_mode) ? clka1d2_out : clkout2_out;
   assign CLKOUT1 = (pmcd_mode) ? clka1d4_out : clkout1_out;
   assign CLKOUT0 = (pmcd_mode) ? clka1d8_out : clkout0_out;
   assign CLKFBOUT = (pmcd_mode) ? clkb1_out : clkfb_out;
   assign CLKOUTDCM5 = (pmcd_mode) ? 0 : clkout5_out;
   assign CLKOUTDCM4 = (pmcd_mode) ? 0 : clkout4_out;
   assign CLKOUTDCM3 = (pmcd_mode) ? clka1_out : clkout3_out;
   assign CLKOUTDCM2 = (pmcd_mode) ? clka1d2_out : clkout2_out;
   assign CLKOUTDCM1 = (pmcd_mode) ? clka1d4_out : clkout1_out;
   assign CLKOUTDCM0 = (pmcd_mode) ? clka1d8_out : clkout0_out;
   assign CLKFBDCM = (pmcd_mode) ? clkb1_out : clkfb_out;

// PLL  function

always @(clkinsel_in ) 
 if (pmcd_mode != 1) begin
  if ($time >1 && rst_in != 1'b1) begin
      $display("Input Error : PLL input clock can only be switched when RST=1.  CLKINSEL on instance %m at time %t changed when RST low, should change at RST high.", $time);
      $finish;
  end
  if (clkinsel_in ==1) begin
    if (CLKIN1_PERIOD > (1000.0 /CLKIN_FREQ_MIN) || CLKIN1_PERIOD < (1000.0 / CLKIN_FREQ_MAX)) begin
     $display (" Attribute Syntax Error : The attribute CLKIN1_PERIOD is set to %f ns and out the allowed range %f ns to %f ns.", CLKIN1_PERIOD, 1000.0/CLKIN_FREQ_MAX, 1000.0/CLKIN_FREQ_MIN);
     $finish;
   end
  end 
  else if (clkinsel_in ==0) begin
    if (CLKIN2_PERIOD > (1000.0 /CLKIN_FREQ_MIN) || CLKIN2_PERIOD < (1000.0 / CLKIN_FREQ_MAX)) begin
     $display (" Attribute Syntax Error : The attribute CLKIN2_PERIOD is set to %f ns and out the allowed range %f ns to %f ns.", CLKIN2_PERIOD, 1000.0/CLKIN_FREQ_MAX, 1000.0/CLKIN_FREQ_MIN);
     $finish;
   end
  end

  period_clkin =  (clkinsel_in) ? CLKIN1_PERIOD : CLKIN2_PERIOD;
  clkvco_freq_init_chk =  1000.0 * CLKFBOUT_MULT / (period_clkin  * DIVCLK_DIVIDE);

   if (clkvco_freq_init_chk > VCOCLK_FREQ_MAX || clkvco_freq_init_chk < VCOCLK_FREQ_MIN) begin
     $display (" Attribute Syntax Error : The calculation of VCO frequency=%f Mhz. This exceeds the permitted VCO frequency range of %f Mhz to %f Mhz. The VCO frequency is calculated with formula: VCO frequency =  CLKFBOUT_MULT / (DIVCLK_DIVIDE * CLKIN_PERIOD). Please adjust the attributes to the permitted VCO frequency range.", clkvco_freq_init_chk, VCOCLK_FREQ_MIN, VCOCLK_FREQ_MAX);
     $finish;
   end
end

 assign  init_trig = 1;

   
  assign clkpll_tmp = (clkinsel_in) ? clkin1_in : clkin2_in;
  assign clkpll = (pmcd_mode) ? 0 : clkpll_tmp;

  assign orig_rst_in =  rst_input; 

always @(posedge clkpll or posedge orig_rst_in)
  if (orig_rst_in)
     rst_in1 <= 1;
  else
     rst_in1 <= orig_rst_in;

  assign rst_in = (rst_in1 || rst_unlock);
  
  always @(posedge pll_unlock)
  if (rst_on_loss ) begin
     rst_unlock <= 1'b1;
     rst_unlock <= #10000 1'b0;
  end

always @(rst_input )
  if (rst_input==1)
     rst_edge = $time;
  else if (rst_input==0 && rst_edge > 1) begin
     rst_ht = $time - rst_edge;
     if (rst_ht < 10000) 
        $display("Input Error : RST on instance %m at time %t must be asserted at least for 10 ns.", $time);
  end 

//
// DRP port read and write
//

  assign do_out = dr_sram[daddr_lat];

always @(posedge dclk_in or posedge gsr_in)
  if (gsr_in == 1) begin
       drp_lock <= 0;
    end
  else begin
    if (den_in == 1) begin
        valid_daddr = addr_is_valid(daddr_in);
        if (drp_lock == 1) begin
          $display(" Warning : DEN is high at PLL_ADV instance %m at time %t. Need wait for DRDY signal before next read/write operation through DRP. ", $time);
          $finish;
        end
        else begin
          drp_lock <= 1;
          daddr_lat <= daddr_in;
        end
 
        if (valid_daddr && ( daddr_in == 5'b00110 || daddr_in == 5'b00001 || daddr_in == 5'b00000 ||
                         (daddr_in >= 5'b01100 && daddr_in <= 5'b11100 && daddr_in != 5'b10000 && 
                                  daddr_in != 5'b10001 && daddr_in != 5'b11000 ))) begin
              end
        else begin
                  $display(" Warning : Address DADDR=%b is unsupported at PLL_ADV instance %m at time %t.  ",  DADDR, $time);
        end

        if (dwe_in == 1) begin          // write process
          if (rst_input == 1) begin
             if (valid_daddr && ( daddr_in == 5'b00110 || daddr_in == 5'b00001 || daddr_in == 5'b00000 ||
                                  (daddr_in >= 5'b01100 && daddr_in <= 5'b11100 && daddr_in != 5'b10000 && 
                                  daddr_in != 5'b10001 && daddr_in != 5'b11000 ))) begin
                  dr_sram[daddr_in] <= di_in;
              end

             if (daddr_in == 5'b11100) 
                 clkout_delay_para_drp (clkout0_dly, clk0_nocnt, clk0_edge, di_in, daddr_in);

             if (daddr_in == 5'b11011)
                 clkout_hl_para_drp (clk0_lt, clk0_ht, clk0pm_sel, di_in, daddr_in);

             if (daddr_in == 5'b11010) 
                 clkout_delay_para_drp (clkout1_dly, clk1_nocnt, clk1_edge, di_in, daddr_in);

             if (daddr_in == 5'b11001)
                 clkout_hl_para_drp (clk1_lt, clk1_ht, clk1pm_sel, di_in, daddr_in);

             if (daddr_in == 5'b10111) 
                 clkout_delay_para_drp (clkout2_dly, clk2_nocnt, clk2_edge, di_in, daddr_in);

             if (daddr_in == 5'b10110)
                 clkout_hl_para_drp (clk2_lt, clk2_ht, clk2pm_sel, di_in, daddr_in);

             if (daddr_in == 5'b10101) 
                 clkout_delay_para_drp (clkout3_dly, clk3_nocnt, clk3_edge, di_in, daddr_in);

             if (daddr_in == 5'b10100)
                 clkout_hl_para_drp (clk3_lt, clk3_ht, clk3pm_sel, di_in, daddr_in);

             if (daddr_in == 5'b10011) 
                 clkout_delay_para_drp (clkout4_dly, clk4_nocnt, clk4_edge, di_in, daddr_in);

             if (daddr_in == 5'b10010)
                 clkout_hl_para_drp (clk4_lt, clk4_ht, clk4pm_sel, di_in, daddr_in);

             if (daddr_in == 5'b01111) 
                 clkout_delay_para_drp (clkout5_dly, clk5_nocnt, clk5_edge, di_in, daddr_in);

             if (daddr_in == 5'b01110)
                 clkout_hl_para_drp (clk5_lt, clk5_ht, clk5pm_sel, di_in, daddr_in);

             if (daddr_in == 5'b01101) 
                 clkout_delay_para_drp (clkfbm1_dly, clkfbm1_nocnt, clkfbm1_edge, di_in, daddr_in);

             if (daddr_in == 5'b01100)
                 clkout_hl_para_drp (clkfbm1_lt, clkfbm1_ht, clkfbm1pm_sel, di_in, daddr_in);

             if (daddr_in == 5'b00110) begin
                 clkind_lt <= di_in[5:0];
                 clkind_ht <= di_in[11:6];
                 if ( di_in[5:0] == 6'b0 && di_in[11:6] == 6'b0 )
                     clkind_div <= 8'b10000000;
                 else if (di_in[5:0] == 6'b0 && di_in[11:6] != 6'b0 )
                     clkind_div <= 64 + di_in[11:6];
                 else if (di_in[5:0] == 6'b0 && di_in[11:6] != 6'b0 )
                     clkind_div <= 64 + di_in[5:0];
                 else
                      clkind_div <= di_in[5:0] + di_in[11:6];
                 clkind_nocnt <= di_in[12];
                 clkind_edge <= di_in[13];
             end

          end
          else begin
                  $display(" Error : RST is low at PLL_ADV instance %m at time %t. RST need to be high when change X_PLL_ADV paramters through DRP. ", $time);
          end

        end //DWE
                  
    end  //DEN
    if ( drp_lock == 1) begin
          drp_lock <= 0;
          drp_lock1 <= 1;
    end
    if (drp_lock1 == 1) begin
         drp_lock1 <= 0;
         drdy_out <= 1;
    end 
    if (drdy_out == 1)
        drdy_out <= 0;
end

function addr_is_valid;
input [6:0] daddr_funcin;
begin
  addr_is_valid = 1;
  for (i=0; i<=6; i=i+1)
    if ( daddr_funcin[i] != 0 && daddr_funcin[i] != 1)
       addr_is_valid = 0;
end
endfunction


// end process drp;


//
// determine clock period
//

  always @(posedge clkpll or posedge rst_in)
   if (rst_in)
   begin
     clkin_period[0] <= period_vco_target;
     clkin_period[1] <= period_vco_target;
     clkin_period[2] <= period_vco_target;
     clkin_period[3] <= period_vco_target;
     clkin_period[4] <= period_vco_target;
     clkin_jit <= 0;
     clkin_lock_cnt <= 0;
     pll_locked_tm <= 0;
     lock_period <= 0;
     pll_locked_tmp1 <= 0;
     clkout_en0_tmp <= 0;
     unlock_recover <= 0;
     clkin_edge <= 0;
   end
   else  begin
       clkin_edge <= $time;
       clkin_period[4] <= clkin_period[3];
       clkin_period[3] <= clkin_period[2];
       clkin_period[2] <= clkin_period[1];
       clkin_period[1] <= clkin_period[0];
       if (clkin_edge != 0 && clkin_stopped_p == 0 && clkin_stopped_n == 0) 
          clkin_period[0] <= $time - clkin_edge;
  
       if (pll_unlock == 0)
          clkin_jit <=  $time - clkin_edge - clkin_period[0];
       else
          clkin_jit <= 0;

      if ( (clkin_lock_cnt < lock_cnt_max) && fb_delay_found && pll_unlock == 0)
            clkin_lock_cnt <= clkin_lock_cnt + 1;
      else if (pll_unlock == 1 && rst_on_loss ==0 && pll_locked_tmp1 ==1 ) begin
            clkin_lock_cnt <= locked_en_time;
            unlock_recover <= 1;
      end

      if ( clkin_lock_cnt >= `PLL_LOCK_TIME && pll_unlock == 0)
        pll_locked_tm <= 1;

      if ( clkin_lock_cnt == 6 )
        lock_period <= 1;

      if (clkin_lock_cnt >= clkout_en_time) begin
         clkout_en0_tmp <= 1;
      end

      if (clkin_lock_cnt >= locked_en_time)
        pll_locked_tmp1 <= 1;

      if (unlock_recover ==1 && clkin_lock_cnt  >= lock_cnt_max)
        unlock_recover <= 0;
  end

   always @(clkout_en0_tmp)
   if (clkout_en0_tmp==0)
        clkout_en0 = 0;
   else
      @(negedge clkpll)
        clkout_en0 <= #(clkin_period[0]/2) clkout_en0_tmp;

  always @(clkout_en0)
       clkout_en <= #(clkvco_delay) clkout_en0;

  always @(pll_locked_tmp1 )
  if (pll_locked_tmp1==0)
         pll_locked_tmp2 =  pll_locked_tmp1;
  else begin
         pll_locked_tmp2 <= #pll_locked_delay  pll_locked_tmp1;
  end


  always @(rst_in)
  if (rst_in) begin
     assign pll_locked_tmp2 = 0;
     assign clkout_en0 = 0;
     assign clkout_en = 0;
  end
  else begin
    deassign pll_locked_tmp2;
    deassign clkout_en0;
    deassign clkout_en;
  end
    
  assign locked_out = (pll_locked_tm && pll_locked_tmp2 && ~pll_unlock && !unlock_recover) ? 1 : 0;


  always @(clkin_period[0] or clkin_period[1] or clkin_period[2] or 
                 clkin_period[3] or clkin_period[4] or period_avg)
    if ( clkin_period[0] != period_avg) 
          period_avg = (clkin_period[0] + clkin_period[1] + clkin_period[2] 
                       + clkin_period[3] + clkin_period[4])/5;

  always @(period_avg or clkind_div or clkfbm1_div) begin
   period_fb = period_avg * clkind_div;
   period_vco = period_fb / clkfbm1_div;
   period_vco_half = period_vco /2;
   pll_locked_delay = period_fb * clkfbm1_div;
   clkin_dly_t =  period_avg * (clkind_div + 1.25);
   clkfb_dly_t = period_fb * 2.25 ;
   period_vco1 = period_vco / 8;
   period_vco2 = period_vco / 4;
   period_vco3 = period_vco * 3/ 8;
   period_vco4 = period_vco / 2;
   period_vco5 = period_vco * 5 / 8;
   period_vco6 = period_vco *3 / 4;
   period_vco7 = period_vco * 7 / 8;
   md_product = clkind_div * clkfbm1_div;
   md_product_dbl = clkind_div * clkfbm1_div * 2;
  end

  assign clkvco_lk_rst = ( rst_in == 1  ||  pll_unlock == 1 || pll_locked_tm == 0) ? 1 : 0;
  
  always @(clkvco_lk_rst)
   if (clkvco_lk_rst)
      assign  clkvco_lk = 0;
   else
      deassign clkvco_lk;


//  always @(posedge clkpll or posedge rst_in or  posedge pll_unlock) 
//   if ( rst_in == 1  ||  pll_unlock == 1 || pll_locked_tm == 0) begin
//        clkvco_lk <= 0;
//   end
//   else begin
  always @(posedge clkpll) 
     if (pll_locked_tm ==1) begin
       clkvco_lk <= 1;
        for (i1=1; i1 < md_product_dbl; i1=i1+1)
               #(period_vco_half) clkvco_lk <= ~clkvco_lk;
     end


  always @(fb_delay or period_vco or clkfbm1_dly or clkfbm1pm_rl) begin
     val_tmp = period_vco * md_product;
     fbm1_comp_delay = period_vco *(clkfbm1_dly  + clkfbm1pm_rl );
     dly_tmp = fb_delay + fbm1_comp_delay;
    if (fb_delay == 0)
        clkvco_delay = 0;
    else if ( dly_tmp < val_tmp)
       clkvco_delay = val_tmp - dly_tmp;
    else
     clkvco_delay = val_tmp - dly_tmp % val_tmp ;
  end

  always @(clkfbm1pm_sel)
   case (clkfbm1pm_sel)
     3'b000 : clkfbm1pm_rl = 0.0;
     3'b001 : clkfbm1pm_rl = 0.125;
     3'b010 : clkfbm1pm_rl = 0.25;
     3'b011 : clkfbm1pm_rl = 0.375;
     3'b100 : clkfbm1pm_rl = 0.50;
     3'b101 : clkfbm1pm_rl = 0.625;
     3'b110 : clkfbm1pm_rl = 0.75;
     3'b111 : clkfbm1pm_rl = 0.875;
   endcase

  always @(clkvco_free )
    if (pmcd_mode != 1 && pll_locked_tm == 0)
      clkvco_free <= #period_vco_target_half ~clkvco_free;
  
  always @(clkvco_lk or clkvco_free or pll_locked_tm)
   if ( pll_locked_tm)
       clkvco <=  #clkvco_delay clkvco_lk;
   else
       clkvco <=  #clkvco_delay clkvco_free;

  always @(clk0_ht or clk0_lt or clk0_nocnt or init_trig)
   clkout_pm_cal(clk0_ht1, clk0_div, clk0_div1, clk0_ht, clk0_lt, clk0_nocnt, clk0_edge);

  always @(clk1_ht or clk1_lt or clk1_nocnt or init_trig)
   clkout_pm_cal(clk1_ht1, clk1_div, clk1_div1, clk1_ht, clk1_lt, clk1_nocnt, clk1_edge);

  always @(clk2_ht or clk2_lt or clk2_nocnt or init_trig)
   clkout_pm_cal(clk2_ht1, clk2_div, clk2_div1, clk2_ht, clk2_lt, clk2_nocnt, clk2_edge);

  always @(clk3_ht or clk3_lt or clk3_nocnt or init_trig)
   clkout_pm_cal(clk3_ht1, clk3_div, clk3_div1, clk3_ht, clk3_lt, clk3_nocnt, clk3_edge);

  always @(clk4_ht or clk4_lt or clk4_nocnt or init_trig)
   clkout_pm_cal(clk4_ht1, clk4_div, clk4_div1, clk4_ht, clk4_lt, clk4_nocnt, clk4_edge);

  always @(clk5_ht or clk5_lt or clk5_nocnt or init_trig)
   clkout_pm_cal(clk5_ht1, clk5_div, clk5_div1, clk5_ht, clk5_lt, clk5_nocnt, clk5_edge);

  always @(clkfbm1_ht or clkfbm1_lt or clkfbm1_nocnt or init_trig)
   clkout_pm_cal(clkfbm1_ht1, clkfbm1_div, clkfbm1_div1, clkfbm1_ht, clkfbm1_lt, clkfbm1_nocnt, clkfbm1_edge);

  always @(rst_in)
  if (rst_in)
    assign clkout_mux = 8'b0;
  else 
    deassign clkout_mux;

 always @(clkvco or clkout_en ) 
  if (clkout_en) begin
   clkout_mux[0] <= clkvco;
   clkout_mux[1] <= #(period_vco1) clkvco;
   clkout_mux[2] <= #(period_vco2) clkvco;
   clkout_mux[3] <= #(period_vco3) clkvco;
   clkout_mux[4] <= #(period_vco4) clkvco;
   clkout_mux[5] <= #(period_vco5) clkvco;
   clkout_mux[6] <= #(period_vco6) clkvco;
   clkout_mux[7] <= #(period_vco7) clkvco;
 end
   
 assign clk0in = clkout_mux[clk0pm_sel];
 assign clk1in = clkout_mux[clk1pm_sel];
 assign clk2in = clkout_mux[clk2pm_sel];
 assign clk3in = clkout_mux[clk3pm_sel];
 assign clk4in = clkout_mux[clk4pm_sel];
 assign clk5in = clkout_mux[clk5pm_sel];
 assign clkfbm1in = clkout_mux[clkfbm1pm_sel];

 assign clk0ps_en = (clk0_dly_cnt == clkout0_dly) ? clkout_en : 0;
 assign clk1ps_en = (clk1_dly_cnt == clkout1_dly) ? clkout_en : 0;
 assign clk2ps_en = (clk2_dly_cnt == clkout2_dly) ? clkout_en : 0;
 assign clk3ps_en = (clk3_dly_cnt == clkout3_dly) ? clkout_en : 0;
 assign clk4ps_en = (clk4_dly_cnt == clkout4_dly) ? clkout_en : 0;
 assign clk5ps_en = (clk5_dly_cnt == clkout5_dly) ? clkout_en : 0;
 assign clkfbm1ps_en = (clkfbm1_dly_cnt == clkfbm1_dly) ? clkout_en : 0;

 always  @(negedge clk0in or posedge rst_in) 
     if (rst_in)
        clk0_dly_cnt <= 6'b0;
     else
       if (clk0_dly_cnt < clkout0_dly && clkout_en ==1)
          clk0_dly_cnt <= clk0_dly_cnt + 1;

 always  @(negedge clk1in or posedge rst_in)
     if (rst_in)
        clk1_dly_cnt <= 6'b0;
     else
       if (clk1_dly_cnt < clkout1_dly && clkout_en ==1)
          clk1_dly_cnt <= clk1_dly_cnt + 1;

 always  @(negedge clk2in or posedge rst_in)
     if (rst_in)
        clk2_dly_cnt <= 6'b0;
     else
       if (clk2_dly_cnt < clkout2_dly && clkout_en ==1)
          clk2_dly_cnt <= clk2_dly_cnt + 1;

 always  @(negedge clk3in or posedge rst_in)
     if (rst_in)
        clk3_dly_cnt <= 6'b0;
     else
       if (clk3_dly_cnt < clkout3_dly && clkout_en ==1)
          clk3_dly_cnt <= clk3_dly_cnt + 1;

 always  @(negedge clk4in or posedge rst_in)
     if (rst_in)
        clk4_dly_cnt <= 6'b0;
     else
       if (clk4_dly_cnt < clkout4_dly && clkout_en ==1)
          clk4_dly_cnt <= clk4_dly_cnt + 1;

 always  @(negedge clk5in or posedge rst_in)
     if (rst_in)
        clk5_dly_cnt <= 6'b0;
     else
       if (clk5_dly_cnt < clkout5_dly && clkout_en ==1)
          clk5_dly_cnt <= clk5_dly_cnt + 1;

 always  @(negedge clkfbm1in or posedge rst_in)
     if (rst_in)
        clkfbm1_dly_cnt <= 6'b0;
     else
       if (clkfbm1_dly_cnt < clkfbm1_dly && clkout_en ==1)
          clkfbm1_dly_cnt <= clkfbm1_dly_cnt + 1;

  always @(posedge clk0in or negedge clk0in or posedge rst_in)
     if (rst_in) begin
        clk0_cnt <= 8'b0;
        clk0_out <= 0;
     end
     else if (clk0ps_en) begin
          if (clk0_cnt < clk0_div1)
                clk0_cnt <= clk0_cnt + 1;
             else
                clk0_cnt <= 8'b0; 

           if (clk0_cnt < clk0_ht1)
               clk0_out <= 1;
           else
               clk0_out <= 0;
     end
     else begin
        clk0_cnt <= 8'b0;
        clk0_out <= 0;
     end

  always @(posedge clk1in or negedge clk1in or posedge rst_in)
     if (rst_in) begin
        clk1_cnt <= 8'b0;
        clk1_out <= 0;
     end
     else if (clk1ps_en) begin
          if (clk1_cnt < clk1_div1)
                clk1_cnt <= clk1_cnt + 1;
             else
                clk1_cnt <= 8'b0;

           if (clk1_cnt < clk1_ht1)
               clk1_out <= 1;
           else
               clk1_out <= 0;
     end
     else begin
        clk1_cnt <= 8'b0;
        clk1_out <= 0;
     end

  always @(posedge clk2in or negedge clk2in or posedge rst_in)
     if (rst_in) begin
        clk2_cnt <= 8'b0;
        clk2_out <= 0;
     end
     else if (clk2ps_en) begin
          if (clk2_cnt < clk2_div1)
                clk2_cnt <= clk2_cnt + 1;
             else
                clk2_cnt <= 8'b0;

           if (clk2_cnt < clk2_ht1)
               clk2_out <= 1;
           else
               clk2_out <= 0;
     end
     else begin
        clk2_cnt <= 8'b0;
        clk2_out <= 0;
     end

  always @(posedge clk3in or negedge clk3in or posedge rst_in)
     if (rst_in) begin
        clk3_cnt <= 8'b0;
        clk3_out <= 0;
     end
     else if (clk3ps_en) begin
          if (clk3_cnt < clk3_div1)
                clk3_cnt <= clk3_cnt + 1;
             else
                clk3_cnt <= 8'b0;

           if (clk3_cnt < clk3_ht1)
               clk3_out <= 1;
           else
               clk3_out <= 0;
     end
     else begin
        clk3_cnt <= 8'b0;
        clk3_out <= 0;
     end


  always @(posedge clk4in or negedge clk4in or posedge rst_in)
     if (rst_in) begin
        clk4_cnt <= 8'b0;
        clk4_out <= 0;
     end
     else if (clk4ps_en) begin
          if (clk4_cnt < clk4_div1)
                clk4_cnt <= clk4_cnt + 1;
             else
                clk4_cnt <= 8'b0;

           if (clk4_cnt < clk4_ht1)
               clk4_out <= 1;
           else
               clk4_out <= 0;
     end
     else begin
        clk4_cnt <= 8'b0;
        clk4_out <= 0;
     end


  always @(posedge clk5in or negedge clk5in or posedge rst_in)
     if (rst_in) begin
        clk5_cnt <= 8'b0;
        clk5_out <= 0;
     end
     else if (clk5ps_en) begin
          if (clk5_cnt < clk5_div1)
                clk5_cnt <= clk5_cnt + 1;
             else
                clk5_cnt <= 8'b0;

           if (clk5_cnt < clk5_ht1)
               clk5_out <= 1;
           else
               clk5_out <= 0;
     end
     else begin
        clk5_cnt <= 8'b0;
        clk5_out <= 0;
     end


  always @(posedge clkfbm1in or negedge clkfbm1in or posedge rst_in)
     if (rst_in) begin
        clkfbm1_cnt <= 8'b0;
        clkfbm1_out <= 0;
     end
     else if (clkfbm1ps_en) begin
          if (clkfbm1_cnt < clkfbm1_div1)
                clkfbm1_cnt <= clkfbm1_cnt + 1;
             else
                clkfbm1_cnt <= 8'b0;

           if (clkfbm1_cnt < clkfbm1_ht1)
               clkfbm1_out <= 1;
           else
               clkfbm1_out <= 0;
     end
     else begin
        clkfbm1_cnt <= 8'b0;
        clkfbm1_out <= 0;
     end



   always @(clk0_out or clkfb_tst or fb_delay_found)
    if (fb_delay_found == 1)
          clkout0_out =  clk0_out;
    else
          clkout0_out = clkfb_tst;

   always @(clk1_out or clkfb_tst or fb_delay_found)
    if (fb_delay_found == 1)
          clkout1_out =  clk1_out;
    else
          clkout1_out = clkfb_tst;

   always @(clk2_out or clkfb_tst or fb_delay_found)
    if (fb_delay_found == 1)
          clkout2_out =  clk2_out;
    else
          clkout2_out = clkfb_tst;

   always @(clk3_out or clkfb_tst or fb_delay_found)
    if (fb_delay_found == 1)
          clkout3_out =  clk3_out;
    else
          clkout3_out = clkfb_tst;

   always @(clk4_out or clkfb_tst or fb_delay_found)
    if (fb_delay_found == 1)
          clkout4_out =  clk4_out;
    else
          clkout4_out = clkfb_tst;

   always @(clk5_out or clkfb_tst or fb_delay_found)
    if (fb_delay_found == 1)
          clkout5_out =  clk5_out;
    else
          clkout5_out = clkfb_tst;

   always @(clkfbm1_out or clkfb_tst or fb_delay_found)
    if (fb_delay_found == 1)
          clkfb_out =  clkfbm1_out;
    else
          clkfb_out = clkfb_tst;

//
// determine feedback delay
//

always @(rst_in1)
  if (rst_in1)
      assign clkfb_tst = 0;
  else
      deassign clkfb_tst;

always @(posedge clkpll )
    if (fb_delay_found_tmp == 0 && GSR == 0 && rst_in1 == 0) begin
         clkfb_tst <=  1'b1;
    end
    else
         clkfb_tst <=  1'b0;

  
always @( posedge clkfb_tst or posedge rst_in1 )
  if (rst_in1)
    delay_edge  <= 0;
  else 
    delay_edge <= $time;

always @(posedge clkfb_in or posedge rst_in1 ) 
  if (rst_in1) begin
    fb_delay  <= 0;
    fb_delay_found_tmp <= 0;
  end
 else 
   if (fb_delay_found_tmp ==0 ) begin
     if ( delay_edge != 0)
      fb_delay <= ($time - delay_edge);
     else
      fb_delay <= 0;
     fb_delay_found_tmp <=  1;
   end

always @(rst_in1)
  if (rst_in1)
     assign fb_delay_found = 0;
  else
     deassign fb_delay_found;

always @(fb_delay_found_tmp or clkvco_delay )
        fb_delay_found <= #(clkvco_delay) fb_delay_found_tmp;


always @(fb_delay)
  if (rst_in1==0 && (fb_delay/1000.0 > fb_delay_max)) begin
            $display("Warning : The feedback delay on PLL_ADV instance %m at time %t is %f ns. It is over the maximun value %f ns.", $time, fb_delay / 1000.0, fb_delay_max);
        end

//
// generate unlock signal
//

always @(clkpll) 
  clkpll_dly <= #clkin_dly_t clkpll;

always @(clkfb_in)
  if (pmcd_mode != 1)
     clkfbin_dly <= #clkfb_dly_t clkfb_in;
  else 
      clkfbin_dly = 0;

always @( posedge clkpll_dly or negedge clkpll or posedge rst_in)
  if (rst_in || clkpll == 0) begin
      clkstop_cnt_p = 0;
      clkin_stopped_p = 0;
  end
  else   
    if (fb_delay_found && pll_locked_tmp2) begin 
      if (clkpll && clkpll_jitter_unlock == 0)
            clkstop_cnt_p <= clkstop_cnt_p +1;
      else
            clkstop_cnt_p = 0;

      if (clkstop_cnt_p > clkin_stop_max)
            clkin_stopped_p <= 1;
      else
            clkin_stopped_p = 0;
     end
    else begin
      clkstop_cnt_p = 0;
      clkin_stopped_p = 0;
   end

always @( posedge clkpll_dly or posedge clkpll or posedge rst_in)
  if (rst_in || clkpll == 1) begin
      clkstop_cnt_n = 0;
      clkin_stopped_n = 0;
  end
  else
    if (fb_delay_found && pll_locked_tmp2) begin
      if (clkpll==0 && clkpll_jitter_unlock == 0)
            clkstop_cnt_n <= clkstop_cnt_n +1;
      else
            clkstop_cnt_n = 0;

      if (clkstop_cnt_n > clkin_stop_max)
            clkin_stopped_n <= 1;
      else
            clkin_stopped_n = 0;
     end
    else begin
      clkstop_cnt_n = 0;
      clkin_stopped_n = 0;
   end


always @( posedge clkfbin_dly or negedge clkfb_in or posedge rst_in)
  if (rst_in || clkfb_in == 0) begin
      clkfbstop_cnt_p = 0;
      clkfb_stopped_p = 0;
  end
  else   
    if (fb_delay_found && pll_locked_tmp2) begin 
      if (clkfb_in && clkpll_jitter_unlock == 0)
            clkfbstop_cnt_p <= clkfbstop_cnt_p +1;
      else
            clkfbstop_cnt_p = 0;

      if (clkfbstop_cnt_p > clkfb_stop_max)
            clkfb_stopped_p <= 1;
      else
            clkfb_stopped_p = 0;
     end
    else begin
      clkfbstop_cnt_p = 0;
      clkfb_stopped_p = 0;
   end

always @( posedge clkfbin_dly or posedge clkfb_in or posedge rst_in)
  if (rst_in==1 || clkfb_in == 1) begin
      clkfbstop_cnt_n = 0;
      clkfb_stopped_n = 0;
  end
  else   
    if (fb_delay_found && pll_locked_tmp2) begin
      if (clkfb_in==0 && clkpll_jitter_unlock == 0)
            clkfbstop_cnt_n <= clkfbstop_cnt_n +1;
      else
            clkfbstop_cnt_n = 0;

      if (clkfbstop_cnt_n > clkfb_stop_max)
            clkfb_stopped_n <= 1;
      else
            clkfb_stopped_n = 0;
     end
    else begin
      clkfbstop_cnt_n = 0;
      clkfb_stopped_n = 0;
   end

always @(clkin_jit or rst_in )
  if (rst_in)
      clkpll_jitter_unlock = 0;
  else
   if (  pll_locked_tmp2 && clkfb_stopped == 0 && clkin_stopped == 0) begin
      if ((clkin_jit > REF_CLK_JITTER_MAX_tmp) || (clkin_jit < -REF_CLK_JITTER_MAX_tmp))
        clkpll_jitter_unlock = 1;
      else
         clkpll_jitter_unlock = 0;
   end
   else
         clkpll_jitter_unlock = 0;
      
  assign  clkin_stopped = (clkin_stopped_p || clkin_stopped_n) ? 1 : 0;
  assign  clkfb_stopped = (clkfb_stopped_p ||clkfb_stopped_n) ? 1 : 0;
  assign pll_unlock = (clkin_stopped || clkfb_stopped || clkpll_jitter_unlock) ? 1 : 0; 

// tasks


task clkout_dly_cal;
output [5:0] clkout_dly;
output [2:0] clkpm_sel;
input  clkdiv;
input  clk_ps;
input reg [160:0] clk_ps_name;

integer clkdiv;
real clk_ps;
real clk_ps_rl;

real clk_dly_rl, clk_dly_rem;
integer clkout_dly_tmp;

begin

 if (clk_ps < 0.0)
    clk_dly_rl = (360.0 + clk_ps) * clkdiv / 360.0; 
 else
   clk_dly_rl = clk_ps * clkdiv / 360.0;

   clkout_dly_tmp =  $rtoi(clk_dly_rl);

  if (clkout_dly_tmp > 63) begin
    $display(" Warning : Attribute %s of PLL_ADV on instance %m is set to %f. Required phase shifting can not be reached since it is over the maximum phase shifting ability of X_PLL_ADV", clk_ps_name, clk_ps);
    clkout_dly = 6'b111111;
    end
   else
     clkout_dly = clkout_dly_tmp;

    clk_dly_rem = clk_dly_rl - clkout_dly;

    if (clk_dly_rem < 0.125)
        clkpm_sel =  0;
    else if (clk_dly_rem >=  0.125 && clk_dly_rem < 0.25)
        clkpm_sel =  1;
    else if (clk_dly_rem >=  0.25 && clk_dly_rem < 0.375)
        clkpm_sel =  2;
    else if (clk_dly_rem >=  0.375 && clk_dly_rem < 0.5)
        clkpm_sel =  3;
    else if (clk_dly_rem >=  0.5 && clk_dly_rem < 0.625)
        clkpm_sel =  4;
    else if (clk_dly_rem >=  0.625 && clk_dly_rem < 0.75)
        clkpm_sel =  5;
    else if (clk_dly_rem >=  0.75 && clk_dly_rem < 0.875)
        clkpm_sel =  6;
    else if (clk_dly_rem >=  0.875 )
        clkpm_sel =  7;

    if (clk_ps < 0.0)
       clk_ps_rl = (clkout_dly + 0.125 * clkpm_sel)* 360.0 / clkdiv - 360.0;
    else
       clk_ps_rl = (clkout_dly + 0.125 * clkpm_sel) * 360.0 / clkdiv;

    if (((clk_ps_rl- clk_ps) > 0.001) || ((clk_ps_rl- clk_ps) < -0.001))
    $display(" Warning : Attribute %s of PLL_ADV on instance %m is set to %f. Real phase shifting is %f. Required phase shifting can not be reached.", clk_ps_name, clk_ps, clk_ps_rl);

end
endtask


task   clk_out_para_cal;
output [6:0] clk_ht;
output [6:0] clk_lt; 
output clk_nocnt;
output clk_edge; 
input  CLKOUT_DIVIDE;
input  CLKOUT_DUTY_CYCLE;

integer CLKOUT_DIVIDE;
real  CLKOUT_DUTY_CYCLE;

real tmp_value;
integer tmp_value1;
real tmp_value2;

begin
   tmp_value = CLKOUT_DIVIDE * CLKOUT_DUTY_CYCLE;
   tmp_value1 = $rtoi(tmp_value * 2) % 2;
   tmp_value2 = CLKOUT_DIVIDE - tmp_value;
   

   if ((tmp_value) >= O_MAX_HT_LT) begin
//       clk_ht = O_MAX_HT_LT;
       clk_ht = 7'b1000000;
   end
   else begin
       if  (tmp_value < 1.0)
          clk_ht = 1;
       else  
          if ( tmp_value1  != 0)
             clk_ht = $rtoi(tmp_value) + 1; 
          else
             clk_ht = $rtoi(tmp_value);
   end

   if ( (CLKOUT_DIVIDE -  clk_ht) >= O_MAX_HT_LT)
       clk_lt = 7'b1000000;
   else 
      clk_lt =  CLKOUT_DIVIDE -  clk_ht;

   clk_nocnt = (CLKOUT_DIVIDE ==1) ? 1 : 0;
   if ( tmp_value < 1.0)
     clk_edge = 1;
   else if (tmp_value1 != 0)
     clk_edge = 1;
   else
     clk_edge = 0;
end
endtask


function  clkout_duty_chk;
   input  CLKOUT_DIVIDE;
   input  CLKOUT_DUTY_CYCLE;
   input reg [160:0] CLKOUT_DUTY_CYCLE_N; 

   integer CLKOUT_DIVIDE, step_tmp;
   real CLKOUT_DUTY_CYCLE;

   real CLK_DUTY_CYCLE_MIN, CLK_DUTY_CYCLE_MAX, CLK_DUTY_CYCLE_STEP;
   real CLK_DUTY_CYCLE_MIN_rnd;
   reg clk_duty_tmp_int;
   
begin

   if (CLKOUT_DIVIDE > O_MAX_HT_LT) begin
      CLK_DUTY_CYCLE_MIN = (CLKOUT_DIVIDE - O_MAX_HT_LT)/CLKOUT_DIVIDE;
      CLK_DUTY_CYCLE_MAX = (O_MAX_HT_LT + 0.5)/CLKOUT_DIVIDE;
      CLK_DUTY_CYCLE_MIN_rnd = CLK_DUTY_CYCLE_MIN;
   end
   else begin
    if (CLKOUT_DIVIDE == 1) begin
        CLK_DUTY_CYCLE_MIN = 0.0;
        CLK_DUTY_CYCLE_MIN_rnd = 0.0;
     end
     else begin
      step_tmp = 1000 / CLKOUT_DIVIDE;
      CLK_DUTY_CYCLE_MIN_rnd = step_tmp / 1000.0;
      CLK_DUTY_CYCLE_MIN = 1.0 /CLKOUT_DIVIDE;
     end
    CLK_DUTY_CYCLE_MAX = 1.0;
   end

   if (CLKOUT_DUTY_CYCLE > CLK_DUTY_CYCLE_MAX || CLKOUT_DUTY_CYCLE < CLK_DUTY_CYCLE_MIN_rnd) begin
     $display(" Attribute Syntax Warning : %s is set to %f on instance %m and is not in the allowed range %f to %f.", CLKOUT_DUTY_CYCLE_N, CLKOUT_DUTY_CYCLE, CLK_DUTY_CYCLE_MIN, CLK_DUTY_CYCLE_MAX );
   end

    clk_duty_tmp_int = 0;
    CLK_DUTY_CYCLE_STEP = 0.5 / CLKOUT_DIVIDE;
    for (j = 0; j < (2 * CLKOUT_DIVIDE - CLK_DUTY_CYCLE_MIN/CLK_DUTY_CYCLE_STEP); j = j + 1)
        if (((CLK_DUTY_CYCLE_MIN + CLK_DUTY_CYCLE_STEP * j) - CLKOUT_DUTY_CYCLE) > -0.001 && 
             ((CLK_DUTY_CYCLE_MIN + CLK_DUTY_CYCLE_STEP * j) - CLKOUT_DUTY_CYCLE) < 0.001)
            clk_duty_tmp_int = 1;

   if ( clk_duty_tmp_int != 1) begin
    $display(" Attribute Syntax Warning : %s is set to %f on instance %m and is  not an allowed value. Allowed values are:",  CLKOUT_DUTY_CYCLE_N, CLKOUT_DUTY_CYCLE);
    for (j = 0; j < (2 * CLKOUT_DIVIDE - CLK_DUTY_CYCLE_MIN/CLK_DUTY_CYCLE_STEP); j = j + 1)
       $display("%f", CLK_DUTY_CYCLE_MIN + CLK_DUTY_CYCLE_STEP * j);
  end

  clkout_duty_chk = 1'b1;
end
endfunction


function  para_int_pmcd_chk;
   input  para_in;
   input reg [160:0] para_name;
   input  range_low;
   input  range_high;
   input  pmcd_mode;
   input  pmcd_value;

    integer para_in;
    integer range_low;
    integer range_high;
    integer pmcd_value;
begin

        if (para_in < range_low || para_in > range_high)
        begin
            $display("Attribute Syntax Error : The Attribute %s on PLL_ADV instance %m is set to %d.  Legal values for this attribute are %d to %d.", para_name, para_in, range_low, range_high);
            $finish;
        end
        else if (pmcd_mode == 1 && para_in != pmcd_value) begin
            $display("Attribute Syntax Error : The Attribute %s on PLL_ADV instance %m is set to %d when attribute PLL_PMCD_MODE is set to TRUE.  Legal values for this attribute is %d when PLL in PMCD MODE.", para_name, para_in, pmcd_value);
            $finish;
        end

   para_int_pmcd_chk = 1'b1;
end
endfunction

function  para_real_pmcd_chk;
   input  para_in;
   input reg [160:0] para_name;
   input  range_low;
   input  range_high;
   input  pmcd_mode;
   input  pmcd_value;

    real para_in;
    real range_low;
    real range_high;
    real pmcd_value;
begin

        if (para_in < range_low || para_in > range_high)
        begin
            $display("Attribute Syntax Error : The Attribute %s on PLL_ADV instance %m is set to %f.  Legal values for this attribute are %f to %f.", para_name, para_in, range_low, range_high);
            $finish;
        end
        else if (pmcd_mode == 1 && para_in != pmcd_value) begin
            $display("Attribute Syntax Error : The Attribute %s on PLL_ADV instance %m is set to %f when attribute PLL_PMCD_MODE is set to TRUE.  Legal values for this attribute is %f when PLL in PMCD MODE.", para_name, para_in, pmcd_value);
            $finish;
        end
 
    para_real_pmcd_chk = 1'b0;
end
endfunction

function  para_int_range_chk;
   input  para_in; 
   input reg [160:0] para_name;
   input  range_low;
   input  range_high;

    integer para_in;
    integer range_low;
    integer  range_high;
begin
        if ( para_in < range_low || para_in > range_high) begin
           $display("Attribute Syntax Error : The Attribute %s on PLL_ADV instance %m is set to %d.  Legal values for this attribute are %d to %d.", para_name, para_in, range_low, range_high);
           $finish;
          end
        para_int_range_chk = 1'b1;
end
endfunction

function  para_real_range_chk;
   input  para_in;
   input reg [160:0] para_name;
   input  range_low;
   input  range_high;

   real para_in;
   real range_low;
   real range_high;
begin
        if ( para_in < range_low || para_in > range_high) begin
           $display("Attribute Syntax Error : The Attribute %s on PLL_ADV instance %m is set to %f.  Legal values for this attribute are %f to %f.", para_name, para_in, range_low, range_high);
           $finish;
          end

        para_real_range_chk = 1'b0;
end
endfunction

task clkout_pm_cal;
   output [7:0] clk_ht1;
   output [7:0] clk_div;
   output [7:0] clk_div1;
   input [6:0] clk_ht;
   input [6:0] clk_lt;
   input clk_nocnt;
   input clk_edge;

begin
    if (clk_nocnt ==1) begin
        clk_div = 8'b00000001;
         clk_div1 = 8'b00000001;
        clk_ht1 = 8'b00000001;
    end
    else begin
       if ( clk_edge == 1)
               clk_ht1 = 2 * clk_ht -1;
          else
               clk_ht1 = 2 * clk_ht;
       clk_div = clk_ht  + clk_lt ;
       clk_div1 = 2 * clk_div -1;
    end
end
endtask

task clkout_delay_para_drp;
  output [5:0] clkout_dly;
  output clk_nocnt;
  output clk_edge;
  input [15:0]  di_in;
  input [4:0] daddr_in;
begin

//     if (di_in[15:8] != 8'h00) begin
//          $display(" Error : PLL_ADV on instance %m input DI[15:8] is set to %h and need to be set to 00h at address DADDR=%b at time %t.", di_in[15:8], daddr_in, $time); 
//          $finish;
//     end
     clkout_dly = di_in[5:0];
     clk_nocnt = di_in[6];
     clk_edge = di_in[7];
end
endtask

task clkout_hl_para_drp;
  output  [6:0] clk_lt;
  output  [6:0] clk_ht;
  output  [2:0] clkpm_sel;
  input [15:0] di_in_tmp;
  input [4:0] daddr_in_tmp;
begin
    if (di_in_tmp[12] != 1) begin
         $display(" Error : PLL_ADV on instance %m input DI is %h at address DADDR=%b at time %t. The bit 12 need to be set to 1 .", di_in_tmp, daddr_in_tmp, $time); 
//         $finish;
    end
    if ( di_in_tmp[5:0] == 6'b0)
       clk_lt = 7'b1000000;
    else
        clk_lt = { 1'b0, di_in[5:0]};
    if (di_in_tmp[11:6] == 6'b0)
      clk_ht = 7'b1000000;
    else
       clk_ht = { 1'b0, di_in_tmp[11:6]};
    clkpm_sel = di_in_tmp[15:13];
end
endtask



endmodule
