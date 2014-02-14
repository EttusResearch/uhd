//
// Copyright 2011 Ettus Research LLC
//




module capture_ddrlvds
  #(parameter WIDTH=7,
    parameter B250=0)
   (input clk,
    input reset,
    input ssclk_p,
    input ssclk_n,
    input [WIDTH-1:0] in_p,
    input [WIDTH-1:0] in_n,
    output [(2*WIDTH)-1:0] out);

   wire [WIDTH-1:0] 	   ddr_dat;
   wire 		   ssclk;
   wire [(2*WIDTH)-1:0]    out_pre1;
   wire 		   ssclk_bufio1, ssclk_bufio2, ssclk_bufr, ssclk_bufmr;
   
   IBUFGDS #(.DIFF_TERM("TRUE")) 
   clkbuf (.O(ssclk), .I(ssclk_p), .IB(ssclk_n));

   BUFMR clkbufmr (
		   .I(ssclk),
		   .O(ssclk_bufmr)
		   );
  
   BUFIO clkbufio1 (
		    .I(ssclk_bufmr),
		    .O(ssclk_bufio1)
		   );
   BUFIO clkbufio2 (
		    .I(ssclk_bufmr),
		    .O(ssclk_bufio2)
		    );
  
   BUFR     
     #(.SIM_DEVICE("7SERIES"),
       .BUFR_DIVIDE("BYPASS"))
   clkbufr (
	    .I(ssclk_bufmr),
	    .O(ssclk_bufr)
	    );
   

   genvar 		   i;
   
   generate
      for(i = 0; i < WIDTH; i = i + 1)
	begin : gen_lvds_pins
	   if ((i == 10) && (B250 == 1)) begin
	      IBUFDS #(.DIFF_TERM("FALSE")) ibufds
		(.O(ddr_dat[i]), .I(in_p[i]), .IB(in_n[i]) );
	      IDDR #(.DDR_CLK_EDGE("SAME_EDGE_PIPELINED")) iddr
		(.Q1(out_pre1[2*i]), .Q2(out_pre1[(2*i)+1]), .C(ssclk_bufio2),
		 .CE(1'b1), .D(ddr_dat[i]), .R(1'b0), .S(1'b0));
	   end else begin
	      IBUFDS #(.DIFF_TERM("TRUE")) ibufds
		(.O(ddr_dat[i]), .I(in_p[i]), .IB(in_n[i]) );
	      IDDR #(.DDR_CLK_EDGE("SAME_EDGE_PIPELINED")) iddr
		(.Q1(out_pre1[2*i]), .Q2(out_pre1[(2*i)+1]), .C(ssclk_bufio1),
		 .CE(1'b1), .D(ddr_dat[i]), .R(1'b0), .S(1'b0));
	   end
	end
   endgenerate

   
   
   reg rd_en;
   wire full, empty, almost_empty;
   
   
   input_sample_fifo  input_sample_fifo_i
     (
      .rst(reset), // input rst
      .wr_clk(ssclk_bufr), // input wr_clk
      .rd_clk(clk), // input rd_clk
      .din(out_pre1), // input [27 : 0] din
      .wr_en(1'b1), // input wr_en
      .rd_en(rd_en), // input rd_en
      .dout(out), // output [27 : 0] dout
      .full(full), // output full
      .empty(empty), // output empty
      .almost_empty(almost_empty) // output almost_empty
      );

   
   always @(posedge clk) begin
      if (reset)
	rd_en <= 0;
      else if (~almost_empty)
	rd_en <= 1;
      else if (empty)
	rd_en <= 0;
   end

endmodule // capture_ddrlvds
