//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_logpwr
//
// Description:
//
//   This block takes in signed 16-bit complex samples and computes an
//   estimate of 1024 * log2(i^2+q^2), and puts the result in the upper
//   16-bits of each 32-bit output item. The log is estimated using a lookup
//   table and random noise is added to reduce quantization effects.
//
// Parameters:
//
//   THIS_PORTID : Control crossbar port to which this block is connected
//   CHDR_W      : AXIS-CHDR data bus width
//   MTU         : Maximum transmission unit (i.e., maximum packet size in
//                 CHDR words is 2**MTU).
//   NUM_PORTS   : Number of Log-Power module instances to include.
//   RANDOM_MODE : Configures the random_mode for the logpwr block.
//                 [0] = Enable random LSBs on each input
//                 [1] = Enable random noise addition
//

`default_nettype none


module rfnoc_block_logpwr #(
  parameter [9:0] THIS_PORTID = 10'd0,
  parameter       CHDR_W      = 64,
  parameter [5:0] MTU         = 10,
  parameter       NUM_PORTS   = 1,
  parameter       RANDOM_MODE = 2'b11
) (
  // RFNoC Framework Clocks and Resets
  input  wire                        rfnoc_chdr_clk,
  input  wire                        rfnoc_ctrl_clk,
  input  wire                        ce_clk,
  // RFNoC Backend Interface
  input  wire [               511:0] rfnoc_core_config,
  output wire [               511:0] rfnoc_core_status,
  // AXIS-CHDR Input Ports (from framework)
  input  wire [NUM_PORTS*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tlast,
  input  wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tvalid,
  output wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tready,
  // AXIS-CHDR Output Ports (to framework)
  output wire [NUM_PORTS*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [       NUM_PORTS-1:0] m_rfnoc_chdr_tlast,
  output wire [       NUM_PORTS-1:0] m_rfnoc_chdr_tvalid,
  input  wire [       NUM_PORTS-1:0] m_rfnoc_chdr_tready,
  // AXIS-Ctrl Input Port (from framework)
  input  wire [                31:0] s_rfnoc_ctrl_tdata,
  input  wire                        s_rfnoc_ctrl_tlast,
  input  wire                        s_rfnoc_ctrl_tvalid,
  output wire                        s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Output Port (to framework)
  output wire [                31:0] m_rfnoc_ctrl_tdata,
  output wire                        m_rfnoc_ctrl_tlast,
  output wire                        m_rfnoc_ctrl_tvalid,
  input  wire                        m_rfnoc_ctrl_tready
);

  `include "../../core/rfnoc_chdr_utils.vh"

  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  // Payload Stream to User Logic: in
  wire [NUM_PORTS*32*1-1:0]   m_in_payload_tdata;
  wire [NUM_PORTS-1:0]        m_in_payload_tlast;
  wire [NUM_PORTS-1:0]        m_in_payload_tvalid;
  wire [NUM_PORTS-1:0]        m_in_payload_tready;
  // Context Stream to User Logic: in
  wire [NUM_PORTS*CHDR_W-1:0] m_in_context_tdata;
  wire [NUM_PORTS*4-1:0]      m_in_context_tuser;
  wire [NUM_PORTS-1:0]        m_in_context_tlast;
  wire [NUM_PORTS-1:0]        m_in_context_tvalid;
  reg  [NUM_PORTS-1:0]        m_in_context_tready;
  // Payload Stream to User Logic: out
  wire [NUM_PORTS*16*1-1:0]   s_out_payload_tdata;
  wire [NUM_PORTS-1:0]        s_out_payload_tlast;
  wire [NUM_PORTS-1:0]        s_out_payload_tvalid;
  wire [NUM_PORTS-1:0]        s_out_payload_tready;
  // Context Stream to User Logic: out
  reg  [NUM_PORTS*CHDR_W-1:0] s_out_context_tdata;
  reg  [NUM_PORTS*4-1:0]      s_out_context_tuser;
  reg  [NUM_PORTS-1:0]        s_out_context_tlast;
  reg  [NUM_PORTS-1:0]        s_out_context_tvalid;
  wire [NUM_PORTS-1:0]        s_out_context_tready;

  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  wire ce_rst;

  noc_shell_logpwr #(
    .CHDR_W      (CHDR_W),
    .THIS_PORTID (THIS_PORTID),
    .MTU         (MTU),
    .NUM_PORTS   (NUM_PORTS)
  ) noc_shell_logpwr_i (
    //---------------------
    // Framework Interface
    //---------------------

    // Clock Inputs
    .rfnoc_chdr_clk       (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk       (rfnoc_ctrl_clk),
    .ce_clk               (ce_clk),
    // Reset Outputs
    .rfnoc_chdr_rst       (),
    .rfnoc_ctrl_rst       (),
    .ce_rst               (ce_rst),
    // RFNoC Backend Interface
    .rfnoc_core_config    (rfnoc_core_config),
    .rfnoc_core_status    (rfnoc_core_status),
    // CHDR Input Ports  (from framework)
    .s_rfnoc_chdr_tdata   (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast   (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid  (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready  (s_rfnoc_chdr_tready),
    // CHDR Output Ports (to framework)
    .m_rfnoc_chdr_tdata   (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast   (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid  (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready  (m_rfnoc_chdr_tready),
    // AXIS-Ctrl Input Port (from framework)
    .s_rfnoc_ctrl_tdata   (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast   (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid  (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready  (s_rfnoc_ctrl_tready),
    // AXIS-Ctrl Output Port (to framework)
    .m_rfnoc_ctrl_tdata   (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast   (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid  (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready  (m_rfnoc_ctrl_tready),

    //---------------------
    // Client Interface
    //---------------------

    // AXI-Stream Payload Context Clock and Reset
    .axis_data_clk        (),
    .axis_data_rst        (),
    // Payload Stream to User Logic: in
    .m_in_payload_tdata   (m_in_payload_tdata),
    .m_in_payload_tkeep   (),
    .m_in_payload_tlast   (m_in_payload_tlast),
    .m_in_payload_tvalid  (m_in_payload_tvalid),
    .m_in_payload_tready  (m_in_payload_tready),
    // Context Stream to User Logic: in
    .m_in_context_tdata   (m_in_context_tdata),
    .m_in_context_tuser   (m_in_context_tuser),
    .m_in_context_tlast   (m_in_context_tlast),
    .m_in_context_tvalid  (m_in_context_tvalid),
    .m_in_context_tready  (m_in_context_tready),
    // Payload Stream from User Logic: out
    .s_out_payload_tdata  (s_out_payload_tdata),
    .s_out_payload_tkeep  ({NUM_PORTS{1'b1}}),
    .s_out_payload_tlast  (s_out_payload_tlast),
    .s_out_payload_tvalid (s_out_payload_tvalid),
    .s_out_payload_tready (s_out_payload_tready),
    // Context Stream from User Logic: out
    .s_out_context_tdata  (s_out_context_tdata),
    .s_out_context_tuser  (s_out_context_tuser),
    .s_out_context_tlast  (s_out_context_tlast),
    .s_out_context_tvalid (s_out_context_tvalid),
    .s_out_context_tready (s_out_context_tready)
  );


  //---------------------------------------------------------------------------
  // Context Handling
  //---------------------------------------------------------------------------
  //
  // Output packets have half the payload size of input packets, so we need to
  // update the header length field as it passes through.
  //
  //---------------------------------------------------------------------------

  genvar port;

  for (port = 0; port < NUM_PORTS; port = port+1) begin : gen_context_ports

    always @(*) begin : update_packet_length
      reg [CHDR_W-1:0] old_tdata;
      reg [CHDR_W-1:0] new_tdata;

      old_tdata = m_in_context_tdata[CHDR_W*port +: CHDR_W];

      // Check if this context word contains the header
      if (m_in_context_tuser[4*port +: 4] == CONTEXT_FIELD_HDR || 
          m_in_context_tuser[4*port +: 4] == CONTEXT_FIELD_HDR_TS
      ) begin : change_header
        // Update the lower 64-bits (the header word) with the new length
        reg [15:0] pyld_length;
        pyld_length     = chdr_calc_payload_length(CHDR_W, old_tdata) / 2;
        new_tdata       = old_tdata;
        new_tdata[63:0] = chdr_update_length(CHDR_W, old_tdata, pyld_length);
      end else begin : pass_through_header
        // Not a header word, so pass through unchanged
        new_tdata = old_tdata;
      end

      s_out_context_tdata  [CHDR_W*port +: CHDR_W] = new_tdata;
      s_out_context_tuser  [     4*port +:      4] = m_in_context_tuser   [4*port +: 4];
      s_out_context_tlast  [     1*port +:      1] = m_in_context_tlast   [1*port +: 1];
      s_out_context_tvalid [     1*port +:      1] = m_in_context_tvalid  [1*port +: 1];
      m_in_context_tready  [     1*port +:      1] = s_out_context_tready [1*port +: 1];
    end // update_packet_length

  end // gen_context_ports


  //---------------------------------------------------------------------------
  // Log-Power
  //---------------------------------------------------------------------------

  for (port = 0; port < NUM_PORTS; port = port+1) begin : gen_logpwr_ports

    wire [15:0] s_out_payload_tdata_temp;

    axi_logpwr #(
      .RANDOM_MODE (RANDOM_MODE)
    ) inst_axi_logpwr (
      .clk      (ce_clk),
      .reset    (ce_rst),
      .i_tdata  (m_in_payload_tdata   [port*32 +: 32]),
      .i_tlast  (m_in_payload_tlast   [port]),
      .i_tvalid (m_in_payload_tvalid  [port]),
      .i_tready (m_in_payload_tready  [port]),
      .o_tdata  (s_out_payload_tdata_temp),
      .o_tlast  (s_out_payload_tlast  [port]),
      .o_tvalid (s_out_payload_tvalid [port]),
      .o_tready (s_out_payload_tready [port])
    );

    // Convert the 16-bit unsigned result to a signed 16-bit result. This
    // makes the output an estimate of 1024 * log2(i^2+q^2) instead of the
    // 2048 * log2(i^2+q^2) returned by the block.
    assign s_out_payload_tdata[port*16 +: 16] = {
      1'b0, s_out_payload_tdata_temp[15:1]
    };

  end // gen_logpwr_ports

endmodule // rfnoc_block_logpwr


`default_nettype wire
