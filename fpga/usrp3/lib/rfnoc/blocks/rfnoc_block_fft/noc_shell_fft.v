//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: noc_shell_fft
//

module noc_shell_fft #(
  parameter [31:0] NOC_ID          = 32  'h0,
  parameter [ 9:0] THIS_PORTID     = 10  'd0,
  parameter        CHDR_W          = 64,
  parameter [ 0:0] CTRLPORT_SLV_EN = 1,
  parameter [ 0:0] CTRLPORT_MST_EN = 1,
  parameter        SYNC_CLKS       = 0,
  parameter [ 5:0] NUM_DATA_I      = 1,
  parameter [ 5:0] NUM_DATA_O      = 1,
  parameter        ITEM_W          = 32,
  parameter        NIPC            = 2,
  parameter        PYLD_FIFO_SIZE  = 5,
  parameter        CTXT_FIFO_SIZE  = 5,
  parameter        MTU             = 10
) (
  //---------------------------------------------------------------------------
  // Framework Interface
  //---------------------------------------------------------------------------

  // RFNoC Framework Clocks and Resets
  input  wire                           rfnoc_chdr_clk,
  output wire                           rfnoc_chdr_rst,
  input  wire                           rfnoc_ctrl_clk,
  output wire                           rfnoc_ctrl_rst,
  // RFNoC Backend Interface
  input  wire [                  511:0] rfnoc_core_config,
  output wire [                  511:0] rfnoc_core_status,
  // CHDR Input Ports (from framework)
  input  wire [(CHDR_W*NUM_DATA_I)-1:0] s_rfnoc_chdr_tdata,
  input  wire [         NUM_DATA_I-1:0] s_rfnoc_chdr_tlast,
  input  wire [         NUM_DATA_I-1:0] s_rfnoc_chdr_tvalid,
  output wire [         NUM_DATA_I-1:0] s_rfnoc_chdr_tready,
  // CHDR Output Ports (to framework)
  output wire [(CHDR_W*NUM_DATA_O)-1:0] m_rfnoc_chdr_tdata,
  output wire [         NUM_DATA_O-1:0] m_rfnoc_chdr_tlast,
  output wire [         NUM_DATA_O-1:0] m_rfnoc_chdr_tvalid,
  input  wire [         NUM_DATA_O-1:0] m_rfnoc_chdr_tready,
  // AXIS-Ctrl Input Port (from framework)
  input  wire [                   31:0] s_rfnoc_ctrl_tdata,
  input  wire                           s_rfnoc_ctrl_tlast,
  input  wire                           s_rfnoc_ctrl_tvalid,
  output wire                           s_rfnoc_ctrl_tready,
  // AXIS-Ctrl Output Port (to framework)
  output wire [                   31:0] m_rfnoc_ctrl_tdata,
  output wire                           m_rfnoc_ctrl_tlast,
  output wire                           m_rfnoc_ctrl_tvalid,
  input  wire                           m_rfnoc_ctrl_tready,

  //---------------------------------------------------------------------------
  // Client Control Port Interface
  //---------------------------------------------------------------------------

  // Clock
  input  wire        ctrlport_clk,
  input  wire        ctrlport_rst,
  // Master
  output wire        m_ctrlport_req_wr,
  output wire        m_ctrlport_req_rd,
  output wire [19:0] m_ctrlport_req_addr,
  output wire [31:0] m_ctrlport_req_data,
  output wire [ 3:0] m_ctrlport_req_byte_en,
  output wire        m_ctrlport_req_has_time,
  output wire [63:0] m_ctrlport_req_time,
  input  wire        m_ctrlport_resp_ack,
  input  wire [ 1:0] m_ctrlport_resp_status,
  input  wire [31:0] m_ctrlport_resp_data,
  // Slave
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [ 9:0] s_ctrlport_req_portid,
  input  wire [15:0] s_ctrlport_req_rem_epid,
  input  wire [ 9:0] s_ctrlport_req_rem_portid,
  input  wire [31:0] s_ctrlport_req_data,
  input  wire [ 3:0] s_ctrlport_req_byte_en,
  input  wire        s_ctrlport_req_has_time,
  input  wire [63:0] s_ctrlport_req_time,
  output wire        s_ctrlport_resp_ack,
  output wire [ 1:0] s_ctrlport_resp_status,
  output wire [31:0] s_ctrlport_resp_data,

  //---------------------------------------------------------------------------
  // Client Data Interface
  //---------------------------------------------------------------------------

  // Clock
  input wire axis_data_clk,
  input wire axis_data_rst,

  // Output data stream (to user logic)
  output wire [(NUM_DATA_I*ITEM_W*NIPC)-1:0] m_axis_payload_tdata,
  output wire [       (NUM_DATA_I*NIPC)-1:0] m_axis_payload_tkeep,
  output wire [              NUM_DATA_I-1:0] m_axis_payload_tlast,
  output wire [              NUM_DATA_I-1:0] m_axis_payload_tvalid,
  input  wire [              NUM_DATA_I-1:0] m_axis_payload_tready,

  // Input data stream (from user logic)
  input  wire [(NUM_DATA_O*ITEM_W*NIPC)-1:0] s_axis_payload_tdata,
  input  wire [       (NUM_DATA_O*NIPC)-1:0] s_axis_payload_tkeep,
  input  wire [              NUM_DATA_O-1:0] s_axis_payload_tlast,
  input  wire [              NUM_DATA_O-1:0] s_axis_payload_tvalid,
  output wire [              NUM_DATA_O-1:0] s_axis_payload_tready,

  // Output context stream (to user logic)
  output wire [(NUM_DATA_I*CHDR_W)-1:0] m_axis_context_tdata,
  output wire [     (4*NUM_DATA_I)-1:0] m_axis_context_tuser,
  output wire [         NUM_DATA_I-1:0] m_axis_context_tlast,
  output wire [         NUM_DATA_I-1:0] m_axis_context_tvalid,
  input  wire [         NUM_DATA_I-1:0] m_axis_context_tready,

  // Input context stream (from user logic)
  input  wire [(NUM_DATA_O*CHDR_W)-1:0] s_axis_context_tdata,
  input  wire [     (4*NUM_DATA_O)-1:0] s_axis_context_tuser,
  input  wire [         NUM_DATA_O-1:0] s_axis_context_tlast,
  input  wire [         NUM_DATA_O-1:0] s_axis_context_tvalid,
  output wire [         NUM_DATA_O-1:0] s_axis_context_tready
);
  
  localparam CTRL_FIFO_SIZE = 5;


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
    .NOC_ID        (NOC_ID),
    .NUM_DATA_I    (NUM_DATA_I),
    .NUM_DATA_O    (NUM_DATA_O),
    .CTRL_FIFOSIZE (CTRL_FIFO_SIZE),
    .MTU           (MTU)
  ) backend_iface_i (
    .rfnoc_chdr_clk       (rfnoc_chdr_clk),
    .rfnoc_ctrl_clk       (rfnoc_ctrl_clk),
    .rfnoc_core_config    (rfnoc_core_config),
    .rfnoc_core_status    (rfnoc_core_status),
    .rfnoc_chdr_rst       (rfnoc_chdr_rst),
    .rfnoc_ctrl_rst       (rfnoc_ctrl_rst),
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

  ctrlport_endpoint #(
    .THIS_PORTID              (THIS_PORTID    ),
    .SYNC_CLKS                (0              ),
    .AXIS_CTRL_MST_EN         (CTRLPORT_SLV_EN),
    .AXIS_CTRL_SLV_EN         (CTRLPORT_MST_EN),
    .SLAVE_FIFO_SIZE          (CTRL_FIFO_SIZE )
  ) ctrlport_ep_i (
    .rfnoc_ctrl_clk           (rfnoc_ctrl_clk           ),
    .rfnoc_ctrl_rst           (rfnoc_ctrl_rst           ),
    .ctrlport_clk             (ctrlport_clk             ),
    .ctrlport_rst             (ctrlport_rst             ),
    .s_rfnoc_ctrl_tdata       (s_rfnoc_ctrl_tdata       ),
    .s_rfnoc_ctrl_tlast       (s_rfnoc_ctrl_tlast       ),
    .s_rfnoc_ctrl_tvalid      (s_rfnoc_ctrl_tvalid      ),
    .s_rfnoc_ctrl_tready      (s_rfnoc_ctrl_tready      ),
    .m_rfnoc_ctrl_tdata       (m_rfnoc_ctrl_tdata       ),
    .m_rfnoc_ctrl_tlast       (m_rfnoc_ctrl_tlast       ),
    .m_rfnoc_ctrl_tvalid      (m_rfnoc_ctrl_tvalid      ),
    .m_rfnoc_ctrl_tready      (m_rfnoc_ctrl_tready      ),
    .m_ctrlport_req_wr        (m_ctrlport_req_wr        ),
    .m_ctrlport_req_rd        (m_ctrlport_req_rd        ),
    .m_ctrlport_req_addr      (m_ctrlport_req_addr      ),
    .m_ctrlport_req_data      (m_ctrlport_req_data      ),
    .m_ctrlport_req_byte_en   (m_ctrlport_req_byte_en   ),
    .m_ctrlport_req_has_time  (m_ctrlport_req_has_time  ),
    .m_ctrlport_req_time      (m_ctrlport_req_time      ),
    .m_ctrlport_resp_ack      (m_ctrlport_resp_ack      ),
    .m_ctrlport_resp_status   (m_ctrlport_resp_status   ),
    .m_ctrlport_resp_data     (m_ctrlport_resp_data     ),
    .s_ctrlport_req_wr        (s_ctrlport_req_wr        ),
    .s_ctrlport_req_rd        (s_ctrlport_req_rd        ),
    .s_ctrlport_req_addr      (s_ctrlport_req_addr      ),
    .s_ctrlport_req_portid    (s_ctrlport_req_portid    ),
    .s_ctrlport_req_rem_epid  (s_ctrlport_req_rem_epid  ),
    .s_ctrlport_req_rem_portid(s_ctrlport_req_rem_portid),
    .s_ctrlport_req_data      (s_ctrlport_req_data      ),
    .s_ctrlport_req_byte_en   (s_ctrlport_req_byte_en   ),
    .s_ctrlport_req_has_time  (s_ctrlport_req_has_time  ),
    .s_ctrlport_req_time      (s_ctrlport_req_time      ),
    .s_ctrlport_resp_ack      (s_ctrlport_resp_ack      ),
    .s_ctrlport_resp_status   (s_ctrlport_resp_status   ),
    .s_ctrlport_resp_data     (s_ctrlport_resp_data     )
  );

  //---------------------------------------------------------------------------
  //  Data Path
  //---------------------------------------------------------------------------

  genvar i;
  generate

    for (i = 0; i < NUM_DATA_I; i = i + 1) begin: chdr_to_data
      chdr_to_axis_pyld_ctxt #(
        .CHDR_W               (CHDR_W        ),
        .ITEM_W               (ITEM_W        ),
        .NIPC                 (NIPC          ),
        .SYNC_CLKS            (SYNC_CLKS     ),
        .CONTEXT_FIFO_SIZE    (CTXT_FIFO_SIZE),
        .PAYLOAD_FIFO_SIZE    (PYLD_FIFO_SIZE),
        .CONTEXT_PREFETCH_EN  (1             )
      ) chdr_to_axis_pyld_ctxt_i (
        .axis_chdr_clk        (rfnoc_chdr_clk                                       ),
        .axis_chdr_rst        (rfnoc_chdr_rst                                       ),
        .axis_data_clk        (axis_data_clk                                        ),
        .axis_data_rst        (axis_data_rst                                        ),
        .s_axis_chdr_tdata    (s_rfnoc_chdr_tdata   [(i*CHDR_W)+:CHDR_W]            ),
        .s_axis_chdr_tlast    (s_rfnoc_chdr_tlast   [i]                             ),
        .s_axis_chdr_tvalid   (s_rfnoc_chdr_tvalid  [i]                             ),
        .s_axis_chdr_tready   (s_rfnoc_chdr_tready  [i]                             ),
        .m_axis_payload_tdata (m_axis_payload_tdata [(i*ITEM_W*NIPC)+:(ITEM_W*NIPC)]),
        .m_axis_payload_tkeep (m_axis_payload_tkeep [(i*NIPC)+:NIPC]                ),
        .m_axis_payload_tlast (m_axis_payload_tlast [i]                             ),
        .m_axis_payload_tvalid(m_axis_payload_tvalid[i]                             ),
        .m_axis_payload_tready(m_axis_payload_tready[i]                             ),
        .m_axis_context_tdata (m_axis_context_tdata [(i*CHDR_W)+:(CHDR_W)]          ),
        .m_axis_context_tuser (m_axis_context_tuser [(i*4)+:4]                      ),
        .m_axis_context_tlast (m_axis_context_tlast [i]                             ),
        .m_axis_context_tvalid(m_axis_context_tvalid[i]                             ),
        .m_axis_context_tready(m_axis_context_tready[i]                             ),
        .flush_en             (data_i_flush_en                                      ),
        .flush_timeout        (data_i_flush_timeout                                 ),
        .flush_active         (data_i_flush_active  [i]                             ),
        .flush_done           (data_i_flush_done    [i]                             )
      );
    end

    for (i = 0; i < NUM_DATA_O; i = i + 1) begin: data_to_chdr
      axis_pyld_ctxt_to_chdr #(
        .CHDR_W               (CHDR_W        ),
        .ITEM_W               (ITEM_W        ),
        .NIPC                 (NIPC          ),
        .SYNC_CLKS            (SYNC_CLKS     ),
        .CONTEXT_FIFO_SIZE    (CTXT_FIFO_SIZE),
        .PAYLOAD_FIFO_SIZE    (PYLD_FIFO_SIZE),
        .CONTEXT_PREFETCH_EN  (1             ),
        .MTU                  (MTU           )
      ) axis_pyld_ctxt_to_chdr_i (
        .axis_chdr_clk        (rfnoc_chdr_clk                                       ),
        .axis_chdr_rst        (rfnoc_chdr_rst                                       ),
        .axis_data_clk        (axis_data_clk                                        ),
        .axis_data_rst        (axis_data_rst                                        ),
        .m_axis_chdr_tdata    (m_rfnoc_chdr_tdata   [(i*CHDR_W)+:CHDR_W]            ),
        .m_axis_chdr_tlast    (m_rfnoc_chdr_tlast   [i]                             ),
        .m_axis_chdr_tvalid   (m_rfnoc_chdr_tvalid  [i]                             ),
        .m_axis_chdr_tready   (m_rfnoc_chdr_tready  [i]                             ),
        .s_axis_payload_tdata (s_axis_payload_tdata [(i*ITEM_W*NIPC)+:(ITEM_W*NIPC)]),
        .s_axis_payload_tkeep (s_axis_payload_tkeep [(i*NIPC)+:NIPC]                ),
        .s_axis_payload_tlast (s_axis_payload_tlast [i]                             ),
        .s_axis_payload_tvalid(s_axis_payload_tvalid[i]                             ),
        .s_axis_payload_tready(s_axis_payload_tready[i]                             ),
        .s_axis_context_tdata (s_axis_context_tdata [(i*CHDR_W)+:(CHDR_W)]          ),
        .s_axis_context_tuser (s_axis_context_tuser [(i*4)+:4]                      ),
        .s_axis_context_tlast (s_axis_context_tlast [i]                             ),
        .s_axis_context_tvalid(s_axis_context_tvalid[i]                             ),
        .s_axis_context_tready(s_axis_context_tready[i]                             ),
        .framer_errors        (                                                     ),
        .flush_en             (data_o_flush_en                                      ),
        .flush_timeout        (data_o_flush_timeout                                 ),
        .flush_active         (data_o_flush_active  [i]                             ),
        .flush_done           (data_o_flush_done    [i]                             )
      );
    end
  endgenerate

endmodule
