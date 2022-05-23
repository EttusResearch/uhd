//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_xport_adapter (Ethernet IPV4)
//
// Description:
//
// A transport adapter module that does the following:
//
//   - Exposes a configuration port for mgmt packets to configure the node.
//     (chdr_mgmt_pkt_handler)
//   - Implements a return-address map for packets with metadata other than
//     the CHDR. Additional metadata can be passed as a tuser to this module
//     which will store it in a map indexed by the SrcEPID in a management
//     packet. For all returning packets, the metadata will be looked up in
//     the map and attached as the outgoing tuser. (kv_map)
//   - Implements a loopback path for node-info discovery (axi_switch/axi_mux)
//   - Strips incoming UDP headers and extracts MAC/IP/UDP source addresses
//   - Adds UDP/IP/Eth headers for outgoing packets
//   - Optionally removes CHDR header from outgoing packets to enable raw UDP
//
// Parameters:
//
//   PROTOVER       : RFNoC protocol version {8'd<major>, 8'd<minor>}
//   TBL_SIZE       : Log2 of the depth of the routing table
//   NODE_SUBTYPE   : The node subtype to return for a node-info discovery
//   NODE_INST      : The node type to return for a node-info discovery
//   ALLOW_DISC     : Controls if the external transport network should be
//                    discoverable by management packets from RFNoC side.
//   NET_CHDR_W     : CHDR width used over the network connection
//   EN_RX_RAW_PYLD : Enable raw payload (CHDR header removal) on the data
//                    path from the USRP towards the transport interface.
//
// Signals:
//
//   device_id : The ID of the device that has instantiated this module
//   my_*      : MAC address, IP address, and UDP port that responds/accepts CHDR traffic
//   kv_*      : Allows the transport adapter to add entries to KV map
//   eth_rx    : The input CHDR stream from the transport
//   eth_tx    : The output CHDR stream to transport
//   v2e       : The input CHDR stream from the rfnoc infrastructure
//   e2v       : The output CHDR stream to the rfnoc infrastructure
//

`default_nettype none


`include "../xport/rfnoc_xport_types.vh"

