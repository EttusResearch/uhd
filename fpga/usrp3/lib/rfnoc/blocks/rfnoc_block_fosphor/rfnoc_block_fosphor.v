//
// Copyright 2020 Ettus Research, A National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_fosphor
//
// Description:
//
//   Fosphor RFNoC block. This block accepts packets containing FFT data (one
//   FFT output per packet) and generates two output data streams, one
//   containing histogram data and the other containing waterfall plot data.
//
//   Each set of waterfall data is output as a single packet. The frequency of
//   waterfall output depends on the waterfall decimation register setting.
//
//   Each set of histogram data is output as a burst of 64 packets, followed by
//   a single packet of max values and then a single packet of average values.
//   The frequency of waterfall output bursts depends on the waterfall
//   decimation register setting.
//
//   For all outputs, the packets contain byte values, and the number of bytes
//   in each packet matches the number of 4-byte sc16 samples in the input
//   packets (i.e., the FFT size). In other words, the output packet size is
//   1/4th the input packet size.
//
//   Many registers control the visual effects and behavior of the waterfall
//   and histogram. See the register descriptions in
//   rfnoc_block_fosphor_regs.vh for details.
//
// Parameters:
//
//   THIS_PORTID : Control crossbar port to which this block is connected
//   CHDR_W      : AXIS-CHDR data bus width
//   MTU         : Maximum transmission unit (i.e., maximum packet size in
//                 CHDR words is 2**MTU).
//

