//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Adds preamble, EOP, and CRC/num_words check
// <preamble> <packet> <EOP> [control_chksum,word_count,payload_chksum] 
// <preamble>   = 64'h9E6774129E677412
// <EOP>        = 64'h2A1D632F2A1D632F

module axi_add_preamble #(
  parameter WIDTH=64
) (
   input clk,
   input reset,
   input clear,
   //
   input [WIDTH-1:0] i_tdata,
   input i_tlast,
   input i_tvalid,
   output i_tready,
   //
   output reg [WIDTH-1:0] o_tdata,
   output o_tvalid,
   input o_tready
);

    function [0:0] cvita_get_has_time;
        input [63:0] header;
        cvita_get_has_time = header[61];
    endfunction

   //States
   localparam IDLE = 0;
   localparam PREAMBLE = 1;
   localparam PASS = 3;
   localparam EOP = 4;
   localparam CRC = 5;
   
   localparam PAYLOAD_WORDCOUNT_WIDTH = 16;
   localparam PAYLOAD_CHKSUM_WIDTH = 32;
   localparam CONTROL_CHKSUM_WIDTH = 16;
   
   reg [2:0]  state, next_state;
   
   reg [PAYLOAD_WORDCOUNT_WIDTH-1:0] word_count;
   reg [PAYLOAD_WORDCOUNT_WIDTH-1:0] cntrl_length = 16'd2;
   wire [PAYLOAD_CHKSUM_WIDTH-1:0] payload_chksum;
   wire [CONTROL_CHKSUM_WIDTH-1:0] control_chksum;

  // Payload LFSR
  crc_xnor #(.INPUT_WIDTH(WIDTH), .OUTPUT_WIDTH(PAYLOAD_CHKSUM_WIDTH)) payload_chksum_gen (
     .clk(clk), .rst(word_count<=cntrl_length), .hold(~(i_tready && i_tvalid)),
     .input_data(i_tdata), .crc_out(payload_chksum)
  );
  
  // Control LFSR
  crc_xnor #(.INPUT_WIDTH(WIDTH), .OUTPUT_WIDTH(CONTROL_CHKSUM_WIDTH)) control_chksum_gen (
     .clk(clk), .rst(word_count=='d0), .hold(~(i_tready && i_tvalid) || word_count>=cntrl_length),
     .input_data(i_tdata), .crc_out(control_chksum)
  );
  
  //Update control length so control checksum is correct
  always @(posedge clk) begin
    if (state == IDLE && i_tvalid)
        cntrl_length <= cvita_get_has_time(i_tdata) ? 16'd2 : 16'd1;
  end
  
  //Note that word_count includes EOP
  always @(posedge clk) begin
     if (state == IDLE) begin
        word_count <= 0;
     end else if (i_tready && i_tvalid || (o_tready && state == EOP)) begin
        word_count <= word_count+1;
     end
  end
      
   always @(posedge clk) 
      if (reset | clear) begin
         state <= IDLE;
      end else begin
         state <= next_state;
      end 

   always @(*) begin
      case(state)
         IDLE: begin
            if (i_tvalid) begin
               next_state = PREAMBLE;
            end else begin
               next_state = IDLE;
            end
         end 
         
         PREAMBLE: begin
            if(o_tready) begin
                next_state = PASS;
            end else begin
                next_state = PREAMBLE;
            end
         end                

         PASS: begin
            if(i_tready && i_tvalid && i_tlast) begin
                 next_state = EOP;
             end else begin
                 next_state = PASS;
             end
         end
         
         EOP: begin
            if(o_tready) begin
                next_state = CRC;
            end else begin
                next_state = EOP;
            end
         end          

         CRC: begin
            if(o_tready) begin
                 next_state = IDLE;
             end else begin
                 next_state = CRC;
             end
         end
         
         default: begin
            next_state = IDLE;
         end
        
      endcase
   end 

   //
   // Muxes
   //
   always @*
      begin
         case(state)
            IDLE:           o_tdata = 0; 
            PASS:           o_tdata = i_tdata;
            PREAMBLE:       o_tdata = 64'h9E6774129E677412;
            EOP:            o_tdata = 64'h2A1D632F2A1D632F;
            CRC:            o_tdata = {control_chksum,word_count,payload_chksum};
            default:        o_tdata = 0;
            
         endcase 
      end

   assign o_tvalid = (state == PASS) ? i_tvalid : (state != IDLE);
   assign i_tready = (state == PASS) ? o_tready : 1'b0;

endmodule 



