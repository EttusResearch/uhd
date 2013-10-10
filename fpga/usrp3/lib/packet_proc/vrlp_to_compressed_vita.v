
module vrlp_to_compressed_vita
  (input clk, input reset, input clear,
   input [63:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
   output [63:0] o_tdata, output o_tlast, output o_tvalid, input o_tready);

   wire [63:0] 	 o_tdata_int;
   wire 	 o_tlast_int, o_tvalid_int, o_tready_int;

   reg [1:0] 	 v2cv_state;
   reg [11:0] 	 seqnum;
   reg 		 trim_line;
		 
   localparam V2CV_VRLP = 2'd0;
   localparam V2CV_VRTH = 2'd1;
   localparam V2CV_BODY = 2'd2;
   localparam V2CV_DUMP = 2'd3;

   wire 	 is_ec = i_tdata[63:60] == 4'h5;
   wire 	 has_trailer = i_tdata[58] & ~is_ec;
   wire 	 has_time = |i_tdata[53:52];
   wire 	 eob = i_tdata[56] & ~is_ec;
   wire [15:0] 	 len = i_tdata[47:32];
   wire [31:0] 	 sid = i_tdata[31:0];
   
   wire [63:0] 	 compressed_hdr = { is_ec, has_trailer, has_time, eob, seqnum, len, sid };
   wire 	 bad_vita = |i_tdata[55:54]  /* has secs */ | i_tdata[59] /* has class */ | ( {i_tdata[63],i_tdata[61:60]} != 3'b001 );
   reg [15:0] 	 len_reg;
   wire [16:0] vita_words32 = i_tdata[16:0]-17'd4;
   assign trim_now = 0;

   always @(posedge clk)
     if(reset | clear)
       begin
	  v2cv_state <= V2CV_VRLP;
	  seqnum <= 12'd0;
	  trim_line <= 1'b0;
	  len_reg <= 16'd0;
       end
     else
       case(v2cv_state)
	 V2CV_VRLP :
	   if(i_tvalid)
	     begin
		seqnum <= i_tdata[31:20];
		trim_line <= i_tdata[0];
		len_reg <= vita_words32[16:1];
		if(~i_tlast)
		  v2cv_state <= V2CV_VRTH;
	     end

	 V2CV_VRTH :
	   if(i_tvalid & o_tready_int)
	     begin
	     len_reg <= len_reg - 16'd1;
	     if(i_tlast)
	       v2cv_state <= V2CV_VRLP;
	     else if(bad_vita)
	       v2cv_state <= V2CV_DUMP;
	     else
	       v2cv_state <= V2CV_BODY;
	     end

	 V2CV_BODY :
	   if(i_tvalid & o_tready_int)
	     begin
		len_reg <= len_reg - 16'd1;
		if(i_tlast)
		  v2cv_state <= V2CV_VRLP;
		else if(len_reg == 16'd0)
		  v2cv_state <= V2CV_DUMP;
	     end
	 V2CV_DUMP :
	   if(i_tvalid)
	     if(i_tlast)
	       v2cv_state <= V2CV_VRLP;
       endcase // case (v2cv_state)

   assign o_tdata_int = (v2cv_state == V2CV_VRTH) ? compressed_hdr : i_tdata;
   assign o_tlast_int = i_tlast | (len_reg == 16'd0);
   assign o_tvalid_int = i_tvalid && (((v2cv_state == V2CV_VRTH) && !bad_vita) || (v2cv_state == V2CV_BODY));
   assign i_tready = o_tready_int | (v2cv_state == V2CV_VRLP) | (v2cv_state == V2CV_DUMP);

   axi_fifo_short #(.WIDTH(65)) short_fifo
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata({o_tlast_int,o_tdata_int}), .i_tvalid(o_tvalid_int), .i_tready(o_tready_int),
      .o_tdata({o_tlast,o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready),
      .space(), .occupied());

endmodule // vrlp_to_compressed_vita
