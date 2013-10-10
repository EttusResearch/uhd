`timescale 1ns/1ps

module catcap_tb();

   wire GSR, GTS;
   glbl glbl( );

   reg 	clk    = 0;
   reg 	ddrclk = 0;
   reg 	reset  = 1;

   always #100 clk = ~clk;
   always @(negedge clk) ddrclk <= ~ddrclk;

   initial $dumpfile("catcap_tb.vcd");
   initial $dumpvars(0,catcap_tb);

   wire [11:0] i0 = {4'hA,count};
   wire [11:0] q0 = {4'hB,count};
   wire [11:0] i1 = {4'hC,count};
   wire [11:0] q1 = {4'hD,count};

   reg 	       mimo;
   reg [11:0]  pins;
   reg 	       frame;
   reg [7:0]   count;
   
   initial
     begin
	#1000 reset = 0;
	MIMO_BURST(4);
	MIMO_BURST(5);
	BURST(4);
	BURST(5);
	#2000;
	$finish;
     end
   
   task BURST;
      input [7:0] len;
      begin
	 frame <= 0;
	 @(posedge clk);
	 @(posedge clk);
	 mimo <= 0;
	 @(posedge clk);
	 @(posedge clk);
	 @(posedge clk);
	 @(posedge ddrclk);
	 count <= 0;
	 repeat(len)
	   begin
	      @(posedge clk);
	      pins <= i0;
	      frame <= 1;
	      @(posedge clk);
	      pins <= q0;
	      frame <= 0;
	      count <= count + 1;
	   end
      end
   endtask // BURST
   
   task MIMO_BURST;
      input [7:0] len;
      begin
	 frame <= 0;
	 @(posedge clk);
	 @(posedge clk);
	 mimo <= 1;
	 @(posedge clk);
	 @(posedge clk);
	 @(posedge clk);
	 @(posedge ddrclk);
	 count <= 0;
	 repeat(len)
	   begin
	      @(posedge clk);
	      pins <= i0;
	      frame <= 1;
	      @(posedge clk);
	      pins <= q0;
	      @(posedge clk);
	      pins <= i1;
	      frame <= 0;
	      @(posedge clk);
	      pins <= q1;
	      count <= count + 1;
	   end
	 @(posedge clk);
	 @(posedge clk);
      end
   endtask // MIMO_BURST
      
   wire        rx_clk, rx_strobe;
   wire [11:0] i0o,i1o,q0o,q1o;
   
   catcap_ddr_cmos catcap
     (.data_clk(ddrclk),
      .reset(reset),
      .mimo(mimo),
      .rx_frame(frame),
      .rx_d(pins),
      .rx_clk(rx_clk),
      .rx_strobe(rx_strobe),
      .i0(i0o),.q0(q0o),
      .i1(i1o),.q1(q1o));
   
endmodule // hb_chain_tb
