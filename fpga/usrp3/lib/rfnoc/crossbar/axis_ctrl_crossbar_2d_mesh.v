//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_ctrl_crossbar_2d_mesh
// Description: 
//   This module implements a 2-dimentional (2d) mesh network (mesh) crossbar
//   for AXIS-CTRL traffic. Supports mesh and torus topologies.
//   It uses AXI-Stream for all of its links. 
//   The torus topology, routing algorithms and the router architecture is 
//   described in README.md in this directory. 
// Parameters:
//   - WIDTH: Width of the AXI-Stream data bus
//   - DIM_SIZE: Number of routers alone one dimension (# Nodes = DIM_SIZE * DIM_SIZE)
//   - TOPOLOGY: Is this a mesh (MESH) or a torus (TORUS) topology
//   - INGRESS_BUFF_SIZE: log2 of the ingress terminal buffer size (in words)
//   - ROUTER_BUFF_SIZE: log2 of the ingress inter-router buffer size (in words)
//   - ROUTING_ALLOC: Algorithm to allocate routing paths between routers.
//     * WORMHOLE: Allocate route as soon as first word in pkt arrives
//     * CUT-THROUGH: Allocate route only after the full pkt arrives
//   - SWITCH_ALLOC: Algorithm to allocate the switch
//     * PRIO: Priority based. Priority: Y-dim > X-dim > Term
//     * ROUND-ROBIN: Round robin input port allocation
//   - DEADLOCK_TIMEOUT: Number of cycles to wait until a deadlock is detected
// Signals:
//   - s_axis_*: Slave port for router (flattened)
//   - m_axis_*: Master port for router (flattened)
//

