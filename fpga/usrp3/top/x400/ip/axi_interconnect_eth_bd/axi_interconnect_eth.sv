//
// Copyright 2021 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axi_interconnect_eth
//
// Description:
//
//   Wrapper for the Xilinx AXI Lite interconnect block
//

module axi_interconnect_eth (
  // All interfaces on s_axi_eth.clk domain
  AxiLiteIf.master  m_axi_dma[3:0],  // maps to CPU_DMA  +0x00000-0x07FFF
  AxiLiteIf.master  m_axi_misc[3:0], // maps to nixge    +0x08000-0x09FFF
                                     //         UIO      +0x0A000-0x0BFFF
  AxiLiteIf.master  m_axi_mac[3:0],  // maps to 100G Mac +0x0C000-0x0DFFF

  AxiLiteIf.slave   s_axi_eth   // incoming axi_net bus
);


  // AxiLiteIf_v has no procedural assignments so it can be
  // driven by a port.
  `include "../../../../lib/axi4lite_sv/axi_lite.vh"
  AxiLiteIf_v #(32,40)
    m_axi_dma_v[3:0](.clk(s_axi_eth.clk),.rst(s_axi_eth.rst));
  AxiLiteIf_v #(32,40)
    m_axi_misc_v[3:0](.clk(s_axi_eth.clk),.rst(s_axi_eth.rst));
  AxiLiteIf_v #(32,40)
    m_axi_mac_v[3:0](.clk(s_axi_eth.clk),.rst(s_axi_eth.rst));
  AxiLiteIf_v #(32,40)
    s_axi_eth_v(.clk(s_axi_eth.clk),.rst(s_axi_eth.rst));

  //                          O = I;(O         ,I           )
  always_comb begin `AXI4LITE_ASSIGN(m_axi_dma[0],m_axi_dma_v[0]) end
  always_comb begin `AXI4LITE_ASSIGN(m_axi_dma[1],m_axi_dma_v[1]) end
  always_comb begin `AXI4LITE_ASSIGN(m_axi_dma[2],m_axi_dma_v[2]) end
  always_comb begin `AXI4LITE_ASSIGN(m_axi_dma[3],m_axi_dma_v[3]) end
  always_comb begin `AXI4LITE_ASSIGN(m_axi_misc[0],m_axi_misc_v[0]) end
  always_comb begin `AXI4LITE_ASSIGN(m_axi_misc[1],m_axi_misc_v[1]) end
  always_comb begin `AXI4LITE_ASSIGN(m_axi_misc[2],m_axi_misc_v[2]) end
  always_comb begin `AXI4LITE_ASSIGN(m_axi_misc[3],m_axi_misc_v[3]) end
  always_comb begin `AXI4LITE_ASSIGN(m_axi_mac[0],m_axi_mac_v[0]) end
  always_comb begin `AXI4LITE_ASSIGN(m_axi_mac[1],m_axi_mac_v[1]) end
  always_comb begin `AXI4LITE_ASSIGN(m_axi_mac[2],m_axi_mac_v[2]) end
  always_comb begin `AXI4LITE_ASSIGN(m_axi_mac[3],m_axi_mac_v[3]) end
  always_comb begin `AXI4LITE_ASSIGN(s_axi_eth_v,s_axi_eth) end

  axi_interconnect_eth_bd axi_interconnect_eth_bd_i
       (
        `AXI4LITE_PORT_ASSIGN_NR(m_axi_dma0,m_axi_dma_v[0])
        .m_axi_dma0_arprot(),
        .m_axi_dma0_awprot(),
        `AXI4LITE_PORT_ASSIGN_NR(m_axi_dma1,m_axi_dma_v[1])
        .m_axi_dma1_arprot(),
        .m_axi_dma1_awprot(),
        `AXI4LITE_PORT_ASSIGN_NR(m_axi_dma2,m_axi_dma_v[2])
        .m_axi_dma2_arprot(),
        .m_axi_dma2_awprot(),
        `AXI4LITE_PORT_ASSIGN_NR(m_axi_dma3,m_axi_dma_v[3])
        .m_axi_dma3_arprot(),
        .m_axi_dma3_awprot(),

        `AXI4LITE_PORT_ASSIGN_NR(m_axi_mac0,m_axi_mac_v[0])
        .m_axi_mac0_arprot(),
        .m_axi_mac0_awprot(),
        `AXI4LITE_PORT_ASSIGN_NR(m_axi_mac1,m_axi_mac_v[1])
        .m_axi_mac1_arprot(),
        .m_axi_mac1_awprot(),
        `AXI4LITE_PORT_ASSIGN_NR(m_axi_mac2,m_axi_mac_v[2])
        .m_axi_mac2_arprot(),
        .m_axi_mac2_awprot(),
        `AXI4LITE_PORT_ASSIGN_NR(m_axi_mac3,m_axi_mac_v[3])
        .m_axi_mac3_arprot(),
        .m_axi_mac3_awprot(),

        `AXI4LITE_PORT_ASSIGN_NR(m_axi_misc0,m_axi_misc_v[0])
        .m_axi_misc0_arprot(),
        .m_axi_misc0_awprot(),
        `AXI4LITE_PORT_ASSIGN_NR(m_axi_misc1,m_axi_misc_v[1])
        .m_axi_misc1_arprot(),
        .m_axi_misc1_awprot(),
        `AXI4LITE_PORT_ASSIGN_NR(m_axi_misc2,m_axi_misc_v[2])
        .m_axi_misc2_arprot(),
        .m_axi_misc2_awprot(),
        `AXI4LITE_PORT_ASSIGN_NR(m_axi_misc3,m_axi_misc_v[3])
        .m_axi_misc3_arprot(),
        .m_axi_misc3_awprot(),

        `AXI4LITE_PORT_ASSIGN_NR(s_axi_eth,s_axi_eth_v)
        .s_axi_eth_arprot(3'b0),
        .s_axi_eth_awprot(3'b0),
        .clk40(s_axi_eth.clk),
        .clk40_rstn(!s_axi_eth.rst));

endmodule
