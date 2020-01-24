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
//   BASE_ADDR : Base address for this radio block instance
//   SAMP_W    : Width of a radio sample
//   NSPC      : Number of radio samples per radio clock cycle
//


module radio_core #(
  parameter SAMP_W    = 32,
  parameter NSPC      = 1
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
  output wire        s_ctrlport_resp_ack,
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
  reg         ctrlport_general_resp_ack  = 1'b0;
  reg  [31:0] ctrlport_general_resp_data = 0;

  // Tx core registers
  wire        ctrlport_tx_req_wr;
  wire        ctrlport_tx_req_rd;
  wire [19:0] ctrlport_tx_req_addr;
  wire [31:0] ctrlport_tx_req_data;
  wire        ctrlport_tx_resp_ack;
  wire [31:0] ctrlport_tx_resp_data;

  // Rx core registers
  wire        ctrlport_rx_req_wr;
  wire        ctrlport_rx_req_rd;
  wire [19:0] ctrlport_rx_req_addr;
  wire [31:0] ctrlport_rx_req_data;
  wire        ctrlport_rx_resp_ack;
  wire [31:0] ctrlport_rx_resp_data;

  ctrlport_splitter #(
    .NUM_SLAVES (3)
  ) ctrlport_decoder_i (
    .ctrlport_clk            (radio_clk),
    .ctrlport_rst            (radio_rst),
    .s_ctrlport_req_wr       (s_ctrlport_req_wr),
    .s_ctrlport_req_rd       (s_ctrlport_req_rd),
    .s_ctrlport_req_addr     (s_ctrlport_req_addr),
    .s_ctrlport_req_data     (s_ctrlport_req_data),
    .s_ctrlport_req_byte_en  (4'b0),
    .s_ctrlport_req_has_time (1'b0),
    .s_ctrlport_req_time     (64'b0),
    .s_ctrlport_resp_ack     (s_ctrlport_resp_ack),
    .s_ctrlport_resp_status  (),
    .s_ctrlport_resp_data    (s_ctrlport_resp_data),
    .m_ctrlport_req_wr       ({ctrlport_general_req_wr,
                               ctrlport_tx_req_wr,
                               ctrlport_rx_req_wr}),
    .m_ctrlport_req_rd       ({ctrlport_general_req_rd,
                               ctrlport_tx_req_rd,
                               ctrlport_rx_req_rd}),
    .m_ctrlport_req_addr     ({ctrlport_general_req_addr,
                               ctrlport_tx_req_addr,
                               ctrlport_rx_req_addr}),
    .m_ctrlport_req_data     ({ctrlport_general_req_data,
                               ctrlport_tx_req_data,
                               ctrlport_rx_req_data}),
    .m_ctrlport_req_byte_en  (),
    .m_ctrlport_req_has_time (),
    .m_ctrlport_req_time     (),
    .m_ctrlport_resp_ack     ({ctrlport_general_resp_ack,
                               ctrlport_tx_resp_ack,
                               ctrlport_rx_resp_ack}),
    .m_ctrlport_resp_status  (6'b0),
    .m_ctrlport_resp_data    ({ctrlport_general_resp_data,
                               ctrlport_tx_resp_data,
                               ctrlport_rx_resp_data})
  );


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
        endcase
      end
    end
  end


  //---------------------------------------------------------------------------
  // Tx to Rx Loopback
  //---------------------------------------------------------------------------

  wire [SAMP_W*NSPC-1:0] radio_rx_data_mux;
  wire                   radio_rx_stb_mux;

  assign radio_rx_data_mux = reg_loopback_en ? radio_tx_data : radio_rx_data;
  assign radio_rx_stb_mux  = reg_loopback_en ? radio_tx_stb  : radio_rx_stb;


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
    .radio_tx_data             (radio_tx_data),
    .radio_tx_stb              (radio_tx_stb),
    .radio_tx_running          (radio_tx_running),
    .s_axis_tdata              (s_axis_tdata),
    .s_axis_tlast              (s_axis_tlast),
    .s_axis_tvalid             (s_axis_tvalid),
    .s_axis_tready             (s_axis_tready),
    .s_axis_ttimestamp         (s_axis_ttimestamp),
    .s_axis_thas_time          (s_axis_thas_time),
    .s_axis_teob               (s_axis_teob)
  );


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
    .radio_time                (radio_time),
    .radio_rx_data             (radio_rx_data_mux),
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
