
module round_sd_tb();
   
   reg clk, rst;

   initial rst = 1;
   initial #1000 rst = 0;
   initial clk = 0;
   always #5 clk = ~clk;
   
   initial $dumpfile("round_sd_tb.vcd");
   initial $dumpvars(0,round_sd_tb);

   localparam WIDTH_IN = 14;
   localparam WIDTH_OUT = 10;
   
   reg [WIDTH_IN-1:0] adc_in, adc_in_del;
   wire [WIDTH_OUT-1:0] adc_out;

   integer 		factor = 1<<(WIDTH_IN-WIDTH_OUT);
   
   always @(posedge clk)
     if(~rst)
	begin  
	   if(adc_in_del[WIDTH_IN-1])
	     $write("-%d\t",-adc_in_del);
	   else
	     $write("%d\t",adc_in_del);
	   if(adc_out[WIDTH_OUT-1])
	     $write("-%d\t",-adc_out);
	   else
	     $write("%d\t",adc_out);
	   $write("%f\t",adc_in_del/factor);
	   $write("%f\n",adc_in_del/factor-adc_out);
	end	
   
   round_sd #(.WIDTH_IN(WIDTH_IN),.WIDTH_OUT(WIDTH_OUT)) 
   round_sd(.clk(clk),.reset(rst), .in(adc_in),.out(adc_out));

   always @(posedge clk)
     adc_in <= 4734;
     //adc_in <= $random % 4739;

   always @(posedge clk)
     adc_in_del <= adc_in;

   initial #10000 $finish;
   
endmodule // longfifo_tb
