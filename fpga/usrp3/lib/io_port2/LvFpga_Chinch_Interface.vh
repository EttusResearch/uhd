/*
-------------------------------------------------------------------------------
--
-- File: LvFpga_Chinch_Interface.vh
-- Author: Ashish Chaudhari
-- Original Project: EttusUsrpB250Top
-- Date: 1 Oct 2013
--
-------------------------------------------------------------------------------
-- (c) 2013 Copyright National Instruments Corporation
-- All Rights Reserved
-- National Instruments Internal Information
-------------------------------------------------------------------------------
*/

//Physical link width for the IoPort2 interface to the STC3
`define LVFPGA_IFACE_LINK_WIDTH         16

//DMA Related Constants
`define LVFPGA_IFACE_DMA_CHAN_WIDTH     64	//DMA data bus width
`define LVFPGA_IFACE_DMA_SIZE_WIDTH     11	//DMA FIFO fullness count width
`define LVFPGA_IFACE_NUM_RX_DMA_CNT     6	//Number of RX DMA channels
`define LVFPGA_IFACE_NUM_TX_DMA_CNT     6	//Number of TX DMA channels
`define LVFPGA_IFACE_RX_DMA_INDEX       0	//Index for the first RX DMA channel
`define LVFPGA_IFACE_TX_DMA_INDEX       6	//Index for the first TX DMA channel

//User register port constants
`define LVFPGA_IFACE_UREG_ADDR_WIDTH    20	//Address width
`define LVFPGA_IFACE_UREG_DATA_WIDTH    32	//Payload width
`define LVFPGA_IFACE_UREG_SIZE_WIDTH    2	//Transaction size width

//Chinch register port constants
`define LVFPGA_IFACE_CREG_ADDR_WIDTH    32	//Address width
`define LVFPGA_IFACE_CREG_DATA_WIDTH    64	//Payload width
`define LVFPGA_IFACE_CREG_SIZE_WIDTH    2	//Transaction size width