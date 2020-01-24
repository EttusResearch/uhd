//
// Copyright 2011-2012 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//


module vita_tx_deframer
  #(parameter BASE=0,
    parameter MAXCHAN=1,
    parameter USE_TRANS_HEADER=0)
   (input clk, input reset, input clear, input clear_seqnum,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    
    // To FIFO interface of Buffer Pool
    input [35:0] data_i,
    input src_rdy_i,
    output dst_rdy_o,
    
    output [5+64+16+(32*MAXCHAN)-1:0] sample_fifo_o,
    output sample_fifo_src_rdy_o,
    input sample_fifo_dst_rdy_i,

    output [31:0] current_seqnum,
    
    // FIFO Levels
    output [15:0] fifo_occupied,
    output fifo_full,
    output fifo_empty,
    output [31:0] debug
    );

   localparam FIFOWIDTH = 5+64+16+(32*MAXCHAN);
   
   wire [1:0] numchan = 0;/*
   setting_reg #(.my_addr(BASE), .at_reset(0), .width(2)) sr_numchan
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(numchan),.changed());
   */

   reg [3:0] vita_state;
   wire      has_streamid, has_classid, has_secs, has_tics, has_trailer;
   assign has_streamid 	= (data_i[31:28]==4'b0001);
   assign has_classid 	= data_i[27];
   assign has_secs 	= ~(data_i[23:22]==2'b00);
   assign has_tics 	= ~(data_i[21:20]==2'b00);
   assign has_trailer 	= data_i[26];
   wire      is_sob = data_i[25];
   wire      is_eob = data_i[24];
   wire      eof = data_i[33];
   reg 	     has_streamid_reg, has_classid_reg, has_secs_reg, has_tics_reg;
   reg 	     has_trailer_reg, is_sob_reg, is_eob_reg;
   
   reg [15:0] pkt_len;
   reg [1:0]  vector_phase;
   wire       line_done;
   
   wire [31:0] seqnum = data_i;
   reg [31:0]  seqnum_reg;
   wire [31:0] next_seqnum = seqnum_reg + 32'd1;
   wire [3:0]  vita_seqnum = data_i[19:16];
   reg [3:0]   vita_seqnum_reg;
   wire [3:0]  next_vita_seqnum = vita_seqnum_reg[3:0] + 4'd1;
   reg 	       seqnum_err;

   assign current_seqnum = seqnum_reg;
   
   // Output FIFO for packetized data
   localparam VITA_TRANS_HEADER  = 0;
   localparam VITA_HEADER 	 = 1;
   localparam VITA_STREAMID 	 = 2;
   localparam VITA_CLASSID 	 = 3;
   localparam VITA_CLASSID2 	 = 4;
   localparam VITA_SECS 	 = 5;
   localparam VITA_TICS 	 = 6;
   localparam VITA_TICS2 	 = 7;
   localparam VITA_PAYLOAD 	 = 8;
   localparam VITA_TRAILER 	 = 10;
   localparam VITA_DUMP          = 11;
   
   wire [15:0] hdr_len = 2 + has_streamid_reg + has_classid_reg + has_classid_reg + has_secs_reg + 
	       has_tics_reg + has_tics_reg + has_trailer_reg;

   wire        vita_eof = (pkt_len==hdr_len);
   wire        eop = eof | vita_eof;  // FIXME would ignoring eof allow larger VITA packets?
   wire        fifo_space;

   always @(posedge clk)
     if(reset | clear | clear_seqnum)
       begin
	  seqnum_reg <= 32'hFFFF_FFFF;
	  vita_seqnum_reg <= 4'hF;
       end
     else
       begin
	  if((vita_state==VITA_TRANS_HEADER) & src_rdy_i)
	    seqnum_reg <= seqnum;
	  if((vita_state==VITA_HEADER) & src_rdy_i)
	    vita_seqnum_reg <= vita_seqnum;
       end // else: !if(reset | clear_seqnum)
   
   always @(posedge clk)
     if(reset | clear)
       begin
	  vita_state 		<= (USE_TRANS_HEADER==1) ? VITA_TRANS_HEADER : VITA_HEADER;
	  {has_streamid_reg, has_classid_reg, has_secs_reg, has_tics_reg, has_trailer_reg, is_sob_reg, is_eob_reg} 
	    <= 0;
	  seqnum_err <= 0;
       end
     else if(src_rdy_i & dst_rdy_o) begin //valid read
	 case(vita_state)
	   VITA_TRANS_HEADER :
	     begin
		seqnum_err <= ~(seqnum == next_seqnum);
		vita_state <= VITA_HEADER;
	     end
	   VITA_HEADER :
	     begin
		{has_streamid_reg, has_classid_reg, has_secs_reg, has_tics_reg, has_trailer_reg, is_sob_reg, is_eob_reg} 
 		  <= {has_streamid, has_classid, has_secs, has_tics, has_trailer, is_sob, is_eob};
		pkt_len <= data_i[15:0];
		vector_phase <= 0;
		if(has_streamid)
		  vita_state <= VITA_STREAMID;
		else if(has_classid)
		  vita_state <= VITA_CLASSID;
		else if(has_secs)
		  vita_state <= VITA_SECS;
		else if(has_tics)
		  vita_state <= VITA_TICS;
		else
		  vita_state <= VITA_PAYLOAD;
		seqnum_err <= seqnum_err | ~(vita_seqnum == next_vita_seqnum);
	     end // case: VITA_HEADER
	   VITA_STREAMID :
	     if(has_classid_reg)
	       vita_state <= VITA_CLASSID;
	     else if(has_secs_reg)
	       vita_state <= VITA_SECS;
	     else if(has_tics_reg)
	       vita_state <= VITA_TICS;
	     else
	       vita_state <= VITA_PAYLOAD;
	   VITA_CLASSID :
	     vita_state <= VITA_CLASSID2;
	   VITA_CLASSID2 :
	     if(has_secs_reg)
	       vita_state <= VITA_SECS;
	     else if(has_tics_reg)
	       vita_state <= VITA_TICS;
	     else
	       vita_state <= VITA_PAYLOAD;
	   VITA_SECS :
	     if(has_tics_reg)
	       vita_state <= VITA_TICS;
	     else
	       vita_state <= VITA_PAYLOAD;
	   VITA_TICS :
	     vita_state <= VITA_TICS2;
	   VITA_TICS2 :
	     vita_state <= VITA_PAYLOAD;

        VITA_PAYLOAD : begin

            //step through each element until line done, then reset
            vector_phase <= (line_done)? 0: vector_phase + 1;

            //decrement the packet count after each line
            pkt_len <= (line_done)? pkt_len - 1 : pkt_len;

            //end of frame reached, determine next state
            //otherwise, keep processing through the payload
            if (line_done && vita_eof) begin

                if (eof) begin
                    vita_state <= (USE_TRANS_HEADER==1) ? VITA_TRANS_HEADER : VITA_HEADER;
                end
                else if (has_trailer_reg) begin
                    vita_state <= VITA_TRAILER;
                end
                else begin
                    vita_state <= VITA_DUMP;
                end

            end //line_done && vita_eof

        end //end VITA_PAYLOAD

	   VITA_TRAILER :
	     if(eof)
	       vita_state <= (USE_TRANS_HEADER==1) ? VITA_TRANS_HEADER : VITA_HEADER;
	     else
	       vita_state <= VITA_DUMP;
	   VITA_DUMP :
	     if(eof)
	       vita_state <= (USE_TRANS_HEADER==1) ? VITA_TRANS_HEADER : VITA_HEADER;
	   default :
	     vita_state <= (USE_TRANS_HEADER==1) ? VITA_TRANS_HEADER : VITA_HEADER;
	 endcase // case (vita_state)

     end //valid read

   assign line_done = (MAXCHAN == 1)? 1 : (vector_phase == numchan);
   
   wire [FIFOWIDTH-1:0] fifo_i;
   reg [63:0] 		      send_time;
   
   always @(posedge clk)
     case(vita_state)
       VITA_TICS :
	 send_time[63:32] <= data_i[31:0];
       VITA_TICS2 :
	 send_time[31:0] <= data_i[31:0];
     endcase // case (vita_state)

   //sample registers for de-framing a vector input
   reg [31:0] sample_reg [1:0];
   always @(posedge clk)
     if(src_rdy_i && dst_rdy_o)
        sample_reg[vector_phase] <= data_i[31:0];

   wire store = (vita_state == VITA_PAYLOAD)? (src_rdy_i && line_done) : 0;
   assign dst_rdy_o = (vita_state == VITA_PAYLOAD)? fifo_space : 1;

   fifo_short #(.WIDTH(FIFOWIDTH)) short_tx_q
     (.clk(clk), .reset(reset), .clear(clear),
      .datain(fifo_i), .src_rdy_i(store), .dst_rdy_o(fifo_space),
      .dataout(sample_fifo_o), .src_rdy_o(sample_fifo_src_rdy_o), .dst_rdy_i(sample_fifo_dst_rdy_i) );

    //assign registered/live data to the samples vector
    //the numchan'th sample vector is muxed to live data
    wire [(32*MAXCHAN)-1:0] samples;
    generate
    genvar i;
    for (i=0; i < MAXCHAN; i = i +1) begin : assign_samples
        wire live_data = (i == (MAXCHAN-1))? 1 : numchan == i;
        assign samples[32*i + 31:32*i] = (live_data)? data_i[31:0] : sample_reg[i];
    end
    endgenerate

   // sob, eob, has_tics (send_at) ignored on all lines except first
   assign fifo_i = {samples,seqnum_err,has_tics_reg,is_sob_reg,is_eob_reg,eop,
		    12'd0,seqnum_reg[3:0],send_time};

   assign debug = { { 8'b0 },
		    { 8'b0 },
		    { eof, line_done, store, fifo_space, src_rdy_i, dst_rdy_o, vector_phase[1:0] },
		    { has_secs_reg, is_sob_reg, is_eob_reg, eop, vita_state[3:0] } };
   
endmodule // vita_tx_deframer
