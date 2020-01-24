//
// Copyright 2019 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: noc_shell_axi_ram_fifo
//
// Description: A NoC Shell for the RFNoC AXI RAM FIFO. This NoC Shell 
//              implements the control port interface but does nothing to the 
//              data path other than moving it to the requested clock domain.
//

`define MAX(X,Y) ((X) > (Y) ? (X) : (Y))


module noc_shell_axi_ram_fifo #(
  parameter [31:0] NOC_ID           = 32'h0,
  parameter [ 9:0] THIS_PORTID      = 10'd0,
  parameter        CHDR_W           = 64,
  parameter        DATA_W           = 64,
  parameter [ 5:0] CTRL_FIFO_SIZE   = 0,
  parameter [ 0:0] CTRLPORT_MST_EN  = 1,
  parameter [ 0:0] CTRLPORT_SLV_EN  = 1,
  parameter [ 5:0] NUM_DATA_I       = 1,
  parameter [ 5:0] NUM_DATA_O       = 1,
  parameter [ 5:0] MTU              = 10,
  parameter        SYNC_DATA_CLOCKS = 0
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
  output wire [                (NUM_DATA_I*DATA_W)-1:0] m_axis_tdata,
  output wire [(NUM_DATA_I*`MAX(DATA_W/CHDR_W, 1))-1:0] m_axis_tkeep,
  output wire [                         NUM_DATA_I-1:0] m_axis_tlast,
  output wire [                         NUM_DATA_I-1:0] m_axis_tvalid,
  input  wire [                         NUM_DATA_I-1:0] m_axis_tready,

  // Input data stream (from user logic)
  input  wire [                (NUM_DATA_O*DATA_W)-1:0] s_axis_tdata,
  input  wire [(NUM_DATA_O*`MAX(DATA_W/CHDR_W, 1))-1:0] s_axis_tkeep,
  input  wire [                         NUM_DATA_O-1:0] s_axis_tlast,
  input  wire [                         NUM_DATA_O-1:0] s_axis_tvalid,
  output wire [                         NUM_DATA_O-1:0] s_axis_tready
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
  // Data Path
  //---------------------------------------------------------------------------

  // Set WORD_W to the smaller of DATA_W and CHDR_W. This will be our common 
  // word size between the CHDR and user data ports.
  localparam WORD_W = DATA_W < CHDR_W ? DATA_W : CHDR_W;
  localparam KEEP_W = `MAX(DATA_W/CHDR_W, 1);

  genvar i;

  for (i = 0; i < NUM_DATA_I; i = i + 1) begin : gen_in
    wire [CHDR_W-1:0] temp_in_tdata;
    wire              temp_in_tlast;
    wire              temp_in_tvalid;
    wire              temp_in_tready;

    axis_packet_flush #(
      .WIDTH              (CHDR_W),
      .FLUSH_PARTIAL_PKTS (0),
      .TIMEOUT_W          (32),
      .PIPELINE           ("IN")
    ) in_packet_flush_i (
      .clk           (rfnoc_chdr_clk),
      .reset         (rfnoc_chdr_rst),
      .enable        (data_i_flush_en),
      .timeout       (data_i_flush_timeout),
      .flushing      (data_i_flush_active[i]),
      .done          (data_i_flush_done[i]),
      .s_axis_tdata  (s_rfnoc_chdr_tdata[i*CHDR_W +: CHDR_W]),
      .s_axis_tlast  (s_rfnoc_chdr_tlast[i]),
      .s_axis_tvalid (s_rfnoc_chdr_tvalid[i]),
      .s_axis_tready (s_rfnoc_chdr_tready[i]),
      .m_axis_tdata  (temp_in_tdata),
      .m_axis_tlast  (temp_in_tlast),
      .m_axis_tvalid (temp_in_tvalid),
      .m_axis_tready (temp_in_tready)
    );

    axis_width_conv #(
      .WORD_W    (WORD_W),
      .IN_WORDS  (CHDR_W/WORD_W),
      .OUT_WORDS (DATA_W/WORD_W),
      .SYNC_CLKS (SYNC_DATA_CLOCKS),
      .PIPELINE  ("NONE")
    ) in_width_conv_i (
      .s_axis_aclk   (rfnoc_chdr_clk),
      .s_axis_rst    (rfnoc_chdr_rst),
      .s_axis_tdata  (temp_in_tdata),
      .s_axis_tkeep  ({CHDR_W/WORD_W{1'b1}}),
      .s_axis_tlast  (temp_in_tlast),
      .s_axis_tvalid (temp_in_tvalid),
      .s_axis_tready (temp_in_tready),
      .m_axis_aclk   (axis_data_clk),
      .m_axis_rst    (axis_data_rst),
      .m_axis_tdata  (m_axis_tdata[i*DATA_W +: DATA_W]),
      .m_axis_tkeep  (m_axis_tkeep[i*KEEP_W +: KEEP_W]),
      .m_axis_tlast  (m_axis_tlast[i]),
      .m_axis_tvalid (m_axis_tvalid[i]),
      .m_axis_tready (m_axis_tready[i])
    );
  end


  for (i = 0; i < NUM_DATA_O; i = i + 1) begin : gen_out
    wire [       CHDR_W-1:0] temp_out_tdata;
    wire [CHDR_W/WORD_W-1:0] temp_out_tkeep;
    wire                     temp_out_tlast;
    wire                     temp_out_tvalid;
    wire                     temp_out_tready;

    axis_width_conv #(
      .WORD_W    (WORD_W),
      .IN_WORDS  (DATA_W/WORD_W),
      .OUT_WORDS (CHDR_W/WORD_W),
      .SYNC_CLKS (SYNC_DATA_CLOCKS),
      .PIPELINE  ("NONE")
    ) out_width_conv_i (
      .s_axis_aclk   (axis_data_clk),
      .s_axis_rst    (axis_data_rst),
      .s_axis_tdata  (s_axis_tdata[i*DATA_W +: DATA_W]),
      .s_axis_tkeep  (s_axis_tkeep[i*KEEP_W +: KEEP_W]),
      .s_axis_tlast  (s_axis_tlast[i]),
      .s_axis_tvalid (s_axis_tvalid[i]),
      .s_axis_tready (s_axis_tready[i]),
      .m_axis_aclk   (rfnoc_chdr_clk),
      .m_axis_rst    (rfnoc_chdr_rst),
      .m_axis_tdata  (temp_out_tdata),
      .m_axis_tkeep  (),
      .m_axis_tlast  (temp_out_tlast),
      .m_axis_tvalid (temp_out_tvalid),
      .m_axis_tready (temp_out_tready)
    );

    axis_packet_flush #(
      .WIDTH              (CHDR_W),
      .FLUSH_PARTIAL_PKTS (0),
      .TIMEOUT_W          (32),
      .PIPELINE           ("OUT")
    ) out_packet_flush_i (
      .clk           (rfnoc_chdr_clk),
      .reset         (rfnoc_chdr_rst),
      .enable        (data_o_flush_en),
      .timeout       (data_o_flush_timeout),
      .flushing      (data_o_flush_active[i]),
      .done          (data_o_flush_done[i]),
      .s_axis_tdata  (temp_out_tdata),
      .s_axis_tlast  (temp_out_tlast),
      .s_axis_tvalid (temp_out_tvalid),
      .s_axis_tready (temp_out_tready),
      .m_axis_tdata  (m_rfnoc_chdr_tdata[i*CHDR_W +: CHDR_W]),
      .m_axis_tlast  (m_rfnoc_chdr_tlast[i]),
      .m_axis_tvalid (m_rfnoc_chdr_tvalid[i]),
      .m_axis_tready (m_rfnoc_chdr_tready[i])
    );

  end

endmodule
