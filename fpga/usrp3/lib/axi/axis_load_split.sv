//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: axis_load_split
//
// Description:
//
//   Takes an AXI4-Stream interface and distributes the input packets evenly
//   among the output ports. The packets will be distributed sequentially, with
//   the first packet going to output port 0, the second to port 1, and so on,
//   in a circular manner. The data output can be optionally resized to a new
//   data width.
//
//   The purpose of this module is to take a high-throughput input that needs
//   to be processed and farm it out to multiple instances of a processing
//   module, sending one packet to each instance.
//
// Parameters:
//
//   IN_DATA_W     : Width of TDATA in bits for the input port.
//   IN_FIFO_SIZE  : Log base-2 of the input FIFO size, in units of
//                   IN_DATA_W-sized words. Set to -1 to remove, 1 to add a
//                   register that cuts timing paths, or whichever size you
//                   desire.
//   OUT_DATA_W    : Width of TDATA in bits for the output port.
//   OUT_FIFO_SIZE : Log base-2 of the output FIFO size used for each output,
//                   in units of OUT_DATA_W-sized words. Typically, this would
//                   be large enough to buffer one entire packet. Set to -1 to
//                   remove.
//   OUT_NUM_PORTS : The number of output streams across which to distribute
//                   the packets.
//   USER_W        : Width of TUSER in bits for both input and output ports.
//

