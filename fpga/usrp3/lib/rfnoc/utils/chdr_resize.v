//
// Copyright 2021 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_resize
//
// Description:
//
//   Takes a CHDR packet data stream and converts it from one CHDR width to a
//   different CHDR width. It can also do CHDR width conversion without
//   changing the bus width, if the bus width is the same size as the smaller
//   CHDR width.
//
//   For example, to convert from a 64-bit CHDR_W to a 256-bit CHDR_W, you
//   would set I_CHDR_W to 64 and O_CHDR_W to 256 (by default, I_DATA_W will be
//   set to 64 and O_DATA_W will be set to 256).
//
//   But you could also convert from 64-bit CHDR to 256-bit CHDR while keeping
//   the bus width at 64 bits. In this case you would set I_CHDR_W to 64 and
//   O_CHDR_W to 256, but set both I_DATA_W and O_DATA_W to 64.
//
//   There are some restrictions, including the requirement that I_CHDR_W ==
//   I_DATA_W or O_CHDR_W == O_DATA_W, and that MIN(I_DATA_W, O_DATA_W) ==
//   MIN(I_CHDR_W, O_CHDR_W). Basically, it can't do CHDR width conversion
//   where the smaller CHDR width is smaller than the bus width(s). For
//   example, you could not do conversion from 64-bit to 256-bit CHDR with
//   input and output bus widths of 256.
//
//   TUSER is supported, but is not resized. TUSER is sampled along with the
//   first word of the input packet and is assumed to be the same for the
//   duration of the packet.
//
//   Also, note that packets with different CHDR_W have a different maximum
//   number of metadata bytes. This module repacks the metadata in
//   little-endian order in the new word size. If there is too much metadata
//   for a smaller CHDR_W packet, the extra data will be discarded.
//
// Parameters:
//
//   I_CHDR_W : CHDR_W for the input data stream on i_chdr.
//   O_CHDR_W : CHDR_W for the output data stream on o_chdr.
//   I_DATA_W : Bus width for i_chdr_tdata.
//   O_DATA_W : Bus width for o_chdr_tdata.
//   USER_W   : Width for i_chdr_tuser and o_chdr_tuser.
//   PIPELINE : Indicates whether to add pipeline stages to the input and/or
//              output. This can be: "NONE", "IN", "OUT", or "INOUT".
//