`default_nettype none


module rfnoc_block_fosphor #(
  parameter [9:0] THIS_PORTID     = 10'd0,
  parameter       CHDR_W          = 64,
  parameter [5:0] MTU             = 10
) (
  // RFNoC Framework Clocks and Resets
  input  wire                rfnoc_chdr_clk,
  input  wire                rfnoc_ctrl_clk,
  input  wire                ce_clk,
  // RFNoC Backend Interface
  input  wire [       511:0] rfnoc_core_config,
  output wire [       511:0] rfnoc_core_status,
  // AXIS-CHDR Input Ports (from framework)
  input  wire [1*CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire [     (1)-1:0] s_rfnoc_chdr_tlast,
  input  wire [     (1)-1:0] s_rfnoc_chdr_tvalid,
  output wire [     (1)-1:0] s_rfnoc_chdr_tready,
  // AXIS-CHDR Output Ports (to framework)
  output wire [2*CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire [     (2)-1:0] m_rfnoc_chdr_tlast,
  output wire [     (2)-1:0] m_rfnoc_chdr_tvalid,
  input  wire [     (2)-1:0] m_rfnoc_chdr_tready,
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

  `include "../../core/rfnoc_chdr_utils.vh"


  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  // CtrlPort Master
  wire               m_ctrlport_req_wr;
  wire               m_ctrlport_req_rd;
  wire [19:0]        m_ctrlport_req_addr;
  wire [31:0]        m_ctrlport_req_data;
  reg                m_ctrlport_resp_ack;
  reg  [31:0]        m_ctrlport_resp_data;
  // Data Stream to User Logic: in
  wire [32*1-1:0]    in_tdata;
  wire               in_tlast;
  wire               in_tvalid;
  wire               in_tready;
  wire [15:0]        in_tlength;
  // Data Stream from User Logic: hist
  wire [8*4-1:0]     hist_tdata;
  wire               hist_tlast;
  wire               hist_tvalid;
  wire               hist_tready;
  wire [15:0]        hist_tlength;
  wire               hist_teob;
  // Data Stream from User Logic: wf
  wire [8*4-1:0]     wf_tdata;
  wire               wf_tlast;
  wire               wf_tvalid;
  wire               wf_tready;
  wire [15:0]        wf_tlength;

  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  wire ce_rst;

  noc_shell_fosphor #(
    .CHDR_W      (CHDR_W),
    .THIS_PORTID (THIS_PORTID),
    .MTU         (MTU)
  ) noc_shell_fosphor_i (
    //---------------------
    // Framework Interface
    //---------------------

    // Clock Inputs
    .rfnoc_chdr_clk           (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk           (rfnoc_ctrl_clk),
    .ce_clk                   (ce_clk),
    // Reset Outputs
    .rfnoc_chdr_rst           (),
    .rfnoc_ctrl_rst           (),
    .ce_rst                   (ce_rst),
    // RFNoC Backend Interface
    .rfnoc_core_config        (rfnoc_core_config),
    .rfnoc_core_status        (rfnoc_core_status),
    // CHDR Input Ports  (from framework)
    .s_rfnoc_chdr_tdata       (s_rfnoc_chdr_tdata),
    .s_rfnoc_chdr_tlast       (s_rfnoc_chdr_tlast),
    .s_rfnoc_chdr_tvalid      (s_rfnoc_chdr_tvalid),
    .s_rfnoc_chdr_tready      (s_rfnoc_chdr_tready),
    // CHDR Output Ports (to framework)
    .m_rfnoc_chdr_tdata       (m_rfnoc_chdr_tdata),
    .m_rfnoc_chdr_tlast       (m_rfnoc_chdr_tlast),
    .m_rfnoc_chdr_tvalid      (m_rfnoc_chdr_tvalid),
    .m_rfnoc_chdr_tready      (m_rfnoc_chdr_tready),
    // AXIS-Ctrl Input Port (from framework)
    .s_rfnoc_ctrl_tdata       (s_rfnoc_ctrl_tdata),
    .s_rfnoc_ctrl_tlast       (s_rfnoc_ctrl_tlast),
    .s_rfnoc_ctrl_tvalid      (s_rfnoc_ctrl_tvalid),
    .s_rfnoc_ctrl_tready      (s_rfnoc_ctrl_tready),
    // AXIS-Ctrl Output Port (to framework)
    .m_rfnoc_ctrl_tdata       (m_rfnoc_ctrl_tdata),
    .m_rfnoc_ctrl_tlast       (m_rfnoc_ctrl_tlast),
    .m_rfnoc_ctrl_tvalid      (m_rfnoc_ctrl_tvalid),
    .m_rfnoc_ctrl_tready      (m_rfnoc_ctrl_tready),

    //---------------------
    // Client Interface
    //---------------------

    // CtrlPort Clock and Reset
    .ctrlport_clk             (),
    .ctrlport_rst             (),
    // CtrlPort Master
    .m_ctrlport_req_wr        (m_ctrlport_req_wr),
    .m_ctrlport_req_rd        (m_ctrlport_req_rd),
    .m_ctrlport_req_addr      (m_ctrlport_req_addr),
    .m_ctrlport_req_data      (m_ctrlport_req_data),
    .m_ctrlport_resp_ack      (m_ctrlport_resp_ack),
    .m_ctrlport_resp_data     (m_ctrlport_resp_data),

    // AXI-Stream Clock and Reset
    .axis_data_clk            (),
    .axis_data_rst            (),
    // Data Stream to User Logic: in
    .m_fft_in_axis_tdata      (in_tdata),
    .m_fft_in_axis_tkeep      (),
    .m_fft_in_axis_tlast      (in_tlast),
    .m_fft_in_axis_tvalid     (in_tvalid),
    .m_fft_in_axis_tready     (in_tready),
    .m_fft_in_axis_ttimestamp (),
    .m_fft_in_axis_thas_time  (),
    .m_fft_in_axis_tlength    (in_tlength),
    .m_fft_in_axis_teov       (),
    .m_fft_in_axis_teob       (),
    // Data Stream from User Logic: hist
    .s_hist_axis_tdata        (hist_tdata),
    .s_hist_axis_tkeep        (4'hF),
    .s_hist_axis_tlast        (hist_tlast),
    .s_hist_axis_tvalid       (hist_tvalid),
    .s_hist_axis_tready       (hist_tready),
    .s_hist_axis_ttimestamp   (64'b0),
    .s_hist_axis_thas_time    (1'b0),
    .s_hist_axis_tlength      (hist_tlength),
    .s_hist_axis_teov         (1'b0),
    .s_hist_axis_teob         (hist_teob),
    // Data Stream from User Logic: wf
    .s_wf_axis_tdata          (wf_tdata),
    .s_wf_axis_tkeep          (4'hF),
    .s_wf_axis_tlast          (wf_tlast),
    .s_wf_axis_tvalid         (wf_tvalid),
    .s_wf_axis_tready         (wf_tready),
    .s_wf_axis_ttimestamp     (64'b0),
    .s_wf_axis_thas_time      (1'b0),
    .s_wf_axis_tlength        (wf_tlength),
    .s_wf_axis_teov           (1'b0),
    .s_wf_axis_teob           (1'b0)
  );


  //---------------------------------------------------------------------------
  // Registers
  //---------------------------------------------------------------------------

  `include "rfnoc_block_fosphor_regs.vh"

  // Configuration registers
  reg [REG_ENABLE_LEN-1:0]     cfg_enable;
  reg                          clear_req;
  reg                          fosphor_rst = 1;
  reg [REG_RANDOM_LEN-1:0]     cfg_random;
  reg [REG_HIST_DECIM_LEN-1:0] cfg_hist_decim;
  reg [REG_OFFSET_LEN-1:0]     cfg_offset;
  reg [REG_SCALE_LEN-1:0]      cfg_scale;
  reg [REG_TRISE_LEN-1:0]      cfg_trise;
  reg [REG_TDECAY_LEN-1:0]     cfg_tdecay;
  reg [REG_ALPHA_LEN-1:0]      cfg_alpha;
  reg [REG_EPSILON_LEN-1:0]    cfg_epsilon;
  reg [REG_WF_DIV_LEN-1:0]     cfg_wf_div;
  reg                          cfg_wf_mode;
  reg [REG_WF_DECIM_LEN-1:0]   cfg_wf_decim;
  reg                          cfg_hist_decim_changed;
  reg                          cfg_wf_decim_changed;

  always @(posedge ce_clk) begin
    if (ce_rst) begin
      m_ctrlport_resp_ack    <= 0;
      m_ctrlport_resp_data   <= 'bX;
      cfg_enable             <= 0;
      clear_req              <= 0;
      fosphor_rst            <= 1;
      cfg_random             <= 0;
      cfg_hist_decim         <= 0;
      cfg_hist_decim_changed <= 0;
      cfg_offset             <= 0;
      cfg_scale              <= 0;
      cfg_trise              <= 0;
      cfg_tdecay             <= 0;
      cfg_alpha              <= 0;
      cfg_epsilon            <= 0;
      cfg_wf_div             <= 0;
      cfg_wf_mode            <= 0;
      cfg_wf_decim           <= 0;
      cfg_wf_decim_changed   <= 0;
    end else begin
      // Default assignments
      m_ctrlport_resp_ack    <= 0;
      m_ctrlport_resp_data   <= 0;
      cfg_hist_decim_changed <= 0;
      cfg_wf_decim_changed   <= 0;
      clear_req              <= 0;
      fosphor_rst            <= 0;
      m_ctrlport_resp_data   <= 0;
      m_ctrlport_resp_ack    <= 0;

      // Handle register writes
      if (m_ctrlport_req_wr) begin
        m_ctrlport_resp_ack <= 1;
        case (m_ctrlport_req_addr)
          REG_ENABLE     : cfg_enable  <= m_ctrlport_req_data[0+:REG_ENABLE_LEN];
          REG_CLEAR      : begin
            fosphor_rst                <= m_ctrlport_req_data[REG_RESET_POS];
            clear_req                  <= m_ctrlport_req_data[REG_CLEAR_POS];
          end
          REG_RANDOM     : cfg_random  <= m_ctrlport_req_data[0+:REG_RANDOM_LEN];
          REG_HIST_DECIM : begin
            cfg_hist_decim             <= m_ctrlport_req_data[0+:REG_HIST_DECIM_LEN];
            cfg_hist_decim_changed     <= 1'b1;
          end
          REG_OFFSET     : cfg_offset  <= m_ctrlport_req_data[0+:REG_OFFSET_LEN];
          REG_SCALE      : cfg_scale   <= m_ctrlport_req_data[0+:REG_SCALE_LEN];
          REG_TRISE      : cfg_trise   <= m_ctrlport_req_data[0+:REG_TRISE_LEN];
          REG_TDECAY     : cfg_tdecay  <= m_ctrlport_req_data[0+:REG_TDECAY_LEN];
          REG_ALPHA      : cfg_alpha   <= m_ctrlport_req_data[0+:REG_ALPHA_LEN];
          REG_EPSILON    : cfg_epsilon <= m_ctrlport_req_data[0+:REG_EPSILON_LEN];
          REG_WF_CTRL    : begin
            cfg_wf_mode                <= m_ctrlport_req_data[REG_WF_MODE_POS];
            cfg_wf_div                 <= m_ctrlport_req_data[REG_WF_DIV_POS+:REG_WF_DIV_LEN];
          end
          REG_WF_DECIM   : begin
            cfg_wf_decim               <= m_ctrlport_req_data[0+:REG_WF_DECIM_LEN];
            cfg_wf_decim_changed       <= 1'b1;
          end
        endcase

      // Handle register reads
      end else if (m_ctrlport_req_rd) begin
        m_ctrlport_resp_ack <= 1;
        case (m_ctrlport_req_addr)
          REG_ENABLE     : m_ctrlport_resp_data[0+:REG_ENABLE_LEN]     <= cfg_enable;
          REG_RANDOM     : m_ctrlport_resp_data[0+:REG_RANDOM_LEN]     <= cfg_random;
          REG_HIST_DECIM : m_ctrlport_resp_data[0+:REG_HIST_DECIM_LEN] <= cfg_hist_decim;
          REG_OFFSET     : m_ctrlport_resp_data[0+:REG_OFFSET_LEN]     <= cfg_offset;
          REG_SCALE      : m_ctrlport_resp_data[0+:REG_SCALE_LEN]      <= cfg_scale;
          REG_TRISE      : m_ctrlport_resp_data[0+:REG_TRISE_LEN]      <= cfg_trise;
          REG_TDECAY     : m_ctrlport_resp_data[0+:REG_TDECAY_LEN]     <= cfg_tdecay;
          REG_ALPHA      : m_ctrlport_resp_data[0+:REG_ALPHA_LEN]      <= cfg_alpha;
          REG_EPSILON    : m_ctrlport_resp_data[0+:REG_EPSILON_LEN]    <= cfg_epsilon;
          REG_WF_CTRL    : begin
            m_ctrlport_resp_data[REG_WF_MODE_POS]                      <= cfg_wf_mode;
            m_ctrlport_resp_data[REG_WF_DIV_POS+:REG_WF_DIV_LEN]       <= cfg_wf_div;
          end
          REG_WF_DECIM   : m_ctrlport_resp_data[0+:REG_WF_DECIM_LEN]   <= cfg_wf_decim;
        endcase
      end
    end
  end


  //---------------------------------------------------------------------------
  // Output Packet Length Register
  //---------------------------------------------------------------------------

  // The output length is always 1/4th the input length, since
  // we output one byte for each sc16 input.
  reg [15:0] out_packet_length;
  reg        start_of_packet = 1'b1;

  assign wf_tlength   = out_packet_length;
  assign hist_tlength = out_packet_length;

  always @(posedge ce_clk) begin
    if (ce_rst) begin
      start_of_packet   <= 1'b1;
      out_packet_length <=  'bX;
    end else begin
      if (in_tvalid && in_tready) begin
        start_of_packet <= in_tlast;
        if (start_of_packet) begin
          out_packet_length <= in_tlength / 4;
        end
      end
    end
  end


  //---------------------------------------------------------------------------
  // Fosphor Core
  //---------------------------------------------------------------------------

  wire hist_tvalid_tmp;
  wire hist_tready_tmp;
  wire wf_tvalid_tmp;
  wire wf_tready_tmp;

  f15_core f15_core_inst (
    .clk                  (ce_clk),
    .reset                (fosphor_rst),
    .clear_req            (clear_req),
    .cfg_random           (cfg_random),
    .cfg_offset           (cfg_offset),
    .cfg_scale            (cfg_scale),
    .cfg_trise            (cfg_trise),
    .cfg_tdecay           (cfg_tdecay),
    .cfg_alpha            (cfg_alpha),
    .cfg_epsilon          (cfg_epsilon),
    .cfg_decim            (cfg_hist_decim),
    .cfg_decim_changed    (cfg_hist_decim_changed),
    .cfg_wf_div           (cfg_wf_div),
    .cfg_wf_mode          (cfg_wf_mode),
    .cfg_wf_decim         (cfg_wf_decim),
    .cfg_wf_decim_changed (cfg_wf_decim_changed),
    .i_tdata              (in_tdata),
    .i_tlast              (in_tlast),
    .i_tvalid             (in_tvalid),
    .i_tready             (in_tready),
    .o_hist_tdata         (hist_tdata),
    .o_hist_tlast         (hist_tlast),
    .o_hist_tvalid        (hist_tvalid_tmp),
    .o_hist_tready        (hist_tready_tmp),
    .o_hist_teob          (hist_teob),
    .o_wf_tdata           (wf_tdata),
    .o_wf_tlast           (wf_tlast),
    .o_wf_tvalid          (wf_tvalid_tmp),
    .o_wf_tready          (wf_tready_tmp)
  );

  // Enable/disable logic. All we're doing here is discarding the output for
  // the "disabled" output. It is still generated internally.
  assign hist_tready_tmp = hist_tready     | ~cfg_enable[0];
  assign hist_tvalid     = hist_tvalid_tmp &  cfg_enable[0];
  assign wf_tready_tmp   = wf_tready       | ~cfg_enable[1];
  assign wf_tvalid       = wf_tvalid_tmp   &  cfg_enable[1];

endmodule // rfnoc_block_fosphor


`default_nettype wire
