
module compressed_vita_to_vrlp
  (input clk, input reset, input clear,
   input [63:0] i_tdata, input i_tlast, input i_tvalid, output i_tready,
   output [63:0] o_tdata, output [15:0] o_tuser, output o_tlast, output o_tvalid, input o_tready
   );

   wire [19:0] 	 vrlp_size = 20'd3 + {4'b0000,i_tdata[47:32]};
   reg 		 odd_len;
   reg [2:0] 	 cv2v_state;

   reg [63:0] 	 o_tdata_int;
   wire 	 o_tlast_int, o_tvalid_int, o_tready_int;
   
   localparam CV2V_VRLP      = 3'd0; // VRLP header
   localparam CV2V_VRT_ECH   = 3'd1; // Extension context header
   localparam CV2V_VRT_IFH   = 3'd2; // IF Data header
   localparam CV2V_BODY      = 3'd3;
   localparam CV2V_VEND_ODD  = 3'd4;
   localparam CV2V_VEND_EVEN = 3'd4;
   
   always @(posedge clk)
     if(reset | clear)
       begin
	  cv2v_state <= CV2V_VRLP;
	  odd_len <= 1'b0;
       end
     else
       case(cv2v_state)
	 CV2V_VRLP :
	   if(i_tvalid & o_tready_int)
	     begin
		odd_len <= i_tdata[32];
		if(i_tdata[63])
		  cv2v_state <= CV2V_VRT_ECH;
		else
		  cv2v_state <= CV2V_VRT_IFH;
	     end

	 CV2V_VRT_ECH, CV2V_VRT_IFH :
	   if(i_tvalid & o_tready_int)
	     cv2v_state <= CV2V_BODY;

	 CV2V_BODY :
	   if(i_tlast & i_tvalid & o_tready_int)
	     if(odd_len)
	       cv2v_state <= CV2V_VRLP;
	     else
	       cv2v_state <= CV2V_VEND_EVEN;
	 
	 CV2V_VEND_EVEN :
	   if(o_tready_int)
	     cv2v_state <= CV2V_VRLP;
       endcase // case (cv2v_state)
   
   assign i_tready = o_tready_int & (cv2v_state != CV2V_VRLP) & (cv2v_state != CV2V_VEND_EVEN);
   assign o_tvalid_int = i_tvalid | (cv2v_state == CV2V_VEND_EVEN);

   always @*
     case(cv2v_state)
       CV2V_VRLP      : o_tdata_int <= { 32'h5652_4c50 /*VRLP*/, i_tdata[59:48] /*seqnum*/, vrlp_size[19:0] };
       CV2V_VRT_ECH   : o_tdata_int <= { 4'h5 /*type*/, 4'h0, 3'b00, i_tdata[61] /*time*/, i_tdata[51:48] /*seqnum*/, i_tdata[47:32] /*len*/, i_tdata[31:0] /*sid*/ };
       CV2V_VRT_IFH   : o_tdata_int <= { 4'h1 /*type*/, 1'b0, i_tdata[62] /*TRL*/, 1'b0, i_tdata[60] /*eob*/, 3'b00, i_tdata[61] /*time*/, i_tdata[51:48] /*seqnum*/, i_tdata[47:32] /*len*/, i_tdata[31:0] /*sid*/ };
       CV2V_BODY      : o_tdata_int <= (i_tlast & odd_len) ? { i_tdata[63:32], 32'h5645_4E44 /*VEND*/ } : i_tdata;
       CV2V_VEND_EVEN : o_tdata_int <= { 32'h5645_4E44 /*VEND*/, 32'h0};
       default : o_tdata_int <= i_tdata;
     endcase // case (cv2v_state)
   
   assign o_tlast_int = (cv2v_state == CV2V_VEND_EVEN) | ((cv2v_state == CV2V_BODY) & i_tlast & odd_len);

   // Short FIFO before output
   axi_fifo_short #(.WIDTH(81)) axi_fifo_short
     (.clk(clk), .reset(reset), .clear(clear),
      .i_tdata({o_tlast_int, i_tdata[15:0], o_tdata_int}), .i_tvalid(o_tvalid_int), .i_tready(o_tready_int),
      .o_tdata({o_tlast, o_tuser, o_tdata}), .o_tvalid(o_tvalid), .o_tready(o_tready),
      .space(), .occupied());
   
endmodule // compressed_vita_to_vrlp
