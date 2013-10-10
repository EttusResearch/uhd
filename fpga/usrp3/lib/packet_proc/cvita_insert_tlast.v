
// Insert tlast bit for fifos that don't support it.  This only works with VALID CVITA frames
//  A single partial or invalid frame will make this wrong FOREVER

module cvita_insert_tlast
  (input clk, input reset, input clear,
   input [63:0] i_tdata, input i_tvalid, output i_tready,
   output [63:0] o_tdata, output o_tlast, output o_tvalid, input o_tready);

   assign o_tdata = i_tdata;
   assign o_tvalid = i_tvalid;
   assign i_tready = o_tready;

   wire [15:0] cvita_len_ceil = i_tdata[47:32] + 7;
   wire [15:0] axi_len = {3'b000, cvita_len_ceil[15:3]};
   
   reg [15:0] 	 count;

   assign o_tlast = (count != 0) ? (count == 16'd1) : (axi_len == 16'd1);

   always @(posedge clk)
     if(reset | clear)
       begin
	  count <= 16'd0;
       end
     else
       if(i_tready & i_tvalid)
	 if(count != 16'd0)
	   count <= count - 16'd1;
	 else
	   count <= axi_len - 16'd1;
   
endmodule // cvita_insert_tlast
