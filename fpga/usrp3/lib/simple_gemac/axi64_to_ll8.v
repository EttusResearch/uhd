

module axi64_to_ll8
  #(parameter START_BYTE=6)
   (input clk, input reset, input clear,
    input [63:0] axi64_tdata, input axi64_tlast, input [3:0] axi64_tuser, input axi64_tvalid, output axi64_tready,
    output [7:0] ll_data, output ll_eof, output ll_src_rdy, input ll_dst_rdy);
   
   reg [7:0] 	 data_int;
   wire 	 eof_int, valid_int, ready_int;
   
   reg [2:0] 	 state = START_BYTE;
   reg 		 eof, done;
   reg [3:0] 	 occ;
   
   always @(posedge clk)
     if(reset | clear)
       state <= START_BYTE;
     else
       if(valid_int & ready_int)
	 if(eof_int)
	   state <= START_BYTE;
	 else
	   state <= state + 3'd1;
   
   assign valid_int = axi64_tvalid;
   assign axi64_tready = ready_int & (eof_int | state == 7);
   assign eof_int = axi64_tlast & (axi64_tuser[2:0] == (state + 3'd1));
      
   always @*
     case(state)
       0 : data_int <= axi64_tdata[63:56];
       1 : data_int <= axi64_tdata[55:48];
       2 : data_int <= axi64_tdata[47:40];
       3 : data_int <= axi64_tdata[39:32];
       4 : data_int <= axi64_tdata[31:24];
       5 : data_int <= axi64_tdata[23:16];
       6 : data_int <= axi64_tdata[15:8];
       7 : data_int <= axi64_tdata[7:0];
       default : data_int <= axi64_tdata[7:0];
     endcase // case (state)
   
   axi_fifo_short #(.WIDTH(9)) ll8_fifo
     (.clk(clk), .reset(reset), .clear(0),
      .i_tdata({eof_int, data_int}), .i_tvalid(valid_int), .i_tready(ready_int),
      .o_tdata({ll_eof, ll_data}), .o_tvalid(ll_src_rdy), .o_tready(ll_dst_rdy),
      .space(), .occupied());
   		  
endmodule // axi64_to_ll8
