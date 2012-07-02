// $Header: /devl/xcs/repo/env/Databases/CAEInterfaces/verunilibs/data/rainier/PLL_BASE.v,v 1.5 2006/03/13 21:53:44 yanx Exp $
///////////////////////////////////////////////////////////////////////////////
// Copyright (c) 1995/2004 Xilinx, Inc.
// All Right Reserved.
///////////////////////////////////////////////////////////////////////////////
//   ____  ____
//  /   /\/   /
// /___/  \  /    Vendor : Xilinx
// \   \   \/     Version : 10.1
//  \   \         Description : Xilinx Timing Simulation Library Component
//  /   /                  Phase Lock Loop Clock
// /___/   /\     Filename : PLL_BASE.v
// \   \  /  \    Timestamp : 
//  \___\/\___\
//
// Revision:
//    12/02/05 - Initial version.
//    02/24/06 - Add real/integer to parameter.
// End Revision


`timescale 1 ps / 1 ps 

module PLL_BASE (
	CLKFBOUT,
	CLKOUT0,
	CLKOUT1,
	CLKOUT2,
	CLKOUT3,
	CLKOUT4,
	CLKOUT5,
	LOCKED,
	CLKFBIN,
	CLKIN,
	RST

);

parameter BANDWIDTH = "OPTIMIZED";
parameter integer CLKFBOUT_MULT = 1;
parameter real CLKFBOUT_PHASE = 0.0;
parameter real CLKIN_PERIOD = 0.000;
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
parameter real REF_JITTER = 0.100;
parameter RESET_ON_LOSS_OF_LOCK = "FALSE";


output CLKFBOUT;
output CLKOUT0;
output CLKOUT1;
output CLKOUT2;
output CLKOUT3;
output CLKOUT4;
output CLKOUT5;
output LOCKED;

input CLKFBIN;
input CLKIN;
input RST;


wire OPEN_CLKFBDCM;
wire OPEN_CLKOUTDCM0;
wire OPEN_CLKOUTDCM1;
wire OPEN_CLKOUTDCM2;
wire OPEN_CLKOUTDCM3;
wire OPEN_CLKOUTDCM4;
wire OPEN_CLKOUTDCM5;
wire OPEN_DRDY;
wire [15:0] OPEN_DO;

PLL_ADV pll_adv_1 (
	.CLKFBDCM (OPEN_CLKFBDCM),
	.CLKFBIN (CLKFBIN),
	.CLKFBOUT (CLKFBOUT),
	.CLKIN1 (CLKIN),
	.CLKIN2 (1'b0),
	.CLKOUT0 (CLKOUT0),
	.CLKOUT1 (CLKOUT1),
	.CLKOUT2 (CLKOUT2),
	.CLKOUT3 (CLKOUT3),
	.CLKOUT4 (CLKOUT4),
	.CLKOUT5 (CLKOUT5),
	.CLKOUTDCM0 (OPEN_CLKOUTDCM0),
	.CLKOUTDCM1 (OPEN_CLKOUTDCM1),
	.CLKOUTDCM2 (OPEN_CLKOUTDCM2),
	.CLKOUTDCM3 (OPEN_CLKOUTDCM3),
	.CLKOUTDCM4 (OPEN_CLKOUTDCM4),
	.CLKOUTDCM5 (OPEN_CLKOUTDCM5),
	.DADDR (5'b0),
	.DCLK (1'b0),
	.DEN (1'b0),
	.DI (16'b0),
	.DO (OPEN_DO),
	.DRDY (OPEN_DRDY),
	.DWE (1'b0),
	.LOCKED (LOCKED),
        .CLKINSEL(1'b1),
	.REL (1'b0),
	.RST (RST)
);

defparam pll_adv_1.BANDWIDTH = BANDWIDTH;
defparam pll_adv_1.CLKFBOUT_MULT = CLKFBOUT_MULT;
defparam pll_adv_1.CLKFBOUT_PHASE = CLKFBOUT_PHASE;
defparam pll_adv_1.CLKIN1_PERIOD = CLKIN_PERIOD;
defparam pll_adv_1.CLKIN2_PERIOD = 10.0;
defparam pll_adv_1.CLKOUT0_DIVIDE = CLKOUT0_DIVIDE;
defparam pll_adv_1.CLKOUT0_DUTY_CYCLE = CLKOUT0_DUTY_CYCLE;
defparam pll_adv_1.CLKOUT0_PHASE = CLKOUT0_PHASE;
defparam pll_adv_1.CLKOUT1_DIVIDE = CLKOUT1_DIVIDE;
defparam pll_adv_1.CLKOUT1_DUTY_CYCLE = CLKOUT1_DUTY_CYCLE;
defparam pll_adv_1.CLKOUT1_PHASE = CLKOUT1_PHASE;
defparam pll_adv_1.CLKOUT2_DIVIDE = CLKOUT2_DIVIDE;
defparam pll_adv_1.CLKOUT2_DUTY_CYCLE = CLKOUT2_DUTY_CYCLE;
defparam pll_adv_1.CLKOUT2_PHASE = CLKOUT2_PHASE;
defparam pll_adv_1.CLKOUT3_DIVIDE = CLKOUT3_DIVIDE;
defparam pll_adv_1.CLKOUT3_DUTY_CYCLE = CLKOUT3_DUTY_CYCLE;
defparam pll_adv_1.CLKOUT3_PHASE = CLKOUT3_PHASE;
defparam pll_adv_1.CLKOUT4_DIVIDE = CLKOUT4_DIVIDE;
defparam pll_adv_1.CLKOUT4_DUTY_CYCLE = CLKOUT4_DUTY_CYCLE;
defparam pll_adv_1.CLKOUT4_PHASE = CLKOUT4_PHASE;
defparam pll_adv_1.CLKOUT5_DIVIDE = CLKOUT5_DIVIDE;
defparam pll_adv_1.CLKOUT5_DUTY_CYCLE = CLKOUT5_DUTY_CYCLE;
defparam pll_adv_1.CLKOUT5_PHASE = CLKOUT5_PHASE;
defparam pll_adv_1.COMPENSATION = COMPENSATION;
defparam pll_adv_1.DIVCLK_DIVIDE = DIVCLK_DIVIDE;
defparam pll_adv_1.REF_JITTER = REF_JITTER;
defparam pll_adv_1.RESET_ON_LOSS_OF_LOCK = RESET_ON_LOSS_OF_LOCK;

endmodule
