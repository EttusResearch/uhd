
`timescale 1ns/1ns
module rx_frontend_tb();
   
   reg clk, rst;

   initial rst = 1;
   initial #1000 rst = 0;
   initial clk = 0;
   always #5 clk = ~clk;
   
   initial $dumpfile("rx_frontend_tb.vcd");
   initial $dumpvars(0,rx_frontend_tb);

   reg [15:0] adc_in;
   wire [17:0] adc_out;

   always @(posedge clk)
     begin
	if(adc_in[13])
	  $write("-%d,",-adc_in);
	else
	  $write("%d,",adc_in);
	if(adc_out[13])
	  $write("-%d\n",-adc_out);
	else
	  $write("%d\n",adc_out);
     end	
   
   rx_frontend #(.BASE(0)) rx_frontend
     (.clk(clk),.rst(rst),
      .set_stb(0),.set_addr(0),.set_data(0),
      .adc_a(adc_in), .adc_ovf_a(0),
      .adc_b(0), .adc_ovf_b(0),
      .i_out(adc_out),.q_out(),
      .run(), .debug());

   always @(posedge clk)
     if(rst)
       adc_in <= 0;
     else
       adc_in <= adc_in + 4;
   //adc_in <= (($random % 473) + 23)/4;
   
endmodule // rx_frontend_tb
