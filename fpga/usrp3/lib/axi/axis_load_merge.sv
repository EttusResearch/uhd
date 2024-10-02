//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_load_merge
//
// Description:
//
//   Takes in multiple AXI4-Stream interfaces and merges the contents into a
//   single stream, packet by packet. The packets will be merged sequentially,
//   first accepting a packet from port 0, then from port 1, and so on, in a
//   circular manner. The module will wait for the packet to arrive on the
//   expected port before moving onto the next port.
//
//   This module matches the behavior of axis_load_split, such that the results
//   from multiple processing instances can be merged back together in the same
//   order they were received by the axis_load_split module.
//
//   The handling of TUSER deserves some explanation. If the input-port width
//   is the same as the output-port width, then the TUSER input is passed
//   through along with the corresponding TDATA like normal. But, if the
//   output-word size is larger than the input-word size, then there are
//   multiple N TUSER inputs for each TUSER output. The USER_SEL parameter
//   selects which one will be output. It can be in the range [0,N-1], where 0
//   selects the first of the N TUSER inputs for the next TUSER output and N-1
//   selects the last of the N TUSER inputs.
//
// Parameters:
//
//   IN_DATA_W     : Width of TDATA in bits for the input port.
//   IN_FIFO_SIZE  : Log base-2 of the input FIFO size, in units of
//                   IN_DATA_W-sized words. Typically, this would be large
//                   enough to buffer one entire packet. Set to -1 to remove.
//   IN_NUM_PORTS  : The number of input streams across which to distribute
//                   the packets.
//   OUT_DATA_W    : Width of TDATA in bits for the output port.
//   OUT_FIFO_SIZE : Log base-2 of the output FIFO size, in units of
//                   OUT_DATA_W-sized words. Set to -1 to remove, 1 to add a
//                   register that cuts timing paths, or whichever size you
//                   desire.
//   USER_W        : Width of TUSER in bits for both input and output ports.
//   USER_SEL      : When the output word size is a multiple of the input word
//                   size, USER_SEL indicates which TUSER input value will be
//                   passed along with each TDATA output value. A value of 0
//                   indicates that the first TUSER value that was input for a
//                   corresponding output word will be used for that output
//                   word. It can be in the range from 0 up to the number of
//                   output words per input words.
//

