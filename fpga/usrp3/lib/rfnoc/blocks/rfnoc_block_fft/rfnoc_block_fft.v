//
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_block_fft
//
// Description:  An FFT block for RFNoC.
//
// Parameters:
//
//   THIS_PORTID   : Control crossbar port to which this block is connected
//   CHDR_W        : AXIS CHDR interface data width
//   MTU           : Maximum transmission unit (i.e., maximum packet size) in
//                   CHDR words is 2**MTU.
//   EN_MAGNITUDE_OUT : CORDIC based magnitude calculation
//   EN_MAGNITUDE_APPROX_OUT : Multipler-less, lower resource usage
//   EN_MAGNITUDE_SQ_OUT : Magnitude squared
//   EN_FFT_SHIFT : Center zero frequency bin
//

module rfnoc_block_fft #(
  parameter THIS_PORTID   = 0,
  parameter CHDR_W        = 64,
  parameter MTU           = 10,

  parameter EN_MAGNITUDE_OUT = 0,        
  parameter EN_MAGNITUDE_APPROX_OUT = 1,
  parameter EN_MAGNITUDE_SQ_OUT = 1,    
  parameter EN_FFT_SHIFT = 1
  )
(
  //---------------------------------------------------------------------------
  // AXIS CHDR Port
  //---------------------------------------------------------------------------

  input wire rfnoc_chdr_clk,
  input wire ce_clk,

  // CHDR inputs from framework
  input  wire [CHDR_W-1:0] s_rfnoc_chdr_tdata,
  input  wire s_rfnoc_chdr_tlast,
  input  wire s_rfnoc_chdr_tvalid,
  output wire s_rfnoc_chdr_tready,

  // CHDR outputs to framework
  output wire [CHDR_W-1:0] m_rfnoc_chdr_tdata,
  output wire m_rfnoc_chdr_tlast,
  output wire m_rfnoc_chdr_tvalid,
  input  wire m_rfnoc_chdr_tready,

  // Backend interface
  input  wire [511:0] rfnoc_core_config,
  output wire [511:0] rfnoc_core_status,

  //---------------------------------------------------------------------------
  // AXIS CTRL Port
  //---------------------------------------------------------------------------

  input wire rfnoc_ctrl_clk,

  // CTRL port requests from framework
  input  wire [31:0] s_rfnoc_ctrl_tdata,
  input  wire        s_rfnoc_ctrl_tlast,
  input  wire        s_rfnoc_ctrl_tvalid,
  output wire        s_rfnoc_ctrl_tready,

  // CTRL port requests to framework
  output wire [31:0] m_rfnoc_ctrl_tdata,
  output wire        m_rfnoc_ctrl_tlast,
  output wire        m_rfnoc_ctrl_tvalid,
  input  wire        m_rfnoc_ctrl_tready
);

  // These are the only supported values for now
  localparam ITEM_W = 32;
  localparam NIPC   = 1;

  localparam NOC_ID = 32'hFF70_0000;

  `include "../../core/rfnoc_axis_ctrl_utils.vh"

  //---------------------------------------------------------------------------
  // Signal Declarations
  //---------------------------------------------------------------------------

  wire rfnoc_chdr_rst;

  wire        ctrlport_req_wr;
  wire        ctrlport_req_rd;
  wire [19:0] ctrlport_req_addr;
  wire [31:0] ctrlport_req_data;
  wire        ctrlport_req_has_time;
  wire [63:0] ctrlport_req_time;
  wire        ctrlport_resp_ack;
  wire [31:0] ctrlport_resp_data;

  wire [ITEM_W-1:0] axis_to_fft_tdata;
  wire  axis_to_fft_tlast;
  wire  axis_to_fft_tvalid;
  wire  axis_to_fft_tready;

  wire [ITEM_W-1:0] axis_from_fft_tdata;
  wire  axis_from_fft_tlast;
  wire  axis_from_fft_tvalid;
  wire  axis_from_fft_tready;

  wire [CHDR_W-1:0] m_axis_context_tdata;
  wire [       3:0] m_axis_context_tuser;
  wire [       0:0] m_axis_context_tlast;
  wire [       0:0] m_axis_context_tvalid;
  wire [       0:0] m_axis_context_tready;

  wire [CHDR_W-1:0] s_axis_context_tdata;
  wire [       3:0] s_axis_context_tuser;
  wire [       0:0] s_axis_context_tlast;
  wire [       0:0] s_axis_context_tvalid;
  wire [       0:0] s_axis_context_tready;

  wire ce_rst;

  // Cross the CHDR reset to the radio_clk domain
  pulse_synchronizer #(
    .MODE ("POSEDGE")
  ) ctrl_rst_sync_i (
    .clk_a   (rfnoc_chdr_clk),
    .rst_a   (1'b0),
    .pulse_a (rfnoc_chdr_rst),
    .busy_a  (),
    .clk_b   (ce_clk),
    .pulse_b (ce_rst)
  );

  //---------------------------------------------------------------------------
  // NoC Shell
  //---------------------------------------------------------------------------

  noc_shell_fft #(
    .NOC_ID         (NOC_ID     ),
    .THIS_PORTID    (THIS_PORTID),
    .CHDR_W         (CHDR_W     ),
    .CTRLPORT_SLV_EN(0          ),
    .CTRLPORT_MST_EN(1          ),
    .SYNC_CLKS      (0          ),
    .NUM_DATA_I     (1          ),
    .NUM_DATA_O     (1          ),
    .ITEM_W         (ITEM_W     ),
    .NIPC           (NIPC       ),
    .PYLD_FIFO_SIZE (MTU        ),
    .CTXT_FIFO_SIZE (1          ),
    .MTU            (MTU        )
  ) noc_shell_fft_i (
    .rfnoc_chdr_clk           (rfnoc_chdr_clk       ),
    .rfnoc_chdr_rst           (rfnoc_chdr_rst       ),
    .rfnoc_ctrl_clk           (rfnoc_ctrl_clk       ),
    .rfnoc_ctrl_rst           (                     ),
    .rfnoc_core_config        (rfnoc_core_config    ),
    .rfnoc_core_status        (rfnoc_core_status    ),
    .s_rfnoc_chdr_tdata       (s_rfnoc_chdr_tdata   ),
    .s_rfnoc_chdr_tlast       (s_rfnoc_chdr_tlast   ),
    .s_rfnoc_chdr_tvalid      (s_rfnoc_chdr_tvalid  ),
    .s_rfnoc_chdr_tready      (s_rfnoc_chdr_tready  ),
    .m_rfnoc_chdr_tdata       (m_rfnoc_chdr_tdata   ),
    .m_rfnoc_chdr_tlast       (m_rfnoc_chdr_tlast   ),
    .m_rfnoc_chdr_tvalid      (m_rfnoc_chdr_tvalid  ),
    .m_rfnoc_chdr_tready      (m_rfnoc_chdr_tready  ),
    .s_rfnoc_ctrl_tdata       (s_rfnoc_ctrl_tdata   ),
    .s_rfnoc_ctrl_tlast       (s_rfnoc_ctrl_tlast   ),
    .s_rfnoc_ctrl_tvalid      (s_rfnoc_ctrl_tvalid  ),
    .s_rfnoc_ctrl_tready      (s_rfnoc_ctrl_tready  ),
    .m_rfnoc_ctrl_tdata       (m_rfnoc_ctrl_tdata   ),
    .m_rfnoc_ctrl_tlast       (m_rfnoc_ctrl_tlast   ),
    .m_rfnoc_ctrl_tvalid      (m_rfnoc_ctrl_tvalid  ),
    .m_rfnoc_ctrl_tready      (m_rfnoc_ctrl_tready  ),
    .ctrlport_clk             (ce_clk               ),
    .ctrlport_rst             (ce_rst               ),
    .m_ctrlport_req_wr        (ctrlport_req_wr      ),
    .m_ctrlport_req_rd        (ctrlport_req_rd      ),
    .m_ctrlport_req_addr      (ctrlport_req_addr    ),
    .m_ctrlport_req_data      (ctrlport_req_data    ),
    .m_ctrlport_req_byte_en   (                     ),
    .m_ctrlport_req_has_time  (ctrlport_req_has_time),
    .m_ctrlport_req_time      (ctrlport_req_time    ),
    .m_ctrlport_resp_ack      (ctrlport_resp_ack    ),
    .m_ctrlport_resp_status   (AXIS_CTRL_STS_OKAY   ),
    .m_ctrlport_resp_data     (ctrlport_resp_data   ),
    .s_ctrlport_req_wr        (1'b0                 ),
    .s_ctrlport_req_rd        (1'b0                 ),
    .s_ctrlport_req_addr      (20'b0                ),
    .s_ctrlport_req_portid    (10'b0                ),
    .s_ctrlport_req_rem_epid  (16'b0                ),
    .s_ctrlport_req_rem_portid(10'b0                ),
    .s_ctrlport_req_data      (32'b0                ),
    .s_ctrlport_req_byte_en   (4'b0                 ),
    .s_ctrlport_req_has_time  (1'b0                 ),
    .s_ctrlport_req_time      (64'b0                ),
    .s_ctrlport_resp_ack      (                     ),
    .s_ctrlport_resp_status   (                     ),
    .s_ctrlport_resp_data     (                     ),
    .axis_data_clk            (ce_clk               ),
    .axis_data_rst            (ce_rst               ),
    .m_axis_payload_tdata     (axis_to_fft_tdata    ),
    .m_axis_payload_tkeep     (                     ),
    .m_axis_payload_tlast     (axis_to_fft_tlast    ),
    .m_axis_payload_tvalid    (axis_to_fft_tvalid   ),
    .m_axis_payload_tready    (axis_to_fft_tready   ),
    .s_axis_payload_tdata     (axis_from_fft_tdata  ),
    .s_axis_payload_tkeep     ({1*NIPC{1'b1}}       ),
    .s_axis_payload_tlast     (axis_from_fft_tlast  ),
    .s_axis_payload_tvalid    (axis_from_fft_tvalid ),
    .s_axis_payload_tready    (axis_from_fft_tready ),
    .m_axis_context_tdata     (m_axis_context_tdata ),
    .m_axis_context_tuser     (m_axis_context_tuser ),
    .m_axis_context_tlast     (m_axis_context_tlast ),
    .m_axis_context_tvalid    (m_axis_context_tvalid),
    .m_axis_context_tready    (m_axis_context_tready),
    .s_axis_context_tdata     (s_axis_context_tdata ),
    .s_axis_context_tuser     (s_axis_context_tuser ),
    .s_axis_context_tlast     (s_axis_context_tlast ),
    .s_axis_context_tvalid    (s_axis_context_tvalid),
    .s_axis_context_tready    (s_axis_context_tready)
  );

  // The input packets are the same configuration as the output packets, so
  // just use the header information for each incoming to create the header for
  // each outgoing packet. This is done by connecting m_axis_context to
  // directly to s_axis_context.
  assign s_axis_context_tdata  = m_axis_context_tdata;
  assign s_axis_context_tuser  = m_axis_context_tuser;
  assign s_axis_context_tlast  = m_axis_context_tlast;
  assign s_axis_context_tvalid = m_axis_context_tvalid;
  assign m_axis_context_tready = s_axis_context_tready;

  wire [ 8-1:0] set_addr;
  wire [32-1:0] set_data;
  wire  set_has_time;
  wire  set_stb;
  wire [ 8-1:0] rb_addr;
  reg  [64-1:0] rb_data;

  ctrlport_to_settings_bus # (
    .NUM_PORTS (1)
  ) ctrlport_to_settings_bus_i (
    .ctrlport_clk             (ce_clk),
    .ctrlport_rst             (ce_rst),
    .s_ctrlport_req_wr        (ctrlport_req_wr),
    .s_ctrlport_req_rd        (ctrlport_req_rd),
    .s_ctrlport_req_addr      (ctrlport_req_addr),
    .s_ctrlport_req_data      (ctrlport_req_data),
    .s_ctrlport_req_has_time  (ctrlport_req_has_time),
    .s_ctrlport_req_time      (ctrlport_req_time),
    .s_ctrlport_resp_ack      (ctrlport_resp_ack),
    .s_ctrlport_resp_data     (ctrlport_resp_data),
    .set_data                 (set_data),
    .set_addr                 (set_addr),
    .set_stb                  (set_stb),
    .set_time                 (),
    .set_has_time             (set_has_time),
    .rb_stb                   (1'b1),
    .rb_addr                  (rb_addr),
    .rb_data                  (rb_data));

  localparam MAX_FFT_SIZE_LOG2          = 11;

  localparam [31:0] SR_FFT_RESET        = 131;
  localparam [31:0] SR_FFT_SIZE_LOG2    = 132;
  localparam [31:0] SR_MAGNITUDE_OUT    = 133;
  localparam [31:0] SR_FFT_DIRECTION    = 134;
  localparam [31:0] SR_FFT_SCALING      = 135;
  localparam [31:0] SR_FFT_SHIFT_CONFIG = 136;

  // FFT Output
  localparam [1:0] COMPLEX_OUT = 0;
  localparam [1:0] MAG_OUT     = 1;
  localparam [1:0] MAG_SQ_OUT  = 2;

  // FFT Direction
  localparam [0:0] FFT_REVERSE = 0;
  localparam [0:0] FFT_FORWARD = 1;

  wire [1:0]  magnitude_out;
  wire [31:0] fft_data_o_tdata;
  wire        fft_data_o_tlast;
  wire        fft_data_o_tvalid;
  wire        fft_data_o_tready;
  wire [15:0] fft_data_o_tuser;
  wire [31:0] fft_shift_o_tdata;
  wire        fft_shift_o_tlast;
  wire        fft_shift_o_tvalid;
  wire        fft_shift_o_tready;
  wire [31:0] fft_mag_i_tdata, fft_mag_o_tdata, fft_mag_o_tdata_int;
  wire        fft_mag_i_tlast, fft_mag_o_tlast;
  wire        fft_mag_i_tvalid, fft_mag_o_tvalid;
  wire        fft_mag_i_tready, fft_mag_o_tready;
  wire [31:0] fft_mag_sq_i_tdata, fft_mag_sq_o_tdata;
  wire        fft_mag_sq_i_tlast, fft_mag_sq_o_tlast;
  wire        fft_mag_sq_i_tvalid, fft_mag_sq_o_tvalid;
  wire        fft_mag_sq_i_tready, fft_mag_sq_o_tready;
  wire [31:0] fft_mag_round_i_tdata, fft_mag_round_o_tdata;
  wire        fft_mag_round_i_tlast, fft_mag_round_o_tlast;
  wire        fft_mag_round_i_tvalid, fft_mag_round_o_tvalid;
  wire        fft_mag_round_i_tready, fft_mag_round_o_tready;

  // Settings Registers
  wire fft_reset;
  setting_reg #(
    .my_addr(SR_FFT_RESET), .awidth(8), .width(1))
  sr_fft_reset (
    .clk(ce_clk), .rst(ce_rst),
    .strobe(set_stb), .addr(set_addr), .in(set_data), .out(fft_reset), .changed());

  // Two instances of FFT size register, one for FFT core and one for FFT shift
  localparam DEFAULT_FFT_SIZE = 8; // 256
  wire [7:0] fft_size_log2_tdata ,fft_core_size_log2_tdata;
  wire fft_size_log2_tvalid, fft_core_size_log2_tvalid, fft_size_log2_tready, fft_core_size_log2_tready;
  axi_setting_reg #(
    .ADDR(SR_FFT_SIZE_LOG2), .AWIDTH(8), .WIDTH(8), .DATA_AT_RESET(DEFAULT_FFT_SIZE), .VALID_AT_RESET(1))
  sr_fft_size_log2 (
    .clk(ce_clk), .reset(ce_rst),
    .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
    .o_tdata(fft_size_log2_tdata), .o_tlast(), .o_tvalid(fft_size_log2_tvalid), .o_tready(fft_size_log2_tready));

  axi_setting_reg #(
    .ADDR(SR_FFT_SIZE_LOG2), .AWIDTH(8), .WIDTH(8), .DATA_AT_RESET(DEFAULT_FFT_SIZE), .VALID_AT_RESET(1))
  sr_fft_size_log2_2 (
    .clk(ce_clk), .reset(ce_rst),
    .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
    .o_tdata(fft_core_size_log2_tdata), .o_tlast(), .o_tvalid(fft_core_size_log2_tvalid), .o_tready(fft_core_size_log2_tready));

  // Forward = 0, Reverse = 1
  localparam DEFAULT_FFT_DIRECTION = 0;
  wire fft_direction_tdata;
  wire fft_direction_tvalid, fft_direction_tready;
  axi_setting_reg #(
    .ADDR(SR_FFT_DIRECTION), .AWIDTH(8), .WIDTH(1), .DATA_AT_RESET(DEFAULT_FFT_DIRECTION), .VALID_AT_RESET(1))
  sr_fft_direction (
    .clk(ce_clk), .reset(ce_rst),
    .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
    .o_tdata(fft_direction_tdata), .o_tlast(), .o_tvalid(fft_direction_tvalid), .o_tready(fft_direction_tready));

  localparam [11:0] DEFAULT_FFT_SCALING = 12'b011010101010; // Conservative 1/N scaling
  wire [11:0] fft_scaling_tdata;
  wire fft_scaling_tvalid, fft_scaling_tready;
  axi_setting_reg #(
    .ADDR(SR_FFT_SCALING), .AWIDTH(8), .WIDTH(12), .DATA_AT_RESET(DEFAULT_FFT_SCALING), .VALID_AT_RESET(1))
  sr_fft_scaling (
    .clk(ce_clk), .reset(ce_rst),
    .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
    .o_tdata(fft_scaling_tdata), .o_tlast(), .o_tvalid(fft_scaling_tvalid), .o_tready(fft_scaling_tready));

  wire [1:0] fft_shift_config_tdata;
  wire fft_shift_config_tvalid, fft_shift_config_tready;
  axi_setting_reg #(
    .ADDR(SR_FFT_SHIFT_CONFIG), .AWIDTH(8), .WIDTH(2))
  sr_fft_shift_config (
    .clk(ce_clk), .reset(ce_rst),
    .set_stb(set_stb), .set_addr(set_addr), .set_data(set_data),
    .o_tdata(fft_shift_config_tdata), .o_tlast(), .o_tvalid(fft_shift_config_tvalid), .o_tready(fft_shift_config_tready));

  // Synchronize writing configuration to the FFT core
  reg fft_config_ready;
  wire fft_config_write = fft_config_ready & axis_to_fft_tvalid & axis_to_fft_tready;
  always @(posedge ce_clk) begin
    if (ce_rst | fft_reset) begin
      fft_config_ready   <= 1'b1;
    end else begin
      if (fft_config_write) begin
        fft_config_ready <= 1'b0;
      end else if (axis_to_fft_tlast) begin
        fft_config_ready <= 1'b1;
      end
    end
  end

  wire [23:0] fft_config_tdata     = {3'd0, fft_scaling_tdata, fft_direction_tdata, fft_core_size_log2_tdata};
  wire fft_config_tvalid           = fft_config_write & (fft_scaling_tvalid | fft_direction_tvalid | fft_core_size_log2_tvalid);
  wire fft_config_tready;
  assign fft_core_size_log2_tready = fft_config_tready & fft_config_write;
  assign fft_direction_tready      = fft_config_tready & fft_config_write;
  assign fft_scaling_tready        = fft_config_tready & fft_config_write;
  axi_fft inst_axi_fft (
    .aclk(ce_clk), .aresetn(~(fft_reset)),
    .s_axis_data_tvalid(axis_to_fft_tvalid),
    .s_axis_data_tready(axis_to_fft_tready),
    .s_axis_data_tlast(axis_to_fft_tlast),
    .s_axis_data_tdata({axis_to_fft_tdata[15:0],axis_to_fft_tdata[31:16]}),
    .m_axis_data_tvalid(fft_data_o_tvalid),
    .m_axis_data_tready(fft_data_o_tready),
    .m_axis_data_tlast(fft_data_o_tlast),
    .m_axis_data_tdata({fft_data_o_tdata[15:0],fft_data_o_tdata[31:16]}),
    .m_axis_data_tuser(fft_data_o_tuser), // FFT index
    .s_axis_config_tdata(fft_config_tdata),
    .s_axis_config_tvalid(fft_config_tvalid),
    .s_axis_config_tready(fft_config_tready),
    .event_frame_started(),
    .event_tlast_unexpected(),
    .event_tlast_missing(),
    .event_status_channel_halt(),
    .event_data_in_channel_halt(),
    .event_data_out_channel_halt());

  // Mux control signals
  assign fft_shift_o_tready     = (magnitude_out == MAG_OUT)    ? fft_mag_i_tready         :
                                  (magnitude_out == MAG_SQ_OUT) ? fft_mag_sq_i_tready      : axis_from_fft_tready;
  assign fft_mag_i_tvalid       = (magnitude_out == MAG_OUT)    ? fft_shift_o_tvalid       : 1'b0;
  assign fft_mag_i_tlast        = (magnitude_out == MAG_OUT)    ? fft_shift_o_tlast        : 1'b0;
  assign fft_mag_i_tdata        = fft_shift_o_tdata;
  assign fft_mag_o_tready       = (magnitude_out == MAG_OUT)    ? fft_mag_round_i_tready   : 1'b0;
  assign fft_mag_sq_i_tvalid    = (magnitude_out == MAG_SQ_OUT) ? fft_shift_o_tvalid       : 1'b0;
  assign fft_mag_sq_i_tlast     = (magnitude_out == MAG_SQ_OUT) ? fft_shift_o_tlast        : 1'b0;
  assign fft_mag_sq_i_tdata     = fft_shift_o_tdata;
  assign fft_mag_sq_o_tready    = (magnitude_out == MAG_SQ_OUT) ? fft_mag_round_i_tready   : 1'b0;
  assign fft_mag_round_i_tvalid = (magnitude_out == MAG_OUT)    ? fft_mag_o_tvalid         :
                                  (magnitude_out == MAG_SQ_OUT) ? fft_mag_sq_o_tvalid      : 1'b0;
  assign fft_mag_round_i_tlast  = (magnitude_out == MAG_OUT)    ? fft_mag_o_tlast          :
                                  (magnitude_out == MAG_SQ_OUT) ? fft_mag_sq_o_tlast       : 1'b0;
  assign fft_mag_round_i_tdata  = (magnitude_out == MAG_OUT)    ? fft_mag_o_tdata          : fft_mag_sq_o_tdata;
  assign fft_mag_round_o_tready = axis_from_fft_tready;
  assign axis_from_fft_tvalid     = (magnitude_out == MAG_OUT | magnitude_out == MAG_SQ_OUT) ? fft_mag_round_o_tvalid : fft_shift_o_tvalid;
  assign axis_from_fft_tlast      = (magnitude_out == MAG_OUT | magnitude_out == MAG_SQ_OUT) ? fft_mag_round_o_tlast  : fft_shift_o_tlast;
  assign axis_from_fft_tdata      = (magnitude_out == MAG_OUT | magnitude_out == MAG_SQ_OUT) ? fft_mag_round_o_tdata  : fft_shift_o_tdata;

  // Conditionally synth magnitude / magnitude^2 logic
  generate
    if (EN_MAGNITUDE_OUT | EN_MAGNITUDE_APPROX_OUT | EN_MAGNITUDE_SQ_OUT) begin : generate_magnitude_out
      setting_reg #(
        .my_addr(SR_MAGNITUDE_OUT), .awidth(8), .width(2))
      sr_magnitude_out (
        .clk(ce_clk), .rst(ce_rst),
        .strobe(set_stb), .addr(set_addr), .in(set_data), .out(magnitude_out), .changed());
    end else begin : generate_magnitude_out_else
      // Magnitude calculation logic not included, so always bypass
      assign magnitude_out = 2'd0;
    end

    if (EN_FFT_SHIFT) begin : generate_fft_shift
      fft_shift #(
        .MAX_FFT_SIZE_LOG2(MAX_FFT_SIZE_LOG2),
        .WIDTH(32))
      inst_fft_shift (
        .clk(ce_clk), .reset(ce_rst | fft_reset),
        .config_tdata(fft_shift_config_tdata),
        .config_tvalid(fft_shift_config_tvalid),
        .config_tready(fft_shift_config_tready),
        .fft_size_log2_tdata(fft_size_log2_tdata[$clog2(MAX_FFT_SIZE_LOG2)-1:0]),
        .fft_size_log2_tvalid(fft_size_log2_tvalid),
        .fft_size_log2_tready(fft_size_log2_tready),
        .i_tdata(fft_data_o_tdata),
        .i_tlast(fft_data_o_tlast),
        .i_tvalid(fft_data_o_tvalid),
        .i_tready(fft_data_o_tready),
        .i_tuser(fft_data_o_tuser[MAX_FFT_SIZE_LOG2-1:0]),
        .o_tdata(fft_shift_o_tdata),
        .o_tlast(fft_shift_o_tlast),
        .o_tvalid(fft_shift_o_tvalid),
        .o_tready(fft_shift_o_tready));
    end 
    else begin : generate_fft_shift_else 
      assign fft_shift_o_tdata = fft_data_o_tdata;
      assign fft_shift_o_tlast = fft_data_o_tlast;
      assign fft_shift_o_tvalid = fft_data_o_tvalid;
      assign fft_data_o_tready = fft_shift_o_tready;
    end

    // More accurate magnitude calculation takes precedence if enabled
    if (EN_MAGNITUDE_OUT) begin : generate_complex_to_magphase 
      complex_to_magphase
      inst_complex_to_magphase (
        .aclk(ce_clk), .aresetn(~(ce_rst | fft_reset)),
        .s_axis_cartesian_tvalid(fft_mag_i_tvalid),
        .s_axis_cartesian_tlast(fft_mag_i_tlast),
        .s_axis_cartesian_tready(fft_mag_i_tready),
        .s_axis_cartesian_tdata(fft_mag_i_tdata),
        .m_axis_dout_tvalid(fft_mag_o_tvalid),
        .m_axis_dout_tlast(fft_mag_o_tlast),
        .m_axis_dout_tdata(fft_mag_o_tdata_int),
        .m_axis_dout_tready(fft_mag_o_tready));
      assign fft_mag_o_tdata = {1'b0, fft_mag_o_tdata_int[15:0], 15'd0};
    end 
    else if (EN_MAGNITUDE_APPROX_OUT) begin : generate_complex_to_mag_approx
      complex_to_mag_approx
      inst_complex_to_mag_approx (
        .clk(ce_clk), .reset(ce_rst | fft_reset), .clear(1'b0),
        .i_tvalid(fft_mag_i_tvalid),
        .i_tlast(fft_mag_i_tlast),
        .i_tready(fft_mag_i_tready),
        .i_tdata(fft_mag_i_tdata),
        .o_tvalid(fft_mag_o_tvalid),
        .o_tlast(fft_mag_o_tlast),
        .o_tready(fft_mag_o_tready),
        .o_tdata(fft_mag_o_tdata_int[15:0]));
      assign fft_mag_o_tdata = {1'b0, fft_mag_o_tdata_int[15:0], 15'd0};
    end  
    else begin : generate_complex_to_mag_approx_else
      assign fft_mag_o_tdata = fft_mag_i_tdata;
      assign fft_mag_o_tlast = fft_mag_i_tlast;
      assign fft_mag_o_tvalid = fft_mag_i_tvalid;
      assign fft_mag_i_tready = fft_mag_o_tready;
    end

    if (EN_MAGNITUDE_SQ_OUT) begin : generate_complex_to_magsq
      complex_to_magsq
      inst_complex_to_magsq (
        .clk(ce_clk), .reset(ce_rst | fft_reset), .clear(1'b0),
        .i_tvalid(fft_mag_sq_i_tvalid),
        .i_tlast(fft_mag_sq_i_tlast),
        .i_tready(fft_mag_sq_i_tready),
        .i_tdata(fft_mag_sq_i_tdata),
        .o_tvalid(fft_mag_sq_o_tvalid),
        .o_tlast(fft_mag_sq_o_tlast),
        .o_tready(fft_mag_sq_o_tready),
        .o_tdata(fft_mag_sq_o_tdata));
    end
    else begin : generate_complex_to_magsq_else
      assign fft_mag_sq_o_tdata = fft_mag_sq_i_tdata;
      assign fft_mag_sq_o_tlast = fft_mag_sq_i_tlast;
      assign fft_mag_sq_o_tvalid = fft_mag_sq_i_tvalid;
      assign fft_mag_sq_i_tready = fft_mag_sq_o_tready;
    end

    // Convert to SC16
    if (EN_MAGNITUDE_OUT | EN_MAGNITUDE_APPROX_OUT | EN_MAGNITUDE_SQ_OUT) begin : generate_axi_round_and_clip
      axi_round_and_clip #(
        .WIDTH_IN(32),
        .WIDTH_OUT(16),
        .CLIP_BITS(1))
      inst_axi_round_and_clip (
        .clk(ce_clk), .reset(ce_rst | fft_reset),
        .i_tdata(fft_mag_round_i_tdata),
        .i_tlast(fft_mag_round_i_tlast),
        .i_tvalid(fft_mag_round_i_tvalid),
        .i_tready(fft_mag_round_i_tready),
        .o_tdata(fft_mag_round_o_tdata[31:16]),
        .o_tlast(fft_mag_round_o_tlast),
        .o_tvalid(fft_mag_round_o_tvalid),
        .o_tready(fft_mag_round_o_tready));
      assign fft_mag_round_o_tdata[15:0] = {16{16'd0}};
    end 
    else begin : generate_axi_round_and_clip_else
      assign fft_mag_round_o_tdata = fft_mag_round_i_tdata;
      assign fft_mag_round_o_tlast = fft_mag_round_i_tlast;
      assign fft_mag_round_o_tvalid = fft_mag_round_i_tvalid;
      assign fft_mag_round_i_tready = fft_mag_round_o_tready;
    end 
  endgenerate

  // Readback registers
  always @*
    case(rb_addr)
      3'd0    : rb_data <= {63'd0, fft_reset};
      3'd1    : rb_data <= {62'd0, magnitude_out};
      3'd2    : rb_data <= {fft_size_log2_tdata};
      3'd3    : rb_data <= {63'd0, fft_direction_tdata};
      3'd4    : rb_data <= {52'd0, fft_scaling_tdata};
      3'd5    : rb_data <= {62'd0, fft_shift_config_tdata};
      default : rb_data <= 64'h0BADC0DE0BADC0DE;
  endcase

endmodule
