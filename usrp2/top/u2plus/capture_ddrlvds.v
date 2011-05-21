

module capture_ddrlvds
  #(parameter WIDTH=7)
   (input clk,
    input ssclk_p,
    input ssclk_n,
    input [WIDTH-1:0] in_p,
    input [WIDTH-1:0] in_n,
    output reg [(2*WIDTH)-1:0] out);

   wire [WIDTH-1:0] 	   ddr_dat;
   wire 		   ssclk;
   wire [(2*WIDTH)-1:0]    out_pre1;
   reg [(2*WIDTH)-1:0] 	   out_pre2;
   
   IBUFGDS #(.IOSTANDARD("LVDS_33"), .DIFF_TERM("FALSE")) 
   clkbuf (.O(ssclk), .I(ssclk_p), .IB(ssclk_n));
   
   genvar 	       i;
   generate
      for(i = 0; i < WIDTH; i = i + 1)
	begin : gen_lvds_pins
	   IBUFDS #(.IOSTANDARD("LVDS_33"),.DIFF_TERM("FALSE")) ibufds 
	      (.O(ddr_dat[i]), .I(in_p[i]), .IB(in_n[i]) );
	   IDDR2 #(.DDR_ALIGNMENT("C1")) iddr2
	     (.Q0(out_pre1[2*i]), .Q1(out_pre1[(2*i)+1]), .C0(ssclk), .C1(~ssclk),
	      .CE(1'b1), .D(ddr_dat[i]), .R(1'b0), .S(1'b0));
	end
   endgenerate

   always @(negedge clk)
     out_pre2 <= out_pre1;

   always @(posedge clk)
     out      <= out_pre2;
   
endmodule // capture_ddrlvds
