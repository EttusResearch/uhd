/////////////////////////////////////////////////////////////////////
//
// Copyright 2018 Ettus Research, A National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Module: chdr_deframer_2clk
// Description:
//  - Takes a sample stream in and uses the tuser input to frame
//   a CHDR packet which is output by the module
//   samples at the output
//
/////////////////////////////////////////////////////////////////////


module chdr_deframer_2clk #(
  parameter WIDTH = 32      // 32 and 64 bits supported
) (
  input samp_clk, input samp_rst, input pkt_clk, input pkt_rst,
  input [63:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
  output [WIDTH-1:0] o_tdata, output [127:0] o_tuser, output o_tlast, output o_tvalid, input o_tready
);

  localparam [1:0] ST_HEAD = 2'd0;
  localparam [1:0] ST_TIME = 2'd1;
  localparam [1:0] ST_BODY = 2'd2;

  reg [1:0]     chdr_state;

  wire [127:0]  hdr_i_tuser, hdr_o_tuser;
  wire          hdr_i_tvalid, hdr_i_tready;
  wire          hdr_o_tvalid, hdr_o_tready;

  wire [63:0]   body_i_tdata, body_o_tdata;
  wire          body_i_tlast, body_o_tlast;
  wire          body_i_tvalid, body_o_tvalid;
  wire          body_i_tready, body_o_tready;

  wire          has_time = i_tdata[61];
  reg [63:0]    held_i_tdata;
  reg           second_half;

  assign body_i_tdata = i_tdata;
  assign body_i_tlast = i_tlast;
  assign body_i_tvalid = (chdr_state == ST_BODY) ? i_tvalid : 1'b0;

  assign hdr_i_tuser = (chdr_state == ST_HEAD) ? { i_tdata, i_tdata } : { held_i_tdata, i_tdata }; // 2nd half ignored if no time
  assign hdr_i_tvalid = (chdr_state == ST_TIME) ? i_tvalid :
       ((chdr_state == ST_HEAD) & ~has_time) ? i_tvalid :
       1'b0;

  assign i_tready = (chdr_state == ST_BODY) ? body_i_tready : hdr_i_tready;

  // FIXME handle packets with no body
  always @(posedge pkt_clk) begin
    if (pkt_rst) begin
      chdr_state <= ST_HEAD;
    end else begin
      case(chdr_state)
        ST_HEAD:
          if (i_tvalid & hdr_i_tready)
            if (has_time) begin
              chdr_state <= ST_TIME;
              held_i_tdata <= i_tdata;
            end else begin
              chdr_state <= ST_BODY;
            end
        ST_TIME:
          if (i_tvalid & hdr_i_tready)
            chdr_state <= ST_BODY;
        ST_BODY:
          if (i_tvalid & body_i_tready & i_tlast)
            chdr_state <= ST_HEAD;
      endcase
    end
  end

  wire pkt_rst_stretch;
  pulse_stretch #(.SCALE('d10)) pkt_reset_i (
    .clk(pkt_clk),
    .rst(1'b0),
    .pulse(pkt_rst),
    .pulse_stretched(pkt_rst_stretch)
  );

  axi_fifo_2clk #(.WIDTH(128), .SIZE(5)) hdr_fifo_i (
    .i_aclk(pkt_clk), .o_aclk(samp_clk), .reset(pkt_rst_stretch),
    .i_tdata(hdr_i_tuser), .i_tvalid(hdr_i_tvalid), .i_tready(hdr_i_tready),
    .o_tdata(hdr_o_tuser), .o_tvalid(hdr_o_tvalid), .o_tready(hdr_o_tready)
  );

  axi_fifo_2clk #(.WIDTH(65), .SIZE(9)) body_fifo (
    .i_aclk(pkt_clk), .o_aclk(samp_clk), .reset(pkt_rst_stretch),
    .i_tdata({body_i_tlast, body_i_tdata}), .i_tvalid(body_i_tvalid), .i_tready(body_i_tready),
    .o_tdata({body_o_tlast, body_o_tdata}), .o_tvalid(body_o_tvalid), .o_tready(body_o_tready)
  );

  wire odd_len = hdr_o_tuser[98] ^ |hdr_o_tuser[97:96];

  generate
    if (WIDTH == 32) begin : gen_32bit_output
      // 32-bit Output

      always @(posedge samp_clk) begin
        if(samp_rst) begin
          second_half <= 1'b0;
        end else begin
          if(o_tvalid & o_tready) begin
            if(o_tlast)
              second_half <= 1'b0;
            else
              second_half <= ~second_half;
          end
        end
      end
      
      assign o_tdata = second_half ? body_o_tdata[WIDTH-1:0] : body_o_tdata[(2*WIDTH)-1:WIDTH];
      assign o_tlast = body_o_tlast & (second_half | odd_len);
      assign o_tuser = hdr_o_tuser;
      assign o_tvalid = hdr_o_tvalid & body_o_tvalid;
      
      assign hdr_o_tready = o_tvalid & o_tready & o_tlast;
      assign body_o_tready = o_tvalid & o_tready & (o_tlast | second_half);

    end else begin : gen_64bit_output
      // 64-bit Output

      assign o_tdata  = body_o_tdata;
      assign o_tlast  = body_o_tlast;
      assign o_tuser  = hdr_o_tuser;
      assign o_tvalid = hdr_o_tvalid & body_o_tvalid;
      
      assign hdr_o_tready  = o_tvalid & o_tready & o_tlast;
      assign body_o_tready = o_tvalid & o_tready;

    end
  endgenerate

endmodule