module chdr_xport_adapter #(
  parameter int          PREAMBLE_BYTES   = 6,
  parameter int          MAX_PACKET_BYTES = 2**16,
  parameter logic [15:0] PROTOVER         = {8'd1, 8'd0},
  parameter int          TBL_SIZE         = 6,
  parameter logic [7:0]  NODE_SUBTYPE     = NODE_SUBTYPE_XPORT_IPV4_CHDR64,
  parameter int          NODE_INST        = 0,
  parameter bit          ALLOW_DISC       = 1,
  parameter int          NET_CHDR_W       = 64,
  parameter bit          EN_RX_RAW_PYLD   = 1,

  // TUSER used to store {raw_udp, UDP port, IPv4 addr, MAC addr}
  localparam int USER_META_W = 97
) (
  // Device info (domain: eth_rx.clk)
  input  wire  [15:0] device_id,

  // Device addresses (domain: eth_rx.clk)
  input  wire  [47:0] my_mac,
  input  wire  [31:0] my_ip,
  input  wire  [15:0] my_udp_chdr_port,

  // KV map insertion port
  input  wire                    kv_stb,
  output logic                   kv_busy,
  input  wire  [           15:0] kv_dst_epid,
  input  wire  [USER_META_W-1:0] kv_data,

  // Ethernet (domain: eth_rx.clk)
  AxiStreamIf.slave  eth_rx, // tUser={*not used*}
  AxiStreamIf.master eth_tx, // tUser={1'b0,trailing bytes}

  // CHDR (domain: eth_rx.clk)
  AxiStreamIf.slave  v2e,  // tUser={*not used*}
  AxiStreamIf.master e2v   // tUser={*not used*}
);

  localparam int ENET_USER_W = $clog2(eth_rx.DATA_WIDTH/8)+1;

  // ---------------------------------------------------
  // RFNoC Includes
  // ---------------------------------------------------
  `include "../core/rfnoc_chdr_utils.vh"
  `include "../core/rfnoc_chdr_internal_utils.vh"
  `include "eth_constants.vh"
  `include "../../axi4s_sv/axi4s.vh"

  // tUser={None}
  AxiStreamPacketIf #(.DATA_WIDTH(eth_rx.DATA_WIDTH),.TKEEP(0),.TUSER(0),
                .MAX_PACKET_BYTES(MAX_PACKET_BYTES))
    ru1(eth_rx.clk,eth_rx.rst);// Packet handler input
  // tUser={None}
  AxiStreamIf #(.DATA_WIDTH(eth_rx.DATA_WIDTH),.TKEEP(0),.TUSER(0),
                .MAX_PACKET_BYTES(MAX_PACKET_BYTES))
    ru2(eth_rx.clk,eth_rx.rst);// Packet handler input
  // tUser={udp_src_port,ipv4_src_addr,eth_src_addr}
  AxiStreamIf #(.DATA_WIDTH(eth_rx.DATA_WIDTH),.USER_WIDTH(USER_META_W),.TKEEP(0))
    ru3(eth_rx.clk,eth_rx.rst);// Packet handler input
  AxiStreamIf #(.DATA_WIDTH(eth_rx.DATA_WIDTH),.USER_WIDTH(USER_META_W),.TKEEP(0))
    ru4(eth_rx.clk,eth_rx.rst);// Packet handler input
  AxiStreamIf #(.DATA_WIDTH(eth_rx.DATA_WIDTH),.USER_WIDTH(USER_META_W),.TKEEP(0))
    ph(eth_rx.clk,eth_rx.rst);// Packet handler input
  // tUser={udp_src_port,ipv4_src_addr,eth_src_addr}
  AxiStreamIf #(.DATA_WIDTH(eth_rx.DATA_WIDTH),.USER_WIDTH(USER_META_W),.TKEEP(0))
    e2d(eth_rx.clk,eth_rx.rst);// Eth => Demux
  AxiStreamIf #(.DATA_WIDTH(eth_rx.DATA_WIDTH),.USER_WIDTH(USER_META_W),.TKEEP(0))
    e2v_resize(eth_rx.clk,eth_rx.rst);// RX Resize => Management packet handler
  logic [1:0]                  e2d_tid;
  // tUser={udp_src_port,ipv4_src_addr,eth_src_addr}
  AxiStreamIf #(.DATA_WIDTH(eth_rx.DATA_WIDTH),.USER_WIDTH(USER_META_W),.TKEEP(0))
    e2e(eth_rx.clk,eth_rx.rst);// Eth => Eth (loopback)
  // tUser={udp_dst_port, ipv4_dst_addr, eth_dst_addr}
  AxiStreamIf #(.DATA_WIDTH(eth_rx.DATA_WIDTH),.USER_WIDTH(USER_META_W),.TKEEP(0))
    m2e(eth_rx.clk,eth_rx.rst);// Mux => Eth
  logic                        m2e_tdest;  // 1: Return to src, 0: CHDR input

  // ---------------------------------------------------
  // Strip UDP and grab {udp_src_port_old, ipv4_src_addr_old, eth_src_addr_old}
  // ---------------------------------------------------

  always_comb begin : assign_ru1
    `AXI4S_ASSIGN(ru1,eth_rx)
  end

  // Cached fields
  logic [47:0] eth_src_addr_new,  eth_src_addr_old;
  logic [31:0] ipv4_src_addr_new, ipv4_src_addr_old;
  logic [15:0] udp_src_port_new,  udp_src_port_old;

  // save the fields
  always_ff @(posedge eth_rx.clk) begin : field_ff
    if (eth_rx.rst) begin
      eth_src_addr_old  <= '0;
      ipv4_src_addr_old <= '0;
      udp_src_port_old  <= '0;
    end else begin
      eth_src_addr_old  <= eth_src_addr_new;
      ipv4_src_addr_old <= ipv4_src_addr_new;
      udp_src_port_old  <= udp_src_port_new;
    end
  end

  // get the fields - don't use assign. assign will not activate with changes to eth_rx.
  always_comb begin  : get_fields
    eth_src_addr_new  = ru1.get_packet_field48(eth_src_addr_old,SRC_MAC_BYTE,.NETWORK_ORDER(1));
    ipv4_src_addr_new = ru1.get_packet_field32(ipv4_src_addr_old,SRC_IP_BYTE,.NETWORK_ORDER(1));
    udp_src_port_new  = ru1.get_packet_field16(udp_src_port_old,SRC_PORT_BYTE,.NETWORK_ORDER(1));
  end

  // Strip the udp header
  axi4s_remove_bytes #(.REM_START(0),.REM_END(UDP_END)
  ) strip_udp (
    .i(ru1), .o(ru2)
  );

  // start driving the port information
  always_comb begin : assign_ru3
    `AXI4S_ASSIGN(ru3,ru2)
    ru3.tuser     = {1'b0, udp_src_port_old, ipv4_src_addr_old, eth_src_addr_old};
  end


  chdr_trim_payload #(
    .CHDR_W(eth_rx.DATA_WIDTH), .USER_W(USER_META_W)
  ) chdr_trim_i (
    .clk(eth_rx.clk), .rst(eth_rx.rst),
    .s_axis_tdata(ru3.tdata), .s_axis_tuser(ru3.tuser),
    .s_axis_tlast(ru3.tlast), .s_axis_tvalid(ru3.tvalid), .s_axis_tready(ru3.tready),
    .m_axis_tdata(ru4.tdata), .m_axis_tuser(ru4.tuser),
    .m_axis_tlast(ru4.tlast), .m_axis_tvalid(ru4.tvalid), .m_axis_tready(ru4.tready)
  );

  // Pay close attention to when ph.tuser switches versus when it is needed!
  always_comb begin : assign_ph
    `AXI4S_ASSIGN(ph,ru4)
  end

  // ---------------------------------------------------
  // Rewrite packets from network to use FPGA CHDR_W
  // ---------------------------------------------------
  if (NET_CHDR_W != eth_rx.DATA_WIDTH) begin : gen_chdr_resize_e2v
    chdr_resize #(
      .I_CHDR_W (NET_CHDR_W),
      .O_CHDR_W (eth_rx.DATA_WIDTH),
      .I_DATA_W (eth_rx.DATA_WIDTH),
      .O_DATA_W (eth_rx.DATA_WIDTH),
      .USER_W   (USER_META_W),
      .PIPELINE ("IN")
    ) chdr_resize_e2v (
      .clk           (eth_rx.clk),
      .rst           (eth_rx.rst),
      .i_chdr_tdata  (ph.tdata),
      .i_chdr_tuser  (ph.tuser),
      .i_chdr_tlast  (ph.tlast),
      .i_chdr_tvalid (ph.tvalid),
      .i_chdr_tready (ph.tready),
      .o_chdr_tdata  (e2v_resize.tdata),
      .o_chdr_tuser  (e2v_resize.tuser),
      .o_chdr_tlast  (e2v_resize.tlast),
      .o_chdr_tvalid (e2v_resize.tvalid),
      .o_chdr_tready (e2v_resize.tready)
    );
  end else begin : gen_no_chdr_resize_e2v
    always_comb begin
      `AXI4S_ASSIGN(e2v_resize, ph);
    end
  end

  // ---------------------------------------------------
  // Transport => DEMUX
  // ---------------------------------------------------
  logic              op_stb;
  logic [15:0]       op_src_epid;
  logic [USER_META_W-1:0] op_data;
  logic              lookup_stb, lookup_done_stb, lookup_result_match;
  logic [15:0]       lookup_epid;
  logic [USER_META_W-1:0] lookup_result_value;
  logic [47:0]       node_info;

  always_comb node_info = chdr_mgmt_build_node_info(
      { 10'h0, NODE_SUBTYPE},
      NODE_INST, NODE_TYPE_TRANSPORT, device_id);

  chdr_mgmt_pkt_handler #(
    .PROTOVER(PROTOVER), .CHDR_W(eth_rx.DATA_WIDTH), .USER_W(USER_META_W), .MGMT_ONLY(0)
  ) mgmt_ep_i (
    .clk(eth_rx.clk), .rst(eth_rx.rst),
    .node_info(node_info),
    //ph in
    .s_axis_chdr_tdata(e2v_resize.tdata), .s_axis_chdr_tlast(e2v_resize.tlast),
    .s_axis_chdr_tvalid(e2v_resize.tvalid), .s_axis_chdr_tready(e2v_resize.tready),
    .s_axis_chdr_tuser(e2v_resize.tuser),
    //e2d out
    .m_axis_chdr_tdata(e2d.tdata), .m_axis_chdr_tlast(e2d.tlast),
    .m_axis_chdr_tdest(/* unused */), .m_axis_chdr_tid(e2d_tid),
    .m_axis_chdr_tvalid(e2d.tvalid), .m_axis_chdr_tready(e2d.tready),
    //unused ctrlport
    .ctrlport_req_wr     (/* unused */),
    .ctrlport_req_rd     (/* unused */),
    .ctrlport_req_addr   (/* unused */),
    .ctrlport_req_data   (/* unused */),
    .ctrlport_resp_ack   (1'b0  /* unused */),
    .ctrlport_resp_data  (32'b0 /* unused */),
    // kv_map lookups
    .op_stb(op_stb),
    .op_dst_epid(/* unused */),
    .op_src_epid(op_src_epid),
    .op_data(op_data)
  );

  // Key/Value map.
  // Stores the destination address information for each destination EPID.
  //  - Writes come from the chdr_mgmt_pkt_handler or kv configuration port.
  //  - Lookup is done on each packet going from RFNoC to Ethernet.
  //  - We assume that we will never try to insert faster than the kv_map can
  //    handle from CHDR mgmt interface (time between op_stb > insertion time).
  //    This is not assumed for the kv_* interface (kv_busy must be false
  //    before inserting a new entry).
  //  - We assume we will never try to insert from mgmt interface and transport
  //    adapter at the same time. Mgmt interface takes precedence.
  kv_map #(
    .KEY_WIDTH(16         ),
    .VAL_WIDTH(USER_META_W),
    .SIZE     (TBL_SIZE   )
  ) kv_map_i (
    .clk           (eth_rx.clk                        ),
    .reset         (eth_rx.rst                        ),
    .insert_stb    (op_stb | kv_stb                   ),
    .insert_key    (op_stb ? op_src_epid : kv_dst_epid),
    .insert_val    (op_stb ? op_data     : kv_data    ),
    .insert_busy   (kv_busy                           ),
    .find_key_stb  (lookup_stb                        ),
    .find_key      (lookup_epid                       ),
    .find_res_stb  (lookup_done_stb                   ),
    .find_res_match(lookup_result_match               ),
    .find_res_val  (lookup_result_value               ),
    .count         (/* unused */                      )
  );

  logic ph_hdr = 1'b1;
  always_ff @(posedge eth_rx.clk) begin
    if (eth_rx.rst)
      ph_hdr <= 1'b1;
    else if (ph.tvalid && ph.tready)
      ph_hdr <= ph.tlast;
  end

  // chdr_mgmt_pkt_handler does not buffer packets and has at least one cycle of delay.
  // The tuser caching logic could be more robust.
  always_ff @(posedge eth_rx.clk) begin
    if (ph.tvalid && ph.tready && ph_hdr)
      e2d.tuser <= ph.tuser;
  end

  // ---------------------------------------------------
  // Optional management filter
  // ---------------------------------------------------
  // tUser={*not used*}
  AxiStreamIf #(.DATA_WIDTH(eth_rx.DATA_WIDTH),.TUSER(0),.TKEEP(0))
    f2m(eth_rx.clk,eth_rx.rst);

  if (ALLOW_DISC) begin : gen_no_mgmt_filter
    // Allow all packets to pass through
    always_comb begin
      f2m.tdata  = v2e.tdata;
      f2m.tlast  = v2e.tlast;
      f2m.tvalid = v2e.tvalid;
      v2e.tready = f2m.tready;
    end

  end else begin : gen_mgmt_filter
    // Disallow forwarding of management discovery packets from RFNoC to the
    // transport interface for transports that don't support them.
    //vhook_nowarn unused_*
    logic [eth_rx.DATA_WIDTH-1:0] unused_tdata;
    logic                         unused_tlast, unused_tvalid;
    logic [eth_rx.DATA_WIDTH-1:0] s_header;
    logic                         dispose_pkt;

    // We identify discovery packets by the fact that they are management
    // packets and that they use the null EPID as the destination.
    always_comb dispose_pkt = (chdr_get_pkt_type(s_header[63:0]) == CHDR_PKT_TYPE_MGMT) &&
                              (chdr_get_dst_epid(s_header[63:0]) == NULL_EPID);


    axi_demux #(
      .WIDTH          (eth_rx.DATA_WIDTH),
      .SIZE           (2),
      .PRE_FIFO_SIZE  (0),
      .POST_FIFO_SIZE (1)
    ) axi_demux_mgmt_filter_i (
      .clk      (eth_rx.clk),
      .reset    (eth_rx.rst),
      .clear    (1'b0),
      .header   (s_header),
      .dest     (dispose_pkt),
      .i_tdata  (v2e.tdata),
      .i_tlast  (v2e.tlast),
      .i_tvalid (v2e.tvalid),
      .i_tready (v2e.tready),
      .o_tdata  ({unused_tdata, f2m.tdata}),
      .o_tlast  ({unused_tlast, f2m.tlast}),
      .o_tvalid ({unused_tvalid, f2m.tvalid}),
      .o_tready ({1'b1, f2m.tready})
    );
  end

  // ---------------------------------------------------
  // MUX and DEMUX for return path
  // ---------------------------------------------------

  logic [USER_META_W-1:0] unused_tuser;

  axis_switch #(
    .DATA_W(eth_rx.DATA_WIDTH+USER_META_W), .DEST_W(1), .IN_PORTS(1), .OUT_PORTS(2), .PIPELINE(0)
  ) rtn_demux_i (
    .clk(eth_rx.clk), .reset(eth_rx.rst),
    .s_axis_tdata({e2d.tuser, e2d.tdata}), .s_axis_alloc(1'b0),
    .s_axis_tdest(e2d_tid == CHDR_MGMT_RETURN_TO_SRC ? 2'b01 : 2'b00),
    .s_axis_tlast(e2d.tlast), .s_axis_tvalid(e2d.tvalid), .s_axis_tready(e2d.tready),
    .m_axis_tdata({e2e.tuser, e2e.tdata, unused_tuser, e2v.tdata}),
    .m_axis_tdest(/* unused */),
    .m_axis_tlast({e2e.tlast, e2v.tlast}),
    .m_axis_tvalid({e2e.tvalid, e2v.tvalid}),
    .m_axis_tready({e2e.tready, e2v.tready})
  );

  axi_mux #(
    .WIDTH(eth_rx.DATA_WIDTH+USER_META_W+1), .SIZE(2), .PRE_FIFO_SIZE(0), .POST_FIFO_SIZE(0)
  ) rtn_mux_i (
    .clk(eth_rx.clk), .reset(eth_rx.rst), .clear(1'b0),
    .i_tdata({1'b1, e2e.tuser, e2e.tdata, 1'b0, {USER_META_W{1'b0}}, f2m.tdata}),
    .i_tlast({e2e.tlast, f2m.tlast}),
    .i_tvalid({e2e.tvalid, f2m.tvalid}), .i_tready({e2e.tready, f2m.tready}),
    .o_tdata({m2e_tdest, m2e.tuser, m2e.tdata}), .o_tlast(m2e.tlast),
    .o_tvalid(m2e.tvalid), .o_tready(m2e.tready)
  );

  // ---------------------------------------------------
  // MUX => Transport
  // ---------------------------------------------------

  // In this section we must determine what value to put in tuser. If tdest is
  // 1 then tuser is passed through unchanged. If tdest is 0 then the tuser
  // value is looked up in the KV map using the EPID in the packet header.
  //
  // To do this we split the data (tdata, tlast) and the routing information
  // (tdest, tuser, and the EPID) into two FIFOs. This allows us to perform a
  // routing lookup and decide what to do while we continue to buffer data.
  //
  // With small packets, multiple routing lookups might be enqueued in the
  // lookup_fifo, but we can only do one lookup at a time. Output logic
  // controls release of packets from the data FIFO to ensure we only output
  // one packet per lookup after the lookup is complete.

  AxiStreamIf #(.DATA_WIDTH(eth_rx.DATA_WIDTH),.TUSER(0),.TKEEP(0))
    resize_v2e(eth_rx.clk,eth_rx.rst);
  AxiStreamIf #(.DATA_WIDTH(eth_rx.DATA_WIDTH),.USER_WIDTH(0),.TKEEP(0))
    data_fifo_o(eth_rx.clk,eth_rx.rst);// TX Resize => Management packet handler
  AxiStreamIf #(.DATA_WIDTH(eth_rx.DATA_WIDTH),.TUSER(0),.TKEEP(0))
    data_fifo_i(eth_rx.clk,eth_rx.rst);

  AxiStreamIf #(.DATA_WIDTH(1+USER_META_W+16),.TUSER(0),.TKEEP(0))
    lookup_fifo_o(eth_rx.clk,eth_rx.rst);
  AxiStreamIf #(.DATA_WIDTH(1+USER_META_W+16),.TUSER(0),.TKEEP(0))
    lookup_fifo_i(eth_rx.clk,eth_rx.rst);

  logic              lookup_fifo_tdest;
  logic [USER_META_W-1:0] lookup_fifo_tuser;
  logic [      15:0] lookup_fifo_tepid;
  logic              non_lookup_done_stb;
  logic              resize_v2e_hdr = 1'b1;
  logic              pass_packet;
  logic [USER_META_W-1:0] result_tuser;
  logic              result_tuser_valid;
  logic [USER_META_W-1:0] reg_o_tuser;

  // Track when the next m2e word contains is the start of a new packet
  logic m2e_hdr = 1'b1;
  always_ff @(posedge eth_rx.clk) begin : m2e_hdr_ff
    if (eth_rx.rst)
      m2e_hdr <= 1'b1;
    else if (m2e.tvalid && m2e.tready)
      m2e_hdr <= m2e.tlast;
  end

  // We can only accept data from the mux when when both the data_fifo and
  // lookup_fifo are ready.
  always_comb  data_fifo_i.tdata   = m2e.tdata;
  always_comb  data_fifo_i.tlast   = m2e.tlast;
  always_comb  data_fifo_i.tvalid  = m2e.tvalid && m2e.tready;
  always_comb  m2e.tready          = data_fifo_i.tready && lookup_fifo_i.tready;

  // The data_fifo only takes the packet data (tdata, tlast). We use an
  // axi_fifo_short module for the data_fifo because it can tolerate tvalid
  // going low before a transfer completes.
  axi_fifo_short #(
    .WIDTH (1+eth_rx.DATA_WIDTH)
  ) data_fifo (
    .clk      (eth_rx.clk),
    .reset    (eth_rx.rst),
    .clear    (1'b0),
    .i_tdata  ({data_fifo_i.tlast, data_fifo_i.tdata}),
    .i_tvalid (data_fifo_i.tvalid),
    .i_tready (data_fifo_i.tready),
    .o_tdata  ({data_fifo_o.tlast, data_fifo_o.tdata}),
    .o_tvalid (data_fifo_o.tvalid),
    .o_tready (data_fifo_o.tready),
    .space    (),
    .occupied ()
  );

  // ---------------------------------------------------
  // Rewrite packets from FPGA to use network CHDR_W
  // ---------------------------------------------------
  if (NET_CHDR_W != eth_rx.DATA_WIDTH) begin : gen_chdr_resize_v2e
    chdr_resize #(
      .I_CHDR_W (eth_rx.DATA_WIDTH),
      .O_CHDR_W (NET_CHDR_W),
      .I_DATA_W (eth_rx.DATA_WIDTH),
      .O_DATA_W (eth_rx.DATA_WIDTH),
      .USER_W   (1),
      .PIPELINE ("OUT")
    ) chdr_resize_v2e (
      .clk           (eth_rx.clk),
      .rst           (eth_rx.rst),
      .i_chdr_tdata  (data_fifo_o.tdata),
      .i_chdr_tuser  (1'b0),
      .i_chdr_tlast  (data_fifo_o.tlast),
      .i_chdr_tvalid (data_fifo_o.tvalid),
      .i_chdr_tready (data_fifo_o.tready),
      .o_chdr_tdata  (resize_v2e.tdata),
      .o_chdr_tuser  (),
      .o_chdr_tlast  (resize_v2e.tlast),
      .o_chdr_tvalid (resize_v2e.tvalid),
      .o_chdr_tready (resize_v2e.tready)
    );
  end else begin : gen_no_chdr_resize_v2e
    always_comb begin
      `AXI4S_ASSIGN(resize_v2e, data_fifo_o);
    end
  end

  // The lookup FIFO only takes the header routing info (tdest, tuser, epid).
  // We use axi_fifo_short since it can tolerate tvalid going low before a
  // transfer completes.

  always_comb lookup_fifo_i.tdata   = {m2e_tdest, m2e.tuser, chdr_get_dst_epid(m2e.tdata[63:0])};
  always_comb {lookup_fifo_tdest, lookup_fifo_tuser, lookup_fifo_tepid} = lookup_fifo_o.tdata;
  always_comb lookup_fifo_i.tvalid  = m2e.tvalid && m2e.tready && m2e_hdr;

  axi_fifo_short #(
    .WIDTH (1+USER_META_W+16)
  ) lookup_fifo (
    .clk      (eth_rx.clk),
    .reset    (eth_rx.rst),
    .clear    (1'b0),
    .i_tdata  (lookup_fifo_i.tdata),
    .i_tvalid (lookup_fifo_i.tvalid),
    .i_tready (lookup_fifo_i.tready),
    .o_tdata  (lookup_fifo_o.tdata),
    .o_tvalid (lookup_fifo_o.tvalid),
    .o_tready (lookup_fifo_o.tready),
    .space    (),
    .occupied ()
  );

  // Keep track of when we are busy doing a lookup in the KV map.
  logic lookup_busy = 1'b0;
  always_ff @(posedge eth_rx.clk) begin : lookup_busy_ff
    if (eth_rx.rst)
      lookup_busy <= 1'b0;
    else begin
      if (lookup_stb)
        lookup_busy <= 1'b1;
      else if (lookup_done_stb)
        lookup_busy <= 1'b0;
    end
  end

  // Determine if we can use the output of the lookup_fifo to do a KV map
  // lookup. We only perform a KV map lookup if tdest is 0 and we can only do
  // so if the KV map is free and the holding register for the tuser value is
  // available.
  always_comb lookup_epid = lookup_fifo_tepid;
  always_comb lookup_stb  = lookup_fifo_o.tvalid && !lookup_busy &&
                       !lookup_fifo_tdest   && !result_tuser_valid;

  // Determine if we can use the output of the lookup FIFO directly (no lookup
  // is needed). We can only use it if we're not already doing a KV lookup and
  // if the holding register for the tuser value is available.
  always_comb non_lookup_done_stb = lookup_fifo_o.tvalid && !lookup_busy &&
                               lookup_fifo_tdest    && !result_tuser_valid;

  // Pop the routing info off of the lookup_fifo if we've started its lookup
  always_comb lookup_fifo_o.tready = lookup_stb || non_lookup_done_stb;

  // Track when the next resize_v2e word is the start of a new packet
  always_ff @(posedge eth_rx.clk) begin : resize_v2e_hdr_ff
    if (eth_rx.rst)
      resize_v2e_hdr <= 1'b1;
    else if (resize_v2e.tvalid && resize_v2e.tready && pass_packet)
      resize_v2e_hdr <= resize_v2e.tlast;
  end

  // Store the lookup result in a holding register. This can come from the KV
  // map or the incoming tuser.
  always_ff @(posedge eth_rx.clk) begin : result_tuser_ff
    if (eth_rx.rst) begin
      result_tuser       <= {USER_META_W{1'bX}};    // Don't care
      result_tuser_valid <= 1'b0;
    end else begin
      // The tuser holding register becomes available as soon as we start
      // transmitting the corresponding packet.
      if (resize_v2e.tvalid && resize_v2e.tready && resize_v2e_hdr && pass_packet) begin
        result_tuser_valid <= 1'b0;
      end

      // Load the result of the lookup
      if (lookup_done_stb) begin
        result_tuser       <= lookup_result_match ? lookup_result_value : {USER_META_W{1'b0}};
        result_tuser_valid <= 1'b1;
      end else if (non_lookup_done_stb) begin
        result_tuser       <= lookup_fifo_tuser;
        result_tuser_valid <= 1'b1;
      end
    end
  end

  // Control when the packet from the data_fifo can be passed through. Put the
  // tuser value into a register for the duration of the packet.
  always_ff @(posedge eth_rx.clk) begin : pass_packet_ff
    if (eth_rx.rst) begin
      pass_packet <= 1'b0;
      reg_o_tuser <= {USER_META_W{1'bX}};    // Don't care
    end else begin
      // We're done passing through a packet when tlast goes out
      if (resize_v2e.tvalid && resize_v2e.tready && resize_v2e.tlast && pass_packet) begin
        pass_packet <= 1'b0;
      end

      // We can pass the next packet through when we're at the start of a
      // packet and we have the tuser value waiting in the holding register.
      if (resize_v2e_hdr && result_tuser_valid && !pass_packet) begin
        reg_o_tuser <= result_tuser;
        pass_packet <= 1'b1;
      end
    end
  end

  // Device addresses
  logic        au_raw_udp;
  logic [15:0] au_udp_dst;
  logic [31:0] au_ip_dst;
  logic [47:0] au_mac_dst;

  AxiStreamIf #(.DATA_WIDTH(eth_rx.DATA_WIDTH), .TKEEP(0), .TUSER(0))
    au (eth_rx.clk, eth_rx.rst);

  always_comb begin
    {au_raw_udp, au_udp_dst, au_ip_dst, au_mac_dst} = reg_o_tuser;
    au.tdata  = resize_v2e.tdata;
    au.tlast  = resize_v2e.tlast;
    au.tvalid = resize_v2e.tvalid & pass_packet;
    resize_v2e.tready  = au.tready & pass_packet;
  end

  //---------------------------------------------------------------------------
  // Optionally strip CHDR header
  //---------------------------------------------------------------------------

  AxiStreamIf #(.DATA_WIDTH(eth_rx.DATA_WIDTH), .USER_WIDTH(16), .TKEEP(0), .TUSER(1))
    stripped (eth_rx.clk, eth_rx.rst);

  if (EN_RX_RAW_PYLD) begin: gen_chdr_strip_header
    chdr_strip_header #(
      .CHDR_W(au.DATA_WIDTH)
    ) chdr_strip_header_i (
      .clk          (au.clk    ),
      .rst          (au.rst    ),
      .strip_en     (au_raw_udp),
      .s_chdr_tdata (au.tdata  ),
      .s_chdr_tlast (au.tlast  ),
      .s_chdr_tvalid(au.tvalid ),
      .s_chdr_tready(au.tready ),
      .m_tdata      (stripped.tdata ),
      .m_tuser      (stripped.tuser ),   // Packet length
      .m_tlast      (stripped.tlast ),
      .m_tvalid     (stripped.tvalid),
      .m_tready     (stripped.tready)
    );
  end else begin : gen_no_chdr_strip_header
    always_comb begin
      `AXI4S_ASSIGN(stripped, au);
    end
  end

  //---------------------------------------------------------------------------
  // Add UDP/IP/Eth header
  //---------------------------------------------------------------------------

  // Clock Crossing to the Ethernet clock domain
  logic [47:0] e_my_mac;
  logic [31:0] e_my_ip;
  logic [15:0] e_my_udp_chdr_port;

  // Crossing clock boundaries. my_mac, my_ip, my_udp_chdr_port must be written
  // prior to traffic, or an inconsistent version will exist for a clock period
  // or 2.
  synchronizer #(
    .WIDTH (96),
    .STAGES(1 )
  ) synchronizer_i (
    .clk(eth_rx.clk                             ),
    .rst(eth_rx.rst                             ),
    .in ({my_mac, my_ip, my_udp_chdr_port}      ),
    .out({e_my_mac, e_my_ip, e_my_udp_chdr_port})
  );

  // Add the headers before sending to eth_tx
  eth_ipv4_add_udp #(
    .PREAMBLE_BYTES  (PREAMBLE_BYTES  ),
    .MAX_PACKET_BYTES(MAX_PACKET_BYTES),
    .LENGTH_IN_TUSER (1)
  ) eth_ipv4_add_udp_i (
    .i      (stripped          ),
    .o      (eth_tx            ),
    .mac_src(e_my_mac          ),
    .ip_src (e_my_ip           ),
    .udp_src(e_my_udp_chdr_port),
    .mac_dst(au_mac_dst        ),
    .ip_dst (au_ip_dst         ),
    .udp_dst(au_udp_dst        )
  );

endmodule : chdr_xport_adapter


`default_nettype wire
