//
// Copyright 2016 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//
// Strips preamble, EOP, and CRC/num_words check
// <preamble> <packet> <EOP> [control_chksum,word_count,payload_chksum] 
// <preamble>   = 64'h9E6774129E677412
// <EOP>        = 64'h2A1D632F2A1D632F

module axi_strip_preamble #(
  parameter WIDTH=64,
  parameter MAX_PKT_SIZE=512 //Set to 128 in sim to fill up buffers faster to help try and trigger more fail cases. 
) (
   input clk,
   input reset,
   input clear,
   //
   input [WIDTH-1:0] i_tdata,
   input i_tvalid,
   output i_tready,
   //
   output [WIDTH-1:0] o_tdata,
   output o_tlast,
   output o_tvalid,
   input  o_tready,
   //
   output pkt_dropped,
   output crc_err,
   output crit_error
);

    function [0:0] cvita_get_has_time;
        input [63:0] header;
        cvita_get_has_time = header[61];
    endfunction
    
   //State machine info
   reg [1:0] state, next_state;
   
   localparam IDLE = 0;
   localparam CHECK_HDR = 1; 
   localparam PASS = 2;
   localparam CHECK_CRC = 3;
   
   localparam PAYLOAD_WORDCOUNT_WIDTH = 16;
   localparam PAYLOAD_CHKSUM_WIDTH = 32;
   localparam CONTROL_CHKSUM_WIDTH = 16;  

   //Note that held_word is required when EOP is detected
   //so that we can rewrite into memory the last word + last bit
   reg [WIDTH-1:0] held_word;
   reg [WIDTH-1:0] held_word_r;
   always @(posedge clk) begin
    if(i_tvalid && i_tready) begin
        held_word <= i_tdata;
        held_word_r <= held_word;
    end
   end   
   
   //Look for next word that specifies if frame has timestamp
   reg [PAYLOAD_WORDCOUNT_WIDTH-1:0] cntrl_length = 16'd2;
   always @(posedge clk) begin 
     if ((next_state == CHECK_HDR || state == CHECK_HDR) && i_tvalid)
         cntrl_length <= cvita_get_has_time(i_tdata) ? 16'd2 : 16'd1;
   end

   reg [PAYLOAD_WORDCOUNT_WIDTH-1:0] word_count;
   wire det_preamble =  (i_tdata == 64'h9E6774129E677412);
   wire det_eop =       (i_tdata == 64'h2A1D632F2A1D632F);

   wire [PAYLOAD_CHKSUM_WIDTH-1:0] payload_chksum;
   wire [CONTROL_CHKSUM_WIDTH-1:0] control_chksum;
   
   // Payload LFSR. Must hold LFSR once detected EOP so checksum does not keep updating after EOP
   // Note the payload LFSR also includes the EOP in its checksum
   crc_xnor #(.INPUT_WIDTH(WIDTH), .OUTPUT_WIDTH(PAYLOAD_CHKSUM_WIDTH)) payload_chksum_gen (
      .clk(clk), .rst(word_count<=cntrl_length), .hold(~(i_tready && i_tvalid) || det_eop || state == CHECK_CRC),
      .input_data(i_tdata), .crc_out(payload_chksum)
   );
   
   // Control LFSR. Varies in size based on whether the control information includes a timestamp
   // Hold the LFSR once the control word(s) have been parsed
   crc_xnor #(.INPUT_WIDTH(WIDTH), .OUTPUT_WIDTH(CONTROL_CHKSUM_WIDTH)) control_chksum_gen (
      .clk(clk), .rst(word_count=='d0), .hold(~(i_tready && i_tvalid) || word_count>=cntrl_length),
      .input_data(i_tdata), .crc_out(control_chksum)
   );    
   
   //Good frame is when the word_count is correct and the control checksum passes. 
   //Allows passthrough of payloads with bit errors to reduce overall dropped frame rate
   wire frame_good = (word_count == i_tdata[47:32]) && (control_chksum == i_tdata[63:48]) && state == CHECK_CRC;
   
   //CRC error only increments when the state machine makes it to CHECK_CRC state
   //It will not increment if a preamble or eop is detected outside of IDLE
   wire payload_crc_check = (payload_chksum == i_tdata[31:0]) && state == CHECK_CRC;
   assign crc_err = (~frame_good || ~payload_crc_check) && state == CHECK_CRC && i_tvalid;

   //Increment word_count for payload and EOP
   always @(posedge clk) begin
      if (state == IDLE || pkt_dropped) begin
         word_count <= 0;
      end else if ((state == PASS || state == CHECK_HDR) && i_tready && i_tvalid) begin
         word_count <= word_count+1'b1;
      end
   end

   always @(posedge clk) begin
      if (reset | clear) begin
         state <= IDLE;
      end else begin
         state <= next_state;
      end
   end
   
   //Only drop packet if preamble detected outside of idle or bad frame was detected during CRC check
   assign pkt_dropped = ((state != IDLE) && det_preamble && i_tvalid) || ((state == CHECK_CRC) && ~frame_good && i_tvalid);

   //When preamble is missing or has bit error, state machine stays in IDLE
   //When EOP is missing or has bit error, either the next preamble is detected and resets logic
   //or state machine exits on next EOP and fails CRC check.
   //For cables with very high BER its possible for the write buffer to fill up which causes a critical error and resets everything
   always @(*) begin
      case(state)
         IDLE: begin
            if (det_preamble && i_tvalid) //Preamble detected so check to see if timestamp is part of header
               next_state = CHECK_HDR;
            else
               next_state = IDLE;
         end 
         
         //Check incoming word to see if frame will have timestamp
         CHECK_HDR: begin
            if(crit_error) begin //Critical error so reset SM
                next_state = IDLE; 
            end else if(~det_preamble && i_tvalid && i_tready) begin //Found control word so go to normal pass state
                next_state = PASS; 
            end else begin
                next_state = CHECK_HDR;
            end
         end             
         
         //Note if early preamble is detected in PASS state everything is reset for the next frame
         PASS: begin
            if(crit_error) begin //Critical error so reset SM
                next_state = IDLE;
            end else if(det_preamble && i_tvalid) begin //Saw preamble so drop packet and start over
                next_state = CHECK_HDR;  
            end else if(det_eop && i_tvalid && i_tready) begin //Saw EOP so check for crc on next word
                next_state = CHECK_CRC; 
            end else begin
                next_state = PASS;
            end
         end     
         
         //Check for crc and go to idle or go back to pass if another preamble is detected
         CHECK_CRC: begin
             if(crit_error) begin //Critical error so reset SM
                 next_state = IDLE; 
             end else if(det_preamble && i_tvalid) begin //Saw preamble so drop packet and start over
                 next_state = CHECK_HDR; 
             end else if(i_tvalid) begin //Got word which should've been the CRC
                 next_state = IDLE;
             end else begin
                 next_state = CHECK_CRC;
             end
          end
         
         default: begin
            next_state = IDLE;
         end

      endcase
   end 

   wire [WIDTH-1:0] buf_tdata;
   wire buf_tlast, buf_tvalid, buf_tready, buf_empty;
   reg buf_full = 1'b0;
   wire [$clog2(MAX_PKT_SIZE)-1:0] valid_rd_addr;
   reg buf_empty_r;

   assign mem_tvalid = (state == PASS || state == CHECK_HDR) ? (i_tvalid && ~pkt_dropped) : 1'b0;
   assign i_tready = (state == PASS || state == CHECK_HDR) ?  buf_tready : 1'b1;
   
   assign crit_error = buf_full && buf_empty; //This should never happen, if it does that indicates poor BER over Aurora or packet size too large
   
   /////////////////////////////////////////////////
   //Fifo to store incoming packets
   //The write pntr rewinds whenever an error occurs
   /////////////////////////////////////////////////
   
   wire int_tready;

   reg [$clog2(MAX_PKT_SIZE)-1:0] wr_addr, prev_wr_addr, rd_addr, old_rd_addr;
   reg [$clog2(MAX_PKT_SIZE):0] in_pkt_cnt, out_pkt_cnt;
   wire read         = ~buf_empty && (int_tready || buf_empty_r); //Read from buffer if its no longer empty to prime output reg
   wire almost_full  = (wr_addr == valid_rd_addr-1'b1); //We need to look at the masked rd_addr in case its 1 ahead

   assign buf_tready   = ~buf_full;
   wire write        = mem_tvalid && buf_tready && ~det_eop;
   
   //If frame was good we need to go back and rewrite the last word and set the last bit
   wire [WIDTH:0] int_write_data = (frame_good) ? {1'b1,held_word_r} : {1'b0,i_tdata};
   wire [$clog2(MAX_PKT_SIZE)-1:0] int_wr_addr = (frame_good) ? wr_addr-1 : wr_addr;
   
   //BRAM inferred
   wire [WIDTH:0] buf_data;
    ram_2port #(.DWIDTH(WIDTH+1), .AWIDTH($clog2(MAX_PKT_SIZE))) pkt_buf
      (.clka(clk), .ena(1'b1), .wea(1'b1), .addra(int_wr_addr),
       .dia(int_write_data), .doa(),
       .clkb(clk), .enb(read), .web(1'b0), .addrb(rd_addr), .dib(),
       .dob(buf_data));

   // Write logic
   always @(posedge clk) begin
        
        // Rewind logic
        if(pkt_dropped || crit_error)
            wr_addr <= prev_wr_addr;
        else if(write)
            wr_addr <= wr_addr + 1'b1;
        
        if (almost_full) begin
            if (write && ~read) begin
                buf_full       <= 1'b1;
            end
        end else begin
            if (~write && read) begin
                buf_full       <= 1'b0;
            end
        end
            
        if (frame_good) begin
            in_pkt_cnt   <= in_pkt_cnt + 1'b1;
            prev_wr_addr <= wr_addr;
        end
           
        if (reset || clear) begin
            wr_addr       <= 0;
            prev_wr_addr  <= 0;
            in_pkt_cnt    <= 0;
        end
        
        if(reset || clear || crit_error) begin
            buf_full          <= 1'b0;
        end
   end

   // Read logic. Hold data if pkt_count is equal
   assign buf_empty = in_pkt_cnt == out_pkt_cnt;
   reg last_word;
   
   //Use current read addr only if read is enabled
   assign valid_rd_addr = (read) ? rd_addr : old_rd_addr;
   
   assign buf_tvalid = ~buf_empty_r && ~(last_word && buf_empty);
   
   assign buf_tdata = buf_data[WIDTH-1:0];
   assign buf_tlast = buf_data[WIDTH];

   always @(posedge clk) begin
     buf_empty_r <= buf_empty;
     
     if (read) old_rd_addr <= rd_addr; //Keeps track of last valid rd_addr
     
     //Last word has two possibilities
     //If buffer empty then we need to rewind rd_addr and mask reading from buffer
     //If buffer is not empty continue with rd_addr and continue reading from buffer
     last_word <= buf_tvalid && int_tready && buf_tlast;
     
     //Need to rewind rd_addr since it incremented one too far
     //This means there will be one cycle where rd_addr is ahead of where it should be
     //Other logic that uses rd_addr will have it masked for that cycle
     if (last_word && buf_empty)    rd_addr      <= rd_addr - 1;
     else if (read)                 rd_addr      <= rd_addr + 1;
     
     // Prevent output until we have a full packet
     if (buf_tvalid && int_tready && buf_tlast) begin
       out_pkt_cnt  <= out_pkt_cnt + 1'b1;
     end
     
     if (reset || clear) begin
       old_rd_addr <= 0;
       rd_addr     <= 0;
       out_pkt_cnt <= 0;
     end
   end
   
   assign o_tlast = buf_tlast;
   assign o_tdata = buf_tdata;
   assign o_tvalid = buf_tvalid;
   assign int_tready = o_tready;

endmodule

  
