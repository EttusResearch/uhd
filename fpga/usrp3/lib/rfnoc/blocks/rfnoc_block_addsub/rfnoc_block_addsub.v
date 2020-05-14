//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_addsub
//
// Description:
//
//   This block takes in two streams and adds and subtracts them, creating two
//   output streams with the sum and difference of the input streams. It
//   assumes the input and output packets are all the same length and use sc16
//   samples.
//
//   This block also demonstrates how to use Verilog, VHDL and/or
//   high-level-synthesis (HLS) in a design. You can set the USE_IMPL parameter
//   to control which implementation is used.
//
// Parameters:
//
//   THIS_PORTID : Control crossbar port to which this block is connected
//   CHDR_W      : AXIS-CHDR data bus width
//   MTU         : Maximum transmission unit (i.e., maximum packet size in CHDR
//                 words is 2**MTU).
//   USE_IMPL    : Indicates which implementation to use. This is a string that
//                 can be set to "Verilog", "VHDL", or "HLS".
//
`default_nettype none


module rfnoc_block_addsub #(
  parameter [9:0] THIS_PORTID = 10'd0,
  parameter       CHDR_W      = 64,
  parameter [5:0] MTU         = 10,
  parameter       USE_IMPL    = "Verilog"
) (
  // RFNoC Framework Clocks and Resets
  input  wire                rfnoc_chdr_clk,
  input  wire                rfnoc_ctrl_clk,
  input  wire                ce_clk,
  // RFNoC Backend Interface
  input  wire [       511:0] rfnoc_core_config,
  output wire [       511:0] rfnoc_core_status,
  // AXIS-CHDR Input Ports (from framework)
  input  wire [2*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [       2-1:0] s_rfnoc_chdr_tlast,
  input  wire [       2-1:0] s_rfnoc_chdr_tvalid,
  output wire [       2-1:0] s_rfnoc_chdr_tready,
  // AXIS-CHDR Output Ports (to framework)
  output wire [2*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [       2-1:0] m_rfnoc_chdr_tlast,
  output wire [       2-1:0] m_rfnoc_chdr_tvalid,
  input  wire [       2-1:0] m_rfnoc_chdr_tready,
  // AXIS-Ctrl Input Port (from framework)
  input  wire [        31:0] s_rfnoc_ctrl_tdata,
  input  wire                s_rfnoc_ctrl_tlast,
  input  wire                s_rfnoc_ctrl_tvalid,
  output wire                s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Output Port (to framework)
  output wire [        31:0] m_rfnoc_ctrl_tdata,
  output wire                m_rfnoc_ctrl_tlast,
  output wire                m_rfnoc_ctrl_tvalid,
  input  wire                m_rfnoc_ctrl_tready
);

  // This block currently only supports 64-bit CHDR
  if (CHDR_W != 64) begin
    CHDR_W_must_be_64_for_the_addsub_block();
  end


  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  // Clocks and Resets
  wire               axis_data_clk;
  wire               axis_data_rst;
  // Payload Stream to User Logic: in_a
  wire [32*1-1:0]    m_in_a_payload_tdata;
  wire               m_in_a_payload_tlast;
  wire               m_in_a_payload_tvalid;
  wire               m_in_a_payload_tready;
  // Context Stream to User Logic: in_a
  wire [CHDR_W-1:0]  m_in_a_context_tdata;
  wire [3:0]         m_in_a_context_tuser;
  wire               m_in_a_context_tlast;
  wire               m_in_a_context_tvalid;
  wire               m_in_a_context_tready;
  // Payload Stream to User Logic: in_b
  wire [32*1-1:0]    m_in_b_payload_tdata;
  wire               m_in_b_payload_tlast;
  wire               m_in_b_payload_tvalid;
  wire               m_in_b_payload_tready;
  // Context Stream to User Logic: in_b
  wire               m_in_b_context_tready;
  // Payload Stream from User Logic: add
  wire [32*1-1:0]    s_add_payload_tdata;
  wire               s_add_payload_tlast;
  wire               s_add_payload_tvalid;
  wire               s_add_payload_tready;
  // Context Stream from User Logic: add
  wire [CHDR_W-1:0]  s_add_context_tdata;
  wire [3:0]         s_add_context_tuser;
  wire               s_add_context_tlast;
  wire               s_add_context_tvalid;
  wire               s_add_context_tready;
  // Payload Stream from User Logic: sub
  wire [32*1-1:0]    s_sub_payload_tdata;
  wire               s_sub_payload_tlast;
  wire               s_sub_payload_tvalid;
  wire               s_sub_payload_tready;
  // Context Stream from User Logic: sub
  wire [CHDR_W-1:0]  s_sub_context_tdata;
  wire [3:0]         s_sub_context_tuser;
  wire               s_sub_context_tlast;
  wire               s_sub_context_tvalid;
  wire               s_sub_context_tready;


  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  noc_shell_addsub #(
    .CHDR_W      (CHDR_W),
    .THIS_PORTID (THIS_PORTID),
    .MTU         (MTU),
    .USE_IMPL    (USE_IMPL)
  ) noc_shell_addsub_i (
    //---------------------
    // Framework Interface
    //---------------------

    // Clock Inputs
    .rfnoc_chdr_clk        (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk        (rfnoc_ctrl_clk),
    .ce_clk                (ce_clk),
    // Reset Outputs
    .rfnoc_chdr_rst        (),
    .rfnoc_ctrl_rst        (),
    .ce_rst                (),
    // RFNoC Backend Interface
    .rfnoc_core_config     (rfnoc_core_config),
    .rfnoc_core_status     (rfnoc_core_status),
    // CHDR Input Ports  (from framework)
    .s_rfnoc_chdr_tdata    (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast    (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid   (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready   (s_rfnoc_chdr_tready),
    // CHDR Output Ports (to framework)
    .m_rfnoc_chdr_tdata    (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast    (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid   (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready   (m_rfnoc_chdr_tready),
    // AXIS-Ctrl Input Port (from framework)
    .s_rfnoc_ctrl_tdata    (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast    (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid   (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready   (s_rfnoc_ctrl_tready),
    // AXIS-Ctrl Output Port (to framework)
    .m_rfnoc_ctrl_tdata    (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast    (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid   (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready   (m_rfnoc_ctrl_tready),

    //---------------------
    // Client Interface
    //---------------------

    // AXI-Stream Payload Context Clock and Reset
    .axis_data_clk         (axis_data_clk),
    .axis_data_rst         (axis_data_rst),
    // Payload Stream to User Logic: in_a
    .m_in_a_payload_tdata  (m_in_a_payload_tdata),
    .m_in_a_payload_tkeep  (),
    .m_in_a_payload_tlast  (m_in_a_payload_tlast),
    .m_in_a_payload_tvalid (m_in_a_payload_tvalid),
    .m_in_a_payload_tready (m_in_a_payload_tready),
    // Context Stream to User Logic: in_a
    .m_in_a_context_tdata  (m_in_a_context_tdata),
    .m_in_a_context_tuser  (m_in_a_context_tuser),
    .m_in_a_context_tlast  (m_in_a_context_tlast),
    .m_in_a_context_tvalid (m_in_a_context_tvalid),
    .m_in_a_context_tready (m_in_a_context_tready),
    // Payload Stream to User Logic: in_b
    .m_in_b_payload_tdata  (m_in_b_payload_tdata),
    .m_in_b_payload_tkeep  (),
    .m_in_b_payload_tlast  (m_in_b_payload_tlast),
    .m_in_b_payload_tvalid (m_in_b_payload_tvalid),
    .m_in_b_payload_tready (m_in_b_payload_tready),
    // Context Stream to User Logic: in_b
    .m_in_b_context_tdata  (),
    .m_in_b_context_tuser  (),
    .m_in_b_context_tlast  (),
    .m_in_b_context_tvalid (),
    .m_in_b_context_tready (m_in_b_context_tready),
    // Payload Stream from User Logic: add
    .s_add_payload_tdata   (s_add_payload_tdata),
    .s_add_payload_tkeep   (1'b1),
    .s_add_payload_tlast   (s_add_payload_tlast),
    .s_add_payload_tvalid  (s_add_payload_tvalid),
    .s_add_payload_tready  (s_add_payload_tready),
    // Context Stream from User Logic: add
    .s_add_context_tdata   (s_add_context_tdata),
    .s_add_context_tuser   (s_add_context_tuser),
    .s_add_context_tlast   (s_add_context_tlast),
    .s_add_context_tvalid  (s_add_context_tvalid),
    .s_add_context_tready  (s_add_context_tready),
    // Payload Stream from User Logic: diff
    .s_sub_payload_tdata  (s_sub_payload_tdata),
    .s_sub_payload_tkeep  (1'b1),
    .s_sub_payload_tlast  (s_sub_payload_tlast),
    .s_sub_payload_tvalid (s_sub_payload_tvalid),
    .s_sub_payload_tready (s_sub_payload_tready),
    // Context Stream from User Logic: diff
    .s_sub_context_tdata  (s_sub_context_tdata),
    .s_sub_context_tuser  (s_sub_context_tuser),
    .s_sub_context_tlast  (s_sub_context_tlast),
    .s_sub_context_tvalid (s_sub_context_tvalid),
    .s_sub_context_tready (s_sub_context_tready)
  );


  //---------------------------------------------------------------------------
  // Context Handling
  //---------------------------------------------------------------------------

  // We use the A input to control the packet size and other attributes of the
  // output packets. So we duplicate the A context and discard the B context.
  assign m_in_b_context_tready = 1;

  axis_split #(
    .DATA_W    (1 + 4 + CHDR_W),    // TLAST + TUSER + TDATA
    .NUM_PORTS (2)
  ) axis_split_i (
    .clk           (axis_data_clk),
    .rst           (axis_data_rst),
    .s_axis_tdata  ({m_in_a_context_tlast,
                     m_in_a_context_tuser,
                     m_in_a_context_tdata}),
    .s_axis_tvalid (m_in_a_context_tvalid),
    .s_axis_tready (m_in_a_context_tready),
    .m_axis_tdata  ({s_sub_context_tlast,
                     s_sub_context_tuser,
                     s_sub_context_tdata,
                     s_add_context_tlast,
                     s_add_context_tuser,
                     s_add_context_tdata}),
    .m_axis_tvalid ({s_sub_context_tvalid, s_add_context_tvalid}),
    .m_axis_tready ({s_sub_context_tready, s_add_context_tready})
  );


  //---------------------------------------------------------------------------
  // Add/Subtract logic
  //---------------------------------------------------------------------------

  generate
    if (USE_IMPL == "HLS") begin : gen_hls
      // Use the module generated through Vivado High-Level Synthesis (see
      // addsub_hls.cpp).
      addsub_hls addsub_hls_i (
        .ap_clk     (axis_data_clk),
        .ap_rst_n   (~axis_data_rst),
        .a_TDATA    (m_in_a_payload_tdata),
        .a_TVALID   (m_in_a_payload_tvalid),
        .a_TREADY   (m_in_a_payload_tready),
        .a_TLAST    (m_in_a_payload_tlast),
        .b_TDATA    (m_in_b_payload_tdata),
        .b_TVALID   (m_in_b_payload_tvalid),
        .b_TREADY   (m_in_b_payload_tready),
        .b_TLAST    (m_in_b_payload_tlast),
        .add_TDATA  (s_add_payload_tdata),
        .add_TVALID (s_add_payload_tvalid),
        .add_TREADY (s_add_payload_tready),
        .add_TLAST  (s_add_payload_tlast),
        .sub_TDATA  (s_sub_payload_tdata),
        .sub_TVALID (s_sub_payload_tvalid),
        .sub_TREADY (s_sub_payload_tready),
        .sub_TLAST  (s_sub_payload_tlast)
      );
    end else if (USE_IMPL == "VHDL") begin : gen_vhdl
      // Use the VHDL implementation
      addsub_vhdl #(
        .width_g (16)
      ) addsub_vhdl_i (
        .clk_i       (axis_data_clk),
        .rst_i       (axis_data_rst),
        .i0_tdata    (m_in_a_payload_tdata),
        .i0_tlast    (m_in_a_payload_tlast),
        .i0_tvalid   (m_in_a_payload_tvalid),
        .i0_tready   (m_in_a_payload_tready),
        .i1_tdata    (m_in_b_payload_tdata),
        .i1_tlast    (m_in_b_payload_tlast),
        .i1_tvalid   (m_in_b_payload_tvalid),
        .i1_tready   (m_in_b_payload_tready),
        .sum_tdata   (s_add_payload_tdata),
        .sum_tlast   (s_add_payload_tlast),
        .sum_tvalid  (s_add_payload_tvalid),
        .sum_tready  (s_add_payload_tready),
        .diff_tdata  (s_sub_payload_tdata),
        .diff_tlast  (s_sub_payload_tlast),
        .diff_tvalid (s_sub_payload_tvalid),
        .diff_tready (s_sub_payload_tready)
      );
    end else begin : gen_verilog
      // Use Verilog implementation
      addsub #(
        .WIDTH (16)
      ) inst_addsub (
        .clk         (axis_data_clk),
        .reset       (axis_data_rst),
        .i0_tdata    (m_in_a_payload_tdata),
        .i0_tlast    (m_in_a_payload_tlast),
        .i0_tvalid   (m_in_a_payload_tvalid),
        .i0_tready   (m_in_a_payload_tready),
        .i1_tdata    (m_in_b_payload_tdata),
        .i1_tlast    (m_in_b_payload_tlast),
        .i1_tvalid   (m_in_b_payload_tvalid),
        .i1_tready   (m_in_b_payload_tready),
        .sum_tdata   (s_add_payload_tdata),
        .sum_tlast   (s_add_payload_tlast),
        .sum_tvalid  (s_add_payload_tvalid),
        .sum_tready  (s_add_payload_tready),
        .diff_tdata  (s_sub_payload_tdata),
        .diff_tlast  (s_sub_payload_tlast),
        .diff_tvalid (s_sub_payload_tvalid),
        .diff_tready (s_sub_payload_tready)
      );
    end
  endgenerate

endmodule // rfnoc_block_addsub

`default_nettype wire
