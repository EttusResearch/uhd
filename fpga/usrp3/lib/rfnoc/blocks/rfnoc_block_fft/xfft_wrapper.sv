//
// Copyright 2024 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: xfft_wrapper
//
// Description:
//
//   Wrapper for the Xilinx FFT core, which allows you to configure the maximum
//   FFT size using parameters.
//
// Parameters:
//
//   MAX_FFT_SIZE_LOG2 : Log2 of maximum configurable FFT size. That is, the
//                       max supported FFT size will be 2**MAX_FFT_SIZE_LOG2.
//

`default_nettype none


module xfft_wrapper
  import xfft_config_pkg::*;
#(
  parameter  int MAX_FFT_SIZE_LOG2 = 12,
  localparam int FFT_CONFIG_W      = fft_config_w(MAX_FFT_SIZE_LOG2)
) (
  input  wire                    aclk,
  input  wire                    aresetn,
  input  wire [FFT_CONFIG_W-1:0] s_axis_config_tdata,
  input  wire                    s_axis_config_tvalid,
  output wire                    s_axis_config_tready,
  input  wire [            31:0] s_axis_data_tdata,
  input  wire                    s_axis_data_tvalid,
  output wire                    s_axis_data_tready,
  input  wire                    s_axis_data_tlast,
  output wire [            31:0] m_axis_data_tdata,
  output wire [            23:0] m_axis_data_tuser,
  output wire                    m_axis_data_tvalid,
  input  wire                    m_axis_data_tready,
  output wire                    m_axis_data_tlast,
  output wire [             7:0] m_axis_status_tdata,
  output wire                    m_axis_status_tvalid,
  input  wire                    m_axis_status_tready,
  output wire                    event_frame_started,
  output wire                    event_tlast_unexpected,
  output wire                    event_tlast_missing,
  output wire                    event_fft_overflow,
  output wire                    event_status_channel_halt,
  output wire                    event_data_in_channel_halt,
  output wire                    event_data_out_channel_halt
);
  localparam MAX_FFT_SIZE = 2**MAX_FFT_SIZE_LOG2;

  if (MAX_FFT_SIZE == 1024) begin : gen_1k_fft
    xfft_1k_16b xfft_1k_16b_i (
      .aclk                        (aclk),
      .aresetn                     (aresetn),
      .s_axis_config_tdata         (s_axis_config_tdata),
      .s_axis_config_tvalid        (s_axis_config_tvalid),
      .s_axis_config_tready        (s_axis_config_tready),
      .s_axis_data_tdata           (s_axis_data_tdata),
      .s_axis_data_tlast           (s_axis_data_tlast),
      .s_axis_data_tvalid          (s_axis_data_tvalid),
      .s_axis_data_tready          (s_axis_data_tready),
      .m_axis_data_tdata           (m_axis_data_tdata),
      .m_axis_data_tuser           (m_axis_data_tuser),
      .m_axis_data_tlast           (m_axis_data_tlast),
      .m_axis_data_tvalid          (m_axis_data_tvalid),
      .m_axis_data_tready          (m_axis_data_tready),
      .m_axis_status_tdata         (m_axis_status_tdata),
      .m_axis_status_tvalid        (m_axis_status_tvalid),
      .m_axis_status_tready        (m_axis_status_tready),
      .event_frame_started         (event_frame_started),
      .event_tlast_unexpected      (event_tlast_unexpected),
      .event_tlast_missing         (event_tlast_missing),
      .event_fft_overflow          (event_fft_overflow),
      .event_status_channel_halt   (event_status_channel_halt),
      .event_data_in_channel_halt  (event_data_in_channel_halt),
      .event_data_out_channel_halt (event_data_out_channel_halt)
    );
  end else if (MAX_FFT_SIZE == 2048) begin : gen_2k_fft
    xfft_2k_16b xfft_2k_16b_i (
      .aclk                        (aclk),
      .aresetn                     (aresetn),
      .s_axis_config_tdata         (s_axis_config_tdata),
      .s_axis_config_tvalid        (s_axis_config_tvalid),
      .s_axis_config_tready        (s_axis_config_tready),
      .s_axis_data_tdata           (s_axis_data_tdata),
      .s_axis_data_tlast           (s_axis_data_tlast),
      .s_axis_data_tvalid          (s_axis_data_tvalid),
      .s_axis_data_tready          (s_axis_data_tready),
      .m_axis_data_tdata           (m_axis_data_tdata),
      .m_axis_data_tuser           (m_axis_data_tuser),
      .m_axis_data_tlast           (m_axis_data_tlast),
      .m_axis_data_tvalid          (m_axis_data_tvalid),
      .m_axis_data_tready          (m_axis_data_tready),
      .m_axis_status_tdata         (m_axis_status_tdata),
      .m_axis_status_tvalid        (m_axis_status_tvalid),
      .m_axis_status_tready        (m_axis_status_tready),
      .event_frame_started         (event_frame_started),
      .event_tlast_unexpected      (event_tlast_unexpected),
      .event_tlast_missing         (event_tlast_missing),
      .event_fft_overflow          (event_fft_overflow),
      .event_status_channel_halt   (event_status_channel_halt),
      .event_data_in_channel_halt  (event_data_in_channel_halt),
      .event_data_out_channel_halt (event_data_out_channel_halt)
    );
  end else if (MAX_FFT_SIZE == 4096) begin : gen_4k_fft
    xfft_4k_16b xfft_4k_16b_i (
      .aclk                        (aclk),
      .aresetn                     (aresetn),
      .s_axis_config_tdata         (s_axis_config_tdata),
      .s_axis_config_tvalid        (s_axis_config_tvalid),
      .s_axis_config_tready        (s_axis_config_tready),
      .s_axis_data_tdata           (s_axis_data_tdata),
      .s_axis_data_tlast           (s_axis_data_tlast),
      .s_axis_data_tvalid          (s_axis_data_tvalid),
      .s_axis_data_tready          (s_axis_data_tready),
      .m_axis_data_tdata           (m_axis_data_tdata),
      .m_axis_data_tuser           (m_axis_data_tuser),
      .m_axis_data_tlast           (m_axis_data_tlast),
      .m_axis_data_tvalid          (m_axis_data_tvalid),
      .m_axis_data_tready          (m_axis_data_tready),
      .m_axis_status_tdata         (m_axis_status_tdata),
      .m_axis_status_tvalid        (m_axis_status_tvalid),
      .m_axis_status_tready        (m_axis_status_tready),
      .event_frame_started         (event_frame_started),
      .event_tlast_unexpected      (event_tlast_unexpected),
      .event_tlast_missing         (event_tlast_missing),
      .event_fft_overflow          (event_fft_overflow),
      .event_status_channel_halt   (event_status_channel_halt),
      .event_data_in_channel_halt  (event_data_in_channel_halt),
      .event_data_out_channel_halt (event_data_out_channel_halt)
    );
  end else if (MAX_FFT_SIZE == 8192) begin : gen_8k_fft
    xfft_8k_16b xfft_8k_16b_i (
      .aclk                        (aclk),
      .aresetn                     (aresetn),
      .s_axis_config_tdata         (s_axis_config_tdata),
      .s_axis_config_tvalid        (s_axis_config_tvalid),
      .s_axis_config_tready        (s_axis_config_tready),
      .s_axis_data_tdata           (s_axis_data_tdata),
      .s_axis_data_tlast           (s_axis_data_tlast),
      .s_axis_data_tvalid          (s_axis_data_tvalid),
      .s_axis_data_tready          (s_axis_data_tready),
      .m_axis_data_tdata           (m_axis_data_tdata),
      .m_axis_data_tuser           (m_axis_data_tuser),
      .m_axis_data_tlast           (m_axis_data_tlast),
      .m_axis_data_tvalid          (m_axis_data_tvalid),
      .m_axis_data_tready          (m_axis_data_tready),
      .m_axis_status_tdata         (m_axis_status_tdata),
      .m_axis_status_tvalid        (m_axis_status_tvalid),
      .m_axis_status_tready        (m_axis_status_tready),
      .event_frame_started         (event_frame_started),
      .event_tlast_unexpected      (event_tlast_unexpected),
      .event_tlast_missing         (event_tlast_missing),
      .event_fft_overflow          (event_fft_overflow),
      .event_status_channel_halt   (event_status_channel_halt),
      .event_data_in_channel_halt  (event_data_in_channel_halt),
      .event_data_out_channel_halt (event_data_out_channel_halt)
    );
  end else if (MAX_FFT_SIZE == 16384) begin : gen_16k_fft
    xfft_16k_16b xfft_16k_16b_i (
      .aclk                        (aclk),
      .aresetn                     (aresetn),
      .s_axis_config_tdata         (s_axis_config_tdata),
      .s_axis_config_tvalid        (s_axis_config_tvalid),
      .s_axis_config_tready        (s_axis_config_tready),
      .s_axis_data_tdata           (s_axis_data_tdata),
      .s_axis_data_tlast           (s_axis_data_tlast),
      .s_axis_data_tvalid          (s_axis_data_tvalid),
      .s_axis_data_tready          (s_axis_data_tready),
      .m_axis_data_tdata           (m_axis_data_tdata),
      .m_axis_data_tuser           (m_axis_data_tuser),
      .m_axis_data_tlast           (m_axis_data_tlast),
      .m_axis_data_tvalid          (m_axis_data_tvalid),
      .m_axis_data_tready          (m_axis_data_tready),
      .m_axis_status_tdata         (m_axis_status_tdata),
      .m_axis_status_tvalid        (m_axis_status_tvalid),
      .m_axis_status_tready        (m_axis_status_tready),
      .event_frame_started         (event_frame_started),
      .event_tlast_unexpected      (event_tlast_unexpected),
      .event_tlast_missing         (event_tlast_missing),
      .event_fft_overflow          (event_fft_overflow),
      .event_status_channel_halt   (event_status_channel_halt),
      .event_data_in_channel_halt  (event_data_in_channel_halt),
      .event_data_out_channel_halt (event_data_out_channel_halt)
    );
  end else if (MAX_FFT_SIZE == 32768) begin : gen_32k_fft
    xfft_32k_16b xfft_32k_16b_i (
      .aclk                        (aclk),
      .aresetn                     (aresetn),
      .s_axis_config_tdata         (s_axis_config_tdata),
      .s_axis_config_tvalid        (s_axis_config_tvalid),
      .s_axis_config_tready        (s_axis_config_tready),
      .s_axis_data_tdata           (s_axis_data_tdata),
      .s_axis_data_tlast           (s_axis_data_tlast),
      .s_axis_data_tvalid          (s_axis_data_tvalid),
      .s_axis_data_tready          (s_axis_data_tready),
      .m_axis_data_tdata           (m_axis_data_tdata),
      .m_axis_data_tuser           (m_axis_data_tuser),
      .m_axis_data_tlast           (m_axis_data_tlast),
      .m_axis_data_tvalid          (m_axis_data_tvalid),
      .m_axis_data_tready          (m_axis_data_tready),
      .m_axis_status_tdata         (m_axis_status_tdata),
      .m_axis_status_tvalid        (m_axis_status_tvalid),
      .m_axis_status_tready        (m_axis_status_tready),
      .event_frame_started         (event_frame_started),
      .event_tlast_unexpected      (event_tlast_unexpected),
      .event_tlast_missing         (event_tlast_missing),
      .event_fft_overflow          (event_fft_overflow),
      .event_status_channel_halt   (event_status_channel_halt),
      .event_data_in_channel_halt  (event_data_in_channel_halt),
      .event_data_out_channel_halt (event_data_out_channel_halt)
    );
  end else if (MAX_FFT_SIZE == 65536) begin : gen_64k_fft
    xfft_64k_16b xfft_64k_16b_i (
      .aclk                        (aclk),
      .aresetn                     (aresetn),
      .s_axis_config_tdata         (s_axis_config_tdata),
      .s_axis_config_tvalid        (s_axis_config_tvalid),
      .s_axis_config_tready        (s_axis_config_tready),
      .s_axis_data_tdata           (s_axis_data_tdata),
      .s_axis_data_tlast           (s_axis_data_tlast),
      .s_axis_data_tvalid          (s_axis_data_tvalid),
      .s_axis_data_tready          (s_axis_data_tready),
      .m_axis_data_tdata           (m_axis_data_tdata),
      .m_axis_data_tuser           (m_axis_data_tuser),
      .m_axis_data_tlast           (m_axis_data_tlast),
      .m_axis_data_tvalid          (m_axis_data_tvalid),
      .m_axis_data_tready          (m_axis_data_tready),
      .m_axis_status_tdata         (m_axis_status_tdata),
      .m_axis_status_tvalid        (m_axis_status_tvalid),
      .m_axis_status_tready        (m_axis_status_tready),
      .event_frame_started         (event_frame_started),
      .event_tlast_unexpected      (event_tlast_unexpected),
      .event_tlast_missing         (event_tlast_missing),
      .event_fft_overflow          (event_fft_overflow),
      .event_status_channel_halt   (event_status_channel_halt),
      .event_data_in_channel_halt  (event_data_in_channel_halt),
      .event_data_out_channel_halt (event_data_out_channel_halt)
    );
  end else begin
    ERROR_Invalid_FFT_parameters();
  end

endmodule : xfft_wrapper


`default_nettype wire
