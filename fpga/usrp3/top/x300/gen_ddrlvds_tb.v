`timescale 1ns/1ps

module gen_ddrlvds_tb();

   wire GSR, GTS;
   glbl glbl( );

   reg clk    = 1;
   reg reset  = 1;
   reg tx_strobe = 0;
   
   always #100 clk = ~clk;
   always #200 tx_strobe = ~tx_strobe;
   
   
   initial $dumpfile("gen_ddrlvds_tb.vcd");
   initial $dumpvars(0,gen_ddrlvds_tb);

   wire [7:0] pins_p, pins_n;
   wire       frame_p, frame_n;
   wire       clk_p, clk_n;
      
   reg [7:0]  count;
   
   wire [15:0] i = {4'hA,count};
   wire [15:0] q = {4'hB,count};

   initial
     begin
	#10000 reset = 0;
	BURST(4);
	BURST(5);
	#2000;
	$finish;
     end
   
   task BURST;
      input [7:0] len;

      begin
//	 tx_strobe <= 0;
	 count <= 0;
	 @(posedge clk);
	 @(posedge clk);
	 repeat(len)
	   begin
	  //    tx_strobe <= 1;
	      @(posedge clk);
	  //    tx_strobe <= 0;
	      @(posedge clk);
	      count <= count + 1;
	   end
//	 tx_strobe <= 0;
	 @(posedge clk);
	 @(posedge clk);
	 @(posedge clk);
      end
   endtask // BURST
   
   gen_ddrlvds gen_ddrlvds
     (.rst(reset),
      .tx_clk_p(clk_p), .tx_clk_n(clk_n),
      .tx_frame_p(frame_p), .tx_frame_n(frame_n),
      .tx_d_p(pins_p), .tx_d_n(pins_n),
      .tx_clk(clk), .tx_strobe(tx_strobe),
      .i(i), .q(q)
      );
   
      
endmodule // gen_ddrlvds_tb
