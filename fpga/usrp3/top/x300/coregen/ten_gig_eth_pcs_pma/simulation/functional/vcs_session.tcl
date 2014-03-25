## (c) Copyright 2009 - 2012 Xilinx, Inc. All rights reserved.
##
## This file contains confidential and proprietary information
## of Xilinx, Inc. and is protected under U.S. and 
## international copyright and other intellectual property
## laws.
##
## DISCLAIMER
## This disclaimer is not a license and does not grant any
## rights to the materials distributed herewith. Except as
## otherwise provided in a valid license issued to you by
## Xilinx, and to the maximum extent permitted by applicable
## law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
## WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
## AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
## BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
## INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
## (2) Xilinx shall not be liable (whether in contract or tort,
## including negligence, or under any other theory of
## liability) for any loss or damage of any kind or nature
## related to, arising under or in connection with these
## materials, including for any direct, or any indirect,
## special, incidental, or consequential loss or damage
## (including loss of data, profits, goodwill, or any type of
## loss or damage suffered as a result of any action brought
## by a third party) even if such damage or loss was
## reasonably foreseeable or Xilinx had been advised of the
## possibility of the same.
##
## CRITICAL APPLICATIONS
## Xilinx products are not designed or intended to be fail-
## safe, or for use in any application requiring fail-safe
## performance, such as life-support or safety devices or
## systems, Class III medical devices, nuclear facilities,
## applications related to the deployment of airbags, or any
## other applications that could lead to death, personal
## injury, or severe property or environmental damage
## (individually and collectively, "Critical
## Applications"). Customer assumes the sole risk and
## liability of any use of Xilinx products in Critical
## Applications, subject only to applicable laws and
## regulations governing limitations on product liability.
##
## THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
## PART OF THIS FILE AT ALL TIMES.
gui_open_window Wave
gui_sg_create 10GBASER_Group
gui_list_add_group -id Wave.1 {10GBASER_Group}
gui_sg_addsignal -group 10GBASER_Group {{System_Signals}} -divider
gui_sg_addsignal -group 10GBASER_Group {{demo_tb.reset}}
gui_sg_addsignal -group 10GBASER_Group {{demo_tb.refclk_p} {demo_tb.refclk_n}}
gui_sg_addsignal -group 10GBASER_Group {{demo_tb.core_clk156_out}}
gui_sg_addsignal -group 10GBASER_Group {{XGMII_Signals}} -divider
gui_sg_addsignal -group 10GBASER_Group {{demo_tb.xgmii_txd} {demo_tb.xgmii_txc}}
gui_sg_addsignal -group 10GBASER_Group {{demo_tb.xgmii_rx_clk}}
gui_sg_addsignal -group 10GBASER_Group {{demo_tb.xgmii_rxd} {demo_tb.xgmii_rxc}}
gui_sg_addsignal -group 10GBASER_Group {{Serial_Signals}} -divider
gui_sg_addsignal -group 10GBASER_Group {{demo_tb.txp} {demo_tb.txn} {demo_tb.rxp} {demo_tb.rxn}}
gui_sg_addsignal -group 10GBASER_Group {{Control_Signals}} -divider
gui_sg_addsignal -group 10GBASER_Group {{demo_tb.resetdone} {demo_tb.signal_detect} {demo_tb.tx_fault} {demo_tb.tx_disable}}
gui_sg_addsignal -group 10GBASER_Group {{Management_Signals}} -divider
gui_sg_addsignal -group 10GBASER_Group {{demo_tb.mdc} {demo_tb.mdio_in} {demo_tb.mdio_out} {demo_tb.mdio_tri} {demo_tb.prtad}}
gui_zoom -window Wave.1 -full