module axis_ctrl_crossbar_2d_mesh #(
  parameter DIM_SIZE          = 4,
  parameter WIDTH             = 64,
  parameter TOPOLOGY          = "MESH",
  parameter INGRESS_BUFF_SIZE = 5,
  parameter ROUTER_BUFF_SIZE  = 5,
  parameter ROUTING_ALLOC     = "WORMHOLE",
  parameter SWITCH_ALLOC      = "PRIO",
  parameter DEADLOCK_TIMEOUT  = 16384
) (
  input  wire                                 clk,
  input  wire                                 reset,
  // Inputs
  input  wire [(DIM_SIZE*DIM_SIZE*WIDTH)-1:0] s_axis_tdata,
  input  wire [DIM_SIZE*DIM_SIZE-1:0]         s_axis_tlast,
  input  wire [DIM_SIZE*DIM_SIZE-1:0]         s_axis_tvalid,
  output wire [DIM_SIZE*DIM_SIZE-1:0]         s_axis_tready,
  // Output
  output wire [(DIM_SIZE*DIM_SIZE*WIDTH)-1:0] m_axis_tdata,
  output wire [DIM_SIZE*DIM_SIZE-1:0]         m_axis_tlast,
  output wire [DIM_SIZE*DIM_SIZE-1:0]         m_axis_tvalid,
  input  wire [DIM_SIZE*DIM_SIZE-1:0]         m_axis_tready,
  // Deadlock alert
  output wire                                 deadlock_detected
);

  `include "mesh_node_mapping.vh"

  //-------------------------------------------------------
  // Unflatten input and output ports
  //-------------------------------------------------------

  wire [WIDTH-1:0] i_tdata_arr [0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             i_tlast_arr [0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             i_tvalid_arr[0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             i_tready_arr[0:DIM_SIZE-1][0:DIM_SIZE-1];

  wire [WIDTH-1:0] o_tdata_arr [0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             o_tlast_arr [0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             o_tvalid_arr[0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             o_tready_arr[0:DIM_SIZE-1][0:DIM_SIZE-1];

  wire             clear_routers = deadlock_detected;

  genvar p,x,y;
  generate
    for (p = 0; p < DIM_SIZE*DIM_SIZE; p=p+1) begin
      assign i_tdata_arr[node_to_ydst(p)][node_to_xdst(p)] = s_axis_tdata[p*WIDTH +: WIDTH];
      assign i_tlast_arr[node_to_ydst(p)][node_to_xdst(p)] = s_axis_tlast[p];
      assign i_tvalid_arr[node_to_ydst(p)][node_to_xdst(p)] = s_axis_tvalid[p];
      assign s_axis_tready[p] = i_tready_arr[node_to_ydst(p)][node_to_xdst(p)] | clear_routers;

      assign m_axis_tdata[p*WIDTH +: WIDTH] = o_tdata_arr[node_to_ydst(p)][node_to_xdst(p)];
      assign m_axis_tlast[p] = o_tlast_arr [node_to_ydst(p)][node_to_xdst(p)];
      assign m_axis_tvalid[p] = o_tvalid_arr[node_to_ydst(p)][node_to_xdst(p)] & ~clear_routers;
      assign o_tready_arr[node_to_ydst(p)][node_to_xdst(p)] = m_axis_tready[p];
    end
  endgenerate

  //-------------------------------------------------------
  // Instantiate routers
  //-------------------------------------------------------

  wire [WIDTH-1:0] e2w_tdata_arr [0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             e2w_tdest_arr [0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             e2w_tlast_arr [0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             e2w_tvalid_arr[0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             e2w_tready_arr[0:DIM_SIZE-1][0:DIM_SIZE-1];

  wire [WIDTH-1:0] w2e_tdata_arr [0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             w2e_tdest_arr [0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             w2e_tlast_arr [0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             w2e_tvalid_arr[0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             w2e_tready_arr[0:DIM_SIZE-1][0:DIM_SIZE-1];

  wire [WIDTH-1:0] n2s_tdata_arr [0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             n2s_tdest_arr [0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             n2s_tlast_arr [0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             n2s_tvalid_arr[0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             n2s_tready_arr[0:DIM_SIZE-1][0:DIM_SIZE-1];

  wire [WIDTH-1:0] s2n_tdata_arr [0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             s2n_tdest_arr [0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             s2n_tlast_arr [0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             s2n_tvalid_arr[0:DIM_SIZE-1][0:DIM_SIZE-1];
  wire             s2n_tready_arr[0:DIM_SIZE-1][0:DIM_SIZE-1];

  localparam              N    = DIM_SIZE;
  localparam              NEND = DIM_SIZE - 1;
  localparam [WIDTH-1:0]  ZERO = {WIDTH{1'b0}};

  generate
    for (y = 0; y < DIM_SIZE; y=y+1) begin: ydim
      for (x = 0; x < DIM_SIZE; x=x+1) begin: xdim
        if (TOPOLOGY == "MESH") begin
          mesh_2d_dor_router_single_sw #(
            .WIDTH              (WIDTH),
            .DIM_SIZE           (DIM_SIZE),
            .XB_ADDR_X          (x),
            .XB_ADDR_Y          (y),
            .TERM_BUFF_SIZE     (INGRESS_BUFF_SIZE),
            .XB_BUFF_SIZE       (ROUTER_BUFF_SIZE),
            .ROUTING_ALLOC      (ROUTING_ALLOC),
            .SWITCH_ALLOC       (SWITCH_ALLOC)
          ) rtr_i (
            // Clock and reset
            .clk                (clk),
            .reset              (reset | clear_routers),
            // Terminals
            .s_axis_ter_tdata   (i_tdata_arr [y][x]),
            .s_axis_ter_tlast   (i_tlast_arr [y][x]),
            .s_axis_ter_tvalid  (i_tvalid_arr[y][x]),
            .s_axis_ter_tready  (i_tready_arr[y][x]),
            .m_axis_ter_tdata   (o_tdata_arr [y][x]),
            .m_axis_ter_tlast   (o_tlast_arr [y][x]),
            .m_axis_ter_tvalid  (o_tvalid_arr[y][x]),
            .m_axis_ter_tready  (o_tready_arr[y][x]),
            // West connections
            .s_axis_wst_tdata   ((x != 0)    ? e2w_tdata_arr [y][x]         : ZERO),
            .s_axis_wst_tdest   ((x != 0)    ? e2w_tdest_arr [y][x]         : 1'b0),
            .s_axis_wst_tlast   ((x != 0)    ? e2w_tlast_arr [y][x]         : 1'b0),
            .s_axis_wst_tvalid  ((x != 0)    ? e2w_tvalid_arr[y][x]         : 1'b0),
            .s_axis_wst_tready  (              e2w_tready_arr[y][x]               ),
            .m_axis_wst_tdata   (              w2e_tdata_arr [y][(x+N-1)%N]       ),
            .m_axis_wst_tdest   (              w2e_tdest_arr [y][(x+N-1)%N]       ),
            .m_axis_wst_tlast   (              w2e_tlast_arr [y][(x+N-1)%N]       ),
            .m_axis_wst_tvalid  (              w2e_tvalid_arr[y][(x+N-1)%N]       ),
            .m_axis_wst_tready  ((x != 0)    ? w2e_tready_arr[y][(x+N-1)%N] : 1'b1),
            // East connections
            .s_axis_est_tdata   ((x != NEND) ? w2e_tdata_arr [y][x]         : ZERO),
            .s_axis_est_tdest   ((x != NEND) ? w2e_tdest_arr [y][x]         : 1'b0),
            .s_axis_est_tlast   ((x != NEND) ? w2e_tlast_arr [y][x]         : 1'b0),
            .s_axis_est_tvalid  ((x != NEND) ? w2e_tvalid_arr[y][x]         : 1'b0),
            .s_axis_est_tready  (              w2e_tready_arr[y][x]               ),
            .m_axis_est_tdata   (              e2w_tdata_arr [y][(x+1)%N]         ),
            .m_axis_est_tdest   (              e2w_tdest_arr [y][(x+1)%N]         ),
            .m_axis_est_tlast   (              e2w_tlast_arr [y][(x+1)%N]         ),
            .m_axis_est_tvalid  (              e2w_tvalid_arr[y][(x+1)%N]         ),
            .m_axis_est_tready  ((x != NEND) ? e2w_tready_arr[y][(x+1)%N]   : 1'b1),
            // North connections
            .s_axis_nor_tdata   ((y != 0)    ? s2n_tdata_arr [y][x]         : ZERO),
            .s_axis_nor_tdest   ((y != 0)    ? s2n_tdest_arr [y][x]         : 1'b0),
            .s_axis_nor_tlast   ((y != 0)    ? s2n_tlast_arr [y][x]         : 1'b0),
            .s_axis_nor_tvalid  ((y != 0)    ? s2n_tvalid_arr[y][x]         : 1'b0),
            .s_axis_nor_tready  (              s2n_tready_arr[y][x]               ),
            .m_axis_nor_tdata   (              n2s_tdata_arr [(y+N-1)%N][x]       ),
            .m_axis_nor_tdest   (              n2s_tdest_arr [(y+N-1)%N][x]       ),
            .m_axis_nor_tlast   (              n2s_tlast_arr [(y+N-1)%N][x]       ),
            .m_axis_nor_tvalid  (              n2s_tvalid_arr[(y+N-1)%N][x]       ),
            .m_axis_nor_tready  ((y != 0)    ? n2s_tready_arr[(y+N-1)%N][x] : 1'b1),
            // South connections
            .s_axis_sou_tdata   ((y != NEND) ? n2s_tdata_arr [y][x]         : ZERO),
            .s_axis_sou_tdest   ((y != NEND) ? n2s_tdest_arr [y][x]         : 1'b0),
            .s_axis_sou_tlast   ((y != NEND) ? n2s_tlast_arr [y][x]         : 1'b0),
            .s_axis_sou_tvalid  ((y != NEND) ? n2s_tvalid_arr[y][x]         : 1'b0),
            .s_axis_sou_tready  (              n2s_tready_arr[y][x]               ),
            .m_axis_sou_tdata   (              s2n_tdata_arr [(y+1)%N][x]         ),
            .m_axis_sou_tdest   (              s2n_tdest_arr [(y+1)%N][x]         ),
            .m_axis_sou_tlast   (              s2n_tlast_arr [(y+1)%N][x]         ),
            .m_axis_sou_tvalid  (              s2n_tvalid_arr[(y+1)%N][x]         ),
            .m_axis_sou_tready  ((y != NEND) ? s2n_tready_arr[(y+1)%N][x]   : 1'b1)
          );
        end else begin
          torus_2d_dor_router_single_sw #(
            .WIDTH              (WIDTH),
            .DIM_SIZE           (DIM_SIZE),
            .XB_ADDR_X          (x),
            .XB_ADDR_Y          (y),
            .TERM_BUFF_SIZE     (INGRESS_BUFF_SIZE),
            .XB_BUFF_SIZE       (ROUTER_BUFF_SIZE),
            .ROUTING_ALLOC      (ROUTING_ALLOC),
            .SWITCH_ALLOC       (SWITCH_ALLOC)
          ) rtr_i (
            // Clock and reset
            .clk                (clk),
            .reset              (reset | clear_routers),
            // Terminals
            .s_axis_term_tdata  (i_tdata_arr [y][x]),
            .s_axis_term_tlast  (i_tlast_arr [y][x]),
            .s_axis_term_tvalid (i_tvalid_arr[y][x]),
            .s_axis_term_tready (i_tready_arr[y][x]),
            .m_axis_term_tdata  (o_tdata_arr [y][x]),
            .m_axis_term_tlast  (o_tlast_arr [y][x]),
            .m_axis_term_tvalid (o_tvalid_arr[y][x]),
            .m_axis_term_tready (o_tready_arr[y][x]),
            // X-dim connections
            .s_axis_xdim_tdata  (e2w_tdata_arr [y][x]      ),
            .s_axis_xdim_tdest  (e2w_tdest_arr [y][x]      ),
            .s_axis_xdim_tlast  (e2w_tlast_arr [y][x]      ),
            .s_axis_xdim_tvalid (e2w_tvalid_arr[y][x]      ),
            .s_axis_xdim_tready (e2w_tready_arr[y][x]      ),
            .m_axis_xdim_tdata  (e2w_tdata_arr [y][(x+1)%N]),
            .m_axis_xdim_tdest  (e2w_tdest_arr [y][(x+1)%N]),
            .m_axis_xdim_tlast  (e2w_tlast_arr [y][(x+1)%N]),
            .m_axis_xdim_tvalid (e2w_tvalid_arr[y][(x+1)%N]),
            .m_axis_xdim_tready (e2w_tready_arr[y][(x+1)%N]),
            // Y-dim connections
            .s_axis_ydim_tdata  (s2n_tdata_arr [y][x]      ),
            .s_axis_ydim_tdest  (s2n_tdest_arr [y][x]      ),
            .s_axis_ydim_tlast  (s2n_tlast_arr [y][x]      ),
            .s_axis_ydim_tvalid (s2n_tvalid_arr[y][x]      ),
            .s_axis_ydim_tready (s2n_tready_arr[y][x]      ),
            .m_axis_ydim_tdata  (s2n_tdata_arr [(y+1)%N][x]),
            .m_axis_ydim_tdest  (s2n_tdest_arr [(y+1)%N][x]),
            .m_axis_ydim_tlast  (s2n_tlast_arr [(y+1)%N][x]),
            .m_axis_ydim_tvalid (s2n_tvalid_arr[(y+1)%N][x]),
            .m_axis_ydim_tready (s2n_tready_arr[(y+1)%N][x])
          );
        end
      end
    end
  endgenerate

  //-------------------------------------------------------
  // Deadlock detector
  //-------------------------------------------------------
  // A deadlock is defined on an AXIS bus as an extended period
  // where tvlid=1 but tready=0. If at least one slave port is in
  // this state and none of the master ports are then this router
  // will go into a failsafe deadlock recovery mode. The DEADLOCK_TIMEOUT 
  // parameter defines the duration for which this condition has 
  // to be true. In deadlock recovery mode, all routers are held in reset
  // (thus losing all packets in flights) and all input ports are flushed.
  
  wire m_locked = |(m_axis_tvalid & ~m_axis_tready);
  wire s_locked = |(s_axis_tvalid & ~s_axis_tready);

  // A counter that tracks the duration for which the router is livelocked
  // If the livelock duration is higher than DEADLOCK_TIMEOUT then it is a
  // deadlock
  reg [$clog2(DEADLOCK_TIMEOUT)-1:0] deadlock_counter = DEADLOCK_TIMEOUT-1;
  always @(posedge clk) begin
    if (reset | ~(s_locked & ~m_locked)) begin
      deadlock_counter <= DEADLOCK_TIMEOUT-1;
    end else if (deadlock_counter != 'd0) begin
      deadlock_counter <= deadlock_counter - 1;
    end
  end

  // A counter that tracks the deadlock recovery period. If the slave ports
  // have no activity for DEADLOCK_TIMEOUT cycles then the router can
  // successfully come out of the deadlocked state.
  reg [$clog2(DEADLOCK_TIMEOUT)-1:0] deadlock_recover_counter = 'd0;
  always @(posedge clk) begin
    if (reset) begin
      deadlock_recover_counter <= 'd0;
    end else if (deadlock_detected) begin
      if (|s_axis_tvalid)
        deadlock_recover_counter <= DEADLOCK_TIMEOUT-1;
      else
        deadlock_recover_counter <= deadlock_recover_counter - 1;
    end else if (deadlock_counter == 'd0) begin
      deadlock_recover_counter <= DEADLOCK_TIMEOUT-1;
    end
  end
  assign deadlock_detected = (deadlock_recover_counter != 0);

endmodule
