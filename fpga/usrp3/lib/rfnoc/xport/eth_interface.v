//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Adapts from internal VITA to ethernet packets.  Also handles CPU and ethernet crossover interfaces.

module eth_interface #(
  parameter [15:0] PROTOVER    = {8'd1, 8'd0},
  parameter        MTU         = 10,
  parameter        NODE_INST   = 0,
  parameter        RT_TBL_SIZE = 6,
  parameter        REG_AWIDTH  = 14,
  parameter        BASE        = 0
) (
  input         clk,
  input         reset,
  input [15:0]  device_id,

  // Register port: Write port (domain: clk)
  input        reg_wr_req,
  input [REG_AWIDTH-1:0] reg_wr_addr,
  input [31:0] reg_wr_data,

  // Register port: Read port (domain: clk)
  input         reg_rd_req,
  input  [REG_AWIDTH-1:0] reg_rd_addr,
  output reg       reg_rd_resp,
  output reg [31:0] reg_rd_data,

  // Status ports (domain: clk)
  output [47:0] my_mac,
  output [31:0] my_ip,
  output [15:0] my_udp_port,

  // Ethernet ports
  output [63:0] eth_tx_tdata,
  output [3:0]  eth_tx_tuser,
  output        eth_tx_tlast,
  output        eth_tx_tvalid,
  input         eth_tx_tready,
  input [63:0]  eth_rx_tdata,
  input [3:0]   eth_rx_tuser,
  input         eth_rx_tlast,
  input         eth_rx_tvalid,
  output        eth_rx_tready,

  // Vita router interface
  output [63:0] e2v_tdata,
  output        e2v_tlast,
  output        e2v_tvalid,
  input         e2v_tready,
  input [63:0]  v2e_tdata,
  input         v2e_tlast,
  input         v2e_tvalid,
  output        v2e_tready,

  // CPU
  output [63:0] e2c_tdata,
  output [3:0]  e2c_tuser,
  output        e2c_tlast,
  output        e2c_tvalid,
  input         e2c_tready,
  input [63:0]  c2e_tdata,
  input [3:0]   c2e_tuser,
  input         c2e_tlast,
  input         c2e_tvalid,
  output        c2e_tready
);

  localparam [47:0] DEFAULT_MAC_ADDR  = {8'h00, 8'h80, 8'h2f, 8'h16, 8'hc5, 8'h2f};
  localparam [31:0] DEFAULT_IP_ADDR   = {8'd192, 8'd168, 8'd10, 8'd2};
  localparam [31:0] DEFAULT_UDP_PORT  = 16'd49153;

  //---------------------------------------------------------
  // Registers
  //---------------------------------------------------------

  `include "../xport_sv/eth_regs.vh"

  // MAC address for the dispatcher module.
  // This value is used to determine if the packet is meant
  // for this device should be consumed
  // IP address for the dispatcher module.
  // This value is used to determine if the packet is addressed
  // to this device
  // This module supports two destination ports
  reg [47:0]      mac_reg;
  reg [31:0]      ip_reg;
  reg [15:0]      udp_port;
  reg [47:0]      bridge_mac_reg;
  reg [31:0]      bridge_ip_reg;
  reg [15:0]      bridge_udp_port;
  reg             bridge_en;

  assign my_mac       = bridge_en ? bridge_mac_reg : mac_reg;
  assign my_ip        = bridge_en ? bridge_ip_reg : ip_reg;
  assign my_udp_port  = bridge_en ? bridge_udp_port : udp_port;

  always @(posedge clk) begin
    if (reset) begin
      mac_reg         <= DEFAULT_MAC_ADDR;
      ip_reg          <= DEFAULT_IP_ADDR;
      udp_port        <= DEFAULT_UDP_PORT;
      bridge_en       <= 1'b0;
      bridge_mac_reg  <= DEFAULT_MAC_ADDR;
      bridge_ip_reg   <= DEFAULT_IP_ADDR;
      bridge_udp_port <= DEFAULT_UDP_PORT;
    end
    else begin
      if (reg_wr_req)
        case (reg_wr_addr)

        REG_MAC_LSB:
          mac_reg[31:0]         <= reg_wr_data;

        REG_MAC_MSB:
          mac_reg[47:32]        <= reg_wr_data[15:0];

        REG_IP:
          ip_reg                <= reg_wr_data;

        REG_UDP:
          udp_port              <= reg_wr_data[15:0];

        REG_BRIDGE_MAC_LSB:
          bridge_mac_reg[31:0]  <= reg_wr_data;

        REG_BRIDGE_MAC_MSB:
          bridge_mac_reg[47:32] <= reg_wr_data[15:0];

        REG_BRIDGE_IP:
          bridge_ip_reg         <= reg_wr_data;

        REG_BRIDGE_UDP:
          bridge_udp_port       <= reg_wr_data[15:0];

        REG_BRIDGE_ENABLE:
          bridge_en             <= reg_wr_data[0];
        endcase
    end
  end

  always @ (posedge clk) begin
    // No reset handling required for readback
    if (reg_rd_req) begin
      // Assert read response one cycle after read request
      reg_rd_resp <= 1'b1;
      case (reg_rd_addr)
        REG_MAC_LSB:
          reg_rd_data <= mac_reg[31:0];

        REG_MAC_MSB:
          reg_rd_data <= {16'b0,mac_reg[47:32]};

        REG_IP:
          reg_rd_data <= ip_reg;

        REG_UDP:
          reg_rd_data <= {16'b0, udp_port};

        REG_BRIDGE_MAC_LSB:
          reg_rd_data <= bridge_mac_reg[31:0];

        REG_BRIDGE_MAC_MSB:
          reg_rd_data <= {16'b0,bridge_mac_reg[47:32]};

        REG_BRIDGE_IP:
          reg_rd_data <= bridge_ip_reg;

        REG_BRIDGE_UDP:
          reg_rd_data <= {16'b0, bridge_udp_port};

        REG_BRIDGE_ENABLE:
          reg_rd_data <= {31'b0,bridge_en};

        REG_XPORT_COMPAT:
          // Return compat of 0 to indicate this is the old transport adapter
          reg_rd_data <= 32'b0;

        REG_XPORT_INFO:
          // This TA has no advanced capabilities (e.g., raw UDP)
          reg_rd_data <= 32'b0;

        REG_XPORT_NODE_INST:
          reg_rd_data <= NODE_INST;

        default:
          reg_rd_resp <= 1'b0;
      endcase
    end
    // Deassert read response after one clock cycle
    if (reg_rd_resp) begin
      reg_rd_resp <= 1'b0;
    end
  end

  // In AXI Stream, tkeep is the byte qualifier that indicates
  // whether the content of the associated byte
  // of TDATA is processed as part of the data stream.
  // tuser as used in eth_switch is the number of valid bytes

  eth_ipv4_chdr64_adapter #(
    .PROTOVER        (PROTOVER),
    .MTU             (MTU),
    .CPU_FIFO_SIZE   (MTU),
    .RT_TBL_SIZE     (RT_TBL_SIZE),
    .NODE_INST       (NODE_INST),
    .DROP_UNKNOWN_MAC(0),
    .IS_CPU_ARM      (1)
  ) eth_adapter_i (
    .clk             (clk          ),
    .rst             (reset        ),
    .device_id       (device_id    ),
    .s_mac_tdata     (eth_rx_tdata ),
    .s_mac_tuser     (eth_rx_tuser ),
    .s_mac_tlast     (eth_rx_tlast ),
    .s_mac_tvalid    (eth_rx_tvalid),
    .s_mac_tready    (eth_rx_tready),
    .m_mac_tdata     (eth_tx_tdata ),
    .m_mac_tuser     (eth_tx_tuser ),
    .m_mac_tlast     (eth_tx_tlast ),
    .m_mac_tvalid    (eth_tx_tvalid),
    .m_mac_tready    (eth_tx_tready),
    .s_chdr_tdata    (v2e_tdata    ),
    .s_chdr_tlast    (v2e_tlast    ),
    .s_chdr_tvalid   (v2e_tvalid   ),
    .s_chdr_tready   (v2e_tready   ),
    .m_chdr_tdata    (e2v_tdata    ),
    .m_chdr_tlast    (e2v_tlast    ),
    .m_chdr_tvalid   (e2v_tvalid   ),
    .m_chdr_tready   (e2v_tready   ),
    .s_cpu_tdata     (c2e_tdata    ),
    .s_cpu_tuser     (c2e_tuser    ),
    .s_cpu_tlast     (c2e_tlast    ),
    .s_cpu_tvalid    (c2e_tvalid   ),
    .s_cpu_tready    (c2e_tready   ),
    .m_cpu_tdata     (e2c_tdata    ),
    .m_cpu_tuser     (e2c_tuser    ),
    .m_cpu_tlast     (e2c_tlast    ),
    .m_cpu_tvalid    (e2c_tvalid   ),
    .m_cpu_tready    (e2c_tready   ),
    .my_eth_addr     (my_mac       ),
    .my_ipv4_addr    (my_ip        ),
    .my_udp_chdr_port(my_udp_port  )
  );


endmodule // eth_interface
