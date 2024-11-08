//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_fft
//
// Description:
//
//   RFNoC block for multichannel FFT/IFFT plus cyclic prefix insertion/removal.
//
// User Parameters:
//
//   THIS_PORTID              : Control crossbar port to which this block is connected
//   CHDR_W                   : AXIS-CHDR data bus width
//   MTU                      : Log2 of maximum transmission unit
//   NIPC                     : Number of samples/items per clock cycle to
//                              process internally.
//   NUM_PORTS                : Total number of FFT channels
//   NUM_CORES                : Number of individual cores to instantiate.
//                              Setting to 1 means all ports use a shared core
//                              and therefore all ports share the same control
//                              logic and all ports must be used simultaneously.
//                              Setting to NUM_PORTS means that each port will
//                              use its own core, and therefore each port can
//                              be configured and used independently. NUM_PORTS
//                              must be a multiple of NUM_CORES.
//   MAX_FFT_SIZE_LOG2        : Log2 of maximum configurable FFT size. That is,
//                              the FFT size is exactly 2**fft_size_log2.
//   EN_CP_INSERTION          : Controls whether to include the cyclic prefix
//                              insertion logic. If included, EN_FFT_ORDER must
//                              be 1.
//   EN_CP_REMOVAL            : Controls whether to include the cyclic prefix
//                              removal logic.
//   MAX_CP_LIST_LEN_INS_LOG2 : Log2 of max length of cyclic prefix insertion
//                              list. Actual max is 2**MAX_CP_LIST_LEN_INS_LOG2.
//   MAX_CP_LIST_LEN_REM_LOG2 : Log2 of max length of cyclic prefix removal
//                              list. Actual max is 2**MAX_CP_LIST_LEN_REM_LOG2.
//   CP_INSERTION_REPEAT      : Enable repeating the CP insertion list. When 1,
//                              the list repeats. When 0, CP insertion will
//                              stop when the list is finished.
//   CP_REMOVAL_REPEAT        : Enable repeating the CP removal list. When 1,
//                              the list repeats. When 0, CP removal will
//                              stop when the list is finished.
//   EN_FFT_BYPASS            : Controls whether to include the FFT bypass logic.
//   EN_FFT_ORDER             : Controls whether to include the FFT reorder logic.
//   EN_MAGNITUDE             : Controls whether to include the magnitude
//                              output calculation logic.
//   EN_MAGNITUDE_SQ          : Controls whether to include the
//                              magnitude-squared output calculation logic.
//   USE_APPROX_MAG           : Controls whether to use the low-resource
//                              approximate calculation (1) or the more exact
//                              and more resource-intensive calculation (0) for
//                              the magnitude calculation.
//

