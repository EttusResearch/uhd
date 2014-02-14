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
*     xilinx.com:ip:ten_gig_eth_pcs_pma:2.6                                    *
*                                                                              *
*     The Xilinx Ten Gigabit Ethernet PCS/PMA (10GBASE-R) core is designed     *
*     to the IEEE specification 802.3 2008. An example design and              *
*     development scripts are provided to accelerate your design cycle. For    *
*     10GBASE-KR interest, please contact your local Xilinx sales              *
*     representative.                                                          *
*******************************************************************************/

// The following must be inserted into your Verilog file for this
// core to be instantiated. Change the instance name and port connections
// (in parentheses) to your own signal names.

//----------- Begin Cut here for INSTANTIATION Template ---// INST_TAG
ten_gig_eth_pcs_pma your_instance_name (
  .reset(reset), // input reset
  .txreset322(txreset322), // input txreset322
  .rxreset322(rxreset322), // input rxreset322
  .dclk_reset(dclk_reset), // input dclk_reset
  .pma_resetout(pma_resetout), // output pma_resetout
  .pcs_resetout(pcs_resetout), // output pcs_resetout
  .clk156(clk156), // input clk156
  .txusrclk2(txusrclk2), // input txusrclk2
  .rxusrclk2(rxusrclk2), // input rxusrclk2
  .dclk(dclk), // input dclk
  .xgmii_txd(xgmii_txd), // input [63 : 0] xgmii_txd
  .xgmii_txc(xgmii_txc), // input [7 : 0] xgmii_txc
  .xgmii_rxd(xgmii_rxd), // output [63 : 0] xgmii_rxd
  .xgmii_rxc(xgmii_rxc), // output [7 : 0] xgmii_rxc
  .mdc(mdc), // input mdc
  .mdio_in(mdio_in), // input mdio_in
  .mdio_out(mdio_out), // output mdio_out
  .mdio_tri(mdio_tri), // output mdio_tri
  .prtad(prtad), // input [4 : 0] prtad
  .pma_pmd_type(pma_pmd_type), // input [2 : 0] pma_pmd_type
  .core_status(core_status), // output [7 : 0] core_status
  .resetdone(resetdone), // input resetdone
  .gt_txd(gt_txd), // output [31 : 0] gt_txd
  .gt_txc(gt_txc), // output [7 : 0] gt_txc
  .gt_rxd(gt_rxd), // input [31 : 0] gt_rxd
  .gt_rxc(gt_rxc), // input [7 : 0] gt_rxc
  .gt_slip(gt_slip), // output gt_slip
  .drp_req(drp_req), // output drp_req
  .drp_gnt(drp_gnt), // input drp_gnt
  .drp_den(drp_den), // output drp_den
  .drp_dwe(drp_dwe), // output drp_dwe
  .drp_daddr(drp_daddr), // output [15 : 0] drp_daddr
  .drp_di(drp_di), // output [15 : 0] drp_di
  .drp_drdy(drp_drdy), // input drp_drdy
  .drp_drpdo(drp_drpdo), // input [15 : 0] drp_drpdo
  .signal_detect(signal_detect), // input signal_detect
  .tx_fault(tx_fault), // input tx_fault
  .tx_disable(tx_disable), // output tx_disable
  .tx_prbs31_en(tx_prbs31_en), // output tx_prbs31_en
  .rx_prbs31_en(rx_prbs31_en), // output rx_prbs31_en
  .clear_rx_prbs_err_count(clear_rx_prbs_err_count), // output clear_rx_prbs_err_count
  .loopback_ctrl(loopback_ctrl) // output [2 : 0] loopback_ctrl
);
// INST_TAG_END ------ End INSTANTIATION Template ---------

// You must compile the wrapper file ten_gig_eth_pcs_pma.v when simulating
// the core, ten_gig_eth_pcs_pma. When compiling the wrapper file, be sure to
// reference the XilinxCoreLib Verilog simulation library. For detailed
// instructions, please refer to the "CORE Generator Help".

