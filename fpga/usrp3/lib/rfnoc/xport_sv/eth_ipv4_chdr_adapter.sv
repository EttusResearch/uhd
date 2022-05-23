//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: eth_ipv4_chdr_adapter
//
// Description:
//
//   An Eth+IP+UDP transport adapter for RFNoC. In the RFNoC-to-Eth direction,
//   this module encapsulates CHDR packets (or just the CHDR packet's payload)
//   inside UDP/IP/Eth packets to be sent onto the network. In the other
//   direction, this module looks at the UDP port in the packet to determine if
//   it's a CHDR packet destined for RFNoC or a packet destined for the CPU,
//   then routes the packet accordingly. If it's destined for RFNoC, the
//   Eth/UDP/IP headers are stripped off.
//
//   Traffic to/from the CPU can only go from/to the Eth interface. Traffic
//   to/from RFNoC can only go from/to the Eth interface. There's no path
//   between the CPU and RFNoC.
//
//                               (CPU)
//                              e2c c2e
//                               ▲   │
//                               │   │
//                               │   ▼
//                           ┌───┼───┼───┐
//                           │   │   │   |
//               eth_tx ◄────┼───┼───█───┼◄──── v2e
//   (UDP/IP/Eth)            │   │       │         (RFNoC CHDR)
//               eth_rx ────►┼───█───────┼────► e2v
//                           |           |
//                           └───────────┘
//                         Transport Adapter
//
// Parameters:
//
//   PROTOVER         : RFNoC protocol version {8'd<major>, 8'd<minor>}
//   CPU_FIFO_SIZE    : Log2 of the FIFO depth (in bytes) for the CPU egress path
//   CHDR_FIFO_SIZE   : Log2 of the FIFO depth (in bytes) for the CHDR egress path
//   RT_TBL_SIZE      : Log2 of the depth of the return-address routing table
//   NODE_INST        : The node type to return for a node-info discovery
//   DROP_UNKNOWN_MAC : Drop packets not addressed to us?
//   DROP_MIN_PACKET  : Drop packets smaller than 64 bytes?
//   PREAMBLE_BYTES   : Number of bytes of Preamble expected
//   ADD_SOF          : Add a SOF indication into the tuser field of the e2c
//                      path. If false use TKEEP instead of USER.
//   SYNC             : Set to 1 if the c2e/e2c, v2e/e2v, and eth_rx/eth_tx are
//                      synchronous to each other. Set to 0 to insert clock
//                      crossing logic.
//   ENET_W           : Width of the link to the Ethernet MAC
//   CPU_W            : Width of the CPU interface
//   CHDR_W           : CHDR width used by RFNoC on the FPGA
//   NET_CHDR_W       : CHDR width used over the network connection
//   EN_RX_RAW_PAYD   : Enable CHDR header removal (raw payload) on v2e path
//
// Signals:
//
//   device_id        : The ID of the device that has instantiated this module
//   eth_rx           : The input Ethernet stream from the MAC
//   eth_tx           : The output Ethernet stream to the MAC
//   v2e              : The input CHDR stream from the rfnoc infrastructure
//   e2v              : The output CHDR stream to the rfnoc infrastructure
//   c2e              : The input Ethernet stream from the CPU
//   e2c              : The output Ethernet stream to the CPU
//   my_mac           : The Ethernet (MAC) address of this endpoint
//   my_ip            : The IPv4 address of this endpoint
//   my_udp_chdr_port : The UDP port allocated for CHDR traffic on this endpoint
//

