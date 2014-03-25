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
view structure
view signals
view wave
onerror {resume}
quietly WaveActivateNextPane {} 0
#
add wave -noupdate -divider {System Signals}
add wave -noupdate -format logic /demo_tb/reset
add wave -noupdate -format logic /demo_tb/refclk_p
add wave -noupdate -format logic /demo_tb/refclk_n
add wave -noupdate -format logic /demo_tb/core_clk156_out
#
add wave -noupdate -divider {XGMII Signals}
add wave -noupdate -format logic -hex /demo_tb/xgmii_txd
add wave -noupdate -format logic -binary /demo_tb/xgmii_txc
add wave -noupdate -format logic -binary /demo_tb/xgmii_rx_clk
add wave -noupdate -format logic -hex /demo_tb/xgmii_rxd
add wave -noupdate -format logic -binary /demo_tb/xgmii_rxc
#
add wave -noupdate -divider {Serial Signals}
add wave -noupdate -format logic -binary /demo_tb/txp
add wave -noupdate -format logic -binary /demo_tb/txn
add wave -noupdate -format logic -binary /demo_tb/rxp
add wave -noupdate -format logic -binary /demo_tb/rxn
#
add wave -noupdate -divider {Control Signals}
add wave -noupdate -format logic -binary /demo_tb/resetdone
add wave -noupdate -format logic -binary /demo_tb/signal_detect
add wave -noupdate -format logic -binary /demo_tb/tx_fault
add wave -noupdate -format logic -binary /demo_tb/tx_disable
#
add wave -noupdate -divider {Management signals}
add wave -noupdate -format logic -binary /demo_tb/mdc
add wave -noupdate -format logic -binary /demo_tb/mdio_in
add wave -noupdate -format logic -binary /demo_tb/mdio_out
add wave -noupdate -format logic -binary /demo_tb/mdio_tri
add wave -noupdate -format logic -hex /demo_tb/prtad

TreeUpdate [SetDefaultTree]
