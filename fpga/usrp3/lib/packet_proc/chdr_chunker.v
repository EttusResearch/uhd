//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
// Copyright 2019 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

// Quantize chdr packets to a configurable quantum value. o_tlast and
// i_tready will be held off until the entire quantized packet is xferred.
// If quantum is changed, it is the responsibility of the client to clear
// this module. error is asserted if a packet is larger than the quantum
// error can be reset by asserting reset or clear.

`default_nettype none
module chdr_chunker # (
   parameter PAD_VALUE = 64'hFFFFFFFF_FFFFFFFF,
             HOLD_ERROR = 1'b1 // If high, hold error until reset, else pulse
) (
   input wire          clk,
   input wire          reset,
   input wire          clear,
   input wire [15:0]   frame_size,

   input wire [63:0]   i_tdata,
   input wire          i_tlast,
   input wire          i_tvalid,
   output reg          i_tready,

   output wire [63:0]  o_tdata,
   output wire         o_tlast,
   output reg          o_tvalid,
   input  wire         o_tready,

   output wire         error
);

   localparam ST_HEADER  = 2'd0;
   localparam ST_DATA    = 2'd1;
   localparam ST_PADDING = 2'd2;
   localparam ST_ERROR   = 2'd3;

   reg [1:0]   state;
   reg [15:0]  frame_rem;

   // axi_len = ceil(length / 8)
   wire [15:0] chdr_len_ceil = i_tdata[31:16] + 16'd7;
   wire [15:0] axi_len = {3'b000, chdr_len_ceil[15:3]};

   always @(posedge clk) begin
      if (reset | clear) begin
         state <= ST_HEADER;
         frame_rem <= 16'd0;
      end else if ((state == ST_ERROR) & i_tlast & i_tvalid & !HOLD_ERROR) begin
         state <= ST_HEADER;
         frame_rem <= 16'd0;
      end else if (o_tready) begin
         case (state)
            ST_HEADER: begin
               if (i_tvalid) begin
                  if ((axi_len > frame_size) | (axi_len == 16'd0))
                     state <= ST_ERROR;
                  else if (i_tlast)
                     state <= ST_PADDING;
                  else
                     state <= ST_DATA;

                  frame_rem <= frame_size - 16'd1;
               end
            end

            ST_DATA: begin
               if (i_tvalid) begin
                  if (i_tlast) begin
                     state   <= o_tlast ? ST_HEADER : ST_PADDING;
                     frame_rem <= o_tlast ? 16'd0 : (frame_rem - 16'd1);
                  end else begin
                     state <= ST_DATA;
                     frame_rem <= frame_rem - 16'd1;
                  end
               end
            end

            ST_PADDING: begin
               if (o_tlast) begin
                  state   <= ST_HEADER;
                  frame_rem <= 16'd0;
               end else begin
                  state   <= ST_PADDING;
                  frame_rem <= frame_rem - 16'd1;
               end
            end

         endcase
      end
   end

   always @(*) begin
      case (state)
         ST_HEADER: begin
            i_tready = o_tready;
            o_tvalid = (axi_len <= frame_size) & (axi_len > 16'd0) & i_tvalid;
         end

         ST_DATA: begin
            i_tready = o_tready;
            o_tvalid = i_tvalid;
         end

         ST_PADDING: begin
            i_tready = 1'b0;
            o_tvalid = 1'b1;
         end

         ST_ERROR: begin
            i_tready = 1'b1;
            o_tvalid = 1'b0;
         end

         default: begin
            i_tready = 1'b0;
            o_tvalid = 1'b0;
         end
      endcase
   end

   assign o_tlast = (frame_rem != 16'd0) ? (frame_rem == 16'd1) : (axi_len == 16'd1);
   assign o_tdata = (state == ST_PADDING) ? PAD_VALUE : i_tdata;

   assign error = (state == ST_ERROR);

endmodule // chdr_chunker

`default_nettype wire

