

module packet_generator
  (input clk, input reset, input clear,
   output reg [7:0] data_o, output sof_o, output eof_o, 
   output src_rdy_o, input dst_rdy_i);

   localparam len = 32'd100;

   reg [31:0] state;
   reg [31:0] seq;
   wire [31:0] crc_out;
   wire        calc_crc = src_rdy_o & dst_rdy_i & ~(state[31:2] == 30'h3FFF_FFFF);
   
	
   always @(posedge clk)
     if(reset | clear)
       seq <= 0;
     else
       if(eof_o & src_rdy_o & dst_rdy_i)
	 seq <= seq + 1;
   
   always @(posedge clk)
     if(reset | clear)
       state <= 0;
     else
       if(src_rdy_o & dst_rdy_i)
	 if(state == (len - 1))
	   state <= 32'hFFFF_FFFC;
	 else
	   state <= state + 1;

   always @*
     case(state)
       0 :   data_o <= len[7:0];
       1 :   data_o <= len[15:8];
       2 :   data_o <= len[23:16];
       3 :   data_o <= len[31:24];
       4 :   data_o <= seq[7:0];
       5 :   data_o <= seq[15:8];
       6 :   data_o <= seq[23:16];
       7 :   data_o <= seq[31:24];
       32'hFFFF_FFFC : data_o <= crc_out[31:24];
       32'hFFFF_FFFD : data_o <= crc_out[23:16];
       32'hFFFF_FFFE : data_o <= crc_out[15:8];
       32'hFFFF_FFFF : data_o <= crc_out[7:0];
       default : data_o <= state[7:0];
     endcase // case (state)
   
   assign src_rdy_o = 1;
   assign sof_o = (state == 0);
   assign eof_o = (state == 32'hFFFF_FFFF);

   wire        clear_crc = eof_o & src_rdy_o & dst_rdy_i;
   
   crc crc(.clk(clk), .reset(reset), .clear(clear_crc), .data(data_o), 
	   .calc(calc_crc), .crc_out(crc_out), .match());
   
endmodule // packet_generator
