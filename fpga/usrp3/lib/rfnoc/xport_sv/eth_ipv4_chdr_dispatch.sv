//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: eth_ipv4_chdr_dispatch
//
// Description:
//  This module serves as an Ethernet endpoint for CHDR traffic.
//  Ethernet frames arrive on the eth_rx port where they are
//  inspected and classified as CHDR or !CHDR. A frame contains
//  CHDR payload if it is addressed to us (Eth and IP), is a UDP
//  packet and the destination port is one of the CHDR ports.
//  The UDP payload for CHDR frame is sent out of the e2v
//  in addition to source information for Eth, IP and UDP. All
//  other traffic address to us (Eth) is sent to the e2c port.
//  Traffic not addressed (Eth) to us is dropped(optionally).
//
// Parameters:
//  - CPU_FIFO_SIZE: Log2 of the FIFO depth (in bytes) for the CPU egress path
//  - CHDR_FIFO_SIZE: Log2 of the FIFO depth (in bytes) for the CHDR egress path
//  - PREAMBLE_BYTES: Number of bytes of preamble on Ethernet interface
//  - CPU_PREAMBLE:   Set to 1 to use PREAMBLE_BYTES on CPU interface (for ZPU)
//                    or set to 0 to remove preamble on CPU interface (for ARM
//                    CPU).
//  - DROP_UNKNOWN_MAC: Drop packets not addressed to us?
//  - DROP_MIN_PACKET: Drop packets smaller than 64 bytes?
//  - ENET_W: Width of AXI bus going to Ethernet Mac
//
// Signals:
//   - eth_rx : The input Ethernet stream from the MAC
//              tUser={error,trailing bytes}
//   - e2v : The output CHDR stream to the rfnoc infrastructure
//   - e2c : The output Ethernet stream to the CPU
//
//   - my_mac           : The Ethernet (MAC) address of this endpoint
//   - my_ip            : The IPv4 address of this endpoint
//   - my_udp_chdr_port : The UDP port allocated for CHDR traffic on this endpoint
//
//   - my pause set   : number of word of fullness on CHDR_FIFO before requesting a pause
//   - my pause clear : number of word of fullness on CHDR_FIFO before clearing a pause request
//

