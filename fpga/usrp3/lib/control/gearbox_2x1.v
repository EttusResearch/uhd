//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: gearbox_2x1
//
// Description:
//
//   Moves data between two clock domains at a constant data rate. This module
//   requires that the clocks be related and that there is a 2:1 ratio between
//   the two clocks. Static timing analysis is assumed for the clock domain
//   crossing. The module supports going from a slow clock to a fast clock (2
//   words per 1x clock cycle to 1 word per 2x clock cycle) or from a fast
//   clock to a slow clock (1 word per 1x clock cycle to 2 words per 2x clock
//   cycles) depending on the parameters provided.
//
//   Note that there are no tready signals, so downstream logic must always be
//   ready.
//
// Parameters:
//
//   WORD_W     : Bits per word
//   IN_WORDS   : Number of input words per clock cycle
//   OUT_WORDS  : Number of output words per clock cycle
//   BIG_ENDIAN : Order in which to input/output words when multiple words per
//                clock cycle are needed. Little endian means the first word is
//                in the least-significant position. Big endian means the first
//                word is in the most-significant position.
//


module gearbox_2x1 #(
  parameter WORD_W     = 8,
  parameter IN_WORDS   = 2,
  parameter OUT_WORDS  = 1,
  parameter BIG_ENDIAN = 0
) (
  input wire                       i_clk,
  input wire                       i_rst,
  input wire [IN_WORDS*WORD_W-1:0] i_tdata,
  input wire                       i_tvalid,

  input  wire                        o_clk,
  input  wire                        o_rst,
  output reg  [OUT_WORDS*WORD_W-1:0] o_tdata,
  output reg                         o_tvalid  = 1'b0
);

  localparam IN_W  = WORD_W * IN_WORDS;
  localparam OUT_W = WORD_W * OUT_WORDS;

  generate

    // Make sure the ratios are supported
    if (IN_WORDS != 2*OUT_WORDS && OUT_WORDS != 2*IN_WORDS) begin : gen_ERROR
      IN_WORDS_and_OUT_WORDS_must_have_a_2_to_1_ratio();
    end


    //-------------------------------------------------------------------------
    // 2 words to 1 word (slow clock to fast clock)
    //-------------------------------------------------------------------------

    if (IN_WORDS > OUT_WORDS) begin : gen_slow_to_fast
      reg [IN_W-1:0] i_tdata_reg;
      reg            i_tvalid_reg;
      reg            i_toggle = 1'b0;

      always @(posedge i_clk) begin
        if (i_rst) begin
          i_tdata_reg  <=  'bX;
          i_tvalid_reg <= 1'b0;
          i_toggle     <= 1'b0;
        end else begin
          i_tdata_reg  <= i_tdata;
          i_tvalid_reg <= i_tvalid;
          if (i_tvalid) begin
            i_toggle <= ~i_toggle;
          end
        end
      end

      reg [IN_W-1:0] o_tdata_reg;
      reg            o_tvalid_reg = 1'b0;
      reg            o_toggle;
      reg            o_toggle_dly;
      reg            o_data_sel;

      always @(posedge o_clk) begin
        if (o_rst) begin
          o_tdata_reg  <=  'bX;
          o_tvalid     <= 1'b0;
          o_tvalid_reg <= 1'b0;
          o_toggle     <= 1'bX;
          o_toggle_dly <= 1'bX;
          o_data_sel   <= 1'bX;
        end else begin
          // Clock crossing
          o_tvalid_reg <= i_tvalid_reg;
          o_toggle     <= i_toggle;
          o_tdata_reg  <= i_tdata_reg;

          // Determine which output to select
          o_toggle_dly <= o_toggle;
          o_data_sel   <= BIG_ENDIAN ^ (o_toggle == o_toggle_dly);

          // Select the correct output for this clock cycle
          o_tvalid <= o_tvalid_reg;
          o_tdata  <= o_data_sel ?
                      o_tdata_reg[0 +: OUT_W] : o_tdata_reg[IN_W/2 +: OUT_W];
        end
      end


    //-------------------------------------------------------------------------
    // 1 word to 2 words (fast clock to slow clock)
    //-------------------------------------------------------------------------

    end else begin : gen_fast_to_slow
      reg [IN_W-1:0]  i_gear_reg;
      reg [OUT_W-1:0] i_gear_tdata;
      reg             i_gear_one_word   = 1'b0;
      reg             i_gear_tvalid     = 1'b0;
      reg             i_gear_tvalid_dly = 1'b0;

      always @(posedge i_clk) begin
        if (i_rst) begin
          i_gear_reg        <= 'bX;
          i_gear_tdata      <= 'bX;
          i_gear_one_word   <= 1'b0;
          i_gear_tvalid     <= 1'b0;
          i_gear_tvalid_dly <= 1'b0;
        end else begin
          // Default assignments
          i_gear_tvalid     <= 1'b0;
          i_gear_tvalid_dly <= i_gear_tvalid;

          if (i_tvalid) begin
            // Track if the gearbox has one word saved
            i_gear_one_word <= ~i_gear_one_word;
            i_gear_reg      <= i_tdata;
            if (i_gear_one_word) begin
              // This is the second word, so output the new word on i_gear_reg_t*
              i_gear_tdata  <= BIG_ENDIAN ?
                { i_gear_reg, i_tdata } : { i_tdata, i_gear_reg };
              i_gear_tvalid <= 1'b1;
            end
          end
        end
      end

      reg [OUT_W-1:0] o_gear_tdata;
      reg             o_gear_tvalid     = 1'b0;
      reg             o_gear_tvalid_dly = 1'b0;
      reg             o_tvalid_reg      = 1'b0;

      always @(posedge o_clk) begin
        if (o_rst) begin
          o_gear_tvalid     <= 1'b0;
          o_gear_tvalid_dly <= 1'b0;
          o_gear_tdata      <=  'bX;
          o_tvalid          <= 1'b0;
          o_tdata           <=  'bX;
        end else begin
          // Clock crossing
          o_gear_tvalid     <= i_gear_tvalid;
          o_gear_tvalid_dly <= i_gear_tvalid_dly;
          o_gear_tdata      <= i_gear_tdata;

          // Control tvalid
          o_tvalid <= o_gear_tvalid | o_gear_tvalid_dly;
          o_tdata  <= o_gear_tdata;
        end
      end
    end

  endgenerate

endmodule
