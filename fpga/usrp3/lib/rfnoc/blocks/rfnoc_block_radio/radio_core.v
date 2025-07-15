//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: radio_core
//
// Description:
//
// A radio core for RFNoC. This core contains all logic in the radio clock
// domain for interfacing to a single RX/TX radio. It includes registers shared
// by both Rx and Tx logic and instantiates Rx and Tx interface cores.
//
// Parameters:
//
//   BASE_ADDR    : Base address for this radio block instance
//   SAMP_W       : Width of a radio sample
//   NSPC         : Number of radio samples per radio clock cycle
//   EN_COMP_GAIN : Enable complex gain functionality
//


module radio_core #(
  parameter SAMP_W          = 32,
  parameter NSPC            = 1,
  parameter EN_COMP_GAIN_TX = 1,
  parameter EN_COMP_GAIN_RX = 1
) (
  input wire radio_clk,
  input wire radio_rst,


  //---------------------------------------------------------------------------
  // Control Interface
  //---------------------------------------------------------------------------

  // Slave
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  input  wire        s_ctrlport_req_has_time,
  input  wire [63:0] s_ctrlport_req_time,
  output wire        s_ctrlport_resp_ack,
  output wire [ 1:0] s_ctrlport_resp_status,
  output wire [31:0] s_ctrlport_resp_data,

  // Master
  output wire        m_ctrlport_req_wr,
  output wire [19:0] m_ctrlport_req_addr,
  output wire [ 9:0] m_ctrlport_req_portid,
  output wire [15:0] m_ctrlport_req_rem_epid,
  output wire [ 9:0] m_ctrlport_req_rem_portid,
  output wire [31:0] m_ctrlport_req_data,
  output wire        m_ctrlport_req_has_time,
  output wire [63:0] m_ctrlport_req_time,
  input  wire        m_ctrlport_resp_ack,


  //---------------------------------------------------------------------------
  // Data Interface
  //---------------------------------------------------------------------------

  // Tx Radio Data Stream
  input  wire [(SAMP_W*NSPC)-1:0] s_axis_tdata,
  input  wire                     s_axis_tlast,
  input  wire                     s_axis_tvalid,
  output wire                     s_axis_tready,
  // Sideband info
  input  wire [             63:0] s_axis_ttimestamp,
  input  wire                     s_axis_thas_time,
  input  wire                     s_axis_teob,

  // Rx Radio Data Stream
  output wire [(SAMP_W*NSPC)-1:0] m_axis_tdata,
  output wire                     m_axis_tlast,
  output wire                     m_axis_tvalid,
  input  wire                     m_axis_tready,
  // Sideband info
  output wire [             63:0] m_axis_ttimestamp,
  output wire                     m_axis_thas_time,
  output wire                     m_axis_teob,


  //---------------------------------------------------------------------------
  // Radio Interface
  //---------------------------------------------------------------------------

  input wire [63:0] radio_time,

  // Radio Rx Interface
  input  wire [SAMP_W*NSPC-1:0] radio_rx_data,
  input  wire                   radio_rx_stb,
  output wire                   radio_rx_running,

  // Radio Tx Interface
  output wire [SAMP_W*NSPC-1:0] radio_tx_data,
  input  wire                   radio_tx_stb,
  output wire                   radio_tx_running
);

  `include "rfnoc_block_radio_regs.vh"
  `include "../../core/ctrlport.vh"

  // Check if at least one feature is enabled.
  localparam COMP_GAIN_TX_PRESENT = (EN_COMP_GAIN_TX != 0) ? 1'b1 : 1'b0;
  localparam COMP_GAIN_RX_PRESENT = (EN_COMP_GAIN_RX != 0) ? 1'b1 : 1'b0;
  localparam FEATURES_PRESENT = COMP_GAIN_TX_PRESENT | COMP_GAIN_RX_PRESENT;
  //---------------------------------------------------------------------------
  // Split Control Port Interface
  //---------------------------------------------------------------------------
  //
  // This block splits the single slave interface of the radio core into
  // multiple interfaces, one for each subcomponent. The responses from each
  // subcomponent are merged into a single response and sent back out the slave
  // interface.
  //
  //---------------------------------------------------------------------------

  // Registers shared by Rx and Tx
  wire        ctrlport_general_req_wr;
  wire        ctrlport_general_req_rd;
  wire [19:0] ctrlport_general_req_addr;
  wire [31:0] ctrlport_general_req_data;
  wire        ctrlport_general_req_has_time;
  wire [63:0] ctrlport_general_req_time;
  reg         ctrlport_general_resp_ack  = 1'b0;
  reg  [31:0] ctrlport_general_resp_data = 0;

  // Tx core registers
  wire        ctrlport_tx_req_wr;
  wire        ctrlport_tx_req_rd;
  wire [19:0] ctrlport_tx_req_addr;
  wire [31:0] ctrlport_tx_req_data;
  wire        ctrlport_tx_req_has_time;
  wire [63:0] ctrlport_tx_req_time;
  wire        ctrlport_tx_resp_ack;
  wire [ 1:0] ctrlport_tx_resp_status;
  wire [31:0] ctrlport_tx_resp_data;

  // Rx core registers
  wire        ctrlport_rx_req_wr;
  wire        ctrlport_rx_req_rd;
  wire [19:0] ctrlport_rx_req_addr;
  wire [31:0] ctrlport_rx_req_data;
  wire        ctrlport_rx_req_has_time;
  wire [63:0] ctrlport_rx_req_time;
  wire        ctrlport_rx_resp_ack;
  wire [ 1:0] ctrlport_rx_resp_status;
  wire [31:0] ctrlport_rx_resp_data;

  // Feature Control registers
  wire        ctrlport_feat_req_wr;
  wire        ctrlport_feat_req_rd;
  wire [19:0] ctrlport_feat_req_addr;
  wire [31:0] ctrlport_feat_req_data;
  wire        ctrlport_feat_req_has_time;
  wire [63:0] ctrlport_feat_req_time;
  wire        ctrlport_feat_resp_ack;
  wire [ 1:0] ctrlport_feat_resp_status;
  wire [31:0] ctrlport_feat_resp_data;

  // Tx comp gain registers
  wire        ctrlport_tx_comp_gain_req_wr;
  wire        ctrlport_tx_comp_gain_req_rd;
  wire [19:0] ctrlport_tx_comp_gain_req_addr;
  wire [31:0] ctrlport_tx_comp_gain_req_data;
  wire        ctrlport_tx_comp_gain_req_has_time;
  wire [63:0] ctrlport_tx_comp_gain_req_time;
  wire        ctrlport_tx_comp_gain_resp_ack;
  wire [ 1:0] ctrlport_tx_comp_gain_resp_status;
  wire [31:0] ctrlport_tx_comp_gain_resp_data;

  // Rx comp gain registers
  wire        ctrlport_rx_comp_gain_req_wr;
  wire        ctrlport_rx_comp_gain_req_rd;
  wire [19:0] ctrlport_rx_comp_gain_req_addr;
  wire [31:0] ctrlport_rx_comp_gain_req_data;
  wire        ctrlport_rx_comp_gain_req_has_time;
  wire [63:0] ctrlport_rx_comp_gain_req_time;
  wire        ctrlport_rx_comp_gain_resp_ack;
  wire [ 1:0] ctrlport_rx_comp_gain_resp_status;
  wire [31:0] ctrlport_rx_comp_gain_resp_data;

  ctrlport_decoder #(
    .NUM_SLAVES   (4),
    .BASE_ADDR    (0),
    .SLAVE_ADDR_W (8)
  ) ctrlport_port_addr_decoder_i (
    .ctrlport_clk(radio_clk),
    .ctrlport_rst(radio_rst),
    .s_ctrlport_req_wr       (s_ctrlport_req_wr),
    .s_ctrlport_req_rd       (s_ctrlport_req_rd),
    .s_ctrlport_req_addr     (s_ctrlport_req_addr),
    .s_ctrlport_req_data     (s_ctrlport_req_data),
    .s_ctrlport_req_byte_en  (4'hF),
    .s_ctrlport_req_has_time (s_ctrlport_req_has_time),
    .s_ctrlport_req_time     (s_ctrlport_req_time),
    .s_ctrlport_resp_ack     (s_ctrlport_resp_ack),
    .s_ctrlport_resp_status  (s_ctrlport_resp_status),
    .s_ctrlport_resp_data    (s_ctrlport_resp_data),
    .m_ctrlport_req_wr       ({ctrlport_feat_req_wr,
                               ctrlport_tx_req_wr,
                               ctrlport_rx_req_wr,
                               ctrlport_general_req_wr}),
    .m_ctrlport_req_rd       ({ctrlport_feat_req_rd,
                               ctrlport_tx_req_rd,
                               ctrlport_rx_req_rd,
                               ctrlport_general_req_rd}),
    .m_ctrlport_req_addr     ({ctrlport_feat_req_addr,
                               ctrlport_tx_req_addr,
                               ctrlport_rx_req_addr,
                               ctrlport_general_req_addr}),
    .m_ctrlport_req_data     ({ctrlport_feat_req_data,
                               ctrlport_tx_req_data,
                               ctrlport_rx_req_data,
                               ctrlport_general_req_data}),
    .m_ctrlport_req_byte_en  (),
    .m_ctrlport_req_has_time ({ctrlport_feat_req_has_time,
                               ctrlport_tx_req_has_time,
                               ctrlport_rx_req_has_time,
                               ctrlport_general_req_has_time}),
    .m_ctrlport_req_time     ({ctrlport_feat_req_time,
                               ctrlport_tx_req_time,
                               ctrlport_rx_req_time,
                               ctrlport_general_req_time}),
    .m_ctrlport_resp_ack     ({ctrlport_feat_resp_ack,
                               ctrlport_tx_resp_ack,
                               ctrlport_rx_resp_ack,
                               ctrlport_general_resp_ack}),
    .m_ctrlport_resp_status  ({ctrlport_feat_resp_status,
                               CTRL_STS_OKAY,
                               CTRL_STS_OKAY,
                               CTRL_STS_OKAY}),
    .m_ctrlport_resp_data    ({ctrlport_feat_resp_data,
                               ctrlport_tx_resp_data,
                               ctrlport_rx_resp_data,
                               ctrlport_general_resp_data})
  );

  // Feature register splitter
  if(FEATURES_PRESENT) begin : gen_split_ctrlport_feat
    ctrlport_splitter #(
      .NUM_SLAVES (2)
    ) feature_ctrlport_splitter_i (
      .ctrlport_clk            (radio_clk),
      .ctrlport_rst            (radio_rst),
      .s_ctrlport_req_wr       (ctrlport_feat_req_wr),
      .s_ctrlport_req_rd       (ctrlport_feat_req_rd),
      .s_ctrlport_req_addr     (ctrlport_feat_req_addr),
      .s_ctrlport_req_data     (ctrlport_feat_req_data),
      .s_ctrlport_req_byte_en  (4'hF),
      .s_ctrlport_req_has_time (ctrlport_feat_req_has_time),
      .s_ctrlport_req_time     (ctrlport_feat_req_time),
      .s_ctrlport_resp_ack     (ctrlport_feat_resp_ack),
      .s_ctrlport_resp_status  (ctrlport_feat_resp_status),
      .s_ctrlport_resp_data    (ctrlport_feat_resp_data),
      .m_ctrlport_req_wr       ({ctrlport_tx_comp_gain_req_wr,
                                 ctrlport_rx_comp_gain_req_wr}),
      .m_ctrlport_req_rd       ({ctrlport_tx_comp_gain_req_rd,
                                 ctrlport_rx_comp_gain_req_rd}),
      .m_ctrlport_req_addr     ({ctrlport_tx_comp_gain_req_addr,
                                 ctrlport_rx_comp_gain_req_addr}),
      .m_ctrlport_req_data     ({ctrlport_tx_comp_gain_req_data,
                                 ctrlport_rx_comp_gain_req_data}),
      .m_ctrlport_req_byte_en  (),
      .m_ctrlport_req_has_time ({ctrlport_tx_comp_gain_req_has_time,
                                 ctrlport_rx_comp_gain_req_has_time}),
      .m_ctrlport_req_time     ({ctrlport_tx_comp_gain_req_time,
                                 ctrlport_rx_comp_gain_req_time}),
      .m_ctrlport_resp_ack     ({ctrlport_tx_comp_gain_resp_ack,
                                 ctrlport_rx_comp_gain_resp_ack}),
      .m_ctrlport_resp_status  ({ctrlport_tx_comp_gain_resp_status,
                                 ctrlport_rx_comp_gain_resp_status}),
      .m_ctrlport_resp_data    ({ctrlport_tx_comp_gain_resp_data,
                                 ctrlport_rx_comp_gain_resp_data})
      );
  end else begin
    assign ctrlport_feat_resp_ack = 0;
    assign ctrlport_feat_resp_data = 0;
  end

  //---------------------------------------------------------------------------
  // Merge Control Port Interfaces
  //---------------------------------------------------------------------------
  //
  // This block merges the master control port interfaces of the Rx and Tx
  // cores into a single master control port interface. Both the Rx and Tx
  // cores support error reporting by writing to a control port interface. This
  // block arbitrates the requests between the Rx and Tx cores. Rx and Tx only
  // support writes for error reporting, not reads. Time and byte enables are
  // also not needed. Hence, several ports are unconnected.
  //
  //---------------------------------------------------------------------------

  // Tx and Rx error reporting signals
  wire        ctrlport_err_tx_req_wr,         ctrlport_err_rx_req_wr;
  wire [19:0] ctrlport_err_tx_req_addr,       ctrlport_err_rx_req_addr;
  wire [31:0] ctrlport_err_tx_req_data,       ctrlport_err_rx_req_data;
  wire        ctrlport_err_tx_req_has_time,   ctrlport_err_rx_req_has_time;
  wire [63:0] ctrlport_err_tx_req_time,       ctrlport_err_rx_req_time;
  wire [ 9:0] ctrlport_err_tx_req_portid,     ctrlport_err_rx_req_portid;
  wire [15:0] ctrlport_err_tx_req_rem_epid,   ctrlport_err_rx_req_rem_epid;
  wire [ 9:0] ctrlport_err_tx_req_rem_portid, ctrlport_err_rx_req_rem_portid;
  wire        ctrlport_err_tx_resp_ack,       ctrlport_err_rx_resp_ack;


  ctrlport_combiner #(
    .NUM_MASTERS (2),
    .PRIORITY    (0)
  ) ctrlport_req_combine_i (
    .ctrlport_clk              (radio_clk),
    .ctrlport_rst              (radio_rst),
    .s_ctrlport_req_wr         ({ctrlport_err_tx_req_wr,         ctrlport_err_rx_req_wr}),
    .s_ctrlport_req_rd         (2'b0),
    .s_ctrlport_req_addr       ({ctrlport_err_tx_req_addr,       ctrlport_err_rx_req_addr}),
    .s_ctrlport_req_portid     ({ctrlport_err_tx_req_portid,     ctrlport_err_rx_req_portid}),
    .s_ctrlport_req_rem_epid   ({ctrlport_err_tx_req_rem_epid,   ctrlport_err_rx_req_rem_epid}),
    .s_ctrlport_req_rem_portid ({ctrlport_err_tx_req_rem_portid, ctrlport_err_rx_req_rem_portid}),
    .s_ctrlport_req_data       ({ctrlport_err_tx_req_data,       ctrlport_err_rx_req_data}),
    .s_ctrlport_req_byte_en    (8'hFF),
    .s_ctrlport_req_has_time   ({ctrlport_err_tx_req_has_time,   ctrlport_err_rx_req_has_time}),
    .s_ctrlport_req_time       ({ctrlport_err_tx_req_time,       ctrlport_err_rx_req_time}),
    .s_ctrlport_resp_ack       ({ctrlport_err_tx_resp_ack,       ctrlport_err_rx_resp_ack}),
    .s_ctrlport_resp_status    (),
    .s_ctrlport_resp_data      (),
    .m_ctrlport_req_wr         (m_ctrlport_req_wr),
    .m_ctrlport_req_rd         (),
    .m_ctrlport_req_addr       (m_ctrlport_req_addr),
    .m_ctrlport_req_portid     (m_ctrlport_req_portid),
    .m_ctrlport_req_rem_epid   (m_ctrlport_req_rem_epid),
    .m_ctrlport_req_rem_portid (m_ctrlport_req_rem_portid),
    .m_ctrlport_req_data       (m_ctrlport_req_data),
    .m_ctrlport_req_byte_en    (),
    .m_ctrlport_req_has_time   (m_ctrlport_req_has_time),
    .m_ctrlport_req_time       (m_ctrlport_req_time),
    .m_ctrlport_resp_ack       (m_ctrlport_resp_ack),
    .m_ctrlport_resp_status    (2'b0),
    .m_ctrlport_resp_data      (0)
  );


  //---------------------------------------------------------------------------
  // General Registers
  //---------------------------------------------------------------------------
  //
  // These are registers that apply to both Rx and Tx and are shared by both.
  //
  //---------------------------------------------------------------------------

  reg reg_loopback_en = 1'b0;

  always @(posedge radio_clk) begin
    if (radio_rst) begin
      ctrlport_general_resp_ack  <= 0;
      ctrlport_general_resp_data <= 0;
      reg_loopback_en            <= 0;
    end else begin
      // Default assignments
      ctrlport_general_resp_ack  <= 0;
      ctrlport_general_resp_data <= 0;

      // Handle register writes
      if (ctrlport_general_req_wr) begin
        case (ctrlport_general_req_addr)
          REG_LOOPBACK_EN: begin
            reg_loopback_en           <= ctrlport_general_req_data[0];
            ctrlport_general_resp_ack <= 1;
          end
        endcase
      end

      // Handle register reads
      if (ctrlport_general_req_rd) begin
        case (ctrlport_general_req_addr)
          REG_LOOPBACK_EN: begin
            ctrlport_general_resp_data    <= 0;
            ctrlport_general_resp_data[0] <= reg_loopback_en;
            ctrlport_general_resp_ack     <= 1;
          end
          REG_RADIO_WIDTH: begin
            ctrlport_general_resp_data <= { SAMP_W[15:0], NSPC[15:0] };
            ctrlport_general_resp_ack  <= 1;
          end
          REG_FEATURES_PRESENT: begin
            ctrlport_general_resp_data    <= { 30'b0,
                                              COMP_GAIN_RX_PRESENT[0],
                                              COMP_GAIN_TX_PRESENT[0]};
            ctrlport_general_resp_ack     <= 1;
          end
        endcase
      end
    end
  end


  //---------------------------------------------------------------------------
  // Tx to Rx Loopback
  //---------------------------------------------------------------------------

  reg  [SAMP_W*NSPC-1:0] radio_tx_data_to_rx;
  reg                    radio_tx_stb_to_rx;
  reg                    radio_tx_running_to_rx;
  wire [SAMP_W*NSPC-1:0] radio_rx_data_mux;
  wire                   radio_rx_stb_mux;

  // Create register without reset on TX to RX loopback path to avoid timing failures.
  always @(posedge radio_clk) begin
    radio_tx_data_to_rx    <= radio_tx_data;
    radio_tx_stb_to_rx     <= radio_tx_stb;
    radio_tx_running_to_rx <= radio_tx_running;
  end

  assign radio_rx_data_mux = reg_loopback_en ? radio_tx_data_to_rx : radio_rx_data;
  assign radio_rx_stb_mux  = reg_loopback_en ? radio_tx_stb_to_rx  : radio_rx_stb;

  //---------------------------------------------------------------------------
  // Timed Complex Gain - TX
  //---------------------------------------------------------------------------

  localparam COMP_GAIN_LATENCY = 6; // Pipeline delay of the complex gain module
  wire [NSPC*SAMP_W-1:0]      pre_gain_radio_tx_data;
  wire                        pre_gain_radio_tx_running;
  reg [COMP_GAIN_LATENCY-1:0] post_gain_tx_running_shift_reg = 0;

  if(EN_COMP_GAIN_TX) begin : gen_comp_gain_tx

    always @(posedge radio_clk) begin
      if (radio_rst) begin
        post_gain_tx_running_shift_reg <= 0;
      end else begin
        // Shift the radio_tx_running signal to match the latency of the gain block
        post_gain_tx_running_shift_reg <= {
          post_gain_tx_running_shift_reg[COMP_GAIN_LATENCY-2:0],
          pre_gain_radio_tx_running
        };
      end
    end
    assign radio_tx_running = post_gain_tx_running_shift_reg[COMP_GAIN_LATENCY-1];

    timed_complex_gain #(
      .ITEM_W(SAMP_W),
      .NIPC  (NSPC),
      .BASE_ADDR(REG_CGAIN_TX_OFFSET)
    ) timed_complex_gain_tx_i (
      .clk                     (radio_clk),
      .rst                     (radio_rst),
      .s_ctrlport_req_wr       (ctrlport_tx_comp_gain_req_wr),
      .s_ctrlport_req_rd       (ctrlport_tx_comp_gain_req_rd),
      .s_ctrlport_req_addr     (ctrlport_tx_comp_gain_req_addr),
      .s_ctrlport_req_data     (ctrlport_tx_comp_gain_req_data),
      .s_ctrlport_req_has_time (ctrlport_tx_comp_gain_req_has_time),
      .s_ctrlport_req_time     (ctrlport_tx_comp_gain_req_time),
      .s_ctrlport_resp_ack     (ctrlport_tx_comp_gain_resp_ack),
      .s_ctrlport_resp_status  (ctrlport_tx_comp_gain_resp_status),
      .s_ctrlport_resp_data    (ctrlport_tx_comp_gain_resp_data),
      .data_in                 (pre_gain_radio_tx_data),
      .data_out                (radio_tx_data),
      .data_in_stb             (radio_tx_stb),
      .data_out_stb            (),
      .timestamp               (radio_time)
    );
  end else begin
    // If complex gain is not enabled, assign
    assign radio_tx_data = pre_gain_radio_tx_data;
    assign radio_tx_running = pre_gain_radio_tx_running;
  end

  //---------------------------------------------------------------------------
  // Tx Core
  //---------------------------------------------------------------------------

  radio_tx_core #(
    .SAMP_W    (SAMP_W),
    .NSPC      (NSPC)
  ) radio_tx_core_i (
    .radio_clk                 (radio_clk),
    .radio_rst                 (radio_rst),
    .s_ctrlport_req_wr         (ctrlport_tx_req_wr),
    .s_ctrlport_req_rd         (ctrlport_tx_req_rd),
    .s_ctrlport_req_addr       (ctrlport_tx_req_addr),
    .s_ctrlport_req_data       (ctrlport_tx_req_data),
    .s_ctrlport_resp_ack       (ctrlport_tx_resp_ack),
    .s_ctrlport_resp_data      (ctrlport_tx_resp_data),
    .m_ctrlport_req_wr         (ctrlport_err_tx_req_wr),
    .m_ctrlport_req_addr       (ctrlport_err_tx_req_addr),
    .m_ctrlport_req_data       (ctrlport_err_tx_req_data),
    .m_ctrlport_req_has_time   (ctrlport_err_tx_req_has_time),
    .m_ctrlport_req_time       (ctrlport_err_tx_req_time),
    .m_ctrlport_req_portid     (ctrlport_err_tx_req_portid),
    .m_ctrlport_req_rem_epid   (ctrlport_err_tx_req_rem_epid),
    .m_ctrlport_req_rem_portid (ctrlport_err_tx_req_rem_portid),
    .m_ctrlport_resp_ack       (ctrlport_err_tx_resp_ack),
    .radio_time                (radio_time),
    .radio_tx_data             (pre_gain_radio_tx_data),
    .radio_tx_stb              (radio_tx_stb),
    .radio_tx_running          (pre_gain_radio_tx_running),
    .s_axis_tdata              (s_axis_tdata),
    .s_axis_tlast              (s_axis_tlast),
    .s_axis_tvalid             (s_axis_tvalid),
    .s_axis_tready             (s_axis_tready),
    .s_axis_ttimestamp         (s_axis_ttimestamp),
    .s_axis_thas_time          (s_axis_thas_time),
    .s_axis_teob               (s_axis_teob)
  );

  //---------------------------------------------------------------------------
  // Timed Complex Gain - RX
  //---------------------------------------------------------------------------

  wire [NSPC*SAMP_W-1:0]      post_gain_radio_rx_data;

  if(EN_COMP_GAIN_RX) begin : gen_comp_gain_rx

  timed_complex_gain #(
    .ITEM_W(SAMP_W),
    .NIPC  (NSPC),
    .BASE_ADDR(REG_CGAIN_RX_OFFSET)
  ) timed_complex_gain_rx_i (
    .clk                     (radio_clk),
    .rst                     (radio_rst),
    .s_ctrlport_req_wr       (ctrlport_rx_comp_gain_req_wr),
    .s_ctrlport_req_rd       (ctrlport_rx_comp_gain_req_rd),
    .s_ctrlport_req_addr     (ctrlport_rx_comp_gain_req_addr),
    .s_ctrlport_req_data     (ctrlport_rx_comp_gain_req_data),
    .s_ctrlport_req_has_time (ctrlport_rx_comp_gain_req_has_time),
    .s_ctrlport_req_time     (ctrlport_rx_comp_gain_req_time),
    .s_ctrlport_resp_ack     (ctrlport_rx_comp_gain_resp_ack),
    .s_ctrlport_resp_status  (ctrlport_rx_comp_gain_resp_status),
    .s_ctrlport_resp_data    (ctrlport_rx_comp_gain_resp_data),
    .data_in                 (radio_rx_data_mux),
    .data_out                (post_gain_radio_rx_data),
    .data_in_stb             (radio_rx_stb_mux),
    .data_out_stb            (),
    .timestamp               (radio_time)
  );
  end else begin
    // If complex gain is not enabled, assign radio_rx_data directly
    assign post_gain_radio_rx_data = radio_rx_data_mux;
  end
  //---------------------------------------------------------------------------
  // Rx Core
  //---------------------------------------------------------------------------

  radio_rx_core #(
    .SAMP_W    (SAMP_W),
    .NSPC      (NSPC)
  ) radio_rx_core_i (
    .radio_clk                 (radio_clk),
    .radio_rst                 (radio_rst),
    .s_ctrlport_req_wr         (ctrlport_rx_req_wr),
    .s_ctrlport_req_rd         (ctrlport_rx_req_rd),
    .s_ctrlport_req_addr       (ctrlport_rx_req_addr),
    .s_ctrlport_req_data       (ctrlport_rx_req_data),
    .s_ctrlport_resp_ack       (ctrlport_rx_resp_ack),
    .s_ctrlport_resp_data      (ctrlport_rx_resp_data),
    .m_ctrlport_req_wr         (ctrlport_err_rx_req_wr),
    .m_ctrlport_req_addr       (ctrlport_err_rx_req_addr),
    .m_ctrlport_req_data       (ctrlport_err_rx_req_data),
    .m_ctrlport_req_has_time   (ctrlport_err_rx_req_has_time),
    .m_ctrlport_req_time       (ctrlport_err_rx_req_time),
    .m_ctrlport_req_portid     (ctrlport_err_rx_req_portid),
    .m_ctrlport_req_rem_epid   (ctrlport_err_rx_req_rem_epid),
    .m_ctrlport_req_rem_portid (ctrlport_err_rx_req_rem_portid),
    .m_ctrlport_resp_ack       (ctrlport_err_rx_resp_ack),
    .tx_trigger                (radio_tx_running_to_rx),
    .radio_time                (radio_time),
    .radio_rx_data             (post_gain_radio_rx_data),
    .radio_rx_stb              (radio_rx_stb_mux),
    .radio_rx_running          (radio_rx_running),
    .m_axis_tdata              (m_axis_tdata),
    .m_axis_tlast              (m_axis_tlast),
    .m_axis_tvalid             (m_axis_tvalid),
    .m_axis_tready             (m_axis_tready),
    .m_axis_ttimestamp         (m_axis_ttimestamp),
    .m_axis_thas_time          (m_axis_thas_time),
    .m_axis_teob               (m_axis_teob)
  );

endmodule