module eth_ipv4_chdr_dispatch #(
  int CPU_FIFO_SIZE    = $clog2(8*1024),
  int CHDR_FIFO_SIZE   = $clog2(8*1024),
  int PREAMBLE_BYTES   = 6,
  int CPU_PREAMBLE     = 0,
  int MAX_PACKET_BYTES = 2**16-1,
  bit DROP_UNKNOWN_MAC = 0,
  bit DROP_MIN_PACKET  = 0,
  int ENET_W           = 64
)(
  // Clock domain: eth_rx.clk (other interface clocks are unused)

  // AXI-Stream interfaces
  output logic        eth_pause_req,
  AxiStreamIf.slave   eth_rx, // tUser={error,trailing bytes};
  AxiStreamIf.master  e2v,    // tUser={1'b0,trailing bytes};
  AxiStreamIf.master  e2c,    // tUser={1'b0,trailing bytes};

  // Device addresses
  input  logic [47:0] my_mac,
  input  logic [31:0] my_ip,
  input  logic [15:0] my_udp_chdr_port,
  // Pause control
  input  logic [15:0] my_pause_set,
  input  logic [15:0] my_pause_clear,

  output logic        chdr_dropped,
  output logic        cpu_dropped
);

  // Clock Crossing to the ethernet clock domain
  logic [47:0] e_my_mac;
  logic [31:0] e_my_ip;
  logic [15:0] e_my_udp_chdr_port;
  logic [15:0] e_pause_set;
  logic [15:0] e_pause_clear;
  // crossing clock boundaries.
  // my_mac, my_ip,,my_udp_chdr_port must be written
  // prior to traffic, or an inconsistent version will
  // exist for a clock period or 2.  This would be better
  // done with a full handshake.
  synchronizer #(.WIDTH(96+32),.STAGES(1))
    e_info_sync (.clk(eth_rx.clk),.rst(eth_rx.rst),
                 .in({my_mac,my_ip,my_udp_chdr_port,my_pause_set,my_pause_clear}),
                 .out({e_my_mac,e_my_ip,e_my_udp_chdr_port,e_pause_set,e_pause_clear}));


  localparam ENET_USER_W = $clog2(ENET_W/8)+1;
  //---------------------------------------
  // Include for byte positions
  //---------------------------------------
  `include "eth_constants.vh"
  // example macro to handle interface assignment
  `define AXI4S_ASSIGN(O,I) \
    ``O.tdata = ``I.tdata;\
    ``O.tuser = ``I.tuser;\
    ``O.tlast = ``I.tlast;\
    ``O.tvalid = ``I.tvalid;\
    ``I.tready = ``O.tready;

  // axi_remov_bytes (PREAMBLE Strip)
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),
                .TKEEP(0),.MAX_PACKET_BYTES(MAX_PACKET_BYTES))
    inp(eth_rx.clk,eth_rx.rst);
  //   tUser = {error,trailing bytes};
  AxiStreamPacketIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),
                .TKEEP(0),.MAX_PACKET_BYTES(MAX_PACKET_BYTES))
    in0(eth_rx.clk,eth_rx.rst);
  // in_regs
  //   tUser = {error,trailing bytes};
  AxiStreamPacketIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),
                .TKEEP(0),.MAX_PACKET_BYTES(MAX_PACKET_BYTES))
    in1(eth_rx.clk,eth_rx.rst);
  AxiStreamPacketIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),
                .TKEEP(0),.MAX_PACKET_BYTES(MAX_PACKET_BYTES))
    in2(eth_rx.clk,eth_rx.rst);
  // STATEMACHINE
  //   tUser = {error,trailing bytes};
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0))
    in3(eth_rx.clk,eth_rx.rst);

  // CPU_BRANCH
  //   tUser = {error,trailing bytes};
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0))
    cpu0(eth_rx.clk,eth_rx.rst);
  // cpu_out_gate - throw away error packets
  //   tUser = {error,trailing bytes};
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0))
    cpu1(eth_rx.clk,eth_rx.rst);
  // cpu_out_fifo
  // e2c (OUTPUT)

  // CHDR_Branch
  //   tUser = {not used};
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.TKEEP(0),.TUSER(0))
    chdr0(eth_rx.clk,eth_rx.rst);
  //   tUser = {not used};
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.TKEEP(0),.TUSER(0))
    chdr1(eth_rx.clk,eth_rx.rst);
  //   tUser = {not used};
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.TKEEP(0),.TUSER(0),.MAX_PACKET_BYTES(MAX_PACKET_BYTES))
    chdr2(eth_rx.clk,eth_rx.rst);
  // chdr_out_fifo
  // e2v(OUTPUT)

  //---------------------------------------
  // Strip Preamble Bytes
  //---------------------------------------
  if (PREAMBLE_BYTES > 0 && !CPU_PREAMBLE) begin : gen_strip_preamble
    // Keep the preamble since the CPU expects it.
    always_comb begin
      `AXI4S_ASSIGN(inp,eth_rx);
    end
    axi4s_remove_bytes #(.REM_START(0),.REM_END(PREAMBLE_BYTES-1)
    ) strip_preamble (
      .i(inp),.o(in0)
    );
  end else begin : gen_no_strip_preamble
    // Remove the preamble since CHDR and CPU don't expect it.
    always_comb begin
      `AXI4S_ASSIGN(in0,eth_rx);
    end
  end

  //---------------------------------------
  // Input pipeline stages
  //---------------------------------------
  axi4s_fifo #(
    .SIZE(1)
  ) in_reg_i (
    .clear(1'b0),.space(),.occupied(),
    .i(in0), .o(in1)
  );

  axi4s_fifo #(
    .SIZE(1)
  ) in_reg2_i (
    .clear(1'b0),.space(),.occupied(),
    .i(in1), .o(in2)
  );

  //---------------------------------------
  // Classification state machine
  //---------------------------------------
   typedef enum logic [2:0] {
      ST_IDLE_ETH_L0    = 3'd0,
      ST_FWD_CHDR       = 3'd1,
      ST_FWD_CPU        = 3'd2,
      ST_DROP_TERM      = 3'd3,
      ST_DROP_WAIT      = 3'd4
   } dispatch_state_t;

  // State info
  dispatch_state_t dispatch_state,next_dispatch_state  = ST_IDLE_ETH_L0;
  logic cpu_error = 1'b0;
  logic chdr_error = 1'b0;
  logic mac_error;
  logic min_packet_error;
  logic reached_min_packet;
  logic cpu_push_error;
  logic chdr_push_error;

  // Cached fields
  logic [47:0] eth_dst_addr_new,  eth_src_addr_new;
  logic [31:0] ipv4_dst_addr_new, ipv4_src_addr_new;
  logic [15:0] udp_dst_port_new,  udp_src_port_new, eth_type_new;
  logic [7:0]  ip_protocol_new, ip_version_new;
  logic [47:0] eth_dst_addr_old,  eth_src_addr_old;
  logic [31:0] ipv4_dst_addr_old, ipv4_src_addr_old;
  logic [15:0] udp_dst_port_old,  udp_src_port_old, eth_type_old;
  logic [7:0]  ip_protocol_old, ip_version_old;
  logic        reached_min_packet_new, reached_min_packet_old;
  logic        reached_end_of_udp;

  logic        eth_dst_is_broadcast;
  logic        eth_dst_is_me;
  logic        udp_dst_is_me;
  logic        ipv4_dst_is_me;
  logic        ipv4_protocol_is_udp;
  logic        eth_type_is_ipv4;

  // save the fields
  always_ff @(posedge eth_rx.clk) begin : field_ff
    if (eth_rx.rst) begin

      eth_dst_addr_old  <= '0;
      eth_src_addr_old  <= '0;
      ip_protocol_old   <= '0;
      ip_version_old    <= '0;
      ipv4_src_addr_old <= '0;
      ipv4_dst_addr_old <= '0;
      udp_src_port_old  <= '0;
      udp_dst_port_old  <= '0;
      eth_type_old      <= '0;
      reached_min_packet_old <= 1'b0;

      // Statemachine Decisions
      eth_dst_is_broadcast <= 1'b0;
      eth_dst_is_me        <= 1'b0;
      udp_dst_is_me        <= 1'b0;
      ipv4_dst_is_me       <= 1'b0;
      ipv4_protocol_is_udp <= 1'b0;
      eth_type_is_ipv4     <= 1'b0;
    end else begin
      eth_dst_addr_old  <= eth_dst_addr_new;
      eth_src_addr_old  <= eth_src_addr_new;
      ip_protocol_old   <= ip_protocol_new;
      ip_version_old    <= ip_version_new;
      ipv4_src_addr_old <= ipv4_src_addr_new;
      ipv4_dst_addr_old <= ipv4_dst_addr_new;
      udp_src_port_old  <= udp_src_port_new;
      udp_dst_port_old  <= udp_dst_port_new;
      eth_type_old      <= eth_type_new;

      if (in1.tvalid && in1.tready) begin
        eth_dst_is_broadcast <= eth_dst_addr_old == ETH_ADDR_BCAST;
        eth_dst_is_me        <= eth_dst_addr_old == e_my_mac;
        udp_dst_is_me        <= udp_dst_port_old == e_my_udp_chdr_port;
        ipv4_dst_is_me       <= ipv4_dst_addr_old == e_my_ip;
        ipv4_protocol_is_udp <= ip_protocol_old == IPV4_PROTO_UDP;
        eth_type_is_ipv4     <= eth_type_old == ETH_TYPE_IPV4;
      end
    end
  end

  // Get the fields - don't use assign. assign will not activate with changes
  // to in0.
  localparam int OFFSET = CPU_PREAMBLE ? PREAMBLE_BYTES : 0;
  always_comb begin  : get_fields
    eth_dst_addr_new  = in0.get_packet_field48(eth_dst_addr_old,  OFFSET+DST_MAC_BYTE,  .NETWORK_ORDER(1));
    eth_src_addr_new  = in0.get_packet_field48(eth_src_addr_old,  OFFSET+SRC_MAC_BYTE,  .NETWORK_ORDER(1));
    ip_version_new    = in0.get_packet_byte   (ip_version_old,    OFFSET+IP_VERSION_BYTE);
    ip_protocol_new   = in0.get_packet_byte   (ip_protocol_old,   OFFSET+PROTOCOL_BYTE  );
    ipv4_src_addr_new = in0.get_packet_field32(ipv4_src_addr_old, OFFSET+SRC_IP_BYTE,   .NETWORK_ORDER(1));
    ipv4_dst_addr_new = in0.get_packet_field32(ipv4_dst_addr_old, OFFSET+DST_IP_BYTE,   .NETWORK_ORDER(1));
    udp_src_port_new  = in0.get_packet_field16(udp_src_port_old,  OFFSET+SRC_PORT_BYTE, .NETWORK_ORDER(1));
    udp_dst_port_new  = in0.get_packet_field16(udp_dst_port_old,  OFFSET+DST_PORT_BYTE, .NETWORK_ORDER(1));
    eth_type_new      = in0.get_packet_field16(eth_type_old,      OFFSET+ETH_TYPE_BYTE, .NETWORK_ORDER(1));
  end

  always_ff @(posedge eth_rx.clk) begin  : reached_bytes
    reached_min_packet_new = in1.reached_packet_byte(OFFSET+MIN_PACKET_SIZE_BYTE);
    reached_end_of_udp     = in1.reached_packet_byte(OFFSET+DST_PORT_BYTE+3);// we have enough to decide
  end

  // calculate error conditions
  assign mac_error          = in2.tuser[ERROR_BIT] && in2.tvalid;
  assign cpu_push_error     = (in3.tvalid && !cpu0.tready);
  assign chdr_push_error    = (in3.tvalid && !chdr0.tready);

  if (DROP_MIN_PACKET) begin
    assign reached_min_packet = (reached_min_packet_new && in2.tuser[BYTES_MSB:0] ==0) || reached_min_packet_old;
    assign min_packet_error   = (in2.tlast && !reached_min_packet);
  end else begin
    assign reached_min_packet = 1'b1;
    assign min_packet_error   = 1'b0;
  end

  always_ff @(posedge eth_rx.clk) begin : dispatch_sm_ff
    if (eth_rx.rst) begin
      dispatch_state  <= ST_IDLE_ETH_L0;
    end else begin
      if (in2.tvalid && in2.tready) begin
        if (in2.tlast)
          dispatch_state <= ST_IDLE_ETH_L0;
        else
         dispatch_state <= next_dispatch_state;
      end
    end
  end

  always_comb begin : dispatch_sm_next_state
      //defaults
      next_dispatch_state = dispatch_state;
      `AXI4S_ASSIGN(in3,in2);
      cpu_error   = 1'b0;
      chdr_error  = 1'b0;
      in2.tready  = 1'b1; // never hold off

      // Statemachine always returns to ST_IDLE_ETH_L0 when tlast is set
      case (dispatch_state)
        ST_IDLE_ETH_L0: begin
          cpu_error   = 1'b0;
          chdr_error  = 1'b0;
          if (mac_error || min_packet_error || cpu_push_error || chdr_push_error) begin
             cpu_error  = 1'b1;
             chdr_error = 1'b1;
             next_dispatch_state = ST_DROP_TERM;
          end else if (reached_end_of_udp) begin
            // all header values are decoded
            if (eth_dst_is_broadcast) begin
              // If Eth destination is bcast then fwd to CPU
              cpu_error  = 1'b0;
              chdr_error = 1'b1;
              next_dispatch_state = ST_FWD_CPU;
            end else if (!eth_dst_is_me && DROP_UNKNOWN_MAC) begin
              // If Eth destination is not us then drop the packet
              cpu_error  = 1'b1;
              chdr_error = 1'b1;
              next_dispatch_state = ST_DROP_TERM;
            end else if (udp_dst_is_me &&
                         ipv4_dst_is_me &&
                         //ip_version_new    == IPV4_LEN5 && // NEW CHECK --verify if this is ok
                         ipv4_protocol_is_udp &&
                         eth_type_is_ipv4) begin
              // The conditions matches CHDR port
              cpu_error  = 1'b1;
              chdr_error = 1'b0;
              next_dispatch_state = ST_FWD_CHDR;
            end else begin
              // Not the CHDR port. Forward to CPU
              cpu_error  = 1'b0;
              chdr_error = 1'b1;
              next_dispatch_state = ST_FWD_CPU;
            end
          end
        end

        // CHDR Payload
        ST_FWD_CHDR: begin
          cpu_error   = 1'b1;
          chdr_error  = 1'b0;
          if (mac_error || min_packet_error || chdr_push_error) begin
             cpu_error  = 1'b1;
             chdr_error = 1'b1;
             next_dispatch_state = ST_DROP_TERM;
          end
        end

        // NotCHDR Payload: Send to CPU
        ST_FWD_CPU: begin
          cpu_error   = 1'b0;
          chdr_error  = 1'b1;
          if (mac_error || min_packet_error || cpu_push_error) begin
             cpu_error  = 1'b1;
             chdr_error = 1'b1;
             next_dispatch_state = ST_DROP_TERM;
          end
        end

        // Unwanted Payload: Drop
        ST_DROP_TERM: begin
          cpu_error   = 1'b1;
          chdr_error  = 1'b1;
          in3.tlast   = 1'b1;
          in3.tvalid  = 1'b1;
          next_dispatch_state = ST_DROP_WAIT;
        end

        // Unwanted Payload: wait
        ST_DROP_WAIT: begin
          cpu_error   = 1'b0;
          chdr_error  = 1'b0;
          in3.tlast   = 1'b0;
          in3.tvalid  = 1'b0;
        end

        // We should never get here
        default: begin
          cpu_error   = 1'b0;
          chdr_error  = 1'b0;
          in3.tvalid  = 1'b0;
          in3.tlast   = 1'b0;
          next_dispatch_state = ST_IDLE_ETH_L0;
        end
      endcase
  end


  //---------------------------------------
  // SPLIT
  //---------------------------------------
  // differentiating push_errors for reporting
  logic cpu0_push_error, cpu0_push_error_old= 1'b0;
  logic chdr0_push_error, chdr0_push_error_old= 1'b0;
  logic chdr0_error, chdr0_error_old = 1'b0;
  logic cpu0_error, cpu0_error_old = 1'b0;

  always_comb begin : cpu0_assign
    cpu0_error  = (cpu_error && in3.tvalid) || cpu0_error_old;
    cpu0_push_error = (cpu_push_error && in3.tvalid)|| cpu0_push_error_old;
    cpu0.tdata = in3.tdata;
    cpu0.tuser = in3.tuser;
    cpu0.tlast = in3.tlast || cpu0_error;
    cpu0.tvalid = in3.tvalid || cpu0_error;

    chdr0_error  = (chdr_error && in3.tvalid) || chdr0_error_old;
    chdr0_push_error = (chdr_push_error && in3.tvalid) || chdr0_push_error_old;
    chdr0.tdata = in3.tdata;
    chdr0.tuser = in3.tuser;
    chdr0.tlast = in3.tlast || chdr0_error;
    chdr0.tvalid = in3.tvalid || chdr0_error;

    // If the downstream sections are not ready, then the packet is dropped
    // there isn't really any buffer up stream, so a hold off here would
    // mean pushing back on a mac that doesn't have the capability to slow
    // down, and a data word would be lost.
    in3.tready = 1'b1;

  end

  // hold the error bits until the end of the packet
  always_ff @(posedge eth_rx.clk) begin : error_ff
    if (eth_rx.rst) begin
      cpu0_error_old  <= 1'b0;
      chdr0_error_old <= 1'b0;
      cpu0_push_error_old  <= 1'b0;
      chdr0_push_error_old <= 1'b0;
      chdr_dropped <= 1'b0;
      cpu_dropped  <= 1'b0;
    end else begin

      // report dropped back at the end of packet when push_error is detected.
      // 1st term counts the drop if ready lets up before the end of the packet
      // 2nd term counts the drop if ready is held through then end of the packet
      // NOTE: Drop counts don't have to be perfect.  This gets pretty close though.
      //       I.e. Don't sweat this more in the future.
      chdr_dropped <= (chdr0_push_error && chdr0.tlast && chdr0.tvalid && chdr0.tready) ||
                      (chdr0_push_error && in2.tlast && in2.tvalid);
      cpu_dropped  <= (cpu0_push_error && cpu0.tlast && cpu0.tvalid && cpu0.tready) ||
                      (cpu0_push_error && in2.tlast && in2.tvalid);

      // don't clear till we discard a packet
      if (cpu0.tlast && cpu0.tvalid && cpu0.tready) begin
        cpu0_push_error_old  <= 1'b0;
      // remember if we saw an error
      end else if (cpu0_push_error && cpu0.tvalid) begin
        cpu0_push_error_old  <= 1'b1;
      end

      // don't clear till we discard a packet
      if (cpu0.tlast && cpu0.tvalid && cpu0.tready) begin
        cpu0_error_old  <= 1'b0;
      // remember if we saw an error
      end else if (cpu0_error && cpu0.tvalid) begin
        cpu0_error_old  <= 1'b1;
      end

      // don't clear till we discard a packet
      if (chdr0.tlast && chdr0.tvalid && chdr0.tready) begin
        chdr0_push_error_old  <= 1'b0;
      // remember if we saw an error
      end else if (chdr0_push_error && chdr0.tvalid) begin
        chdr0_push_error_old  <= 1'b1;
      end

      // don't clear till we discard a packet
      if (chdr0.tlast && chdr0.tvalid && chdr0.tready) begin
        chdr0_error_old  <= 1'b0;
      // remember if we saw an error
      end else if (chdr0_error && chdr0.tvalid) begin
        chdr0_error_old  <= 1'b1;
      end

    end
  end

  // We cannot make a CHDR/noCHDR routing decision until we are in the middle
  // of a packet so we use a packet gate for the CPU path because we can rewind
  // the write pointer and drop the packet in case it's destined for the CHDR
  // path

  //---------------------------------------
  // CPU Output processing
  //---------------------------------------
  // NOTE: This also rejects packets with FCS failures.
  // NOTE: The SIZE of this FIFO must accommodate a 9000 byte jumbo frame
  //       regardless of the CHDR MTU
  // SIZED for 11 bit address when using a 64 bit word -> 16KByte
  // SIZED for 8 bit address when using a 512 bit word -> 16KByte
  axi4s_packet_gate #(
    .SIZE(14-$clog2(ENET_W/8)), .USE_AS_BUFF(0)
  ) cpu_out_gate_i (
    .clear(1'b0), .error(cpu0_error),
    .i(cpu0),.o(cpu1)
  );

  // The CPU can be slow to respond (relative to packet wire speed) so
  // extra buffer for packets destined there so it doesn't back up.
  axi4s_fifo #(
    .SIZE(CPU_FIFO_SIZE-$clog2(ENET_W/8))
  ) cpu_fifo_i (
    .clear(),.space(),.occupied(),
    .i(cpu1),.o(e2c)
  );

  //---------------------------------------
  // CHDR Output processing
  //---------------------------------------
  // CHDR DATA GATE
  // SIZED for 11 bit address when using a 64 bit word -> 16KByte
  // SIZED for 8 bit address when using a 512 bit word -> 16KByte
  axi4s_packet_gate #(
    .SIZE(14-$clog2(ENET_W/8))
  ) chdr_out_gate_i (
    .clear(1'b0),.error(chdr0_error),
    .i(chdr0),.o(chdr1)
  );

  // The transport should hook up to a crossbar downstream, which
  // may back-pressure this module because it is in the middle of
  // transferring a packet. To ensure that upstream logic is not
  // blocked, we instantiate at least one packet of buffering here.
  // The actual size is set by CHDR_FIFO_SIZE.
  logic [15:0] chdr_occupied;
  logic [15:0] chdr_occupied_q;
  localparam CHDR_FIFO_WORD_SIZE = CHDR_FIFO_SIZE-$clog2(ENET_W/8);
  axi4s_fifo #(
    .SIZE(CHDR_FIFO_WORD_SIZE)
  ) chdr_fifo_i (
    .clear(1'b0),.space(),.occupied(chdr_occupied),
    .i(chdr1),.o(chdr2)
  );

  // Remove the preamble on the CHDR path if it was not already removed.
  if (PREAMBLE_BYTES > 0 && CPU_PREAMBLE) begin : gen_strip_chdr_preamble
    axi4s_remove_bytes #(
      .REM_START(0), .REM_END(PREAMBLE_BYTES-1)
    ) strip_preamble (
      .i(chdr2), .o(e2v)
    );
  end else begin : gen_no_strip_chdr_preamble
    always_comb begin
      `AXI4S_ASSIGN(e2v, chdr2);
    end
  end

  // Documentation requires pause requests to be set for a minimum of 16
  // clocks. I'm providing the same guaranteed min time in the set and clear
  // direction.
  logic [3:0] pause_timer;
  typedef enum logic [1:0] {
      ST_IDLE           = 2'd0,
      ST_MIN_DELAY_SET  = 2'd1,
      ST_REQUESTING     = 2'd2,
      ST_MIN_DELAY_CLR  = 2'd3
    } pause_state_t;
  pause_state_t pause_state = ST_IDLE;

  always_ff @(posedge eth_rx.clk) begin : pause_req_ff
    if (eth_rx.rst) begin
      chdr_occupied_q <= 0;
      eth_pause_req   <= 1'b0;
      pause_state     <= ST_IDLE;
      pause_timer     <= 0;
    end else begin
      chdr_occupied_q <= chdr_occupied;

      case (pause_state)

        ST_IDLE: begin
          pause_timer <= 0;
          if (chdr_occupied_q >= e_pause_set) begin
            eth_pause_req   <= 1'b1;
            pause_state     <= ST_MIN_DELAY_SET;
            pause_timer     <= pause_timer-1; // Wrap counter to max value
          end
        end

        ST_MIN_DELAY_SET: begin
          pause_timer     <= pause_timer-1;
          if (pause_timer == 1) begin
            pause_state     <= ST_REQUESTING;
          end
        end

        ST_REQUESTING: begin
          pause_timer     <= 0;
          if (chdr_occupied_q <= e_pause_clear) begin
            eth_pause_req   <= 1'b0;
            pause_state     <= ST_MIN_DELAY_CLR;
            pause_timer     <= pause_timer-1; // Wrap counter to max value
          end
        end

        ST_MIN_DELAY_CLR: begin
          pause_timer     <= pause_timer-1;
          if (pause_timer == 1) begin
            pause_state     <= ST_IDLE;
          end
        end
      endcase
    end
  end


endmodule // eth_ipv4_chdr_dispatch
