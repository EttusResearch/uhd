//
// Copyright 2013 Ettus Research LLC
//

// Quantize cvita packets to a configurable quantum value. o_tlast and 
// i_tready will be held off until the entire quantized packet is xferred. 
// If quantum is changed, it is the responsibility of the client to clear
// this module. error is asserted if a packet is larger than the quantum
// error can be reset by asserting reset or clear.

module cvita_chunker # (
   parameter PAD_VALUE = 64'hFFFFFFFF_FFFFFFFF
) (
   input          clk,
   input          reset,
   input          clear,
   input [15:0]   frame_size,

   input [63:0]   i_tdata,
   input          i_tlast,
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
   reg [15:0]  frame_rem;

   wire [15:0] cvita_len_ceil = i_tdata[47:32] + 7;
   wire [15:0] axi_len = {3'b000, cvita_len_ceil[15:3]};

   always @(posedge clk) begin
      if (reset | clear) begin
         state <= ST_HEADER;
         frame_rem <= 16'd0;
      end else if (o_tvalid & o_tready) begin
         case (state)
            ST_HEADER: begin
               if (axi_len > frame_size)
                  state <= ST_ERROR;
               else if (i_tlast)
                  state <= ST_PADDING;
               else
                  state <= ST_DATA;
                  
               frame_rem <= frame_size - 16'd1;
            end

            ST_DATA: begin
               if (i_tlast) begin
                  state   <= o_tlast ? ST_HEADER : ST_PADDING;
                  frame_rem <= o_tlast ? 16'd0 : (frame_rem - 16'd1);
               end else begin
                  state <= ST_DATA;
                  frame_rem <= frame_rem - 16'd1;
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

   assign i_tready = o_tready & (state != ST_PADDING);
   
   assign o_tvalid = i_tvalid | (state == ST_PADDING);
   assign o_tlast = (frame_rem != 0) ? (frame_rem == 16'd1) : (axi_len == 16'd1);
   assign o_tdata = (state == ST_PADDING) ? PAD_VALUE : i_tdata;

   assign error = (state == ST_ERROR);

endmodule // cvita_chunker
