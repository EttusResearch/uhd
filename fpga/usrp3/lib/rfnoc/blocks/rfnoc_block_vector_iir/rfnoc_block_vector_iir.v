//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_vector_iir
//
// Description:
//
//   This module implements an IIR filter with a variable length delay line.
//   Transfer Function:
//                                   beta
//                      H(z) = ------------------
//                             1 - alpha*z^-delay
//   Where:
//   - beta is the feedforward tap
//   - alpha is the feedback tap
//   - delay is the feedback tap delay
//
// Parameters:
//
//   THIS_PORTID : Control crossbar port to which this block is connected
//   CHDR_W      : AXIS-CHDR data bus width
//   MTU         : Maximum transmission unit (i.e., maximum packet size in
//                 CHDR words is 2**MTU).
//   NUM_PORTS   : Number of Vector IIR instances to instantiate
//   MAX_DELAY   : The maximum supported filter delay. This should correspond
//                 to the maximum SPP. Optimal values are a power of two, minus
//                 one (e.g, 2047).
//

`default_nettype none


module rfnoc_block_vector_iir #(
  parameter [9:0] THIS_PORTID = 10'd0,
  parameter       CHDR_W      = 64,
  parameter [5:0] MTU         = 10,
  parameter       NUM_PORTS   = 1,
  parameter       MAX_DELAY   = (2**MTU*CHDR_W/32-1)
) (
  // RFNoC Framework Clocks and Resets
  input  wire                            rfnoc_chdr_clk,
  input  wire                            rfnoc_ctrl_clk,
  input  wire                            ce_clk,
  // RFNoC Backend Interface
  input  wire [                   511:0] rfnoc_core_config,
  output wire [                   511:0] rfnoc_core_status,
  // AXIS-CHDR Input Ports (from framework)
  input  wire [(0+NUM_PORTS)*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [       (0+NUM_PORTS)-1:0] s_rfnoc_chdr_tlast,
  input  wire [       (0+NUM_PORTS)-1:0] s_rfnoc_chdr_tvalid,
  output wire [       (0+NUM_PORTS)-1:0] s_rfnoc_chdr_tready,
  // AXIS-CHDR Output Ports (to framework)
  output wire [(0+NUM_PORTS)*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [       (0+NUM_PORTS)-1:0] m_rfnoc_chdr_tlast,
  output wire [       (0+NUM_PORTS)-1:0] m_rfnoc_chdr_tvalid,
  input  wire [       (0+NUM_PORTS)-1:0] m_rfnoc_chdr_tready,
  // AXIS-Ctrl Input Port (from framework)
  input  wire [                    31:0] s_rfnoc_ctrl_tdata,
  input  wire                            s_rfnoc_ctrl_tlast,
  input  wire                            s_rfnoc_ctrl_tvalid,
  output wire                            s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Output Port (to framework)
  output wire [                    31:0] m_rfnoc_ctrl_tdata,
  output wire                            m_rfnoc_ctrl_tlast,
  output wire                            m_rfnoc_ctrl_tvalid,
  input  wire                            m_rfnoc_ctrl_tready
);

  `include "rfnoc_block_vector_iir_regs.vh"

  // Make sure MAX_DELAY isn't too big for REG_MAX_DELAY
  if (MAX_DELAY >= 2**REG_MAX_DELAY_LEN) begin
    MAX_DELAY_is_too_large_for_REG_MAX_DELAY();
  end


  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  // CtrlPort Master
  wire        m_ctrlport_req_wr;
  wire        m_ctrlport_req_rd;
  wire [19:0] m_ctrlport_req_addr;
  wire [31:0] m_ctrlport_req_data;
  wire        m_ctrlport_resp_ack;
  wire [31:0] m_ctrlport_resp_data;
  // Payload Stream to User Logic: in
  wire [NUM_PORTS*32*1-1:0] m_in_payload_tdata;
  wire [     NUM_PORTS-1:0] m_in_payload_tlast;
  wire [     NUM_PORTS-1:0] m_in_payload_tvalid;
  wire [     NUM_PORTS-1:0] m_in_payload_tready;
  // Context Stream to User Logic: in
  wire [NUM_PORTS*CHDR_W-1:0] m_in_context_tdata;
  wire [     NUM_PORTS*4-1:0] m_in_context_tuser;
  wire [       NUM_PORTS-1:0] m_in_context_tlast;
  wire [       NUM_PORTS-1:0] m_in_context_tvalid;
  wire [       NUM_PORTS-1:0] m_in_context_tready;
  // Payload Stream to User Logic: out
  wire [NUM_PORTS*32*1-1:0] s_out_payload_tdata;
  wire [     NUM_PORTS-1:0] s_out_payload_tlast;
  wire [     NUM_PORTS-1:0] s_out_payload_tvalid;
  wire [     NUM_PORTS-1:0] s_out_payload_tready;
  // Context Stream to User Logic: out
  wire [NUM_PORTS*CHDR_W-1:0] s_out_context_tdata;
  wire [     NUM_PORTS*4-1:0] s_out_context_tuser;
  wire [       NUM_PORTS-1:0] s_out_context_tlast;
  wire [       NUM_PORTS-1:0] s_out_context_tvalid;
  wire [       NUM_PORTS-1:0] s_out_context_tready;


  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  wire ce_rst;

  noc_shell_vector_iir #(
    .CHDR_W      (CHDR_W),
    .THIS_PORTID (THIS_PORTID),
    .MTU         (MTU),
    .NUM_PORTS   (NUM_PORTS)
  ) noc_shell_vector_iir_i (
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

    // CtrlPort Clock and Reset
    .ctrlport_clk         (),
    .ctrlport_rst         (),
    // CtrlPort Master
    .m_ctrlport_req_wr    (m_ctrlport_req_wr),
    .m_ctrlport_req_rd    (m_ctrlport_req_rd),
    .m_ctrlport_req_addr  (m_ctrlport_req_addr),
    .m_ctrlport_req_data  (m_ctrlport_req_data),
    .m_ctrlport_resp_ack  (m_ctrlport_resp_ack),
    .m_ctrlport_resp_data (m_ctrlport_resp_data),

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
    .s_out_payload_tkeep  (),
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

  // Context is not used because output packets have the same format as input
  // packets, so we pass through the context unchanged.
  assign s_out_context_tdata  = m_in_context_tdata;
  assign s_out_context_tuser  = m_in_context_tuser;
  assign s_out_context_tlast  = m_in_context_tlast;
  assign s_out_context_tvalid = m_in_context_tvalid;
  assign m_in_context_tready  = s_out_context_tready;


  //---------------------------------------------------------------------------
  // CtrlPort Splitter
  //---------------------------------------------------------------------------

  wire [NUM_PORTS* 1-1:0] dec_ctrlport_req_wr;
  wire [NUM_PORTS* 1-1:0] dec_ctrlport_req_rd;
  wire [NUM_PORTS*20-1:0] dec_ctrlport_req_addr;
  wire [NUM_PORTS*32-1:0] dec_ctrlport_req_data;
  wire [NUM_PORTS* 1-1:0] dec_ctrlport_resp_ack;
  wire [NUM_PORTS*32-1:0] dec_ctrlport_resp_data;

  ctrlport_decoder #(
    .NUM_SLAVES   (NUM_PORTS),
    .SLAVE_ADDR_W (VECTOR_IIR_ADDR_W)
  ) ctrlport_decoder_i (
    .ctrlport_clk            (ce_clk),
    .ctrlport_rst            (ce_rst),
    .s_ctrlport_req_wr       (m_ctrlport_req_wr),
    .s_ctrlport_req_rd       (m_ctrlport_req_rd),
    .s_ctrlport_req_addr     (m_ctrlport_req_addr),
    .s_ctrlport_req_data     (m_ctrlport_req_data),
    .s_ctrlport_req_byte_en  (4'hF),
    .s_ctrlport_req_has_time (1'b0),
    .s_ctrlport_req_time     (64'b0),
    .s_ctrlport_resp_ack     (m_ctrlport_resp_ack),
    .s_ctrlport_resp_status  (),
    .s_ctrlport_resp_data    (m_ctrlport_resp_data),
    .m_ctrlport_req_wr       (dec_ctrlport_req_wr),
    .m_ctrlport_req_rd       (dec_ctrlport_req_rd),
    .m_ctrlport_req_addr     (dec_ctrlport_req_addr),
    .m_ctrlport_req_data     (dec_ctrlport_req_data),
    .m_ctrlport_req_byte_en  (),
    .m_ctrlport_req_has_time (),
    .m_ctrlport_req_time     (),
    .m_ctrlport_resp_ack     (dec_ctrlport_resp_ack),
    .m_ctrlport_resp_status  ({NUM_PORTS{2'b0}}),
    .m_ctrlport_resp_data    (dec_ctrlport_resp_data)
  );


  //---------------------------------------------------------------------------
  // Port Instances
  //---------------------------------------------------------------------------

  genvar port;
  generate
    for (port = 0; port < NUM_PORTS; port = port+1) begin : gen_ports

      //-----------------------------------------------------------------------
      // Signal Selection
      //-----------------------------------------------------------------------
      //
      // Grab the appropriate CtrlPort and AXIS payload signals for this port.
      //
      //-----------------------------------------------------------------------

      wire        ctrlport_req_wr;
      wire        ctrlport_req_rd;
      wire [19:0] ctrlport_req_addr;
      wire [31:0] ctrlport_req_data;
      reg         ctrlport_resp_ack;
      reg  [31:0] ctrlport_resp_data;

      assign ctrlport_req_wr   = dec_ctrlport_req_wr[port];
      assign ctrlport_req_rd   = dec_ctrlport_req_rd[port];
      assign ctrlport_req_addr = dec_ctrlport_req_addr[port*20 +: 20];
      assign ctrlport_req_data = dec_ctrlport_req_data[port*32 +: 32];
      //
      assign dec_ctrlport_resp_ack[port]           = ctrlport_resp_ack;
      assign dec_ctrlport_resp_data[port*32 +: 32] = ctrlport_resp_data;

      wire [31:0] in_tdata;
      wire        in_tlast;
      wire        in_tvalid;
      wire        in_tready;
      wire [31:0] out_tdata;
      wire        out_tlast;
      wire        out_tvalid;
      wire        out_tready;

      assign in_tdata                  = m_in_payload_tdata [port*32 +: 32];
      assign in_tlast                  = m_in_payload_tlast [port];
      assign in_tvalid                 = m_in_payload_tvalid[port];
      assign m_in_payload_tready[port] = in_tready;
      //
      assign s_out_payload_tdata [port*32+:32] = out_tdata;
      assign s_out_payload_tlast [       port] = out_tlast;
      assign s_out_payload_tvalid[       port] = out_tvalid;
      assign out_tready                        = s_out_payload_tready[port];


      //-----------------------------------------------------------------------
      // Registers
      //-----------------------------------------------------------------------

      reg [$clog2(MAX_DELAY+1)-1:0] reg_delay;
      reg [      REG_ALPHA_LEN-1:0] reg_alpha;
      reg [       REG_BETA_LEN-1:0] reg_beta;

      reg reg_changed;

      always @(posedge ce_clk) begin
        if (ce_rst) begin
          reg_delay   <= 'bX;
          reg_alpha   <= 'bX;
          reg_beta    <= 'bX;
          reg_changed <= 1'b0;
        end else begin
          // Default assignments
          ctrlport_resp_ack  <= 1'b0;
          ctrlport_resp_data <= 32'b0;
          reg_changed        <= 1'b0;

          //-----------------------------------------
          // Register Reads
          //-----------------------------------------

          if (ctrlport_req_rd) begin
            ctrlport_resp_ack <= 1;
            case (ctrlport_req_addr)
              REG_DELAY : begin
                ctrlport_resp_data[REG_MAX_DELAY_POS +: REG_DELAY_LEN] <= MAX_DELAY;
                ctrlport_resp_data[REG_DELAY_POS     +: REG_DELAY_LEN] <= reg_delay;
              end
              REG_ALPHA :
                ctrlport_resp_data[REG_ALPHA_POS +: REG_ALPHA_LEN] <= reg_alpha;
              REG_BETA :
                ctrlport_resp_data[REG_BETA_POS +: REG_BETA_LEN] <= reg_beta;
            endcase

          //-----------------------------------------
          // Register Writes
          //-----------------------------------------

          end else if (ctrlport_req_wr) begin
            ctrlport_resp_ack <= 1;
            case (ctrlport_req_addr)
              REG_DELAY : begin
                reg_delay <= ctrlport_req_data[REG_DELAY_POS +: REG_DELAY_LEN];
                reg_changed    <= 1'b1;
              end
              REG_ALPHA : begin
                reg_alpha   <= ctrlport_req_data[REG_ALPHA_POS +: REG_ALPHA_LEN];
                reg_changed <= 1'b1;
              end
              REG_BETA : begin
                reg_beta    <= ctrlport_req_data[REG_BETA_POS +: REG_BETA_LEN];
                reg_changed <= 1'b1;
              end
            endcase
          end
        end
      end


      //-----------------------------------------------------------------------
      // Vector IIR Block
      //-----------------------------------------------------------------------

      vector_iir #(
        .MAX_VECTOR_LEN (MAX_DELAY),
        .ALPHA_W        (REG_ALPHA_LEN),
        .BETA_W         (REG_BETA_LEN)
      ) inst_vector_iir (
        .clk            (ce_clk),
        .reset          (ce_rst | reg_changed),
        .set_vector_len (reg_delay),
        .set_alpha      (reg_alpha),
        .set_beta       (reg_beta),
        .i_tdata        (in_tdata),
        .i_tlast        (in_tlast),
        .i_tvalid       (in_tvalid),
        .i_tready       (in_tready),
        .o_tdata        (out_tdata),
        .o_tlast        (out_tlast),
        .o_tvalid       (out_tvalid),
        .o_tready       (out_tready)
      );

    end
  endgenerate

endmodule // rfnoc_block_vector_iir


`default_nettype wire
