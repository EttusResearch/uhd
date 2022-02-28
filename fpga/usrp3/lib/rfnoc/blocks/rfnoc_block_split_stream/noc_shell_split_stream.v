//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: noc_shell_split_stream
//
// Description:
//
//   This is a tool-generated NoC-shell for the split_stream block.
//   See the RFNoC specification for more information about NoC shells.
//
// Parameters:
//
//   THIS_PORTID : Control crossbar port to which this block is connected
//   CHDR_W      : AXIS-CHDR data bus width
//   MTU         : Maximum transmission unit (i.e., maximum packet size in
//                 CHDR words is 2**MTU).
//

`default_nettype none


module noc_shell_split_stream #(
  parameter [9:0] THIS_PORTID     = 10'd0,
  parameter       CHDR_W          = 64,
  parameter [5:0] MTU             = 10,
  parameter       NUM_PORTS       = 1,
  parameter       NUM_BRANCHES    = 2
) (
  //---------------------
  // Framework Interface
  //---------------------

  // RFNoC Framework Clocks
  input  wire rfnoc_chdr_clk,
  input  wire rfnoc_ctrl_clk,

  // NoC Shell Generated Resets
  output wire rfnoc_chdr_rst,
  output wire rfnoc_ctrl_rst,

  // RFNoC Backend Interface
  input  wire [511:0]          rfnoc_core_config,
  output wire [511:0]          rfnoc_core_status,

  // AXIS-CHDR Input Ports (from framework)
  input  wire [(0+NUM_PORTS)*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [(0+NUM_PORTS)-1:0]        s_rfnoc_chdr_tlast,
  input  wire [(0+NUM_PORTS)-1:0]        s_rfnoc_chdr_tvalid,
  output wire [(0+NUM_PORTS)-1:0]        s_rfnoc_chdr_tready,
  // AXIS-CHDR Output Ports (to framework)
  output wire [(0+NUM_PORTS*NUM_BRANCHES)*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [(0+NUM_PORTS*NUM_BRANCHES)-1:0]        m_rfnoc_chdr_tlast,
  output wire [(0+NUM_PORTS*NUM_BRANCHES)-1:0]        m_rfnoc_chdr_tvalid,
  input  wire [(0+NUM_PORTS*NUM_BRANCHES)-1:0]        m_rfnoc_chdr_tready,

  // AXIS-Ctrl Control Input Port (from framework)
  input  wire [31:0]           s_rfnoc_ctrl_tdata,
  input  wire                  s_rfnoc_ctrl_tlast,
  input  wire                  s_rfnoc_ctrl_tvalid,
  output wire                  s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Control Output Port (to framework)
  output wire [31:0]           m_rfnoc_ctrl_tdata,
  output wire                  m_rfnoc_ctrl_tlast,
  output wire                  m_rfnoc_ctrl_tvalid,
  input  wire                  m_rfnoc_ctrl_tready,

  //---------------------
  // Client Interface
  //---------------------

  // AXIS-CHDR Clock and Reset
  output wire               axis_chdr_clk,
  output wire               axis_chdr_rst,
  // Framework to User Logic: in
  output wire [NUM_PORTS*CHDR_W-1:0]  m_in_chdr_tdata,
  output wire [NUM_PORTS-1:0]         m_in_chdr_tlast,
  output wire [NUM_PORTS-1:0]         m_in_chdr_tvalid,
  input  wire [NUM_PORTS-1:0]         m_in_chdr_tready,
  // User Logic to Framework: out
  input  wire [NUM_PORTS*NUM_BRANCHES*CHDR_W-1:0]  s_out_chdr_tdata,
  input  wire [NUM_PORTS*NUM_BRANCHES-1:0]         s_out_chdr_tlast,
  input  wire [NUM_PORTS*NUM_BRANCHES-1:0]         s_out_chdr_tvalid,
  output wire [NUM_PORTS*NUM_BRANCHES-1:0]         s_out_chdr_tready
);

  //---------------------------------------------------------------------------
  //  Backend Interface
  //---------------------------------------------------------------------------

  wire         data_i_flush_en;
  wire [31:0]  data_i_flush_timeout;
  wire [63:0]  data_i_flush_active;
  wire [63:0]  data_i_flush_done;
  wire         data_o_flush_en;
  wire [31:0]  data_o_flush_timeout;
  wire [63:0]  data_o_flush_active;
  wire [63:0]  data_o_flush_done;

  backend_iface #(
    .NOC_ID        (32'h57570000),
    .NUM_DATA_I    (0+NUM_PORTS),
    .NUM_DATA_O    (0+NUM_PORTS*NUM_BRANCHES),
    .CTRL_FIFOSIZE ($clog2(32)),
    .MTU           (MTU)
  ) backend_iface_i (
    .rfnoc_chdr_clk       (rfnoc_chdr_clk),
    .rfnoc_chdr_rst       (rfnoc_chdr_rst),
    .rfnoc_ctrl_clk       (rfnoc_ctrl_clk),
    .rfnoc_ctrl_rst       (rfnoc_ctrl_rst),
    .rfnoc_core_config    (rfnoc_core_config),
    .rfnoc_core_status    (rfnoc_core_status),
    .data_i_flush_en      (data_i_flush_en),
    .data_i_flush_timeout (data_i_flush_timeout),
    .data_i_flush_active  (data_i_flush_active),
    .data_i_flush_done    (data_i_flush_done),
    .data_o_flush_en      (data_o_flush_en),
    .data_o_flush_timeout (data_o_flush_timeout),
    .data_o_flush_active  (data_o_flush_active),
    .data_o_flush_done    (data_o_flush_done)
  );

  //---------------------------------------------------------------------------
  //  Control Path
  //---------------------------------------------------------------------------

  // No control path for this block
  assign s_rfnoc_ctrl_tready =  1'b1;
  assign m_rfnoc_ctrl_tdata  = 32'b0;
  assign m_rfnoc_ctrl_tlast  =  1'b0;
  assign m_rfnoc_ctrl_tvalid =  1'b0;

  //---------------------------------------------------------------------------
  //  Data Path
  //---------------------------------------------------------------------------

  genvar i;

  assign axis_chdr_clk = rfnoc_chdr_clk;
  assign axis_chdr_rst = rfnoc_chdr_rst;

  //---------------------
  // Input Data Paths
  //---------------------

  for (i = 0; i < NUM_PORTS; i = i + 1) begin: gen_input_in
    chdr_to_chdr_data #(
      .CHDR_W (CHDR_W)
    ) chdr_to_chdr_data_in_in (
      .axis_chdr_clk      (rfnoc_chdr_clk),
      .axis_chdr_rst      (rfnoc_chdr_rst),
      .s_axis_chdr_tdata  (s_rfnoc_chdr_tdata[(0+i)*CHDR_W+:CHDR_W]),
      .s_axis_chdr_tlast  (s_rfnoc_chdr_tlast[0+i]),
      .s_axis_chdr_tvalid (s_rfnoc_chdr_tvalid[0+i]),
      .s_axis_chdr_tready (s_rfnoc_chdr_tready[0+i]),
      .m_axis_chdr_tdata  (m_in_chdr_tdata[i*CHDR_W+:CHDR_W]),
      .m_axis_chdr_tlast  (m_in_chdr_tlast[i]),
      .m_axis_chdr_tvalid (m_in_chdr_tvalid[i]),
      .m_axis_chdr_tready (m_in_chdr_tready[i]),
      .flush_en           (data_i_flush_en),
      .flush_timeout      (data_i_flush_timeout),
      .flush_active       (data_i_flush_active[0+i]),
      .flush_done         (data_i_flush_done[0+i])
    );
  end

  //---------------------
  // Output Data Paths
  //---------------------

  for (i = 0; i < NUM_PORTS*NUM_BRANCHES; i = i + 1) begin: gen_output_out
    chdr_to_chdr_data #(
      .CHDR_W (CHDR_W)
    ) chdr_to_chdr_data_out_out (
      .axis_chdr_clk      (rfnoc_chdr_clk),
      .axis_chdr_rst      (rfnoc_chdr_rst),
      .m_axis_chdr_tdata  (m_rfnoc_chdr_tdata[(0+i)*CHDR_W+:CHDR_W]),
      .m_axis_chdr_tlast  (m_rfnoc_chdr_tlast[0+i]),
      .m_axis_chdr_tvalid (m_rfnoc_chdr_tvalid[0+i]),
      .m_axis_chdr_tready (m_rfnoc_chdr_tready[0+i]),
      .s_axis_chdr_tdata  (s_out_chdr_tdata[i*CHDR_W+:CHDR_W]),
      .s_axis_chdr_tlast  (s_out_chdr_tlast[i]),
      .s_axis_chdr_tvalid (s_out_chdr_tvalid[i]),
      .s_axis_chdr_tready (s_out_chdr_tready[i]),
      .flush_en           (data_o_flush_en),
      .flush_timeout      (data_o_flush_timeout),
      .flush_active       (data_o_flush_active[0+i]),
      .flush_done         (data_o_flush_done[0+i])
    );
  end

endmodule // noc_shell_split_stream


`default_nettype wire
