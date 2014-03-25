/*******************************************************************************
*     This file is owned and controlled by Xilinx and must be used solely      *
*     for design, simulation, implementation and creation of design files      *
*     limited to Xilinx devices or technologies. Use with non-Xilinx           *
*     devices or technologies is expressly prohibited and immediately          *
*     terminates your license.                                                 *
*                                                                              *
*     XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS" SOLELY     *
*     FOR USE IN DEVELOPING PROGRAMS AND SOLUTIONS FOR XILINX DEVICES.  BY     *
*     PROVIDING THIS DESIGN, CODE, OR INFORMATION AS ONE POSSIBLE              *
*     IMPLEMENTATION OF THIS FEATURE, APPLICATION OR STANDARD, XILINX IS       *
*     MAKING NO REPRESENTATION THAT THIS IMPLEMENTATION IS FREE FROM ANY       *
*     CLAIMS OF INFRINGEMENT, AND YOU ARE RESPONSIBLE FOR OBTAINING ANY        *
*     RIGHTS YOU MAY REQUIRE FOR YOUR IMPLEMENTATION.  XILINX EXPRESSLY        *
*     DISCLAIMS ANY WARRANTY WHATSOEVER WITH RESPECT TO THE ADEQUACY OF THE    *
*     IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OR           *
*     REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE FROM CLAIMS OF          *
*     INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A    *
*     PARTICULAR PURPOSE.                                                      *
*                                                                              *
*     Xilinx products are not intended for use in life support appliances,     *
*     devices, or systems.  Use in such applications are expressly             *
*     prohibited.                                                              *
*                                                                              *
*     (c) Copyright 1995-2013 Xilinx, Inc.                                     *
*     All rights reserved.                                                     *
*******************************************************************************/

/*******************************************************************************
*     Generated from core with identifier:                                     *
*     xilinx.com:ip:axi_interconnect:1.06.a                                    *
*                                                                              *
*     The AXI Interconnect core connects one or more AXI4 memory-mapped        *
*     master devices to one AXI4 slave device.                                 *
*******************************************************************************/

// Interfaces:
//    AXI4_SLAVE_S00_AXI
//    AXI4_SLAVE_S11_AXI
//    AXI4_SLAVE_S12_AXI
//    AXI4_SLAVE_S10_AXI
//    AXI4_SLAVE_S08_AXI
//    AXI4_SLAVE_S13_AXI
//    AXI4_SLAVE_S02_AXI
//    AXI4_MASTER_M00_AXI
//    AXI4_SLAVE_S15_AXI
//    AXI4_SLAVE_S06_AXI
//    AXI4_SLAVE_S01_AXI
//    AXI4_SLAVE_S09_AXI
//    AXI4_SLAVE_S14_AXI
//    AXI4_SLAVE_S03_AXI
//    AXI4_SLAVE_S04_AXI
//    AXI4_SLAVE_S05_AXI
//    AXI4_SLAVE_S07_AXI

// The following must be inserted into your Verilog file for this
// core to be instantiated. Change the instance name and port connections
// (in parentheses) to your own signal names.

