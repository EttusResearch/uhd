//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_interconnect_dma
//
// Description: Wrapper for the Xilinx AXI interconnect block
//

module axi_interconnect_dma (
  AxiIf.slave  s_axi_hp_dma [3:0], // Incoming AXI from DMA engines
  AxiIf.master m_axi_hp            // Outgoing AXI to memory
);

  // AxiIf_v has no procedural assignments so it can be driven by a port.
  `include "../../../../lib/axi4_sv/axi.vh"
  AxiIf_v #(128,49)
    m_axi_hp_v(.clk(m_axi_hp.clk), .rst(m_axi_hp.rst));
  AxiIf_v #(128,49)
    s_axi_hp_dma_v[3:0](.clk(m_axi_hp.clk), .rst(m_axi_hp.rst));

  //                    O = I;  (O         ,        I       )
  always_comb begin `AXI4_ASSIGN(m_axi_hp,          m_axi_hp_v) end
  always_comb begin `AXI4_ASSIGN(s_axi_hp_dma_v[0], s_axi_hp_dma[0]) end
  always_comb begin `AXI4_ASSIGN(s_axi_hp_dma_v[1], s_axi_hp_dma[1]) end
  always_comb begin `AXI4_ASSIGN(s_axi_hp_dma_v[2], s_axi_hp_dma[2]) end
  always_comb begin `AXI4_ASSIGN(s_axi_hp_dma_v[3], s_axi_hp_dma[3]) end

  axi_interconnect_dma_bd axi_interconnect_dma_bd_i (
    `AXI4_PORT_ASSIGN_NR(m_axi_hp,      m_axi_hp_v)
    `AXI4_PORT_ASSIGN_NR(s_axi_hp_dma0, s_axi_hp_dma_v[0])
    `AXI4_PORT_ASSIGN_NR(s_axi_hp_dma1, s_axi_hp_dma_v[1])
    `AXI4_PORT_ASSIGN_NR(s_axi_hp_dma2, s_axi_hp_dma_v[2])
    `AXI4_PORT_ASSIGN_NR(s_axi_hp_dma3, s_axi_hp_dma_v[3])
    .clk40      (m_axi_hp.clk),
    .clk40_rstn (!m_axi_hp.rst)
  );

endmodule
