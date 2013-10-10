`timescale 1ns/1ps

module catgen_tb();

   wire GSR, GTS;
   glbl glbl( );

   reg 	clk    = 0;
   reg 	reset  = 1;
   wire ddrclk;
   
   always #100 clk = ~clk;

   initial $dumpfile("catgen_tb.vcd");
   initial $dumpvars(0,catgen_tb);

   wire [11:0] pins;
   wire        frame;
   
   reg 	       mimo;
   reg [7:0]   count;
   reg 	       tx_strobe;
   
   wire [11:0] i0 = {4'hA,count};
   wire [11:0] q0 = {4'hB,count};
   wire [11:0] i1 = {4'hC,count};
   wire [11:0] q1 = {4'hD,count};

   initial
     begin
	#1000 reset = 0;
	BURST(4);
	BURST(5);
	MIMO_BURST(4);
	MIMO_BURST(5);
	#2000;
	$finish;
     end
   
   task BURST;
      input [7:0] len;

      begin
	 tx_strobe <= 0;
	 mimo <= 0;
	 count <= 0;
	 @(posedge clk);
	 @(posedge clk);
	 repeat(len)
	   begin
	      tx_strobe <= 1;
	      @(posedge clk);
	      count <= count + 1;
	   end
	 tx_strobe <= 0;
	 @(posedge clk);
	 @(posedge clk);
	 @(posedge clk);
      end
   endtask // BURST
   
   task MIMO_BURST;
      input [7:0] len;

      begin
	 tx_strobe <= 0;
	 mimo <= 1;
	 count <= 0;
	 @(posedge clk);
	 @(posedge clk);
	 repeat(len)
	   begin
	      tx_strobe <= 1;
	      @(posedge clk);
	      tx_strobe <= 0;
	      @(posedge clk);
	      count <= count + 1;
	   end
	 tx_strobe <= 0;
	 @(posedge clk);
	 @(posedge clk);
	 @(posedge clk);
      end
   endtask // BURST
   
   catgen_ddr_cmos catgen
     (.data_clk(ddrclk),
      .reset(reset),
      .mimo(mimo),
      .tx_frame(frame),
      .tx_d(pins),
      .tx_clk(clk),
      .tx_strobe(tx_strobe),
      .i0(i0),.q0(q0),
      .i1(i1),.q1(q1));
   
endmodule // hb_chain_tb