//----------- Begin Cut here for INSTANTIATION Template ---// INST_TAG
axi_intercon_2x64_128 your_instance_name (
  .INTERCONNECT_ACLK(INTERCONNECT_ACLK), // input INTERCONNECT_ACLK
  .INTERCONNECT_ARESETN(INTERCONNECT_ARESETN), // input INTERCONNECT_ARESETN
  .S00_AXI_ARESET_OUT_N(S00_AXI_ARESET_OUT_N), // output S00_AXI_ARESET_OUT_N
  .S00_AXI_ACLK(S00_AXI_ACLK), // input S00_AXI_ACLK
  .S00_AXI_AWID(S00_AXI_AWID), // input [0 : 0] S00_AXI_AWID
  .S00_AXI_AWADDR(S00_AXI_AWADDR), // input [31 : 0] S00_AXI_AWADDR
  .S00_AXI_AWLEN(S00_AXI_AWLEN), // input [7 : 0] S00_AXI_AWLEN
  .S00_AXI_AWSIZE(S00_AXI_AWSIZE), // input [2 : 0] S00_AXI_AWSIZE
  .S00_AXI_AWBURST(S00_AXI_AWBURST), // input [1 : 0] S00_AXI_AWBURST
  .S00_AXI_AWLOCK(S00_AXI_AWLOCK), // input S00_AXI_AWLOCK
  .S00_AXI_AWCACHE(S00_AXI_AWCACHE), // input [3 : 0] S00_AXI_AWCACHE
  .S00_AXI_AWPROT(S00_AXI_AWPROT), // input [2 : 0] S00_AXI_AWPROT
  .S00_AXI_AWQOS(S00_AXI_AWQOS), // input [3 : 0] S00_AXI_AWQOS
  .S00_AXI_AWVALID(S00_AXI_AWVALID), // input S00_AXI_AWVALID
  .S00_AXI_AWREADY(S00_AXI_AWREADY), // output S00_AXI_AWREADY
  .S00_AXI_WDATA(S00_AXI_WDATA), // input [63 : 0] S00_AXI_WDATA
  .S00_AXI_WSTRB(S00_AXI_WSTRB), // input [7 : 0] S00_AXI_WSTRB
  .S00_AXI_WLAST(S00_AXI_WLAST), // input S00_AXI_WLAST
  .S00_AXI_WVALID(S00_AXI_WVALID), // input S00_AXI_WVALID
  .S00_AXI_WREADY(S00_AXI_WREADY), // output S00_AXI_WREADY
  .S00_AXI_BID(S00_AXI_BID), // output [0 : 0] S00_AXI_BID
  .S00_AXI_BRESP(S00_AXI_BRESP), // output [1 : 0] S00_AXI_BRESP
  .S00_AXI_BVALID(S00_AXI_BVALID), // output S00_AXI_BVALID
  .S00_AXI_BREADY(S00_AXI_BREADY), // input S00_AXI_BREADY
  .S00_AXI_ARID(S00_AXI_ARID), // input [0 : 0] S00_AXI_ARID
  .S00_AXI_ARADDR(S00_AXI_ARADDR), // input [31 : 0] S00_AXI_ARADDR
  .S00_AXI_ARLEN(S00_AXI_ARLEN), // input [7 : 0] S00_AXI_ARLEN
  .S00_AXI_ARSIZE(S00_AXI_ARSIZE), // input [2 : 0] S00_AXI_ARSIZE
  .S00_AXI_ARBURST(S00_AXI_ARBURST), // input [1 : 0] S00_AXI_ARBURST
  .S00_AXI_ARLOCK(S00_AXI_ARLOCK), // input S00_AXI_ARLOCK
  .S00_AXI_ARCACHE(S00_AXI_ARCACHE), // input [3 : 0] S00_AXI_ARCACHE
  .S00_AXI_ARPROT(S00_AXI_ARPROT), // input [2 : 0] S00_AXI_ARPROT
  .S00_AXI_ARQOS(S00_AXI_ARQOS), // input [3 : 0] S00_AXI_ARQOS
  .S00_AXI_ARVALID(S00_AXI_ARVALID), // input S00_AXI_ARVALID
  .S00_AXI_ARREADY(S00_AXI_ARREADY), // output S00_AXI_ARREADY
  .S00_AXI_RID(S00_AXI_RID), // output [0 : 0] S00_AXI_RID
  .S00_AXI_RDATA(S00_AXI_RDATA), // output [63 : 0] S00_AXI_RDATA
  .S00_AXI_RRESP(S00_AXI_RRESP), // output [1 : 0] S00_AXI_RRESP
  .S00_AXI_RLAST(S00_AXI_RLAST), // output S00_AXI_RLAST
  .S00_AXI_RVALID(S00_AXI_RVALID), // output S00_AXI_RVALID
  .S00_AXI_RREADY(S00_AXI_RREADY), // input S00_AXI_RREADY
  .S01_AXI_ARESET_OUT_N(S01_AXI_ARESET_OUT_N), // output S01_AXI_ARESET_OUT_N
  .S01_AXI_ACLK(S01_AXI_ACLK), // input S01_AXI_ACLK
  .S01_AXI_AWID(S01_AXI_AWID), // input [0 : 0] S01_AXI_AWID
  .S01_AXI_AWADDR(S01_AXI_AWADDR), // input [31 : 0] S01_AXI_AWADDR
  .S01_AXI_AWLEN(S01_AXI_AWLEN), // input [7 : 0] S01_AXI_AWLEN
  .S01_AXI_AWSIZE(S01_AXI_AWSIZE), // input [2 : 0] S01_AXI_AWSIZE
  .S01_AXI_AWBURST(S01_AXI_AWBURST), // input [1 : 0] S01_AXI_AWBURST
  .S01_AXI_AWLOCK(S01_AXI_AWLOCK), // input S01_AXI_AWLOCK
  .S01_AXI_AWCACHE(S01_AXI_AWCACHE), // input [3 : 0] S01_AXI_AWCACHE
  .S01_AXI_AWPROT(S01_AXI_AWPROT), // input [2 : 0] S01_AXI_AWPROT
  .S01_AXI_AWQOS(S01_AXI_AWQOS), // input [3 : 0] S01_AXI_AWQOS
  .S01_AXI_AWVALID(S01_AXI_AWVALID), // input S01_AXI_AWVALID
  .S01_AXI_AWREADY(S01_AXI_AWREADY), // output S01_AXI_AWREADY
  .S01_AXI_WDATA(S01_AXI_WDATA), // input [63 : 0] S01_AXI_WDATA
  .S01_AXI_WSTRB(S01_AXI_WSTRB), // input [7 : 0] S01_AXI_WSTRB
  .S01_AXI_WLAST(S01_AXI_WLAST), // input S01_AXI_WLAST
  .S01_AXI_WVALID(S01_AXI_WVALID), // input S01_AXI_WVALID
  .S01_AXI_WREADY(S01_AXI_WREADY), // output S01_AXI_WREADY
  .S01_AXI_BID(S01_AXI_BID), // output [0 : 0] S01_AXI_BID
  .S01_AXI_BRESP(S01_AXI_BRESP), // output [1 : 0] S01_AXI_BRESP
  .S01_AXI_BVALID(S01_AXI_BVALID), // output S01_AXI_BVALID
  .S01_AXI_BREADY(S01_AXI_BREADY), // input S01_AXI_BREADY
  .S01_AXI_ARID(S01_AXI_ARID), // input [0 : 0] S01_AXI_ARID
  .S01_AXI_ARADDR(S01_AXI_ARADDR), // input [31 : 0] S01_AXI_ARADDR
  .S01_AXI_ARLEN(S01_AXI_ARLEN), // input [7 : 0] S01_AXI_ARLEN
  .S01_AXI_ARSIZE(S01_AXI_ARSIZE), // input [2 : 0] S01_AXI_ARSIZE
  .S01_AXI_ARBURST(S01_AXI_ARBURST), // input [1 : 0] S01_AXI_ARBURST
  .S01_AXI_ARLOCK(S01_AXI_ARLOCK), // input S01_AXI_ARLOCK
  .S01_AXI_ARCACHE(S01_AXI_ARCACHE), // input [3 : 0] S01_AXI_ARCACHE
  .S01_AXI_ARPROT(S01_AXI_ARPROT), // input [2 : 0] S01_AXI_ARPROT
  .S01_AXI_ARQOS(S01_AXI_ARQOS), // input [3 : 0] S01_AXI_ARQOS
  .S01_AXI_ARVALID(S01_AXI_ARVALID), // input S01_AXI_ARVALID
  .S01_AXI_ARREADY(S01_AXI_ARREADY), // output S01_AXI_ARREADY
  .S01_AXI_RID(S01_AXI_RID), // output [0 : 0] S01_AXI_RID
  .S01_AXI_RDATA(S01_AXI_RDATA), // output [63 : 0] S01_AXI_RDATA
  .S01_AXI_RRESP(S01_AXI_RRESP), // output [1 : 0] S01_AXI_RRESP
  .S01_AXI_RLAST(S01_AXI_RLAST), // output S01_AXI_RLAST
  .S01_AXI_RVALID(S01_AXI_RVALID), // output S01_AXI_RVALID
  .S01_AXI_RREADY(S01_AXI_RREADY), // input S01_AXI_RREADY
  .M00_AXI_ARESET_OUT_N(M00_AXI_ARESET_OUT_N), // output M00_AXI_ARESET_OUT_N
  .M00_AXI_ACLK(M00_AXI_ACLK), // input M00_AXI_ACLK
  .M00_AXI_AWID(M00_AXI_AWID), // output [3 : 0] M00_AXI_AWID
  .M00_AXI_AWADDR(M00_AXI_AWADDR), // output [31 : 0] M00_AXI_AWADDR
  .M00_AXI_AWLEN(M00_AXI_AWLEN), // output [7 : 0] M00_AXI_AWLEN
  .M00_AXI_AWSIZE(M00_AXI_AWSIZE), // output [2 : 0] M00_AXI_AWSIZE
  .M00_AXI_AWBURST(M00_AXI_AWBURST), // output [1 : 0] M00_AXI_AWBURST
  .M00_AXI_AWLOCK(M00_AXI_AWLOCK), // output M00_AXI_AWLOCK
  .M00_AXI_AWCACHE(M00_AXI_AWCACHE), // output [3 : 0] M00_AXI_AWCACHE
  .M00_AXI_AWPROT(M00_AXI_AWPROT), // output [2 : 0] M00_AXI_AWPROT
  .M00_AXI_AWQOS(M00_AXI_AWQOS), // output [3 : 0] M00_AXI_AWQOS
  .M00_AXI_AWVALID(M00_AXI_AWVALID), // output M00_AXI_AWVALID
  .M00_AXI_AWREADY(M00_AXI_AWREADY), // input M00_AXI_AWREADY
  .M00_AXI_WDATA(M00_AXI_WDATA), // output [127 : 0] M00_AXI_WDATA
  .M00_AXI_WSTRB(M00_AXI_WSTRB), // output [15 : 0] M00_AXI_WSTRB
  .M00_AXI_WLAST(M00_AXI_WLAST), // output M00_AXI_WLAST
  .M00_AXI_WVALID(M00_AXI_WVALID), // output M00_AXI_WVALID
  .M00_AXI_WREADY(M00_AXI_WREADY), // input M00_AXI_WREADY
  .M00_AXI_BID(M00_AXI_BID), // input [3 : 0] M00_AXI_BID
  .M00_AXI_BRESP(M00_AXI_BRESP), // input [1 : 0] M00_AXI_BRESP
  .M00_AXI_BVALID(M00_AXI_BVALID), // input M00_AXI_BVALID
  .M00_AXI_BREADY(M00_AXI_BREADY), // output M00_AXI_BREADY
  .M00_AXI_ARID(M00_AXI_ARID), // output [3 : 0] M00_AXI_ARID
  .M00_AXI_ARADDR(M00_AXI_ARADDR), // output [31 : 0] M00_AXI_ARADDR
  .M00_AXI_ARLEN(M00_AXI_ARLEN), // output [7 : 0] M00_AXI_ARLEN
  .M00_AXI_ARSIZE(M00_AXI_ARSIZE), // output [2 : 0] M00_AXI_ARSIZE
  .M00_AXI_ARBURST(M00_AXI_ARBURST), // output [1 : 0] M00_AXI_ARBURST
  .M00_AXI_ARLOCK(M00_AXI_ARLOCK), // output M00_AXI_ARLOCK
  .M00_AXI_ARCACHE(M00_AXI_ARCACHE), // output [3 : 0] M00_AXI_ARCACHE
  .M00_AXI_ARPROT(M00_AXI_ARPROT), // output [2 : 0] M00_AXI_ARPROT
  .M00_AXI_ARQOS(M00_AXI_ARQOS), // output [3 : 0] M00_AXI_ARQOS
  .M00_AXI_ARVALID(M00_AXI_ARVALID), // output M00_AXI_ARVALID
  .M00_AXI_ARREADY(M00_AXI_ARREADY), // input M00_AXI_ARREADY
  .M00_AXI_RID(M00_AXI_RID), // input [3 : 0] M00_AXI_RID
  .M00_AXI_RDATA(M00_AXI_RDATA), // input [127 : 0] M00_AXI_RDATA
  .M00_AXI_RRESP(M00_AXI_RRESP), // input [1 : 0] M00_AXI_RRESP
  .M00_AXI_RLAST(M00_AXI_RLAST), // input M00_AXI_RLAST
  .M00_AXI_RVALID(M00_AXI_RVALID), // input M00_AXI_RVALID
  .M00_AXI_RREADY(M00_AXI_RREADY) // output M00_AXI_RREADY
);
// INST_TAG_END ------ End INSTANTIATION Template ---------

// You must compile the wrapper file axi_intercon_2x64_128.v when simulating
// the core, axi_intercon_2x64_128. When compiling the wrapper file, be sure to
// reference the XilinxCoreLib Verilog simulation library. For detailed
// instructions, please refer to the "CORE Generator Help".

