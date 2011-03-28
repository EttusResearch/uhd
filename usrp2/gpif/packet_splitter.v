
// Split vita packets longer than one GPIF frame, add padding on short frames

module packet_splitter
  #(parameter FRAME_LEN=256)
   (input clk, input reset, input clear,
    input [18:0] data_i,
    input src_rdy_i,
    output dst_rdy_o,
    output [18:0] data_o,
    output src_rdy_o,
    input dst_rdy_i);
   
   reg [1:0] state;
   reg [15:0] length;
   reg [15:0] frame_len;
   
   localparam PS_IDLE = 0;
   localparam PS_FRAME = 1;
   localparam PS_NEW_FRAME = 2;
   localparam PS_PAD = 3;
   
   always @(posedge clk)
     if(reset | clear)
       state <= PS_IDLE;
     else
       case(state)
	 PS_IDLE :
	   if(src_rdy_i & dst_rdy_i)
	     begin
		length <= { data_i[14:0],1'b0};
		frame_len <= FRAME_LEN;
		state <= PS_FRAME;
	     end
	 PS_FRAME :
	   if(src_rdy_i & dst_rdy_i)
	     if(frame_len == 0)
	       if(length == 0)
		 state <= PS_IDLE;
	       else
		 begin
		    state <= PS_NEW_FRAME;
		    frame_len <= FRAME_LEN;
		    length <= length - 1;
		 end
	     else
	       if(length == 0)
		 begin
		    frame_len <= frame_len - 1;
		    state <= PS_PAD;
		 end
	 PS_NEW_FRAME :
	   if(src_rdy_i & dst_rdy_i)
	     begin
		frame_len <= frame_len - 1;
		state <= PS_FRAME;
	     end
	 PS_PAD :
	   if(dst_rdy_i)
	     if(frame_len == 0)
	       state <= PS_IDLE;
	     else
	       frame_len <= frame_len - 1;

       endcase // case (state)
   
	
   
   assign dst_rdy_o = dst_rdy_i & (state != PS_PAD);
   assign src_rdy_o = src_rdy_i | (state == PS_PAD);
   
   wire occ_out = 0;
   wire eof_out = (frame_len == 0) & (state != PS_IDLE);
   wire sof_out = (state == PS_IDLE) | (state == PS_NEW_FRAME);

   wire [15:0] data_out = data_i[15:0];
   assign data_o = {occ_out, eof_out, sof_out, data_out};
   
endmodule // packet_splitter
