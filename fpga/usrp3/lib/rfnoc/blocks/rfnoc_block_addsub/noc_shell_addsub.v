//
// Copyright 2022 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: noc_shell_addsub
//
// Description:
//
//   This is a tool-generated NoC-shell for the addsub block.
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


module noc_shell_addsub #(
  parameter [9:0] THIS_PORTID     = 10'd0,
  parameter       CHDR_W          = 64,
  parameter [5:0] MTU             = 10,
  parameter       USE_IMPL        = "Verilog"
) (
  //---------------------
  // Framework Interface
  //---------------------

  // RFNoC Framework Clocks
  input  wire rfnoc_chdr_clk,
  input  wire rfnoc_ctrl_clk,
  input  wire ce_clk,

  // NoC Shell Generated Resets
  output wire rfnoc_chdr_rst,
  output wire rfnoc_ctrl_rst,
  output wire ce_rst,

  // RFNoC Backend Interface
  input  wire [511:0]          rfnoc_core_config,
  output wire [511:0]          rfnoc_core_status,

  // AXIS-CHDR Input Ports (from framework)
  input  wire [(2)*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [(2)-1:0]        s_rfnoc_chdr_tlast,
  input  wire [(2)-1:0]        s_rfnoc_chdr_tvalid,
  output wire [(2)-1:0]        s_rfnoc_chdr_tready,
  // AXIS-CHDR Output Ports (to framework)
  output wire [(2)*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [(2)-1:0]        m_rfnoc_chdr_tlast,
  output wire [(2)-1:0]        m_rfnoc_chdr_tvalid,
  input  wire [(2)-1:0]        m_rfnoc_chdr_tready,

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

  // AXI-Stream Payload Context Clock and Reset
  output wire               axis_data_clk,
  output wire               axis_data_rst,
  // Payload Stream to User Logic: in_a
  output wire [32*1-1:0]    m_in_a_payload_tdata,
  output wire [1-1:0]       m_in_a_payload_tkeep,
  output wire               m_in_a_payload_tlast,
  output wire               m_in_a_payload_tvalid,
  input  wire               m_in_a_payload_tready,
  // Context Stream to User Logic: in_a
  output wire [CHDR_W-1:0]  m_in_a_context_tdata,
  output wire [3:0]         m_in_a_context_tuser,
  output wire               m_in_a_context_tlast,
  output wire               m_in_a_context_tvalid,
  input  wire               m_in_a_context_tready,
  // Payload Stream to User Logic: in_b
  output wire [32*1-1:0]    m_in_b_payload_tdata,
  output wire [1-1:0]       m_in_b_payload_tkeep,
  output wire               m_in_b_payload_tlast,
  output wire               m_in_b_payload_tvalid,
  input  wire               m_in_b_payload_tready,
  // Context Stream to User Logic: in_b
  output wire [CHDR_W-1:0]  m_in_b_context_tdata,
  output wire [3:0]         m_in_b_context_tuser,
  output wire               m_in_b_context_tlast,
  output wire               m_in_b_context_tvalid,
  input  wire               m_in_b_context_tready,
  // Payload Stream from User Logic: add
  input  wire [32*1-1:0]    s_add_payload_tdata,
  input  wire [0:0]         s_add_payload_tkeep,
  input  wire               s_add_payload_tlast,
  input  wire               s_add_payload_tvalid,
  output wire               s_add_payload_tready,
  // Context Stream from User Logic: add
  input  wire [CHDR_W-1:0]  s_add_context_tdata,
  input  wire [3:0]         s_add_context_tuser,
  input  wire               s_add_context_tlast,
  input  wire               s_add_context_tvalid,
  output wire               s_add_context_tready,
  // Payload Stream from User Logic: sub
  input  wire [32*1-1:0]    s_sub_payload_tdata,
  input  wire [0:0]         s_sub_payload_tkeep,
  input  wire               s_sub_payload_tlast,
  input  wire               s_sub_payload_tvalid,
  output wire               s_sub_payload_tready,
  // Context Stream from User Logic: sub
  input  wire [CHDR_W-1:0]  s_sub_context_tdata,
  input  wire [3:0]         s_sub_context_tuser,
  input  wire               s_sub_context_tlast,
  input  wire               s_sub_context_tvalid,
  output wire               s_sub_context_tready
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
    .NOC_ID        (32'hADD00000),
    .NUM_DATA_I    (2),
    .NUM_DATA_O    (2),
    .CTRL_FIFOSIZE ($clog2(2)),
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
  //  Reset Generation
  //---------------------------------------------------------------------------

  wire ce_rst_pulse;

  pulse_synchronizer #(.MODE ("POSEDGE")) pulse_synchronizer_ce (
    .clk_a(rfnoc_chdr_clk), .rst_a(1'b0), .pulse_a (rfnoc_chdr_rst), .busy_a (),
    .clk_b(ce_clk), .pulse_b (ce_rst_pulse)
  );

  pulse_stretch_min #(.LENGTH(32)) pulse_stretch_min_ce (
    .clk(ce_clk), .rst(1'b0),
    .pulse_in(ce_rst_pulse), .pulse_out(ce_rst)
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

  assign axis_data_clk = ce_clk;
  assign axis_data_rst = ce_rst;

  //---------------------
  // Input Data Paths
  //---------------------

  chdr_to_axis_pyld_ctxt #(
    .CHDR_W              (CHDR_W),
    .ITEM_W              (32),
    .NIPC                (1),
    .SYNC_CLKS           (0),
    .CONTEXT_FIFO_SIZE   ($clog2(2)),
    .PAYLOAD_FIFO_SIZE   ($clog2(2)),
    .CONTEXT_PREFETCH_EN (1)
  ) chdr_to_axis_pyld_ctxt_in_in_a (
    .axis_chdr_clk         (rfnoc_chdr_clk),
    .axis_chdr_rst         (rfnoc_chdr_rst),
    .axis_data_clk         (axis_data_clk),
    .axis_data_rst         (axis_data_rst),
    .s_axis_chdr_tdata     (s_rfnoc_chdr_tdata[(0)*CHDR_W+:CHDR_W]),
    .s_axis_chdr_tlast     (s_rfnoc_chdr_tlast[0]),
    .s_axis_chdr_tvalid    (s_rfnoc_chdr_tvalid[0]),
    .s_axis_chdr_tready    (s_rfnoc_chdr_tready[0]),
    .m_axis_payload_tdata  (m_in_a_payload_tdata),
    .m_axis_payload_tkeep  (m_in_a_payload_tkeep),
    .m_axis_payload_tlast  (m_in_a_payload_tlast),
    .m_axis_payload_tvalid (m_in_a_payload_tvalid),
    .m_axis_payload_tready (m_in_a_payload_tready),
    .m_axis_context_tdata  (m_in_a_context_tdata),
    .m_axis_context_tuser  (m_in_a_context_tuser),
    .m_axis_context_tlast  (m_in_a_context_tlast),
    .m_axis_context_tvalid (m_in_a_context_tvalid),
    .m_axis_context_tready (m_in_a_context_tready),
    .flush_en              (data_i_flush_en),
    .flush_timeout         (data_i_flush_timeout),
    .flush_active          (data_i_flush_active[0]),
    .flush_done            (data_i_flush_done[0])
  );

  chdr_to_axis_pyld_ctxt #(
    .CHDR_W              (CHDR_W),
    .ITEM_W              (32),
    .NIPC                (1),
    .SYNC_CLKS           (0),
    .CONTEXT_FIFO_SIZE   ($clog2(2)),
    .PAYLOAD_FIFO_SIZE   ($clog2(2)),
    .CONTEXT_PREFETCH_EN (1)
  ) chdr_to_axis_pyld_ctxt_in_in_b (
    .axis_chdr_clk         (rfnoc_chdr_clk),
    .axis_chdr_rst         (rfnoc_chdr_rst),
    .axis_data_clk         (axis_data_clk),
    .axis_data_rst         (axis_data_rst),
    .s_axis_chdr_tdata     (s_rfnoc_chdr_tdata[(1)*CHDR_W+:CHDR_W]),
    .s_axis_chdr_tlast     (s_rfnoc_chdr_tlast[1]),
    .s_axis_chdr_tvalid    (s_rfnoc_chdr_tvalid[1]),
    .s_axis_chdr_tready    (s_rfnoc_chdr_tready[1]),
    .m_axis_payload_tdata  (m_in_b_payload_tdata),
    .m_axis_payload_tkeep  (m_in_b_payload_tkeep),
    .m_axis_payload_tlast  (m_in_b_payload_tlast),
    .m_axis_payload_tvalid (m_in_b_payload_tvalid),
    .m_axis_payload_tready (m_in_b_payload_tready),
    .m_axis_context_tdata  (m_in_b_context_tdata),
    .m_axis_context_tuser  (m_in_b_context_tuser),
    .m_axis_context_tlast  (m_in_b_context_tlast),
    .m_axis_context_tvalid (m_in_b_context_tvalid),
    .m_axis_context_tready (m_in_b_context_tready),
    .flush_en              (data_i_flush_en),
    .flush_timeout         (data_i_flush_timeout),
    .flush_active          (data_i_flush_active[1]),
    .flush_done            (data_i_flush_done[1])
  );

  //---------------------
  // Output Data Paths
  //---------------------

  axis_pyld_ctxt_to_chdr #(
    .CHDR_W              (CHDR_W),
    .ITEM_W              (32),
    .NIPC                (1),
    .SYNC_CLKS           (0),
    .CONTEXT_FIFO_SIZE   ($clog2(2)),
    .PAYLOAD_FIFO_SIZE   ($clog2(2)),
    .MTU                 (MTU),
    .CONTEXT_PREFETCH_EN (1)
  ) axis_pyld_ctxt_to_chdr_out_add (
    .axis_chdr_clk         (rfnoc_chdr_clk),
    .axis_chdr_rst         (rfnoc_chdr_rst),
    .axis_data_clk         (axis_data_clk),
    .axis_data_rst         (axis_data_rst),
    .m_axis_chdr_tdata     (m_rfnoc_chdr_tdata[(0)*CHDR_W+:CHDR_W]),
    .m_axis_chdr_tlast     (m_rfnoc_chdr_tlast[0]),
    .m_axis_chdr_tvalid    (m_rfnoc_chdr_tvalid[0]),
    .m_axis_chdr_tready    (m_rfnoc_chdr_tready[0]),
    .s_axis_payload_tdata  (s_add_payload_tdata),
    .s_axis_payload_tkeep  (s_add_payload_tkeep),
    .s_axis_payload_tlast  (s_add_payload_tlast),
    .s_axis_payload_tvalid (s_add_payload_tvalid),
    .s_axis_payload_tready (s_add_payload_tready),
    .s_axis_context_tdata  (s_add_context_tdata),
    .s_axis_context_tuser  (s_add_context_tuser),
    .s_axis_context_tlast  (s_add_context_tlast),
    .s_axis_context_tvalid (s_add_context_tvalid),
    .s_axis_context_tready (s_add_context_tready),
    .framer_errors         (),
    .flush_en              (data_o_flush_en),
    .flush_timeout         (data_o_flush_timeout),
    .flush_active          (data_o_flush_active[0]),
    .flush_done            (data_o_flush_done[0])
  );

  axis_pyld_ctxt_to_chdr #(
    .CHDR_W              (CHDR_W),
    .ITEM_W              (32),
    .NIPC                (1),
    .SYNC_CLKS           (0),
    .CONTEXT_FIFO_SIZE   ($clog2(2)),
    .PAYLOAD_FIFO_SIZE   ($clog2(2)),
    .MTU                 (MTU),
    .CONTEXT_PREFETCH_EN (1)
  ) axis_pyld_ctxt_to_chdr_out_sub (
    .axis_chdr_clk         (rfnoc_chdr_clk),
    .axis_chdr_rst         (rfnoc_chdr_rst),
    .axis_data_clk         (axis_data_clk),
    .axis_data_rst         (axis_data_rst),
    .m_axis_chdr_tdata     (m_rfnoc_chdr_tdata[(1)*CHDR_W+:CHDR_W]),
    .m_axis_chdr_tlast     (m_rfnoc_chdr_tlast[1]),
    .m_axis_chdr_tvalid    (m_rfnoc_chdr_tvalid[1]),
    .m_axis_chdr_tready    (m_rfnoc_chdr_tready[1]),
    .s_axis_payload_tdata  (s_sub_payload_tdata),
    .s_axis_payload_tkeep  (s_sub_payload_tkeep),
    .s_axis_payload_tlast  (s_sub_payload_tlast),
    .s_axis_payload_tvalid (s_sub_payload_tvalid),
    .s_axis_payload_tready (s_sub_payload_tready),
    .s_axis_context_tdata  (s_sub_context_tdata),
    .s_axis_context_tuser  (s_sub_context_tuser),
    .s_axis_context_tlast  (s_sub_context_tlast),
    .s_axis_context_tvalid (s_sub_context_tvalid),
    .s_axis_context_tready (s_sub_context_tready),
    .framer_errors         (),
    .flush_en              (data_o_flush_en),
    .flush_timeout         (data_o_flush_timeout),
    .flush_active          (data_o_flush_active[1]),
    .flush_done            (data_o_flush_done[1])
  );

endmodule // noc_shell_addsub


`default_nettype wire
