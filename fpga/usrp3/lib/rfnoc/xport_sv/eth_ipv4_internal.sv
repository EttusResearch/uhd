//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: eth_ipv4_internal
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
//    CHDR_W         : CHDR width used by RFNoC on the FPGA
//    NET_CHDR_W     : CHDR width used over the network connection
//    BYTE_MTU       : Sets the MTU to 2^BYTE_MTU bytes
//    DWIDTH         : Data width for AXI-Lite interface (32 or 64)
//    AWIDTH         : Address width for AXI-Lite interface
//    PORTNUM        : Ethernet port number
//    RFNOC_PROTOVER : 16-bit RFNoC protocol version (major[7:0], minor[7:0])
//

`default_nettype none


module eth_ipv4_internal #(
  parameter        CHDR_W         = 64,
  parameter        NET_CHDR_W     = CHDR_W,
  parameter        BYTE_MTU       = 10,
  parameter        DWIDTH         = 32,
  parameter        AWIDTH         = 14,
  parameter [ 7:0] PORTNUM        = 0,
  parameter        NODE_INST      = 0,
  parameter [15:0] RFNOC_PROTOVER = {8'd1, 8'd0}
) (
  input wire bus_clk,
  input wire bus_rst,

  // AXI-Lite
  input  wire              s_axi_aclk,
  input  wire              s_axi_aresetn,
  input  wire [AWIDTH-1:0] s_axi_awaddr,
  input  wire              s_axi_awvalid,
  output wire              s_axi_awready,

  input  wire [  DWIDTH-1:0] s_axi_wdata,
  input  wire [DWIDTH/8-1:0] s_axi_wstrb,
  input  wire                s_axi_wvalid,
  output wire                s_axi_wready,

  output wire [1:0] s_axi_bresp,
  output wire       s_axi_bvalid,
  input  wire       s_axi_bready,

  input  wire [AWIDTH-1:0] s_axi_araddr,
  input  wire              s_axi_arvalid,
  output wire              s_axi_arready,

  output wire [DWIDTH-1:0] s_axi_rdata,
  output wire [       1:0] s_axi_rresp,
  output wire              s_axi_rvalid,
  input  wire              s_axi_rready,

  // Host DMA Interface
  output wire [  63:0] e2h_tdata,
  output wire [   7:0] e2h_tkeep,
  output wire          e2h_tlast,
  output wire          e2h_tvalid,
  input  wire          e2h_tready,

  input  wire [  63:0] h2e_tdata,
  input  wire [   7:0] h2e_tkeep,
  input  wire          h2e_tlast,
  input  wire          h2e_tvalid,
  output wire          h2e_tready,

  // RFNoC Interface
  output reg  [CHDR_W-1:0] e2v_tdata,
  output reg               e2v_tlast,
  output reg               e2v_tvalid,
  input  wire              e2v_tready,

  input  wire [CHDR_W-1:0] v2e_tdata,
  input  wire              v2e_tlast,
  input  wire              v2e_tvalid,
  output reg               v2e_tready,

  // Misc
  input  wire [15:0] device_id
);
  // The CPU (host DMA) interface is currently fixed at 64 bits, due to the
  // arp_responder and arm_framer/deframer only supporting 64 bits.
  localparam CPU_W = 64;


  //---------------------------------------------------------------------------
  // AXI-Lite to RegPort Register Access Bridge
  //---------------------------------------------------------------------------

  localparam REG_BASE_ETH_IO      = 14'h0;
  localparam REG_BASE_ETH_SWITCH  = 14'h1000;

  logic                reg_wr_req;
  logic  [AWIDTH-1:0]  reg_wr_addr;
  logic  [DWIDTH-1:0]  reg_wr_data;
  logic                reg_rd_req;
  logic  [AWIDTH-1:0]  reg_rd_addr;
  logic                reg_rd_resp;
  logic  [DWIDTH-1:0]  reg_rd_data;

  axil_regport_master #(
    .DWIDTH         (DWIDTH),   // Width of the AXI4-Lite data bus (must be 32 or 64)
    .AWIDTH         (AWIDTH),   // Width of the address bus
    .WRBASE         (0),        // Write address base
    .RDBASE         (0),        // Read address base
    .TIMEOUT        (10)        // Read will timeout after (2^TIMEOUT-1) cycles
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
    .reg_wr_keep    (),
    // Register port: Read port (domain: reg_clk)
    .reg_rd_req     (reg_rd_req),
    .reg_rd_addr    (reg_rd_addr),
    .reg_rd_resp    (reg_rd_resp),
    .reg_rd_data    (reg_rd_data)
  );

  logic                reg_rd_resp_eth_if;
  logic                reg_rd_resp_io = 1'b0;
  logic  [DWIDTH-1:0]  reg_rd_data_eth_if;
  logic  [DWIDTH-1:0]  reg_rd_data_io = 'd0;

  // RegPort mux for responses
  regport_resp_mux #(
    .WIDTH      (DWIDTH),
    .NUM_SLAVES (2)
  ) reg_resp_mux_i (
    .clk         (bus_clk),
    .reset       (bus_rst),
    .sla_rd_resp ({reg_rd_resp_eth_if, reg_rd_resp_io}),
    .sla_rd_data ({reg_rd_data_eth_if, reg_rd_data_io}),
    .mst_rd_resp (reg_rd_resp),
    .mst_rd_data (reg_rd_data)
  );


  //---------------------------------------------------------------------------
  // ARM Framer/Deframer
  //---------------------------------------------------------------------------
  //
  // The arm_deframer removes bytes from the beginning of every packet sent by
  // the ARM processor to give the packets a specific alignment that will be
  // used later. The framer does the opposite, padding the packet before
  // sending it to the ARM CPU.
  //
  //---------------------------------------------------------------------------

  // Host Ethernet-to-CHDR
  logic [63:0] h2e_chdr_tdata;
  logic [3:0]  h2e_chdr_tuser;
  logic        h2e_chdr_tlast;
  logic        h2e_chdr_tvalid;
  logic        h2e_chdr_tready;
  //
  logic [63:0] e2h_chdr_tdata;
  logic [3:0]  e2h_chdr_tuser;
  logic        e2h_chdr_tlast;
  logic        e2h_chdr_tvalid;
  logic        e2h_chdr_tready;

  logic [3:0] e2h_tuser;
  logic [3:0] h2e_tuser;

  // Converting tuser to tkeep for ingress packets
  assign e2h_tkeep = ~e2h_tlast          ? 8'b1111_1111
                   : (e2h_tuser == 4'd0) ? 8'b1111_1111
                   : (e2h_tuser == 4'd1) ? 8'b0000_0001
                   : (e2h_tuser == 4'd2) ? 8'b0000_0011
                   : (e2h_tuser == 4'd3) ? 8'b0000_0111
                   : (e2h_tuser == 4'd4) ? 8'b0000_1111
                   : (e2h_tuser == 4'd5) ? 8'b0001_1111
                   : (e2h_tuser == 4'd6) ? 8'b0011_1111
                   :                       8'b0111_1111;

  // Convert tkeep to tuser for egress packets
  assign h2e_tuser = ~h2e_tlast                  ? 4'd0
                   : (h2e_tkeep == 8'b1111_1111) ? 4'd0
                   : (h2e_tkeep == 8'b0111_1111) ? 4'd7
                   : (h2e_tkeep == 8'b0011_1111) ? 4'd6
                   : (h2e_tkeep == 8'b0001_1111) ? 4'd5
                   : (h2e_tkeep == 8'b0000_1111) ? 4'd4
                   : (h2e_tkeep == 8'b0000_0111) ? 4'd3
                   : (h2e_tkeep == 8'b0000_0011) ? 4'd2
                   : (h2e_tkeep == 8'b0000_0001) ? 4'd1
                   :                               4'd0;

  arm_deframer arm_deframer_i (
    .clk           (bus_clk),
    .reset         (bus_rst),
    .clear         (1'b0),
    .s_axis_tdata  (h2e_tdata),
    .s_axis_tuser  (h2e_tuser),
    .s_axis_tlast  (h2e_tlast),
    .s_axis_tvalid (h2e_tvalid),
    .s_axis_tready (h2e_tready),
    .m_axis_tdata  (h2e_chdr_tdata),
    .m_axis_tuser  (h2e_chdr_tuser),
    .m_axis_tlast  (h2e_chdr_tlast),
    .m_axis_tvalid (h2e_chdr_tvalid),
    .m_axis_tready (h2e_chdr_tready)
  );

  axi64_to_xge64 arm_framer_i (
    .clk           (bus_clk),
    .reset         (bus_rst),
    .clear         (1'b0),
    .s_axis_tdata  (e2h_chdr_tdata),
    .s_axis_tuser  (e2h_chdr_tuser),
    .s_axis_tlast  (e2h_chdr_tlast),
    .s_axis_tvalid (e2h_chdr_tvalid),
    .s_axis_tready (e2h_chdr_tready),
    .m_axis_tdata  (e2h_tdata),
    .m_axis_tuser  (e2h_tuser),
    .m_axis_tlast  (e2h_tlast),
    .m_axis_tvalid (e2h_tvalid),
    .m_axis_tready (e2h_tready)
  );


  //---------------------------------------------------------------------------
  // Ethernet Interface
  //---------------------------------------------------------------------------

  // FPGA-side addresses for the ARP responder
  logic [47:0] my_mac;
  logic [31:0] my_ip;

  // ARP responder signals
  logic [63:0] e2c_tdata;
  logic [7:0]  e2c_tkeep;
  logic        e2c_tlast;
  logic        e2c_tvalid;
  logic        e2c_tready;
  //
  logic [63:0] c2e_tdata;
  logic [7:0]  c2e_tkeep;
  logic        c2e_tlast;
  logic        c2e_tvalid;
  logic        c2e_tready;

  localparam CPU_USER_W = $clog2(CPU_W/8)+1;  // SOF + trailing bytes

  // Host DMA interfaces
  AxiStreamIf #(.DATA_WIDTH(CPU_W), .USER_WIDTH(CPU_USER_W), .TUSER(1), .TKEEP(0))
    e2h_chdr(bus_clk, bus_rst);
  AxiStreamIf #(.DATA_WIDTH(CPU_W), .USER_WIDTH(CPU_USER_W), .TUSER(1), .TKEEP(0))
    h2e_chdr(bus_clk, bus_rst);

  // RFNoC Interfaces
  AxiStreamIf #(.DATA_WIDTH(CHDR_W), .TUSER(0), .TKEEP(0))
    e2v_chdr(bus_clk, bus_rst);
  AxiStreamIf #(.DATA_WIDTH(CHDR_W), .TUSER(0), .TKEEP(0))
    v2e_chdr(bus_clk, bus_rst);

  // ARP Responder Interfaces
  AxiStreamIf #(.DATA_WIDTH(CPU_W), .TUSER(0), .TKEEP(1))
    e2c_chdr(bus_clk, bus_rst);
  AxiStreamIf #(.DATA_WIDTH(CPU_W), .TUSER(0), .TKEEP(1))
    c2e_chdr(bus_clk, bus_rst);

  // Translate between SystemVerilog interfaces and Verilog signals
  always_comb begin
    e2h_chdr_tdata  = e2h_chdr.tdata;
    e2h_chdr_tlast  = e2h_chdr.tlast;
    e2h_chdr_tvalid = e2h_chdr.tvalid;
    e2h_chdr_tuser  = e2h_chdr.tuser;
    e2h_chdr.tready = e2h_chdr_tready;

    h2e_chdr.tdata  = h2e_chdr_tdata;
    h2e_chdr.tlast  = h2e_chdr_tlast;
    h2e_chdr.tvalid = h2e_chdr_tvalid;
    h2e_chdr.tuser  = h2e_chdr_tuser;
    h2e_chdr_tready = h2e_chdr.tready;

    e2v_tdata       = e2v_chdr.tdata;
    e2v_tlast       = e2v_chdr.tlast;
    e2v_tvalid      = e2v_chdr.tvalid;
    e2v_chdr.tready = e2v_tready;

    v2e_chdr.tdata  = v2e_tdata;
    v2e_chdr.tlast  = v2e_tlast;
    v2e_chdr.tvalid = v2e_tvalid;
    v2e_tready      = v2e_chdr.tready;

    e2c_tdata       = e2c_chdr.tdata;
    e2c_tlast       = e2c_chdr.tlast;
    e2c_tkeep       = e2c_chdr.tkeep;
    e2c_tvalid      = e2c_chdr.tvalid;
    e2c_chdr.tready = e2c_tready;

    c2e_chdr.tdata  = c2e_tdata;
    c2e_chdr.tlast  = c2e_tlast;
    c2e_chdr.tkeep  = c2e_tkeep;
    c2e_chdr.tvalid = c2e_tvalid;
    c2e_tready      = c2e_chdr.tready;
  end

  eth_ipv4_interface #(
    .PROTOVER       (RFNOC_PROTOVER),
    .CPU_FIFO_SIZE  (BYTE_MTU),
    .CHDR_FIFO_SIZE (BYTE_MTU),
    .NODE_INST      (NODE_INST),
    .BASE           (REG_BASE_ETH_SWITCH),
    .PREAMBLE_BYTES (6),
    .ADD_SOF        (1),
    .ENET_W         (CPU_W),
    .CPU_W          (CPU_W),
    .CHDR_W         (CHDR_W),
    .NET_CHDR_W     (NET_CHDR_W)
  ) eth_ipv4_interface_i (
    .bus_clk          (bus_clk),
    .bus_rst          (bus_rst),
    .device_id        (device_id),
    .reg_wr_req       (reg_wr_req),
    .reg_wr_addr      (reg_wr_addr),
    .reg_wr_data      (reg_wr_data),
    .reg_rd_req       (reg_rd_req),
    .reg_rd_addr      (reg_rd_addr),
    .reg_rd_resp      (reg_rd_resp_eth_if),
    .reg_rd_data      (reg_rd_data_eth_if),
    .eth_pause_req    (),
    .eth_tx           (e2h_chdr),
    .eth_rx           (h2e_chdr),
    .e2v              (e2v_chdr),
    .v2e              (v2e_chdr),
    .e2c              (e2c_chdr),
    .c2e              (c2e_chdr),
    .my_udp_chdr_port (),
    .my_ip            (my_ip),
    .my_mac           (my_mac)
  );


  //---------------------------------------------------------------------------
  // ARP Responder
  //---------------------------------------------------------------------------
  //
  // This block sends replies to ARP IPv4 frames.
  //
  //---------------------------------------------------------------------------

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


  //---------------------------------------------------------------------------
  // NIXGE Registers
  //---------------------------------------------------------------------------
  //
  // Implement the minimum subset of registers needed by the NIXGE driver for
  // our internal Ethernet port. Only the NIXGE_REG_LED_CTL register is
  // actually used, but the internal adapter doesn't need LED control. So all
  // registers read as 0 and all writes are ignored.
  //
  //---------------------------------------------------------------------------

  // NIXGE Registers
  localparam REG_PORT_INFO       = REG_BASE_ETH_IO + 'h0;
  localparam REG_MAC_CTRL_STATUS = REG_BASE_ETH_IO + 'h4;
  localparam REG_PHY_CTRL_STATUS = REG_BASE_ETH_IO + 'h8;
  localparam REG_MAC_LED_CTL     = REG_BASE_ETH_IO + 'hC;

  always @(posedge bus_clk) begin
    if (reg_rd_req) begin
      case(reg_rd_addr[AWIDTH-1:2])
        REG_PORT_INFO      [AWIDTH-1:2] |
        REG_MAC_CTRL_STATUS[AWIDTH-1:2] |
        REG_PHY_CTRL_STATUS[AWIDTH-1:2] |
        REG_MAC_LED_CTL    [AWIDTH-1:2]:
          reg_rd_resp_io <= 1'b1;
        default:
          reg_rd_resp_io <= 1'b0;
      endcase
    end
  end

endmodule

`default_nettype wire