`default_nettype none


module rfnoc_block_fft #(
  logic [9:0] THIS_PORTID              = 10'd0,
  int         CHDR_W                   = 64,
  logic [5:0] MTU                      = 6'd10,
  int         NIPC                     = 1,
  int         NUM_PORTS                = 1,
  int         NUM_CORES                = 1,
  int         MAX_FFT_SIZE_LOG2        = 10,
  bit         EN_CP_REMOVAL            = 1,
  bit         EN_CP_INSERTION          = 1,
  int         MAX_CP_LIST_LEN_INS_LOG2 = 5,
  int         MAX_CP_LIST_LEN_REM_LOG2 = 5,
  bit         CP_INSERTION_REPEAT      = 1,
  bit         CP_REMOVAL_REPEAT        = 1,
  bit         EN_FFT_BYPASS            = 0,
  bit         EN_FFT_ORDER             = 1,
  bit         EN_MAGNITUDE             = 0,
  bit         EN_MAGNITUDE_SQ          = 1,
  bit         USE_APPROX_MAG           = 1
) (
  // RFNoC Framework Clocks and Resets
  input  wire                        rfnoc_chdr_clk,
  input  wire                        rfnoc_ctrl_clk,
  input  wire                        ce_clk,

  // RFNoC Backend Interface
  input  wire [               511:0] rfnoc_core_config,
  output wire [               511:0] rfnoc_core_status,

  // AXIS-CHDR Input Ports (from framework)
  input  wire [CHDR_W*NUM_PORTS-1:0] s_rfnoc_chdr_tdata,
  input  wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tlast,
  input  wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tvalid,
  output wire [       NUM_PORTS-1:0] s_rfnoc_chdr_tready,

  // AXIS-CHDR Output Ports (to framework)
  output wire [CHDR_W*NUM_PORTS-1:0] m_rfnoc_chdr_tdata,
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

  `include "usrp_utils.svh"

  import ctrlport_pkg::*;
  import rfnoc_chdr_utils_pkg::*;
  import fft_core_regs_pkg::FFT_CORE_ADDR_W;

  localparam ITEM_W = 32;

  // Calculate the number of channels per core
  localparam int NCPC = NUM_PORTS / NUM_CORES;

  // We require each FFT core instance to have the same number of channels
  if (NUM_CORES * NCPC != NUM_PORTS) begin : check_num_ports_per_core
    $error("NUM_PORTS must be a multiple of NUM_CORES");
  end


  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  // Clocks and Resets
  logic ce_rst;

  logic                       ctrlport_req_wr;
  logic                       ctrlport_req_rd;
  logic [CTRLPORT_ADDR_W-1:0] ctrlport_req_addr;
  logic [CTRLPORT_DATA_W-1:0] ctrlport_req_data;
  logic                       ctrlport_resp_ack;
  logic [CTRLPORT_DATA_W-1:0] ctrlport_resp_data;

  logic [NUM_CORES-1:0][NCPC-1:0][     ITEM_W*NIPC-1:0] in_axis_tdata;
  logic [NUM_CORES-1:0][NCPC-1:0][            NIPC-1:0] in_axis_tkeep;
  logic [NUM_CORES-1:0][NCPC-1:0][                 0:0] in_axis_tlast;
  logic [NUM_CORES-1:0][NCPC-1:0][                 0:0] in_axis_tvalid;
  logic [NUM_CORES-1:0][NCPC-1:0][                 0:0] in_axis_tready;
  logic [NUM_CORES-1:0][NCPC-1:0][CHDR_TIMESTAMP_W-1:0] in_axis_ttimestamp;
  logic [NUM_CORES-1:0][NCPC-1:0][                 0:0] in_axis_thas_time;
  logic [NUM_CORES-1:0][NCPC-1:0][   CHDR_LENGTH_W-1:0] in_axis_tlength;
  logic [NUM_CORES-1:0][NCPC-1:0][                 0:0] in_axis_teov;
  logic [NUM_CORES-1:0][NCPC-1:0][                 0:0] in_axis_teob;

  logic [NUM_CORES-1:0][NCPC-1:0][     ITEM_W*NIPC-1:0] out_axis_tdata;
  logic [NUM_CORES-1:0][NCPC-1:0][            NIPC-1:0] out_axis_tkeep;
  logic [NUM_CORES-1:0][NCPC-1:0][                 0:0] out_axis_tlast;
  logic [NUM_CORES-1:0][NCPC-1:0][                 0:0] out_axis_tvalid;
  logic [NUM_CORES-1:0][NCPC-1:0][                 0:0] out_axis_tready;
  logic [NUM_CORES-1:0][NCPC-1:0][CHDR_TIMESTAMP_W-1:0] out_axis_ttimestamp;
  logic [NUM_CORES-1:0][NCPC-1:0][                 0:0] out_axis_thas_time;
  logic [NUM_CORES-1:0][NCPC-1:0][   CHDR_LENGTH_W-1:0] out_axis_tlength;
  logic [NUM_CORES-1:0][NCPC-1:0][                 0:0] out_axis_teov;
  logic [NUM_CORES-1:0][NCPC-1:0][                 0:0] out_axis_teob;


  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  noc_shell_fft #(
    .CHDR_W     (CHDR_W),
    .THIS_PORTID(THIS_PORTID),
    .MTU        (MTU),
    .NUM_PORTS  (NUM_PORTS),
    .NIPC       (NIPC),
    .ITEM_W     (ITEM_W)
  ) noc_shell_fft_i (
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
    .m_ctrlport_req_wr    (ctrlport_req_wr),
    .m_ctrlport_req_rd    (ctrlport_req_rd),
    .m_ctrlport_req_addr  (ctrlport_req_addr),
    .m_ctrlport_req_data  (ctrlport_req_data),
    .m_ctrlport_resp_ack  (ctrlport_resp_ack),
    .m_ctrlport_resp_data (ctrlport_resp_data),
    // AXI-Stream Clock and Reset
    .axis_data_clk        (),
    .axis_data_rst        (),
    // Data Stream to User Logic: in
    .m_in_axis_tdata      (in_axis_tdata),
    .m_in_axis_tkeep      (in_axis_tkeep),
    .m_in_axis_tlast      (in_axis_tlast),
    .m_in_axis_tvalid     (in_axis_tvalid),
    .m_in_axis_tready     (in_axis_tready),
    .m_in_axis_ttimestamp (in_axis_ttimestamp),
    .m_in_axis_thas_time  (in_axis_thas_time),
    .m_in_axis_tlength    (in_axis_tlength),
    .m_in_axis_teov       (in_axis_teov),
    .m_in_axis_teob       (in_axis_teob),
    // Data Stream from User Logic: out
    .s_out_axis_tdata     (out_axis_tdata),
    .s_out_axis_tkeep     (out_axis_tkeep),
    .s_out_axis_tlast     (out_axis_tlast),
    .s_out_axis_tvalid    (out_axis_tvalid),
    .s_out_axis_tready    (out_axis_tready),
    .s_out_axis_ttimestamp(out_axis_ttimestamp),
    .s_out_axis_thas_time (out_axis_thas_time),
    .s_out_axis_tlength   (out_axis_tlength),
    .s_out_axis_teov      (out_axis_teov),
    .s_out_axis_teob      (out_axis_teob)
  );


  //---------------------------------------------------------------------------
  // CtrlPort Splitter
  //---------------------------------------------------------------------------

  logic [NUM_CORES-1:0][                0:0] dec_ctrlport_req_wr;
  logic [NUM_CORES-1:0][                0:0] dec_ctrlport_req_rd;
  logic [NUM_CORES-1:0][CTRLPORT_ADDR_W-1:0] dec_ctrlport_req_addr;
  logic [NUM_CORES-1:0][CTRLPORT_DATA_W-1:0] dec_ctrlport_req_data;
  logic [NUM_CORES-1:0][                0:0] dec_ctrlport_resp_ack;
  logic [NUM_CORES-1:0][CTRLPORT_DATA_W-1:0] dec_ctrlport_resp_data;

  generate
    if (NUM_CORES > 1) begin : gen_ctrlport_decoder
      ctrlport_decoder #(
        .NUM_SLAVES   (NUM_CORES),
        .BASE_ADDR    (0),
        .SLAVE_ADDR_W (FFT_CORE_ADDR_W)
      ) ctrlport_decoder_i (
        .ctrlport_clk            (ce_clk),
        .ctrlport_rst            (ce_rst),
        .s_ctrlport_req_wr       (ctrlport_req_wr),
        .s_ctrlport_req_rd       (ctrlport_req_rd),
        .s_ctrlport_req_addr     (ctrlport_req_addr),
        .s_ctrlport_req_data     (ctrlport_req_data),
        .s_ctrlport_req_byte_en  ('1),
        .s_ctrlport_req_has_time ('0),
        .s_ctrlport_req_time     ('0),
        .s_ctrlport_resp_ack     (ctrlport_resp_ack),
        .s_ctrlport_resp_status  (),
        .s_ctrlport_resp_data    (ctrlport_resp_data),
        .m_ctrlport_req_wr       (dec_ctrlport_req_wr),
        .m_ctrlport_req_rd       (dec_ctrlport_req_rd),
        .m_ctrlport_req_addr     (dec_ctrlport_req_addr),
        .m_ctrlport_req_data     (dec_ctrlport_req_data),
        .m_ctrlport_req_byte_en  (),
        .m_ctrlport_req_has_time (),
        .m_ctrlport_req_time     (),
        .m_ctrlport_resp_ack     (dec_ctrlport_resp_ack),
        .m_ctrlport_resp_status  ('0),
        .m_ctrlport_resp_data    (dec_ctrlport_resp_data)
      );
    end else begin : gen_no_decoder
      assign dec_ctrlport_req_wr   = ctrlport_req_wr;
      assign dec_ctrlport_req_rd   = ctrlport_req_rd;
      assign dec_ctrlport_req_addr = CTRLPORT_ADDR_W'(ctrlport_req_addr[FFT_CORE_ADDR_W-1:0]);
      assign dec_ctrlport_req_data = ctrlport_req_data;
      assign ctrlport_resp_ack     = dec_ctrlport_resp_ack;
      assign ctrlport_resp_data    = dec_ctrlport_resp_data;
    end
  endgenerate


  //---------------------------------------------------------------------------
  // FFT Core
  //---------------------------------------------------------------------------

  // Convert CHDR MTU to packet size in items
  localparam int MAX_PKT_SIZE_LOG2 = $clog2(2**MTU * CHDR_W/ITEM_W);

  for (genvar core_i = 0; core_i < NUM_CORES; core_i = core_i+1) begin : gen_fft_cores
    fft_core #(
      .NIPC                    (NIPC                    ),
      .NUM_CHAN                (NCPC                    ),
      .NUM_CORES               (NUM_CORES               ),
      .MAX_PKT_SIZE_LOG2       (MAX_PKT_SIZE_LOG2       ),
      .MAX_FFT_SIZE_LOG2       (MAX_FFT_SIZE_LOG2       ),
      .EN_CP_REMOVAL           (EN_CP_REMOVAL           ),
      .EN_CP_INSERTION         (EN_CP_INSERTION         ),
      .MAX_CP_LIST_LEN_INS_LOG2(MAX_CP_LIST_LEN_INS_LOG2),
      .MAX_CP_LIST_LEN_REM_LOG2(MAX_CP_LIST_LEN_REM_LOG2),
      .CP_INSERTION_REPEAT     (CP_INSERTION_REPEAT     ),
      .CP_REMOVAL_REPEAT       (CP_REMOVAL_REPEAT       ),
      .EN_FFT_BYPASS           (EN_FFT_BYPASS           ),
      .EN_FFT_ORDER            (EN_FFT_ORDER            ),
      .EN_MAGNITUDE            (EN_MAGNITUDE            ),
      .EN_MAGNITUDE_SQ         (EN_MAGNITUDE_SQ         ),
      .USE_APPROX_MAG          (USE_APPROX_MAG          )
    ) fft_core_i (
      .ce_clk               (ce_clk),
      .ce_rst               (ce_rst),
      .s_ctrlport_req_wr    (dec_ctrlport_req_wr   [core_i]),
      .s_ctrlport_req_rd    (dec_ctrlport_req_rd   [core_i]),
      .s_ctrlport_req_addr  (dec_ctrlport_req_addr [core_i]),
      .s_ctrlport_req_data  (dec_ctrlport_req_data [core_i]),
      .s_ctrlport_resp_ack  (dec_ctrlport_resp_ack [core_i]),
      .s_ctrlport_resp_data (dec_ctrlport_resp_data[core_i]),
      .s_in_axis_tdata      (in_axis_tdata         [core_i]),
      .s_in_axis_tkeep      (in_axis_tkeep         [core_i]),
      .s_in_axis_tlast      (in_axis_tlast         [core_i]),
      .s_in_axis_tvalid     (in_axis_tvalid        [core_i]),
      .s_in_axis_tready     (in_axis_tready        [core_i]),
      .s_in_axis_ttimestamp (in_axis_ttimestamp    [core_i]),
      .s_in_axis_thas_time  (in_axis_thas_time     [core_i]),
      .s_in_axis_tlength    (in_axis_tlength       [core_i]),
      .s_in_axis_teov       (in_axis_teov          [core_i]),
      .s_in_axis_teob       (in_axis_teob          [core_i]),
      .m_out_axis_tdata     (out_axis_tdata        [core_i]),
      .m_out_axis_tkeep     (out_axis_tkeep        [core_i]),
      .m_out_axis_tlast     (out_axis_tlast        [core_i]),
      .m_out_axis_tvalid    (out_axis_tvalid       [core_i]),
      .m_out_axis_tready    (out_axis_tready       [core_i]),
      .m_out_axis_ttimestamp(out_axis_ttimestamp   [core_i]),
      .m_out_axis_thas_time (out_axis_thas_time    [core_i]),
      .m_out_axis_tlength   (out_axis_tlength      [core_i]),
      .m_out_axis_teov      (out_axis_teov         [core_i]),
      .m_out_axis_teob      (out_axis_teob         [core_i])
    );
  end : gen_fft_cores

endmodule : rfnoc_block_fft


`default_nettype wire
