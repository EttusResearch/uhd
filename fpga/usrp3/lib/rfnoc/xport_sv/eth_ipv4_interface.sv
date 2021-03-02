//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
// Module: eth_ipv4_interface
//
// Description:
//   Adapts from internal CHDR to UDP/IPV4 Ethernet packets.
//   Packets not specifically addressed to CHDR are routed
//   to the CPU
//
// Parameters:
//   - PROTOVER: RFNoC protocol version {8'd<major>, 8'd<minor>}
//   - CPU_FIFO_SIZE: Log2 of the FIFO depth (in bytes) for the CPU egress path
//   - CHDR_FIFO_SIZE: Log2 of the FIFO depth (in bytes) for the CHDR egress path
//   - RT_TBL_SIZE: Log2 of the depth of the return-address routing table
//   - NODE_INST: The node type to return for a node-info discovery
//   - DROP_UNKNOWN_MAC: Drop packets not addressed to us?
//   - DROP_MIN_PACKET: Drop packets smaller than 64 bytes?
//   - PREAMBLE_BYTES: Number of bytes of Preamble expected
//   - ADD_SOF: Add a SOF indication into the tuser field of e2c
//   - SYNC: Set if the CPU clock domain (c2e, e2c) is not the same as the
//           Ethernet clock domain (eth_rx, eth_tx).
//   - ENET_W: Width of the link to the Ethernet MAC
//   - CPU_W: Width of the CPU interface
//   - CHDR_W: Width of the CHDR interface
//

