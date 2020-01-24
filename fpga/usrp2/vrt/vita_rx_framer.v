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


module vita_rx_framer
  #(parameter BASE=0,
    parameter MAXCHAN=1)
   (input clk, input reset, input clear,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,
    
    // To FIFO interface of Buffer Pool
    output [35:0] data_o,
    input dst_rdy_i,
    output src_rdy_o,
    
    // From vita_rx_control
    input [5+64+(32*MAXCHAN)-1:0] sample_fifo_i,
    input sample_fifo_src_rdy_i,
    output sample_fifo_dst_rdy_o,
    
    output [31:0] debug_rx
    );

   localparam SAMP_WIDTH  = 5+64+(32*MAXCHAN);
   reg [3:0] 	  sample_phase;
   wire [3:0] 	  numchan;
   wire [4:0] 	  flags_fifo_o = sample_fifo_i[SAMP_WIDTH-1:SAMP_WIDTH-5];
   wire [63:0] 	  vita_time_fifo_o = sample_fifo_i[SAMP_WIDTH-6:SAMP_WIDTH-69];

   reg [31:0] 	  data_fifo_o;

   // The tools won't synthesize properly without this kludge because of the variable
   // parameter length
   
   wire [127:0]   FIXED_WIDTH_KLUDGE = sample_fifo_i;
   always @*
     case(sample_phase)
       4'd0 : data_fifo_o = FIXED_WIDTH_KLUDGE[31:0];
       4'd1 : data_fifo_o = FIXED_WIDTH_KLUDGE[63:32];
       4'd2 : data_fifo_o = FIXED_WIDTH_KLUDGE[95:64];
       4'd3 : data_fifo_o = FIXED_WIDTH_KLUDGE[127:96];
       default : data_fifo_o = 32'hDEADBEEF;
     endcase // case (sample_phase)
   
   wire 	  clear_pkt_count, pkt_fifo_rdy, sample_fifo_in_rdy;
   
   wire [31:0] 	  vita_header, vita_streamid, vita_trailer;
   wire [15:0] 	  samples_per_packet;
   
   reg [33:0] 	  pkt_fifo_line;
   reg [3:0] 	  vita_state;
   reg [15:0] 	  sample_ctr;
   reg [3:0] 	  pkt_count;
   
   wire [15:0] 	  vita_pkt_len = samples_per_packet + 6;
   //wire [4:0] flags = {signal_zerolen,signal_overrun,signal_brokenchain,signal_latecmd,signal_cmd_done};

   setting_reg #(.my_addr(BASE+4)) sr_header
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(vita_header),.changed());

   setting_reg #(.my_addr(BASE+5)) sr_streamid
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(vita_streamid),.changed(clear_pkt_count));

   setting_reg #(.my_addr(BASE+6)) sr_trailer
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(vita_trailer),.changed());

   setting_reg #(.my_addr(BASE+7),.width(16)) sr_samples_per_pkt
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(samples_per_packet),.changed());

   assign numchan = 0;/*
   setting_reg #(.my_addr(BASE+8),.width(4), .at_reset(0)) sr_numchan
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(numchan),.changed());
   */

   // Output FIFO for packetized data
   localparam VITA_IDLE 	 = 0;
   localparam VITA_HEADER 	 = 1;
   localparam VITA_STREAMID 	 = 2;
   localparam VITA_TICS 	 = 3;
   localparam VITA_TICS2 	 = 4;
   localparam VITA_PAYLOAD 	 = 5;
   localparam VITA_TRAILER 	 = 6;
   localparam VITA_ERR_HEADER 	 = 7;  // All ERR at 4'b1000 or'ed with base
   localparam VITA_ERR_STREAMID  = 8;
   localparam VITA_ERR_TICS 	 = 9;
   localparam VITA_ERR_TICS2 	 = 10;
   localparam VITA_ERR_PAYLOAD 	 = 11;
   localparam VITA_ERR_TRAILER 	 = 12; // Extension context packets have no trailer
      
   always @(posedge clk)
     if(reset | clear | clear_pkt_count)
       pkt_count <= 0;
     else if((vita_state == VITA_TRAILER) & pkt_fifo_rdy)
       pkt_count <= pkt_count + 1;

   wire 	  has_streamid = vita_header[28];
   wire 	  has_trailer = vita_header[26];
   reg 		  trl_eob;
   
   always @*
     case(vita_state)
       // Data packets are IF Data packets with or w/o streamid, no classid, with trailer
       VITA_HEADER : pkt_fifo_line <= {2'b01,3'b000,vita_header[28],2'b01,vita_header[25:24],
				       vita_header[23:20],pkt_count[3:0],vita_pkt_len[15:0]};
       VITA_STREAMID : pkt_fifo_line <= {2'b00,vita_streamid};
       VITA_TICS : pkt_fifo_line <= {2'b00,vita_time_fifo_o[63:32]};
       VITA_TICS2 : pkt_fifo_line <= {2'b00,vita_time_fifo_o[31:0]};
       VITA_PAYLOAD : pkt_fifo_line <= {2'b00,data_fifo_o};
       VITA_TRAILER : pkt_fifo_line <= {2'b10,vita_trailer[31:21],1'b1,vita_trailer[19:9],trl_eob,8'd0};

       // Error packets are Extension Context packets, which have no trailer
       VITA_ERR_HEADER : pkt_fifo_line <= {2'b01,4'b0101,4'b0000,vita_header[23:20],pkt_count,16'd5};
       VITA_ERR_STREAMID : pkt_fifo_line <= {2'b00,vita_streamid};
       VITA_ERR_TICS : pkt_fifo_line <= {2'b00,vita_time_fifo_o[63:32]};
       VITA_ERR_TICS2 : pkt_fifo_line <= {2'b00,vita_time_fifo_o[31:0]};
       VITA_ERR_PAYLOAD : pkt_fifo_line <= {2'b10,27'd0,flags_fifo_o};
       //VITA_ERR_TRAILER : pkt_fifo_line <= {2'b11,vita_trailer};
       
       default : pkt_fifo_line <= 34'h0_FFFF_FFFF;
       endcase // case (vita_state)

   always @(posedge clk)
     if(reset | clear)
       begin
	  vita_state   <= VITA_IDLE;
	  sample_ctr   <= 0;
	  sample_phase <= 0;
       end
     else
       if(vita_state==VITA_IDLE)
	 begin
	    sample_ctr <= 1;
	    sample_phase <= 0;
	    if(sample_fifo_src_rdy_i)
	      if(|flags_fifo_o[4:1])
		vita_state <= VITA_ERR_HEADER;
	      else
		vita_state <= VITA_HEADER;
	 end
       else if(pkt_fifo_rdy)
	 case(vita_state)
	   VITA_HEADER :
	     if(has_streamid)
	       vita_state <= VITA_STREAMID;
	     else
	       vita_state <= VITA_TICS;
	   VITA_PAYLOAD :
	     if(sample_fifo_src_rdy_i)
	       begin
		  if(sample_phase == numchan)
		    begin
		       sample_phase <= 0;
		       sample_ctr   <= sample_ctr + 1;
		       trl_eob <= flags_fifo_o[0];
		       if(sample_ctr == samples_per_packet)
			 vita_state <= VITA_TRAILER;
		       if(|flags_fifo_o)   // end early if any flag is set
			 vita_state <= VITA_TRAILER;
		    end
		  else
		    sample_phase <= sample_phase + 1;
	       end // if (sample_fifo_src_rdy_i)
	   VITA_ERR_PAYLOAD :
	     vita_state <= VITA_IDLE;
	   VITA_TRAILER :
	     vita_state <= VITA_IDLE;
	   default :
	     vita_state 	   <= vita_state + 1;
	 endcase // case (vita_state)

   reg req_write_pkt_fifo;
   always @*
     case(vita_state)
       VITA_IDLE :
	 req_write_pkt_fifo <= 0;
       VITA_HEADER, VITA_STREAMID, VITA_TICS, VITA_TICS2, VITA_TRAILER :
	 req_write_pkt_fifo <= 1;
       VITA_PAYLOAD :
	 // Write if sample ready and no error flags
     	 req_write_pkt_fifo <= (sample_fifo_src_rdy_i & ~|flags_fifo_o[4:1]);
       VITA_ERR_HEADER, VITA_ERR_STREAMID, VITA_ERR_TICS, VITA_ERR_TICS2, VITA_ERR_PAYLOAD :
	 req_write_pkt_fifo <= 1;
       default :
	 req_write_pkt_fifo <= 0;
     endcase // case (vita_state)
   
   //wire req_write_pkt_fifo  = (vita_state != VITA_IDLE) & (sample_fifo_src_rdy_i | (vita_state != VITA_PAYLOAD));
   
   // Short FIFO to buffer between us and the FIFOs outside
   fifo_short #(.WIDTH(34)) rx_pkt_fifo 
     (.clk(clk), .reset(reset), .clear(clear),
      .datain(pkt_fifo_line), .src_rdy_i(req_write_pkt_fifo), .dst_rdy_o(pkt_fifo_rdy),
      .dataout(data_o[33:0]), .src_rdy_o(src_rdy_o), .dst_rdy_i(dst_rdy_i),
      .space(),.occupied() );

   assign data_o[35:34] = 2'b00;  // Always write full lines
   assign sample_fifo_dst_rdy_o  = pkt_fifo_rdy & 
				   ( ((vita_state==VITA_PAYLOAD) & 
				      (sample_phase == numchan) & 
				      ~|flags_fifo_o[4:1]) |
				     (vita_state==VITA_ERR_PAYLOAD));
   
   assign debug_rx  = vita_state;
   
endmodule // vita_rx_framer
