
module ethtx_realign
   (input clk, input reset, input clear,
    input [35:0] datain, input src_rdy_i, output dst_rdy_o,
    output [35:0] dataout, output src_rdy_o, input dst_rdy_i);

   reg 		  state;

   wire 	  eof_in = datain[33];
   wire [1:0] 	  occ_in = datain[35:34];
   
   always @(posedge clk)
     if(reset | clear)
       state <= 0;
     else if
       
   assign dataout[15:0] = datain[31:16];
   assign dataout[31:16] = stored;

   always @(posedge clk)
     stored <= datain[15:0];


endmodule // ethtx_realign
