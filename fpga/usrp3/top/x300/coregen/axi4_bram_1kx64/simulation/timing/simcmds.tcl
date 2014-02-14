# (c) Copyright 2009 - 2010 Xilinx, Inc. All rights reserved.
# 
# This file contains confidential and proprietary information
# of Xilinx, Inc. and is protected under U.S. and
# international copyright and other intellectual property
# laws.
# 
# DISCLAIMER
# This disclaimer is not a license and does not grant any
# rights to the materials distributed herewith. Except as
# otherwise provided in a valid license issued to you by
# Xilinx, and to the maximum extent permitted by applicable
# law: (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND
# WITH ALL FAULTS, AND XILINX HEREBY DISCLAIMS ALL WARRANTIES
# AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, INCLUDING
# BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-
# INFRINGEMENT, OR FITNESS FOR ANY PARTICULAR PURPOSE; and
# (2) Xilinx shall not be liable (whether in contract or tort,
# including negligence, or under any other theory of
# liability) for any loss or damage of any kind or nature
# related to, arising under or in connection with these
# materials, including for any direct, or any indirect,
# special, incidental, or consequential loss or damage
# (including loss of data, profits, goodwill, or any type of
# loss or damage suffered as a result of any action brought
# by a third party) even if such damage or loss was
# reasonably foreseeable or Xilinx had been advised of the
# possibility of the same.
# 
# CRITICAL APPLICATIONS
# Xilinx products are not designed or intended to be fail-
# safe, or for use in any application requiring fail-safe
# performance, such as life-support or safety devices or
# systems, Class III medical devices, nuclear facilities,
# applications related to the deployment of airbags, or any
# other applications that could lead to death, personal
# injury, or severe property or environmental damage
# (individually and collectively, "Critical
# Applications"). Customer assumes the sole risk and
# liability of any use of Xilinx products in Critical
# Applications, subject only to applicable laws and
# regulations governing limitations on product liability.
# 
# THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS
# PART OF THIS FILE AT ALL TIMES.
 
 
 

 



wcfg new
isim set radix hex
wave add /axi4_bram_1kx64_tb/status
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_ACLK
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_AWADDR
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_AWVALID
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_AWREADY
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_AWLEN
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_AWSIZE
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_AWBURST
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_AWID
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_WDATA
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_WSTRB
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_WVALID
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_WREADY
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_WLAST
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_BID
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_BRESP
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_BVALID
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_BREADY
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_ARID
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_ARADDR
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_ARLEN
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_ARSIZE
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_ARBURST
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_ARVALID
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_ARREADY
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_RID
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_RDATA
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_RRESP
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_RLAST
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_RVALID
    wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_AXI_RREADY
      wave add  /axi4_bram_1kx64_tb/axi4_bram_1kx64_synth_inst/BMG_PORT/S_ARESETN
run all
quit