`default_nettype none


module axis_load_merge #(
  int IN_DATA_W     = 32,
  int IN_FIFO_SIZE  = 10,
  int IN_NUM_PORTS  = 2,
  int OUT_DATA_W    = 64,
  int OUT_FIFO_SIZE = 1,
  int USER_W        = 1,
  int USER_SEL      = 0
  ) (
    input  wire clk,
    input  wire rst,

    // Input streams
    input  wire [IN_DATA_W-1:0] i_tdata  [IN_NUM_PORTS],
    input  wire [   USER_W-1:0] i_tuser  [IN_NUM_PORTS],
    input  wire                 i_tlast  [IN_NUM_PORTS],
    input  wire                 i_tvalid [IN_NUM_PORTS],
    output logic                i_tready [IN_NUM_PORTS],

    // Single output stream
    output logic [OUT_DATA_W-1:0] o_tdata,
    output logic [    USER_W-1:0] o_tuser,
    output logic                  o_tlast,
    output logic                  o_tvalid,
    input  wire                   o_tready
  );

    // Elaboration-time assertions
    if (IN_DATA_W > OUT_DATA_W) begin : check_size
      $error("IN_DATA_W must not exceed OUT_DATA_W");
    end
    if (OUT_DATA_W % IN_DATA_W != 0) begin : check_multiple
      $error("OUT_DATA_W must be a multiple of IN_DATA_W");
    end


    //-------------------------------------------------------------------------
    // Resize
    //-------------------------------------------------------------------------

    logic [OUT_DATA_W-1:0] resize_tdata  [IN_NUM_PORTS];
    logic [    USER_W-1:0] resize_tuser  [IN_NUM_PORTS];
    logic                  resize_tlast  [IN_NUM_PORTS];
    logic                  resize_tvalid [IN_NUM_PORTS];
    logic                  resize_tready [IN_NUM_PORTS];

    if (IN_DATA_W == OUT_DATA_W) begin : gen_no_resize
      assign resize_tdata  = i_tdata;
      assign resize_tuser  = i_tuser;
      assign resize_tlast  = i_tlast;
      assign resize_tvalid = i_tvalid;
      assign i_tready      = resize_tready;
    end else begin : gen_resize
      for (genvar idx = 0; idx < IN_NUM_PORTS; idx++) begin : gen_axis_width_conv
        localparam int OUT_WORDS = OUT_DATA_W/IN_DATA_W;
        localparam int COUNT_W   = $clog2(OUT_WORDS);

        if (USER_SEL < 0 || USER_SEL >= OUT_WORDS) begin : check_parameters
          $error("USER_SEL is outside of allowed range");
        end

        //-----------------------------------------
        // Data Width Conversion
        //-----------------------------------------

        axis_width_conv #(
          .WORD_W   (IN_DATA_W),
          .IN_WORDS (1        ),
          .OUT_WORDS(OUT_WORDS),
          .SYNC_CLKS(1        ),
          .PIPELINE ("INOUT"  )
        ) axis_width_conv_i (
          .s_axis_aclk  (clk               ),
          .s_axis_rst   (rst               ),
          .s_axis_tdata (i_tdata[idx]      ),
          .s_axis_tkeep ('1                ),
          .s_axis_tlast (i_tlast[idx]      ),
          .s_axis_tvalid(i_tvalid[idx]     ),
          .s_axis_tready(i_tready[idx]     ),
          .m_axis_aclk  (clk               ),
          .m_axis_rst   (rst               ),
          .m_axis_tdata (resize_tdata[idx] ),
          .m_axis_tkeep (                  ),
          .m_axis_tlast (resize_tlast[idx] ),
          .m_axis_tvalid(resize_tvalid[idx]),
          .m_axis_tready(resize_tready[idx])
        );

        //-----------------------------------------
        // TUSER Handling
        //-----------------------------------------

        // We write to the TUSER FIFO at a rate of 1/OUT_WORDS, but read from
        // the TUSER FIFO in lock-step with the output of axis_width_conv
        // module. The USER_SEL parameter selects which TUSER gets input.

        logic               user_fifo_i_tvalid;
        logic               user_fifo_i_tready;
        logic               user_fifo_o_tvalid;
        logic [COUNT_W-1:0] word_count = '0;

        // This counter tells when we're inputting the selected TUSER for the
        // current output word, so we know when to write it to the FIFO. The
        // counter is initialized such that it will assert after the first
        // USER_SEL cycles, then repeats every OUT_WORDS cycles after that.
        always_ff @(posedge clk) begin
          if (i_tvalid[idx] && i_tready[idx]) begin
            if (word_count == OUT_WORDS-1) begin
              word_count <= '0;
            end else begin
              word_count <= word_count + 1;
            end
          end

          if (rst) begin
            word_count <= '0;
          end
        end

        assign user_fifo_i_tvalid = (word_count == USER_SEL) && i_tvalid[idx] && i_tready[idx];

        axi_fifo #(
          .WIDTH(USER_W),
          .SIZE (2     )
        ) axi_fifo_tuser (
          .clk     (clk                                     ),
          .reset   (rst                                     ),
          .clear   ('0                                      ),
          .i_tdata (i_tuser[idx]                            ),
          .i_tvalid(user_fifo_i_tvalid                      ),
          .i_tready(user_fifo_i_tready                      ),
          .o_tdata (resize_tuser[idx]                       ),
          .o_tvalid(user_fifo_o_tvalid                      ),
          .o_tready(resize_tvalid[idx] && resize_tready[idx]),
          .occupied(                                        ),
          .space   (                                        )
        );

        // Make sure we don't overflow/underflow the TUSER FIFO
        //synthesis translate_off
        always_ff @(posedge clk) begin
          if (user_fifo_i_tvalid && !user_fifo_i_tready) begin
            $error("TUSER FIFO overflow");
          end
          if (resize_tvalid[idx] && resize_tready[idx] && !user_fifo_o_tvalid) begin
            $error("TUSER FIFO underflow");
          end
        end
        //synthesis translate_on

      end
    end


    //---------------------------------------------------------------------------
    // Input FIFOs
    //---------------------------------------------------------------------------

    logic [OUT_DATA_W-1:0] fifo_tdata  [IN_NUM_PORTS];
    logic [    USER_W-1:0] fifo_tuser  [IN_NUM_PORTS];
    logic                  fifo_tlast  [IN_NUM_PORTS];
    logic                  fifo_tvalid [IN_NUM_PORTS];
    logic                  fifo_tready [IN_NUM_PORTS];

    for (genvar idx = 0; idx < IN_NUM_PORTS; idx++) begin : gen_axi_fifos
      axi_fifo #(
        .WIDTH(1 + USER_W + OUT_DATA_W                    ),
        .SIZE (IN_FIFO_SIZE - $clog2(OUT_DATA_W/IN_DATA_W))
      ) axi_fifo_in (
        .clk     (clk                                                      ),
        .reset   (rst                                                      ),
        .clear   (1'b0                                                     ),
        .i_tdata ({resize_tlast[idx], resize_tuser[idx], resize_tdata[idx]}),
        .i_tvalid(resize_tvalid[idx]                                       ),
        .i_tready(resize_tready[idx]                                       ),
        .o_tdata ({fifo_tlast[idx], fifo_tuser[idx], fifo_tdata[idx]}      ),
        .o_tvalid(fifo_tvalid[idx]                                         ),
        .o_tready(fifo_tready[idx]                                         ),
        .space   (                                                         ),
        .occupied(                                                         )
      );
    end


    //---------------------------------------------------------------------------
    // Merge Logic
    //---------------------------------------------------------------------------

    logic [OUT_DATA_W-1:0] merge_tdata;
    logic [    USER_W-1:0] merge_tuser;
    logic                  merge_tlast;
    logic                  merge_tvalid;
    logic                  merge_tready;

    // Currently selected port
    logic [$clog2(IN_NUM_PORTS)-1:0] st_port;

    // Merge state machine. Tracks and advances the currently selected port.
    always_ff @(posedge clk) begin
      if (rst) begin
        st_port <= '0;
      end else begin
        if (merge_tvalid && merge_tready && merge_tlast) begin
          if (st_port == IN_NUM_PORTS-1) begin
            st_port <= '0;
          end else begin
            st_port <= st_port + 1;
          end
        end
      end
    end

    // Connect the selected input to the output
    assign merge_tdata  = fifo_tdata[st_port];
    assign merge_tuser  = fifo_tuser[st_port];
    assign merge_tlast  = fifo_tlast[st_port];
    assign merge_tvalid = fifo_tvalid[st_port];

    for (genvar idx = 0; idx < IN_NUM_PORTS; idx++) begin : gen_merge
      assign fifo_tready[idx] = merge_tready && (st_port == idx);
    end


    //---------------------------------------------------------------------------
    // Output FIFOs
    //---------------------------------------------------------------------------

    axi_fifo #(
      .WIDTH(1 + USER_W + OUT_DATA_W),
      .SIZE (OUT_FIFO_SIZE          )
    ) axi_fifo_out (
      .clk     (clk                                    ),
      .reset   (rst                                    ),
      .clear   (1'b0                                   ),
      .i_tdata ({merge_tlast, merge_tuser, merge_tdata}),
      .i_tvalid(merge_tvalid                           ),
      .i_tready(merge_tready                           ),
      .o_tdata ({o_tlast, o_tuser, o_tdata}            ),
      .o_tvalid(o_tvalid                               ),
      .o_tready(o_tready                               ),
      .space   (                                       ),
      .occupied(                                       )
    );

  endmodule : axis_load_merge


  `default_nettype wire
