//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: rf_down_4to2
//
// Description:
//
//   Implements a down-sampling filter that accepts 4 samples per cycle on the
//   input and outputs 2 samples per cycle. A 2x speed clock is used to perform
//   the DSP computation, so that less logic can be used to implement the
//   half-band filter.
//
//     Data Path  :  In --> Gearbox --> Filter --> Gearbox --> Out
//     SPC        :     4           2          1           2
//     Clock Rate :     1x          2x         2x          1x
//

`default_nettype none

module rf_down_4to2 #(
  parameter NUM_CHANNELS = 1
) (
  input  wire clk,
  input  wire clk_2x,

  // Synchronous resets
  input  wire rst,
  input  wire rst_2x,

  // Input - 4 SPC, synchronous to "clk"
  input  wire [NUM_CHANNELS*128-1:0] i_tdata,
  input  wire [NUM_CHANNELS*  1-1:0] i_tvalid,

  // Output - 2 SPC, synchronous to "clk"
  output wire [NUM_CHANNELS*64-1:0] o_tdata,
  output wire [NUM_CHANNELS* 1-1:0] o_tvalid
);

  generate
    genvar ch;
    for (ch = 0; ch < NUM_CHANNELS; ch = ch + 1) begin : gen_channel

      //-----------------------------------------------------------------------
      // Input Gearbox
      //-----------------------------------------------------------------------
      //
      // Convert from 4 SPC on clk to 2 SPC on clk_2x.
      //
      //-----------------------------------------------------------------------

      wire [63:0] gear_to_filt_tdata;
      wire        gear_to_filt_tvalid;

      gearbox_2x1 #(
        .WORD_W     (32),
        .IN_WORDS   (4),
        .OUT_WORDS  (2),
        .BIG_ENDIAN (0)
      ) gearbox_2x1_in (
        .i_clk    (clk),
        .i_rst    (rst),
        .i_tdata  (i_tdata[ch*128 +: 128]),
        .i_tvalid (i_tvalid[ch]),
        .o_clk    (clk_2x),
        .o_rst    (rst_2x),
        .o_tdata  (gear_to_filt_tdata),
        .o_tvalid (gear_to_filt_tvalid)
      );


      //-----------------------------------------------------------------------
      // Interpolating Filter
      //-----------------------------------------------------------------------

      wire [47:0] filt_to_clip_tdata;
      wire        filt_to_clip_tvalid;

      hb47_2to1 hb47_2to1_i (
        .aresetn            (~rst_2x),
        .aclk               (clk_2x),
        .s_axis_data_tvalid (gear_to_filt_tvalid),
        .s_axis_data_tready (),
        .s_axis_data_tdata  (gear_to_filt_tdata),
        .m_axis_data_tvalid (filt_to_clip_tvalid),
        .m_axis_data_tuser  (),
        .m_axis_data_tdata  (filt_to_clip_tdata)
      );


      //-----------------------------------------------------------------------
      // Saturation
      //-----------------------------------------------------------------------

      wire [31:0] clip_to_gear_tdata;
      wire        clip_to_gear_tvalid;

      genvar word;
      for (word = 0; word < 2; word = word+1) begin : gen_sat
        axi_clip #(
          .WIDTH_IN  (24),
          .WIDTH_OUT (16),
          .FIFOSIZE  (0)
        ) axi_clip_i (
          .clk      (clk_2x),
          .reset    (rst_2x),
          .i_tdata  (filt_to_clip_tdata[word*24 +: 24]),
          .i_tlast  (1'b0),
          .i_tvalid (filt_to_clip_tvalid),
          .i_tready (),
          .o_tdata  (clip_to_gear_tdata[word*16 +: 16]),
          .o_tlast  (),
          .o_tvalid (clip_to_gear_tvalid),
          .o_tready (1'b1)
        );
      end


      //-----------------------------------------------------------------------
      // Output Gearbox
      //-----------------------------------------------------------------------
      //
      // Convert from 1 SPC on clk_2x to 2 SPC on clk.
      //
      //-----------------------------------------------------------------------

      gearbox_2x1 #(
        .WORD_W     (32),
        .IN_WORDS   (1),
        .OUT_WORDS  (2),
        .BIG_ENDIAN (0)
      ) gearbox_2x1_out (
        .i_clk    (clk_2x),
        .i_rst    (rst_2x),
        .i_tdata  (clip_to_gear_tdata),
        .i_tvalid (clip_to_gear_tvalid),
        .o_clk    (clk),
        .o_rst    (rst),
        .o_tdata  (o_tdata[ch*64 +: 64]),
        .o_tvalid (o_tvalid[ch])
      );

    end // for
  endgenerate

endmodule

`default_nettype wire
