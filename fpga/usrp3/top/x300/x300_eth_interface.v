//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Adapts from internal VITA to ethernet packets.  Also handles ZPU and ethernet crossover interfaces.

module x300_eth_interface #(
  parameter [15:0] PROTOVER    = {8'd1, 8'd0},
  parameter        MTU         = 10,
  parameter        NODE_INST   = 0,
  parameter        RT_TBL_SIZE = 6,
  parameter        BASE        = 0
) (
  input clk, input reset,
  input [15:0] device_id,
  input set_stb, input [7:0] set_addr, input [31:0] set_data,
  // Eth ports
  output [63:0] eth_tx_tdata, output [3:0] eth_tx_tuser, output eth_tx_tlast, output eth_tx_tvalid, input eth_tx_tready,
  input [63:0] eth_rx_tdata, input [3:0] eth_rx_tuser, input eth_rx_tlast, input eth_rx_tvalid, output eth_rx_tready,
  // Vita router interface
  output [63:0] e2v_tdata, output e2v_tlast, output e2v_tvalid, input e2v_tready,
  input [63:0] v2e_tdata, input v2e_tlast, input v2e_tvalid, output v2e_tready,
  // ZPU
  output [63:0] e2z_tdata, output [3:0] e2z_tuser, output e2z_tlast, output e2z_tvalid, input e2z_tready,
  input [63:0] z2e_tdata, input [3:0] z2e_tuser, input z2e_tlast, input z2e_tvalid, output z2e_tready
);

  // UNUSED: BASE to BASE+7
  localparam MY_ETH_ADDR_LO_REG = BASE + 8;
  localparam MY_ETH_ADDR_HI_REG = BASE + 9;
  localparam MY_IP_ADDR_REG     = BASE + 10;
  localparam MY_UDP_PORT_REG    = BASE + 11;
  // UNUSED: BASE+12 to BASE+15

  wire [47:0] my_eth_addr;
  wire [31:0] my_ipv4_addr;
  wire [15:0] my_udp_chdr_port;

  // MAC address for the dispatcher module.
  // This value is used to determine if the packet is meant 
  // for this device should be consumed
  setting_reg #(.my_addr(MY_ETH_ADDR_LO_REG), .awidth(8), .width(32)) sr_my_mac_lsb
    (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
     .in(set_data),.out(my_eth_addr[31:0]),.changed());
  setting_reg #(.my_addr(MY_ETH_ADDR_HI_REG), .awidth(8), .width(16)) sr_my_mac_msb
    (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
     .in(set_data),.out(my_eth_addr[47:32]),.changed());

  // IP address for the dispatcher module.
  // This value is used to determine if the packet is addressed
  // to this device 
  setting_reg #(.my_addr(MY_IP_ADDR_REG), .awidth(8), .width(32)) sr_my_ip
    (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
     .in(set_data),.out(my_ipv4_addr[31:0]),.changed());

  // This module supports one destinatio port
  setting_reg #(.my_addr(MY_UDP_PORT_REG), .awidth(8), .width(16)) sr_udp_port
    (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
     .in(set_data),.out({my_udp_chdr_port[15:0]}),.changed());

  eth_ipv4_chdr64_adapter #(
    .PROTOVER        (PROTOVER),
    .MTU             (MTU),
    .CPU_FIFO_SIZE   (MTU),
    .RT_TBL_SIZE     (RT_TBL_SIZE),
    .NODE_INST       (NODE_INST),
    .DROP_UNKNOWN_MAC(1)
  ) eth_adapter_i (
    .clk             (clk             ),
    .rst             (reset           ),
    .device_id       (device_id       ),
    .s_mac_tdata     (eth_rx_tdata    ),
    .s_mac_tuser     (eth_rx_tuser    ),
    .s_mac_tlast     (eth_rx_tlast    ),
    .s_mac_tvalid    (eth_rx_tvalid   ),
    .s_mac_tready    (eth_rx_tready   ),
    .m_mac_tdata     (eth_tx_tdata    ),
    .m_mac_tuser     (eth_tx_tuser    ),
    .m_mac_tlast     (eth_tx_tlast    ),
    .m_mac_tvalid    (eth_tx_tvalid   ),
    .m_mac_tready    (eth_tx_tready   ),
    .s_chdr_tdata    (v2e_tdata       ),
    .s_chdr_tlast    (v2e_tlast       ),
    .s_chdr_tvalid   (v2e_tvalid      ),
    .s_chdr_tready   (v2e_tready      ),
    .m_chdr_tdata    (e2v_tdata       ),
    .m_chdr_tlast    (e2v_tlast       ),
    .m_chdr_tvalid   (e2v_tvalid      ),
    .m_chdr_tready   (e2v_tready      ),
    .s_cpu_tdata     ({z2e_tdata[7:0],   z2e_tdata[15:8],
                       z2e_tdata[23:16], z2e_tdata[31:24],
                       z2e_tdata[39:32], z2e_tdata[47:40],
                       z2e_tdata[55:48], z2e_tdata[63:56]}),
    .s_cpu_tuser     (z2e_tuser       ),
    .s_cpu_tlast     (z2e_tlast       ),
    .s_cpu_tvalid    (z2e_tvalid      ),
    .s_cpu_tready    (z2e_tready      ),
    .m_cpu_tdata     ({e2z_tdata[7:0],   e2z_tdata[15:8],
                       e2z_tdata[23:16], e2z_tdata[31:24],
                       e2z_tdata[39:32], e2z_tdata[47:40],
                       e2z_tdata[55:48], e2z_tdata[63:56]}),
    .m_cpu_tuser     (e2z_tuser       ),
    .m_cpu_tlast     (e2z_tlast       ),
    .m_cpu_tvalid    (e2z_tvalid      ),
    .m_cpu_tready    (e2z_tready      ),
    .my_eth_addr     (my_eth_addr     ),
    .my_ipv4_addr    (my_ipv4_addr    ),
    .my_udp_chdr_port(my_udp_chdr_port)
  );


endmodule // x300_eth_interface
