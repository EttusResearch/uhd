//
// Copyright 2023 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_crossbar_nxn
//
// Description:
//
//   This module implements a full-bandwidth NxN crossbar with N input and
//   output ports for CHDR traffic. It supports multiple optimization
//   strategies for performance, area and timing trade-offs. It uses AXI-Stream
//   for all of its links. The crossbar has a dynamic routing table based on a
//   Content Addressable Memory (CAM). The SID is used to determine the
//   destination of a packet and the routing table contains a re-programmable
//   SID to crossbar port mapping. The table is programmed using special route
//   config packets on the data input ports or using an optional management
//   port.
//
//   The topology, routing algorithms and the router architecture is described
//   in README.pdf in this directory.
//
//   This crossbar also supports multiple port sizes. By default, each port
//   will be PORT_W bits wide. This can be changed using the CHDR_WIDTHS
//   parameter. This parameter allows the CHDR width of each port to be
//   specified. When using multiple CHDR widths, the PORT_W parameter should be
//   the size of the widest port. The CHDR_W value reported by the management
//   port will be the value specified for that port in CHDR_WIDTHS.
//
// Parameters:
//
//   PORT_W        : Width of the AXI-Stream data buses s_axis and m_axis. If
//                   using multiple port widths, this should be set to the
//                   width of the widest port.
//   NPORTS        : Number of ports to instantiate.
//   CHDR_WIDTHS   : Descending array of NUM_PORT integers representing the
//                   width of each crossbar port. The width of port n is given
//                   by CHDR_WIDTHS[(N+1)*32-1 : N*32].
//   ROUTES        : Descending array representing which crossbar routes to
//                   enable. This is an NPORTS*NPORTS-bit array where bit
//                   [NPORTS*A + B] corresponds to the path from input port A
//                   to output port B. A '1' indicates the logic for that route
//                   is included. All routes are enabled by default.
//   EN_ROUTE_FIFO : Set to 1 to include a FIFO on all routes going from a wide
//                   port to a narrow port. This may improve performance when a
//                   single wide input port streams to multiple narrow output
//                   ports by buffering the input data while it's resized for
//                   the slower output port. This helps to avoid congestion on
//                   the input port.
//   EN_ROUTE_GATE : Set to 1 to include a packet gate on all routes going from
//                   a narrow port to a wide port. This may improve performance
//                   when multiple narrow input ports stream to a single wide
//                   output port by removing idle transfer cycles caused by the
//                   slower rate of the narrow input port. This helps to avoid
//                   congestion on the output port.
//   DEFAULT_PORT  : The fail-safe port to forward a packet to if SID mapping
//                   is missing.
//   BYTE_MTU      : log2 of the max packet size in bytes.
//   ROUTE_TBL_SIZE: log2 of the number of mappings that the routing table can
//                   hold at any time. Mapping values are maintained in a FIFO
//                   fashion.
//   MUX_ALLOC     : Algorithm to allocate the egress MUX. Possible values:
//                   * "PRIO": Priority based. Lower port numbers have a
//                   higher priority
//                   * "ROUND-ROBIN": Round robin input port allocation
//   OPTIMIZE      : Optimization strategy for performance vs area vs timing
//                   trade-offs. Possible values:
//                   * "AREA": Attempt to minimize area at the cost of
//                     performance (throughput) and/or timing.
//                   * "PERFORMANCE": Attempt to maximize performance at the
//                     cost of area and/or timing.
//                   * "TIMING": Attempt to maximize Fmax at the cost of area
//                      and/or performance.
//   NPORTS_MGMT   : Number of ports with management endpoint. The first
//                   NPORTS_MGMT ports will have the management port
//                   instantiated.
//   EXT_RTCFG_PORT: Enable a side-channel AXI-Stream management port to
//                   configure the routing table.
//
//  CHDR_WIDTHS Bit Mapping Example (4x4):
//
//     Port #:      3       2       1       0
//                  ↓       ↓       ↓       ↓
//              {32'd64, 32'd64, 32'd64, 32'd64}
//
//   ROUTES Bit Mapping Example (4x4):
//
//          Output Port:  3210
//                        ↓↓↓↓
//     Input Port 3 → {4'b1111,
//     Input Port 2 →  4'b1111,
//     Input Port 1 →  4'b1111,
//     Input Port 0 →  4'b1111}
//
// Ports:
//
//   s_axis_*     : Slave port for router (flattened)
//   m_axis_*     : Master port for router (flattened)
//   s_axis_mgmt_*: Management slave port
//   device_id    : The ID of the device that has instantiated this module
//

