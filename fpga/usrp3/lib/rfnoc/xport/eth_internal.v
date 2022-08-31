//
// Copyright 2019 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: eth_internal
//
// Description:
//
//   This internal Ethernet port is responsible for routing CHDR data between
//   the ARM CPU and RFNoC. Treating the RFNoC interface to the CPU like an
//   internal Ethernet device allows the ARM processor to take advantage of
//   highly optimized DMA engines and software designed for Ethernet. This
//   block also includes an ARP responder for IP address discovery.
//
//   Prefixes are used to distinguish the various AXI-Stream buses:
//
//    - e2h : Ethernet to Host (Ethernet transport adapter to ARM)
//    - h2e : Host to Ethernet (ARM to Ethernet transport adapter)
//    - e2v : Ethernet to CHDR (Ethernet transport to RFNoC)
//    - v2e : CHDR to Ethernet (RFNoC to Ethernet transport adapter)
//    - e2c : Ethernet to CPU (Ethernet transport adapter to ARP responder)
//    - c2e : CPU to Ethernet (ARP responder to Ethernet transport adapter)
//
// Parameters:
//
//    DWIDTH         : Data width for AXI-Lite interface (32 or 64)
//    AWIDTH         : Address width for AXI-Lite interface
//    PORTNUM        : Ethernet port number
//    BYTE_MTU       : Sets the MTU to 2^BYTE_MTU bytes
//    RFNOC_PROTOVER : 16-bit RFNoC protocol version (major[7:0], minor[7:0])
//    NODE_INST      : The node instance to identify this transport adapter
//

