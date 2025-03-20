//
// Copyright 2025 Ettus Research, a National Instruments Brand
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_channel_demux.sv
//
// Description:
//
//   The module is used to demultiplex CHDR data packets from on stream into
//   multiple channels based on the virtual channel (VC) field of the CHDR
//   header.
//
//   By default, VC 0 is routed to output port 0, VC 1 to port 1, and so on.
//   The CHANNEL_OFFSET can be used to shift the channel numbers, so that VC
//   channel N gets routed to port N-CHANNEL_OFFSET.
//
//   This module does not implement any checking on the VC to make sure it is
//   in a valid range. Behavior is undefined in this case. Therefore, it is
//   recommended to not input packets with an invalid VC.
//
//   Input and output FIFOs can be added using the PRE_FIFO_SIZE and
//   POST_FIFO_SIZE parameters to cut combinational paths or add additional
//   buffering.
//
// Parameters:
//
//   NUM_PORTS      : The number of data channels to multiplex. Can be 1 or
//                    more.
//   CHDR_W         : The width of the CHDR data packets.
//   CHANNEL_OFFSET : The offset for virtual channel numbers. Each packet will
//                    be routed to the output port VC-CHANNEL_OFFSET.
//   PRE_FIFO_SIZE  : The size of the FIFO buffering the input in log2. Set to
//                    1 to register all input paths. Set to -1 to remove this
//                    logic.
//   POST_FIFO_SIZE : The size of the FIFO buffering the outputs in log2. Set
//                    to 1 to register all input paths. Set to -1 to remove
//                    this logic.
//

`default_nettype none


module chdr_channel_demux #(
  parameter int NUM_PORTS      = 2,
  parameter int CHDR_W         = 64,
  parameter int CHANNEL_OFFSET = 0,
  parameter int PRE_FIFO_SIZE  = 1,
  parameter int POST_FIFO_SIZE = 1
) (
  input  wire logic                             clk,
  input  wire logic                             rst,

  input  wire logic [   CHDR_W-1:0]             in_tdata,
  input  wire logic                             in_tvalid,
  input  wire logic                             in_tlast,
  output wire logic                             in_tready,

  output wire logic [NUM_PORTS-1:0][CHDR_W-1:0] out_tdata,
  output wire logic [NUM_PORTS-1:0]             out_tvalid,
  output wire logic [NUM_PORTS-1:0]             out_tlast,
  input  wire logic [NUM_PORTS-1:0]             out_tready
);
  import rfnoc_chdr_utils_pkg::*;

  // Make sure channel number fits in VC field
  if ($clog2(CHANNEL_OFFSET + NUM_PORTS) > CHDR_VC_W) begin : gen_vc_assertion
    $error("Channel number exceeds VC field size!");
  end

  // Make sure we chose a sane number of ports
  if (NUM_PORTS < 1) begin : gen_num_ports_assertion
    $error("NUM_PORTS must be at least 1");
  end

  logic [CHDR_W-1:0] in_fifo_tdata;
  logic              in_fifo_tvalid;
  logic              in_fifo_tlast;
  logic              in_fifo_tready;

  logic [NUM_PORTS-1:0][CHDR_W-1:0] out_fifo_tdata;
  logic [NUM_PORTS-1:0][       0:0] out_fifo_tvalid;
  logic [NUM_PORTS-1:0][       0:0] out_fifo_tlast;
  logic [NUM_PORTS-1:0][       0:0] out_fifo_tready;


  //---------------------------------------------------------------------------
  // Input FIFO
  //---------------------------------------------------------------------------

  axi_fifo #(
    .WIDTH(1 + CHDR_W   ),
    .SIZE (PRE_FIFO_SIZE)
  ) axi_fifo_in (
    .clk     (clk                           ),
    .reset   (rst                           ),
    .clear   ('0                            ),
    .i_tdata ({in_tlast, in_tdata}          ),
    .i_tvalid(in_tvalid                     ),
    .i_tready(in_tready                     ),
    .o_tdata ({in_fifo_tlast, in_fifo_tdata}),
    .o_tvalid(in_fifo_tvalid                ),
    .o_tready(in_fifo_tready                ),
    .space   (                              ),
    .occupied(                              )
  );


  //---------------------------------------------------------------------------
  // Demultiplexer
  //---------------------------------------------------------------------------

  if (NUM_PORTS > 1) begin : gen_demux
    localparam NUM_PORTS_W = $clog2(NUM_PORTS);

    logic [     CHDR_W-1:0] chdr_header;
    logic [NUM_PORTS_W-1:0] port_num;

    assign port_num = chdr_get_vc(chdr_header[CHDR_HEADER_W-1:0]) - CHANNEL_OFFSET;

    axi_demux #(
      .WIDTH         (CHDR_W   ),
      .SIZE          (NUM_PORTS),
      .PRE_FIFO_SIZE (0        ),
      .POST_FIFO_SIZE(0        )
    ) axis_m_demux_i (
      .clk     (clk            ),
      .reset   (rst            ),
      .clear   (1'b0           ),
      .header  (chdr_header    ),
      .dest    (port_num       ),
      .i_tdata (in_fifo_tdata  ),
      .i_tlast (in_fifo_tlast  ),
      .i_tvalid(in_fifo_tvalid ),
      .i_tready(in_fifo_tready ),
      .o_tdata (out_fifo_tdata ),
      .o_tlast (out_fifo_tlast ),
      .o_tvalid(out_fifo_tvalid),
      .o_tready(out_fifo_tready)
    );

    //synthesis translate_off
    // Check the VC value in the header to make sure it's in the legal range
    // during simulation.
    logic sop = 1'b1;

    always_ff @(posedge clk) begin
      if (rst) begin
        sop <= 1'b1;
      end else begin
        if (in_fifo_tvalid && in_fifo_tready) begin
          if (sop) begin
            int vc, port;
            vc = chdr_get_vc(in_fifo_tdata[CHDR_HEADER_W-1:0]);
            port = vc - CHANNEL_OFFSET;
            assert (port >= 0 && port < NUM_PORTS) else $error(
              "Destination port %0d is not within allowed range [%0d, %0d]!",
                port, CHANNEL_OFFSET, CHANNEL_OFFSET+NUM_PORTS-1);
          end
          sop <= in_fifo_tlast;
        end
      end
    end
    //synthesis translate_on

  end else begin : gen_no_demux
    assign out_fifo_tdata  = in_fifo_tdata;
    assign out_fifo_tlast  = in_fifo_tlast;
    assign out_fifo_tvalid = in_fifo_tvalid;
    assign in_fifo_tready  = out_fifo_tready;
  end


  //---------------------------------------------------------------------------
  // Output FIFO
  //---------------------------------------------------------------------------

  for (genvar ch = 0; ch < NUM_PORTS; ch++) begin : gen_output_fifos
    axi_fifo #(
      .WIDTH(1 + CHDR_W    ),
      .SIZE (POST_FIFO_SIZE)
    ) axi_fifo_out (
      .clk     (clk                                     ),
      .reset   (rst                                     ),
      .clear   ('0                                      ),
      .i_tdata ({out_fifo_tlast[ch], out_fifo_tdata[ch]}),
      .i_tvalid(out_fifo_tvalid[ch]                     ),
      .i_tready(out_fifo_tready[ch]                     ),
      .o_tdata ({out_tlast[ch], out_tdata[ch]}          ),
      .o_tvalid(out_tvalid[ch]                          ),
      .o_tready(out_tready[ch]                          ),
      .space   (                                        ),
      .occupied(                                        )
    );
  end : gen_output_fifos

endmodule : chdr_channel_demux


`default_nettype wire
