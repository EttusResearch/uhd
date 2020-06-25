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
//  - CPU_FIFO_SIZE: log2 size of CPU RX fifo
//  - PREAMBLE_BYTES: Number of bytes in the Preamble
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

module eth_ipv4_chdr_dispatch #(
  int CPU_FIFO_SIZE    = $clog2(1558),
  int PREAMBLE_BYTES   = 6,
  int MAX_PACKET_BYTES = 2**16-1,
  bit DROP_UNKNOWN_MAC = 0,
  bit DROP_MIN_PACKET  = 0,
  int ENET_W           = 64
)(

  // AXI-Stream interfaces
  AxiStreamIf.slave   eth_rx, // tUser={error,trailing bytes};
  AxiStreamIf.master  e2v,    // tUser={1'b0,trailing bytes};
  AxiStreamIf.master  e2c,    // tUser={1'b0,trailing bytes};

  // Device addresses
  input  logic [47:0] my_mac,
  input  logic [31:0] my_ip,
  input  logic [15:0] my_udp_chdr_port
);

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
  //   tUser = {error,trailing bytes};
  AxiStreamPacketIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),
                .TKEEP(0),.MAX_PACKET_BYTES(MAX_PACKET_BYTES))
    in0(eth_rx.clk,eth_rx.rst);
  // in_reg
  //   tUser = {error,trailing bytes};
  AxiStreamPacketIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),
                .TKEEP(0),.MAX_PACKET_BYTES(MAX_PACKET_BYTES))
    in1(eth_rx.clk,eth_rx.rst);
  // STATEMACHINE
  //   tUser = {error,trailing bytes};
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0))
    in2(eth_rx.clk,eth_rx.rst);

  // CPU_BRANCH
  //   tUser = {error,trailing bytes};
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0))
    cpu0(eth_rx.clk,eth_rx.rst);
  // out_reg_cpu
  //   tUser = {error,trailing bytes};
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0))
    cpu1(eth_rx.clk,eth_rx.rst);
  // cpu_out_gate - throw away error packets
  //   tUser = {1'b0,trailing bytes};
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.USER_WIDTH(ENET_USER_W),.TKEEP(0))
    cpu2(eth_rx.clk,eth_rx.rst);
  // cpu_out_fifo
  // e2c (OUTPUT)

  // CHDR_Branch
  //   tUser = {error,trailing bytes};
  AxiStreamIf #(.DATA_WIDTH(ENET_W),.TKEEP(0),.TUSER(0))
    chdr0(eth_rx.clk,eth_rx.rst);
  // e2v(OUTPUT)

  //---------------------------------------
  // Strip Bytes
  //---------------------------------------
  if (PREAMBLE_BYTES > 0) begin : gen_strip_preamble
    // Strip the preamble
    axi4s_remove_bytes #(.REM_START(0),.REM_END(PREAMBLE_BYTES-1)
    ) strip_preamble (
      .i(eth_rx),.o(in0)
    );
  end else begin : gen_no_preamble
    always_comb begin
      `AXI4S_ASSIGN(in0,eth_rx);
    end
  end

  //---------------------------------------
  // Input pipeline stage
  //---------------------------------------
  axi4s_fifo #(
    .SIZE(1)
  ) in_reg_i (
    .clear(1'b0),.space(),.occupied(),
    .i(in0), .o(in1)
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
  logic chdr0_error = 1'b0;
  logic mac_error, mac_error_old = 1'b0;
  logic min_packet_error, min_packet_error_old = 1'b0;
  logic reached_min_packet;

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
      mac_error_old        <= 1'b0;
      min_packet_error_old <= 1'b0;
      reached_min_packet_old <= 1'b0;

      // Statemachine Decisions
      eth_dst_is_broadcast <=1'b0;
      eth_dst_is_me        <=1'b0;
      udp_dst_is_me        <=1'b0;
      ipv4_dst_is_me       <=1'b0;
      ipv4_protocol_is_udp <=1'b0;
      eth_type_is_ipv4     <=1'b0;
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

      if (in0.tvalid && in0.tready) begin
        eth_dst_is_broadcast <= eth_dst_addr_new == ETH_ADDR_BCAST;
        eth_dst_is_me        <= eth_dst_addr_new == my_mac;
        udp_dst_is_me        <= udp_dst_port_new == my_udp_chdr_port;
        ipv4_dst_is_me       <= ipv4_dst_addr_new == my_ip;
        ipv4_protocol_is_udp <= ip_protocol_new == IPV4_PROTO_UDP;
        eth_type_is_ipv4     <= eth_type_new == ETH_TYPE_IPV4;
      end
      if (in1.tvalid && in1.tready) begin
        if (in1.tlast) begin
          mac_error_old <= 1'b0;
          min_packet_error_old <= 1'b0;
          reached_min_packet_old <= 1'b0;
        end else begin
          if (mac_error)
            mac_error_old <= 1'b1;
          if(min_packet_error)
            min_packet_error_old <= 1'b1;
          if(reached_min_packet_new)
            reached_min_packet_old <= 1'b1;
        end
      end
    end
  end

 // get the fields - don't use assign. assign will not activate with changes to in0.
  always_comb begin  : get_fields
    eth_dst_addr_new  = in0.get_packet_field48(eth_dst_addr_old,DST_MAC_BYTE,.NETWORK_ORDER(1));
    eth_src_addr_new  = in0.get_packet_field48(eth_src_addr_old,SRC_MAC_BYTE,.NETWORK_ORDER(1));
    ip_version_new    = in0.get_packet_byte(ip_version_old,IP_VERSION_BYTE);
    ip_protocol_new   = in0.get_packet_byte(ip_protocol_old,PROTOCOL_BYTE);
    ipv4_src_addr_new = in0.get_packet_field32(ipv4_src_addr_old,SRC_IP_BYTE,.NETWORK_ORDER(1));
    ipv4_dst_addr_new = in0.get_packet_field32(ipv4_dst_addr_old,DST_IP_BYTE,.NETWORK_ORDER(1));
    udp_src_port_new  = in0.get_packet_field16(udp_src_port_old,SRC_PORT_BYTE,.NETWORK_ORDER(1));
    udp_dst_port_new  = in0.get_packet_field16(udp_dst_port_old,DST_PORT_BYTE,.NETWORK_ORDER(1));
    eth_type_new      = in0.get_packet_field16(eth_type_old,ETH_TYPE_BYTE,.NETWORK_ORDER(1));
  end


  always_comb begin  : reached_bytes
    reached_min_packet_new = in1.reached_packet_byte(MIN_PACKET_SIZE_BYTE);
    reached_end_of_udp     = in1.reached_packet_byte(DST_PORT_BYTE+3);// we have enough to decide
  end
  assign mac_error          = in1.tuser[ERROR_BIT] || mac_error_old;
  if (DROP_MIN_PACKET) begin
    assign reached_min_packet = (reached_min_packet_new && in1.tuser[BYTES_MSB:0] ==0) || reached_min_packet_old;
    assign min_packet_error   = (in1.tlast && !reached_min_packet) || min_packet_error_old;
  end else begin
    assign reached_min_packet = 1'b1;
    assign min_packet_error   = 1'b0;
  end

  always_ff @(posedge eth_rx.clk) begin : dispatch_sm_ff
    if (eth_rx.rst) begin
      dispatch_state  <= ST_IDLE_ETH_L0;
    end else begin
      if (in1.tvalid && in1.tready) begin
        if (in1.tlast)
          dispatch_state <= ST_IDLE_ETH_L0;
        else
         dispatch_state <= next_dispatch_state;
      end
    end
  end

  always_comb begin : dispatch_sm_next_state
      //defaults
      next_dispatch_state = dispatch_state;
      `AXI4S_ASSIGN(in2,in1);
      in2.tuser[ERROR_BIT] = mac_error || min_packet_error;
      cpu_error   = 1'b0;
      chdr_error  = 1'b0;

      // Statemachine always returns to ST_IDLE_ETH_L0 when tlast is set
      case (dispatch_state)
        ST_IDLE_ETH_L0: begin
          cpu_error   = 1'b0;
          chdr_error  = 1'b0;
          if (mac_error || min_packet_error) begin
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
          if (mac_error || min_packet_error) begin
             cpu_error  = 1'b1;
             chdr_error = 1'b1;
             next_dispatch_state = ST_DROP_TERM;
          end
        end

        // NotCHDR Payload: Send to CPU
        ST_FWD_CPU: begin
          cpu_error   = 1'b0;
          chdr_error  = 1'b1;
          if (mac_error || min_packet_error) begin
             cpu_error  = 1'b1;
             chdr_error = 1'b1;
             next_dispatch_state = ST_DROP_TERM;
          end
        end

        // Unwanted Payload: Drop
        ST_DROP_TERM: begin
          cpu_error   = 1'b1;
          chdr_error  = 1'b1;
          in2.tlast   = 1'b1;
          in2.tvalid  = 1'b1;
          in1.tready  = in2.tready;
          next_dispatch_state = ST_DROP_WAIT;
        end

        // Unwanted Payload: wait
        ST_DROP_WAIT: begin
          cpu_error   = 1'b0;
          chdr_error  = 1'b0;
          in2.tlast   = 1'b0;
          in2.tvalid  = 1'b0;
          in1.tready  = 1'b1;
        end

        // We should never get here
        default: begin
          cpu_error   = 1'b0;
          chdr_error  = 1'b0;
          in1.tready  = 1'b1;
          in2.tvalid  = 1'b0;
          in2.tlast   = 1'b0;
          next_dispatch_state = ST_IDLE_ETH_L0;
        end
      endcase
  end


  //---------------------------------------
  // SPLIT
  //---------------------------------------
  always_comb begin : cpu0_assign
    cpu0.tdata = in2.tdata;
    cpu0.tuser = in2.tuser;
    cpu0.tlast = in2.tlast;
    cpu0.tvalid = in2.tvalid && chdr0.tready;
    cpu0.tuser[ERROR_BIT]  = in2.tuser[ERROR_BIT] || cpu_error;

    chdr0.tdata = in2.tdata;
    chdr0.tuser = in2.tuser;
    chdr0.tlast = in2.tlast;
    chdr0.tvalid = in2.tvalid && cpu0.tready;
    chdr0_error  = in2.tuser[ERROR_BIT] || chdr_error;

    in2.tready = cpu0.tready && chdr0.tready;
  end

  //---------------------------------------
  // CPU Output processing
  //---------------------------------------
  axi4s_fifo #(
    .SIZE(1)
  ) out_reg_cpu_i (
    .clear(),.space(),.occupied(),
    .i(cpu0),.o(cpu1)
  );

  // We cannot make a CHDR/noCHDR routing decision until we are in the middle
  // of a packet so we use a packet gate for the CPU path because we can rewind
  // the write pointer and drop the packet in case it's destined for the CHDR
  // path.
  // NOTE: This also rejects packets with FCS failures.
  // NOTE: The SIZE of this FIFO must accommodate a 9000 byte jumbo frame
  //       regardless of the CHDR MTU
  // SIZED for 11 bit address when using a 64 bit word -> 16KByte
  // SIZED for 8 bit address when using a 512 bit word -> 16KByte
  axi4s_packet_gate #(
    .SIZE(17-$clog2(ENET_W)), .USE_AS_BUFF(0)
  ) cpu_out_gate_i (
    .clear(1'b0), .error(cpu1.tuser[ERROR_BIT]),
    .i(cpu1),.o(cpu2)
  );

  // The CPU can be slow to respond (relative to packet wirespeed) so
  // extra buffer for packets destined there so it doesn't back up.
  axi4s_fifo #(
    .SIZE(CPU_FIFO_SIZE)
  ) cpu_fifo_i (
    .clear(),.space(),.occupied(),
    .i(cpu2),.o(e2c)
  );

  // CHDR DATA GATE
  // SIZED for 11 bit address when using a 64 bit word -> 16KByte
  // SIZED for 8 bit address when using a 512 bit word -> 16KByte
  axi4s_packet_gate #(
    .SIZE(17-$clog2(ENET_W))
  ) chdr_out_gate_i (
    .clear(1'b0),.error(chdr0_error),
    .i(chdr0),.o(e2v)
  );

endmodule // eth_ipv4_chdr_dispatch