`default_nettype none


module eth_ipv4_chdr_adapter #(
  logic [15:0] PROTOVER         = {8'd1, 8'd0},
  int          CPU_FIFO_SIZE    = $clog2(8*1024),
  int          CHDR_FIFO_SIZE   = $clog2(8*1024),
  int          RT_TBL_SIZE      = 6,
  int          NODE_INST        = 0,
  bit          DROP_UNKNOWN_MAC = 0,
  bit          DROP_MIN_PACKET  = 0,
  int          PREAMBLE_BYTES   = 6,
  bit          ADD_SOF          = 1,
  bit          SYNC             = 0,
  int          ENET_W           = 64,
  int          CPU_W            = 64,
  int          CHDR_W           = 64,
  int          NET_CHDR_W       = CHDR_W,
  bit          EN_RX_RAW_PYLD   = 1
)(

  // Device info
  input  wire  [15:0] device_id,

  // Device addresses
  input  wire  [47:0] my_mac,
  input  wire  [31:0] my_ip,
  input  wire  [15:0] my_udp_chdr_port,
  input  wire  [15:0] my_pause_set,
  input  wire  [15:0] my_pause_clear,

  // Key-value map interface
  input  wire         kv_stb,
  output wire         kv_busy,
  input  wire  [47:0] kv_mac_addr,
  input  wire  [31:0] kv_ip_addr,
  input  wire  [15:0] kv_udp_port,
  input  wire  [15:0] kv_dst_epid,
  input  wire         kv_raw_udp,

  // Dropped packet debug values
  output logic        chdr_dropped,
  output logic        cpu_dropped,

  // Ethernet MAC (domain: eth_rx.clk)
  output logic       eth_pause_req,
  AxiStreamIf.master eth_tx, // tUser = {1'b0,trailing bytes};
  AxiStreamIf.slave  eth_rx, // tUser = {error,trailing bytes};

  // CHDR router interface (eth_rx.clk)
  AxiStreamIf.master e2v, // tUser = {*not used*};
  AxiStreamIf.slave  v2e, // tUser = {*not used*};

  // CPU DMA
  // (domain: e2c.clk if SYNC=0, else eth_rx.clk)
  AxiStreamIf.master e2c, // tUser = {sof,trailing bytes};
  // (domain: c2e.clk if SYNC=0, else eth_rx.clk)
  AxiStreamIf.slave  c2e  // tUser = {1'b0,trailing bytes};

);

  `include "../core/rfnoc_chdr_utils.vh"
  `include "../core/rfnoc_chdr_internal_utils.vh"
  `include "../../axi4s_sv/axi4s.vh"


  localparam ENET_USER_W = $clog2(ENET_W/8)+1;
  localparam CPU_USER_W  = $clog2(CPU_W/8)+1;
  localparam CHDR_USER_W = $clog2(CHDR_W/8);
  localparam MAX_PACKET_BYTES = 2**16;
  localparam DEBUG = 0;

  `include "eth_constants.vh"


  //   tUser = {error,trailing_bytes}
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0)) eth_rx1(eth_rx.clk,eth_rx.rst);

  //---------------------------------------
  // E2V and E2C DEMUX
  //---------------------------------------
  //   tUser = {*not used*}
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.TKEEP(0),.TUSER(0)) e2v1(eth_rx.clk,eth_rx.rst);
  //   tUser = {*not used*}
  AxiStreamIf #(.DATA_WIDTH(CHDR_W),.TKEEP(0),.TUSER(0)) e2v2(e2v.clk,e2v.rst);
  //   tUser = {*not used*}
  AxiStreamIf #(.DATA_WIDTH(CHDR_W),.TKEEP(0),.TUSER(0)) e2v3(e2v.clk,e2v.rst);


  //   tUser = {1'b0,trailing bytes}
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0)) e2c1(eth_rx.clk,eth_rx.rst);
  //   tUser = {sof,trailing bytes} IF ADD_SOF
  AxiStreamIf #(.DATA_WIDTH(CPU_W),.USER_WIDTH(CPU_USER_W),
                .TKEEP(!ADD_SOF), .TUSER(ADD_SOF))
     e2c2(e2c.clk,e2c.rst);

  // The older implementation connected with tUser containing trailing bytes
  // The newer implementation brings in TKEEP
  // inside this block we expect trailing bytes.
  always_comb begin
    `AXI4S_ASSIGN(eth_rx1,eth_rx)
    if (eth_rx.TKEEP) begin
      eth_rx1.tuser = {eth_rx.tuser[ENET_USER_W-1],eth_rx.keep2trailing(eth_rx.tkeep)};
    end
  end

  // Ethernet sink. Inspects packet and dispatches
  // to the correct port.
  eth_ipv4_chdr_dispatch #(
    .CPU_FIFO_SIZE    (CPU_FIFO_SIZE),
    .CHDR_FIFO_SIZE   (CHDR_FIFO_SIZE),
    .PREAMBLE_BYTES   (PREAMBLE_BYTES),
    .MAX_PACKET_BYTES (MAX_PACKET_BYTES),
    .DROP_UNKNOWN_MAC (DROP_UNKNOWN_MAC),
    .DROP_MIN_PACKET  (DROP_MIN_PACKET),
    .ENET_W           (ENET_W)
  ) eth_dispatch_i (
    .eth_pause_req    (eth_pause_req),
    .eth_rx           (eth_rx1),
    .e2v              (e2v1),
    .e2c              (e2c1),
    .my_mac           (my_mac),
    .my_ip            (my_ip),
    .my_udp_chdr_port (my_udp_chdr_port),
    .my_pause_set     (my_pause_set),
    .my_pause_clear   (my_pause_clear),
    .chdr_dropped     (chdr_dropped),
    .cpu_dropped      (cpu_dropped)
  );

  //---------------------------------------
  // E2C Path
  //---------------------------------------
  if (ENET_W != CPU_W || !SYNC) begin : gen_e2c_width_conv
    axi4s_width_conv #(.I_USER_TRAILING_BYTES(1),.O_USER_TRAILING_BYTES(ADD_SOF),.SYNC_CLKS(0))
      e2c_width_conv (.i(e2c1), .o(e2c2));
  end else begin : gen_e2c_width_match
    always_comb begin : e2c_assign
    `AXI4S_ASSIGN(e2c2,e2c1)
    end
  end

  if (ADD_SOF) begin : add_sof
    logic sof = 1'b1;

    // Add SOF
    always_ff @(posedge e2c.clk) begin : cpu3_find_sof
      if (e2c.rst) begin
        sof <= 1'b1;
      end else if (e2c2.tvalid && e2c2.tready) begin
         sof <= e2c2.tlast;
      end
    end
    always_comb begin : e2c2_sof_assign
     `AXI4S_ASSIGN(e2c,e2c2)
      e2c.tuser   = {sof,e2c2.tuser[CPU_USER_W-2:0]};
    end
  end else begin : no_sof
    if (DEBUG) begin
      `AXI4S_DEBUG_ASSIGN(e2c,e2c2)
    end else begin
      always_comb begin : e2c_nodebug_assign
        `AXI4S_ASSIGN(e2c,e2c2)
      end
    end
  end

  //---------------------------------------
  // E2V Path
  //---------------------------------------
  if (ENET_W != CHDR_W || !SYNC) begin : gen_e2v_width_conv
    // assumes full words on input
    axi4s_width_conv #(.SYNC_CLKS(0))
      e2v_width_conv (.i(e2v1), .o(e2v2));
  end else begin : gen_e2v_width_match
    always_comb begin : e2v_assign
      `AXI4S_ASSIGN(e2v2,e2v1)
    end
  end


  //---------------------------------------
  // CHDR Transport Adapter
  //---------------------------------------

  //   tUser = {*not used*}
  AxiStreamIf #(.DATA_WIDTH(CHDR_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0))
    v2e1D(v2e.clk,v2e.rst);
  //   tUser = {*not used*}
  AxiStreamIf #(.DATA_WIDTH(CHDR_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0))
    v2e1(v2e.clk,v2e.rst);
  //   tUser = {*not used*}
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0))
    v2e2(eth_rx.clk,eth_rx.rst);
  //   tUser = {*not used*}
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0))
    v2e3(eth_rx.clk,eth_rx.rst);

  chdr_xport_adapter #(
    .PREAMBLE_BYTES  (PREAMBLE_BYTES  ),
    .MAX_PACKET_BYTES(MAX_PACKET_BYTES),
    .PROTOVER        (PROTOVER        ),
    .TBL_SIZE        (RT_TBL_SIZE     ),
    .NODE_INST       (NODE_INST       ),
    .ALLOW_DISC      (1               ),
    .NET_CHDR_W      (NET_CHDR_W      ),
    .EN_RX_RAW_PYLD  (EN_RX_RAW_PYLD  )
  ) chdr_xport_adapter_i (
    .device_id       (device_id       ),
    .my_mac          (my_mac          ),
    .my_ip           (my_ip           ),
    .my_udp_chdr_port(my_udp_chdr_port),
    .kv_stb          (kv_stb          ),
    .kv_busy         (kv_busy         ),
    .kv_dst_epid     (kv_dst_epid     ),
    .kv_data         ({kv_raw_udp,
                       kv_udp_port,
                       kv_ip_addr,
                       kv_mac_addr   }),
    .eth_rx          (e2v2            ), // from Ethernet
    .e2v             (e2v3            ), // to   CHDR
    .v2e             (v2e             ), // from CHDR
    .eth_tx          (v2e1D           )  // to   Ethernet
  );

  if (DEBUG) begin
    `AXI4S_DEBUG_ASSIGN(v2e1,v2e1D)
  end else begin
    always_comb begin : v2e_nodebug_assign
      `AXI4S_ASSIGN(v2e1,v2e1D)
    end
  end

  // Convert incoming CHDR_W
  if (ENET_W != CHDR_W || !SYNC) begin : gen_v2e_width_conv
    axi4s_width_conv #(.SYNC_CLKS(0),.I_USER_TRAILING_BYTES(1),.O_USER_TRAILING_BYTES(1))
      v2e_width_conv (.i(v2e1), .o(v2e2));
  end else begin : gen_v2e_width_match
    always_comb begin : v2e1_assign
     `AXI4S_ASSIGN(v2e2,v2e1)
    end
  end

  // Adding so packet will be contiguous going out
  // The MAC needs bandwidth feeding it to be greater than the line rate
  if (ENET_W > CHDR_W || !SYNC) begin : gen_v2e_packet_gate
    axi4s_packet_gate #(.SIZE(17-$clog2(ENET_W)), .USE_AS_BUFF(0))
      v2e_gate_i (.clear(1'b0),.error(1'b0),.i(v2e2),.o(v2e3));
  end else begin : gen_v2e_no_packet_gate
    always_comb begin : v2e1_assign
      `AXI4S_ASSIGN(v2e3,v2e2)
    end
  end

  //---------------------------------------
  // E2V Output Buffering
  //---------------------------------------

  if (DEBUG) begin
    `AXI4S_DEBUG_ASSIGN(e2v,e2v3)
  end else begin
    always_comb begin : e2v_direct_assign
      `AXI4S_ASSIGN(e2v,e2v3)
    end
  end

  //---------------------------------------
  // C2E Path
  //---------------------------------------
  //   tUser = {1'b0,trailing bytes}
  AxiStreamIf #(.DATA_WIDTH(c2e.DATA_WIDTH),.USER_WIDTH(c2e.USER_WIDTH),
                .TKEEP(c2e.TKEEP),.TUSER(c2e.TUSER))
    c2eD(c2e.clk,c2e.rst);
  //   tUser = {1'b0,trailing bytes}
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0),.MAX_PACKET_BYTES(MAX_PACKET_BYTES)) c2e1(eth_rx.clk,eth_rx.rst);
  //   tUser = {1'b0,trailing bytes}
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0),.MAX_PACKET_BYTES(MAX_PACKET_BYTES)) c2e2(eth_rx.clk,eth_rx.rst);
  //   tUser = {1'b0,trailing bytes}
  AxiStreamPacketIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0),.MAX_PACKET_BYTES(MAX_PACKET_BYTES)) c2e3(eth_rx.clk,eth_rx.rst);


 if (DEBUG) begin
    `AXI4S_DEBUG_ASSIGN(c2eD,c2e)
  end else begin
    always_comb begin : c2e_nodebug_assign
      `AXI4S_ASSIGN(c2eD,c2e)
    end
  end

  if (ENET_W != CPU_W  || !SYNC) begin : gen_c2e_width_conv
    AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0)) c2e1_0(eth_rx.clk,eth_rx.rst);
    axi4s_width_conv #(.I_USER_TRAILING_BYTES(c2eD.TUSER),.O_USER_TRAILING_BYTES(1),.SYNC_CLKS(0))
      c2e_width_conv (.i(c2eD), .o(c2e1_0));

    if (ENET_W > CPU_W || !SYNC) begin : gen_c2e_packet_gate
      // Adding so packet will be contiguous going out
      // I think the MAC needs bandwidth feeding it to
      // be greater than the line rate
      axi4s_packet_gate #(.SIZE(17-$clog2(ENET_W)), .USE_AS_BUFF(0))
       c2e_gate_i (.clear(1'b0),.error(1'b0),.i(c2e1_0),.o(c2e1));
    end else begin : gen_c2e_no_packet_gate
       always_comb begin : c2e1_assign
         `AXI4S_ASSIGN(c2e1,c2e1_0)
       end
    end
  end else begin : gen_c2e_width_match
    always_comb begin : c2e1_assign
      `AXI4S_ASSIGN(c2e1,c2eD)
    end
  end

  if (PREAMBLE_BYTES > 0) begin : gen_add_preamble
    // Add pad of PREAMBLE_BYTES empty bytes to the ethernet packet going
    // from the CPU to the SFP. This padding added before MAC addresses
    // aligns the source and destination IP addresses, UDP headers etc.
    // Note that the xge_mac_wrapper strips this padding to recreate the
    // ethernet packet.
    axi4s_add_bytes #(.ADD_START(0),.ADD_BYTES(PREAMBLE_BYTES)
    ) add_header (
     .i(c2e1), .o(c2e2)
    );
  end else begin : gen_no_preamble
    always_comb begin : c2e2_assign
      `AXI4S_ASSIGN(c2e2,c2e1)
    end
  end

  localparam FORCE_MIN_PACKET = 1;
  if (FORCE_MIN_PACKET) begin : gen_force_min
    // add extra zero bytes to the end of a packet if it is less
    // than the minimum packet size.
    typedef enum logic {
      ST_IDLE,
      ST_AFTER
    } pad_state_t;
    pad_state_t pad_state = ST_IDLE;
    logic clk_before_minpacket;
    logic pad_last;

    always_comb clk_before_minpacket = c2e3.reached_packet_byte(MIN_PACKET_SIZE_BYTE);

    always_ff @(posedge eth_rx.clk) begin : pad_state_ff
      if (eth_rx.rst) begin
        pad_state <= ST_IDLE;
        pad_last  <= 0;
      end else begin
        if (c2e3.tready && c2e3.tvalid && c2e3.tlast) begin
          pad_state <= ST_IDLE;
        end else if (c2e3.tready && c2e3.tvalid && clk_before_minpacket) begin
          pad_state <= ST_AFTER;
        end
        if (c2e3.tready && c2e3.tvalid && c2e3.tlast) begin
          pad_last <= 0;
        end else if (c2e3.tready && c2e3.tvalid && c2e2.tlast) begin
          pad_last <= 1;
        end


      end
    end

    always_comb begin : c2e3_pad
      if (pad_state == ST_IDLE) begin
        // force to a full word
        // preserve SOF if it's there, but force
        // trailing bytes to zero (full word)
        c2e3.tuser = 0;
        c2e3.tuser[ENET_USER_W-1] = c2e2.tuser[ENET_USER_W-1];
        c2e3.tvalid = c2e2.tvalid;

        // force any tdata bytes that we pad with zero
        // SW recommended forcing the padding bytes to zero
        // but I suspect we could save logic by just allowing
        // trash data.
        foreach (c2e2.tkeep[i]) begin
          if (pad_last || (i >= c2e2.tuser[ENET_USER_W-2:0] &&
                           c2e2.tuser[ENET_USER_W-2:0] != 0)) begin
            c2e3.tdata[i*8 +:8] = 0;
          end else begin
            c2e3.tdata[i*8 +:8] = c2e2.tdata[i*8 +:8];
          end
        end

        if (ENET_W < 512) begin
          // hold off input if we reach the end early
          if (c2e2.tlast) begin
            c2e2.tready = clk_before_minpacket && c2e3.tready;
          end else begin
            c2e2.tready = c2e3.tready;
          end
          // add tlast at end of idle
          c2e3.tlast  = clk_before_minpacket && c2e2.tlast;
       end else begin
         c2e2.tready = c2e3.tready;
         c2e3.tlast  = c2e2.tlast;
       end

      end else begin
        `AXI4S_ASSIGN(c2e3,c2e2)
      end

    end
  end else begin : gen_no_force_min
    always_comb begin : c2e3_assign
      `AXI4S_ASSIGN(c2e3,c2e2)
    end
  end


  //---------------------------------------
  // V2E and C2E MUX
  //---------------------------------------
  //   tUser = {1'b0,trailing bytes}
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0)) eth_tx1 (eth_rx.clk,eth_rx.rst);
  //   tUser = {1'b0,trailing bytes}
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W)) eth_tx2 (eth_rx.clk,eth_rx.rst);


  logic c2e3_tready;
  always_comb begin
    c2e3.tready    = c2e3_tready;
  end
  axi_mux #(
    .SIZE(2), .PRIO(0), .WIDTH(ENET_W+ENET_USER_W), .PRE_FIFO_SIZE(0), .POST_FIFO_SIZE(1)
  ) eth_mux_i (
    .clk(eth_rx.clk), .reset(eth_rx.rst), .clear(1'b0),
    .i_tdata({c2e3.tuser, c2e3.tdata, v2e3.tuser, v2e3.tdata}), .i_tlast({c2e3.tlast, v2e3.tlast}),
    .i_tvalid({c2e3.tvalid, v2e3.tvalid}), .i_tready({c2e3_tready, v2e3.tready}),
    .o_tdata({eth_tx1.tuser, eth_tx1.tdata}), .o_tlast(eth_tx1.tlast),
    .o_tvalid(eth_tx1.tvalid), .o_tready(eth_tx1.tready)
  );



  // Clean up the noisy mux output.  I suspect it is annoying
  // the Xilinx cores that tlast and tuser(tkeep) flop around
  // when tvalid isn't true.
  always_comb begin : eth_tx_assign
    if (eth_tx1.tvalid) begin
      eth_tx2.tvalid = 1'b1;
      eth_tx2.tdata  = eth_tx1.tdata;
      eth_tx2.tlast  = eth_tx1.tlast;
      // driving both tuser and tkeep
      if (eth_tx1.tlast) begin
        eth_tx2.tuser  = eth_tx1.tuser;
        eth_tx2.tkeep  = eth_tx1.trailing2keep(eth_tx1.tuser);
      end else begin
        eth_tx2.tuser  = '0;
        eth_tx2.tkeep  = '1;
      end
    end else begin
      eth_tx2.tvalid = 1'b0;
      eth_tx2.tdata  = 'X; // use X so synth will optimize
      eth_tx2.tlast  = 0;
      eth_tx2.tuser  = '0;
      eth_tx2.tkeep  = '1;
    end
    eth_tx1.tready = eth_tx2.tready;
  end

  //---------------------------------------
  // Output pipeline stage
  //---------------------------------------
  axi4s_fifo #(
    .SIZE(1)
  ) in_reg_i (
    .clear(1'b0),.space(),.occupied(),
    .i(eth_tx2), .o(eth_tx)
  );


endmodule : eth_ipv4_chdr_adapter


`default_nettype wire
