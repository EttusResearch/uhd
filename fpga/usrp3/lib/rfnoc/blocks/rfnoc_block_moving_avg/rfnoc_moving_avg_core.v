//
// Copyright 2020 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rfnoc_moving_avg_core
//
// Description:
//
//   This module contains the registers and core logic for a single RFNoC
//   Moving Average module instance.
//


module rfnoc_moving_avg_core (
  input wire clk,
  input wire rst,

  // CtrlPort Slave
  input  wire        s_ctrlport_req_wr,
  input  wire        s_ctrlport_req_rd,
  input  wire [19:0] s_ctrlport_req_addr,
  input  wire [31:0] s_ctrlport_req_data,
  output reg         s_ctrlport_resp_ack,
  output reg  [31:0] s_ctrlport_resp_data,

  // Input data stream
  input  wire [31:0] i_tdata,
  input  wire        i_tlast,
  input  wire        i_tvalid,
  output wire        i_tready,

  // Output data stream
  output wire [31:0] o_tdata,
  output wire        o_tlast,
  output wire        o_tvalid,
  input  wire        o_tready
);

  `include "rfnoc_block_moving_avg_regs.vh"


  //---------------------------------------------------------------------------
  // Registers
  //---------------------------------------------------------------------------

  // Number of samples to accumulate
  reg [REG_SUM_LENGTH_LEN-1:0] sum_len_reg;
  reg                          sum_len_reg_changed;

  // Sum will be divided by this number
  reg [REG_DIVISOR_LEN-1:0] divisor_reg;

  always @(posedge clk) begin
    if (rst) begin
      sum_len_reg_changed <= 1'b0;
      sum_len_reg         <=  'bX;
      divisor_reg         <=  'bX;
    end else begin
      // Default assignments
      s_ctrlport_resp_ack  <= 1'b0;
      s_ctrlport_resp_data <= 0;
      sum_len_reg_changed  <= 1'b0;

      // Handle register writes
      if (s_ctrlport_req_wr) begin
        s_ctrlport_resp_ack <= 1;
        case (s_ctrlport_req_addr)
          REG_SUM_LENGTH : begin
            sum_len_reg         <= s_ctrlport_req_data[REG_SUM_LENGTH_LEN-1:0];
            sum_len_reg_changed <= 1'b1;
          end
          REG_DIVISOR : begin
            divisor_reg <= s_ctrlport_req_data[REG_DIVISOR_LEN-1:0];
          end
        endcase
      end

      // Handle register reads
      if (s_ctrlport_req_rd) begin
        s_ctrlport_resp_ack <= 1;
        case (s_ctrlport_req_addr)
          REG_SUM_LENGTH : begin
            s_ctrlport_resp_data[REG_SUM_LENGTH_LEN-1:0] <= sum_len_reg;
          end
          REG_DIVISOR : begin
            s_ctrlport_resp_data[REG_DIVISOR_LEN-1:0] <= divisor_reg;
          end
          default : begin
            s_ctrlport_resp_data <= 32'h0BADC0DE;
          end
        endcase
      end
    end
  end


  //---------------------------------------------------------------------------
  // Moving Average Core Logic
  //---------------------------------------------------------------------------

  // I part
  wire [15:0] ipart_tdata;
  wire        ipart_tlast;
  wire        ipart_tvalid;
  wire        ipart_tready;

  // Q part
  wire [15:0] qpart_tdata;
  wire        qpart_tlast;
  wire        qpart_tvalid;
  wire        qpart_tready;

  // I sum
  wire [23:0] isum_tdata;
  wire        isum_tlast;
  wire        isum_tvalid;
  wire        isum_tready;

  // Q sum
  wire [23:0] qsum_tdata;
  wire        qsum_tlast;
  wire        qsum_tvalid;
  wire        qsum_tready;

  // I average
  wire        [47:0] iavg_uncorrected_tdata;
  wire signed [46:0] iavg_tdata;
  wire               iavg_tlast;
  wire               iavg_tvalid;
  wire               iavg_tready;
  wire        [15:0] iavg_rnd_tdata;
  wire               iavg_rnd_tlast;
  wire               iavg_rnd_tvalid;
  wire               iavg_rnd_tready;
  wire               idivisor_tready;
  wire               idividend_tready;

  // Q average
  wire        [47:0] qavg_uncorrected_tdata;
  wire signed [46:0] qavg_tdata;
  wire               qavg_tlast;
  wire               qavg_tvalid;
  wire               qavg_tready;
  wire        [15:0] qavg_rnd_tdata;
  wire               qavg_rnd_tlast;
  wire               qavg_rnd_tvalid;
  wire               qavg_rnd_tready;
  wire               qdivisor_tready;
  wire               qdividend_tready;

  // The core logic below is hard coded for 8-bit sum length and 24-bit
  // divider. So make sure the registers are configured that way. If we want
  // to support longer sums, then the code below needs to be updated.
  generate
    if (REG_SUM_LENGTH_LEN != 8) begin : sum_length_assertion
      SUM_LENGTH_must_be_8_bits();
    end
    if (REG_DIVISOR_LEN != 24) begin : divisor_length_assertion
      REG_DIVISOR_must_be_24_bits();
    end
  endgenerate

  // Split incoming data into I and Q parts
  split_complex #(
    .WIDTH (16)
  ) split_complex_inst (
    .i_tdata   (i_tdata),
    .i_tlast   (i_tlast),
    .i_tvalid  (i_tvalid),
    .i_tready  (i_tready),
    .oi_tdata  (ipart_tdata),
    .oi_tlast  (ipart_tlast),
    .oi_tvalid (ipart_tvalid),
    .oi_tready (ipart_tready),
    .oq_tdata  (qpart_tdata),
    .oq_tlast  (qpart_tlast),
    .oq_tvalid (qpart_tvalid),
    .oq_tready (qpart_tready),
    .error     ()
  );

  // Accumulate I values
  moving_sum #(
    .MAX_LEN (255),
    .WIDTH   (16)
  ) moving_isum_inst (
    .clk      (clk),
    .reset    (rst),
    .clear    (sum_len_reg_changed),
    .len      (sum_len_reg),
    .i_tdata  (ipart_tdata),
    .i_tlast  (ipart_tlast),
    .i_tvalid (ipart_tvalid),
    .i_tready (ipart_tready),
    .o_tdata  (isum_tdata),
    .o_tlast  (isum_tlast),
    .o_tvalid (isum_tvalid),
    .o_tready (isum_tready)
  );

  // Accumulate Q values
  moving_sum #(
    .MAX_LEN (255),
    .WIDTH   (16)
  ) moving_qsum_inst (
    .clk      (clk),
    .reset    (rst),
    .clear    (sum_len_reg_changed),
    .len      (sum_len_reg),
    .i_tdata  (qpart_tdata),
    .i_tlast  (qpart_tlast),
    .i_tvalid (qpart_tvalid),
    .i_tready (qpart_tready),
    .o_tdata  (qsum_tdata),
    .o_tlast  (qsum_tlast),
    .o_tvalid (qsum_tvalid),
    .o_tready (qsum_tready)
  );

  // Make sure dividers are ready. The handshake logic here makes the
  // assumption that the divider_int24 instances can always accept a divisor
  // and a dividend on the same clock cycle. That is, as long as we always
  // input them together, we'll never have a situation where the divisor input
  // is ready and the dividend input is not, or vice versa.
  assign isum_tready = idivisor_tready & idividend_tready;
  assign qsum_tready = qdivisor_tready & qdividend_tready;

  // Divide I part by divisor from register
  divide_int24 divide_i_inst (
    .aclk                   (clk),
    .aresetn                (~rst),
    .s_axis_divisor_tvalid  (isum_tvalid),
    .s_axis_divisor_tready  (idivisor_tready),
    .s_axis_divisor_tlast   (isum_tlast),
    .s_axis_divisor_tdata   (divisor_reg),
    .s_axis_dividend_tvalid (isum_tvalid),
    .s_axis_dividend_tready (idividend_tready),
    .s_axis_dividend_tlast  (isum_tlast),
    .s_axis_dividend_tdata  (isum_tdata),
    .m_axis_dout_tvalid     (iavg_tvalid),
    .m_axis_dout_tready     (iavg_tready),
    .m_axis_dout_tuser      (),
    .m_axis_dout_tlast      (iavg_tlast),
    .m_axis_dout_tdata      (iavg_uncorrected_tdata)
  );

  // Divide Q part by divisor from register
  divide_int24 divide_q_inst (
    .aclk                   (clk),
    .aresetn                (~rst),
    .s_axis_divisor_tvalid  (qsum_tvalid),
    .s_axis_divisor_tready  (qdivisor_tready),
    .s_axis_divisor_tlast   (qsum_tlast),
    .s_axis_divisor_tdata   (divisor_reg),
    .s_axis_dividend_tvalid (qsum_tvalid),
    .s_axis_dividend_tready (qdividend_tready),
    .s_axis_dividend_tlast  (qsum_tlast),
    .s_axis_dividend_tdata  (qsum_tdata),
    .m_axis_dout_tvalid     (qavg_tvalid),
    .m_axis_dout_tready     (qavg_tready),
    .m_axis_dout_tuser      (),
    .m_axis_dout_tlast      (qavg_tlast),
    .m_axis_dout_tdata      (qavg_uncorrected_tdata)
  );

  // Xilinx divider separates integer and fractional parts. Combine into fixed
  // point value Q23.23.
  assign iavg_tdata = $signed({iavg_uncorrected_tdata[47:24],23'd0}) + 
                      $signed(iavg_uncorrected_tdata[23:0]);
  assign qavg_tdata = $signed({qavg_uncorrected_tdata[47:24],23'd0}) + 
                      $signed(qavg_uncorrected_tdata[23:0]);

  axi_round_and_clip #(
    .WIDTH_IN  (47),
    .WIDTH_OUT (16),
    .CLIP_BITS (8)
  ) axi_round_and_clip_i (
    .clk      (clk),
    .reset    (rst),
    .i_tdata  (iavg_tdata),
    .i_tlast  (iavg_tlast),
    .i_tvalid (iavg_tvalid),
    .i_tready (iavg_tready),
    .o_tdata  (iavg_rnd_tdata),
    .o_tlast  (iavg_rnd_tlast),
    .o_tvalid (iavg_rnd_tvalid),
    .o_tready (iavg_rnd_tready)
  );

  axi_round_and_clip #(
    .WIDTH_IN  (47),
    .WIDTH_OUT (16),
    .CLIP_BITS (8)
  ) axi_round_and_clip_q (
    .clk      (clk),
    .reset    (rst),
    .i_tdata  (qavg_tdata),
    .i_tlast  (qavg_tlast),
    .i_tvalid (qavg_tvalid),
    .i_tready (qavg_tready),
    .o_tdata  (qavg_rnd_tdata),
    .o_tlast  (qavg_rnd_tlast),
    .o_tvalid (qavg_rnd_tvalid),
    .o_tready (qavg_rnd_tready)
  );

  // Concatenate I and Q part again
  join_complex #(
    .WIDTH (16)
  ) join_complex_inst (
    .ii_tdata  (iavg_rnd_tdata),
    .ii_tlast  (iavg_rnd_tlast),
    .ii_tvalid (iavg_rnd_tvalid),
    .ii_tready (iavg_rnd_tready),
    .iq_tdata  (qavg_rnd_tdata),
    .iq_tlast  (qavg_rnd_tlast),
    .iq_tvalid (qavg_rnd_tvalid),
    .iq_tready (qavg_rnd_tready),
    .o_tdata   (o_tdata),
    .o_tlast   (o_tlast),
    .o_tvalid  (o_tvalid),
    .o_tready  (o_tready),
    .error     ()
  );

endmodule
