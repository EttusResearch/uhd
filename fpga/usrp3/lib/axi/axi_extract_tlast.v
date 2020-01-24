//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
//
// AXI stream neds N+1 bits to transmit packets of N bits so that the LAST bit can be represented.
// LAST occurs relatively infrequently and can be synthesized by using an in-band ESC code to generate
// a multi-word sequence to encode it (and the escape character when it appears as data input).
//
// 0x1234567887654321 with last becomes
// 0xDEADBEEFFEEDCAFE 0x0000000000000001 0x1234567887654321
//
// 0xDEADBEEFFEEDCAFE with last becomes
// 0xDEADBEEFFEEDCAFE 0x0000000000000001 0xDEADBEEFFEEDCAFE
//
// 0xDEADBEEFFEEDCAFE without last becomes
// 0xDEADBEEFFEEDCAFE 0x0000000000000000 0xDEADBEEFFEEDCAFE
//

module axi_extract_tlast #(
  parameter WIDTH=64,
  parameter VALIDATE_CHECKSUM=0
) (
   input clk,
   input reset,
   input clear,
   //
   input [WIDTH-1:0] i_tdata,
   input i_tvalid,
   output reg i_tready,
   //
   output [WIDTH-1:0] o_tdata,
   output reg o_tlast,
   output reg o_tvalid,
   input  o_tready,
   //
   output reg checksum_error
);

   reg [1:0] state, next_state;
   
   localparam IDLE = 0;
   localparam EXTRACT1 = 1;
   localparam EXTRACT2 = 2;
   localparam EXTRACT3 = 3;
   
   assign     o_tdata = i_tdata;

   reg        checksum_error_pre;
   reg [31:0] checksum, old_checksum;

   always @(posedge clk) 
      if (reset | clear) begin
         checksum <= 0;
         old_checksum <= 0;
      end else if (VALIDATE_CHECKSUM && o_tready && i_tvalid && o_tlast) begin
         checksum <= 0;
         old_checksum <= 0;
      end else if (VALIDATE_CHECKSUM && i_tready && i_tvalid && (state == IDLE)) begin
         checksum <= checksum ^ i_tdata[31:0] ^ i_tdata[63:32];
         old_checksum <= checksum;
      end

   always @(posedge clk)
      checksum_error <= checksum_error_pre;

   always @(posedge clk) 
      if (reset | clear) begin
         state <= IDLE;
      end else begin
         state <= next_state;
      end 

   always @(*) begin
      checksum_error_pre = 0;
      case(state)
         //
         // Search for Escape sequence "0xDEADBEEFFEEDCAFE"
         // If ESC found don't pass data downstream but transition to next state.
         // else pass data downstream.
         //
         IDLE: begin
            o_tlast = 1'b0;
            if ((i_tdata == 64'hDEADBEEFFEEDCAFE) && i_tvalid) begin
               next_state = EXTRACT1;
               o_tvalid = 1'b0;
               i_tready = 1'b1;
            end else begin
               next_state = IDLE;
               o_tvalid = i_tvalid;
               i_tready = o_tready;
            end // else: !if((i_tdata == 'hDEADBEEFFEEDCAFE) && i_tvalid)
         end // case: IDLE
         //
         // Look at next data. If it's a 0x1 then o_tlast should be asserted with next data word.
         // if it's 0x0 then it signals emulation of the Escape code in the original data stream
         // and we should just pass the next data word through unchanged with no o_tlast indication.
         //
         EXTRACT1: begin
            o_tvalid = 1'b0;
            i_tready = 1'b1;
            o_tlast = 1'b0;
            if (i_tvalid) begin
               if (i_tdata[31:0] == 'h1) begin
                  if (VALIDATE_CHECKSUM && (old_checksum != i_tdata[63:32])) 
                     checksum_error_pre = 1'b1;
                  next_state = EXTRACT2;
               end else  begin
                  // We assume emulation and don't look for illegal codes.
                  next_state = EXTRACT3;
               end // else: !if(i_tdata == 'h1)
            end else begin // if (i_tvalid)
               next_state = EXTRACT1;
            end // else: !if(i_tvalid)
         end // case: EXTRACT1
         //
         // Assert o_tlast with data word.
         //
         EXTRACT2: begin
            o_tvalid = i_tvalid;
            i_tready = o_tready;
            o_tlast = 1'b1;
            if (i_tvalid & o_tready)
               next_state = IDLE;
            else
               next_state = EXTRACT2;
         end
         //
         // Emulation, don't assert o_tlast with dataword.
         //
         EXTRACT3: begin
            o_tvalid = i_tvalid;
            i_tready = o_tready;
            o_tlast = 1'b0;
            if (i_tvalid & o_tready)
               next_state = IDLE;
            else
               next_state = EXTRACT2;
         end
      endcase // case(state)
   end

endmodule // axi_extract_tlast

  