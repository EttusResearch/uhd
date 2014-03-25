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
*     (c) Copyright 1995-2012 Xilinx, Inc.                                     *
*     All rights reserved.                                                     *
*******************************************************************************/

/*******************************************************************************
*     Generated from core with identifier: xilinx.com:ip:axi_vfifo_ctrl:1.1    *
*                                                                              *
*     The AXI Virtual FIFO Controller is a parameterizable number of multi     *
*     channel FIFO.                                                            *
*******************************************************************************/
// Synthesized Netlist Wrapper
// This file is provided to wrap around the synthesized netlist (if appropriate)

// Interfaces:
//    AXI4Stream_MASTER_M_AXIS
//    AXI4Stream_SLAVE_S_AXIS
//    AXI4_MASTER_M_AXI

module axi_vfifo_64 (
  aclk,
  aresetn,
  m_axi_awid,
  m_axi_awaddr,
  m_axi_awlen,
  m_axi_awsize,
  m_axi_awburst,
  m_axi_awlock,
  m_axi_awcache,
  m_axi_awprot,
  m_axi_awqos,
  m_axi_awregion,
  m_axi_awuser,
  m_axi_awvalid,
  m_axi_awready,
  m_axi_wdata,
  m_axi_wstrb,
  m_axi_wlast,
  m_axi_wuser,
  m_axi_wvalid,
  m_axi_wready,
  m_axi_bid,
  m_axi_bresp,
  m_axi_buser,
  m_axi_bvalid,
  m_axi_bready,
  m_axi_arid,
  m_axi_araddr,
  m_axi_arlen,
  m_axi_arsize,
  m_axi_arburst,
  m_axi_arlock,
  m_axi_arcache,
  m_axi_arprot,
  m_axi_arqos,
  m_axi_arregion,
  m_axi_aruser,
  m_axi_arvalid,
  m_axi_arready,
  m_axi_rid,
  m_axi_rdata,
  m_axi_rresp,
  m_axi_rlast,
  m_axi_ruser,
  m_axi_rvalid,
  m_axi_rready,
  s_axis_tvalid,
  s_axis_tready,
  s_axis_tdata,
  s_axis_tstrb,
  s_axis_tkeep,
  s_axis_tlast,
  s_axis_tid,
  s_axis_tdest,
  m_axis_tvalid,
  m_axis_tready,
  m_axis_tdata,
  m_axis_tstrb,
  m_axis_tkeep,
  m_axis_tlast,
  m_axis_tid,
  m_axis_tdest,
  vfifo_mm2s_channel_full,
  vfifo_s2mm_channel_full,
  vfifo_mm2s_channel_empty,
  vfifo_mm2s_rresp_err_intr,
  vfifo_s2mm_bresp_err_intr,
  vfifo_s2mm_overrun_err_intr,
  vfifo_idle
);

  input aclk;
  input aresetn;
  output [0 : 0] m_axi_awid;
  output [31 : 0] m_axi_awaddr;
  output [7 : 0] m_axi_awlen;
  output [2 : 0] m_axi_awsize;
  output [1 : 0] m_axi_awburst;
  output [0 : 0] m_axi_awlock;
  output [3 : 0] m_axi_awcache;
  output [2 : 0] m_axi_awprot;
  output [3 : 0] m_axi_awqos;
  output [3 : 0] m_axi_awregion;
  output [0 : 0] m_axi_awuser;
  output m_axi_awvalid;
  input m_axi_awready;
  output [63 : 0] m_axi_wdata;
  output [7 : 0] m_axi_wstrb;
  output m_axi_wlast;
  output [0 : 0] m_axi_wuser;
  output m_axi_wvalid;
  input m_axi_wready;
  input [0 : 0] m_axi_bid;
  input [1 : 0] m_axi_bresp;
  input [0 : 0] m_axi_buser;
  input m_axi_bvalid;
  output m_axi_bready;
  output [0 : 0] m_axi_arid;
  output [31 : 0] m_axi_araddr;
  output [7 : 0] m_axi_arlen;
  output [2 : 0] m_axi_arsize;
  output [1 : 0] m_axi_arburst;
  output [0 : 0] m_axi_arlock;
  output [3 : 0] m_axi_arcache;
  output [2 : 0] m_axi_arprot;
  output [3 : 0] m_axi_arqos;
  output [3 : 0] m_axi_arregion;
  output [0 : 0] m_axi_aruser;
  output m_axi_arvalid;
  input m_axi_arready;
  input [0 : 0] m_axi_rid;
  input [63 : 0] m_axi_rdata;
  input [1 : 0] m_axi_rresp;
  input m_axi_rlast;
  input [0 : 0] m_axi_ruser;
  input m_axi_rvalid;
  output m_axi_rready;
  input s_axis_tvalid;
  output s_axis_tready;
  input [63 : 0] s_axis_tdata;
  input [7 : 0] s_axis_tstrb;
  input [7 : 0] s_axis_tkeep;
  input s_axis_tlast;
  input [0 : 0] s_axis_tid;
  input [0 : 0] s_axis_tdest;
  output m_axis_tvalid;
  input m_axis_tready;
  output [63 : 0] m_axis_tdata;
  output [7 : 0] m_axis_tstrb;
  output [7 : 0] m_axis_tkeep;
  output m_axis_tlast;
  output [0 : 0] m_axis_tid;
  output [0 : 0] m_axis_tdest;
  input [1 : 0] vfifo_mm2s_channel_full;
  output [1 : 0] vfifo_s2mm_channel_full;
  output [1 : 0] vfifo_mm2s_channel_empty;
  output vfifo_mm2s_rresp_err_intr;
  output vfifo_s2mm_bresp_err_intr;
  output vfifo_s2mm_overrun_err_intr;
  output [1 : 0] vfifo_idle;

  // WARNING: This file provides a module declaration only, it does not support
  //          direct instantiation. Please use an instantiation template (VEO) to
  //          instantiate the IP within a design.

endmodule

