//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_split_stream
//
// Description:
//
//   This RFNoC block takes in a single CHDR stream and duplicates it, creating
//   NUM_BRANCHES output streams for each input stream.
//
//   The NUM_PORTS parameter corresponds to the number of inputs that you want
//   to split. That is, the block creates NUM_PORTS instances of 1:NUM_BRANCHES
//   splitters. The figure below illustrates how the CHDR ports are ordered
//   when NUM_PORTS = 2 and NUM_BRANCHES = 3.
//
//                  ┌──────────┐
//     Stream A --->│0        0│---> Stream A
//     Stream B --->│1        1│---> Stream B
//                  │         2│---> Stream A
//                  │         3│---> Stream B
//                  │         4│---> Stream A
//                  │         5│---> Stream B
//                  └──────────┘
//
// Parameters:
//
//   THIS_PORTID  : Control crossbar port to which this block is connected
//   CHDR_W       : AXIS-CHDR data bus width
//   MTU          : Maximum transmission unit (i.e., maximum packet size in
//                  CHDR words is 2**MTU).
//   NUM_PORTS    : Number of input ports or number of splitters to create
//   NUM_BRANCHES : Number of branches at the output of each splitter
//

`default_nettype none


module rfnoc_block_split_stream #(
  parameter [9:0] THIS_PORTID  = 10'd0,
  parameter       CHDR_W       = 64,
  parameter [5:0] MTU          = 10,
  parameter       NUM_PORTS    = 1,
  parameter       NUM_BRANCHES = 2
)(
  // RFNoC Framework Clocks and Resets
  input  wire                   rfnoc_chdr_clk,
  input  wire                   rfnoc_ctrl_clk,
  // RFNoC Backend Interface
  input  wire [511:0]           rfnoc_core_config,
  output wire [511:0]           rfnoc_core_status,
  // AXIS-CHDR Input Ports (from framework)
  input  wire [NUM_PORTS*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [NUM_PORTS-1:0]        s_rfnoc_chdr_tlast,
  input  wire [NUM_PORTS-1:0]        s_rfnoc_chdr_tvalid,
  output wire [NUM_PORTS-1:0]        s_rfnoc_chdr_tready,
  // AXIS-CHDR Output Ports (to framework)
  output wire [NUM_BRANCHES*NUM_PORTS*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [NUM_BRANCHES*NUM_PORTS-1:0]        m_rfnoc_chdr_tlast,
  output wire [NUM_BRANCHES*NUM_PORTS-1:0]        m_rfnoc_chdr_tvalid,
  input  wire [NUM_BRANCHES*NUM_PORTS-1:0]        m_rfnoc_chdr_tready,
  // AXIS-Ctrl Input Port (from framework)
  input  wire [31:0]            s_rfnoc_ctrl_tdata,
  input  wire                   s_rfnoc_ctrl_tlast,
  input  wire                   s_rfnoc_ctrl_tvalid,
  output wire                   s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Output Port (to framework)
  output wire [31:0]            m_rfnoc_ctrl_tdata,
  output wire                   m_rfnoc_ctrl_tlast,
  output wire                   m_rfnoc_ctrl_tvalid,
  input  wire                   m_rfnoc_ctrl_tready
);

  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  // Framework to User Logic: in
  wire [NUM_PORTS*CHDR_W-1:0] in_chdr_tdata;
  wire [       NUM_PORTS-1:0] in_chdr_tlast;
  wire [       NUM_PORTS-1:0] in_chdr_tvalid;
  wire [       NUM_PORTS-1:0] in_chdr_tready;
  // User Logic to Framework: out
  wire [NUM_BRANCHES*NUM_PORTS*CHDR_W-1:0] out_chdr_tdata;
  wire [       NUM_BRANCHES*NUM_PORTS-1:0] out_chdr_tlast;
  wire [       NUM_BRANCHES*NUM_PORTS-1:0] out_chdr_tvalid;
  wire [       NUM_BRANCHES*NUM_PORTS-1:0] out_chdr_tready;


  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  wire rfnoc_chdr_rst;

  noc_shell_split_stream #(
    .CHDR_W       (CHDR_W),
    .THIS_PORTID  (THIS_PORTID),
    .MTU          (MTU),
    .NUM_PORTS    (NUM_PORTS),
    .NUM_BRANCHES (NUM_BRANCHES)
  ) noc_shell_split_stream_i (
    //---------------------
    // Framework Interface
    //---------------------
    
    // Clock Inputs
    .rfnoc_chdr_clk      (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk      (rfnoc_ctrl_clk),
    // Reset Outputs
    .rfnoc_chdr_rst      (rfnoc_chdr_rst),
    .rfnoc_ctrl_rst      (),
    // RFNoC Backend Interface
    .rfnoc_core_config   (rfnoc_core_config),
    .rfnoc_core_status   (rfnoc_core_status),
    // CHDR Input Ports  (from framework)
    .s_rfnoc_chdr_tdata  (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast  (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready (s_rfnoc_chdr_tready),
    // CHDR Output Ports (to framework)
    .m_rfnoc_chdr_tdata  (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast  (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready (m_rfnoc_chdr_tready),
    // AXIS-Ctrl Input Port (from framework)
    .s_rfnoc_ctrl_tdata  (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast  (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready (s_rfnoc_ctrl_tready),
    // AXIS-Ctrl Output Port (to framework)
    .m_rfnoc_ctrl_tdata  (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast  (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready (m_rfnoc_ctrl_tready),
    
    //---------------------
    // Client Interface
    //---------------------
    
    // AXIS-CHDR Clock and Reset
    .axis_chdr_clk       (),
    .axis_chdr_rst       (),
    // AXIS-CHDR to User Logic
    .m_in_chdr_tdata     (in_chdr_tdata),
    .m_in_chdr_tlast     (in_chdr_tlast),
    .m_in_chdr_tvalid    (in_chdr_tvalid),
    .m_in_chdr_tready    (in_chdr_tready),
    // AXIS-CHDR from User Logic
    .s_out_chdr_tdata    (out_chdr_tdata),
    .s_out_chdr_tlast    (out_chdr_tlast),
    .s_out_chdr_tvalid   (out_chdr_tvalid),
    .s_out_chdr_tready   (out_chdr_tready)
  );


  //---------------------------------------------------------------------------
  // Split Stream
  //---------------------------------------------------------------------------

  genvar port;
  generate
    for (port = 0; port < NUM_PORTS; port = port+1) begin : gen_splitters
      wire [             (CHDR_W+1)-1:0] in_tdata;
      wire                               in_tvalid;
      wire                               in_tready;
      wire [NUM_BRANCHES*(CHDR_W+1)-1:0] out_tdata;
      wire [           NUM_BRANCHES-1:0] out_tvalid;
      wire [           NUM_BRANCHES-1:0] out_tready;

      // Connect the NoC shell master data port to the input of the splitter
      assign in_tdata = { in_chdr_tlast[port], in_chdr_tdata[port*CHDR_W +: CHDR_W] };
      assign in_tvalid = in_chdr_tvalid[port];
      assign in_chdr_tready[port] = in_tready;

      // A single 1:NUM_BRANCHES splitter
      axis_split #(
        .DATA_W    (CHDR_W+1),
        .NUM_PORTS (NUM_BRANCHES),
        .INPUT_REG (1)
      ) axis_split_i (
        .clk           (rfnoc_chdr_clk),
        .rst           (rfnoc_chdr_rst),
        .s_axis_tdata  (in_tdata),
        .s_axis_tvalid (in_tvalid),
        .s_axis_tready (in_tready),
        .m_axis_tdata  (out_tdata),
        .m_axis_tvalid (out_tvalid),
        .m_axis_tready (out_tready)
      );

      // Connect the outputs of the splitter to the NoC shell slave data ports
      genvar split;
      for (split = 0; split < NUM_BRANCHES; split = split+1) begin : gen_outputs
        // Connect port index "split" of the splitter output to port index
        // (split*NUM_PORTS + port) of the NoC shell slave data port.
        assign out_chdr_tlast[split*NUM_PORTS+port] =
          out_tdata[split*(CHDR_W+1)+CHDR_W];
        assign out_chdr_tdata[(split*NUM_PORTS+port)*CHDR_W +: CHDR_W] =
          out_tdata[split*(CHDR_W+1) +: CHDR_W];
        assign out_chdr_tvalid[split*NUM_PORTS+port] =
          out_tvalid[split];
        assign out_tready[split] =
          out_chdr_tready[split*NUM_PORTS+port];
      end
    end
  endgenerate

endmodule // rfnoc_block_split_stream


`default_nettype wire
