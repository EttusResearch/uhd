`timescale 1ns/1ps

module bus_int_tb();

   wire GSR, GTS;
   xlnx_glbl glbl( );

   reg clk    = 0;
   reg reset  = 1;
   
   always #10 clk = ~clk;
   
   initial $dumpfile("bus_int_tb.vcd");
   initial $dumpvars(0,bus_int_tb);

   initial
     begin
	#1000 reset = 0;
	#2000000;
	$finish;
     end
   
   wire sen, sclk, mosi, miso;
   wire scl, sda;
   
   bus_int bus_int
     (.clk(clk), .reset(reset), 
      .sen(sen), .sclk(sclk), .mosi(mosi), .miso(miso),
      .scl(scl), .sda(sda)
      );
   
endmodule // bus_int_tb
