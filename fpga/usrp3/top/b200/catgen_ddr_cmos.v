

module catgen_ddr_cmos
  (output data_clk,
   input reset,
   input mimo,
   output tx_frame,
   output [11:0] tx_d,
   input tx_clk, output reg tx_strobe,
   input [11:0] i0, input [11:0] q0,
   input [11:0] i1, input [11:0] q1);
   
   //IBUFGDS #(.IOSTANDARD("LVDS_33"), .DIFF_TERM("TRUE")) 
   //clkbuf (.O(ssclk), .I(ssclk_p), .IB(ssclk_n));

   reg [11:0] 	i,q;
   genvar 	z;
   reg 		tx_strobe_d;
   
   generate
      for(z = 0; z < 12; z = z + 1)
	begin : gen_pins
	   ODDR2 #(.DDR_ALIGNMENT("C0"),.SRTYPE("ASYNC")) oddr2
	     (.Q(tx_d[z]), .C0(tx_clk), .C1(~tx_clk),
	      .CE(1'b1), .D0(i[z]), .D1(q[z]), .R(1'b0), .S(1'b0));
	end
   endgenerate

   ODDR2 #(.DDR_ALIGNMENT("C0"),.SRTYPE("ASYNC")) oddr2_frame
     (.Q(tx_frame), .C0(tx_clk), .C1(~tx_clk),
      .CE(1'b1), .D0(tx_strobe_d), .D1(mimo&tx_strobe_d), .R(1'b0), .S(1'b0));

   ODDR2 #(.DDR_ALIGNMENT("C0"),.SRTYPE("ASYNC")) oddr2_clk
     (.Q(data_clk), .C0(tx_clk), .C1(~tx_clk),
      .CE(1'b1), .D0(1'b1), .D1(1'b0), .R(1'b0), .S(1'b0));

   always @(posedge tx_clk)
     tx_strobe <= (mimo)? ~tx_strobe : 1'b1;

   always @(posedge tx_clk)
     tx_strobe_d <= tx_strobe;
   
   always @(posedge tx_clk)
     if(tx_strobe)
       {i,q} <= {i0,q0};
     else
       {i,q} <= {i1,q1};
   
endmodule // catgen_ddr_cmos