module eth_ipv4_interface #(
  logic [15:0] PROTOVER         = {8'd1, 8'd0},
  int          CPU_FIFO_SIZE    = $clog2(8*1024),
  int          CHDR_FIFO_SIZE   = $clog2(8*1024),
  int          NODE_INST        = 0,
  int          RT_TBL_SIZE      = 6,
  int          REG_AWIDTH       = 14,
  int          BASE             = 0,
  bit          DROP_UNKNOWN_MAC = 0,
  bit          DROP_MIN_PACKET  = 0,
  int          PREAMBLE_BYTES   = 6,
  bit          ADD_SOF          = 1,
  bit          SYNC             = 0,
  bit          PAUSE_EN         = 0,
  int          ENET_W           = 64,
  int          CPU_W            = 64,
  int          CHDR_W           = 64
) (
  input logic        bus_clk,
  input logic        bus_rst,
  input logic [15:0] device_id,

  // Register port: Write port (domain: bus_clk)
  input logic                  reg_wr_req,
  input logic [REG_AWIDTH-1:0] reg_wr_addr,
  input logic [31:0]           reg_wr_data,

  // Register port: Read port (domain: bus_clk)
  input  logic                  reg_rd_req,
  input  logic [REG_AWIDTH-1:0] reg_rd_addr,
  output logic                  reg_rd_resp,
  output logic     [31:0]       reg_rd_data,

  // Status ports (domain: bus_clk)
  output logic [47:0] my_mac,
  output logic [31:0] my_ip,
  output logic [15:0] my_udp_chdr_port,

  // Ethernet MAC (domain: eth_rx.clk)
  output logic       eth_pause_req,
  AxiStreamIf.master eth_tx, // tUser = {1'b0,trailing bytes};
  AxiStreamIf.slave  eth_rx, // tUser = {error,trailing bytes};

  // CHDR router interface (domain: eth_rx.clk)
  AxiStreamIf.master e2v, // tUser = {*not used*};
  AxiStreamIf.slave  v2e, // tUser = {*not used*};

  // CPU DMA
  // (domain: e2c.clk if SYNC=0, else eth_rx.clk)
  AxiStreamIf.master e2c, // tUser = {sof,trailing bytes};
  // (domain: c2e.clk if SYNC=0, else eth_rx.clk)
  AxiStreamIf.slave  c2e  // tUser = {1'b0,trailing bytes};

 );

  localparam [47:0] DEFAULT_MAC_ADDR  = {8'h00, 8'h80, 8'h2f, 8'h16, 8'hc5, 8'h2f};
  localparam [31:0] DEFAULT_IP_ADDR   = {8'd192, 8'd168, 8'd10, 8'd2};
  localparam [31:0] DEFAULT_UDP_PORT  = 16'd49153;
  localparam [15:0] DEFAULT_PAUSE_SET   = 16'd00040;
  localparam [15:0] DEFAULT_PAUSE_CLEAR = 16'd00020;

  //---------------------------------------------------------
  // Registers
  //---------------------------------------------------------
  // Include for register offsets
  `include "eth_regs.vh"
  // Allocate one full page for M
  // mac_reg: MAC address for the dispatcher module. This value is used to
  // determine if the packet is meant for this device and should be consumed.
  //
  // ip_reg: IP address for the dispatcher module. This value is used to
  // determine if the packet is addressed to this device
  //
  // This module supports two destination ports.
  logic [47:0] mac_reg         = DEFAULT_MAC_ADDR;
  logic [31:0] ip_reg          = DEFAULT_IP_ADDR;
  logic [15:0] udp_port        = DEFAULT_UDP_PORT;
  logic [47:0] bridge_mac_reg  = DEFAULT_MAC_ADDR;
  logic [31:0] bridge_ip_reg   = DEFAULT_IP_ADDR;
  logic [15:0] bridge_udp_port = DEFAULT_UDP_PORT;
  logic        bridge_en;
  logic        cpu_dropped;
  logic        chdr_dropped;
  logic [31:0] chdr_drop_count = 0;
  logic [31:0] cpu_drop_count  = 0;
  logic [15:0] my_pause_set    = DEFAULT_PAUSE_SET;
  logic [15:0] my_pause_clear  = DEFAULT_PAUSE_CLEAR;

  always_comb begin : bridge_mux
    my_mac            = bridge_en ? bridge_mac_reg : mac_reg;
    my_ip             = bridge_en ? bridge_ip_reg : ip_reg;
    my_udp_chdr_port  = bridge_en ? bridge_udp_port : udp_port;
  end

  always_ff @(posedge bus_clk) begin : reg_wr_ff
    if (bus_rst) begin
      mac_reg         <= DEFAULT_MAC_ADDR;
      ip_reg          <= DEFAULT_IP_ADDR;
      udp_port        <= DEFAULT_UDP_PORT;
      bridge_en       <= 1'b0;
      bridge_mac_reg  <= DEFAULT_MAC_ADDR;
      bridge_ip_reg   <= DEFAULT_IP_ADDR;
      bridge_udp_port <= DEFAULT_UDP_PORT;
      my_pause_set    <= DEFAULT_PAUSE_SET;
      my_pause_clear  <= DEFAULT_PAUSE_CLEAR;
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

        REG_PAUSE:
          begin
            if (PAUSE_EN) begin
              my_pause_set        <= reg_wr_data[15:0];
              my_pause_clear      <= reg_wr_data[31:16];
            end
          end
        endcase
    end
  end

  always_ff @ (posedge bus_clk) begin : reg_rd_ff
    if (bus_rst) begin
      reg_rd_resp <= 1'b0;
      reg_rd_data <= 32'd0;
      chdr_drop_count <= 32'd0;
      cpu_drop_count  <= 32'd0;
    end
    else begin
      if (chdr_dropped) begin
        chdr_drop_count <= chdr_drop_count+1;
      end
      if (cpu_dropped) begin
        cpu_drop_count <= cpu_drop_count+1;
      end
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
          // Drop counts are used to debug situations
          // Where the incoming data goes faster than
          // chdr can consume it
          REG_CHDR_DROPPED:
            begin
              reg_rd_data <= chdr_drop_count;
              chdr_drop_count <= 0; // clear when read
            end
          REG_CPU_DROPPED:
            begin
              reg_rd_data <= cpu_drop_count;
              cpu_drop_count <= 0; // clear when read
            end

          REG_PAUSE:
            begin
              if (PAUSE_EN) begin
                reg_rd_data[15:0]  <= my_pause_set;
                reg_rd_data[31:16] <= my_pause_clear;
              end
            end
         default:
            reg_rd_resp <= 1'b0;
         endcase
      end
      // Deassert read response after one clock cycle
      if (reg_rd_resp) begin
         reg_rd_resp <= 1'b0;
      end
    end
  end

  logic b_dropped_valid;
  logic e_cpu_dropped,  b_cpu_dropped;
  logic e_chdr_dropped, b_chdr_dropped;
  // push over the clock domain
  // Sized to fit into 2 SRL's
  axi_fifo_2clk #(.WIDTH(2), .SIZE(4)) fifo_i (
    .reset(eth_rx.rst),
    .i_aclk(eth_rx.clk),
    .i_tdata({e_cpu_dropped, e_chdr_dropped}),
    .i_tvalid(e_cpu_dropped || e_chdr_dropped), .i_tready(/*not used*/),
    .o_aclk(bus_clk),
    .o_tdata({b_cpu_dropped, b_chdr_dropped}),
    .o_tvalid(b_dropped_valid), .o_tready(1'b1)
  );

  always_comb begin
    cpu_dropped  = b_cpu_dropped  && b_dropped_valid;
    chdr_dropped = b_chdr_dropped && b_dropped_valid;
  end

  eth_ipv4_chdr_adapter #(
    .PROTOVER        (PROTOVER),
    .CPU_FIFO_SIZE   (CPU_FIFO_SIZE),
    .CHDR_FIFO_SIZE  (CHDR_FIFO_SIZE),
    .RT_TBL_SIZE     (RT_TBL_SIZE),
    .NODE_INST       (NODE_INST),
    .DROP_UNKNOWN_MAC(DROP_UNKNOWN_MAC),
    .DROP_MIN_PACKET (DROP_MIN_PACKET),
    .PREAMBLE_BYTES  (PREAMBLE_BYTES),
    .ADD_SOF         (ADD_SOF),
    .SYNC            (SYNC),
    .ENET_W          (ENET_W),
    .CPU_W           (CPU_W),
    .CHDR_W          (CHDR_W)
  ) eth_adapter_i (
    .eth_pause_req   (eth_pause_req),
    .eth_rx          (eth_rx   ),
    .eth_tx          (eth_tx   ),
    .v2e             (v2e      ),
    .e2v             (e2v      ),
    .c2e             (c2e      ),
    .e2c             (e2c      ),
    .device_id       (device_id),
    .my_mac          (my_mac   ),
    .my_ip           (my_ip    ),
    .my_udp_chdr_port(my_udp_chdr_port),
    .my_pause_set    (my_pause_set),
    .my_pause_clear  (my_pause_clear),
    .chdr_dropped    (e_chdr_dropped),
    .cpu_dropped     (e_cpu_dropped)
  );


endmodule : eth_ipv4_interface