`default_nettype none
module eth_internal #(
  parameter        DWIDTH         = 32,
  parameter        AWIDTH         = 14,
  parameter [7:0]  PORTNUM        = 0,
  parameter        BYTE_MTU       = $clog2(8192),
  parameter [15:0] RFNOC_PROTOVER = {8'd1, 8'd0},
  parameter        NODE_INST      = 0
)(
  // Resets
  input wire        bus_rst,

  // Clocks
  input wire        bus_clk,

  //Axi-lite
  input  wire                s_axi_aclk,
  input  wire                s_axi_aresetn,
  input  wire [AWIDTH-1:0]   s_axi_awaddr,
  input  wire                s_axi_awvalid,
  output wire                s_axi_awready,

  input  wire [DWIDTH-1:0]   s_axi_wdata,
  input  wire [DWIDTH/8-1:0] s_axi_wstrb,
  input  wire                s_axi_wvalid,
  output wire                s_axi_wready,

  output wire [1:0]          s_axi_bresp,
  output wire                s_axi_bvalid,
  input  wire                s_axi_bready,

  input  wire [AWIDTH-1:0]   s_axi_araddr,
  input  wire                s_axi_arvalid,
  output wire                s_axi_arready,

  output wire [DWIDTH-1:0]   s_axi_rdata,
  output wire [1:0]          s_axi_rresp,
  output wire                s_axi_rvalid,
  input  wire                s_axi_rready,

  // Host-Ethernet DMA interface
  output wire [63:0]  e2h_tdata,
  output wire [7:0]   e2h_tkeep,
  output wire         e2h_tlast,
  output wire         e2h_tvalid,
  input  wire         e2h_tready,

  input  wire [63:0]  h2e_tdata,
  input  wire [7:0]   h2e_tkeep,
  input  wire         h2e_tlast,
  input  wire         h2e_tvalid,
  output wire         h2e_tready,

  // RFNoC interface
  output wire [63:0]  e2v_tdata,
  output wire         e2v_tlast,
  output wire         e2v_tvalid,
  input  wire         e2v_tready,

  input  wire [63:0]  v2e_tdata,
  input  wire         v2e_tlast,
  input  wire         v2e_tvalid,
  output wire         v2e_tready,

  // MISC
  output wire  [31:0] port_info,
  input  wire  [15:0] device_id,

  output wire         link_up,
  output reg          activity

);

  localparam REG_BASE_ETH_IO      = 14'h0;
  localparam REG_BASE_ETH_SWITCH  = 14'h1000;

  // AXI4-Lite to RegPort (PS to PL Register Access)
  wire                reg_wr_req;
  wire  [AWIDTH-1:0]  reg_wr_addr;
  wire  [DWIDTH-1:0]  reg_wr_data;
  wire                reg_rd_req;
  wire  [AWIDTH-1:0]  reg_rd_addr;
  wire                reg_rd_resp, reg_rd_resp_eth_if;
  reg                 reg_rd_resp_io = 1'b0;
  wire  [DWIDTH-1:0]  reg_rd_data, reg_rd_data_eth_if;
  reg   [DWIDTH-1:0]  reg_rd_data_io = 'd0;

  axil_regport_master #(
    .DWIDTH         (DWIDTH),   // Width of the AXI4-Lite data bus (must be 32 or 64)
    .AWIDTH         (AWIDTH),   // Width of the address bus
    .WRBASE         (0),        // Write address base
    .RDBASE         (0),        // Read address base
    .TIMEOUT        (10)        // log2(timeout). Read will timeout after (2^TIMEOUT - 1) cycles
  ) eth_dma_reg_mst_i (
    // Clock and reset
    .s_axi_aclk     (s_axi_aclk),
    .s_axi_aresetn  (s_axi_aresetn),
    // AXI4-Lite: Write address port (domain: s_axi_aclk)
    .s_axi_awaddr   (s_axi_awaddr),
    .s_axi_awvalid  (s_axi_awvalid),
    .s_axi_awready  (s_axi_awready),
    // AXI4-Lite: Write data port (domain: s_axi_aclk)
    .s_axi_wdata    (s_axi_wdata),
    .s_axi_wstrb    (s_axi_wstrb),
    .s_axi_wvalid   (s_axi_wvalid),
    .s_axi_wready   (s_axi_wready),
    // AXI4-Lite: Write response port (domain: s_axi_aclk)
    .s_axi_bresp    (s_axi_bresp),
    .s_axi_bvalid   (s_axi_bvalid),
    .s_axi_bready   (s_axi_bready),
    // AXI4-Lite: Read address port (domain: s_axi_aclk)
    .s_axi_araddr   (s_axi_araddr),
    .s_axi_arvalid  (s_axi_arvalid),
    .s_axi_arready  (s_axi_arready),
    // AXI4-Lite: Read data port (domain: s_axi_aclk)
    .s_axi_rdata    (s_axi_rdata),
    .s_axi_rresp    (s_axi_rresp),
    .s_axi_rvalid   (s_axi_rvalid),
    .s_axi_rready   (s_axi_rready),
    // Register port: Write port (domain: reg_clk)
    .reg_clk        (bus_clk),
    .reg_wr_req     (reg_wr_req),
    .reg_wr_addr    (reg_wr_addr),
    .reg_wr_data    (reg_wr_data),
    // Register port: Read port (domain: reg_clk)
    .reg_rd_req     (reg_rd_req),
    .reg_rd_addr    (reg_rd_addr),
    .reg_rd_resp    (reg_rd_resp),
    .reg_rd_data    (reg_rd_data)
  );

  // Regport Mux for response
  regport_resp_mux #(
    .WIDTH      (DWIDTH),
    .NUM_SLAVES (2)
  ) reg_resp_mux_i (
    .clk(bus_clk), .reset(bus_rst),
    .sla_rd_resp({reg_rd_resp_eth_if, reg_rd_resp_io}),
    .sla_rd_data({reg_rd_data_eth_if, reg_rd_data_io}),
    .mst_rd_resp(reg_rd_resp), .mst_rd_data(reg_rd_data)
  );

  // ARP responder
  wire [63:0] e2c_tdata;
  wire [7:0]  e2c_tkeep;
  wire        e2c_tlast;
  wire        e2c_tvalid;
  wire        e2c_tready;

  wire [63:0] c2e_tdata;
  wire [7:0]  c2e_tkeep;
  wire        c2e_tlast;
  wire        c2e_tvalid;
  wire        c2e_tready;

  wire [3:0] e2c_tuser;
  wire [3:0] c2e_tuser;

  // ARM Host-to-Ethernet
  wire [3:0] e2h_tuser;
  wire [3:0] h2e_tuser;

  // Host Ethernet-to-CHDR
  wire [63:0] h2e_chdr_tdata;
  wire [3:0]  h2e_chdr_tuser;
  wire        h2e_chdr_tlast;
  wire        h2e_chdr_tvalid;
  wire        h2e_chdr_tready;
  wire [63:0] e2h_chdr_tdata;
  wire [3:0]  e2h_chdr_tuser;
  wire        e2h_chdr_tlast;
  wire        e2h_chdr_tvalid;
  wire        e2h_chdr_tready;


  // In AXI Stream, tkeep is the byte qualifier that indicates
  // whether the content of the associated byte
  // of TDATA is processed as part of the data stream.
  // tuser as used in eth_interface is the number of valid bytes

  // Converting tuser to tkeep for ingress packets
  assign e2c_tkeep = ~e2c_tlast ? 8'b1111_1111
                   : (e2c_tuser == 4'd0) ? 8'b1111_1111
                   : (e2c_tuser == 4'd1) ? 8'b0000_0001
                   : (e2c_tuser == 4'd2) ? 8'b0000_0011
                   : (e2c_tuser == 4'd3) ? 8'b0000_0111
                   : (e2c_tuser == 4'd4) ? 8'b0000_1111
                   : (e2c_tuser == 4'd5) ? 8'b0001_1111
                   : (e2c_tuser == 4'd6) ? 8'b0011_1111
                   : 8'b0111_1111;

  // Converting tkeep to tuser for egress packets
  assign c2e_tuser = ~c2e_tlast ? 4'd0
                   : (c2e_tkeep == 8'b1111_1111) ? 4'd0
                   : (c2e_tkeep == 8'b0111_1111) ? 4'd7
                   : (c2e_tkeep == 8'b0011_1111) ? 4'd6
                   : (c2e_tkeep == 8'b0001_1111) ? 4'd5
                   : (c2e_tkeep == 8'b0000_1111) ? 4'd4
                   : (c2e_tkeep == 8'b0000_0111) ? 4'd3
                   : (c2e_tkeep == 8'b0000_0011) ? 4'd2
                   : (c2e_tkeep == 8'b0000_0001) ? 4'd1
                   : 4'd0;

  // Converting tuser to tkeep for ingress packets
  assign e2h_tkeep = ~e2h_tlast ? 8'b1111_1111
                   : (e2h_tuser == 4'd0) ? 8'b1111_1111
                   : (e2h_tuser == 4'd1) ? 8'b0000_0001
                   : (e2h_tuser == 4'd2) ? 8'b0000_0011
                   : (e2h_tuser == 4'd3) ? 8'b0000_0111
                   : (e2h_tuser == 4'd4) ? 8'b0000_1111
                   : (e2h_tuser == 4'd5) ? 8'b0001_1111
                   : (e2h_tuser == 4'd6) ? 8'b0011_1111
                   : 8'b0111_1111;

  // Converting tkeep to tuser for egress packets
  assign h2e_tuser = ~h2e_tlast ? 4'd0
                   : (h2e_tkeep == 8'b1111_1111) ? 4'd0
                   : (h2e_tkeep == 8'b0111_1111) ? 4'd7
                   : (h2e_tkeep == 8'b0011_1111) ? 4'd6
                   : (h2e_tkeep == 8'b0001_1111) ? 4'd5
                   : (h2e_tkeep == 8'b0000_1111) ? 4'd4
                   : (h2e_tkeep == 8'b0000_0111) ? 4'd3
                   : (h2e_tkeep == 8'b0000_0011) ? 4'd2
                   : (h2e_tkeep == 8'b0000_0001) ? 4'd1
                   : 4'd0;

  // FPGA-side addresses for the ARP responder
  wire [47:0] my_mac;
  wire [31:0] my_ip;
  wire [15:0] my_udp_port;

  arm_deframer arm_deframer_i (
    .clk(bus_clk),
    .reset(bus_rst),
    .clear(1'b0),
    .s_axis_tdata(h2e_tdata),
    .s_axis_tuser(h2e_tuser),
    .s_axis_tlast(h2e_tlast),
    .s_axis_tvalid(h2e_tvalid),
    .s_axis_tready(h2e_tready),
    .m_axis_tdata(h2e_chdr_tdata),
    .m_axis_tuser(h2e_chdr_tuser),
    .m_axis_tlast(h2e_chdr_tlast),
    .m_axis_tvalid(h2e_chdr_tvalid),
    .m_axis_tready(h2e_chdr_tready)
  );

  axi64_to_xge64 arm_framer (
    .clk(bus_clk),
    .reset(bus_rst),
    .clear(1'b0),
    .s_axis_tdata(e2h_chdr_tdata),
    .s_axis_tuser(e2h_chdr_tuser),
    .s_axis_tlast(e2h_chdr_tlast),
    .s_axis_tvalid(e2h_chdr_tvalid),
    .s_axis_tready(e2h_chdr_tready),
    .m_axis_tdata(e2h_tdata),
    .m_axis_tuser(e2h_tuser),
    .m_axis_tlast(e2h_tlast),
    .m_axis_tvalid(e2h_tvalid),
    .m_axis_tready(e2h_tready)
  );

  eth_interface #(
     .PROTOVER(RFNOC_PROTOVER),
     .MTU(BYTE_MTU-3),                // Log base 2 of the MTU in 64-bit words
     .NODE_INST(NODE_INST),
     .REG_AWIDTH (AWIDTH),
     .BASE(REG_BASE_ETH_SWITCH)
  ) eth_interface (
    .clk           (bus_clk),
    .reset         (bus_rst),
    .device_id     (device_id),
    .reg_wr_req    (reg_wr_req),
    .reg_wr_addr   (reg_wr_addr),
    .reg_wr_data   (reg_wr_data),
    .reg_rd_req    (reg_rd_req),
    .reg_rd_addr   (reg_rd_addr),
    .reg_rd_resp   (reg_rd_resp_eth_if),
    .reg_rd_data   (reg_rd_data_eth_if),
    .my_mac        (my_mac),
    .my_ip         (my_ip),
    .my_udp_port   (my_udp_port),
    .eth_tx_tdata  (e2h_chdr_tdata),
    .eth_tx_tuser  (e2h_chdr_tuser),
    .eth_tx_tlast  (e2h_chdr_tlast),
    .eth_tx_tvalid (e2h_chdr_tvalid),
    .eth_tx_tready (e2h_chdr_tready),
    .eth_rx_tdata  (h2e_chdr_tdata),
    .eth_rx_tuser  (h2e_chdr_tuser),
    .eth_rx_tlast  (h2e_chdr_tlast),
    .eth_rx_tvalid (h2e_chdr_tvalid),
    .eth_rx_tready (h2e_chdr_tready),
    .e2v_tdata     (e2v_tdata),
    .e2v_tlast     (e2v_tlast),
    .e2v_tvalid    (e2v_tvalid),
    .e2v_tready    (e2v_tready),
    .v2e_tdata     (v2e_tdata),
    .v2e_tlast     (v2e_tlast),
    .v2e_tvalid    (v2e_tvalid),
    .v2e_tready    (v2e_tready),
    .e2c_tdata     (e2c_tdata),
    .e2c_tuser     (e2c_tuser),
    .e2c_tlast     (e2c_tlast),
    .e2c_tvalid    (e2c_tvalid),
    .e2c_tready    (e2c_tready),
    .c2e_tdata     (c2e_tdata),
    .c2e_tuser     (c2e_tuser),
    .c2e_tlast     (c2e_tlast),
    .c2e_tvalid    (c2e_tvalid),
    .c2e_tready    (c2e_tready)
  );

  arp_responder arp_responder_i (
    .aclk          (bus_clk),
    .aresetn       (~bus_rst),
    .mac_addr      (my_mac),
    .ip_addr       (my_ip),
    .s_axis_tdata  (e2c_tdata),
    .s_axis_tvalid (e2c_tvalid),
    .s_axis_tready (e2c_tready),
    .s_axis_tkeep  (e2c_tkeep),
    .s_axis_tlast  (e2c_tlast),
    .s_axis_tuser  (1'b0),
    .m_axis_tdata  (c2e_tdata),
    .m_axis_tvalid (c2e_tvalid),
    .m_axis_tready (c2e_tready),
    .m_axis_tkeep  (c2e_tkeep),
    .m_axis_tlast  (c2e_tlast),
    .m_axis_tuser  ()
  );

  //-----------------------------------------------------------------
  // "I/O" Registers
  //-----------------------------------------------------------------
  localparam [7:0] COMPAT_NUM         = 8'd2;
  localparam [7:0] MGT_PROTOCOL       = 8'd4; // 10 GbE Internal (8'd2 is 10 GbE External)

  // Common registers
  localparam REG_PORT_INFO            = REG_BASE_ETH_IO + 'h0;
  localparam REG_MAC_CTRL_STATUS      = REG_BASE_ETH_IO + 'h4;
  localparam REG_PHY_CTRL_STATUS      = REG_BASE_ETH_IO + 'h8;
  localparam REG_MAC_LED_CTL          = REG_BASE_ETH_IO + 'hC;

  // Protocol specific constants
  localparam [1:0]  MAC_LED_CTL_RST_VAL = 2'h0;

  localparam [31:0] MAC_CTRL_RST_VAL = {31'h0, 1'b1}; // tx_enable on reset
  localparam [31:0] PHY_CTRL_RST_VAL = 32'h0;

  // Writable registers
  reg [31:0] mac_ctrl_reg = MAC_CTRL_RST_VAL;
  reg [31:0] phy_ctrl_reg = PHY_CTRL_RST_VAL;
  reg [1:0]  mac_led_ctl  = MAC_LED_CTL_RST_VAL;

  always @(posedge bus_clk) begin
    if (bus_rst) begin
      mac_ctrl_reg <= MAC_CTRL_RST_VAL;
      phy_ctrl_reg <= PHY_CTRL_RST_VAL;
      mac_led_ctl  <= MAC_LED_CTL_RST_VAL;
    end else if (reg_wr_req) begin
      case(reg_wr_addr)
        REG_MAC_CTRL_STATUS:
          mac_ctrl_reg <= reg_wr_data;
        REG_PHY_CTRL_STATUS:
          phy_ctrl_reg <= reg_wr_data;
        REG_MAC_LED_CTL:
          mac_led_ctl <= reg_wr_data[1:0];
      endcase
    end
  end

  // Readable registers
  wire [31:0] mac_status, phy_status;

  assign port_info = {COMPAT_NUM, 6'h0, activity, link_up, MGT_PROTOCOL, PORTNUM};

  always @(posedge bus_clk) begin
    // No reset handling needed for readback
    if (reg_rd_req) begin
      reg_rd_resp_io <= 1'b1;
      case(reg_rd_addr)
        REG_PORT_INFO:
          reg_rd_data_io <= port_info;
        REG_MAC_CTRL_STATUS:
          reg_rd_data_io <= mac_status;
        REG_PHY_CTRL_STATUS:
          reg_rd_data_io <= phy_status;
        REG_MAC_LED_CTL:
          reg_rd_data_io <= {30'd0, mac_led_ctl};
        default:
          reg_rd_resp_io <= 1'b0;
      endcase
    end if (reg_rd_resp_io) begin
      reg_rd_resp_io <= 1'b0;
    end
  end

  assign mac_status = 'd0;
  assign phy_status[31:8] = 24'h0;
  assign link_up = 1'b1;

  wire identify_enable = mac_led_ctl[0];
  wire identify_value  = mac_led_ctl[1];

  //-----------------------------------------------------------------
  // Activity detector
  //-----------------------------------------------------------------
  wire activity_int;

  pulse_stretch act_pulse_str_i (
    .clk(bus_clk),
    .rst(bus_rst | ~link_up),
    .pulse((h2e_tvalid & h2e_tready) | (e2h_tvalid & e2h_tready)),
    .pulse_stretched(activity_int)
  );

  always @ (posedge bus_clk) activity <= identify_enable ? identify_value : activity_int;

endmodule
`default_nettype wire
