//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_xport_adapter_generic
// Description: A generic transport adapter module that can be used in
//   a variety of transports. It does the following:
//   - Exposes a configuration port for mgmt packets to configure the node
//   - Implements a return-address map for packets with metadata other than
//     the CHDR. Additional metadata can be passed as a tuser to this module
//     which will store it in a map indexed by the SrcEPID in a management
//     packet. For all returning packets, the metadata will be looked up in
//     the map and attached as the outgoing tuser.
//   - Implements a loopback path for node-info discovery
//   - Converts data stream to/from "RFNoC Network Order" (64-bit-Big-Endian)
//
// Parameters:
//   - PROTOVER: RFNoC protocol version {8'd<major>, 8'd<minor>}
//   - CHDR_W: Width of the CHDR bus in bits
//   - USER_W: Width of the tuser bus in bits
//   - TBL_SIZE: Log2 of the depth of the routing table
//   - NODE_SUBTYPE: The node subtype to return for a node-info discovery
//   - NODE_INST: The node type to return for a node-info discovery
//   - ALLOW_DISC: Controls if the external transport network should be
//                 discoverable by management packets from RFNoC side.
//
// Signals:
//   - device_id     : The ID of the device that has instantiated this module
//   - s_axis_xport_*: The input CHDR stream from the transport (plus tuser metadata)
//   - m_axis_xport_*: The output CHDR stream to transport (plus tuser metadata)
//   - s_axis_rfnoc_*: The input CHDR stream from the rfnoc infrastructure
//   - m_axis_rfnoc_*: The output CHDR stream to the rfnoc infrastructure
//   - ctrlport_*    : The ctrlport interface for the configuration port
//