module chdr_crossbar_nxn #(
  parameter [15:0]          PROTOVER       = {8'd1, 8'd0},
  parameter [31:0]          PORT_W         = 64,
  parameter [7:0]           NPORTS         = 8,
  parameter [NPORTS*32-1:0] CHDR_WIDTHS    = {NPORTS{PORT_W}},
  parameter                 EN_ROUTE_FIFO  = 0,
  parameter                 EN_ROUTE_GATE  = 0,
  parameter [7:0]           DEFAULT_PORT   = 0,
  parameter [NPORTS**2-1:0] ROUTES         = {NPORTS*NPORTS{1'b1}},
  parameter                 BYTE_MTU       = $clog2(8192),
  parameter                 ROUTE_TBL_SIZE = 6,
  parameter                 MUX_ALLOC      = "ROUND-ROBIN",
  parameter                 OPTIMIZE       = "AREA",
  parameter [7:0]           NPORTS_MGMT    = NPORTS,
  parameter                 EXT_RTCFG_PORT = 0
) (
  input  wire                       clk,
  input  wire                       reset,
  // Device info
  input  wire [15:0]                device_id,
  // Inputs
  input  wire [(PORT_W*NPORTS)-1:0] s_axis_tdata,
  input  wire [NPORTS-1:0]          s_axis_tlast,
  input  wire [NPORTS-1:0]          s_axis_tvalid,
  output wire [NPORTS-1:0]          s_axis_tready,
  // Output
  output wire [(PORT_W*NPORTS)-1:0] m_axis_tdata,
  output wire [NPORTS-1:0]          m_axis_tlast,
  output wire [NPORTS-1:0]          m_axis_tvalid,
  input  wire [NPORTS-1:0]          m_axis_tready,
  // Router config management port
  input  wire                       ext_rtcfg_stb,
  input  wire [15:0]                ext_rtcfg_addr,
  input  wire [31:0]                ext_rtcfg_data,
  output wire                       ext_rtcfg_ack
);
  //---------------------------------------------------------------------------
  //  RFNoC Includes
  //---------------------------------------------------------------------------

  `include "../core/rfnoc_chdr_utils.vh"
  `include "../core/rfnoc_chdr_internal_utils.vh"

  //---------------------------------------------------------------------------
  // Parameters
  //---------------------------------------------------------------------------

  localparam        NPORTS_W = $clog2(NPORTS);
  localparam        EPID_W   = 16;
  localparam [17:0] EXT_INFO = {1'b0, EXT_RTCFG_PORT, NPORTS_MGMT, NPORTS};

  localparam [0:0] PKT_ST_HEAD = 1'b0;
  localparam [0:0] PKT_ST_BODY = 1'b1;

  //---------------------------------------------------------------------------
  // Functions
  //---------------------------------------------------------------------------

  // The compute_mux_alloc function is the switch allocation function for the
  // MUX. That is, it chooses which input port reserves the output MUX for
  // packet transfer.
  function [NPORTS_W-1:0] compute_mux_alloc(
    input [  NPORTS-1:0] pkt_waiting,
    input [NPORTS_W-1:0] last_alloc
  );
    reg signed [NPORTS_W:0] i;
    begin
      compute_mux_alloc = last_alloc;
      for (i = NPORTS-1; i >= 0; i=i-1) begin
        if (MUX_ALLOC == "PRIO") begin
          // Priority. Lower port index gets a higher priority.
          if (pkt_waiting[i]) begin
            compute_mux_alloc = i;
          end
        end else begin
          // Round-robin
          if (pkt_waiting[(last_alloc + i + 1) % NPORTS]) begin
            compute_mux_alloc = (last_alloc + i + 1) % NPORTS;
          end
        end
      end
    end
  endfunction

  // Return the CHDR width of the given port.
  function [31:0] CHDR_W(input integer n);
    CHDR_W = CHDR_WIDTHS[32*n +: 32];
  endfunction

  // Return the MTU size for the given port in terms of its CHDR width.
  function [31:0] WORD_MTU(input integer n);
    WORD_MTU = BYTE_MTU - $clog2(CHDR_W(n)/8);
  endfunction

  // Return bit indicating if the route between input port i and output port j
  // is enabled.
  function [0:0] ROUTE_ENABLED(input integer i, j);
    ROUTE_ENABLED = ROUTES[NPORTS*i + j];
  endfunction

  // Return bit indicating if the given input port has any routes connected to
  // it.
  function [0:0] INPUT_HAS_ROUTES(input integer i);
    INPUT_HAS_ROUTES = |ROUTES[NPORTS*i +: NPORTS];
  endfunction

  // Return bit indicating if the given output port has any routes connected to
  // it.
  function automatic [0:0] OUTPUT_HAS_ROUTES(input integer j);
    integer i;
    begin
      OUTPUT_HAS_ROUTES = 1'b0;
      for (i = 0; i < NPORTS; i = i+1) begin
        OUTPUT_HAS_ROUTES = OUTPUT_HAS_ROUTES | ROUTES[NPORTS*i + j];
      end
    end
  endfunction

  //---------------------------------------------------------------------------
  // CHDR Routing Table
  //---------------------------------------------------------------------------

  wire [NPORTS-1:0]            rtcfg_req_wr;
  wire [(16*NPORTS)-1:0]       rtcfg_req_addr;
  wire [(32*NPORTS)-1:0]       rtcfg_req_data;
  wire [NPORTS-1:0]            rtcfg_resp_ack;
  wire [(EPID_W*NPORTS)-1:0]   find_tdata;
  wire [NPORTS-1:0]            find_tvalid;
  wire [NPORTS-1:0]            find_tready;
  wire [(NPORTS_W*NPORTS)-1:0] result_tdata;
  wire [NPORTS-1:0]            result_tkeep;
  wire [NPORTS-1:0]            result_tvalid;
  wire [NPORTS-1:0]            result_tready;

  // Instantiate a single CAM-based routing table that will be shared between
  // all input ports. Configuration and lookup is performed using an AXI-Stream
  // interface. If multiple packets arrive simultaneously, only the headers of
  // those packets will be serialized in order to arbitrate this map. Selection
  // is done round-robin.
  chdr_xb_routing_table #(
    .SIZE(ROUTE_TBL_SIZE), .NPORTS(NPORTS),
    .EXT_INS_PORT_EN(EXT_RTCFG_PORT)
  ) chdr_xb_routing_table_i (
    .clk               (clk           ),
    .reset             (reset         ),
    .port_req_wr       (rtcfg_req_wr  ),
    .port_req_addr     (rtcfg_req_addr),
    .port_req_data     (rtcfg_req_data),
    .port_resp_ack     (rtcfg_resp_ack),
    .ext_req_wr        (ext_rtcfg_stb ),
    .ext_req_addr      (ext_rtcfg_addr),
    .ext_req_data      (ext_rtcfg_data),
    .ext_resp_ack      (ext_rtcfg_ack ),
    .axis_find_tdata   (find_tdata    ),
    .axis_find_tvalid  (find_tvalid   ),
    .axis_find_tready  (find_tready   ),
    .axis_result_tdata (result_tdata  ),
    .axis_result_tkeep (result_tkeep  ),
    .axis_result_tvalid(result_tvalid ),
    .axis_result_tready(result_tready )
  );

  wire [PORT_W-1:0]          i_tdata   [0:NPORTS-1];
  wire [9:0]                 i_tdest   [0:NPORTS-1];
  wire [1:0]                 i_tid     [0:NPORTS-1];
  wire                       i_tlast   [0:NPORTS-1];
  wire                       i_tvalid  [0:NPORTS-1];
  wire                       i_tready  [0:NPORTS-1];
  wire [PORT_W-1:0]          buf_tdata [0:NPORTS-1];
  wire [NPORTS_W-1:0]        buf_tdest [0:NPORTS-1], buf_tdest_tmp[0:NPORTS-1];
  wire                       buf_tkeep [0:NPORTS-1];
  wire                       buf_tlast [0:NPORTS-1];
  wire                       buf_tvalid[0:NPORTS-1];
  wire                       buf_tready[0:NPORTS-1];
  wire [PORT_W-1:0]          swi_tdata [0:NPORTS-1];
  wire [NPORTS_W-1:0]        swi_tdest [0:NPORTS-1];
  wire                       swi_tlast [0:NPORTS-1];
  wire                       swi_tvalid[0:NPORTS-1];
  wire                       swi_tready[0:NPORTS-1];
  wire [(PORT_W*NPORTS)-1:0] swo_tdata [0:NPORTS-1], muxi_tdata [0:NPORTS-1];
  wire [NPORTS-1:0]          swo_tlast [0:NPORTS-1], muxi_tlast [0:NPORTS-1];
  wire [NPORTS-1:0]          swo_tvalid[0:NPORTS-1], muxi_tvalid[0:NPORTS-1];
  wire [NPORTS-1:0]          swo_tready[0:NPORTS-1], muxi_tready[0:NPORTS-1];

  //---------------------------------------------------------------------------
  // Port Generation
  //---------------------------------------------------------------------------

  genvar n, i, j, port;
  generate
    for (n = 0; n < NPORTS; n = n + 1) begin: gen_in_ports
      // Only generate the input logic for this input port if it has routes
      if (INPUT_HAS_ROUTES(n)) begin : gen_in_port

        //-----------------------------------------------------------------------
        // Assertions
        //-----------------------------------------------------------------------

        // Make sure the width of this port does not exceed the given maximum
        // port width.
        if (CHDR_W(n) > PORT_W) begin : gen_chdr_w_too_large
          ERROR__CHDR_W_must_not_exceed_PORT_W_parameter();
        end

        // Make sure the port's CHDR width is a valid CHDR width (a power of 2
        // and at least 64 bits).
        if (2**$clog2(CHDR_W(n)) != CHDR_W(n) || CHDR_W(n) < 64) begin : gen_invalid_chdr_w
          ERROR__CHDR_W_is_not_a_valid_CHDR_width();
        end

        // Make sure the maximum port width is a valid CHDR width (a power of 2
        // and at least 64 bits).
        if (2**$clog2(PORT_W) != PORT_W || PORT_W < 64) begin : gen_invalid_port_w
          ERROR__PORT_W_is_not_a_valid_CHDR_width();
        end

        //-----------------------------------------------------------------------
        // Management Ports
        //-----------------------------------------------------------------------

        wire [47:0] node_info =
          chdr_mgmt_build_node_info(EXT_INFO, n, NODE_TYPE_XBAR, device_id);

        // For each input port, first check if we have a management packet
        // arriving. If it arrives, the top config commands are extracted, sent
        // to the routing table for configuration, and the rest of the packet is
        // forwarded down to the router. the router.
        if (n < NPORTS_MGMT) begin : gen_mgmt
          chdr_mgmt_pkt_handler #(
            .PROTOVER  (PROTOVER ),
            .CHDR_W    (CHDR_W(n)),
            .MGMT_ONLY (0        )
          ) chdr_mgmt_pkt_handler_i (
            .clk                (clk                                 ),
            .rst                (reset                               ),
            .node_info          (node_info                           ),
            .s_axis_chdr_tdata  (s_axis_tdata [(n*PORT_W)+:CHDR_W(n)]),
            .s_axis_chdr_tlast  (s_axis_tlast [n]                    ),
            .s_axis_chdr_tvalid (s_axis_tvalid[n]                    ),
            .s_axis_chdr_tready (s_axis_tready[n]                    ),
            .s_axis_chdr_tuser  (1'd0                                ),
            .m_axis_chdr_tdata  (i_tdata      [n][CHDR_W(n)-1:0]     ),
            .m_axis_chdr_tdest  (i_tdest      [n]                    ),
            .m_axis_chdr_tid    (i_tid        [n]                    ),
            .m_axis_chdr_tlast  (i_tlast      [n]                    ),
            .m_axis_chdr_tvalid (i_tvalid     [n]                    ),
            .m_axis_chdr_tready (i_tready     [n]                    ),
            .ctrlport_req_wr    (rtcfg_req_wr [n]                    ),
            .ctrlport_req_rd    (/* unused */                        ),
            .ctrlport_req_addr  (rtcfg_req_addr[(n*16)+:16]          ),
            .ctrlport_req_data  (rtcfg_req_data[(n*32)+:32]          ),
            .ctrlport_resp_ack  (rtcfg_resp_ack[n]                   ),
            .ctrlport_resp_data (32'h0 /* unused */                  ),
            .op_stb             (/* unused */                        ),
            .op_dst_epid        (/* unused */                        ),
            .op_src_epid        (/* unused */                        ),
            .op_data            (/* unused */                        )
          );
        end else begin : gen_no_mgmt
          assign i_tdata      [n] = s_axis_tdata [(n*PORT_W)+:CHDR_W(n)];
          assign i_tid        [n] = CHDR_MGMT_ROUTE_EPID;
          assign i_tdest      [n] = 10'd0;  // Unused
          assign i_tlast      [n] = s_axis_tlast [n];
          assign i_tvalid     [n] = s_axis_tvalid[n];
          assign s_axis_tready[n] = i_tready     [n];

          assign rtcfg_req_wr  [n]          =  1'b0;
          assign rtcfg_req_addr[(n*16)+:16] = 16'h0;
          assign rtcfg_req_data[(n*32)+:32] = 32'h0;
        end

        //-----------------------------------------------------------------------
        // Port Ingress Buffer
        //-----------------------------------------------------------------------

        // Ingress buffer module that does the following:
        // - Stores and gates an incoming packet
        // - Looks up destination in routing table and attaches a tdest for the packet
        chdr_xb_ingress_buff #(
          .WIDTH  (CHDR_W(n)  ),
          .MTU    (WORD_MTU(n)),
          .DEST_W (NPORTS_W   ),
          .NODE_ID(n          )
        ) chdr_xb_ingress_buff_i (
          .clk                 (clk                                  ),
          .reset               (reset                                ),
          .s_axis_chdr_tdata   (i_tdata      [n][CHDR_W(n)-1:0]      ),
          .s_axis_chdr_tdest   (i_tdest      [n][NPORTS_W-1:0]       ),
          .s_axis_chdr_tid     (i_tid        [n]                     ),
          .s_axis_chdr_tlast   (i_tlast      [n]                     ),
          .s_axis_chdr_tvalid  (i_tvalid     [n]                     ),
          .s_axis_chdr_tready  (i_tready     [n]                     ),
          .m_axis_chdr_tdata   (buf_tdata    [n][CHDR_W(n)-1:0]      ),
          .m_axis_chdr_tdest   (buf_tdest_tmp[n]                     ),
          .m_axis_chdr_tkeep   (buf_tkeep    [n]                     ),
          .m_axis_chdr_tlast   (buf_tlast    [n]                     ),
          .m_axis_chdr_tvalid  (buf_tvalid   [n]                     ),
          .m_axis_chdr_tready  (buf_tready   [n]                     ),
          .m_axis_find_tdata   (find_tdata   [(n*EPID_W)+:EPID_W]    ),
          .m_axis_find_tvalid  (find_tvalid  [n]                     ),
          .m_axis_find_tready  (find_tready  [n]                     ),
          .s_axis_result_tdata (result_tdata [(n*NPORTS_W)+:NPORTS_W]),
          .s_axis_result_tkeep (result_tkeep [n]                     ),
          .s_axis_result_tvalid(result_tvalid[n]                     ),
          .s_axis_result_tready(result_tready[n]                     )
        );
        assign buf_tdest[n] = buf_tkeep[n] ? buf_tdest_tmp[n] : DEFAULT_PORT[NPORTS_W-1:0];

        // Pipeline stage
        axi_fifo #(
          .WIDTH(CHDR_W(n)+1+NPORTS_W),
          .SIZE (1                   )
        ) axi_fifo_i (
          .clk     (clk                                                      ),
          .reset   (reset                                                    ),
          .clear   (1'b0                                                     ),
          .i_tdata ({buf_tlast[n], buf_tdest[n], buf_tdata[n][CHDR_W(n)-1:0]}),
          .i_tvalid(buf_tvalid[n]                                            ),
          .i_tready(buf_tready[n]                                            ),
          .o_tdata ({swi_tlast[n], swi_tdest[n], swi_tdata[n][CHDR_W(n)-1:0]}),
          .o_tvalid(swi_tvalid[n]                                            ),
          .o_tready(swi_tready[n]                                            ),
          .space   (/* Unused */                                             ),
          .occupied(/* Unused */                                             )
        );

        //-----------------------------------------------------------------------
        // Ingress Switch (De-multiplexers)
        //-----------------------------------------------------------------------

        wire [CHDR_W(n)*NPORTS-1:0] swo_tdata_packed;

        // Ingress de-mux. Use the tdest field to determine packet destination.
        axis_switch #(
          .DATA_W   (CHDR_W(n)),
          .DEST_W   (1        ),
          .IN_PORTS (1        ),
          .OUT_PORTS(NPORTS   ),
          .PIPELINE (1        )
        ) axis_switch_demux (
          .clk          (clk                        ),
          .reset        (reset                      ),
          .s_axis_tdata (swi_tdata[n][CHDR_W(n)-1:0]),
          .s_axis_tdest ({1'b0, swi_tdest[n]}       ),
          .s_axis_tlast (swi_tlast [n]              ),
          .s_axis_tvalid(swi_tvalid[n]              ),
          .s_axis_tready(swi_tready[n]              ),
          .s_axis_alloc (1'b0                       ),
          .m_axis_tdata (swo_tdata_packed           ),
          .m_axis_tdest (/* Unused */               ),
          .m_axis_tlast (swo_tlast [n]              ),
          .m_axis_tvalid(swo_tvalid[n]              ),
          .m_axis_tready(swo_tready[n]              )
        );

        // Unpack the switch output to handle the case where this port's CHDR_W
        // is narrower than PORT_W.
        for (port = 0; port < NPORTS; port = port+1) begin : gen_switch_output
          assign swo_tdata[n][PORT_W*port +: CHDR_W(n)] =
            swo_tdata_packed[CHDR_W(n)*port +: CHDR_W(n)];
        end
      end // gen_in_port
    end // gen_in_ports

    //-------------------------------------------------------------------------
    // Crossbar Routing
    //-------------------------------------------------------------------------

    // Generate the routing for a full NxN crossbar where i is the input port
    // number and j is the output port number. Some paths are resized,
    // depending on CHDR_WIDTHS, or excluded, depending on ROUTES.
    for (i = 0; i < NPORTS; i = i + 1) begin : gen_for_i
      for (j = 0; j < NPORTS; j = j + 1) begin : gen_for_j
        if (ROUTE_ENABLED(i,j)) begin : gen_enabled_route
          wire [CHDR_W(i)-1:0] rs_i_tdata;
          wire                 rs_i_tlast;
          wire                 rs_i_tvalid;
          wire                 rs_i_tready;

          wire [CHDR_W(j)-1:0] rs_o_tdata;
          wire                 rs_o_tlast;
          wire                 rs_o_tvalid;
          wire                 rs_o_tready;

          // Connect output j of ingress port i to input i of egress port j.
          // Resize the bus if the ports have different widths, otherwise
          // directly connect them.
          if (CHDR_W(i) != CHDR_W(j)) begin : gen_port_resize
            if (CHDR_W(i) > CHDR_W(j) && EN_ROUTE_FIFO) begin : gen_input_fifo
              // If we're downsizing, we need a wide FIFO on the input to the
              // resize block to buffer the fast incoming packet.
              axi_fifo #(
                .WIDTH(CHDR_W(i)+1),
                .SIZE (WORD_MTU(i))
              ) axi_fifo_i (
                .clk     (clk                                                 ),
                .reset   (reset                                               ),
                .clear   (1'b0                                                ),
                .i_tdata ({swo_tlast[i][j], swo_tdata[i][j*PORT_W+:CHDR_W(i)]}),
                .i_tvalid(swo_tvalid[i][j]                                    ),
                .i_tready(swo_tready[i][j]                                    ),
                .o_tdata ({rs_i_tlast, rs_i_tdata}                            ),
                .o_tvalid(rs_i_tvalid                                         ),
                .o_tready(rs_i_tready                                         ),
                .space   (                                                    ),
                .occupied(                                                    )
              );
            end else begin : gen_no_input_fifo
                assign rs_i_tdata       = swo_tdata[i][j*PORT_W+:CHDR_W(i)];
                assign rs_i_tlast       = swo_tlast[i][j];
                assign rs_i_tvalid      = swo_tvalid[i][j];
                assign swo_tready[i][j] = rs_i_tready;
            end

            chdr_resize #(
              .I_CHDR_W(CHDR_W(i)),
              .O_CHDR_W(CHDR_W(j)),
              .I_DATA_W(CHDR_W(i)),
              .O_DATA_W(CHDR_W(j)),
              .USER_W  (1        ),
              .PIPELINE("OUT"    )
            ) chdr_resize_i (
              .clk          (clk        ),
              .rst          (reset      ),
              .i_chdr_tdata (rs_i_tdata ),
              .i_chdr_tuser (1'b0       ),
              .i_chdr_tlast (rs_i_tlast ),
              .i_chdr_tvalid(rs_i_tvalid),
              .i_chdr_tready(rs_i_tready),
              .o_chdr_tdata (rs_o_tdata ),
              .o_chdr_tuser (           ),
              .o_chdr_tlast (rs_o_tlast ),
              .o_chdr_tvalid(rs_o_tvalid),
              .o_chdr_tready(rs_o_tready)
            );

            if (CHDR_W(i) < CHDR_W(j) && EN_ROUTE_GATE) begin : gen_output_pkt_gate
              // If we are up-sizing, then there will be idle cycles on the
              // wider output bus that will waste time on the output mux. To
              // maximize throughput on the output port, we gate packets here
              // so that we can output a continuous stream of data without idle
              // cycles.
              axi_packet_gate #(
                .WIDTH(CHDR_W(j)  ),
                .SIZE (WORD_MTU(j))
              ) axi_packet_gate_i (
                .clk     (clk                               ),
                .reset   (reset                             ),
                .clear   (1'b0                              ),
                .i_tdata (rs_o_tdata                        ),
                .i_tlast (rs_o_tlast                        ),
                .i_terror(1'b0                              ),
                .i_tvalid(rs_o_tvalid                       ),
                .i_tready(rs_o_tready                       ),
                .o_tdata (muxi_tdata[j][i*PORT_W+:CHDR_W(j)]),
                .o_tlast (muxi_tlast[j][i]                  ),
                .o_tvalid(muxi_tvalid[j][i]                 ),
                .o_tready(muxi_tready[j][i]                 )
              );
            end else begin : gen_no_output_pkt_gate
              assign muxi_tdata[j][i*PORT_W+:CHDR_W(j)] = rs_o_tdata;
              assign muxi_tlast[j][i]                   = rs_o_tlast;
              assign muxi_tvalid[j][i]                  = rs_o_tvalid;
              assign rs_o_tready                        = muxi_tready[j][i];
            end

          end else begin : gen_port_same_size
            assign muxi_tdata[j][i*PORT_W+:CHDR_W(j)] = swo_tdata  [i][j*PORT_W+:CHDR_W(i)];
            assign muxi_tlast[j][i]                   = swo_tlast  [i][j];
            assign muxi_tvalid[j][i]                  = swo_tvalid [i][j];
            assign swo_tready[i][j]                   = muxi_tready[j][i];
          end
        end else begin : gen_disabled_route
          // Tie off these unused paths so they can be optimized out.
          assign muxi_tdata[j][i*PORT_W+:PORT_W] = { PORT_W {1'b0} };
          assign muxi_tlast[j][i]                = 1'b0;
          assign muxi_tvalid[j][i]               = 1'b0;
          assign swo_tready[i][j]                = 1'b1;
        end
      end
    end

    //-------------------------------------------------------------------------
    // Egress Switch (Multiplexers)
    //-------------------------------------------------------------------------

    for (n = 0; n < NPORTS; n = n + 1) begin: gen_out_ports
      // Only generate egress logic for this output port if it has routes
      if (OUTPUT_HAS_ROUTES(n)) begin : gen_out_port
        wire [CHDR_W(n)*NPORTS-1:0] muxi_tdata_repacked;

        // Repack the mux input to handle the case where this port's CHDR_W is
        // narrower than PORT_W.
        for (port = 0; port < NPORTS; port = port+1) begin : gen_mux_input
          assign muxi_tdata_repacked[CHDR_W(n)*port +: CHDR_W(n)] =
            muxi_tdata[n][PORT_W*port +: CHDR_W(n)];
        end

        if (OPTIMIZE == "PERFORMANCE") begin : gen_performance
          // Use the axis_switch module when optimizing for performance
          // This logic has some extra levels of logic to ensure
          // that the switch allocation happens in 0 clock cycles which
          // means that Fmax for this implementation will be lower.

          wire mux_ready = |muxi_tready[n];   // Max 1 bit should be high
          wire mux_valid = |muxi_tvalid[n];
          wire mux_last  = |(muxi_tvalid[n] & muxi_tlast[n]);

          // Track the input packet state
          reg [0:0] pkt_state = PKT_ST_HEAD;
          always @(posedge clk) begin
            if (reset) begin
              pkt_state <= PKT_ST_HEAD;
            end else if (mux_valid & mux_ready) begin
              pkt_state <= mux_last ? PKT_ST_HEAD : PKT_ST_BODY;
            end
          end

          // The switch requires the allocation to stay valid until the
          // end of the packet. We also might need to keep the previous
          // packet's allocation to compute the current one
          reg  [NPORTS_W-1:0] prev_sw_alloc = {NPORTS_W{1'b0}};
          reg  [NPORTS_W-1:0] pkt_sw_alloc  = {NPORTS_W{1'b0}};
          wire [NPORTS_W-1:0] muxi_sw_alloc = (mux_valid && pkt_state == PKT_ST_HEAD) ?
            compute_mux_alloc(muxi_tvalid[n], prev_sw_alloc) : pkt_sw_alloc;

          always @(posedge clk) begin
            if (reset) begin
              prev_sw_alloc <= {NPORTS_W{1'b0}};
              pkt_sw_alloc <= {NPORTS_W{1'b0}};
            end else if (mux_valid & mux_ready) begin
              if (pkt_state == PKT_ST_HEAD)
                pkt_sw_alloc <= muxi_sw_alloc;
              if (mux_last)
                prev_sw_alloc <= muxi_sw_alloc;
            end
          end

          axis_switch #(
            .DATA_W    (CHDR_W(n)),
            .DEST_W    (1        ),
            .IN_PORTS  (NPORTS   ),
            .OUT_PORTS (1        ),
            .PIPELINE  (0        )
          ) axis_switch_mux (
            .clk           (clk                                 ),
            .reset         (reset                               ),
            .s_axis_tdata  (muxi_tdata_repacked                  ),
            .s_axis_tdest  ({NPORTS{1'b0}} /* Unused */         ),
            .s_axis_tlast  (muxi_tlast [n]                      ),
            .s_axis_tvalid (muxi_tvalid[n]                      ),
            .s_axis_tready (muxi_tready[n]                      ),
            .s_axis_alloc  (muxi_sw_alloc                       ),
            .m_axis_tdata  (m_axis_tdata [(n*PORT_W)+:CHDR_W(n)]),
            .m_axis_tdest  (/* Unused */                        ),
            .m_axis_tlast  (m_axis_tlast [n]                    ),
            .m_axis_tvalid (m_axis_tvalid[n]                    ),
            .m_axis_tready (m_axis_tready[n]                    )
          );
        end else begin : gen_not_performance
          // axi_mux has an additional bubble cycle but the logic
          // to allocate an input port has fewer levels and takes
          // up fewer resources.
          axi_mux #(
            .PRIO          (MUX_ALLOC == "PRIO"         ),
            .WIDTH         (CHDR_W(n)                   ),
            .SIZE          (NPORTS                      ),
            .PRE_FIFO_SIZE (OPTIMIZE == "TIMING" ? 1 : 0),
            .POST_FIFO_SIZE(1                           )
          ) axi_mux_i (
            .clk     (clk                                 ),
            .reset   (reset                               ),
            .clear   (1'b0                                ),
            .i_tdata (muxi_tdata_repacked                  ),
            .i_tlast (muxi_tlast   [n]                    ),
            .i_tvalid(muxi_tvalid  [n]                    ),
            .i_tready(muxi_tready  [n]                    ),
            .o_tdata (m_axis_tdata [(n*PORT_W)+:CHDR_W(n)]),
            .o_tlast (m_axis_tlast [n]                    ),
            .o_tvalid(m_axis_tvalid[n]                    ),
            .o_tready(m_axis_tready[n]                    )
          );
        end
      end
    end
  endgenerate


endmodule
