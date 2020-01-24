//
// Copyright 2015 Ettus Research
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Parses CVITA packet header and outputs discrete signals.

module cvita_hdr_parser #(
  parameter REGISTER = 1    // 0 = No registering, header / vita_time only valid when hdr_stb / vita_time_stb is asserted (lower resource utilization)
                            // 1 = Header / vita time are registered and valid for the length of entire packet.
)(
  input clk, input reset, input clear,
  output hdr_stb,
  output [1:0] pkt_type, output eob, output has_time,
  output [11:0] seqnum, output [15:0] length, output [15:0] payload_length,
  output [15:0] src_sid, output [15:0] dst_sid,
  output vita_time_stb,
  output [63:0] vita_time,
  input [63:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
  output [63:0] o_tdata, output o_tlast, output o_tvalid, input o_tready
);

  generate
    if (REGISTER) begin
      axi_fifo_flop2 #(.WIDTH(65)) axi_fifo_flop (
        .clk(clk), .reset(reset), .clear(clear),
        .i_tdata({i_tlast,i_tdata}), .i_tvalid(i_tvalid), .i_tready(i_tready),
        .o_tdata({o_tlast,o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready),
        .space(), .occupied());
    end else begin
      assign o_tdata  = i_tdata;
      assign o_tlast  = i_tlast;
      assign o_tvalid = i_tvalid;
      assign i_tready = o_tready;
    end
  endgenerate

  reg first_time, first_line, read_time;
  wire [63:0] hdr, hdr_vita_time;
  reg [63:0] hdr_reg, vita_time_reg;

  always @(posedge clk) begin
    if (reset | clear) begin
      first_time    <= 1'b1;
      first_line    <= 1'b1;
      read_time     <= 1'b0;
      hdr_reg       <= 64'd0;
      vita_time_reg <= 64'd0;
    end else begin
      if (o_tvalid & o_tready) begin
        first_time      <= 1'b0;
        if (first_line) begin
          hdr_reg       <= o_tdata;
          first_line    <= 1'b0;
          if (has_time & ~o_tlast) begin
            read_time   <= 1'b1;
          end
        end
        if (read_time) begin
          vita_time_reg <= o_tdata;
          read_time     <= 1'b0;
        end
        if (o_tlast) begin
          first_line    <= 1'b1;
        end
      end
    end
  end

  // REGISTER = 0: Always use o_tdata, output only valid when hdr_stb = 1
  // REGISTER = 1: Mux to make sure header output is available immediately and also registered for rest of packet.
  assign hdr            = (hdr_stb       | (REGISTER == 0)) ? o_tdata : hdr_reg;
  assign hdr_vita_time  = (vita_time_stb | (REGISTER == 0)) ? o_tdata : vita_time_reg;

  assign hdr_stb        = first_line & o_tvalid & o_tready;
  assign pkt_type       = hdr[63:62];
  assign has_time       = hdr[61];
  assign eob            = hdr[60];
  assign seqnum         = hdr[59:48];
  assign length         = hdr[47:32];
  assign payload_length = length - (has_time ? 16'd16 : 16'd8);
  assign src_sid        = hdr[31:16];
  assign dst_sid        = hdr[15:0];

  assign vita_time_stb  = read_time & o_tvalid & o_tready;
  assign vita_time      = hdr_vita_time;

endmodule
