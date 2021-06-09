//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_eth_dma
//
// Description: Wrapper for the Xilinx AXI DMA
//

module axi_eth_dma (
  // All interfaces on s_axi_eth_dma.clk domain
  AxiStreamIf.slave  e2c,            // from enet mac
  AxiStreamIf.master c2e,            // to enet mac
  AxiLiteIf.slave    s_axi_eth_dma,  // Register interface
  AxiIf.master       axi_hp,         // DMA interface to host memory
  output logic       eth_rx_irq,
  output logic       eth_tx_irq

);

  `include "../../../../lib/axi4lite_sv/axi_lite.vh"
  `include "../../../../lib/axi4s_sv/axi4s.vh"
  `include "../../../../lib/axi4_sv/axi.vh"

  //  _v versions have no procedural assignment so they can drive a port.
  AxiLiteIf_v #(32,10)
    s_axi_eth_dma_v(.clk(s_axi_eth_dma.clk), .rst(s_axi_eth_dma.rst));
  AxiIf_v #(128,49)
    axi_hp_v(.clk(s_axi_eth_dma.clk), .rst(s_axi_eth_dma.rst));

  AxiStreamIf #(.DATA_WIDTH(c2e.DATA_WIDTH), .USER_WIDTH(c2e.USER_WIDTH), .TUSER(0))
    c2e_v    (c2e.clk, c2e.rst);
  AxiStreamIf #(.DATA_WIDTH(e2c.DATA_WIDTH), .USER_WIDTH(e2c.USER_WIDTH), .TUSER(0))
    e2c_v    (e2c.clk, e2c.rst);

  always_comb begin
    //     O = I;   (O              , I            )
    `AXI4LITE_ASSIGN(s_axi_eth_dma_v, s_axi_eth_dma)
    // Overriding assignments to mask off upper address bits
    s_axi_eth_dma_v.araddr = 0;
    s_axi_eth_dma_v.araddr[9:0] = s_axi_eth_dma.araddr[9:0];
    s_axi_eth_dma_v.awaddr = 0;
    s_axi_eth_dma_v.awaddr[9:0] = s_axi_eth_dma.awaddr[9:0];
  end
  always_comb begin `AXI4S_ASSIGN(e2c_v, e2c) end
  always_comb begin `AXI4S_ASSIGN(c2e, c2e_v) end
  always_comb begin `AXI4_ASSIGN(axi_hp, axi_hp_v) end

  axi_eth_dma_bd axi_eth_dma_bd_i (
    .clk40      (s_axi_eth_dma.clk),
    .clk40_rstn (!s_axi_eth_dma.rst),
    
    .c2e_tdata  (c2e_v.tdata),
    .c2e_tkeep  (c2e_v.tkeep),
    .c2e_tlast  (c2e_v.tlast),
    .c2e_tready (c2e_v.tready),
    .c2e_tvalid (c2e_v.tvalid),
    
    .e2c_tdata  (e2c_v.tdata),
    .e2c_tkeep  (e2c_v.tkeep),
    .e2c_tlast  (e2c_v.tlast),
    .e2c_tready (e2c_v.tready),
    .e2c_tvalid (e2c_v.tvalid),
    
    `AXI4LITE_PORT_ASSIGN_NRS(axi_eth_dma, s_axi_eth_dma_v)
    `AXI4_PORT_ASSIGN_NR(axi_hp, axi_hp_v)
    
    .eth_tx_irq (eth_tx_irq),
    .eth_rx_irq (eth_rx_irq)
  );

endmodule
