//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Adapts from internal CHDR to Ethernet packets. Also handles ZPU and Ethernet
// crossover interfaces.
//

`default_nettype none


module x300_eth_interface #(
  parameter [15:0] PROTOVER         = {8'd1, 8'd0},
  parameter        MTU              = 10,
  parameter        NODE_INST        = 0,
  parameter        RT_TBL_SIZE      = 6,
  parameter        ETH_BASE         = 0,
  parameter        TA_BASE          = 16,
  parameter [ 0:0] EN_RX_RAW_PYLD   = 1,
  parameter [ 0:0] EN_RX_KV_MAP_CFG = 1
) (
  input  wire        clk,
  input  wire        reset,

  // RFNoC Device ID
  input  wire [15:0] device_id,

  // Settings bus
  input  wire        set_stb,
  input  wire [ 7:0] set_addr,
  input  wire [31:0] set_data,
  input  wire [ 7:0] rb_addr,
  output reg  [31:0] rb_data,

  // Eth ports
  output wire [63:0] eth_tx_tdata,
  output wire [ 3:0] eth_tx_tuser,
  output wire        eth_tx_tlast,
  output wire        eth_tx_tvalid,
  input  wire        eth_tx_tready,
  input  wire [63:0] eth_rx_tdata,
  input  wire [ 3:0] eth_rx_tuser,
  input  wire        eth_rx_tlast,
  input  wire        eth_rx_tvalid,
  output wire        eth_rx_tready,

  // CHDR Interface
  output wire [63:0] e2v_tdata,
  output wire        e2v_tlast,
  output wire        e2v_tvalid,
  input  wire        e2v_tready,
  input  wire [63:0] v2e_tdata,
  input  wire        v2e_tlast,
  input  wire        v2e_tvalid,
  output wire        v2e_tready,

  // ZPU
  output wire [63:0] e2z_tdata,
  output wire [ 3:0] e2z_tuser,
  output wire        e2z_tlast,
  output wire        e2z_tvalid,
  input  wire        e2z_tready,
  input  wire [63:0] z2e_tdata,
  input  wire [ 3:0] z2e_tuser,
  input  wire        z2e_tlast,
  input  wire        z2e_tvalid,
  output wire        z2e_tready
);

  //---------------------------------------------------------------------------
  // Ethernet Registers
  //---------------------------------------------------------------------------

  // UNUSED: ETH_BASE to ETH_BASE+7
  localparam MY_ETH_ADDR_LO_REG = ETH_BASE + 8;
  localparam MY_ETH_ADDR_HI_REG = ETH_BASE + 9;
  localparam MY_IP_ADDR_REG     = ETH_BASE + 10;
  localparam MY_UDP_PORT_REG    = ETH_BASE + 11;
  // UNUSED: ETH_BASE+12 to ETH_BASE+15

  wire [47:0] my_eth_addr;
  wire [31:0] my_ipv4_addr;
  wire [15:0] my_udp_chdr_port;

  // MAC address for the dispatcher module.
  // This value is used to determine if the packet is meant for this device.
  setting_reg #(.my_addr(MY_ETH_ADDR_LO_REG), .awidth(8), .width(32)) sr_my_mac_lsb
    (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
     .in(set_data),.out(my_eth_addr[31:0]),.changed());
  setting_reg #(.my_addr(MY_ETH_ADDR_HI_REG), .awidth(8), .width(16)) sr_my_mac_msb
    (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
     .in(set_data),.out(my_eth_addr[47:32]),.changed());

  // IP address for the dispatcher module.
  // This value is used to determine if the packet is addressed
  // to this device.
  setting_reg #(.my_addr(MY_IP_ADDR_REG), .awidth(8), .width(32)) sr_my_ip
    (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
     .in(set_data),.out(my_ipv4_addr[31:0]),.changed());

  // This module supports one destination port
  setting_reg #(.my_addr(MY_UDP_PORT_REG), .awidth(8), .width(16)) sr_udp_port
    (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
     .in(set_data),.out({my_udp_chdr_port[15:0]}),.changed());

  //---------------------------------------------------------------------------
  // Transport Adapter Registers
  //---------------------------------------------------------------------------

  // See eth_regs for a description of the bit fields.
  localparam [7:0] REG_XPORT_COMPAT    = TA_BASE + 0;
  localparam [7:0] REG_XPORT_INFO      = TA_BASE + 1;
  localparam [7:0] REG_XPORT_NODE_INST = TA_BASE + 2;
  localparam [7:0] REG_XPORT_KV_MAC_LO = TA_BASE + 3;
  localparam [7:0] REG_XPORT_KV_MAC_HI = TA_BASE + 4;
  localparam [7:0] REG_XPORT_KV_IP     = TA_BASE + 5;
  localparam [7:0] REG_XPORT_KV_UDP    = TA_BASE + 6;
  localparam [7:0] REG_XPORT_KV_CFG    = TA_BASE + 7;
  // Note: TA_BASE + 8 is a virtual register used for ARP (see x300 firmware)
  localparam [7:0] REG_IPV4_ARP        = TA_BASE + 8;

  localparam [7:0] COMPAT_MAJOR = 8'd1;
  localparam [7:0] COMPAT_MINOR = 8'd0;

  // KV map configuration registers. Make them default to 0 so that the extra
  // logic for this configuration interface gets optimized out when not used.
  reg         kv_stb      = 0;
  reg  [47:0] kv_mac_addr = 0;
  reg  [31:0] kv_ip_addr  = 0;
  reg  [15:0] kv_udp_port = 0;
  reg  [15:0] kv_dst_epid = 0;
  reg         kv_raw_udp  = 0;
  wire        kv_busy;

  if (EN_RX_KV_MAP_CFG) begin : gen_write_regs
    // Implement register writes (settings regs)
    always @(posedge clk) begin : ta_reg_write
      if (reset) begin
        kv_stb <= 0;
      end else begin
        kv_stb <= 0;
        if (set_stb) begin
          case (set_addr)
          REG_XPORT_KV_MAC_LO:
            kv_mac_addr[31:0]  <= set_data;
          REG_XPORT_KV_MAC_HI:
            kv_mac_addr[47:32] <= set_data[15:0];
          REG_XPORT_KV_IP:
            kv_ip_addr         <= set_data[31:0];
          REG_XPORT_KV_UDP:
            kv_udp_port        <= set_data[15:0];
          REG_XPORT_KV_CFG: begin
            kv_dst_epid        <= set_data[15:0];
            kv_raw_udp         <= set_data[16];
            kv_stb             <= 1;
          end
          endcase
        end
      end
    end // ta_reg_write
  end // gen_write_regs


  // Implement register reads (readback regs)
  always @(*) begin : ta_reg_read
    rb_data = 32'd0;

    case (rb_addr)
      REG_XPORT_COMPAT:
        rb_data[15:0] = { COMPAT_MAJOR, COMPAT_MINOR };

      REG_XPORT_INFO:
        rb_data[1:0] = { EN_RX_RAW_PYLD, EN_RX_KV_MAP_CFG };

      REG_XPORT_NODE_INST:
        rb_data = NODE_INST;

      REG_XPORT_KV_CFG:
        if (EN_RX_KV_MAP_CFG) begin
          rb_data[31] = kv_busy;
        end

      default:
        rb_data = 32'd0;
     endcase
  end

  //---------------------------------------------------------------------------
  // Transport Adapter
  //---------------------------------------------------------------------------

  if (EN_RX_KV_MAP_CFG || EN_RX_RAW_PYLD) begin : gen_adv_ta
    // Incoming MTU parameter is in CHDR_W words but eht_ipv4_chdr_adapter
    // expects it in bytes.
    localparam CPU_FIFO_SIZE  = $clog2(8*(2**MTU));
    localparam CHDR_FIFO_SIZE = $clog2(8*(2**MTU));

    eth_ipv4_chdr_adapter_wrapper #(
      .PROTOVER        (PROTOVER      ),
      .CPU_FIFO_SIZE   (CPU_FIFO_SIZE ),
      .CHDR_FIFO_SIZE  (CHDR_FIFO_SIZE),
      .RT_TBL_SIZE     (RT_TBL_SIZE   ),
      .NODE_INST       (NODE_INST     ),
      .DROP_UNKNOWN_MAC(1             ),
      .PREAMBLE_BYTES  (6             ),
      .CPU_PREAMBLE    (1             ),
      .SYNC            (1             ),
      .EN_RX_RAW_PYLD  (EN_RX_RAW_PYLD)
    ) eth_ipv4_chdr_adapter_wrapper_i (
      .bus_clk         (clk               ),
      .bus_rst         (reset             ),
      .device_id       (device_id         ),
      .my_mac          (my_eth_addr       ),
      .my_ip           (my_ipv4_addr      ),
      .my_udp_chdr_port(my_udp_chdr_port  ),
      .my_pause_set    (16'd0             ),
      .my_pause_clear  (16'd0             ),
      .kv_stb          (kv_stb            ),
      .kv_busy         (kv_busy           ),
      .kv_mac_addr     (kv_mac_addr       ),
      .kv_ip_addr      (kv_ip_addr        ),
      .kv_udp_port     (kv_udp_port       ),
      .kv_dst_epid     (kv_dst_epid       ),
      .kv_raw_udp      (kv_raw_udp        ),
      .eth_clk         (clk               ),
      .eth_rst         (reset             ),
      .eth_pause_req   (                  ),
      .eth_tx_tdata    (eth_tx_tdata      ),
      .eth_tx_tuser    (eth_tx_tuser      ),
      .eth_tx_tkeep    (                  ),
      .eth_tx_tlast    (eth_tx_tlast      ),
      .eth_tx_tvalid   (eth_tx_tvalid     ),
      .eth_tx_tready   (eth_tx_tready     ),
      .eth_rx_tdata    (eth_rx_tdata      ),
      .eth_rx_tuser    (eth_rx_tuser      ),
      .eth_rx_tlast    (eth_rx_tlast      ),
      .eth_rx_tvalid   (eth_rx_tvalid     ),
      .eth_rx_tready   (eth_rx_tready     ),
      .e2v_tdata       (e2v_tdata         ),
      .e2v_tlast       (e2v_tlast         ),
      .e2v_tvalid      (e2v_tvalid        ),
      .e2v_tready      (e2v_tready        ),
      .v2e_tdata       (v2e_tdata         ),
      .v2e_tlast       (v2e_tlast         ),
      .v2e_tvalid      (v2e_tvalid        ),
      .v2e_tready      (v2e_tready        ),
      .cpu_clk         (clk               ),
      .cpu_rst         (reset             ),
      .e2c_tdata       ({e2z_tdata[ 7: 0],
                         e2z_tdata[15: 8],
                         e2z_tdata[23:16],
                         e2z_tdata[31:24],
                         e2z_tdata[39:32],
                         e2z_tdata[47:40],
                         e2z_tdata[55:48],
                         e2z_tdata[63:56]}),
      .e2c_tuser       (e2z_tuser         ),
      .e2c_tlast       (e2z_tlast         ),
      .e2c_tvalid      (e2z_tvalid        ),
      .e2c_tready      (e2z_tready        ),
      .c2e_tdata       ({z2e_tdata[ 7: 0],
                         z2e_tdata[15: 8],
                         z2e_tdata[23:16],
                         z2e_tdata[31:24],
                         z2e_tdata[39:32],
                         z2e_tdata[47:40],
                         z2e_tdata[55:48],
                         z2e_tdata[63:56]}),
      .c2e_tuser       (z2e_tuser         ),
      .c2e_tlast       (z2e_tlast         ),
      .c2e_tvalid      (z2e_tvalid        ),
      .c2e_tready      (z2e_tready        )
    );
  end else begin : gen_basic_ta
    eth_ipv4_chdr64_adapter #(
      .PROTOVER        (PROTOVER),
      .MTU             (MTU),
      .CPU_FIFO_SIZE   (MTU),
      .RT_TBL_SIZE     (RT_TBL_SIZE),
      .NODE_INST       (NODE_INST),
      .DROP_UNKNOWN_MAC(1)
    ) eth_ipv4_chdr64_adapter_i (
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
  end

endmodule // x300_eth_interface

`default_nettype wire
