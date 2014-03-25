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
*     Generated from core with identifier: xilinx.com:ip:fifo_generator:9.3    *
*                                                                              *
*     Rev 1. The FIFO Generator is a parameterizable first-in/first-out        *
*     memory queue generator. Use it to generate resource and performance      *
*     optimized FIFOs with common or independent read/write clock domains,     *
*     and optional fixed or programmable full and empty flags and              *
*     handshaking signals.  Choose from a selection of memory resource         *
*     types for implementation.  Optional Hamming code based error             *
*     detection and correction as well as error injection capability for       *
*     system test help to insure data integrity.  FIFO width and depth are     *
*     parameterizable, and for native interface FIFOs, asymmetric read and     *
*     write port widths are also supported.                                    *
*******************************************************************************/

// Interfaces:
//    AXI4Stream_MASTER_M_AXIS
//    AXI4Stream_SLAVE_S_AXIS
//    AXI4_MASTER_M_AXI
//    AXI4_SLAVE_S_AXI
//    AXI4Lite_MASTER_M_AXI
//    AXI4Lite_SLAVE_S_AXI
//    master_aclk
//    slave_aclk
//    slave_aresetn

// The following must be inserted into your Verilog file for this
// core to be instantiated. Change the instance name and port connections
// (in parentheses) to your own signal names.

//----------- Begin Cut here for INSTANTIATION Template ---// INST_TAG
axi64_4k_2clk_fifo your_instance_name (
  .m_aclk(m_aclk), // input m_aclk
  .s_aclk(s_aclk), // input s_aclk
  .s_aresetn(s_aresetn), // input s_aresetn
  .s_axis_tvalid(s_axis_tvalid), // input s_axis_tvalid
  .s_axis_tready(s_axis_tready), // output s_axis_tready
  .s_axis_tdata(s_axis_tdata), // input [63 : 0] s_axis_tdata
  .s_axis_tlast(s_axis_tlast), // input s_axis_tlast
  .s_axis_tuser(s_axis_tuser), // input [3 : 0] s_axis_tuser
  .m_axis_tvalid(m_axis_tvalid), // output m_axis_tvalid
  .m_axis_tready(m_axis_tready), // input m_axis_tready
  .m_axis_tdata(m_axis_tdata), // output [63 : 0] m_axis_tdata
  .m_axis_tlast(m_axis_tlast), // output m_axis_tlast
  .m_axis_tuser(m_axis_tuser), // output [3 : 0] m_axis_tuser
  .axis_wr_data_count(axis_wr_data_count), // output [9 : 0] axis_wr_data_count
  .axis_rd_data_count(axis_rd_data_count) // output [9 : 0] axis_rd_data_count
);
// INST_TAG_END ------ End INSTANTIATION Template ---------

// You must compile the wrapper file axi64_4k_2clk_fifo.v when simulating
// the core, axi64_4k_2clk_fifo. When compiling the wrapper file, be sure to
// reference the XilinxCoreLib Verilog simulation library. For detailed
// instructions, please refer to the "CORE Generator Help".

