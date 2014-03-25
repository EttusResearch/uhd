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
# SimVision Command Script

#
# groups
#

if {[catch {group new -name {System Signals} -overlay 0}] != ""} {
    group using {System Signals}
    group set -overlay 0
    group set -comment {}
    group clear 0 end
}
group insert \
    demo_tb.reset \
    demo_tb.refclk_p \
    demo_tb.refclk_n \
    demo_tb.core_clk156_out

if {[catch {group new -name {XGMII Signals} -overlay 0}] != ""} {
    group using {XGMII Signals}
    group set -overlay 0
    group set -comment {}
    group clear 0 end
}
group insert \
    demo_tb.xgmii_txd \
    demo_tb.xgmii_txc \
    demo_tb.xgmii_rx_clk \
    demo_tb.xgmii_rxd \
    demo_tb.xgmii_rxc

if {[catch {group new -name {Serial Signals} -overlay 0}] != ""} {
    group using {Serial Signals}
    group set -overlay 0
    group set -comment {}
    group clear 0 end
}
group insert \
    demo_tb.txp \
    demo_tb.txn \
    demo_tb.rxp \
    demo_tb.rxn 

if {[catch {group new -name {Control Signals} -overlay 0}] != ""} {
    group using {Control Signals}
    group set -overlay 0
    group set -comment {}
    group clear 0 end
}

group insert \
    demo_tb.resetdone \
    demo_tb.signal_detect \
    demo_tb.tx_fault \
    demo_tb.tx_disable

if {[catch {group new -name {Management signals} -overlay 0}] != ""} {
    group using {Management signals}
    group set -overlay 0
    group set -comment {}
    group clear 0 end
}
group insert \
    demo_tb.mdc \
    demo_tb.mdio_in \
    demo_tb.mdio_out \
    demo_tb.mdio_tri \
    demo_tb.prtad 

#
# Waveform windows
#
if {[window find -match exact -name "Waveform 1"] == {}} {
    window new WaveWindow -name "Waveform 1" -geometry 906x585+25+55
} else {
    window geometry "Waveform 1" 906x585+25+55
}
window target "Waveform 1" on
waveform using {Waveform 1}
waveform sidebar visibility partial
waveform set \
    -primarycursor TimeA \
    -signalnames name \
    -signalwidth 175 \
    -units fs \
    -valuewidth 75
cursor set -using TimeA -time 50,000,000,000fs
cursor set -using TimeA -marching 1
waveform baseline set -time 0

set groupId [waveform add -groups {{System Signals}}]
set groupId [waveform add -groups {{XGMII Signals}}]
set groupId [waveform add -groups {{Serial Signals}}]
set groupId [waveform add -groups {{Control Signals}}]
set groupId [waveform add -groups {{Management signals}}]

waveform xview limits 0fs 10us

simcontrol run -time 500us