`default_nettype none


module chdr_resize #(
  parameter I_CHDR_W = 64,
  parameter O_CHDR_W = 512,
  parameter I_DATA_W = I_CHDR_W,
  parameter O_DATA_W = O_CHDR_W,
  parameter USER_W   = 1,
  parameter PIPELINE = "NONE"
) (
  input wire clk,
  input wire rst,

  // Input
  input  wire [I_DATA_W-1:0] i_chdr_tdata,
  input  wire [  USER_W-1:0] i_chdr_tuser,
  input  wire                i_chdr_tlast,
  input  wire                i_chdr_tvalid,
  output wire                i_chdr_tready,

  // Input
  output wire [O_DATA_W-1:0] o_chdr_tdata,
  output wire [  USER_W-1:0] o_chdr_tuser,
  output wire                o_chdr_tlast,
  output wire                o_chdr_tvalid,
  input  wire                o_chdr_tready
);

  `define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

  // Determine the bus width of the CHDR converter, which is always the smaller
  // bus width of the input and output.
  localparam CONVERT_W = `MIN(I_DATA_W, O_DATA_W);
  // Determine if we need the bus down-sizer
  localparam DO_DOWNSIZE = (I_DATA_W > O_DATA_W);
  // Determine if we need the CHDR width converter
  localparam DO_CONVERT = (I_CHDR_W != O_CHDR_W);
  // Determine if we need the bus up-sizer
  localparam DO_UPSIZE = (I_DATA_W < O_DATA_W);

  // Determine the pipeline settings. We want pipeline stages on the input
  // and/or output, depending on the PIPELINE parameter, as well as between the
  // up-sizer and converter, and between the converter and down-sizer, any of
  // which may or may not be present. We don't, however, want back-to-back
  // pipeline stages (e.g., on the output of the down-sizer and the input to
  // the converter). If both an up/down-sizer and converter are used, the
  // settings below will turn on the adjacent pipeline stage in the converter
  // and turn off the corresponding pipeline stage in the up/down-sizer.
  localparam DOWNSIZE_PIPELINE =
    (PIPELINE == "IN"    &&  DO_CONVERT) ? "IN"    :
    (PIPELINE == "IN"    && !DO_CONVERT) ? "IN"    :
    (PIPELINE == "INOUT" &&  DO_CONVERT) ? "IN"    :
    (PIPELINE == "INOUT" && !DO_CONVERT) ? "INOUT" :
    (PIPELINE == "OUT"   &&  DO_CONVERT) ? "NONE"  :
    (PIPELINE == "OUT"   && !DO_CONVERT) ? "OUT"   :
                                           "NONE"  ;
  localparam CONVERT_PIPELINE =
    (PIPELINE == "IN"    &&  DO_DOWNSIZE) ? "IN"    :
    (PIPELINE == "IN"    &&  DO_UPSIZE  ) ? "INOUT" :
    (PIPELINE == "IN"      /* neither */) ? "IN"    :
    (PIPELINE == "INOUT" &&  DO_DOWNSIZE) ? "INOUT" :
    (PIPELINE == "INOUT" &&  DO_UPSIZE  ) ? "INOUT" :
    (PIPELINE == "INOUT"   /* neither */) ? "INOUT" :
    (PIPELINE == "OUT"   &&  DO_DOWNSIZE) ? "INOUT" :
    (PIPELINE == "OUT"   &&  DO_UPSIZE  ) ? "OUT"   :
    (PIPELINE == "OUT"     /* neither */) ? "OUT"   :
                                            "NONE"  ;
  localparam UPSIZE_PIPELINE =
    (PIPELINE == "IN"    &&  DO_CONVERT) ? "NONE"  :
    (PIPELINE == "IN"    && !DO_CONVERT) ? "IN"    :
    (PIPELINE == "INOUT" &&  DO_CONVERT) ? "OUT"   :
    (PIPELINE == "INOUT" && !DO_CONVERT) ? "INOUT" :
    (PIPELINE == "OUT"   &&  DO_CONVERT) ? "OUT"   :
    (PIPELINE == "OUT"   && !DO_CONVERT) ? "OUT"   :
                                           "NONE"  ;


  generate

    //-------------------------------------------------------------------------
    // Check Parameters
    //-------------------------------------------------------------------------

    if (!(
      // All widths must be valid CHDR widths (at least 64 and powers of 2)
      (2**$clog2(I_CHDR_W) == I_CHDR_W) &&
      (2**$clog2(O_CHDR_W) == O_CHDR_W) &&
      (2**$clog2(I_DATA_W) == I_DATA_W) &&
      (2**$clog2(O_DATA_W) == O_DATA_W) &&
      (I_CHDR_W >= 64) &&
      (O_CHDR_W >= 64) &&
      (I_DATA_W >= 64) &&
      (O_DATA_W >= 64) &&
      // The converter width must match the smaller bus width. It doesn't work
      // on buses wider than the CHDR width.
      (CONVERT_W == `MIN(I_CHDR_W, O_CHDR_W))
    )) begin : gen_error
      ERROR__Invalid_CHDR_or_data_width_parameters();
    end


    //-------------------------------------------------------------------------
    // TUSER Data Path
    //-------------------------------------------------------------------------
    //
    // Sample TUSER at the beginning of the input packet and output it for the
    // duration of the output packet.
    //
    //-------------------------------------------------------------------------

    if (DO_DOWNSIZE || DO_UPSIZE || DO_CONVERT || PIPELINE == "INOUT") begin : gen_tuser_buffer
      if (!DO_DOWNSIZE && !DO_UPSIZE && DO_CONVERT && PIPELINE == "NONE") begin : gen_tuser_reg
        // In this case, there's a combinatorial path from i_chdr to o_chdr, so
        // we can't use a FIFO to buffer TUSER.

        // Track start of packet on o_chdr
        reg o_chdr_sop = 1;
        always @(posedge clk) begin
          if (rst) begin
            o_chdr_sop <= 1;
          end else if (o_chdr_tvalid && o_chdr_tready) begin
            o_chdr_sop <= o_chdr_tlast;
          end
        end

        reg [USER_W-1:0] o_tuser_reg;
        always @(posedge clk) begin
          if (rst) begin
            o_tuser_reg <= {USER_W{1'bX}};
          end else if (o_chdr_tvalid && o_chdr_tready && o_chdr_sop) begin
            o_tuser_reg <= i_chdr_tuser;
          end
        end

        // Pass through TUSER for first word in the packet, then use a holding
        // register for the rest of the packet.
        assign o_chdr_tuser = (o_chdr_sop) ? i_chdr_tuser : o_tuser_reg;

      end else begin : gen_tuser_fifo
        // In this case we use a FIFO to buffer TUSER.

        // Track start of packet on i_chdr
        reg i_chdr_sop = 1;
        always @(posedge clk) begin
          if (rst) begin
            i_chdr_sop <= 1;
          end else if (i_chdr_tvalid && i_chdr_tready) begin
            i_chdr_sop <= i_chdr_tlast;
          end
        end

        axi_fifo_short #(
          .WIDTH (USER_W)
        ) axi_fifo_short_i (
          .clk      (clk),
          .reset    (rst),
          .clear    (1'b0),
          .i_tdata  (i_chdr_tuser),
          .i_tvalid (i_chdr_tvalid && i_chdr_tready && i_chdr_sop),
          .i_tready (),
          .o_tdata  (o_chdr_tuser),
          .o_tvalid (),
          .o_tready (o_chdr_tready && o_chdr_tvalid && o_chdr_tlast),
          .space    (),
          .occupied ()
        );
      end
    end else begin : gen_tuser_pass_through
      // In this case there's no logic on the data path, so we can pass TUSER
      // through directly.
      assign o_chdr_tuser = i_chdr_tuser;
    end


    //-------------------------------------------------------------------------
    // Down-Size Input Bus Width
    //-------------------------------------------------------------------------

    wire [CONVERT_W-1:0] resized_tdata;
    wire                 resized_tlast;
    wire                 resized_tvalid;
    wire                 resized_tready;

    if (DO_DOWNSIZE) begin : gen_bus_downsize
      axis_width_conv #(
        .WORD_W    (CONVERT_W),
        .IN_WORDS  (I_DATA_W / CONVERT_W),
        .OUT_WORDS (1),
        .SYNC_CLKS (1),
        .PIPELINE  (DOWNSIZE_PIPELINE)
      ) axis_width_conv_i (
        .s_axis_aclk   (clk),
        .s_axis_rst    (rst),
        .s_axis_tdata  (i_chdr_tdata),
        .s_axis_tkeep  ({(I_DATA_W / CONVERT_W){1'b1}}),
        .s_axis_tlast  (i_chdr_tlast),
        .s_axis_tvalid (i_chdr_tvalid),
        .s_axis_tready (i_chdr_tready),
        .m_axis_aclk   (clk),
        .m_axis_rst    (rst),
        .m_axis_tdata  (resized_tdata),
        .m_axis_tkeep  (),
        .m_axis_tlast  (resized_tlast),
        .m_axis_tvalid (resized_tvalid),
        .m_axis_tready (resized_tready)
      );
    end else begin : gen_no_bus_downsize
      assign resized_tdata  = i_chdr_tdata;
      assign resized_tlast  = i_chdr_tlast;
      assign resized_tvalid = i_chdr_tvalid;
      assign i_chdr_tready  = resized_tready;
    end


    //-------------------------------------------------------------------------
    // CHDR Width Protocol Conversion
    //-------------------------------------------------------------------------

    wire [CONVERT_W-1:0] converted_tdata;
    wire                 converted_tlast;
    wire                 converted_tvalid;
    wire                 converted_tready;

    if (DO_CONVERT) begin : gen_convert
      if (I_CHDR_W > O_CHDR_W) begin : gen_chdr_convert_down
        chdr_convert_down #(
          .I_CHDR_W (I_CHDR_W),
          .DATA_W   (CONVERT_W),
          .PIPELINE (CONVERT_PIPELINE)
        ) chdr_convert_down_i (
          .clk           (clk),
          .rst           (rst),
          .i_chdr_tdata  (resized_tdata),
          .i_chdr_tlast  (resized_tlast),
          .i_chdr_tvalid (resized_tvalid),
          .i_chdr_tready (resized_tready),
          .o_chdr_tdata  (o_chdr_tdata),
          .o_chdr_tlast  (o_chdr_tlast),
          .o_chdr_tvalid (o_chdr_tvalid),
          .o_chdr_tready (o_chdr_tready)
        );
      end else if (I_CHDR_W < O_CHDR_W) begin : gen_chdr_convert_up
        chdr_convert_up #(
          .DATA_W   (CONVERT_W),
          .O_CHDR_W (O_CHDR_W),
          .PIPELINE (PIPELINE)
        ) chdr_convert_up_i (
          .clk           (clk),
          .rst           (rst),
          .i_chdr_tdata  (resized_tdata),
          .i_chdr_tlast  (resized_tlast),
          .i_chdr_tvalid (resized_tvalid),
          .i_chdr_tready (resized_tready),
          .o_chdr_tdata  (converted_tdata),
          .o_chdr_tlast  (converted_tlast),
          .o_chdr_tvalid (converted_tvalid),
          .o_chdr_tready (converted_tready)
        );
      end
    end else begin : gen_no_convert
      if (PIPELINE == "INOUT" && !DO_DOWNSIZE && !DO_UPSIZE) begin : gen_pipeline
        // In this case there's no conversion or up-size/down-size, so we're
        // just passing the data through unchanged. However, if PIPELINE is set
        // to INOUT then we should have a pipeline stage, so we add that here.
        axi_fifo_flop2 #(
          .WIDTH (1 + CONVERT_W)
        ) axi_fifo_flop2_i (
          .clk      (clk),
          .reset    (rst),
          .clear    (1'b0),
          .i_tdata  ({ resized_tlast, resized_tdata }),
          .i_tvalid (resized_tvalid),
          .i_tready (resized_tready),
          .o_tdata  ({ converted_tlast, converted_tdata }),
          .o_tvalid (converted_tvalid),
          .o_tready (converted_tready),
          .space    (),
          .occupied ()
        );
      end else begin : gen_convert_bypass
        assign converted_tdata  = resized_tdata;
        assign converted_tlast  = resized_tlast;
        assign converted_tvalid = resized_tvalid;
        assign resized_tready   = converted_tready;
      end
    end


    //-------------------------------------------------------------------------
    // Up-Size Output Bus Width
    //-------------------------------------------------------------------------

    if (DO_UPSIZE) begin : gen_bus_upsize
      axis_width_conv #(
        .WORD_W    (CONVERT_W),
        .IN_WORDS  (1),
        .OUT_WORDS (O_DATA_W / CONVERT_W),
        .SYNC_CLKS (1),
        .PIPELINE  (UPSIZE_PIPELINE)
      ) axis_width_conv_i (
        .s_axis_aclk   (clk),
        .s_axis_rst    (rst),
        .s_axis_tdata  (converted_tdata),
        .s_axis_tkeep  (1'b1),
        .s_axis_tlast  (converted_tlast),
        .s_axis_tvalid (converted_tvalid),
        .s_axis_tready (converted_tready),
        .m_axis_aclk   (clk),
        .m_axis_rst    (rst),
        .m_axis_tdata  (o_chdr_tdata),
        .m_axis_tkeep  (),
        .m_axis_tlast  (o_chdr_tlast),
        .m_axis_tvalid (o_chdr_tvalid),
        .m_axis_tready (o_chdr_tready)
      );
    end else begin : gen_no_bus_upsize
      assign o_chdr_tdata     = converted_tdata;
      assign o_chdr_tlast     = converted_tlast;
      assign o_chdr_tvalid    = converted_tvalid;
      assign converted_tready = o_chdr_tready;
    end

  endgenerate

endmodule


`default_nettype wire
