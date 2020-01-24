//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_ctrl_crossbar_nxn
// Description: 
//   This module implements a 2-dimentional (2d) mesh network (mesh) crossbar
//   for AXIS-CTRL traffic. Supports mesh and torus topologies.
//   It uses AXI-Stream for all of its links. 
//   The torus topology, routing algorithms and the router architecture is 
//   described in README.md in this directory. 
// Parameters:
//   - WIDTH: Width of the AXI-Stream data bus
//   - NPORTS: Number of ports (maximum 1024)
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

module axis_ctrl_crossbar_nxn #(
  parameter WIDTH             = 32,
  parameter NPORTS            = 10,
  parameter TOPOLOGY          = "TORUS",
  parameter INGRESS_BUFF_SIZE = 5,
  parameter ROUTER_BUFF_SIZE  = 5,
  parameter ROUTING_ALLOC     = "WORMHOLE",
  parameter SWITCH_ALLOC      = "PRIO",
  parameter DEADLOCK_TIMEOUT  = 16384
) (
  input  wire                      clk,
  input  wire                      reset,
  // Inputs
  input  wire [(NPORTS*WIDTH)-1:0] s_axis_tdata,
  input  wire [NPORTS-1:0]         s_axis_tlast,
  input  wire [NPORTS-1:0]         s_axis_tvalid,
  output wire [NPORTS-1:0]         s_axis_tready,
  // Output
  output wire [(NPORTS*WIDTH)-1:0] m_axis_tdata,
  output wire [NPORTS-1:0]         m_axis_tlast,
  output wire [NPORTS-1:0]         m_axis_tvalid,
  input  wire [NPORTS-1:0]         m_axis_tready,
  // Deadlock alert
  output wire                      deadlock_detected
);

  function integer csqrt_max1024;
    input integer value;
    integer i;
  begin
    csqrt_max1024 = 1;
    for (i = 1; i <= 32; i = i + 1)  // sqrt(1024) = 32
      csqrt_max1024 = csqrt_max1024 + (i*i < value ? 1 : 0);
  end
  endfunction

  localparam integer DIM_SIZE = csqrt_max1024(NPORTS);

  wire [(DIM_SIZE*DIM_SIZE*WIDTH)-1:0] i_tdata,  o_tdata ;
  wire [DIM_SIZE*DIM_SIZE-1:0]         i_tlast,  o_tlast ;
  wire [DIM_SIZE*DIM_SIZE-1:0]         i_tvalid, o_tvalid;
  wire [DIM_SIZE*DIM_SIZE-1:0]         i_tready, o_tready;

  // axis_ctrl_crossbar_2d_mesh needs to scale up in squares
  // i.e. 4, 9, 16, 25, ... but NPORTS can be any number, so
  // instantiate the next highest square number of ports and
  // terminate the rest.
  axis_ctrl_crossbar_2d_mesh #(
    .WIDTH            (WIDTH),
    .DIM_SIZE         (DIM_SIZE),
    .TOPOLOGY         (TOPOLOGY),
    .INGRESS_BUFF_SIZE(INGRESS_BUFF_SIZE),
    .ROUTER_BUFF_SIZE (ROUTER_BUFF_SIZE),
    .ROUTING_ALLOC    (ROUTING_ALLOC),
    .SWITCH_ALLOC     (SWITCH_ALLOC),
    .DEADLOCK_TIMEOUT (DEADLOCK_TIMEOUT)
  ) router_dut_i (
    .clk              (clk),
    .reset            (reset),
    .s_axis_tdata     (i_tdata),
    .s_axis_tlast     (i_tlast),
    .s_axis_tvalid    (i_tvalid),
    .s_axis_tready    (i_tready),
    .m_axis_tdata     (o_tdata), 
    .m_axis_tlast     (o_tlast),
    .m_axis_tvalid    (o_tvalid), 
    .m_axis_tready    (o_tready),
    .deadlock_detected(deadlock_detected)
  );

  // Connect the bottom NPORTS to the IO
  assign i_tdata[(NPORTS*WIDTH)-1:0] = s_axis_tdata;
  assign i_tlast[NPORTS-1:0]         = s_axis_tlast;
  assign i_tvalid[NPORTS-1:0]        = s_axis_tvalid;
  assign s_axis_tready               = i_tready[NPORTS-1:0];

  assign m_axis_tdata                = o_tdata[(NPORTS*WIDTH)-1:0];
  assign m_axis_tlast                = o_tlast[NPORTS-1:0];
  assign m_axis_tvalid               = o_tvalid[NPORTS-1:0];
  assign o_tready[NPORTS-1:0]        = m_axis_tready;

  // Terminate the rest
  genvar i;
  generate for (i = NPORTS; i < (DIM_SIZE*DIM_SIZE); i = i + 1) begin: ports
    axis_port_terminator #(.DATA_W(WIDTH)) term_i (
      .clk          (clk),
      .reset        (reset),
      .s_axis_tdata (o_tdata[(i*WIDTH)+:WIDTH]),
      .s_axis_tlast (o_tlast[i]),
      .s_axis_tvalid(o_tvalid[i]),
      .s_axis_tready(o_tready[i]),
      .m_axis_tdata (i_tdata[(i*WIDTH)+:WIDTH]),
      .m_axis_tlast (i_tlast[i]),
      .m_axis_tvalid(i_tvalid[i]),
      .m_axis_tready(i_tready[i]),
      .pkts_dropped ()
    );
  end endgenerate

endmodule
