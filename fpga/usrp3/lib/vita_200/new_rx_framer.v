//
// Copyright 2014 Ettus Research LLC
// Copyright 2018 Ettus Research, a National Instruments Company
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

module new_rx_framer
  #(
    parameter BASE=0,
    parameter CHIPSCOPE=0,
    parameter SAMPLE_FIFO_SIZE=10
    )
   (input clk, input reset, input clear,
    input set_stb, input [7:0] set_addr, input [31:0] set_data,

    input [63:0] vita_time,

    input strobe,
    input [31:0] sample,
    input run,
    input eob,
    output full,
    output reg [11:0] seqnum,
    output [31:0] sid,

    output [63:0] o_tdata, output o_tlast, output o_tvalid, input o_tready,

    output [31:0] debug
    );

   reg [15:0] 	  len;
   reg [63:0] 	  hold_time;

   wire [63:0] 	  dfifo_tdata;
   wire 	  dfifo_tlast, dfifo_tvalid, dfifo_tready;

   wire [80:0] 	  hfifo_tdata;
   wire 	  hfifo_tvalid, hfifo_tready;

   wire [63:0] 	  o_tdata_int;
   wire 	  o_tlast_int, o_tvalid_int, o_tready_int;

   wire [15:0] 	  sample_space;

   wire [15:0] 	  maxlen;
   reg [31:0] 	  holding;


   // FIXME need to handle case where hdr fifo is full (i.e. too many tiny packets)
   assign full = (sample_space == 16'd0) | (sample_space == 16'd1) | ~hdr_tready;

   setting_reg #(.my_addr(BASE), .width(16)) sr_maxlen
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(maxlen),.changed());

   wire sid_changed;
   setting_reg #(.my_addr(BASE+1), .width(32)) sr_sid
     (.clk(clk),.rst(reset),.strobe(set_stb),.addr(set_addr),
      .in(set_data),.out(sid),.changed(sid_changed));

   localparam START = 0;
   localparam SECOND = 1;
   localparam FIRST = 2;

   reg [1:0] 	  instate;
   reg [15:0] 	  numsamps;
   reg 		  nearly_eop;


   always @(posedge clk)
     if(reset | clear)
       begin
	  instate <= START;
	  numsamps <= 0;
	  nearly_eop <= 0;

       end
     else if (run)
       case(instate)
	 //
	 // Start a new packet in this state
	 //
	 START :
	   if(strobe)
	     if(eop)
	       begin
		  instate <= START;
		  numsamps <= 0;
		  nearly_eop <= 0;
	       end
	     else
	       begin
		  instate <= SECOND;
		  numsamps <= numsamps + 1;
		  nearly_eop <= (numsamps >= (maxlen-2));
	       end // else: !if(eop)
	 //
	 // Second 32 bit sample in a 64bit word
	 //
	 SECOND :
	   if(strobe)
	     if(eop)
	       begin
		  instate <= START;
		  numsamps <= 0;
		  nearly_eop <= 0;
	       end
	     else
	       begin
		  instate <= FIRST;
		  numsamps <= numsamps + 1;
		  nearly_eop <= (numsamps >= (maxlen-2));
	       end // else: !if(eop)
	 //
	 // First 32bit sample in a 64bit word.
	 //
	 FIRST :
	   if(strobe)
	     if(eop)
	       begin
		  instate <= START;
		  numsamps <= 0;
		  nearly_eop <= 0;
	       end
	     else
	       begin
		  instate <= SECOND;
		  numsamps <= numsamps + 1;
		  nearly_eop <= (numsamps >= (maxlen-2));
	       end
       endcase // case (instate)
     else  begin
	  instate <= START;
	  numsamps <= 0;
	  nearly_eop <= 0;
     end


   always @(posedge clk)
     if(strobe && run)
       begin
	  holding <= sample;
	  if(instate == START)
	    hold_time <= vita_time;
       end

   always @(posedge clk)
     if(reset | clear)
       len <= 5;
     else
       if(strobe && run)
	 if(sample_tlast)
	   len <= 5;
	 else
	   len <= len + 1;

   always @(posedge clk)
     if(reset | clear | sid_changed)
       seqnum <= 12'd0;
     else
       if(o_tlast_int & o_tvalid_int & o_tready_int)
	 seqnum <= seqnum + 12'd1;



   wire 	  eop = eob | nearly_eop | full;

   wire [63:0] 	  sample_tdata = (instate == SECOND) ? {holding, sample} : {sample, 32'h0};
   wire 	  sample_tlast = eop;
   wire 	  sample_tvalid = run & strobe & ( (instate == SECOND) | eop );
   wire 	  sample_tready;

   wire [80:0] 	  hdr_tdata = {eob,len[13:0],2'b0,(instate == START) ? vita_time : hold_time};
   wire 	  hdr_tvalid = sample_tlast && sample_tvalid && sample_tready;
   wire 	  hdr_tready;

   wire [80:0] 	  hfifo_tdata_tmp;
   wire 	  hfifo_tvalid_tmp, hfifo_tready_tmp;



   axi_fifo #(.WIDTH(65), .SIZE(SAMPLE_FIFO_SIZE)) datafifo
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata({sample_tlast,sample_tdata}), .i_tvalid(sample_tvalid), .i_tready(sample_tready),
      .o_tdata({dfifo_tlast,dfifo_tdata}), .o_tvalid(dfifo_tvalid), .o_tready(dfifo_tready),
      .space(sample_space), .occupied());

   axi_fifo_short #(.WIDTH(81)) hdrfifo
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata(hdr_tdata), .i_tvalid(hdr_tvalid), .i_tready(hdr_tready),
      .o_tdata(hfifo_tdata_tmp), .o_tvalid(hfifo_tvalid_tmp), .o_tready(hfifo_tready_tmp),
      .space(), .occupied());

   axi_fifo_short #(.WIDTH(81)) hdrfifo2
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata(hfifo_tdata_tmp), .i_tvalid(hfifo_tvalid_tmp), .i_tready(hfifo_tready_tmp),
      .o_tdata(hfifo_tdata), .o_tvalid(hfifo_tvalid), .o_tready(hfifo_tready),
      .space(), .occupied());




   // The output state machine is responsible for forming output packets.
   // Output packets are formed by combining the entries in the header fifo,
   // and the samples in the data fifo. A single entry in the header fifo
   // contains both the compressed header and the 64 bit time stamp.

   reg [1:0] 	  outstate;
   localparam OUT_IDLE = 2'd0;
   localparam OUT_HEAD = 2'd1;
   localparam OUT_TIME = 2'd2;
   localparam OUT_BODY = 2'd3;

   always @(posedge clk)
     if(reset | clear)
       outstate <= OUT_IDLE;
     else
       case(outstate)
	 OUT_IDLE :
	   if(hfifo_tvalid) //having a header signals a complete packet
	     outstate <= OUT_HEAD;
	 OUT_HEAD :
	   if(o_tvalid_int && o_tready_int)
	     outstate <= OUT_TIME;
	 OUT_TIME :
	   if(o_tvalid_int && o_tready_int)
	     outstate <= OUT_BODY;
	 OUT_BODY :
	   if(o_tvalid_int && o_tready_int && o_tlast_int)
	     outstate <= OUT_IDLE;
       endcase // case (outstate)

   //output data mux feeds from single line of header fifo or the data fifo
   assign o_tdata_int = (outstate == OUT_HEAD) ? { 3'b001, hfifo_tdata[80], seqnum, hfifo_tdata[79:64], sid} :
			(outstate == OUT_TIME) ? hfifo_tdata[63:0] : dfifo_tdata;

   //output the last signal from the data fifo
   assign o_tlast_int = (outstate == OUT_BODY) ? dfifo_tlast : 1'b0;

   //output valid connected to data valid in non-IDLE states
   assign o_tvalid_int = (outstate != OUT_IDLE) & dfifo_tvalid;

   //only pop from header fifo on the very last transaction
   assign hfifo_tready = o_tvalid_int && o_tready_int && o_tlast_int;

   //connect data fifo ready with out ready in the BODY state
   assign dfifo_tready = (outstate == OUT_BODY) ? o_tready_int : 1'b0;

   axi_fifo_short #(.WIDTH(65)) output_fifo
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata({o_tlast_int, o_tdata_int}), .i_tvalid(o_tvalid_int), .i_tready(o_tready_int),
      .o_tdata({o_tlast, o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready),
      .space(), .occupied());
/* -----\/----- EXCLUDED -----\/-----

    assign debug[3:0] =  {instate, outstate};
    assign debug[7:4] =  {1'b0, sample_tlast, sample_tvalid, sample_tready};
    assign debug[11:8] =  {1'b0, 1'b0, hfifo_tvalid, hfifo_tready};
    assign debug[15:12] =  {1'b0, dfifo_tlast, dfifo_tvalid, dfifo_tready};
    assign debug[19:16] =  {1'b0, o_tlast_int, o_tvalid_int, o_tready_int};
 -----/\----- EXCLUDED -----/\----- */

   assign debug = {
		   sample_tlast, //15
		   sample_tvalid,//14
		   sample_tready,//13
		   dfifo_tvalid, //12
		   dfifo_tready, //11
		   hdr_tvalid,   //10
		   hdr_tready,   //9
		   hfifo_tvalid, //8
		   hfifo_tready, //7
		   eob,           //6
		   nearly_eop,    //5
		   full,          //4
		   outstate[1:0], //3:2
		   instate[1:0]   //1:0
		   };


endmodule // new_rx_framer
