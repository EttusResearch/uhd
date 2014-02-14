//
// Copyright 2013 Ettus Research LLC
//


//
// Removes 6 alignment bytes at the beginning of every packet.
// This gives us proper alignment to the IP/UDP header.
//
// Place an SOF indication in bit[3] of the output tuser.
//

module axi64_to_xge64
  (
   input clk,
   input reset,
   input clear,
   input [63:0] s_axis_tdata,
   input [3:0] s_axis_tuser,
   input s_axis_tlast,
   input s_axis_tvalid,
   output s_axis_tready,
   output [63:0] m_axis_tdata,
   output [3:0] m_axis_tuser,
   output m_axis_tlast,
   output m_axis_tvalid,
   input m_axis_tready
   );

   localparam STORE_FIRST = 0;
   localparam FORWARD_FULL = 1;
   localparam RELEASE_LAST = 2;

   reg [1:0] state;
   reg [15:0] saved;
   reg [2:0]  last_occ;
   reg 	      last_sof;

   //on last line when eof and not finishing with 7 or 8 bytes as last word.
   wire       last_line = (s_axis_tlast && !(s_axis_tuser[2:0] == 3'b111 || s_axis_tuser[2:0] == 3'b000));

   always @(posedge clk) begin
      if(reset | clear) begin
         last_sof <= 0;
         last_occ <= 0;
         state <= STORE_FIRST;
      end
      else begin
         if (s_axis_tvalid && s_axis_tready) begin
            saved <= s_axis_tdata[15:0];
            last_occ <= s_axis_tuser[2:0];
            last_sof <= (state == STORE_FIRST) ;
         end

         case(state)

           STORE_FIRST: begin
              if (s_axis_tvalid && s_axis_tready) begin
                 state <= FORWARD_FULL;
              end
           end

           FORWARD_FULL: begin
              if (s_axis_tvalid && s_axis_tready && s_axis_tlast) begin
                 state <= last_line? STORE_FIRST : RELEASE_LAST;
              end
           end

           RELEASE_LAST: begin
              if (m_axis_tvalid && m_axis_tready) begin
                 state <= STORE_FIRST;
              end
           end

         endcase //state
      end
   end

   assign m_axis_tdata[63:0] = {saved, s_axis_tdata[63:16]};
   assign m_axis_tuser[3] = last_sof;
   assign m_axis_tlast = (state == RELEASE_LAST)? 1'b1 : last_line;
   assign m_axis_tuser[2:0] = ((state == RELEASE_LAST)? last_occ : (last_line? s_axis_tuser[2:0] : 3'b110)) + 3'b010;
   assign m_axis_tvalid = (state == STORE_FIRST)? 0 : ((state == RELEASE_LAST)? 1 : s_axis_tvalid);
   assign s_axis_tready = (state == STORE_FIRST)? 1 : ((state == RELEASE_LAST)? 0 : m_axis_tready);

endmodule //fifo69_txrealign