module chdr_xport_adapter_generic #(
  parameter [15:0] PROTOVER     = {8'd1, 8'd0},
  parameter        CHDR_W       = 256,
  parameter        USER_W       = 16,
  parameter        TBL_SIZE     = 6,
  parameter [7:0]  NODE_SUBTYPE = 8'd0,
  parameter        NODE_INST    = 0,
  parameter        ALLOW_DISC   = 1
)(
  // Clock and reset
  input  wire               clk,
  input  wire               rst,
  // Device info
  input  wire [15:0]        device_id,
  // Transport stream in (AXI-Stream)
  input  wire [CHDR_W-1:0]  s_axis_xport_tdata,
  input  wire [USER_W-1:0]  s_axis_xport_tuser,
  input  wire               s_axis_xport_tlast,
  input  wire               s_axis_xport_tvalid,
  output wire               s_axis_xport_tready,
  // Transport stream out (AXI-Stream)
  output wire [CHDR_W-1:0]  m_axis_xport_tdata,
  output wire [USER_W-1:0]  m_axis_xport_tuser,
  output wire               m_axis_xport_tlast,
  output wire               m_axis_xport_tvalid,
  input  wire               m_axis_xport_tready,
  // RFNoC stream in (AXI-Stream)
  input  wire [CHDR_W-1:0]  s_axis_rfnoc_tdata,
  input  wire               s_axis_rfnoc_tlast,
  input  wire               s_axis_rfnoc_tvalid,
  output wire               s_axis_rfnoc_tready,
  // RFNoC stream out (AXI-Stream)
  output wire [CHDR_W-1:0]  m_axis_rfnoc_tdata,
  output wire               m_axis_rfnoc_tlast,
  output wire               m_axis_rfnoc_tvalid,
  input  wire               m_axis_rfnoc_tready,
  // Control port endpoint
  output wire               ctrlport_req_wr,
  output wire               ctrlport_req_rd,
  output wire [15:0]        ctrlport_req_addr,
  output wire [31:0]        ctrlport_req_data,
  input  wire               ctrlport_resp_ack,
  input  wire [31:0]        ctrlport_resp_data
);

  // ---------------------------------------------------
  // RFNoC Includes
  // ---------------------------------------------------
  `include "../core/rfnoc_chdr_utils.vh"
  `include "../core/rfnoc_chdr_internal_utils.vh"

  // ---------------------------------------------------
  // Reverse groups of 64-bit words to translate
  // stream to "RFNoC Network Order" i.e. Big-Endian
  // in groups of 8 bytes
  // ---------------------------------------------------
  wire [CHDR_W-1:0]  i_xport_tdata;
  wire [USER_W-1:0]  i_xport_tuser;
  wire               i_xport_tlast, i_xport_tvalid, i_xport_tready;
  wire [CHDR_W-1:0]  o_xport_tdata;
  wire [USER_W-1:0]  o_xport_tuser;
  wire               o_xport_tlast, o_xport_tvalid, o_xport_tready;

  localparam [$clog2(CHDR_W)-1:0] SWAP_LANES = ((CHDR_W / 64) - 1) << 6;

  axis_data_swap #(
    .DATA_W(CHDR_W), .USER_W(USER_W), .STAGES_EN(SWAP_LANES), .DYNAMIC(0)
  ) xport_in_swap_i (
    .clk(clk), .rst(rst),
    .s_axis_tdata(s_axis_xport_tdata), .s_axis_tswap({$clog2(CHDR_W)-1{1'b0}}),
    .s_axis_tuser(s_axis_xport_tuser), .s_axis_tlast(s_axis_xport_tlast),
    .s_axis_tvalid(s_axis_xport_tvalid), .s_axis_tready(s_axis_xport_tready),
    .m_axis_tdata (i_xport_tdata), .m_axis_tuser(i_xport_tuser),
    .m_axis_tlast (i_xport_tlast),
    .m_axis_tvalid(i_xport_tvalid), .m_axis_tready(i_xport_tready)
  );

  axis_data_swap #(
    .DATA_W(CHDR_W), .USER_W(USER_W), .STAGES_EN(SWAP_LANES), .DYNAMIC(0)
  ) xport_out_swap_i (
    .clk(clk), .rst(rst),
    .s_axis_tdata(o_xport_tdata), .s_axis_tswap({$clog2(CHDR_W)-1{1'b0}}),
    .s_axis_tuser(o_xport_tuser), .s_axis_tlast(o_xport_tlast),
    .s_axis_tvalid(o_xport_tvalid), .s_axis_tready(o_xport_tready),
    .m_axis_tdata (m_axis_xport_tdata), .m_axis_tuser (m_axis_xport_tuser),
    .m_axis_tlast (m_axis_xport_tlast),
    .m_axis_tvalid(m_axis_xport_tvalid), .m_axis_tready(m_axis_xport_tready)
  );


  wire [CHDR_W-1:0] x2d_tdata;  // Xport => Demux
  reg  [USER_W-1:0] x2d_tuser;
  wire [1:0]        x2d_tid;
  wire              x2d_tlast, x2d_tvalid, x2d_tready;
  wire [CHDR_W-1:0] x2x_tdata;  // Xport => Xport (loopback)
  wire [USER_W-1:0] x2x_tuser;
  wire              x2x_tlast, x2x_tvalid, x2x_tready;
  wire [CHDR_W-1:0] m2x_tdata;  // Mux => Xport
  wire              m2x_tdest;  // 1: Return to src, 0: CHDR input
  wire [USER_W-1:0] m2x_tuser;
  wire              m2x_tlast, m2x_tvalid, m2x_tready;

  // ---------------------------------------------------
  // Transport => DEMUX
  // ---------------------------------------------------
  wire              op_stb;
  wire [15:0]       op_src_epid;
  wire [USER_W-1:0] op_data;
  wire              lookup_stb, lookup_done_stb, lookup_result_match;
  wire [15:0]       lookup_epid;
  wire [USER_W-1:0] lookup_result_value;
  wire [47:0]       node_info;

  assign node_info = chdr_mgmt_build_node_info({ 10'h0, NODE_SUBTYPE}, 
                       NODE_INST, NODE_TYPE_TRANSPORT, device_id);

  chdr_mgmt_pkt_handler #(
    .PROTOVER(PROTOVER), .CHDR_W(CHDR_W), .USER_W(USER_W), .MGMT_ONLY(0)
  ) mgmt_ep_i (
    .clk(clk), .rst(rst),
    .node_info(node_info),
    .s_axis_chdr_tdata(i_xport_tdata), .s_axis_chdr_tlast(i_xport_tlast),
    .s_axis_chdr_tvalid(i_xport_tvalid), .s_axis_chdr_tready(i_xport_tready),
    .s_axis_chdr_tuser(i_xport_tuser),
    .m_axis_chdr_tdata(x2d_tdata), .m_axis_chdr_tlast(x2d_tlast),
    .m_axis_chdr_tdest(/* unused */), .m_axis_chdr_tid(x2d_tid),
    .m_axis_chdr_tvalid(x2d_tvalid), .m_axis_chdr_tready(x2d_tready),
    .ctrlport_req_wr(ctrlport_req_wr), .ctrlport_req_rd(ctrlport_req_rd),
    .ctrlport_req_addr(ctrlport_req_addr), .ctrlport_req_data(ctrlport_req_data),
    .ctrlport_resp_ack(ctrlport_resp_ack), .ctrlport_resp_data(ctrlport_resp_data),
    .op_stb(op_stb), .op_dst_epid(/* unused */), .op_src_epid(op_src_epid), .op_data(op_data)
  );

  kv_map #(
    .KEY_WIDTH(16), .VAL_WIDTH(USER_W), .SIZE(TBL_SIZE)
  ) kv_map_i (
    .clk(clk), .reset(rst),
    .insert_stb(op_stb), .insert_key(op_src_epid), .insert_val(op_data),
    .insert_busy(/* Time between op_stb > Insertion time */),
    .find_key_stb(lookup_stb), .find_key(lookup_epid),
    .find_res_stb(lookup_done_stb),
    .find_res_match(lookup_result_match), .find_res_val(lookup_result_value),
    .count(/* unused */)
  );

  reg i_xport_hdr = 1'b1;
  always @(posedge clk) begin
    if (rst)
      i_xport_hdr <= 1'b1;
    else if (i_xport_tvalid && i_xport_tready)
      i_xport_hdr <= i_xport_tlast;
  end

  // chdr_mgmt_pkt_handler does not buffer packets and has at least one cycle of delay
  // TODO: The tuser caching logic could be more robust
  always @(posedge clk) begin
    if (i_xport_tvalid && i_xport_tready && i_xport_hdr)
      x2d_tuser <= i_xport_tuser;
  end

  // ---------------------------------------------------
  // Optional management filter
  // ---------------------------------------------------

  wire [CHDR_W-1:0] f2m_tdata;
  wire              f2m_tlast;
  wire              f2m_tvalid;
  wire              f2m_tready;

  if (ALLOW_DISC) begin : gen_no_mgmt_filter
    // Allow all packets to pass through
    assign f2m_tdata           = s_axis_rfnoc_tdata;
    assign f2m_tlast           = s_axis_rfnoc_tlast;
    assign f2m_tvalid          = s_axis_rfnoc_tvalid;
    assign s_axis_rfnoc_tready = f2m_tready;

  end else begin : gen_mgmt_filter
    // Disallow forwarding of management discovery packets from RFNoC to the
    // transport interface for transports that don't support them.
    //vhook_nowarn unused_*
    wire [CHDR_W-1:0] unused_tdata;
    wire              unused_tlast, unused_tvalid, unused_tready;
    wire [CHDR_W-1:0] s_header;
    wire              dispose_pkt;

    // We identify discovery packets by the fact that they are management
    // packets and that they use the null EPID as the destination.
    assign dispose_pkt = (chdr_get_pkt_type(s_header[63:0]) == CHDR_PKT_TYPE_MGMT) && 
                         (chdr_get_dst_epid(s_header[63:0]) == NULL_EPID);

    axi_demux #(
      .WIDTH          (CHDR_W),
      .SIZE           (2),
      .PRE_FIFO_SIZE  (0),
      .POST_FIFO_SIZE (1)
    ) axi_demux_mgmt_filter_i (
      .clk      (clk),
      .reset    (rst),
      .clear    (1'b0),
      .header   (s_header),
      .dest     (dispose_pkt),
      .i_tdata  (s_axis_rfnoc_tdata),
      .i_tlast  (s_axis_rfnoc_tlast),
      .i_tvalid (s_axis_rfnoc_tvalid),
      .i_tready (s_axis_rfnoc_tready),
      .o_tdata  ({unused_tdata, f2m_tdata}),
      .o_tlast  ({unused_tlast, f2m_tlast}),
      .o_tvalid ({unused_tvalid, f2m_tvalid}),
      .o_tready ({1'b1, f2m_tready})
    );
  end

  // ---------------------------------------------------
  // MUX and DEMUX for return path
  // ---------------------------------------------------

  wire [USER_W-1:0] unused_tuser;
  axis_switch #(
    .DATA_W(CHDR_W+USER_W), .DEST_W(1), .IN_PORTS(1), .OUT_PORTS(2), .PIPELINE(0)
  ) rtn_demux_i (
    .clk(clk), .reset(rst),
    .s_axis_tdata({x2d_tuser, x2d_tdata}), .s_axis_alloc(1'b0),
    .s_axis_tdest(x2d_tid == CHDR_MGMT_RETURN_TO_SRC ? 2'b01 : 2'b00), 
    .s_axis_tlast(x2d_tlast), .s_axis_tvalid(x2d_tvalid), .s_axis_tready(x2d_tready),
    .m_axis_tdata({x2x_tuser, x2x_tdata, unused_tuser, m_axis_rfnoc_tdata}),
    .m_axis_tdest(/* unused */),
    .m_axis_tlast({x2x_tlast, m_axis_rfnoc_tlast}),
    .m_axis_tvalid({x2x_tvalid, m_axis_rfnoc_tvalid}),
    .m_axis_tready({x2x_tready, m_axis_rfnoc_tready})
  );

  axi_mux #(
    .WIDTH(CHDR_W+USER_W+1), .SIZE(2), .PRE_FIFO_SIZE(0), .POST_FIFO_SIZE(0)
  ) rtn_mux_i (
    .clk(clk), .reset(rst), .clear(1'b0),
    .i_tdata({1'b1, x2x_tuser, x2x_tdata, 1'b0, {USER_W{1'b0}}, f2m_tdata}),
    .i_tlast({x2x_tlast, f2m_tlast}),
    .i_tvalid({x2x_tvalid, f2m_tvalid}), .i_tready({x2x_tready, f2m_tready}),
    .o_tdata({m2x_tdest, m2x_tuser, m2x_tdata}), .o_tlast(m2x_tlast),
    .o_tvalid(m2x_tvalid), .o_tready(m2x_tready)
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

  wire              data_fifo_i_tready;
  wire [CHDR_W-1:0] data_fifo_o_tdata;
  wire              data_fifo_o_tlast;
  wire              data_fifo_o_tvalid;
  wire              data_fifo_o_tready;
  wire              lookup_fifo_i_tready;
  wire              lookup_fifo_tdest;
  wire [USER_W-1:0] lookup_fifo_tuser;
  wire [      15:0] lookup_fifo_tepid;
  wire              lookup_fifo_o_tvalid;
  wire              lookup_fifo_o_tready;

  wire             non_lookup_done_stb;
  reg              data_fifo_o_hdr = 1'b1;
  reg              pass_packet;
  reg [USER_W-1:0] result_tuser;
  reg              result_tuser_valid;
  reg [USER_W-1:0] reg_o_tuser;


  // Track when the next m2x word contains is the start of a new packet
  reg m2x_hdr = 1'b1;
  always @(posedge clk) begin
    if (rst)
      m2x_hdr <= 1'b1;
    else if (m2x_tvalid && m2x_tready)
      m2x_hdr <= m2x_tlast;
  end

  // We can only accept data from the mux when when both the data_fifo and
  // lookup_fifo are ready.
  assign m2x_tready = data_fifo_i_tready && lookup_fifo_i_tready;

  // The data_fifo only takes the packet data (tdata, tlast). We use an
  // axi_fifo_short module for the data_fifo because it can tolerate tvalid
  // going low before a transfer completes.
  axi_fifo_short #(
    .WIDTH (1+CHDR_W)
  ) data_fifo (
    .clk      (clk),
    .reset    (rst),
    .clear    (1'b0),
    .i_tdata  ({m2x_tlast, m2x_tdata}),
    .i_tvalid (m2x_tvalid && m2x_tready),
    .i_tready (data_fifo_i_tready),
    .o_tdata  ({data_fifo_o_tlast, data_fifo_o_tdata}),
    .o_tvalid (data_fifo_o_tvalid),
    .o_tready (data_fifo_o_tready),
    .space    (),
    .occupied ()
  );

  // The lookup FIFO only takes the header routing info (tdest, tuser, epid).
  // We use axi_fifo_short since it can tolerate tvalid going low before a
  // transfer completes.
  axi_fifo_short #(
    .WIDTH (1+USER_W+16)
  ) lookup_fifo (
    .clk      (clk),
    .reset    (rst),
    .clear    (1'b0),
    .i_tdata  ({m2x_tdest, m2x_tuser, chdr_get_dst_epid(m2x_tdata[63:0])}),
    .i_tvalid (m2x_tvalid && m2x_tready && m2x_hdr),
    .i_tready (lookup_fifo_i_tready),
    .o_tdata  ({lookup_fifo_tdest, lookup_fifo_tuser, lookup_fifo_tepid}),
    .o_tvalid (lookup_fifo_o_tvalid),
    .o_tready (lookup_fifo_o_tready),
    .space    (),
    .occupied ()
  );

  // Keep track of when we are busy doing a lookup in the KV map.
  reg lookup_busy = 1'b0;
  always @(posedge clk) begin
    if (rst)
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
  assign lookup_epid = lookup_fifo_tepid;
  assign lookup_stb  = lookup_fifo_o_tvalid && !lookup_busy &&
                       !lookup_fifo_tdest   && !result_tuser_valid;

  // Determine if we can use the output of the lookup FIFO directly (no lookup
  // is needed). We can only use it if we're not already doing a KV lookup and
  // if the holding register for the tuser value is available.
  assign non_lookup_done_stb = lookup_fifo_o_tvalid && !lookup_busy &&
                               lookup_fifo_tdest    && !result_tuser_valid;

  // Pop the routing info off of the lookup_fifo if we've started its lookup
  assign lookup_fifo_o_tready = lookup_stb || non_lookup_done_stb;

  // Track when the next data_fifo_o word is the start of a new packet
  always @(posedge clk) begin
    if (rst)
      data_fifo_o_hdr <= 1'b1;
    else if (data_fifo_o_tvalid && data_fifo_o_tready && pass_packet)
      data_fifo_o_hdr <= data_fifo_o_tlast;
  end

  // Store the lookup result in a holding register. This can come from the KV
  // map or the incoming tuser.
  always @(posedge clk) begin
    if (rst) begin
      result_tuser       <= {USER_W{1'bX}};    // Don't care
      result_tuser_valid <= 1'b0;
    end else begin
      // The tuser holding register becomes available as soon as we start
      // transmitting the corresponding packet.
      if (data_fifo_o_tvalid && data_fifo_o_tready && data_fifo_o_hdr && pass_packet) begin
        result_tuser_valid <= 1'b0;
      end

      // Load the result of the lookup
      if (lookup_done_stb) begin
        result_tuser       <= lookup_result_match ? lookup_result_value : {USER_W{1'b0}};
        result_tuser_valid <= 1'b1;
      end else if (non_lookup_done_stb) begin
        result_tuser       <= lookup_fifo_tuser;
        result_tuser_valid <= 1'b1;
      end
    end
  end

  // Control when the packet from the data_fifo can be passed through. Put the
  // tuser value into a register for the duration of the packet.
  always @(posedge clk) begin
    if (rst) begin
      pass_packet <= 1'b0;
      reg_o_tuser <= {USER_W{1'bX}};    // Don't care
    end else begin
      // We're done passing through a packet when tlast goes out
      if (data_fifo_o_tvalid && data_fifo_o_tready && data_fifo_o_tlast && pass_packet) begin
        pass_packet <= 1'b0;
      end

      // We can pass the next packet through when we're at the start of a
      // packet and we have the tuser value waiting in the holding register.
      if (data_fifo_o_hdr && result_tuser_valid && !pass_packet) begin
        reg_o_tuser <= result_tuser;
        pass_packet <= 1'b1;
      end
    end
  end

  assign o_xport_tdata      = data_fifo_o_tdata;
  assign o_xport_tuser      = reg_o_tuser;
  assign o_xport_tlast      = data_fifo_o_tlast;
  assign o_xport_tvalid     = data_fifo_o_tvalid & pass_packet;
  assign data_fifo_o_tready = o_xport_tready & pass_packet;

endmodule // chdr_xport_adapter_generic