/*
-------------------------------------------------------------------------------
--
-- File: LvFpga_Chinch_Interface.vhd
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

`include "LvFpga_Chinch_Interface.vh"

module  LvFpga_Chinch_Interface
(
    input   aIoResetIn_n,
    output  bBusReset,

    input   BusClk,
    input   Rio40Clk,
    input   IDelayRefClk,
    input   aRioClkPllLocked,
    output  aRioClkPllReset,

    output  aIoReadyOut,
    input   aIoReadyIn,
    output  aIoPort2Restart,

    input                                   IoRxClock,
    input                                   IoRxClock_n,
    input [`LVFPGA_IFACE_LINK_WIDTH-1:0]    irIoRxData,
    input [`LVFPGA_IFACE_LINK_WIDTH-1:0]    irIoRxData_n,
    input                                   irIoRxHeader,
    input                                   irIoRxHeader_n,
        
    output                                  IoTxClock,
    output                                  IoTxClock_n,
    output [`LVFPGA_IFACE_LINK_WIDTH-1:0]   itIoTxData,
    output [`LVFPGA_IFACE_LINK_WIDTH-1:0]   itIoTxData_n,
    output                                  itIoTxHeader,
    output                                  itIoTxHeader_n,
    
    input [(`LVFPGA_IFACE_NUM_RX_DMA_CNT*`LVFPGA_IFACE_DMA_CHAN_WIDTH)-1:0]  bDmaRxData,
    input [`LVFPGA_IFACE_NUM_RX_DMA_CNT-1:0]                                 bDmaRxValid,
    output [`LVFPGA_IFACE_NUM_RX_DMA_CNT-1:0]                                bDmaRxReady,
    output [`LVFPGA_IFACE_NUM_RX_DMA_CNT-1:0]                                bDmaRxEnabled,
    output [(`LVFPGA_IFACE_NUM_RX_DMA_CNT*`LVFPGA_IFACE_DMA_SIZE_WIDTH)-1:0] bDmaRxFifoFreeCnt,
    
    output [(`LVFPGA_IFACE_NUM_TX_DMA_CNT*`LVFPGA_IFACE_DMA_CHAN_WIDTH)-1:0] bDmaTxData,
    output [`LVFPGA_IFACE_NUM_TX_DMA_CNT-1:0]                                bDmaTxValid,
    input [`LVFPGA_IFACE_NUM_TX_DMA_CNT-1:0]                                 bDmaTxReady,
    output [`LVFPGA_IFACE_NUM_TX_DMA_CNT-1:0]                                bDmaTxEnabled,
    output [(`LVFPGA_IFACE_NUM_TX_DMA_CNT*`LVFPGA_IFACE_DMA_SIZE_WIDTH)-1:0] bDmaTxFifoFullCnt,
    
    output                                      bUserRegPortInWt,
    output                                      bUserRegPortInRd,
    output [`LVFPGA_IFACE_UREG_ADDR_WIDTH-1:0]  bUserRegPortInAddr,
    output [`LVFPGA_IFACE_UREG_DATA_WIDTH-1:0]  bUserRegPortInData,
    output [`LVFPGA_IFACE_UREG_SIZE_WIDTH-1:0]  bUserRegPortInSize,
    input [`LVFPGA_IFACE_UREG_DATA_WIDTH-1:0]   bUserRegPortOutData,
    input                                       bUserRegPortOutDataValid,
    input                                       bUserRegPortOutReady,
    
    input                                       bChinchRegPortOutWt,
    input                                       bChinchRegPortOutRd,
    input [`LVFPGA_IFACE_CREG_ADDR_WIDTH-1:0]   bChinchRegPortOutAddr,
    input [`LVFPGA_IFACE_CREG_DATA_WIDTH-1:0]   bChinchRegPortOutData,
    input [`LVFPGA_IFACE_CREG_SIZE_WIDTH-1:0]   bChinchRegPortOutSize,
    output [`LVFPGA_IFACE_CREG_DATA_WIDTH-1:0]  bChinchRegPortInData,
    output                                      bChinchRegPortInDataValid,
    output                                      bChinchRegPortInReady,
    
    output  aIrq
) /* synthesis syn_black_box syn_noprune=1 */;
// This module serves as an API wrapper for LvFpga_Chinch_Interface.ngc and we don't want
// the tool to accidentally prune out it contents. Hence the syn_black_box syn_noprune=1 directives.
endmodule