`default_nettype none


module axis_load_split #(
  int IN_DATA_W     = 64,
  int IN_FIFO_SIZE  = 1,
  int OUT_DATA_W    = 32,
  int OUT_FIFO_SIZE = 10,
  int OUT_NUM_PORTS = 2,
  int USER_W        = 1
) (
  input  wire clk,
  input  wire rst,

  // Single input stream
  input  wire [IN_DATA_W-1:0] i_tdata,
  input  wire [   USER_W-1:0] i_tuser,
  input  wire                 i_tlast,
  input  wire                 i_tvalid,
  output logic                i_tready,

  // Output streams
  output logic [OUT_DATA_W-1:0] o_tdata  [OUT_NUM_PORTS],
  output wire  [    USER_W-1:0] o_tuser  [OUT_NUM_PORTS],
  output logic                  o_tlast  [OUT_NUM_PORTS],
  output logic                  o_tvalid [OUT_NUM_PORTS],
  input  wire                   o_tready [OUT_NUM_PORTS]
);

  // Elaboration-time assertions
  if (OUT_DATA_W > IN_DATA_W) begin : check_size
    $error("OUT_DATA_W must not exceed IN_DATA_W");
  end
  if (IN_DATA_W % OUT_DATA_W != 0) begin : check_multiple
    $error("IN_DATA_W must be a multiple of OUT_DATA_W");
  end


  //---------------------------------------------------------------------------
  // Input FIFO
  //---------------------------------------------------------------------------

  logic [IN_DATA_W-1:0] in_fifo_tdata;
  logic [   USER_W-1:0] in_fifo_tuser;
  logic                 in_fifo_tlast;
  logic                 in_fifo_tvalid;
  logic                 in_fifo_tready;

  axi_fifo #(
    .WIDTH(1 + USER_W + IN_DATA_W),
    .SIZE (IN_FIFO_SIZE          )
  ) axi_fifo_in (
    .clk     (clk                                          ),
    .reset   (rst                                          ),
    .clear   (1'b0                                         ),
    .i_tdata ({i_tlast, i_tuser, i_tdata}                  ),
    .i_tvalid(i_tvalid                                     ),
    .i_tready(i_tready                                     ),
    .o_tdata ({in_fifo_tlast, in_fifo_tuser, in_fifo_tdata}),
    .o_tvalid(in_fifo_tvalid                               ),
    .o_tready(in_fifo_tready                               ),
    .space   (                                             ),
    .occupied(                                             )
  );


  //---------------------------------------------------------------------------
  // Splitter Logic
  //---------------------------------------------------------------------------

  logic [IN_DATA_W-1:0] split_tdata  [OUT_NUM_PORTS];
  logic [   USER_W-1:0] split_tuser  [OUT_NUM_PORTS];
  logic                 split_tlast  [OUT_NUM_PORTS];
  logic                 split_tvalid [OUT_NUM_PORTS];
  logic                 split_tready [OUT_NUM_PORTS];

  // Currently selected port
  logic [$clog2(OUT_NUM_PORTS)-1:0] st_port;

  // Splitter state machine. Tracks and advances the currently selected port.
  always_ff @(posedge clk) begin
    if (rst) begin
      st_port <= '0;
    end else begin
      if (in_fifo_tvalid && in_fifo_tready && in_fifo_tlast) begin
        if (st_port == OUT_NUM_PORTS-1) begin
          st_port <= '0;
        end else begin
          st_port <= st_port + 1;
        end
      end
    end
  end

  // Distribute the input to each output. Only the currently selected output
  // port will receive the data.
  assign in_fifo_tready = split_tready[st_port];
  for (genvar idx = 0; idx < OUT_NUM_PORTS; idx++) begin : gen_splitter
    assign split_tdata [idx] = in_fifo_tdata;
    assign split_tuser [idx] = in_fifo_tuser;
    assign split_tlast [idx] = in_fifo_tlast;
    assign split_tvalid[idx] = in_fifo_tvalid && (st_port == idx);
  end


  //---------------------------------------------------------------------------
  // Output FIFOs
  //---------------------------------------------------------------------------

  logic [IN_DATA_W-1:0] out_fifo_tdata  [OUT_NUM_PORTS];
  logic [   USER_W-1:0] out_fifo_tuser  [OUT_NUM_PORTS];
  logic                 out_fifo_tlast  [OUT_NUM_PORTS];
  logic                 out_fifo_tvalid [OUT_NUM_PORTS];
  logic                 out_fifo_tready [OUT_NUM_PORTS];

  for (genvar idx = 0; idx < OUT_NUM_PORTS; idx++) begin : gen_axi_fifos
    axi_fifo #(
      .WIDTH(1 + USER_W + IN_DATA_W                      ),
      .SIZE (OUT_FIFO_SIZE - $clog2(IN_DATA_W/OUT_DATA_W))
    ) axi_fifo_out (
      .clk     (clk                                                            ),
      .reset   (rst                                                            ),
      .clear   (1'b0                                                           ),
      .i_tdata ({split_tlast[idx], split_tuser[idx], split_tdata[idx]}         ),
      .i_tvalid(split_tvalid[idx]                                              ),
      .i_tready(split_tready[idx]                                              ),
      .o_tdata ({out_fifo_tlast[idx], out_fifo_tuser[idx], out_fifo_tdata[idx]}),
      .o_tvalid(out_fifo_tvalid[idx]                                           ),
      .o_tready(out_fifo_tready[idx]                                           ),
      .space   (                                                               ),
      .occupied(                                                               )
    );
  end


  //-------------------------------------------------------------------------
  // Resize
  //-------------------------------------------------------------------------

  if (IN_DATA_W == OUT_DATA_W) begin : gen_no_resize
    assign o_tdata         = out_fifo_tdata;
    assign o_tuser         = out_fifo_tuser;
    assign o_tlast         = out_fifo_tlast;
    assign o_tvalid        = out_fifo_tvalid;
    assign out_fifo_tready = o_tready;
  end else begin : gen_resize
    for (genvar idx = 0; idx < OUT_NUM_PORTS; idx++) begin : gen_axis_width_conv
      localparam IN_WORDS = IN_DATA_W/OUT_DATA_W;
      localparam COUNT_W  = $clog2(IN_WORDS);

      //-----------------------------------------
      // Data Width Conversion
      //-----------------------------------------

      axis_width_conv #(
        .WORD_W   (OUT_DATA_W),
        .IN_WORDS (IN_WORDS  ),
        .OUT_WORDS(1         ),
        .SYNC_CLKS(1         ),
        .PIPELINE ("INOUT"   )
      ) axis_width_conv_i (
        .s_axis_aclk  (clk                 ),
        .s_axis_rst   (rst                 ),
        .s_axis_tdata (out_fifo_tdata[idx] ),
        .s_axis_tkeep ('1                  ),
        .s_axis_tlast (out_fifo_tlast[idx] ),
        .s_axis_tvalid(out_fifo_tvalid[idx]),
        .s_axis_tready(out_fifo_tready[idx]),
        .m_axis_aclk  (clk                 ),
        .m_axis_rst   (rst                 ),
        .m_axis_tdata (o_tdata[idx]        ),
        .m_axis_tkeep (                    ),
        .m_axis_tlast (o_tlast[idx]        ),
        .m_axis_tvalid(o_tvalid[idx]       ),
        .m_axis_tready(o_tready[idx]       )
      );

      //-----------------------------------------
      // TUSER Handling
      //-----------------------------------------

      // We write to the TUSER FIFO in lock-step with the axis_width_conv
      // module. But read it out at a rate of 1/IN_WORDS so that the TUSER
      // output matches the same data it did on the way in.

      logic               user_fifo_i_tready;
      logic               user_fifo_o_tvalid;
      logic               user_fifo_o_tready;
      logic [COUNT_W-1:0] word_count;

      // This counter tells when we're outputting the last TUSER for the
      // current input word, so we know when to pop the output from the FIFO.
      always_ff @(posedge clk) begin
        if (o_tvalid[idx] && o_tready[idx]) begin
          if (word_count == IN_WORDS-1) begin
            word_count <= '0;
          end else begin
            word_count <= word_count + 1;
          end
        end

        if (rst) begin
          word_count <= '0;
        end
      end

      assign user_fifo_o_tready = (word_count == IN_WORDS-1) && o_tvalid[idx] && o_tready[idx];

      axi_fifo #(
        .WIDTH(USER_W),
        .SIZE (2     )
      ) axi_fifo_tuser (
        .clk     (clk                                         ),
        .reset   (rst                                         ),
        .clear   ('0                                          ),
        .i_tdata (out_fifo_tuser[idx]                         ),
        .i_tvalid(out_fifo_tvalid[idx] && out_fifo_tready[idx]),
        .i_tready(user_fifo_i_tready                          ),
        .o_tdata (o_tuser[idx]                                ),
        .o_tvalid(user_fifo_o_tvalid                          ),
        .o_tready(user_fifo_o_tready                          ),
        .occupied(                                            ),
        .space   (                                            )
      );

      // Make sure we don't overflow/underflow the TUSER FIFO
      //synthesis translate_off
      always_ff @(posedge clk) begin
        if (out_fifo_tvalid[idx] && out_fifo_tready[idx] && !user_fifo_i_tready) begin
          $error("TUSER FIFO overflow");
        end
        if (user_fifo_o_tready && !user_fifo_o_tvalid) begin
          $error("TUSER FIFO underflow");
        end
      end
      //synthesis translate_on

    end
  end

endmodule : axis_load_split


`default_nettype wire
