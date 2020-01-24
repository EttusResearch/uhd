//
// Copyright 2013 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


module chdr_dechunker # (
   parameter PAD_VALUE = 64'hFFFFFFFF_FFFFFFFF
) (
   input          clk,
   input          reset,
   input          clear,
   input [15:0]   frame_size,

   input [63:0]   i_tdata,
   input          i_tvalid,
   output         i_tready,

   output [63:0]  o_tdata,
   output         o_tlast,
   output         o_tvalid,
   input          o_tready,

   output         error
);

   localparam ST_HEADER  = 2'd0;
   localparam ST_DATA    = 2'd1;
   localparam ST_PADDING = 2'd2;
   localparam ST_ERROR   = 2'd3;

   reg [1:0]   state;
   reg [15:0]  frame_rem, pkt_rem;
   wire        i_tlast;

   // axi_len = ceil(length / 8)
   wire [15:0] cvita_len_ceil = i_tdata[31:16] + 7;
   wire [15:0] axi_len = {3'b000, cvita_len_ceil[15:3]};

   always @(posedge clk) begin
      if (reset | clear) begin
         state <= ST_HEADER;
         frame_rem <= 16'd0;
         pkt_rem   <= 16'd0;
      end else if (i_tvalid & i_tready) begin
         case (state)
            ST_HEADER: begin
               if (axi_len > frame_size)
                  state <= ST_ERROR;
               else if (~o_tlast)
                  state <= ST_DATA;
               else
                  state <= ST_PADDING;

               frame_rem <= frame_size - 16'd1;
               pkt_rem   <= axi_len - 16'd1;
            end

            ST_DATA: begin
               if (o_tlast) begin
                  state   <= i_tlast ? ST_HEADER : ST_PADDING;
                  pkt_rem <= 16'd0;
               end else begin
                  state   <= ST_DATA;
                  pkt_rem <= pkt_rem - 16'd1;
               end
               frame_rem <= frame_rem - 16'd1;
            end

            ST_PADDING: begin
               if (i_tlast) begin
                  state   <= ST_HEADER;
                  frame_rem <= 16'd0;
               end else begin
                  state   <= ST_PADDING;
                  frame_rem <= frame_rem - 16'd1;
               end
            end

            ST_ERROR: begin
               // We never leave the error state. However, we can't reach it
               // with PCIe if we configure our transport according to the
               // NI-RIO configuration.
               state <= ST_ERROR;
            end
         endcase
      end
   end

   assign i_tready = o_tready | (state == ST_PADDING);
   assign i_tlast = (frame_rem == 16'd1); //Temp signal

   assign o_tvalid = i_tvalid & (state != ST_PADDING);
   assign o_tlast = (pkt_rem != 0) ? (pkt_rem == 16'd1) : (axi_len == 16'd1);
   assign o_tdata  = i_tdata;

   assign error = (state == ST_ERROR);

endmodule // chdr_dechunker
